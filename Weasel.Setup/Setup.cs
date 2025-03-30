using Microsoft.Win32;
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using TSF.InteropTypes;
using TSF.TypeLib;

namespace Weasel.Setup
{
    public static class Setup
    {
        private static readonly Guid CLSID_TEXT_SERVICE;
        private static readonly Guid GUID_PROFILE;

        private static readonly string PSZTITLE_HANS;
        private static readonly string PSZTITLE_HANT;

        private static readonly string RIME_ROOT_REG_KEY;
        private static readonly string WEASEL_PROG_REG_KEY;
        private static readonly string WEASEL_SERVER_EXE;
        private static readonly string WEASEL_WER_REG_KEY;

        static Setup()
        {
            CLSID_TEXT_SERVICE = Guid.Parse("{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}");
            GUID_PROFILE = Guid.Parse("{3D02CAB6-2B8E-4781-BA20-1C9267529467}");

            PSZTITLE_HANS = $"0804:{CLSID_TEXT_SERVICE:B}{GUID_PROFILE:B}";
            PSZTITLE_HANT = $"0404:{CLSID_TEXT_SERVICE:B}{GUID_PROFILE:B}";

            RIME_ROOT_REG_KEY = @"SOFTWARE\Rime";
            WEASEL_PROG_REG_KEY = $@"{RIME_ROOT_REG_KEY}\Weasel";
            WEASEL_SERVER_EXE = "WeaselServer.exe";

            WEASEL_WER_REG_KEY = $@"SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\{WEASEL_SERVER_EXE}";
        }

        public static bool IsWeaselInstalled
        {
            get
            {
                var sys32Dir = Environment.GetFolderPath(Environment.SpecialFolder.System);
                var weaselPath = Path.Combine(sys32Dir, "weasel.dll");
                return File.Exists(weaselPath);
            }
        }

        public static void NormalInstall(bool isHant, bool isSilentMode = true)
        {
            // 安装 IME 文件
            var productDir = Path.GetDirectoryName(Application.ExecutablePath);
            var imeSrcPath = Path.Combine(productDir, "weasel.dll");
            InstallImeFiles(imeSrcPath, (libPath) =>
            {
                UpdateServiceState(libPath, true, isHant);
            });

            // 登记注册表
            using (var local = Registry.LocalMachine.CreateSubKey(WEASEL_PROG_REG_KEY))
            {
                local.SetValue("WeaselRoot", productDir);
                local.SetValue("ServerExecutable", WEASEL_SERVER_EXE);
            }

            // 启用键盘布局和文本服务
            var psz = isHant ? PSZTITLE_HANT : PSZTITLE_HANS;
            PInvoke.InstallLayoutOrTip(psz, 0);

            // 收集用户模式转储
            // https://learn.microsoft.com/zh-cn/windows/win32/wer/collecting-user-mode-dumps
            using (var local = Registry.LocalMachine.CreateSubKey(WEASEL_WER_REG_KEY))
            {
                local.SetValue("DumpFolder", Utils.LogPath);
                local.SetValue("DumpType", 0);
                local.SetValue("CustomDumpFlags", 0);
                local.SetValue("DumpCount", 10);
            }

            if (!isSilentMode)
            {
                MessageBox.Show(Localization.Resources.STR_INSTALL_SUCCESS);
            }
        }

        public static void Uninstall(bool isSilentMode)
        {
            // 停用键盘布局和文本服务
            var isHant = Convert.ToBoolean(
                Registry.CurrentUser.GetValue($@"{WEASEL_PROG_REG_KEY}\Hant")
            );
            var psz = isHant ? PSZTITLE_HANT : PSZTITLE_HANS;
            PInvoke.InstallLayoutOrTip(psz, PInvoke.ILOT.ILOT_UNINSTALL);

            UninstallImeFiles("weasel.dll", (imePath) =>
            {
                UpdateServiceState(imePath, enable: false, isHant);
            });

            // 清理注册表
            using (var local = Registry.LocalMachine)
            {
                local.DeleteSubKeyTree(RIME_ROOT_REG_KEY, throwOnMissingSubKey: false);
                local.DeleteSubKey(WEASEL_WER_REG_KEY, throwOnMissingSubKey: false);
            }

            if (!isSilentMode)
            {
                MessageBox.Show(Localization.Resources.STR_UNINSTALL_SUCCESS);
            }
        }

