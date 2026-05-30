' Launch the Buddy voice companion with NO visible console window.
' Handy for auto-start at login once everything works (you won't see logs).
Dim fso, sh
Set fso = CreateObject("Scripting.FileSystemObject")
Set sh = CreateObject("WScript.Shell")
sh.CurrentDirectory = fso.GetParentFolderName(WScript.ScriptFullName)
sh.Run "start-voice.bat", 0, False
