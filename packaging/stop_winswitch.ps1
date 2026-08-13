param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath
)

$targetPath = (Get-Item -LiteralPath $ExecutablePath -ErrorAction Stop).FullName
$processes = Get-CimInstance Win32_Process -Filter "Name='winSwitch.exe'" -ErrorAction SilentlyContinue
foreach ($process in $processes) {
    $processPath = (Get-Item -LiteralPath $process.ExecutablePath -ErrorAction SilentlyContinue).FullName
    if ([string]::Equals($processPath, $targetPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        Stop-Process -Id $process.ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $process.ProcessId -Timeout 10 -ErrorAction SilentlyContinue
    }
}
Start-Sleep -Milliseconds 500
