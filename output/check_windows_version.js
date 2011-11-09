var wbemFlagReturnImmediately = 0x10; 
var wbemFlagForwardOnly = 0x20; 
 
var objWMIService = GetObject("winmgmts:\\\\.\\root\\CIMV2"); 
var colItems = objWMIService.ExecQuery("SELECT * FROM Win32_OperatingSystem", "WQL", wbemFlagReturnImmediately | wbemFlagForwardOnly); 
 
var isWinXP = 0;
var isX64 = 0;
var enumItems = new Enumerator(colItems); 
for (; !enumItems.atEnd(); enumItems.moveNext()) { 
  var objItem = enumItems.item(); 
  if (objItem.Caption.indexOf("Windows XP") != -1) {
    isWinXP = 1;
    break;
  }
  else if (objItem.OSArchitecture.indexOf("64") != -1) {
    isX64 = 2;
    break;
  }
} 
// 0: Win7 32-bit, 1: WinXP 32-bit, 2: Win7 64-bit
WScript.Quit(isWinXP + isX64);
