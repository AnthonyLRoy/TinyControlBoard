<#
.SYNOPSIS
  Pulls the files mirrored under scripts/rpi from the Raspberry Pi so the repo stays in sync.
.PARAMETER Target
  SSH target (user@host). Defaults to the moOde player.
.PARAMETER DryRun
  Report what would change without overwriting local files.
.NOTES
  Uses a single ssh connection (one password prompt if no key is installed).
#>
param(
    [string]$Target = 'antho@192.168.0.10',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$root = Join-Path $PSScriptRoot 'rpi'

# Local top-level folder -> remote absolute path prefix.
$map = @{
    'boot'    = '/boot'
    'home'    = '/home'
    'opt'     = '/opt'
    'systemd' = '/etc/systemd/system'
}

$items = @()
$skipped = 0
Get-ChildItem $root -Recurse -File |
    Where-Object { $_.FullName -notmatch '__pycache__' -and $_.Extension -ne '.pyc' } |
    ForEach-Object {
        $rel = $_.FullName.Substring($root.Length + 1) -replace '\\', '/'
        $top, $rest = $rel -split '/', 2
        if ($map.ContainsKey($top) -and $rest) {
            $items += [pscustomobject]@{
                Local  = $_.FullName
                Rel    = $rel
                Remote = "$($map[$top])/$rest".TrimStart('/')   # relative to / for tar -C /
            }
        } else { $skipped++ }
    }

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("rpi_sync_" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $tmp | Out-Null
try {
    $quoted = ($items | ForEach-Object { "'" + $_.Remote + "'" }) -join ' '
    $remoteCmd = "tar -cf - -C / $quoted 2>/dev/null"
    # cmd handles the binary pipe; PowerShell 5.1 would corrupt it.
    cmd /c "ssh $Target ""$remoteCmd"" | tar -xf - -C ""$tmp"" 2>nul"

    $updated = 0; $same = 0; $missing = @()
    foreach ($i in $items) {
        $got = Join-Path $tmp $i.Remote
        if (-not (Test-Path -LiteralPath $got)) { $missing += $i.Rel; continue }
        if ((Get-FileHash -LiteralPath $got).Hash -eq (Get-FileHash -LiteralPath $i.Local).Hash) {
            $same++
        } else {
            Write-Host "changed: $($i.Rel)"
            if (-not $DryRun) { Copy-Item -LiteralPath $got -Destination $i.Local -Force }
            $updated++
        }
    }
    if (($updated + $same) -eq 0) { throw "No files retrieved from $Target (connection/auth failure?)" }

    $verb = if ($DryRun) { 'would update' } else { 'updated' }
    Write-Host "$verb=$updated unchanged=$same not-on-pi=$($missing.Count) skipped(local-only)=$skipped"
    $missing | ForEach-Object { Write-Host "  missing on Pi: $_" }
} finally {
    Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
