#!/usr/bin/python

from ldtp import *

#launch firefox

#launchapp('firefox')

#generatekeyevent('<alt><tab>');

#window = filter(lambda x: x.endswith('-MozillaFirefox'), getwindowlist())[0]
#time.sleep(1)

#selectmenuitem('*Firefox','mnuFile')
#time.sleep(1)

#generatekeyevent('<ctrl>lhttp://www.cnn.com<enter>');
#time.sleep(4)

#generatekeyevent('<alt><F4>')

#generatekeyevent('<ctrl><alt><t>')
#time.sleep(1)

#if 0==waittillguiexist('*@ubuntu:~'):
#	print >>sys.stderr, "didn't open the terminal"
#	sys.exit(1)

#generatekeyevent('rm Documents/vm_test*<enter>')
#time.sleep(1)
#generatekeyevent('<alt><F4>')
#time.sleep(1)

time.sleep(2)
launchapp('soffice')
time.sleep(1)

if 0==waittillguiexist('*OpenOffice.org'):
	print >>sys.stderr, "didn't open the Open Office"
	sys.exit(1)

#selectmenuitem('frmOpenOffice.org','mnuTextDocument')
click('frmOpenOffice.org', 'btnTextDocument')
time.sleep(2)

if 0==waittillguiexist('*orgWriter'):
	print >>sys.stderr, "didn't open the Writer"
	sys.exit(1)

paragraph_1='Japanese authorities are operating on the presumption that possible meltdowns are under way at two nuclear reactors.'

paragraph_2='The attempts to avert a possible nucleear crisis, centered around the Fukushima Daiichi facility in northeast Japan.'

settextvalue('*orgWriter','txt4',paragraph_1);
generatekeyevent('<enter>')
generatekeyevent(paragraph_2 + '<enter>')
time.sleep(2)

#selectmenuitem('*orgWriter','mnuFile;mnuSave')
click('*orgWriter','btnSave')
time.sleep(1)

if 0==waittillguiexist('dlgSave*'):
	print >>sys.stderr, "didn't open the Save Dialog"
	sys.exit(1)


settextvalue('dlgSave*', 'txtName', 'vm_test')
time.sleep(1)

click('dlgSave*','btnSave')
time.sleep(2)

generatekeyevent('<alt><F4>')

time.sleep(2)
launchapp('soffice')
time.sleep(1)

if 0==waittillguiexist('*OpenOffice.org'):
	print >>sys.stderr, "didn't open the Open Office"
	sys.exit(1)

#selectmenuitem('frmOpenOffice.org','mnuTextDocument')
click('frmOpenOffice.org', 'btnTextDocument')
time.sleep(2)

if 0==waittillguiexist('*orgWriter'):
	print >>sys.stderr, "didn't open the Writer"
	sys.exit(1)



#selectmenuitem('*orgWriter','mnuFile;mnuOpen')
click('*orgWriter','btnOpen')
time.sleep(1)

if 0==waittillguiexist('dlgOpen*'):
	print >>sys.stderr, "didn't open the Save Dialog"
	sys.exit(1)


selectrow('dlgOpen*', 'tblFiles', 'vm_test.odt')
time.sleep(1)

click('dlgOpen*','btnOpen')
time.sleep(2)

eraseword = ''
for i in range(0,10):
	eraseword += '<backspace>'

generatekeyevent(eraseword);
time.sleep(1.5)

generatekeyevent('asdfe beijing!')
time.sleep(1)

click('*orgWriter','btnSpelling*')
time.sleep(1)

#while guiexist('dlgOpenOffice*')==False:
click('dlgSpelling*','btnChange')
time.sleep(1)
click('dlgSpelling*','btnClose')
time.sleep(1)
#if guiexist('dlgOpenOffice*')==True:
#	click('dlgOpenOffice*','btnYes')
	#click('dlgOpenOffice*','btnOK')
#	time.sleep(2)


click('*orgWriter','btnSave')

generatekeyevent('<alt><F4>')



import os

try:
	os.remove(os.getenv("HOME")+"/Documents/vm_test.odt")
	os.remove(os.getenv("HOME")+"/Documents/vm_test2.odt")
except (OSError):
	pass


