
$vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Check if the batch file exists
if (!(Test-Path $vcvarsPath)) {
    Write-Error "vcvars64.bat not found at: $vcvarsPath"
    exit 1
}

cmd /k "`"$vcvarsPath`" && powershell"