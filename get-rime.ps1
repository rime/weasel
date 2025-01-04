param (
  [switch]$h, [string]$help, [string]$proxy, [string]$tag, [string]$os, [string]$build_variant, [boolean]$extract, [string]$use
)
if ($h -or $help -eq "--help") {
  $msg = "
  get-rime.ps1

    a Powershell script to get rime lib files

  -h                             show help message
  --help                         show help message
  -proxy [string]                default no proxy, format like http://yourdomain:port
  -tag [string]                  default the latest public release, or tag_name like 1.11.2 or latest(for nightly build)
  -os [string]                   default your running system if it's not set, Windows or macOS optional
  -build_variant [string]        default msvc for Windows, universal for macOS. clang and mingw are optional for Windows too
  -extract [boolean]             default false, 7z in PATH is required
  -use [string]                  dev is for building weasel, weasel is for common usage in weasel(update rime.dll)

  All these params are optional.
  To use some kind of mirror of github, set it up in ~/.git-rime.conf.ps1
  just like example bellow:

  # -----------------------------------------------------------------------------
  # for github mirror
  # get-rime api url
  `$global:api_pat = 'https://api.github.com/repos/rime/librime/releases/'

  # modify download url with -replace in get-rime.ps1, every download link
  `$global:url_pat = 'github'
  `$global:url_replace = 'github'

  # proxy setting, should be set in ~/.get-rime.conf.ps1
  #`$global:proxy_ = 'http://127.0.0.1:8008'

  # only for handling rate limit issue
  # if `$api_pat is not original, don't touch this for security
  #`$global:authorization = '<your token>'
  # -----------------------------------------------------------------------------
  "
  Write-Host $msg
  exit
}
function SafeExit {
  function SafeDelVars {
    param ( $vars)
    $vars | ForEach-Object {
      try {
        Get-Variable -Name $_ -ErrorAction Stop > $null
        Remove-Variable -Name $_ -Scope Global
      } catch { }
    }
  }
  SafeDelVars @("authorization", "proxy_", "api_pat", "url_pat", "url_replace")
  exit
}
# set $os if $os not provide
if (!$os) {
  if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    $os = "Windows"
  } elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) {
    $os = "macOS"
  }
}
# eixt if $os is not Windows or macOS
if (($os -ne "Windows") -and ($os -ne "macOS")) {
  Write-Host "OS $os not supported, Windows or macOS is required."
  SafeExit
}
# set patterns
if ($os -eq "Windows") {
  if (!$build_variant -or ($build_variant -eq "msvc")) {
    $build_variant = "msvc"
    $pattern = ""
    if ($use -eq "weasel") {
      $pattern = "rime-[0-9a-fA-F]+-" + $os + "-" + $build_variant + "-x(64|86)\.7z"
    } else {
      $pattern = "rime-(deps-)?[0-9a-fA-F]+-" + $os + "-" + $build_variant + "-x(64|86)\.7z"
    }
  } elseif ($build_variant -eq "mingw") {
    $pattern = "rime-[0-9a-fA-F]+-" + $os + "-" + $build_variant + "\.tar.bz2"
  } elseif ($build_variant -eq "clang") {
    $pattern = "rime-(deps-)?[0-9a-fA-F]+-" + $os + "-" + $build_variant + "-x64\.7z"
  }
  $home_dir = $env:UserProfile
} else {
  $build_variant = "universal"
  $pattern = "rime-(deps-)?[0-9a-fA-F]+-" + $os + "-" + $build_variant + "\.tar.bz2"
  $home_dir = $HOME
}
if ($PSBoundParameters.ContainsKey("use") -and ($use -eq "weasel" -or $use -eq "dev")) {
  $extract = $true
}
# -Parallel require Powershell 7.0 or greater, default try to use it
$parallel = ($PSVersionTable.PSVersion.Major -ge 7)
# proxy credential
# $username = "your_proxy_username"
# $password = ConvertTo-SecureString "your_proxy_password" -AsPlainText -Force
# $proxyCredential = New-Object System.Management.Automation.PSCredential($username, $password)

