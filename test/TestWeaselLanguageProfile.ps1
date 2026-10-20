$ErrorActionPreference = 'Stop'

$scriptPath = Join-Path $PSScriptRoot '..\WeaselSetup\WeaselLanguageProfile.ps1'
$tipHans = '0804:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-1C9267529467}'
$tipHant = '0404:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-1C9267529467}'

. $scriptPath -Tip $tipHans -Library

function New-TestLanguage([string]$tag, [string[]]$tips) {
  $language = (New-WinUserLanguageList $tag)[0]
  $language.InputMethodTips.Clear()
  foreach ($tip in $tips) {
    [void]$language.InputMethodTips.Add($tip)
  }
  return $language
}

function Assert-Test([bool]$condition, [string]$message) {
  if (-not $condition) {
    throw $message
  }
}

function New-TestRegistryKey([hashtable]$Values) {
  $key = [pscustomobject]@{ Values = $Values }
  $key | Add-Member ScriptMethod GetValueNames { return @($this.Values.Keys) }
  $key | Add-Member ScriptMethod GetValue { param($Name) return $this.Values[$Name] }
  return $key
}

$languages = @(
  (New-TestLanguage 'zh-Hans-CN' @('0804:00000409')),
  (New-TestLanguage 'en-US' @('0409:00000409'))
)
$languageOrder = $languages.LanguageTag -join ','

Assert-Test ((Get-LanguageId $languages[0]) -eq '0804') `
  'Simplified Chinese was mapped to the wrong language ID.'
Assert-Test ((Get-LanguageId $languages[1]) -eq '0409') `
  'English was mapped to the wrong language ID.'

$result = Update-WeaselLanguageList $languages $tipHans
Assert-Test ($result -eq 1) 'A missing Simplified Chinese profile was not added.'
Assert-Test ($languages[0].InputMethodTips.Count -eq 2) 'The Weasel profile was added to the wrong language.'
Assert-Test ($languages[1].InputMethodTips.Count -eq 1) 'The English input methods changed.'

$result = Update-WeaselLanguageList $languages $tipHans
Assert-Test ($result -eq 0) 'Adding an existing profile was not idempotent.'
Assert-Test ($languages[0].InputMethodTips[0] -eq '0804:00000409') `
  'Existing input method order changed.'
Assert-Test ($languages[0].InputMethodTips[1] -ieq $tipHans) `
  'The Weasel profile was not appended after existing input methods.'

$result = Update-WeaselLanguageList $languages $tipHant
Assert-Test ($result -eq 2) 'A missing target language was treated as installed.'
Assert-Test ($languages[0].InputMethodTips.Count -eq 2) `
  'The language list changed when the target language was absent.'
Assert-Test ($languages[1].InputMethodTips.Count -eq 1) `
  'The English input methods changed when the target language was absent.'

$traditional = @(New-TestLanguage 'zh-Hant-TW' @('0404:00000409'))
$result = Update-WeaselLanguageList $traditional $tipHant
Assert-Test ($result -eq 1) 'The Traditional Chinese profile was not added.'

$result = Update-WeaselLanguageList @($languages + $traditional) $tipHans -Remove
Assert-Test ($result -eq 1) 'The Weasel profile was not removed.'
Assert-Test ($languages[0].InputMethodTips.Count -eq 1) 'Uninstall removed another input method.'
Assert-Test ($languages[1].InputMethodTips.Count -eq 1) 'Uninstall changed another language.'
Assert-Test ($traditional[0].InputMethodTips.Count -eq 1) `
  'Uninstall left a Weasel profile under another Chinese language.'
Assert-Test (($languages.LanguageTag -join ',') -eq $languageOrder) `
  'The language order changed.'

