Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

avrdude = """C:\Users\emili\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17\bin\avrdude.exe"""
config = """C:\Users\emili\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17\etc\avrdude.conf"""

hexfile = """" & fso.GetParentFolderName(WScript.ScriptFullName) & "\blink.ino.hex"""

cmd = avrdude & " -C" & config & " -c usbasp -p m328p -U flash:w:" & hexfile & ":i"

returnCode = shell.Run(cmd, 0, True)

If returnCode = 0 Then
    MsgBox "Programacion completada con exito", 64, "ATmega328P"
Else
    MsgBox "ERROR al programar el ATmega328P" & vbCrLf & "Código: " & returnCode, 16, "ATmega328P"
End If
