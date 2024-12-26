$array = New-Object System.Collections.ArrayList
$Start = Get-Date '01.01.2023'
$End = Get-Date '12.12.2024'
$End2 = Get-Date '25.12.2024'

for ($i = 1; $i -le 1000; $i++) {
    $res = Invoke-RestMethod -Uri "https://randomuser.me/api?nat=US"
    $nameObj = $res.results[0].name;
    $date = Get-Random -Minimum $Start.Ticks -Maximum $End.Ticks
    $array.Add([pscustomobject]@{
        'ID' = '{0:d9}' -f $i
        'Name' = "$($nameObj.title) $($nameObj.first) $($nameObj.last)"
        'Group' = [char] (65 + (Get-Random -Minimum 0 -Maximum 5))
        'StartDate' = ([datetime] $date).ToString("dd/MM/yyyy")
        'EndDate' = ([datetime] (Get-Random -Minimum ([datetime] $date).AddDays(1).Ticks -Maximum $End2.Ticks)).ToString("dd/MM/yyyy")
    })`
}

$array | Export-Csv ".\data\Rumah_Sakit.csv"