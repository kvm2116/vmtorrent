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
Global $notepadDone = 0
Global $winampDone = 0

;;; RUN APPLICATIONS
MsgBox ( 4100, "Music Starting", "2s Timeout", 2)
SoundPlay($execpath & "test_20s.mp3", 1)
MsgBox ( 4100, "Music Done", "2s Timeout", 2)