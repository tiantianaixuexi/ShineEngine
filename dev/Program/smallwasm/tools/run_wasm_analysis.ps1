param(
  [ValidateSet("debug","release")][string]$Mode = "debug",
  [int]$TopN = 20
)

$root = Split-Path -Parent $PSScriptRoot
$buildBat = Join-Path $root "build-wasm.bat"
$dwarfPy = Join-Path $PSScriptRoot "dwarf_report.py"
$mergePy = Join-Path $PSScriptRoot "wasm_size_merge.py"
$mapPs1 = Join-Path $PSScriptRoot "map_report.ps1"
$mapReportOut = Join-Path $root "output.gl.map.report.txt"

if (-not (Test-Path -LiteralPath $buildBat)) { throw "missing: $buildBat" }
if (-not (Test-Path -LiteralPath $dwarfPy)) { throw "missing: $dwarfPy" }
if (-not (Test-Path -LiteralPath $mergePy)) { throw "missing: $mergePy" }
if (-not (Test-Path -LiteralPath $mapPs1)) { throw "missing: $mapPs1" }

$modeArg = if ($Mode -eq "release") { "--release" } else { "--debug" }

Write-Host "[smallwasm] build ($Mode) ..."
& $buildBat $modeArg
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[smallwasm] dwarfdump + report ..."
python $dwarfPy
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[smallwasm] map report ..."
powershell -ExecutionPolicy Bypass -File $mapPs1 -OutPath $mapReportOut -TopN $TopN
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[smallwasm] merge report ..."
python $mergePy --top-n $TopN
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[smallwasm] done"