        private static void InstallImeFiles(string srcPath, Action<string> updateService)
        {
            var baseName = Path.GetFileName(srcPath);
            var sys32Dir = Environment.GetFolderPath(Environment.SpecialFolder.System);
            var destPath = Path.Combine(sys32Dir, baseName);

            if (Environment.Is64BitOperatingSystem)
            {
                if (RuntimeInformation.OSArchitecture == Architecture.Arm64)
                {
                    var sysarm32Dir = Path.Combine(
                        Environment.GetFolderPath(Environment.SpecialFolder.Windows),
                        "SysArm32"
                    );
                    if (Directory.Exists(sysarm32Dir))
                    {
                        // 如果支持 ARM32 子系统，则安装 ARM32 版本（Windows 11 24H2 之前）
                        var arm32SrcPath = srcPath.Insert(srcPath.LastIndexOf('.'), "ARM");
                        var arm32DestPath = Path.Combine(sysarm32Dir, baseName);
                        if (File.Exists(arm32SrcPath))
                        {
                            Utils.CopyFile(arm32SrcPath, arm32DestPath);
                            updateService(arm32SrcPath);
                        }
                    }

                    // 安装 ARM64（和 x64）版本库。
                    // ARM64 系统上进程会被重定向至 ARM64X DLL，
                    // 其加载时，ARM64 进程会被重定向到 ARM64 DLL，x64 进程会被重定向到 x64 DLL，
                    // 所以这三个库都要安装
                    var x64SrcPath = srcPath.Insert(srcPath.LastIndexOf('.'), "x64");
                    var x64DestPath = destPath.Insert(srcPath.LastIndexOf('.'), "x64");
                    if (File.Exists(x64SrcPath)) Utils.CopyFile(x64SrcPath, x64DestPath);

                    var arm64SrcPath = srcPath.Insert(srcPath.LastIndexOf('.'), "x64");
                    var arm64DestPath = destPath.Insert(srcPath.LastIndexOf('.'), "x64");
                    if (File.Exists(arm64SrcPath)) Utils.CopyFile(arm64SrcPath, arm64DestPath);

                    // ARM64X 充当重定向器（转发器），因此我们不必区分其简繁变体
                    var arm64xSrcPath = srcPath.Insert(srcPath.LastIndexOf('.'), "ARM64X");
                    if (File.Exists(arm64xSrcPath))
                    {
                        Utils.CopyFile(arm64xSrcPath, destPath);
                        updateService(destPath);
                    }
                }
                else
                {
                    var sysWow64Dir = Environment.GetFolderPath(Environment.SpecialFolder.SystemX86);
                    var wow64DestPath = Path.Combine(sysWow64Dir, baseName);
                    if (File.Exists(srcPath))
                    {
                        Utils.CopyFile(srcPath, wow64DestPath);
                        updateService(wow64DestPath);
                    }
                    var x64SrcPath = srcPath.Insert(srcPath.LastIndexOf('.'), "x64");
                    if (File.Exists(x64SrcPath))
                    {
                        Utils.CopyFile(x64SrcPath, destPath);
                        updateService(destPath);
                    }
                }
            }
            else
            {
                if (File.Exists(srcPath))
                {
                    File.Copy(srcPath, destPath, true);
                    updateService(destPath);
                }
            }
        }

        private static void UninstallImeFiles(string baseName, Action<string> updateService)
        {
            var sys32Dir = Environment.GetFolderPath(Environment.SpecialFolder.System);
            var imePath = Path.Combine(sys32Dir, baseName);

            if (Environment.Is64BitOperatingSystem)
            {
                var sysWow64Dir = Environment.GetFolderPath(Environment.SpecialFolder.SystemX86);
                var wow64ImePath = Path.Combine(sysWow64Dir, baseName);
                if (File.Exists(wow64ImePath)) Utils.DeleteFile(wow64ImePath);

                if (RuntimeInformation.OSArchitecture == Architecture.Arm64)
                {
                    var sysarm32Dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "SysArm32");
                    if (Directory.Exists(sysarm32Dir))
                    {
                        var arm32ImePath = Path.Combine(sysarm32Dir, baseName);
                        if (File.Exists(arm32ImePath))
                        {
                            updateService(arm32ImePath);
                            Utils.DeleteFile(arm32ImePath);
                        }
                    }

                    var x64ImePath = imePath.Insert(imePath.LastIndexOf('.'), "x64");
                    if (File.Exists(x64ImePath)) Utils.DeleteFile(x64ImePath);

                    var arm64ImePath = imePath.Insert(imePath.LastIndexOf('.'), "ARM64");
                    if (File.Exists(arm64ImePath)) Utils.DeleteFile(arm64ImePath);
                }
                else
                {
                    if (File.Exists(imePath))
                    {
                        updateService(imePath);
                        Utils.DeleteFile(imePath);
                    }
                }
            }
            else
            {
                if (File.Exists(imePath))
                {
                    updateService(imePath);
                    Utils.DeleteFile(imePath);
                }
            }
        }

        private static void UpdateServiceState(string libPath, bool enable, bool isHant)
        {
            if (!enable) UpdateProfile(false, isHant);
            var value = isHant ? "hant" : "hans";
            Environment.SetEnvironmentVariable("TEXTSERVICE_PROFILE", value);
            var sysarm32Dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "SysArm32");
            var regsvr32Path = Directory.Exists(sysarm32Dir)
                ? Path.Combine(sysarm32Dir, "regsvr32.exe")
                : "regsvr32.exe";
            var args = enable ? $"/s \"{libPath}\"" : $"/s /u \"{libPath}\"";
            var updateInfo = new ProcessStartInfo
            {
                FileName = regsvr32Path,
                Arguments = args,
                Verb = "open",
                UseShellExecute = true,
            };
            Process.Start(updateInfo).WaitForExit();

            if (enable) UpdateProfile(true, isHant);
        }

        private static void UpdateProfile(bool enable, bool isHant)
        {
            var langId = isHant ? new LangID(0x0404) : new LangID(0x0804);
            var profiles = new ITfInputProcessorProfiles();
            if (enable)
            {
                profiles.EnableLanguageProfile(CLSID_TEXT_SERVICE, langId, GUID_PROFILE, enable);
                profiles.EnableLanguageProfileByDefault(CLSID_TEXT_SERVICE, langId, GUID_PROFILE, out _);
            }
            else
            {
                profiles.RemoveLanguageProfile(CLSID_TEXT_SERVICE, langId, GUID_PROFILE);
            }
        }
    }
}
