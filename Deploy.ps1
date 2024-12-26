g++ -Wall -o Perpustakaan.exe Perpustakaan.cpp

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

Search-Dependencies -Path Perpustakaan.exe

foreach ($dll in $dependencies) {
    Copy-Item $dll -Destination .\
}