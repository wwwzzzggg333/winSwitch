# Sign winSwitch.exe after a rebuild.
# Usage (from repo root): powershell -ExecutionPolicy Bypass -File scripts/sign.ps1
# Requires: admin rights, the self-signed cert "CN=winSwitch Dev" in LocalMachine\My.
#
# Why PFX and not /sha1: New-SelfSignedCertificate stores the private key in CSP,
# which signtool cannot match by name/thumbprint. Exporting to PFX sidesteps this.
#
# NOTE: Self-signed signatures do NOT satisfy Windows Smart App Control. If SAC is
# fully enforced, this signed exe will still be blocked (Access is denied). This
# script is only useful once a real CA-issued cert (OV/EV) is in LocalMachine\My —
# replace $CertSubject with that cert's subject.

$ErrorActionPreference = 'Stop'
$CertSubject = 'CN=winSwitch Dev'
$PfxPath = Join-Path $env:TEMP 'winswitch_dev_sign.pfx'
$PfxPass = 'winswitch-dev-sign-local'
$Exe = Join-Path $PSScriptRoot '..\build\Release\winSwitch.exe'
$Signtool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe'

if (-not (Test-Path $Exe)) { throw "Not found: $Exe" }

$cert = Get-ChildItem -Path 'Cert:\LocalMachine\My' -CodeSigningCert |
    Where-Object { $_.Subject -eq $CertSubject } | Select-Object -First 1
if (-not $cert) { throw "Cert not found in LocalMachine\My: $CertSubject" }

$secure = ConvertTo-SecureString -String $PfxPass -Force -AsPlainText
$cert | Export-PfxCertificate -FilePath $PfxPath -Password $secure -Force | Out-Null

& $Signtool sign /fd SHA256 /f $PfxPath /p $PfxPass `
    /tr http://timestamp.digicert.com /td SHA256 /v $Exe

Remove-Item -Path $PfxPath -Force -ErrorAction SilentlyContinue

& $Signtool verify /pa /v $Exe
