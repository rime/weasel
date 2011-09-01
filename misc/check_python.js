// checkpython.js

function CreateEnvFile(contents)
{
   var fso, tf;
   fso = new ActiveXObject("Scripting.FileSystemObject");
   tf = fso.CreateTextFile("env.bat", true);
   tf.WriteLine(contents);
   tf.Close();
}

var sh = WScript.CreateObject("Wscript.Shell");
try {
    var path = sh.RegRead("HKLM\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath\\");
    CreateEnvFile("set PATH=%PATH%;\"" + path + "\"");
}
catch (e) {
    CreateEnvFile("@echo Python 2.7 not installed.");
}
