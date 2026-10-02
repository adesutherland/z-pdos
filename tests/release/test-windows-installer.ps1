# SPDX-License-Identifier: MIT
# Adapted from cREXX-RAG disposable-runner installer acceptance.
param([Parameter(Mandatory)][string]$Installer)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:GITHUB_ACTIONS -ne 'true' -or !(Test-Path $env:RUNNER_TEMP)) {
    throw 'Installer execution requires a disposable GitHub Actions runner.'
}
# Native PowerShell does not inherit the MSYS shell step's tool PATH.
# The installed smoke recipe uses that shell's shasum/Perl utilities.
if (!(Test-Path -LiteralPath $env:PDOS_WINDOWS_BASH)) { throw 'Missing configured MSYS shell.' }
$env:PATH = "$(Split-Path -Parent $env:PDOS_WINDOWS_BASH);$env:PATH"
$registration = 'HKCU:\Software\z-pdos'
$uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\z-pdos'
if ((Test-Path $registration) -or (Test-Path $uninstallKey)) { throw 'Existing z-pdos install.' }
if ((Get-AuthenticodeSignature -LiteralPath $Installer).Status -ne 'NotSigned') {
    throw 'CI must produce the unsigned Windows installer.'
}
$environment = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Environment')
$original = $environment.GetValue('Path', $null, 1)
$originalKind = if ($null -ne $original) { $environment.GetValueKind('Path') } else { 'ExpandString' }
function Invoke-Installer([string]$Executable, [string]$Arguments) {
    $process = Start-Process -FilePath $Executable -ArgumentList $Arguments -PassThru
    if (!$process.WaitForExit(60000)) { $process.Kill(); throw 'Installer timed out.' }
    if ($process.ExitCode -ne 0) { throw "Installer exit $($process.ExitCode)" }
}
try {
    foreach ($case in 'normal', 'long', 'absent', 'existing') {
        $installRoot = Join-Path $env:RUNNER_TEMP "z-pdos installer $case"
        if (Test-Path $installRoot) { throw 'Installer destination already exists.' }
        $bin = Join-Path $installRoot 'bin'
        $before = switch ($case) {
            normal { $original }
            long { '%USERPROFILE%\Programs;' + ((1..200 | ForEach-Object { "C:\Existing tools $_\bin" }) -join ';') }
            absent { $null }
            existing { "C:\Existing tools;$bin" }
        }
        if ($null -eq $before) { $environment.DeleteValue('Path', $false) }
        else { $environment.SetValue('Path', $before, [Microsoft.Win32.RegistryValueKind]::ExpandString) }
        $expected = if ($case -eq 'existing') { $before } elseif ($before) { "$before;$bin" } else { $bin }
        try {
            foreach ($iteration in 1..2) {
                Invoke-Installer $Installer "/S /D=$installRoot"
                if ($environment.GetValue('Path', $null, 1) -cne $expected) {
                    throw "$case install/reinstall changed existing PATH or duplicated its entry."
                }
                if ($environment.GetValueKind('Path') -ne 'ExpandString') { throw 'PATH type changed.' }
                if (!(Test-Path $uninstallKey)) { throw 'Missing uninstall registration.' }
            }
            if ($case -eq 'normal') {
                & crexx -nokeep (Join-Path $PSScriptRoot 'check-installed.crexx') --args $installRoot
                if ($LASTEXITCODE -ne 0) { throw 'Installed native smoke failed.' }
            }
            foreach ($macro in 'mf$bsam.mac', 'mf$reg.mac', 'mf$short.mac') {
                if (!(Test-Path -LiteralPath (Join-Path $installRoot "share/z-pdos/pdpclib/source/interfaces/pdos31/$macro"))) {
                    throw "Missing literal macro filename: $macro"
                }
            }
            # Uninstall must preserve user data even inside the application folder.
            Set-Content -LiteralPath (Join-Path $installRoot 'user-library.txt') -Value 'preserve me'
        } finally {
            $uninstaller = Join-Path $installRoot 'Uninstall.exe'
            if (Test-Path $uninstaller) {
                Invoke-Installer $uninstaller '/S'
                $deadline = (Get-Date).AddSeconds(60)
                while ((Test-Path $uninstaller) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
            }
        }
        if ((Test-Path $registration) -or (Test-Path $uninstallKey)) { throw 'Registration not removed.' }
        if ($environment.GetValue('Path', $null, 1) -cne $before) { throw "$case PATH not restored." }
        if ((Get-Content -LiteralPath (Join-Path $installRoot 'user-library.txt')) -ne 'preserve me') { throw 'User data removed.' }
        foreach ($macro in 'mf$bsam.mac', 'mf$reg.mac', 'mf$short.mac') {
            if (Test-Path -LiteralPath (Join-Path $installRoot "share/z-pdos/pdpclib/source/interfaces/pdos31/$macro")) {
                throw "Owned macro filename not removed: $macro"
            }
        }
        foreach ($owned in 'mf-classic-cc.exe', 'mf-classic-cc-cms.exe', 'mf-classic-as.exe', 'mf-classic-ld.exe') {
            if (Test-Path (Join-Path $bin $owned)) { throw 'Owned executable not removed.' }
        }
    }
} finally {
    if ($null -eq $original) { $environment.DeleteValue('Path', $false) }
    else { $environment.SetValue('Path', $original, $originalKind) }
    $environment.Close()
}
Write-Host 'Installer, reinstall, native execution, long/absent/existing PATH and safe uninstall passed.'
