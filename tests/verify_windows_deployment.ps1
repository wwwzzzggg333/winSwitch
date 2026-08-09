param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration
)

$resolvedExe = (Resolve-Path -LiteralPath $ExePath).Path
$outputDir = Split-Path -Parent $resolvedExe
$suffix = if ($Configuration -eq 'Debug') { 'd' } else { '' }
$required = @(
    "Qt6Core$suffix.dll",
    "Qt6Gui$suffix.dll",
    "Qt6Widgets$suffix.dll",
    "platforms/qwindows$suffix.dll"
)
$missing = @($required | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $outputDir $_))
})
if ($missing.Count -gt 0) {
    throw "Missing deployed Qt runtime files: $($missing -join ', ')"
}
Write-Output "Qt runtime deployment verified for $Configuration at $outputDir"
