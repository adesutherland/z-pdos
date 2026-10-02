# SPDX-License-Identifier: MIT
# Windows process isolation and PE import checks require the native interface.
param([Parameter(Mandatory)][string]$Payload)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:GITHUB_ACTIONS -ne 'true' -or !(Test-Path $env:RUNNER_TEMP)) {
    throw 'Runtime execution requires a disposable GitHub Actions runner.'
}
$objdump = (Resolve-Path (Join-Path (Split-Path -Parent $env:PDOS_WINDOWS_BASH) '../../ucrt64/bin/objdump.exe')).Path
$executables = @(Get-ChildItem "$Payload/bin/*.exe") + @(Get-ChildItem "$Payload/libexec/z-pdos/*/*.exe")
if ($executables.Count -ne 8) { throw 'Expected eight native tools.' }
$libraries = @(Get-ChildItem "$Payload/libexec/z-pdos/*/*.dll")
if ($libraries.Count -ne 2) { throw 'Expected private libiconv runtimes for MVS and CMS.' }
function Assert-DependencyClosure($Files) {
foreach ($file in $Files) {
    $imports = & $objdump -p $file.FullName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($file.FullName)" }
    foreach ($line in $imports) {
        if ($line -match 'DLL Name:\s*(\S+)') {
            $name = $Matches[1]
            if ($name -match '^(KERNEL32|ADVAPI32|msvcrt|ucrtbase)\.dll$' -or $name -match '^api-ms-win-crt-.*\.dll$') { continue }
            if (!(Test-Path -LiteralPath (Join-Path $file.DirectoryName $name))) {
                throw "Unbundled dependency $name for $($file.FullName)"
            }
        }
    }
}
}
Assert-DependencyClosure ($executables + $libraries)
function Invoke-Isolated([string]$Executable, [string[]]$Parameters) {
    $info = [System.Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $Executable
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.Environment['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot"
    foreach ($parameter in $Parameters) { $info.ArgumentList.Add($parameter) }
    $process = [System.Diagnostics.Process]::Start($info)
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw 'Isolated compiler timed out.' }
    Write-Host $process.StandardOutput.ReadToEnd()
    Write-Host $process.StandardError.ReadToEnd()
    $code = $process.ExitCode
    $process.Dispose()
    return $code
}
$fixture = Join-Path $env:RUNNER_TEMP "z-pdos clean runtime $([Guid]::NewGuid())"
New-Item -ItemType Directory -Path $fixture | Out-Null
try {
    Copy-Item "$Payload/*" -Destination $fixture -Recurse
    $source = (Resolve-Path (Join-Path $PSScriptRoot '../../compiler/tests/fullword-bits.c')).Path
    foreach ($variant in 'mvs', 'cms') {
        $name = if ($variant -eq 'mvs') { 'mf-classic-cc.exe' } else { 'mf-classic-cc-cms.exe' }
        $command = Join-Path $fixture "bin/$name"
        $assembly = Join-Path $fixture "$variant.asm"
        if ((Invoke-Isolated $command @('-S', '-Os', $source, '-o', $assembly)) -ne 0 -or !(Test-Path $assembly)) {
            throw "$variant compiler requires tools outside the delivery."
        }
        $library = Join-Path $fixture "libexec/z-pdos/$variant/libiconv-2.dll"
        Move-Item -LiteralPath $library -Destination "$library.saved"
        try {
            $rejected = $false
            try { Assert-DependencyClosure @(Get-Item (Join-Path $fixture "libexec/z-pdos/$variant/cc1.exe")) }
            catch {
                if ($_.Exception.Message -notlike 'Unbundled dependency libiconv-2.dll*') { throw }
                $rejected = $true
            }
            if (!$rejected) { throw 'Missing-library negative control was not rejected.' }
        } finally { Move-Item -LiteralPath "$library.saved" -Destination $library }
    }
} finally { Remove-Item -LiteralPath $fixture -Recurse -Force }
Write-Host 'PASS: PE dependency closure and MVS/CMS compilation with Windows-only PATH; missing DLL rejected.'