foreach ($profile in @(@('zh-HK', '0C04'), @('zh-MO', '1404'), @('zh-SG', '1004'))) {
  $regional = @(New-TestLanguage $profile[0] @())
  $tip = $profile[1] + $tipHans.Substring(4)
  Assert-Test ((Update-WeaselLanguageList $regional $tip) -eq 1) `
    ('The regional Chinese profile was not added: ' + $profile[0])
}

$legacyIds = @('E0FE0804', 'E0FD0404', 'D0000804')
$legacy = @(
  (New-TestLanguage 'zh-CN' @('0804:E0FE0804', '0804:00000409')),
  (New-TestLanguage 'zh-TW' @('0404:E0FD0404')),
  (New-TestLanguage 'en-US' @('0409:00000409'))
)
$result = Update-WeaselLanguageList $legacy $tipHans -LegacyLayouts $legacyIds
Assert-Test ($result -eq 1) 'Legacy input methods were not migrated.'
Assert-Test (($legacy[0].InputMethodTips -join ',') -eq ($tipHans + ',0804:00000409')) `
  'Migration changed the input method order or another keyboard.'
Assert-Test ($legacy[1].InputMethodTips[0] -eq $tipHant) `
  'The Traditional Chinese legacy input method was not migrated.'
Assert-Test ($legacy[2].InputMethodTips[0] -eq '0409:00000409') `
  'Migration changed an unrelated language.'
$result = Update-WeaselLanguageList $legacy $tipHans -LegacyLayouts $legacyIds
Assert-Test ($result -eq 0) 'Migration was not idempotent.'

$duplicates = @(New-TestLanguage 'zh-CN' @('0804:D0000804', $tipHans, '0804:E0FE0804'))
$result = Update-WeaselLanguageList $duplicates $tipHans -LegacyLayouts $legacyIds
Assert-Test ($result -eq 1 -and $duplicates[0].InputMethodTips.Count -eq 1) `
  'Migration duplicated an existing TSF profile.'
$tipPinyin = '0804:{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}{FA550B04-5AD7-411F-A5AC-CA038EC515D7}'
foreach ($tips in @(
    @('0804:E0FE0804', $tipPinyin, $tipHans),
    @($tipHans, $tipPinyin, '0804:E0FE0804'))) {
  $duplicates = @(New-TestLanguage 'zh-CN' $tips)
  $result = Update-WeaselLanguageList $duplicates $tipHans -LegacyLayouts $legacyIds
  Assert-Test ($result -eq 1) 'Separated legacy and TSF duplicates were not merged.'
  Assert-Test (($duplicates[0].InputMethodTips -join ',') -eq ($tipHans + ',' + $tipPinyin)) `
    'Migration moved Weasel behind another input method.'
  Assert-Test ((Update-WeaselLanguageList $duplicates $tipHans -LegacyLayouts $legacyIds) -eq 0) `
    'Deduplicating separated Weasel profiles was not idempotent.'
}
$absent = @(New-TestLanguage 'zh-CN' @('0804:E0FE0804'))
$result = Update-WeaselLanguageList $absent $tipHant -LegacyLayouts $legacyIds
Assert-Test ($result -eq 2 -and $absent[0].InputMethodTips[0] -eq '0804:E0FE0804') `
  'Migration removed a working legacy input method when the target language was absent.'
$result = Update-WeaselLanguageList $absent $tipHans -Remove -LegacyLayouts $legacyIds
Assert-Test ($result -eq 1 -and $absent[0].InputMethodTips.Count -eq 0) `
  'Uninstall left a legacy input method enabled.'

# Exercise the script entry point without writing the real user language list.
function Get-ChildItem {
  if ($testState.Layouts) { return $testState.Layouts }
}
function Test-Path([string]$LiteralPath) {
  return $testState.Registry -and $testState.Registry.ContainsKey($LiteralPath)
}
function Get-Item([string]$LiteralPath) { return $testState.Registry[$LiteralPath] }
function New-ItemProperty { throw 'An unrelated registry value was changed.' }
function Remove-ItemProperty { throw 'An unrelated registry value was removed.' }
function Get-WinDefaultInputMethodOverride {
  return [pscustomobject]@{ InputMethodTip = $testState.DefaultTip }
}
function Set-WinDefaultInputMethodOverride([string]$InputTip) {
  $testState.DefaultTip = $InputTip
}
function Remove-Item([string]$LiteralPath) { $testState.Removed += $LiteralPath }
function Get-WinUserLanguageList { return ,$testState.Languages }
function Set-WinUserLanguageList($LanguageList, [switch]$Force) {
  Assert-Test $Force.IsPresent 'The language list write was not non-interactive.'
  $testState.Writes++
  if ($testState.ReuseAlias) {
    $testState.Registry[$substitutesPath].Values['DFFE0804'] = '00000409'
  }
  if ($testState.LoseLanguage) {
    $testState.Languages = New-WinUserLanguageList 'en-US'
  }
}

$testState = @{ Languages = New-WinUserLanguageList 'zh-CN'; Writes = 0 }
& $scriptPath -Tip $tipHans
Assert-Test ($LASTEXITCODE -eq 0) 'A successful update returned a failure code.'
Assert-Test ($testState.Writes -eq 1) 'A changed language list was not written.'
& $scriptPath -Tip $tipHans
Assert-Test ($LASTEXITCODE -eq 0) 'An unchanged list returned a failure code.'
Assert-Test ($testState.Writes -eq 1) 'An unchanged language list was written.'
& $scriptPath -Tip $tipHant
Assert-Test ($LASTEXITCODE -eq 2) 'A missing language did not return exit code 2.'
Assert-Test ($testState.Writes -eq 1) 'A missing language caused a write.'
& $scriptPath -Tip $tipHans -Uninstall
Assert-Test ($LASTEXITCODE -eq 0) 'Uninstall returned a failure code.'
Assert-Test ($testState.Writes -eq 2) 'Uninstall did not write the updated list.'
& $scriptPath -Tip $tipHans -Uninstall
Assert-Test ($LASTEXITCODE -eq 0) 'Repeated uninstall returned a failure code.'
Assert-Test ($testState.Writes -eq 2) 'Repeated uninstall caused a write.'

$testState.Languages = New-WinUserLanguageList 'en-US'
& $scriptPath -Tip $tipHans
Assert-Test ($LASTEXITCODE -eq 2) 'An English-only list was treated as Chinese.'
Assert-Test ($testState.Languages.Count -eq 1) 'A missing Chinese language was created.'
Assert-Test ($testState.Writes -eq 2) 'An English-only language list was written.'

$layout = [pscustomobject]@{ PSChildName = 'E0FE0804'; PSPath = 'Registry::TestWeaselLayout' }
$layout | Add-Member ScriptMethod GetValue { return 'weasel.ime' }
$testState = @{
  Languages = @((New-TestLanguage 'zh-CN' @('0804:E0FE0804')))
  Layouts = @($layout)
  DefaultTip = '0804:E0FE0804'
  Removed = @()
  Writes = 0
}
& $scriptPath -Tip $tipHans
Assert-Test ($testState.Languages[0].InputMethodTips[0] -eq $tipHans) `
  'The legacy input method was not migrated.'
Assert-Test ($testState.DefaultTip -eq $tipHans) 'The legacy default input method was not migrated.'
Assert-Test ($testState.Removed.Count -eq 0) 'Migration removed the shared legacy registration.'
$testState.DefaultTip = '0804:E0FE0804'
& $scriptPath -Tip $tipHans -Uninstall
Assert-Test (-not $testState.DefaultTip) 'Uninstall left an obsolete default input method override.'
Assert-Test ($testState.Removed -contains $layout.PSPath) 'Uninstall left the shared legacy registration.'

$substitutesPath = 'Registry::HKEY_CURRENT_USER\Keyboard Layout\Substitutes'
$preloadPath = 'Registry::HKEY_CURRENT_USER\Keyboard Layout\Preload'
$testState.Registry = @{
  $substitutesPath = (New-TestRegistryKey @{ DFFE0804 = 'E0FE0804' })
  $preloadPath = (New-TestRegistryKey @{ '1' = '00000409'; '2' = 'DFFE0804' })
}
$testState.Languages = @((New-TestLanguage 'zh-CN' @('0804:DFFE0804')))
$testState.ReuseAlias = $true
& $scriptPath -Tip $tipHans
Assert-Test ($testState.Registry[$preloadPath].Values['2'] -eq 'DFFE0804') `
  'Migration removed a preloaded alias that Windows reused for another keyboard.'
$testState.Registry = @{}
$testState.ReuseAlias = $false

$testState.Languages = @((New-TestLanguage 'zh-CN' @('0804:E0FE0804')))
$testState.LoseLanguage = $true
$testState.Removed = @()
try {
  & $scriptPath -Tip $tipHans
  throw 'An unsuccessful migration was accepted.'
} catch {
  Assert-Test ($_.Exception.Message -like 'Input methods changed unexpectedly*') `
    'An unsuccessful migration was not detected.'
}
Assert-Test ($testState.Removed.Count -eq 0) 'A failed migration removed the legacy registration.'

$testState.Layouts = @()
$testState.Languages = New-WinUserLanguageList 'zh-CN'
function Set-WinUserLanguageList { throw 'Language list write failed.' }
try {
  & $scriptPath -Tip $tipHans
  throw 'A write failure was ignored.'
} catch {
  Assert-Test ($_.Exception.Message -eq 'Language list write failed.') `
    'A write failure was not propagated.'
}

Write-Output 'Weasel language profile tests passed.'
exit 0
