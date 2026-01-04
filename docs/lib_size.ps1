# Analyze-LibSize.ps1
# Analyze .lib file size composition script

param(
    [Parameter(Mandatory=$true)]
    [string]$LibPath,
    
    [int]$TopCount = 20,          # Display top N object files
    [switch]$ExportToCSV,         # Export to CSV
    [switch]$ShowSymbols,         # Show symbol info
    [switch]$ShowDetails          # Show details
)

# Use verbose preference
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $ShowDetails = $true
}

# Check if file exists
if (-not (Test-Path $LibPath)) {
    Write-Error "File not found: $LibPath"
    exit 1
}

# Function to check VS tools
function Test-VSTools {
    $dumpbinPath = $null
    $libPath = $null
    
    # Try to find VS from default paths
    $vsPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\*\bin\Hostx64\x64",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\*\bin\Hostx64\x64"
    )
    
    foreach ($pathPattern in $vsPaths) {
        $paths = @(Resolve-Path $pathPattern -ErrorAction SilentlyContinue)
        if ($paths -and $paths.Count -gt 0) {
            $path = $paths[0].Path
            $dumpbinPath = Join-Path $path "dumpbin.exe"
            $libPath = Join-Path $path "lib.exe"
            if (Test-Path $dumpbinPath) {
                return @{ Dumpbin = $dumpbinPath; Lib = $libPath }
            }
        }
    }
    
    # Try from PATH
    $dumpbinInPath = Get-Command "dumpbin.exe" -ErrorAction SilentlyContinue
    $libInPath = Get-Command "lib.exe" -ErrorAction SilentlyContinue
    
    if ($dumpbinInPath -and $libInPath) {
        return @{ Dumpbin = $dumpbinInPath.Source; Lib = $libInPath.Source }
    }
    
    return $null
}

