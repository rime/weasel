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
  [switch]$Remove) {
  $changed = $false
  $profileLanguageId, $profileGuid = $ProfileTip -split ':', 2

  if ($Remove) {
    foreach ($language in $LanguageList) {
      $remaining = @($language.InputMethodTips | Where-Object {
          $_.Substring($_.IndexOf(':') + 1) -ine $profileGuid
        })
      if ($remaining.Count -ne @($language.InputMethodTips).Count) {
        $language.InputMethodTips.Clear()
        foreach ($tip in $remaining) {
          [void]$language.InputMethodTips.Add($tip)
        }
        $changed = $true
      }
    }
    return [int]$changed
  }

  $language = @($LanguageList | Where-Object {
      (Get-LanguageId $_) -ieq $ProfileLanguageId
    }) | Select-Object -First 1
  if ($null -eq $language) {
    return 2
  }

  if (-not @($language.InputMethodTips | Where-Object { $_ -ieq $ProfileTip })) {
    [void]$language.InputMethodTips.Add($ProfileTip)
    $changed = $true
  }
  return [int]$changed
}

if ($Library) {
  return
}

$languageList = Get-WinUserLanguageList
$result = Update-WeaselLanguageList $languageList $Tip -Remove:$Uninstall
if ($result -eq 2) {
  exit 2
}
if ($result) {
  Set-WinUserLanguageList -LanguageList $languageList -Force
}
exit 0
