[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$Tip,

  [switch]$Uninstall,

  [switch]$Library
)

$ErrorActionPreference = 'Stop'

function Get-LanguageId([object]$Language) {
  try {
    return ([Globalization.CultureInfo]::GetCultureInfo(
        $Language.LanguageTag).LCID).ToString('X4')
  } catch {
    return $null
  }
}

# Return codes: 0 unchanged, 1 changed, 2 target language absent.
function Update-WeaselLanguageList(
  [System.Collections.IEnumerable]$LanguageList,
  [string]$ProfileTip,
  [switch]$Remove,
  [string[]]$LegacyLayouts = @()) {
  $changed = $false
  $profileLanguageId, $profileGuid = $ProfileTip -split ':', 2

  if (-not $Remove) {
    $target = @($LanguageList | Where-Object {
        (Get-LanguageId $_) -ieq $profileLanguageId
      }) | Select-Object -First 1
    if ($null -eq $target) {
      return 2
    }
  }

  foreach ($language in $LanguageList) {
    $remaining = @()
    foreach ($tip in $language.InputMethodTips) {
      $id = $tip.Substring($tip.IndexOf(':') + 1)
      if ($Remove -and ($id -ieq $profileGuid -or $LegacyLayouts -icontains $id)) {
        continue
      }
      if (-not $Remove -and $LegacyLayouts -icontains $id) {
        $tip = $tip.Substring(0, $tip.IndexOf(':') + 1) + $profileGuid
        $id = $profileGuid
      }
      if ($id -ieq $profileGuid -and $remaining -icontains $tip) {
        continue
      }
      $remaining += $tip
    }
    if (($remaining -join ',') -cne ($language.InputMethodTips -join ',')) {
      $language.InputMethodTips.Clear()
      foreach ($tip in $remaining) {
        [void]$language.InputMethodTips.Add($tip)
      }
      $changed = $true
    }
  }

  if (-not $Remove -and $target.InputMethodTips -inotcontains $ProfileTip) {
    [void]$target.InputMethodTips.Add($ProfileTip)
    $changed = $true
  }
  return [int]$changed
}

if ($Library) {
  return
}

$legacyLayouts = @(Get-ChildItem `
    'Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Keyboard Layouts' |
  Where-Object {
    $imeFile = $_.GetValue('Ime File')
    $_.PSChildName -match '^[0-9a-f]{8}$' -and
    $imeFile -is [string] -and $imeFile -ieq 'weasel.ime'
  })
$legacyIds = @($legacyLayouts | ForEach-Object { $_.PSChildName })
$substitutesPath = 'Registry::HKEY_CURRENT_USER\Keyboard Layout\Substitutes'
$aliases = @()
if ($legacyIds.Count -and (Test-Path $substitutesPath)) {
  $substitutes = Get-Item $substitutesPath
  $aliases = @($substitutes.GetValueNames() | Where-Object {
      $_ -match '^[0-9a-f]{8}$' -and $legacyIds -icontains $substitutes.GetValue($_)
    })
  $legacyIds += $aliases
}
$defaultTip = if ($legacyLayouts.Count) { (Get-WinDefaultInputMethodOverride).InputMethodTip }
$languageList = Get-WinUserLanguageList
$result = Update-WeaselLanguageList $languageList $Tip `
  -Remove:$Uninstall -LegacyLayouts $legacyIds
if ($result -eq 2) {
  exit 2
}
if ($result) {
  Set-WinUserLanguageList -LanguageList $languageList -Force
}

# Migrate this user's input methods; other accounts may still use the shared IMM.
if ($legacyLayouts.Count) {
  if (-not $Uninstall) {
    $updatedList = Get-WinUserLanguageList
    foreach ($language in $languageList) {
      $actual = @($updatedList | Where-Object { $_.LanguageTag -ieq $language.LanguageTag })
      if (($actual.InputMethodTips -join ',') -ine ($language.InputMethodTips -join ',')) {
        throw 'Input methods changed unexpectedly while migrating the legacy Weasel installation.'
      }
    }
  }
  if ($defaultTip -and $legacyIds -icontains ($defaultTip -split ':', 2)[1]) {
    if ($Uninstall) {
      Set-WinDefaultInputMethodOverride
    } else {
      Set-WinDefaultInputMethodOverride -InputTip (
        ($defaultTip -split ':', 2)[0] + ':' + ($Tip -split ':', 2)[1])
    }
  }
  # Windows may remove an old alias or reuse it for another keyboard.
  $legacyIds = @($legacyLayouts | ForEach-Object { $_.PSChildName })
  if ($aliases.Count -and (Test-Path $substitutesPath)) {
    $substitutes = Get-Item $substitutesPath
    $aliases = @($aliases | Where-Object {
        $value = $substitutes.GetValue($_)
        -not $value -or $legacyIds -icontains $value
      })
  }
  $legacyIds += $aliases
  $preloadPath = 'Registry::HKEY_CURRENT_USER\Keyboard Layout\Preload'
  if (Test-Path $preloadPath) {
    $preload = Get-Item $preloadPath
    $names = @($preload.GetValueNames() | Where-Object { $_ -match '^[1-9][0-9]*$' } |
      Sort-Object { [int]$_ })
    $remaining = @($names | ForEach-Object { $preload.GetValue($_) } |
      Where-Object { $legacyIds -inotcontains $_ })
    if ($remaining.Count -ne $names.Count) {
      for ($i = 0; $i -lt $remaining.Count; $i++) {
        New-ItemProperty -LiteralPath $preloadPath -Name ($i + 1) -Value $remaining[$i] `
          -PropertyType String -Force | Out-Null
      }
      foreach ($name in $names) {
        if ([int]$name -gt $remaining.Count) {
          Remove-ItemProperty -LiteralPath $preloadPath -Name $name
        }
      }
    }
  }
  if ($aliases.Count -and (Test-Path $substitutesPath)) {
    $substitutes = Get-Item $substitutesPath
    foreach ($alias in $aliases) {
      if ($legacyIds -icontains $substitutes.GetValue($alias)) {
        Remove-ItemProperty -LiteralPath $substitutesPath -Name $alias
      }
    }
  }
  if ($Uninstall) {
    foreach ($layout in $legacyLayouts) {
      Remove-Item -LiteralPath $layout.PSPath
    }
  }
}
exit 0
