$Title = "Do you want to clean this repo?"
$Prompt = "Enter your choice"
$Choices = [System.Management.Automation.Host.ChoiceDescription[]] @("&Yes", "&No", "&Cancel")
$Default = 1

$Choice = $host.UI.PromptForChoice($Title, $Prompt, $Choices, $Default)

if ($Choice -eq 2) {
    exit 0;
}

if ($Choices -eq 0) {
    git clean
}

.\Deploy.ps1

Write-Output "Archiving"
git archive -o ../Proyek-Akhir-Strukdat.zip