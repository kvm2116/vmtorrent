#include <INet.au3>
#include <Constants.au3>

;;; SETUP VARIABLES
Global $pause = 2000
Global $spause = 1000
Global $userpath = "C:\Users\test"
Global $execpath = "Z:\hgfs\scripts\"
Global $docpath = $userpath & "\Documents\"
Global $test = "Fail"
Global $ip = _GetIP()
Global $params = "params.txt"
Global $videoDone = 0
Global $pdfDone = 0
Global $pptDone = 0
Global $notepadDone = 0

;;; RUN APPLICATIONS	
PPT("test.ppt")

;;; APPLICATIONS LOGIC
Func Video($video)
	ShellExecuteWait($execpath & $video)
	$status = WinWait("Windows Media Player", "problem", 17)
	If Not $status == 0 Then
		WinClose("Windows Media Player")
		WinWaitClose("Windows Media Player", "", 10)
	EndIf
	WinClose("Windows Media Player")
	$videoClosed = 0
	while($videoClosed == 0)
		$videoClosed = WinWaitClose("Windows Media Player", "", 10)
		sleep(1000)
	WEnd
	while (1)
		If (WinExists("Windows Media Player") == 0) Then
			Return 1
		EndIf
	WEnd	
	;LogTime("VideoComplete")	
EndFunc   ;==>Video

Func PDF($pdf)
	ShellExecute($execpath & $pdf)
	WinWait($pdf)
	If WinActive($pdf) Then
		WinSetState($pdf, "", @SW_MAXIMIZE)
		For $i = 0 To 3 Step 1
			Send("{Space}")
			Sleep($spause)
		Next
	EndIf
	$pdfClosed = 0
	while ($pdfClosed == 0) 
		$pdfClosed = WinClose($pdf)
		sleep(1000)
	WEnd		
	while (1)
		If (WinExists($pdf) == 0) Then
			Return 1
		EndIf
	WEnd	
	;LogTime("PdfComplete")
EndFunc   ;==>PDF

Func PPT($ppt)
	ShellExecute($execpath & $ppt)
	WinWait("Microsoft")
	While WinExists("Microsoft") 
		Sleep($spause * 5)
		Send("{F5}")
		For $i = 0 To 100 Step 1
			Send("{RIGHT}")
			Sleep($spause)
		Next
		Send("{ESC}")
		Send("!f")
		Send("!c")
		Send("!f")
		Send("!x")
	WEnd
	$pptClosed = 0
	while ($pptClosed == 0)
		$pptClosed = WinWaitClose("Microsoft", "", 10) 
		sleep(1000)
	WEnd
	while (1)
		If (WinExists("Microsoft") == 0) Then
			Return 1
		EndIf
	WEnd
	;LogTime("PptComplete")
EndFunc   ;==>PPT

Func Notepad($text)
	Run("notepad.exe")
	WinWait("Untitled - Notepad")
	Sleep($spause)
	Send("Hello World")
	WinClose("Untitled - Notepad")
	Send("!s")
	WinWait("Save")
	$notepadClosed = 0
	while (WinExists("Save") And $notepadClosed == 0)
		$notepadClosed = WinExists("Save")
		Sleep($spause * 5)
		Send($text)
		Sleep($spause * 5)
		Send("!s")
		Send("!y")
		sleep(2000)
	WEnd
	while(1)
		If (WinExists("Save") == 0 And $notepadClosed == 1) Then
			Return 1
		EndIf
	WEnd
	;LogTime("NotepadComplete")
EndFunc   ;==>Notepad

; NOT USED - THIS WAS WASTED WORK BUT I'LL KEEP FOR REFERENCE

;Global $wget = "C:\Program Files\GnuWin32\bin\wget.exe"
;Global $server = "http://"&$ip&":8000/"
;Global $server2 = "http://www.cs.columbia.edu/~orenl/papers/"

;;; RUN SCRIPTED ACTIONS
;$paramFile = FileOpen($params, 0)
;$test = FileReadLine($paramFile)
;FileClose($paramFile)

;;MsgBox(64, "", $test) ;;;THIS CAN BE USED TO POP UP A MESSAGE BOX

; LOGS THE TIME AN EVENT OCCURED
Func LogTime($event)
	$logfile = $docpath & $test & "-" & $ip & "-" & $event
	FileCopy($docpath & "fileholder", $logfile, 1)
	ShellExecuteWait("w32tm", "/resync")
	ShellExecuteWait("\Program Files\WinSCP\WinSCP.exe", "datastore /upload " & $logfile & " /defaults")
	FileDelete($logfile)
EndFunc   ;==>LogTime

Func GetIP()
	Local $sDir = "C:\Program Files\GnuWin32\bin"
	Local $sExe = "wget.exe -qO - http://cfaj.freeshell.org/ipaddr.cgi"
	Local $sOutput = ""
	Local $PID = Run($sDir & "\" & $sExe, $sDir, @SW_SHOW, $STDOUT_CHILD + $STDERR_CHILD)
	Sleep(3000)
	$sOutput &= StdoutRead($PID)
	Return $sOutput
EndFunc   ;==>GetIP