param(
  [Parameter(Mandatory = $true)]
  [string]$Installer
)

$ErrorActionPreference = 'Stop'

# This test installs software and changes language settings on a disposable runner.
if ($env:GITHUB_ACTIONS -ne 'true') {
  throw 'Run this test only on a disposable GitHub Actions runner.'
}

$tipWeasel = '0804:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-1C9267529467}'
$tipPinyin = '0804:{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}{FA550B04-5AD7-411F-A5AC-CA038EC515D7}'
$registryPath = 'HKLM:\SOFTWARE\WOW6432Node\Rime\Weasel'

function Invoke-Setup([string]$Path, [string]$Arguments) {
  Write-Output ('Running: ' + $Path + ' ' + $Arguments)
  # Start-Process in Windows PowerShell appends a space to the argument string.
  $process = [Diagnostics.Process]::Start($Path, $Arguments)
  # Wait only for setup; the server it starts is expected to keep running.
  if (-not $process.WaitForExit(90000)) {
    $process.Kill()
    throw 'Setup timed out.'
  }
  if ($process.ExitCode -ne 0) {
    throw ('Setup failed with exit code ' + $process.ExitCode)
  }
  Write-Output 'Setup completed.'
}

function Get-LanguageState($languages = (Get-WinUserLanguageList)) {
  return ($languages | ForEach-Object {
      $_.LanguageTag + ':' + ($_.InputMethodTips -join ',')
    }) -join ';'
}

$originalLanguages = Get-WinUserLanguageList
$originalDisplayLanguage = Get-WinUILanguageOverride
$originalInputMethod = Get-WinDefaultInputMethodOverride

