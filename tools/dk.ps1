<#
.SYNOPSIS
  Runs a command inside the DSCore toolchain container, with the repo mounted at /project.
.EXAMPLE
  tools\dk.ps1 make
  tools\dk.ps1 make -C tests
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
docker build -q -t dscore-dev $root | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'docker build failed' }
docker run --rm -v "${root}:/project" -w /project dscore-dev @args
exit $LASTEXITCODE
