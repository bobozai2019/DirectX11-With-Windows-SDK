param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [string]$BuildDirectory = '',
    [switch]$Validate
)
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $repoRoot 'build-water-comparison' }
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmakeExe = $cmakeCommand.Source }
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsInstall) { throw 'Install Visual Studio C++ desktop tools and CMake.' }
    $cmakeExe = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
& $cmakeExe -S $PSScriptRoot -B $BuildDirectory -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmakeExe --build $BuildDirectory --config $Configuration --target 41_Godot_Water_Comparison --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
$executable = Join-Path $BuildDirectory "$Configuration\41 Godot Water Comparison.exe"
Write-Output "Executable: $executable"
if ($Validate) {
    $process = Start-Process -FilePath $executable -ArgumentList '--validate' -WindowStyle Hidden -PassThru -Wait
    Get-Content (Join-Path (Split-Path $executable) 'validation.txt') -ErrorAction SilentlyContinue
    if ($process.ExitCode -ne 0) {
        Get-Content (Join-Path (Split-Path $executable) 'error.log') -ErrorAction SilentlyContinue
        throw "Validation failed with exit code $($process.ExitCode)."
    }
}
