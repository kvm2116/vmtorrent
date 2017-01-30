#include <INet.au3>
#include <Constants.au3>

; Script Start - Add your code below here
;Global $wget = "C:\Program Files\GnuWin32\bin\wget.exe"
;Global $ip = _GetIP()
Global $scriptPath = "Z:\hgfs\scripts\"
Global $params = "params.txt"
;Global $usrpath = "C:\Users\test"
;Global $docpath = $usrpath & "\Documents\"
Global $test = "Win7TestAll"
Global $exe = "Win7Complete"
Global $final_action = "nothing"

$paramFile = FileOpen($scriptPath & $params, 0)
$test = FileReadLine($paramFile)
$exe = FileReadLine($paramFile)
$final_action = FileReadLine($paramFile)
FileClose($paramFile)
ShellExecuteWait($scriptPath & $exe)
If $final_action == "shutdown" Then
	Shutdown(5)
EndIf