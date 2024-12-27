$dependencies = New-Object System.Collections.ArrayList
function Search-Dependencies() {
    param (
        $Path
    )

    foreach ($match in (objdump -p $Path | Select-String "DLL Name:")) {
        $dllName = $match.Line.Replace("DLL Name:", "").Trim()
        if ($dllName.StartsWith("api-ms-win-crt")) {
            continue;
        }

        $dllFullPath = (Get-Command $dllName).Path
        if ($dllFullPath.StartsWith("C:\WINDOWS\")) {
            continue;
        }
        if ($dependencies.Contains($dllFullPath)) {
            continue;
        }

        $dependencies.Add($dllFullPath) | Out-Null
        Search-Dependencies -Path $dllFullPath
    }
}

function Invoke-Build() {
    param($SourcePath)

    $DestPath = $SourcePath.Replace(".cpp", ".exe")

    Write-Output "Compiling $SourcePath" 
    g++ -Wall -o $DestPath $SourcePath

    if ($LastExitCode -ne 0) {
        Write-Output "Compile failed"
        exit 1;
    }

    Write-Output "Searching dependecies for $DestPath"
    Search-Dependencies -Path Perpustakaan.exe
}

Invoke-Build -SourcePath "Planner_Acara.cpp"

Write-Output "Copying executable dependencies"
foreach ($dll in $dependencies) {
    Write-Output "Copying $dll"
    Copy-Item $dll -Destination .\
}