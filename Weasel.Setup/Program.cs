using Microsoft.Win32;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Principal;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Weasel.Setup
{
    internal class Program
    {
        private static bool IsRunAsAdmin()
        {
            var identity = WindowsIdentity.GetCurrent();
            var principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }

        private static void RunAsAdmin(string arg)
        {
            var info = new ProcessStartInfo
            {
                FileName = Application.ExecutablePath,
                Arguments = arg,
                Verb = "RunAs",
                UseShellExecute = true,
            };
            Process.Start(info);
        }

        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Run(string.Join(" ", args));
        }

        private static readonly string WEASEL_PROG_REG_KEY = @"SOFTWARE\Rime\Weasel";
        private static readonly string WEASEL_UPDATE_REG_KEY = $@"{WEASEL_PROG_REG_KEY}\Updates";

        private static void Run(string arg)
        {
            try
            {
                if (arg.StartsWith("/userdir:")) // 设置用户目录
                {
                    var dir = arg.Substring(arg.IndexOf(':') + 1);
                    if (dir != null)
                    {
                        Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\RimeUserDir", dir);
                    }
                    return;
                }
                else
                {
                    switch (arg) // 无需管理员权限的操作
                    {
                        case "/ls": // 简体中文
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\Language", "chs");
                            return;
                        case "/lt": // 繁体中文
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\Language", "cht");
                            return;
                        case "/le": // 英语
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\Language", "eng");
                            return;
                        case "/eu": // 启用更新
                            Registry.CurrentUser.SetValue($@"{WEASEL_UPDATE_REG_KEY}\CheckForUpdates", "1");
                            return;
                        case "/du": // 禁用更新
                            Registry.CurrentUser.SetValue($@"{WEASEL_UPDATE_REG_KEY}\CheckForUpdates", "0");
                            return;
                        case "/toggleime":
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\ToggleImeOnOpenClose", "yes");
                            return;
                        case "/toggleascii":
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\ToggleImeOnOpenClose", "no");
                            return;
                        case "/testing": // 测试通道
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\UpdateChannel", "testing");
                            return;
                        case "/release": // 正式通道
                            Registry.CurrentUser.SetValue($@"{WEASEL_PROG_REG_KEY}\UpdateChannel", "release");
                            return;
                        default: // 需要管理员权限的操作
                            if (!IsRunAsAdmin())
                            {
                                RunAsAdmin(arg);
                                Application.Exit();
                                return;
                            }
                            switch (arg)
                            {
                                case "/u": // 卸载
                                    Setup.Uninstall(true);
                                    return;
                                case "/s": // 简体中文安装
                                    Setup.NormalInstall(false);
                                    return;
                                case "/t": // 繁体中文安装
                                    Setup.NormalInstall(true);
                                    return;
                                default:
                                    break;
                            }
                            break;
                    }
                }
                // 自定义安装
                CustomInstall(arg == "/i");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                MessageBox.Show(ex.StackTrace);
            }
        }

        private static void CustomInstall(bool isInstalling)
        {
            var isSilentMode = false;
            var isInstalled = Setup.IsWeaselInstalled;

            bool isHant = false;
            string userDir = string.Empty;
            using (var user = Registry.CurrentUser.OpenSubKey(WEASEL_PROG_REG_KEY))
            {
                if (user != null)
                {
                    userDir = Convert.ToString(user.GetValue("RimeUserDir"));
                    var value = user.GetValue("Hant");
                    if (value != null && value is int)
                    {
                        isHant = Convert.ToBoolean(value);
                        if (isInstalling) isSilentMode = true;
                    }
                }
            }

            if (!isSilentMode)
            {
                var dialog = new InstallOptionDialog
                {
                    IsInstalled = isInstalled,
                    IsHant = isHant,
                    UserDir = userDir
                };
                if (DialogResult.OK == dialog.ShowDialog())
                {
                    isInstalled = dialog.IsInstalled;
                    isHant = dialog.IsHant;
                    userDir = dialog.UserDir;

                    using (var user = Registry.CurrentUser.CreateSubKey(WEASEL_PROG_REG_KEY))
                    {
                        user.SetValue($@"{WEASEL_PROG_REG_KEY}\RimeUserDir", userDir);
                        user.SetValue($@"{WEASEL_PROG_REG_KEY}\Hant", isHant ? 1 : 0);
                    }
                }
                else
                {
                    if (!isInstalling)
                    {
                        Application.Exit();
                        return;
                    }
                }
            }
            if (!isInstalled)
            {
                Setup.NormalInstall(isHant, isSilentMode);
            }
            else
            {
                var installDir = Path.GetDirectoryName(Application.ExecutablePath); ;
                Task.Run(() =>
                {
                    ExecProcess(Path.Combine(installDir, "WeaselServer.exe"), "/q");
                    Task.Delay(500);

                    ExecProcess(Path.Combine(installDir, "WeaselServer.exe"), string.Empty);
                    Task.Delay(500);

                    ExecProcess(Path.Combine(installDir, "WeaselDeployer.exe"), "/deploy");
                });
            }
        }

        private static void ExecProcess(string path, string args)
        {
            var info = new ProcessStartInfo
            {
                FileName = path,
                Arguments = args,
                CreateNoWindow = true,
                WindowStyle = ProcessWindowStyle.Normal,
            };
            Process.Start(info);
        }
    }
}
