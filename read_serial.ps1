$port = New-Object System.IO.Ports.SerialPort 'COM9',115200,None,8,one
$port.ReadTimeout = 6000
$port.Open()
Start-Sleep -Milliseconds 200
Write-Output "Port COM9 opened, waiting for data..."
try {
    for ($i = 0; $i -lt 20; $i++) {
        $line = $port.ReadLine()
        Write-Output $line
    }
} catch {
    Write-Output "Read timeout or error: $_"
}
$port.Close()
Write-Output "Done."
