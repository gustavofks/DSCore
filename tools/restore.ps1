<#
.SYNOPSIS
  Undoes tools\deploy.ps1 on an SD card root or a melonDS SD sync folder.
.PARAMETER Settings
  Also restores _nds\TWiLightMenu\settings.ini from the backup DSCore made before its first write.
#>
param(
	[Parameter(Mandatory = $true)][string]$Target,
	[switch]$Settings
)
$ErrorActionPreference = 'Stop'
$twl = Join-Path $Target '_nds\TWiLightMenu'

$orig = Join-Path $twl 'dsimenu.srldr.dscore-orig'
if (Test-Path $orig) {
	Move-Item $orig (Join-Path $twl 'dsimenu.srldr') -Force
	Write-Host 'Original dsimenu.srldr restored.'
} else {
	Write-Host 'No dsimenu.srldr backup found (nothing to restore).'
}

$app = Join-Path $Target 'dscore.nds'
if (Test-Path $app) {
	Remove-Item $app
	Write-Host 'dscore.nds removed.'
}

if ($Settings) {
	$bak = Join-Path $twl 'settings.ini.dscore-bak'
	if (Test-Path $bak) {
		Copy-Item $bak (Join-Path $twl 'settings.ini') -Force
		Write-Host 'settings.ini restored from settings.ini.dscore-bak.'
	} else {
		Write-Host 'No settings.ini.dscore-bak found.'
	}
}
