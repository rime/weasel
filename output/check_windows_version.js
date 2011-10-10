var wbemFlagReturnImmediately = 0x10; 
var wbemFlagForwardOnly = 0x20; 
 
var objWMIService = GetObject("winmgmts:\\\\.\\root\\CIMV2"); 
var colItems = objWMIService.ExecQuery("SELECT * FROM Win32_OperatingSystem", "WQL", wbemFlagReturnImmediately | wbemFlagForwardOnly); 
 
var isXP = 0;
var enumItems = new Enumerator(colItems); 
for (; !enumItems.atEnd(); enumItems.moveNext()) { 
  var objItem = enumItems.item(); 
  if (objItem.Caption.indexOf("Windows XP") != -1) {
    isXP = 1;
    break;
  }
} 
WScript.Quit(isXP);
