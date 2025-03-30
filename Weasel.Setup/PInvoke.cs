using System;
using System.Runtime.InteropServices;

namespace Weasel.Setup
{
    internal class PInvoke
    {
        [Flags]
        public enum ILOT
        {
            ILOT_UNINSTALL = 0x00000001,
        }

        [DllImport("input.dll", SetLastError = false, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool InstallLayoutOrTip([In, MarshalAs(UnmanagedType.LPWStr)] string psz, [In] ILOT dwFlags);


        [Flags]
        public enum MoveFileFlags
        {
            MOVEFILE_REPLACE_EXISTING = 0x1,
            MOVEFILE_DELAY_UNTIL_REBOOT = 0x4
        }


        [DllImport("kernel32.dll", EntryPoint = "MoveFileExW", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool MoveFileEx(string src, string dest, MoveFileFlags flags);
        
        [Flags]
        public enum PROCESS_DPI_AWARENESS
        {
            PROCESS_DPI_UNAWARE = 0,
            PROCESS_SYSTEM_DPI_AWARE,
            PROCESS_PER_MONITOR_DPI_AWARE,
        }

        [DllImport("Shcore.dll", SetLastError = true)]
        public static extern int SetProcessDpiAwareness(PROCESS_DPI_AWARENESS type);
    }
}
