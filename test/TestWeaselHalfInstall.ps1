param(
  [Parameter(Mandatory = $true)]
  [string]$Installer
)

$ErrorActionPreference = 'Stop'

# This test installs software and opens/locks installed files on a disposable
# GitHub Actions runner. Never run it on a working machine.
if ($env:GITHUB_ACTIONS -ne 'true') {
  throw 'Run this test only on a disposable GitHub Actions runner.'
}

$registryPath = 'HKLM:\SOFTWARE\WOW6432Node\Rime\Weasel'
$uninstallKey = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Weasel'
$isX64 = [Environment]::Is64BitOperatingSystem

$criticalNames = @(
  'WeaselServer.exe',
  'WeaselDeployer.exe',
  'rime.dll',
  'WinSparkle.dll',
  'weasel.dll'
)
if ($isX64) {
  $criticalNames += 'weaselx64.dll'
}

function Invoke-Setup {
  param(
    [string]$Path,
    [string]$Arguments,
    [switch]$ExpectFailure
  )
  Write-Output ('Running: ' + $Path + ' ' + $Arguments)
  $process = [Diagnostics.Process]::Start($Path, $Arguments)
  if (-not $process.WaitForExit(180000)) {
    $process.Kill()
    throw 'Setup timed out.'
  }
  $code = $process.ExitCode
  Write-Output ('Exit code: ' + $code)
  if ($ExpectFailure) {
    if ($code -eq 0) {
      throw 'Setup reported success although a critical file was locked.'
    }
  } elseif ($code -ne 0) {
    throw ('Setup failed with exit code ' + $code)
  }
}

function Get-PeTimestamp {
  param([string]$Path)
  $bytes = [IO.File]::ReadAllBytes($Path)
  if ($bytes.Length -lt 0x40 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw ('Not a PE file: ' + $Path)
  }
  $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
  if ($bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45) {
    throw ('Not a PE file: ' + $Path)
  }
  return [BitConverter]::ToUInt32($bytes, $peOffset + 8)
}

function Get-BinaryState {
  param([string]$Root)
  $state = @{}
  foreach ($name in $criticalNames) {
    $path = Join-Path $Root $name
    if (-not (Test-Path -LiteralPath $path)) {
      $state[$name] = 'missing'
      continue
    }
    $state[$name] = [string](Get-PeTimestamp $path) + '/' +
      [string](Get-Item -LiteralPath $path).Length
  }
  return $state
}

function Compare-BinaryState {
  param($Expected, $Actual, [string]$Context)
  foreach ($name in $criticalNames) {
    if ($Expected[$name] -ne $Actual[$name]) {
      throw ($Context + ': ' + $name + ' expected ' + $Expected[$name] +
        ' but found ' + $Actual[$name])
    }
  }
  Write-Output ($Context + ': all critical binaries match the fresh install.')
}

function Get-DisplayVersion {
  if (-not (Test-Path -LiteralPath $uninstallKey)) {
    return $null
  }
  return (Get-ItemProperty -LiteralPath $uninstallKey).DisplayVersion
}

# The installer starts a server when it finishes; that server holds the very
# files this test rewrites, so stop it (by image path) between phases.
function Stop-WeaselServers {
  param([string]$Root)
  $serverPath = Join-Path $Root 'WeaselServer.exe'
  Get-Process -Name 'WeaselServer' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -ieq $serverPath } |
    ForEach-Object { Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue }
  Start-Sleep -Seconds 1
}

# Start from a known state: this test may run after other installer tests.
if (Test-Path -LiteralPath $registryPath) {
  $existingRoot = (Get-ItemProperty -LiteralPath $registryPath).WeaselRoot
  $existingUninstaller = Join-Path $existingRoot 'uninstall.exe'
  if (Test-Path -LiteralPath $existingUninstaller) {
    Write-Output 'Removing the previous test installation first.'
    Invoke-Setup $existingUninstaller '/S'
  }
}

