param([Parameter(Mandatory)][string]$Executable)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SonoForgeDragInput {
    [StructLayout(LayoutKind.Sequential)] public struct Point { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int w, int h, uint flags);
    [DllImport("user32.dll")] public static extern bool GetCursorPos(out Point point);
    [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(Point point);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint x, uint y, uint data, UIntPtr extra);
}
'@
$oldDpiContext = [SonoForgeDragInput]::SetThreadDpiAwarenessContext([IntPtr]::new(-4))
$originalCursor = [SonoForgeDragInput+Point]::new()
[SonoForgeDragInput]::GetCursorPos([ref]$originalCursor) | Out-Null
$process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -WindowStyle Normal -PassThru
try {
    $deadline = (Get-Date).AddSeconds(30)
    do {
        $process.Refresh()
        if ($process.HasExited) { throw 'SonoForge exited during startup' }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $deadline)
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    function Get-Heading {
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::Text)
        $heading = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition) |
            Where-Object { $_.Current.Name -like 'SonoForge Studio 0.8.2*' } | Select-Object -First 1
        if ($null -eq $heading) { throw 'Heading not found' }
        return $heading.Current.BoundingRectangle
    }
    function Invoke-DoubleClick {
        $heading = Get-Heading
        [SonoForgeDragInput]::SetCursorPos([int]($heading.Left+100), [int]($heading.Top+$heading.Height/2)) | Out-Null
        for ($click = 0; $click -lt 2; $click++) {
            [SonoForgeDragInput]::mouse_event(2,0,0,0,[UIntPtr]::Zero)
            Start-Sleep -Milliseconds 40
            [SonoForgeDragInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
            Start-Sleep -Milliseconds 80
        }
        Start-Sleep -Milliseconds 500
    }
    [SonoForgeDragInput]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    # Windows may deny foreground activation while the Codex window is active.
    # Temporarily raise only this clean test instance so pointer input reaches it.
    [SonoForgeDragInput]::SetWindowPos($process.MainWindowHandle,[IntPtr]::new(-1),0,0,0,0,3) | Out-Null
    Start-Sleep -Milliseconds 300
    $before = $window.Current.BoundingRectangle
    $heading = Get-Heading
    $startX = [int]($heading.Left+100)
    $startY = [int]($heading.Top+$heading.Height/2)
    if (-not [SonoForgeDragInput]::SetCursorPos($startX,$startY)) { throw 'Windows rejected pointer positioning' }
    $cursorPoint = [SonoForgeDragInput+Point]::new()
    [SonoForgeDragInput]::GetCursorPos([ref]$cursorPoint) | Out-Null
    Write-Output "Pointer: $($cursorPoint.X),$($cursorPoint.Y); heading: $heading; target: $($process.MainWindowHandle); hit: $([SonoForgeDragInput]::WindowFromPoint($cursorPoint)); foreground: $([SonoForgeDragInput]::GetForegroundWindow())"
    [SonoForgeDragInput]::mouse_event(2,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    for ($step = 1; $step -le 10; $step++) {
        [SonoForgeDragInput]::SetCursorPos(($startX+12*$step),($startY+6*$step)) | Out-Null
        Start-Sleep -Milliseconds 30
    }
    [SonoForgeDragInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    $after = $window.Current.BoundingRectangle
    if ([Math]::Abs($after.Left-$before.Left) -lt 50 -or [Math]::Abs($after.Top-$before.Top) -lt 20) {
        throw "Window did not move: before=$before after=$after"
    }
    Write-Output "PASS: heading drag moved window from $before to $after"
    Invoke-DoubleClick
    $maximised = $window.Current.BoundingRectangle
    if ($maximised.Width -le $after.Width -and $maximised.Height -le $after.Height) {
        throw 'Double-click did not maximise the window'
    }
    Invoke-DoubleClick
    $restored = $window.Current.BoundingRectangle
    if ([Math]::Abs($restored.Width-$after.Width) -gt 10 -or [Math]::Abs($restored.Height-$after.Height) -gt 10) {
        throw 'Double-click did not restore the window size'
    }
    Write-Output 'PASS: heading double-click maximises and restores the window'
    $process.CloseMainWindow() | Out-Null
    if (-not $process.WaitForExit(5000)) { throw 'Clean test window did not close' }
} finally {
    [SonoForgeDragInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
    $process.Refresh()
    if (-not $process.HasExited) {
        [SonoForgeDragInput]::SetWindowPos($process.MainWindowHandle,[IntPtr]::new(-2),0,0,0,0,3) | Out-Null
    }
    [SonoForgeDragInput]::SetCursorPos($originalCursor.X,$originalCursor.Y) | Out-Null
    [SonoForgeDragInput]::SetThreadDpiAwarenessContext($oldDpiContext) | Out-Null
}
