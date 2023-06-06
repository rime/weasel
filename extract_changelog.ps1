$tag = $env:GITHUB_REF -replace 'refs/tags/'
$changelogPath = Join-Path $PSScriptRoot "CHANGELOG.md"
$outputPath = Join-Path $PSScriptRoot "RELEASE_CHANGELOG.md"

$changeLog = Get-Content $changelogPath
Out-File -FilePath $outputPath -NoNewline

$found = $false
foreach ($line in $changeLog) {
  $versionLine = $line -match '<a name="(.*)"></a>'
  if ($versionLine) {
    $version = $Matches.1
    if (-Not $found) {
      if ($version -ne $tag) {
        # version mismatch
        Write-Output "version mismatch: changelog is ${version} but tag is ${tag}"
        exit 1
      } else {
        $found = $true
        Write-Output "extracting changelog for ${version}"
        continue
      }
    } else {
      exit 0
    }
  }
  $line | Out-File -FilePath $outputPath -Append
}
