// elevate.js -- runs target command line elevated
if (WScript.Arguments.Length >= 1) {
    Application = WScript.Arguments(0);
    Arguments = "";
    for (Index = 1; Index < WScript.Arguments.Length; Index += 1) {
        if (Index > 1) {
            Arguments += " ";
        }
        Arguments += WScript.Arguments(Index);
    }
    new ActiveXObject("Shell.Application").ShellExecute(Application, Arguments, "", "runas");
} else {
    WScript.Echo("Usage: elevate Application Arguments");
}