# --- 1. Fresh install, record what the package actually wrote -----------------
Invoke-Setup $Installer '/S'
$installRoot = (Get-ItemProperty -LiteralPath $registryPath).WeaselRoot
if (-not $installRoot) {
  throw 'WeaselRoot was not written by the installer.'
}
$expected = Get-BinaryState $installRoot
foreach ($name in $criticalNames) {
  if ($expected[$name] -eq 'missing') {
    throw ('A critical binary is missing after a fresh install: ' + $name)
  }
}
Write-Output ('Fresh install recorded: ' + ($expected | Out-String))

# --- 2. Stale binaries must be replaced again ---------------------------------
# Overwrite three of the files named in the PR evidence with a different
# binary: an installer that silently skips locked files would leave these.
Stop-WeaselServers -Root $installRoot
$staleSource = Join-Path $installRoot '7z.exe'
if (-not (Test-Path -LiteralPath $staleSource)) {
  throw '7z.exe is missing from the installation; cannot build stale fixtures.'
}
foreach ($name in @('WeaselServer.exe', 'rime.dll', 'WinSparkle.dll')) {
  Copy-Item -LiteralPath $staleSource -Destination (Join-Path $installRoot $name) -Force
}
$stale = Get-BinaryState $installRoot
Compare-BinaryState -Expected $expected -Actual $stale -Context 'Stale fixtures'
Write-Output 'Stale fixtures installed (verification must fail at this point).'
if (($stale['WeaselServer.exe'] -eq $expected['WeaselServer.exe'])) {
  throw 'The stale fixture is indistinguishable from the real binary.'
}

Invoke-Setup $Installer '/S'
$replaced = Get-BinaryState $installRoot
Compare-BinaryState -Expected $expected -Actual $replaced -Context 'Reinstall over stale binaries'

# --- 3. A locked critical binary must abort the install -----------------------
Stop-WeaselServers -Root $installRoot
$versionBefore = Get-DisplayVersion
$lockedPath = Join-Path $installRoot 'rime.dll'
$handle = [IO.File]::Open($lockedPath, [IO.FileMode]::Open,
  [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try {
  Invoke-Setup $Installer '/S' -ExpectFailure
} finally {
  $handle.Dispose()
}
$versionAfter = Get-DisplayVersion
if ($versionBefore -ne $versionAfter) {
  throw ('A failed install changed DisplayVersion: ' + $versionBefore + ' -> ' + $versionAfter)
}
Write-Output ('Locked-file install aborted without touching DisplayVersion (' + $versionAfter + ').')

# The next install must still work once the lock is gone.
Invoke-Setup $Installer '/S'
$afterLock = Get-BinaryState $installRoot
Compare-BinaryState -Expected $expected -Actual $afterLock -Context 'Reinstall after the lock was released'

# --- 4. A running server must be stopped before the binaries are replaced ----
Stop-WeaselServers -Root $installRoot
$serverPath = Join-Path $installRoot 'WeaselServer.exe'
Start-Process -FilePath $serverPath | Out-Null
$oldPids = @()
$deadline = (Get-Date).AddSeconds(30)
while ((Get-Date) -lt $deadline -and $oldPids.Count -eq 0) {
  Start-Sleep -Seconds 1
  $oldPids = @(Get-Process -Name 'WeaselServer' -ErrorAction SilentlyContinue |
      Where-Object { $_.Path -ieq $serverPath } |
      ForEach-Object { $_.Id })
}
if ($oldPids.Count -eq 0) {
  throw 'Could not start WeaselServer for the stop test.'
}
Write-Output ('Running server before install: ' + ($oldPids -join ','))

Invoke-Setup $Installer '/S'
foreach ($oldPid in $oldPids) {
  if (Get-Process -Id $oldPid -ErrorAction SilentlyContinue) {
    throw ('The installer left the previous server running (pid ' + $oldPid + ').')
  }
}
$afterServer = Get-BinaryState $installRoot
Compare-BinaryState -Expected $expected -Actual $afterServer -Context 'Reinstall with a running server'

Write-Output 'Half-install protection verified: stale files replaced, locked files abort, running server stopped.'