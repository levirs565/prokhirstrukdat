Write-Output "Removing unused filed"
Remove-Item output -Recurse -Force -Confirm:$false
Remove-Item *.exe -Force
Remove-Item *.dll

.\Deploy.ps1

Write-Output "Archiving"
$files = Get-ChildItem -Path .\ -Exclude @(".git")
Compress-Archive -Path $files -DestinationPath ..\Proyek-Akhir-Strukdat.zip -CompressionLevel Optimal