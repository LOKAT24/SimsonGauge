#requires -version 5.1
<#
.SYNOPSIS
  Buduje firmware i tworzy wersjonowany "wklad do procka" (firmware.bin).

.DESCRIPTION
  1. Buduje projekt PlatformIO (pio run).
  2. Kopiuje firmware.bin do  release\MotoGauge_<wersja>.bin  -- gotowe do OTA.
  3. Tworzy tag git <wersja>.
  4. Jesli masz gh CLI + remote na GitHubie: pushuje tag i tworzy GitHub Release
     z dolaczonym firmware.bin. W przeciwnym razie zostawia plik lokalnie i
     mowi, czego brakuje do publikacji.

  Firmware.bin to sama partycja aplikacji - dokladnie to, co przyjmuje Twoj
  OTA (Update.write w src/ota.cpp). Nie trzeba mergowac z bootloaderem.

.EXAMPLE
  .\scripts\release.ps1 v1.0.0
#>
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $Version
)

$ErrorActionPreference = 'Stop'

$EnvName = 'esp32-s3-amoled-1in43'
$Repo    = Split-Path -Parent $PSScriptRoot

if ($Version -notmatch '^v\d+\.\d+\.\d+$') {
    throw "Wersja musi byc w formacie vX.Y.Z (np. v1.0.0). Podano: $Version"
}

Push-Location $Repo
try {
    $BinSrc = ".pio\build\$EnvName\firmware.bin"
    $RelDir = "release"
    $BinDst = "$RelDir\MotoGauge_$Version.bin"

    # --- znajdz pio (PATH, a jak nie ma to penv PlatformIO) ---
    $pio = (Get-Command pio -ErrorAction SilentlyContinue).Source
    if (-not $pio) {
        $fallback = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
        if (Test-Path $fallback) { $pio = $fallback }
    }
    if (-not $pio) { throw "Nie znaleziono pio (PlatformIO Core)." }

    if (git status --porcelain) {
        Write-Warning "Masz niezacommitowane zmiany - wersja w firmware dostanie sufiks -dirty. Zacommituj, by wydac czysta wersje."
    }

    # --- tag git NAJPIERW: git_version.py czyta git describe podczas buildu ---
    $tagCreated = $false
    if (git tag --list $Version) {
        Write-Warning "Tag $Version juz istnieje - buduje z istniejacego tagu."
    } else {
        git tag -a $Version -m "Release $Version"
        $tagCreated = $true
        Write-Host "==> Utworzono tag $Version" -ForegroundColor Green
    }

    # --- build (FW_VERSION = $Version zostaje wkompilowany) ---
    Write-Host "==> Buduje firmware ($EnvName)..." -ForegroundColor Cyan
    & $pio run -e $EnvName
    if ($LASTEXITCODE -ne 0) {
        if ($tagCreated) {
            git tag -d $Version | Out-Null
            Write-Warning "Build nieudany - usunieto swiezo utworzony tag $Version."
        }
        throw "Build nie powiodl sie (pio run exit $LASTEXITCODE)."
    }
    if (-not (Test-Path $BinSrc)) { throw "Brak $BinSrc po buildzie." }

    # --- wersjonowany bin gotowy do OTA ---
    New-Item -ItemType Directory -Force -Path $RelDir | Out-Null
    Copy-Item $BinSrc $BinDst -Force
    $kb = [math]::Round((Get-Item $BinDst).Length / 1KB)
    Write-Host "==> Plik OTA gotowy: $BinDst ($kb KB)" -ForegroundColor Green

    # --- publikacja na GitHub (jesli skonfigurowane) ---
    $gh        = (Get-Command gh -ErrorAction SilentlyContinue).Source
    $hasRemote = [bool](git remote)

    if ($gh -and $hasRemote) {
        Write-Host "==> Publikuje Release na GitHubie..." -ForegroundColor Cyan
        git push origin $Version
        & $gh release create $Version $BinDst --title $Version --generate-notes
        if ($LASTEXITCODE -ne 0) { throw "gh release create exit $LASTEXITCODE." }
        Write-Host "==> Release $Version opublikowany." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Firmware gotowy lokalnie: $BinDst" -ForegroundColor Yellow
        Write-Host "Aby publikowac na GitHub Releases (konfiguracja jednorazowa):" -ForegroundColor Yellow
        if (-not $hasRemote) { Write-Host "  - utworz repo na GitHubie i:  git remote add origin <URL>" }
        if (-not $gh)        { Write-Host "  - zainstaluj gh:  winget install GitHub.cli   (potem: gh auth login)" }
        Write-Host "  Potem uruchom skrypt ponownie - doda sam Release (tag juz istnieje)."
    }
}
finally {
    Pop-Location
}
