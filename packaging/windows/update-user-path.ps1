# SPDX-License-Identifier: MIT
# Adapted from the cREXX-RAG installer registry interface.
# Installer helper: never pass a user's potentially long PATH through NSIS.
$ErrorActionPreference = 'Stop'
$environment = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Environment')
$state = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Software\z-pdos')
try {
    $exists = $environment.GetValueNames() -contains 'Path'
    $path = [string]$environment.GetValue('Path', '', 1)
    $kind = if ($exists) { $environment.GetValueKind('Path') } else { 'ExpandString' }
    $bin = $env:ZPDOS_INSTALL_BIN
    if (!$bin) { throw 'Missing installer directory.' }
    if ($env:ZPDOS_PATH_ACTION -eq 'add') {
        # Reinstall retains the first install's ownership and absence decisions.
        if ($null -eq $state.GetValue('PathAdded', $null)) {
            $state.SetValue('PathAdded', [int](-not ($path.Split(';') -contains $bin)))
            $state.SetValue('PathExisted', [int]$exists)
        }
        if (-not ($path.Split(';') -contains $bin)) {
            $path = if ($path) { "$path;$bin" } else { $bin }
            $environment.SetValue('Path', $path, $kind)
        }
    } elseif ($env:ZPDOS_PATH_ACTION -eq 'remove') {
        if ($state.GetValue('PathAdded', 0) -eq 1) {
            $path = ($path.Split(';') | Where-Object { $_ -ne $bin }) -join ';'
            if ($path -eq '' -and $state.GetValue('PathExisted', 1) -eq 0) {
                $environment.DeleteValue('Path', $false)
            } else {
                $environment.SetValue('Path', $path, $kind)
            }
        }
    } else { throw 'Unknown installer PATH action.' }
} catch {
    Write-Output $_
    exit 1
} finally {
    $state.Close()
    $environment.Close()
}
