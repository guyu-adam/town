param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    throw @"
CMake was not found on PATH.

Install it, then rerun:
  winget install Kitware.CMake

Or install the Visual Studio "C++ CMake tools for Windows" component.
"@
}

& $cmake.Source -S $Root -B $BuildDir
& $cmake.Source --build $BuildDir --config $Config

$exeCandidates = @(
    (Join-Path $BuildDir "$Config\TownMarket.exe"),
    (Join-Path $BuildDir "TownMarket.exe")
)

$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($exe) {
    Write-Host "Built: $exe"
} else {
    Write-Host "Build completed. Check $BuildDir for the executable."
}
