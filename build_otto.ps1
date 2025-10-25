# ESP-IDF Build Script for Otto Robot
Write-Host "Setting up ESP-IDF environment..."

# Try to source ESP-IDF environment
$idfPath = $env:IDF_PATH
if ($idfPath) {
    Write-Host "Found IDF_PATH: $idfPath"
    $exportScript = Join-Path $idfPath "export.ps1"
    if (Test-Path $exportScript) {
        Write-Host "Sourcing ESP-IDF export script..."
        & $exportScript
    }
}

Write-Host "Starting build process..."

# Clean previous build
Write-Host "Cleaning previous build..."
try {
    & idf.py fullclean
    Write-Host "Clean completed"
} catch {
    Write-Host "Clean failed or idf.py not found: $_"
}

# Set target
Write-Host "Setting target to ESP32-S3..."
try {
    & idf.py set-target esp32s3
    Write-Host "Target set to ESP32-S3"
} catch {
    Write-Host "Set target failed: $_"
}

# Build project
Write-Host "Building project..."
try {
    & idf.py build
    Write-Host "Build completed successfully"
    
    # Check if build directory exists
    if (Test-Path "build") {
        Write-Host "Build directory created successfully"
        Write-Host "Contents of build directory:"
        Get-ChildItem build | Select-Object Name, Length | Format-Table
    } else {
        Write-Host "ERROR: Build directory not found!"
    }
} catch {
    Write-Host "Build failed: $_"
}

Write-Host "Build script completed."