# Get friendly size string
function Get-FriendlySize {
    param([long]$Bytes)
    
    if ($Bytes -eq 0) { return "0 B" }
    
    $sizes = 'B', 'KB', 'MB', 'GB', 'TB'
    $order = 0
    while ($Bytes -ge 1024 -and $order -lt $sizes.Length - 1) {
        $order++
        $Bytes = $Bytes / 1024
    }
    
    return [string]::Format('{0:0.##} {1}', $Bytes, $sizes[$order])
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  LIB File Size Analyzer" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Get lib file info
$libFile = Get-Item $LibPath
$libSize = $libFile.Length

Write-Host "File: $LibPath" -ForegroundColor Yellow
Write-Host "Size: $(Get-FriendlySize $libSize)" -ForegroundColor Yellow
Write-Host "Created: $($libFile.CreationTime)" -ForegroundColor Yellow
Write-Host ""

# Check VS tools
$tools = Test-VSTools
if (-not $tools) {
    Write-Host "Checking Visual Studio tools..." -ForegroundColor Yellow
    
    # Try vswhere
    $vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $vsPath = & $vswherePath -latest -property installationPath
        if ($vsPath) {
            $vcToolsPath = Get-ChildItem "$vsPath\VC\Tools\MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
            if ($vcToolsPath) {
                $binPath = Join-Path $vcToolsPath.FullName "bin\Hostx64\x64"
                $dumpbinPath = Join-Path $binPath "dumpbin.exe"
                $libPath = Join-Path $binPath "lib.exe"
                
                if (Test-Path $dumpbinPath) {
                    $tools = @{ Dumpbin = $dumpbinPath; Lib = $libPath }
                }
            }
        }
    }
    
    if (-not $tools) {
        Write-Error "Visual Studio tools not found. Please ensure VS is installed or add dumpbin.exe and lib.exe to PATH."
        Write-Host "Suggestions:" -ForegroundColor Yellow
        Write-Host "1. Run this script in Visual Studio Developer Command Prompt" -ForegroundColor White
        Write-Host "2. Manually add path:" -ForegroundColor White
        Write-Host "   `$env:Path += `";C:\Path\To\VS\VC\Tools\MSVC\...\bin\Hostx64\x64`"" -ForegroundColor White
        exit 1
    }
}

Write-Host "Using Tools:" -ForegroundColor Green
Write-Host "  Dumpbin: $($tools.Dumpbin)" -ForegroundColor White
Write-Host "  Lib: $($tools.Lib)" -ForegroundColor White
Write-Host ""

Write-Host "Analyzing .lib file..." -ForegroundColor Green

# 1. List object files
Write-Host "`n1. Listing object files..." -ForegroundColor Cyan
$tempFile = [System.IO.Path]::GetTempFileName()
$tempDir = $null

try {
    # Get lib list
    $process = Start-Process -FilePath $tools.Lib -ArgumentList "/list", "`"$LibPath`"" -RedirectStandardOutput $tempFile -NoNewWindow -Wait -PassThru
    
    if ($process.ExitCode -ne 0) {
        Write-Error "lib.exe failed with exit code: $($process.ExitCode)"
        exit 1
    }
    
    $objFiles = @()
    
    Get-Content $tempFile | ForEach-Object {
        $line = $_.Trim()
        
        # Check for .obj or .o files directly
        if ($line -ne "" -and $line -match "\.(obj|o)$") {
            $objFiles += $line
            if ($ShowDetails) {
                Write-Verbose "Found object: $line"
            }
        }
    }
    
    Write-Host "Found $($objFiles.Count) object files" -ForegroundColor Green
    
    if ($objFiles.Count -eq 0) {
        Write-Host "Warning: No object files found. The .lib might be empty or format is unrecognized." -ForegroundColor Yellow
        Write-Host "Debug Info - lib.exe output (first 20 lines):" -ForegroundColor Gray
        Get-Content $tempFile -TotalCount 20 | Write-Host -ForegroundColor Gray
        exit 0
    }
    
    # 2. Extract and measure size
    Write-Host "`n2. Analyzing object file sizes..." -ForegroundColor Cyan
    
    $objSizes = @()
    $totalExtractedSize = 0
    
    # Create temp dir
    $tempDir = Join-Path $env:TEMP "lib_analysis_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    
    foreach ($objFile in $objFiles) {
        $cleanObjName = [System.IO.Path]::GetFileName($objFile)
        $outputPath = Join-Path $tempDir $cleanObjName
        
        if ($ShowDetails) {
            Write-Verbose "Extracting: $cleanObjName"
        }
        
        $extractArgs = @("/extract:`"$objFile`"", "`"$LibPath`"", "/out:`"$outputPath`"")
        $process = Start-Process -FilePath $tools.Lib -ArgumentList $extractArgs -NoNewWindow -Wait -PassThru
        
        if ($process.ExitCode -eq 0 -and (Test-Path $outputPath)) {
            $objItem = Get-Item $outputPath
            $size = $objItem.Length
            $totalExtractedSize += $size
            
            $percentage = if ($libSize -gt 0) { [math]::Round(($size / $libSize) * 100, 2) } else { 0 }
            
            $objSizes += [PSCustomObject]@{
                FileName = $cleanObjName
                Size = $size
                FriendlySize = Get-FriendlySize $size
                Percentage = $percentage
            }
        }
    }
    
    # 3. Show results
    Write-Host "`n3. Top $TopCount object files:" -ForegroundColor Cyan
    Write-Host "------------------------------------------------" -ForegroundColor Cyan
    
    if ($objSizes.Count -eq 0) {
        Write-Host "No object files extracted successfully" -ForegroundColor Yellow
    }
    else {
        $sorted = $objSizes | Sort-Object Size -Descending | Select-Object -First $TopCount
        
        $sorted | ForEach-Object {
            $color = if ($_.Percentage -gt 10) { "Red" } 
                    elseif ($_.Percentage -gt 5) { "Yellow" }
                    else { "White" }
            
            Write-Host ("{0,-40} {1,15} ({2,6}%)" -f 
                $_.FileName, 
                $_.FriendlySize, 
                $_.Percentage) -ForegroundColor $color
        }
        
        # 4. Statistics
        Write-Host "`n4. Statistics:" -ForegroundColor Cyan
        Write-Host "------------------------------------------------" -ForegroundColor Cyan
        
        if ($totalExtractedSize -gt 0) {
            $compressionRatio = [math]::Round(($libSize / $totalExtractedSize) * 100, 2)
        } else {
            $compressionRatio = 0
        }
        
        Write-Host "Total LIB Size: $(Get-FriendlySize $libSize)" -ForegroundColor Yellow
        Write-Host "Total Object Size: $(Get-FriendlySize $totalExtractedSize)" -ForegroundColor Yellow
        
        if ($totalExtractedSize -gt 0) {
            Write-Host "Compression Ratio: $compressionRatio%" -ForegroundColor Yellow
            
            if ($compressionRatio -lt 90) {
                Write-Host "Note: LIB file has high compression or overhead." -ForegroundColor Green
            }
        }
        
        # 5. Symbols
        if ($ShowSymbols) {
            Write-Host "`n5. Analyzing symbol size..." -ForegroundColor Cyan
            
            $symbolTempFile = [System.IO.Path]::GetTempFileName()
            & $tools.Dumpbin /symbols "`"$LibPath`"" > $symbolTempFile 2>&1
            
            $symbolCount = 0
            $inSymbolSection = $false
            
            Get-Content $symbolTempFile | ForEach-Object {
                if ($_ -match "COFF SYMBOL TABLE") {
                    $inSymbolSection = $true
                }
                elseif ($inSymbolSection -and $_ -match "Summary") {
                    $inSymbolSection = $false
                }
                elseif ($inSymbolSection -and $_ -match "^[0-9A-F]") {
                    $symbolCount++
                }
            }
            
            Write-Host "Symbol Count: $symbolCount" -ForegroundColor Yellow
            
            $estimatedSymbolSize = $symbolCount * 100
            Write-Host "Estimated Symbol Size: $(Get-FriendlySize $estimatedSymbolSize)" -ForegroundColor Yellow
            
            Remove-Item $symbolTempFile -Force -ErrorAction SilentlyContinue
        }
        
        # 6. Potential optimizations
        Write-Host "`n6. Potential Optimizations:" -ForegroundColor Cyan
        Write-Host "------------------------------------------------" -ForegroundColor Cyan
        
        $largeFiles = $objSizes | Where-Object { $_.Percentage -gt 10 }
        if ($largeFiles.Count -gt 0) {
            Write-Host "Large object files (>10%):" -ForegroundColor Red
            $largeFiles | ForEach-Object {
                Write-Host ("  {0} ({1}%)" -f $_.FileName, $_.Percentage) -ForegroundColor Red
            }
            Write-Host "`nCheck these files for:" -ForegroundColor Yellow
            Write-Host "  - Large static data or arrays" -ForegroundColor Yellow
            Write-Host "  - Unoptimized template instantiations" -ForegroundColor Yellow
            Write-Host "  - Monolithic implementation" -ForegroundColor Yellow
            Write-Host "  - Excessive debug info" -ForegroundColor Yellow
        }
        else {
            Write-Host "No suspiciously large object files found." -ForegroundColor Green
        }
        
        # 7. CSV Export
        if ($ExportToCSV) {
            $csvPath = [System.IO.Path]::ChangeExtension($LibPath, "_analysis.csv")
            $sorted | Export-Csv -Path $csvPath -Encoding UTF8 -NoTypeInformation
            Write-Host "`nResult exported to: $csvPath" -ForegroundColor Green
        }
    }

    
}
finally {
    # Cleanup temp file
    if (Test-Path $tempFile) {
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    }
    
    # Cleanup temp dir
    if ($null -ne $tempDir -and (Test-Path $tempDir)) {
        Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`nDone!" -ForegroundColor Green
