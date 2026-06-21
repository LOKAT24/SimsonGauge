#requires -version 5.1
<#
.SYNOPSIS
  Buduje firmware i tworzy wersjonowany "wklad do procka" (firmware.bin).

.DESCRIPTION
  1. Tworzy tag git <wersja> (przed buildem, by wersja trafila do firmware).
  2. Buduje projekt PlatformIO (pio run).
  3. Kopiuje dwa pliki do release\:
       MotoGauge_<wersja>.bin       - sama aplikacja, do OTA
       MotoGauge_<wersja>_full.bin  - scalony obraz (bootloader+partycje+app),
                                      do wgrania programatorem na czysty uklad.
  4. Jesli masz gh CLI + remote na GitHubie: pushuje tag i tworzy GitHub Release
     z oboma plikami. W przeciwnym razie zostawia je lokalnie i mowi, czego
     brakuje do publikacji.

  - firmware.bin to sama partycja aplikacji - to przyjmuje Twoj OTA
    (Update.write w src/ota.cpp); wgrywany "na zywo" na dzialajace urzadzenie.
  - firmware.factory.bin to caly obraz flash pod adres 0x0 - dla czystego ESP32
    przez esptool:  esptool.py write_flash 0x0 MotoGauge_<wersja>_full.bin

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
    $BinSrc     = ".pio\build\$EnvName\firmware.bin"
    $FactorySrc = ".pio\build\$EnvName\firmware.factory.bin"
    $RelDir     = "release"
    $BinDst     = "$RelDir\MotoGauge_$Version.bin"
    $FullDst    = "$RelDir\MotoGauge_${Version}_full.bin"

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
    if (-not (Test-Path $BinSrc))     { throw "Brak $BinSrc po buildzie." }
    if (-not (Test-Path $FactorySrc)) { throw "Brak $FactorySrc po buildzie." }

    # --- wersjonowane pliki: app (OTA) + scalony obraz (programator) ---
    New-Item -ItemType Directory -Force -Path $RelDir | Out-Null
    Copy-Item $BinSrc     $BinDst  -Force
    Copy-Item $FactorySrc $FullDst -Force
    $kbApp  = [math]::Round((Get-Item $BinDst).Length  / 1KB)
    $kbFull = [math]::Round((Get-Item $FullDst).Length / 1KB)
    Write-Host "==> Plik OTA (app):       $BinDst  ($kbApp KB)"  -ForegroundColor Green
    Write-Host "==> Plik pelny (0x0):     $FullDst ($kbFull KB)" -ForegroundColor Green

    # --- publikacja na GitHub (jesli skonfigurowane) ---
    $gh        = (Get-Command gh -ErrorAction SilentlyContinue).Source
    $hasRemote = [bool](git remote)

    if ($gh -and $hasRemote) {
        Write-Host "==> Publikuje Release na GitHubie..." -ForegroundColor Cyan
        git push origin $Version
        & $gh release create $Version $BinDst $FullDst --title $Version --generate-notes
        if ($LASTEXITCODE -ne 0) { throw "gh release create exit $LASTEXITCODE." }
        Write-Host "==> Release $Version opublikowany." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Pliki gotowe lokalnie:" -ForegroundColor Yellow
        Write-Host "  $BinDst   (OTA / aktualizacja)" -ForegroundColor Yellow
        Write-Host "  $FullDst  (programator / czysty uklad, write_flash 0x0)" -ForegroundColor Yellow
        Write-Host "Aby publikowac na GitHub Releases (konfiguracja jednorazowa):" -ForegroundColor Yellow
        if (-not $hasRemote) { Write-Host "  - utworz repo na GitHubie i:  git remote add origin <URL>" }
        if (-not $gh)        { Write-Host "  - zainstaluj gh:  winget install GitHub.cli   (potem: gh auth login)" }
        Write-Host "  Potem uruchom skrypt ponownie - doda sam Release (tag juz istnieje)."
    }
}
finally {
    Pop-Location
}
