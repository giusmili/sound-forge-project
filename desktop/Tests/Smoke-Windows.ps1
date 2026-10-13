param([Parameter(Mandatory)][string]$Executable, [int]$ExistingProcessId = 0)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
if ($ExistingProcessId -gt 0) {
    $process = Get-Process -Id $ExistingProcessId
    if ($process.Path -ne (Resolve-Path -LiteralPath $Executable).Path) { throw 'Unexpected executable' }
} else {
    $process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -WindowStyle Normal -PassThru
}
$deadline = (Get-Date).AddSeconds(30)
do {
    $process.Refresh()
    if ($process.HasExited) { throw 'SonoForge exited during startup' }
    if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 200
} while ((Get-Date) -lt $deadline)
if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'No window appeared within 30 seconds' }
function Get-TestWindow {
    $process.Refresh()
    if ($process.HasExited) { throw 'SonoForge exited unexpectedly' }
    return [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
}
function Invoke-TestButton([string]$name) {
    $window = Get-TestWindow
    $condition = [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::NameProperty, $name)
    $buttons = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition)
    $button = $buttons | Where-Object { $_.Current.IsEnabled -and $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button } | Select-Object -First 1
    if ($null -eq $button) { throw "Button not found or disabled: $name" }
    $button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Start-Sleep -Milliseconds 100
}
function Assert-TrackCount([int]$count) {
    $window = Get-TestWindow
    $condition = [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::NameProperty, "$count piste(s)")
    if ($null -eq $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)) {
        throw "Expected $count tracks"
    }
}
Assert-TrackCount 0
Invoke-TestButton 'Mixer'
for ($iteration = 0; $iteration -lt 10; $iteration++) {
    Invoke-TestButton '+ MIDI'
    Assert-TrackCount 1
    Invoke-TestButton 'Tester C4'
    Invoke-TestButton 'Annuler'
    Assert-TrackCount 0
    Invoke-TestButton 'Retablir'
    Assert-TrackCount 1
    Invoke-TestButton 'Tester C4'
    Invoke-TestButton 'Supprimer'
    Assert-TrackCount 0
    Invoke-TestButton 'Annuler'
    Assert-TrackCount 1
    Invoke-TestButton 'Retablir'
    Assert-TrackCount 0
}
Invoke-TestButton 'Quitter'
if (-not $process.WaitForExit(5000)) { throw 'Quitter did not close the clean test project' }
Write-Output 'PASS: 10 GUI MIDI cycles with Mixer open, C4 timers active and clean shutdown'
