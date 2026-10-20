$ErrorActionPreference = 'Stop'

$serverDir = 'C:\TP2\Build\GameLift\WindowsServer\TeamProject_DL'
$serverExe = Join-Path $serverDir 'Binaries\Win64\TeamProject_DLServer.exe'
$backendDir = 'C:\TP2\Server'

Get-Process node,TeamProject_DLServer,TeamProject_DL -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

if (!(Test-Path $serverExe)) { throw "Dedicated server package not found: $serverExe" }

Start-Process powershell.exe -WindowStyle Hidden -ArgumentList '-NoProfile','-Command', @"
Set-Location '$backendDir'
`$env:MATCHMAKING_HOST='127.0.0.1'
`$env:MATCHMAKING_PORT='8080'
`$env:MATCHMAKING_PUBLIC_HOST='127.0.0.1'
node .\server.js
"@

foreach ($port in 7777, 7778, 7779) {
    Start-Process $serverExe -WorkingDirectory $serverDir -ArgumentList "-log -port=$port -ExpectedPlayers=2 -PerceptionMode=0 -DedicatedServerFallbackCombat"
}

Start-Sleep -Seconds 5
$health = Invoke-RestMethod 'http://127.0.0.1:8080/health'
if (!$health.ok) { throw 'Matchmaking backend health check failed.' }
Write-Host "Backend OK: http://127.0.0.1:8080" -ForegroundColor Green
Write-Host "Dedicated pool started: 7777, 7778, 7779" -ForegroundColor Green