# set GitHub API URL
$global:api_pat = "https://api.github.com/repos/rime/librime/releases/"
$global:url_pat = "github"
$global:url_replace = "github"
# if ~/.get-rime.conf.ps1 exist, source it
if (Test-Path "$home_dir/.get-rime.conf.ps1") { & "$home_dir/.get-rime.conf.ps1" }
# if $api_pat not set, use the original api url, in case of conf files not exist
if (!$api_pat) { $api_pat = "https://api.github.com/repos/rime/librime/releases/" }
if ($tag) {
  $apiUrl = $api_pat + "tags/$tag"
} else {
  $apiUrl = $api_pat + "latest"
}
$webRequestParams = @{
  Uri = $apiUrl
  Headers = @{ Accept = "application/vnd.github.v3+json" }
}
if (($api_pat -match "^https://api.github.com.*$") -and $authorization) {
  $webRequestParams.Headers.Authorization = "token $authorization"
} elseif ($authorization) {
  Write-Host "⛔ Caution:
  $api_pat is not original api url
  authorization should be removed or commented for security"
  SafeExit
}

if (!$proxy -and $proxy_) {
  $proxy = $proxy_
}
if ($proxy) {
  $webRequestParams.Proxy = $proxy
  $webRequestParams.ProxyCredential = $proxyCredential
}
# try request response
try {
  $response = Invoke-RestMethod @webRequestParams
} catch {
  Write-Host "❌ Error Invoke-RestMethod @webRequestParams: $($_.Exception.Message)"
  SafeExit
}
# check 7z available
try {
  $null = Get-Command 7z -ErrorAction Stop
  $cmdOk = $true
} catch {
  $cmdOk = $false
  if($extract) {
    Write-Host "❌ Error: 7z is not available. Maybe 7z is not in PATH or not installed"
    SafeExit
  }
}
# check 64 bit
function Is64Bit {
  function IsWin11OrGreater {
    $osVersion = (Get-CimInstance Win32_OperatingSystem).Version
    # Windows 11 的版本号从 10.0.22000.0 开始
    if ([Version]$osVersion -ge [Version]"10.0.22000.0") {
        return $true
    } else {
        return $false
    }
  }
  $is64BitOs = [Environment]::Is64BitOperatingSystem
  $processorArchitecture = (Get-CimInstance Win32_Processor).Architecture
  switch ($processorArchitecture) {
      9 { $arch = "x64" }
      12 { $arch = "ARM64" }
      Default { $arch = "其他" }
  }
  if (IsWin11OrGreater) {
    if ($arch -eq "ARM64" -and $is64BitOs) { #ARM64
      return $true
    } elseif ($arch -eq "x64" -and $is64BitOs) {
      return $true
    } else {
      return $false
    }
  } else {
    if ($arch -eq "x64" -and $is64BitOs) {
      return $true
    } else {
      return $false
    }
  }
}
if ($os -eq "Windows" -and $use -eq "weasel") {
  if (Is64Bit) {
    $pattern = "rime-[0-9a-fA-F]+-" + $os + "-" + $build_variant + "-x64\.7z"
  } else {
    $pattern = "rime-[0-9a-fA-F]+-" + $os + "-" + $build_variant + "-x86\.7z"
  }
}
# check if file already downloaded
$ignore_urls = @()
$files_current_dir = Get-ChildItem -Path . -Name
if ($null -ne $response.assets -and $response.assets.Count -gt 0) {
  $response.assets | ForEach-Object {
    $url = $_.browser_download_url
    if ($url -match "http.*\/([^/]+)$") {
      $filename = $matches[1]
      if ($files_current_dir -contains $filename) {
        $file_info = Get-ChildItem -Path . $filename -ErrorAction SilentlyContinue
        # file size the same as the source
        if ($file_info.Length -eq $_.size) {
          $ignore_urls += $url
        }
      }
    }
  }
}
# start to download files
$fileNames = @()
if ($null -ne $response.assets -and $response.assets.Count -gt 0) {
  if ($parallel) {
    $fileNames += $response.assets | ForEach-Object -Parallel {
      $url = $_.browser_download_url
      if ($using:ignore_urls -contains $url -and $url -match $using:pattern) {
        $fileName = [System.IO.Path]::GetFileName($url)
        Write-Host "☑  $fileName is already in the current directory."
        return $fileName
      }
      if ($using:url_pat -and $using:url_replace) {
        $url = $url -replace $using:url_pat, $using:url_replace
      }
      if ($url -match $using:pattern) {
        $fileName = [System.IO.Path]::GetFileName($url)
        Write-Host "⏳ Downloading $fileName..."
        $downloadParams = @{
          Uri = $url
          OutFile = $fileName
        }
        if ($using:proxy) {
          $downloadParams.Proxy = $using:proxy
          $downloadParams.ProxyCredential = $using:webRequestParams.ProxyCredential
        }
        try {
          Invoke-WebRequest @downloadParams
          Write-Host "☑  Downloaded $fileName to the current directory."
          return $fileName
        } catch {
          Write-Host "❌ Error downloading ${fileName}: $($_.Exception.Message)"
        }
      }
    } -ThrottleLimit 4
  } else {
    $response.assets | ForEach-Object {
      $url = $_.browser_download_url
      if ($url_pat -and $url_replace) {
        $url = $url -replace $url_pat, $url_replace
      }
      if ($ignore_urls -contains $url -and $url -match $pattern) {
        $fileName = [System.IO.Path]::GetFileName($url)
        Write-Host "☑  $fileName is already in the current directory."
        $fileNames += $fileName
      } else {
        if ($url -match $pattern) {
          $fileName = [System.IO.Path]::GetFileName($url)
          Write-Host "⏳ Downloading $fileName..."
          $downloadParams = @{
            Uri = $url
            OutFile = $fileName
          }
          if ($proxy) {
            $downloadParams.Proxy = $proxy
            $downloadParams.ProxyCredential = $webRequestParams.proxyCredential
          }
          try {
            Invoke-WebRequest @downloadParams
            Write-Host "☑  Downloaded $fileName to the current directory."
            $fileNames += $fileName
          } catch {
            Write-Host "❌ Error downloading ${fileName}: $($_.Exception.Message)"
          }
        }
      }
    }
  }
  # $fileNames not empty, 7z available, and $extract is $true, to extract files
  if ($fileNames -and $cmdOk -and $extract) {
    $fileNames.GetEnumerator() | ForEach-Object {
      $outPath = $_ -replace "(\.7z|\.tar\.bz2)|(deps-)", ''
      if ((-not $hash) -and $outPath -match "rime-([0-9a-fA-F]+)(.*$)") {
        $hash = $matches[1]
      }
      if ($_ -match ".*\.tar\.bz2") {
        $tmpTarFile = $_ -replace '.bz2$', ''
        7z x $_ > $null
        7z x $tmpTarFile -o"$outPath" -y > $null
        Remove-Item $tmpTarFile > $null
      } else {
        7z x $_ * -o"$outPath" -y > $null
      }
      Write-Host "☑  Extracted $_ to $outPath"
    }
    if ($os -eq "Windows" -and $build_variant -eq "msvc") {
      $dir86 = Get-ChildItem -Path $("rime-"+$hash+"-*x86") -Directory
      $dir64 = Get-ChildItem -Path $("rime-"+$hash+"-*x64") -Directory
      function MyCopyItem {
        param ([string]$src, [string]$subpath, [string]$dest)
        if (-not (Test-Path -Path $dest)) {
          New-Item -ItemType Directory -Path $dest > $null
        }
        if (Test-Path (Join-Path -Path $src -ChildPath $subpath)) {
          Copy-Item "$src\$subpath" $dest -ErrorAction Stop
          Write-Host "☑  $(Split-Path $src -Leaf)\$subpath has been copied to $dest"
        }
      }
      function KillWeaselServer {
        $processName = "WeaselServer"
        $process = Get-Process $processName -ErrorAction SilentlyContinue
        while ($process) {
          if ($process) {
            Stop-Process -Name $processName -Force -ErrorAction SilentlyContinue
          }
          Start-Sleep -Seconds 0.5
          $process = Get-Process $processName -ErrorAction SilentlyContinue
          if (-not $process) {
            Write-Host "☑  $processName has been killed"
          }
        }
      }
      if ( $use -eq "dev"){
        if ((Test-Path ".\include") `
        -and (Test-Path ".\lib") -and (Test-Path ".\lib64") `
        -and (Test-Path ".\output\Win32")) {
          KillWeaselServer
          Remove-Item include\rime_*.h -ErrorAction SilentlyContinue
          MyCopyItem -src $dir86 -subpath "dist\include\rime_*.h" -dest "include\"
          MyCopyItem -src $dir86 -subpath "dist\lib\rime.lib"     -dest "lib\"
          MyCopyItem -src $dir86 -subpath "dist\lib\rime.dll"     -dest "output\Win32\"
          MyCopyItem -src $dir86 -subpath "dist\lib\rime.pdb"     -dest "output\Win32\"
          MyCopyItem -src $dir64 -subpath "dist\lib\rime.lib"     -dest "lib64\"
          MyCopyItem -src $dir64 -subpath "dist\lib\rime.dll"     -dest "output\"
          MyCopyItem -src $dir64 -subpath "dist\lib\rime.pdb"     -dest "output\"
          MyCopyItem -src $dir64 -subpath "share\opencc\*.*"      -dest "output\data\opencc\"
          Remove-Item -Path $dir64 -Recurse -Force -Confirm:$false -ErrorAction SilentlyContinue
          Remove-Item -Path $dir86 -Recurse -Force -Confirm:$false -ErrorAction SilentlyContinue
        } else {
          Write-Host "❌ current directory is not a weasel source directory"
        }
      } elseif ($use -eq "weasel") {
        if ([Environment]::Is64BitOperatingSystem) {
          $registryPath = "HKLM:\SOFTWARE\WOW6432Node\Rime\Weasel"
        } else {
          $registryPath = "HKLM:\SOFTWARE\Rime\Weasel"
        }
        try {
          $weaselRoot = (Get-ItemProperty -Path $registryPath -ErrorAction Stop).'WeaselRoot'
          $servercmd = Join-Path -Path $weaselRoot -ChildPath "WeaselServer.exe"
          KillWeaselServer
          $processName = "WeaselServer"
          $dllbit64 = Is64Bit
          MyCopyItem -src $(if ($dllbit64) { $dir64 } else { $dir86 }) -subpath "dist\lib\rime.dll" -dest $weaselRoot
          MyCopyItem -src $(if ($dllbit64) { $dir64 } else { $dir86 }) -subpath "dist\lib\rime.pdb" -dest $weaselRoot
          if ($dllbit64) {
            Remove-Item -Path $dir64 -Recurse -Force -Confirm:$false -ErrorAction SilentlyContinue
          } else {
            Remove-Item -Path $dir86 -Recurse -Force -Confirm:$false -ErrorAction SilentlyContinue
          }
          Start-Process $servercmd -WorkingDirectory $weaselRoot
          if ($(Get-Process $processName -ErrorAction SilentlyContinue)) {
            Write-Host "☑  $processName has been started"
          }
        } catch [System.UnauthorizedAccessException] {
          Write-Host "❗ $_ please run as Administrator!"
          Start-Process $servercmd -WorkingDirectory $weaselRoot
          Write-Host "☑  $processName started"
          SafeExit
        } catch {
          Write-Host "❌ Error: $_"
        }
      }
    }
  }
} else {
  Write-Host "❌ Error: No assets found for the latest release."
}
SafeExit
