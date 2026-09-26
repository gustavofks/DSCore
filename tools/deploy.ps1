<#
.SYNOPSIS
  Copies dscore.nds to an SD card root or a melonDS SD sync folder.
.PARAMETER Target
  Root of the SD card, e.g. E:\ or C:\workspace\melonds\sd_test
.PARAMETER Mode
  app   : copies to <Target>\dscore.nds (open it from TWiLight's file browser)
  srldr : replaces _nds\TWiLightMenu\dsimenu.srldr; the original is backed up once as
          dsimenu.srldr.dscore-orig
#>
param(
	[Parameter(Mandatory = $true)][string]$Target,
	[ValidateSet('app', 'srldr')][string]$Mode = 'app'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$nds = Join-Path $root 'dscore.nds'
$twl = Join-Path $Target '_nds\TWiLightMenu'

if (-not (Test-Path $nds)) { throw "dscore.nds not found. Run 'tools\dk.ps1 make' first." }
if (-not (Test-Path (Join-Path $twl 'main.srldr'))) { throw "TWiLight Menu++ not found in $twl" }

if ($Mode -eq 'app') {
	$dest = Join-Path $Target 'dscore.nds'
	Copy-Item $nds $dest -Force
	Write-Host "Copied to $dest"
} else {
	$srldr = Join-Path $twl 'dsimenu.srldr'
	$orig = Join-Path $twl 'dsimenu.srldr.dscore-orig'
	if (-not (Test-Path $orig)) {
		Copy-Item $srldr $orig
		Write-Host "Backup created: $orig"
	}
	Copy-Item $nds $srldr -Force
	Write-Host "DSCore installed as $srldr"
}
