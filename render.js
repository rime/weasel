// nanotemplate
(function() {
  var _tregex = /(\$\w+)/g;

  String.prototype.template = String.prototype.t = String.prototype.template || function() {
    if (arguments[0] instanceof Array)
      return arguments[0].map(this.t, this).join("");
    else {
      var args = typeof arguments[0] === "object" ? arguments[0] : arguments;
      return this.replace(_tregex, function(match) { return args[match.substr(1)]; });
    }
  };

  if (typeof Element === "function" || typeof Element === "object")
    Element.prototype.template = Element.prototype.t = Element.prototype.template || function() {
      this._tcache = this._tcache || this.innerHTML;
      this.innerHTML = this._tcache.t.apply(this._tcache, arguments);
    };
})();

// main
var fso = WScript.CreateObject("Scripting.FileSystemObject");
var shell = WScript.CreateObject("Wscript.Shell");
var env = shell.Environment("Process");

function Render(filename, params) {
  var ForReading = 1;
  var templateFile = fso.OpenTextFile(filename + '.template', ForReading);
  var input = templateFile.ReadAll();
  templateFile.Close();

  var output = input.template(params);
  var outputFile = fso.CreateTextFile(filename, true);
  outputFile.Write(output);
  outputFile.Close();
}

if (WScript.Arguments.length == 0) {
  WScript.Echo("Usage: " + WScript.ScriptName + " <TargetFile> ENV_VAR ...");
  WScript.Quit(1);
}

var filename = WScript.Arguments(0);
var params = {};
var variable;
for (var i = 1; i < WScript.Arguments.length; i++) {
  variable = WScript.Arguments(i);
  params[variable] = env(variable);
}
Render(filename, params);
WScript.Echo("Generated " + filename);
