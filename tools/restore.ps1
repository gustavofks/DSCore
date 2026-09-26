<#
.SYNOPSIS
  Undoes tools\deploy.ps1 on an SD card root or a melonDS SD sync folder.
.PARAMETER Settings
  Also restores TWiLight's settings.ini and nds-bootstrap.ini from the backups DSCore made before its
  first edit of each.
.PARAMETER Data
  Also deletes DSCore's own data (sd:/_nds/DSCore: library cache, favorites, history, config, log).
#>
param(
	[Parameter(Mandatory = $true)][string]$Target,
	[switch]$Settings,
	[switch]$Data
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
	$backups = @(
		@{ File = Join-Path $twl 'settings.ini'; Backup = Join-Path $twl 'settings.ini.dscore-bak' },
		@{ File = Join-Path $Target '_nds\nds-bootstrap.ini'; Backup = Join-Path $Target '_nds\nds-bootstrap.ini.dscore-bak' }
	)
	foreach ($b in $backups) {
		if (Test-Path $b.Backup) {
			Copy-Item $b.Backup $b.File -Force
			Write-Host "$(Split-Path -Leaf $b.File) restored from $(Split-Path -Leaf $b.Backup)."
		} else {
			Write-Host "No $(Split-Path -Leaf $b.Backup) found."
		}
	}
}

if ($Data) {
	$dataDir = Join-Path $Target '_nds\DSCore'
	if (Test-Path $dataDir) {
		Remove-Item -Recurse -Force $dataDir
		Write-Host 'DSCore data removed.'
	}
}