try {
  $languages = New-WinUserLanguageList 'zh-CN'
  $languages.Add('en-US')
  Set-WinUserLanguageList $languages -Force
  $expectedLanguages = Get-WinUserLanguageList
  [void]$expectedLanguages[0].InputMethodTips.Add($tipWeasel)
  $expected = Get-LanguageState $expectedLanguages
  Invoke-Setup $Installer '/S'
  if ((Get-LanguageState) -ne $expected) {
    throw ('Installation changed other language settings. Expected: ' + $expected +
      '; actual: ' + (Get-LanguageState))
  }

  $installDirectory = (Get-ItemProperty $registryPath).WeaselRoot
  $setup = Join-Path $installDirectory 'WeaselSetup.exe'
  # /i must replace system DLLs even though the previous TSF install remains.
  Remove-ItemProperty $registryPath -Name WeaselRoot
  Invoke-Setup $setup '/i'
  Write-Output ('After reinstalling: ' + (Get-LanguageState))
  if ((Get-ItemProperty $registryPath).WeaselRoot -ne $installDirectory) {
    throw '/i skipped reinstalling the existing text service.'
  }

  # Check existing IMEs before removing Pinyin; removal disables its TSF profile.
  foreach ($tips in @(@($tipPinyin, $tipWeasel), @($tipWeasel))) {
    if ($tips.Count -eq 1) {
      # Use the same 32-bit host as WeaselSetup to change the test user's TIPs.
      $powershell = "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe"
      $helper = Join-Path $installDirectory 'WeaselLanguageProfile.ps1'
      & $powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $helper -Tip $tipPinyin -Uninstall
      if ($LASTEXITCODE -ne 0) {
        throw 'Removing Pinyin from the test user failed.'
      }
    }
    $expected = Get-LanguageState
    $configured = Get-WinUserLanguageList
    if (($configured[0].InputMethodTips -join ',') -ne ($tips -join ',')) {
      throw ('The test input methods were not configured. Expected: ' +
        ($tips -join ',') + '; actual: ' + (Get-LanguageState))
    }
    $displayLanguage = Get-WinUILanguageOverride
    $inputMethod = Get-WinDefaultInputMethodOverride

    Invoke-Setup $Installer '/S'

    if ((Get-LanguageState) -ne $expected) {
      throw ('Upgrade changed input methods. Before: ' + $expected +
        '; after: ' + (Get-LanguageState))
    }
    if ((Get-WinUILanguageOverride).Name -ne $displayLanguage.Name) {
      throw 'Upgrade changed the display language override.'
    }
    if ((Get-WinDefaultInputMethodOverride).InputMethodTip -ne $inputMethod.InputMethodTip) {
      throw 'Upgrade changed the default input method override.'
    }
  }

  # Shared IMM files must survive upgrades for accounts that have not migrated.
  $layoutPath = 'HKLM:\SYSTEM\CurrentControlSet\Control\Keyboard Layouts\E0FE0804'
  $otherLayoutPath = 'HKLM:\SYSTEM\CurrentControlSet\Control\Keyboard Layouts\E0FD0804'
  $preloadPath = 'HKCU:\Keyboard Layout\Preload'
  $substitutesPath = 'HKCU:\Keyboard Layout\Substitutes'
  $files = @("$env:WINDIR\System32\weasel.ime", "$env:WINDIR\SysWOW64\weasel.ime")
  foreach ($action in @('upgrade', 'uninstall')) {
    $createdPaths = @()
    $createdValues = @()
    try {
      foreach ($path in @($layoutPath, $otherLayoutPath) + $files) {
        if (Test-Path -LiteralPath $path) {
          throw ('A legacy test fixture already exists: ' + $path)
        }
      }
      New-Item -Path $layoutPath -Force | Out-Null
      $createdPaths += $layoutPath
      New-ItemProperty -LiteralPath $layoutPath -Name 'Ime File' -Value 'weasel.ime' | Out-Null
      New-Item -Path $otherLayoutPath -Force | Out-Null
      $createdPaths += $otherLayoutPath
      New-ItemProperty -LiteralPath $otherLayoutPath -Name 'Ime File' -Value 'other.ime' | Out-Null
      foreach ($file in $files) {
        Set-Content -LiteralPath $file -Value 'Legacy cleanup test'
        $createdPaths += $file
      }
      if (-not (Test-Path $substitutesPath)) {
        New-Item -Path $substitutesPath | Out-Null
      }
      if ((Get-Item $substitutesPath).GetValueNames() -contains 'DFFE0804') {
        throw 'The legacy test alias already exists.'
      }
      New-ItemProperty -LiteralPath $substitutesPath -Name 'DFFE0804' -Value 'E0FE0804' | Out-Null
      $createdValues += ,@($substitutesPath, 'DFFE0804')
      $preload = Get-Item $preloadPath
      $last = ($preload.GetValueNames() | Where-Object { $_ -match '^[1-9][0-9]*$' } |
        ForEach-Object { [int]$_ } | Measure-Object -Maximum).Maximum
      foreach ($id in @('E0FE0804', 'DFFE0804')) {
        $last++
        New-ItemProperty -LiteralPath $preloadPath -Name "$last" -Value $id | Out-Null
        $createdValues += ,@($preloadPath, "$last")
      }

      if ($action -eq 'upgrade') {
        Invoke-Setup $Installer '/S'
        if ((Get-LanguageState) -ne $expected) {
          throw 'Legacy cleanup changed the existing TSF input methods.'
        }
      } else {
        Invoke-Setup $setup '/u'
        $remainingTips = @(Get-WinUserLanguageList | ForEach-Object { $_.InputMethodTips })
        if ($remainingTips -icontains $tipWeasel) {
          throw 'Uninstall left the Weasel input method enabled.'
        }
      }
      foreach ($path in @($layoutPath) + $files) {
        if ((Test-Path -LiteralPath $path) -ne ($action -eq 'upgrade')) {
          throw ('Unexpected shared legacy resource after ' + $action + ': ' + $path)
        }
      }
      if ((Get-Item $otherLayoutPath).GetValue('Ime File') -ne 'other.ime') {
        throw 'Legacy cleanup removed another input method.'
      }
      if ((Get-Item $substitutesPath).GetValue('DFFE0804')) {
        throw 'Legacy cleanup left a keyboard layout alias.'
      }
      $preload = Get-Item $preloadPath
      if (@($preload.GetValueNames() | ForEach-Object { $preload.GetValue($_) } |
          Where-Object { $_ -in @('E0FE0804', 'DFFE0804') }).Count) {
        throw 'Legacy cleanup left a preloaded keyboard layout.'
      }
    } finally {
      foreach ($value in $createdValues) {
        $key = Get-Item -LiteralPath $value[0] -ErrorAction SilentlyContinue
        if ($key -and $key.GetValue($value[1]) -in @('E0FE0804', 'DFFE0804')) {
          Remove-ItemProperty -LiteralPath $value[0] -Name $value[1]
        }
      }
      foreach ($path in $createdPaths) {
        if (Test-Path -LiteralPath $path) {
          Remove-Item -LiteralPath $path -Force
        }
      }
    }
  }

} finally {
  Set-WinUserLanguageList $originalLanguages -Force
  if ($null -eq $originalDisplayLanguage) {
    Set-WinUILanguageOverride
  } else {
    Set-WinUILanguageOverride -Language $originalDisplayLanguage
  }
  if ($null -eq $originalInputMethod) {
    Set-WinDefaultInputMethodOverride
  } else {
    Set-WinDefaultInputMethodOverride -InputTip $originalInputMethod.InputMethodTip
  }
}

Write-Output 'Weasel upgrade preserved language settings and shared IMM resources; uninstall cleaned them.'
