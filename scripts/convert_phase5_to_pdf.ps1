# Phase 5 Documentation - Convert Markdown to PDF
# PowerShell script for Windows
# Requires: pandoc (install via: choco install pandoc)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Phase 5 Documentation - MD to PDF Converter" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if pandoc is installed
$pandocInstalled = Get-Command pandoc -ErrorAction SilentlyContinue
if (-not $pandocInstalled) {
    Write-Host "ERROR: pandoc is not installed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Install pandoc using one of these methods:" -ForegroundColor Yellow
    Write-Host "  1. Chocolatey: choco install pandoc" -ForegroundColor Yellow
    Write-Host "  2. Download: https://pandoc.org/installing.html" -ForegroundColor Yellow
    Write-Host "  3. Scoop: scoop install pandoc" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

Write-Host "✓ pandoc found: $($pandocInstalled.Source)" -ForegroundColor Green
Write-Host ""

# Create output directory
$outputDir = "docs/phase5_pdf"
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    Write-Host "✓ Created output directory: $outputDir" -ForegroundColor Green
} else {
    Write-Host "✓ Output directory exists: $outputDir" -ForegroundColor Green
}
Write-Host ""

# Define documents to convert
$documents = @(
    @{
        Name = "PHASE5_EXECUTIVE_SUMMARY.md"
        Title = "Phase 5: USB Mode Switching - Executive Summary"
        Output = "Phase5_Executive_Summary.pdf"
    },
    @{
        Name = "PHASE5_USB_MODE_SWITCHING_RECOMMENDATION.md"
        Title = "Phase 5: USB Mode Switching - Technical Recommendation"
        Output = "Phase5_Technical_Recommendation.pdf"
    },
    @{
        Name = "PHASE5_IMPLEMENTATION_CHECKLIST.md"
        Title = "Phase 5: USB Mode Switching - Implementation Checklist"
        Output = "Phase5_Implementation_Checklist.pdf"
    },
    @{
        Name = "PHASE5_REVIEW_RESPONSE.md"
        Title = "Phase 5: USB Mode Switching - Review Response"
        Output = "Phase5_Review_Response.pdf"
    }
)

# Pandoc options for professional PDF output
$pandocOptions = @(
    "--pdf-engine=xelatex",
    "--variable", "geometry:margin=1in",
    "--variable", "fontsize=11pt",
    "--variable", "colorlinks=true",
    "--variable", "linkcolor=blue",
    "--variable", "urlcolor=blue",
    "--variable", "toccolor=black",
    "--toc",
    "--toc-depth=3",
    "--number-sections",
    "--highlight-style=tango"
)

# Convert each document
$successCount = 0
$failCount = 0

foreach ($doc in $documents) {
    $inputFile = $doc.Name
    $outputFile = Join-Path $outputDir $doc.Output
    
    Write-Host "Converting: $inputFile" -ForegroundColor Cyan
    Write-Host "  → $outputFile" -ForegroundColor Gray
    
    if (-not (Test-Path $inputFile)) {
        Write-Host "  ✗ ERROR: Input file not found!" -ForegroundColor Red
        $failCount++
        Write-Host ""
        continue
    }
    
    try {
        # Build pandoc command
        $pandocArgs = @($inputFile, "-o", $outputFile) + $pandocOptions + @(
            "--metadata", "title=$($doc.Title)",
            "--metadata", "author=Augment Agent (AI Engineering Assistant)",
            "--metadata", "date=$(Get-Date -Format 'MMMM dd, yyyy')"
        )
        
        # Execute pandoc
        & pandoc $pandocArgs 2>&1 | Out-Null
        
        if ($LASTEXITCODE -eq 0) {
            $fileSize = (Get-Item $outputFile).Length / 1KB
            Write-Host "  ✓ Success! ($([math]::Round($fileSize, 1)) KB)" -ForegroundColor Green
            $successCount++
        } else {
            Write-Host "  ✗ ERROR: Conversion failed (exit code: $LASTEXITCODE)" -ForegroundColor Red
            $failCount++
        }
    } catch {
        Write-Host "  ✗ ERROR: $($_.Exception.Message)" -ForegroundColor Red
        $failCount++
    }
    
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Conversion Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✓ Successful: $successCount" -ForegroundColor Green
if ($failCount -gt 0) {
    Write-Host "✗ Failed: $failCount" -ForegroundColor Red
}
Write-Host ""
Write-Host "Output directory: $outputDir" -ForegroundColor Yellow
Write-Host ""

# Open output directory
if ($successCount -gt 0) {
    Write-Host "Opening output directory..." -ForegroundColor Cyan
    Start-Process explorer.exe $outputDir
}

Write-Host "Done!" -ForegroundColor Green

