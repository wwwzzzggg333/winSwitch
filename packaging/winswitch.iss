[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "winSwitch"; ValueData: """{app}\bin\winSwitch.exe"""; Flags: uninsdeletevalue

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File ""{app}\stop_winswitch.ps1"" -ExecutablePath ""{app}\bin\winSwitch.exe"""; Flags: runhidden waituntilterminated; RunOnceId: "StopWinSwitch"
