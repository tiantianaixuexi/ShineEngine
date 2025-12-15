param(
  [Parameter(Mandatory=$true)][string]$MapPath,
  [Parameter(Mandatory=$true)][string]$OutPath,
  [int]$TopN = 20
)

function HexToInt64([string]$h) {
  if ([string]::IsNullOrWhiteSpace($h)) { return 0 }
  $hh = $h.Trim()
  if ($hh -eq "-") { return 0 }
  return [Convert]::ToInt64($hh, 16)
}

if (-not (Test-Path -LiteralPath $MapPath)) {
  throw "map not found: $MapPath"
}

$lines = Get-Content -LiteralPath $MapPath

# Map format is wasm-ld --print-map. We parse it using:
#   <Addr> <Off> <Size> <Remainder>
# The remainder is either a section header (CODE/.rodata/.data/.bss/CUSTOM(...))
# or an entry (often file:(symbol)) or a demangled name line.

$current = ""
$sectionSizes = @{}
$entries = New-Object System.Collections.Generic.List[object]
$lastKey = $null

foreach ($line in $lines) {
  if ([string]::IsNullOrWhiteSpace($line)) { continue }

  # Skip header row.
  if ($line -match '^\s*Addr\s+Off\s+Size\s+') { continue }

  $m = [regex]::Match($line, '^\s*(\S+)\s+(\S+)\s+(\S+)\s+(.*)$')
  if (-not $m.Success) { continue }

  $addr = $m.Groups[1].Value
  $offS = $m.Groups[2].Value
  $sizeS = $m.Groups[3].Value
  $rest = $m.Groups[4].Value.Trim()

  $off = HexToInt64 $offS
  $size = HexToInt64 $sizeS

  # Section header lines look like: "-  312  1a07 CODE" or "400 1d1a 900 .rodata" or "- 274f 66a CUSTOM(name)"
  if ($rest -match '^(TYPE|IMPORT|FUNCTION|TABLE|MEMORY|GLOBAL|EXPORT|ELEM|CODE|DATA|\.rodata|\.data|\.bss|CUSTOM\([^\)]+\))$') {
    $current = $Matches[1]
    if (-not $sectionSizes.ContainsKey($current)) { $sectionSizes[$current] = 0 }
    $sectionSizes[$current] += $size
    continue
  }

  if ([string]::IsNullOrWhiteSpace($current)) { continue }

  # Demangled companion line: same off/size, rest is "shine::foo(...)" without file:(symbol)
  $key = "$current|$offS|$sizeS"
  if ($rest -notmatch ':\(' -and $lastKey -eq $key) {
    $last = $entries[$entries.Count - 1]
    if ($null -ne $last -and $last.OffHex -eq $offS -and $last.SizeHex -eq $sizeS) {
      $name2 = $rest.Trim()
      if (-not [string]::IsNullOrWhiteSpace($name2)) {
        $last.Name = $name2
      }
    }
    continue
  }

  $file = ""
  $name = $rest

  # IMPORTANT: use greedy match for file path because Windows paths contain a drive colon (e.g. E:/...).
  $m2 = [regex]::Match($rest, '^(.*):\((.*)\)$')
  if ($m2.Success) {
    $file = $m2.Groups[1].Value
    $name = $m2.Groups[2].Value
  } elseif ($rest -match '^(<internal>):\((.*)\)$') {
    $file = $Matches[1]
    $name = $Matches[2]
  }

  $entries.Add([pscustomobject]@{
    Section = $current
    OffHex = $offS
    Off = $off
    SizeHex = $sizeS
    Size = $size
    File = $file
    Name = $name
  })

  $lastKey = $key
}

function Bytes([Int64]$n) {
  return "{0} B" -f $n
}

function KB([Int64]$n) {
  return "{0:N2} KB" -f ($n / 1024.0)
}

function Write-TopList([System.Text.StringBuilder]$sb, [string]$title, $items, [int]$topN) {
  [void]$sb.AppendLine($title)
  [void]$sb.AppendLine(('=' * $title.Length))
  $i = 0
  foreach ($it in $items) {
    $i++
    if ($i -gt $topN) { break }
    $sz = [Int64]$it.Size
    $line = "{0,2}. {1,7}  {2,9}  {3}" -f $i, (Bytes $sz), (KB $sz), $it.Label
    [void]$sb.AppendLine($line)
  }
  if ($i -eq 0) { [void]$sb.AppendLine('(no entries)') }
  [void]$sb.AppendLine('')
}

# Helpers: aggregate
function Group-Sum($list, [scriptblock]$keySel, [scriptblock]$labelSel) {
  $ht = @{}
  foreach ($e in $list) {
    $k = & $keySel $e
    if ([string]::IsNullOrWhiteSpace($k)) { continue }
    if (-not $ht.ContainsKey($k)) {
      $ht[$k] = [pscustomobject]@{ Key=$k; Size=0; Label=(& $labelSel $e) }
    }
    $ht[$k].Size += [Int64]$e.Size
  }
  return $ht.Values
}

$code = $entries | Where-Object { $_.Section -eq 'CODE' }
$rodata = $entries | Where-Object { $_.Section -eq '.rodata' }
$data = $entries | Where-Object { $_.Section -eq '.data' }
$bss = $entries | Where-Object { $_.Section -eq '.bss' }

# Prefer human names; also allow empty file for some lines.
$topCodeFns = Group-Sum $code { param($e) $e.Name } { param($e) $e.Name } |
  Sort-Object -Property Size -Descending

$topRodataSyms = Group-Sum $rodata { param($e) $e.Name } { param($e) $e.Name } |
  Sort-Object -Property Size -Descending

$topDataSyms = Group-Sum $data { param($e) $e.Name } { param($e) $e.Name } |
  Sort-Object -Property Size -Descending

$topBssSyms = Group-Sum $bss { param($e) $e.Name } { param($e) $e.Name } |
  Sort-Object -Property Size -Descending

$codeByObj = Group-Sum $code {
  param($e)
  if ([string]::IsNullOrWhiteSpace($e.File)) { return $null }
  # Normalize to filename only (but keep non-path labels like <internal>).
  $f = [string]$e.File
  if ($f.StartsWith('<')) { return $f }
  try { return [IO.Path]::GetFileName($f) } catch { return $f }
} {
  param($e)
  $f = [string]$e.File
  if ($f.StartsWith('<')) { return $f }
  try { return [IO.Path]::GetFileName($f) } catch { return $f }
} | Sort-Object -Property Size -Descending

$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine("smallwasm map report")
[void]$sb.AppendLine("Map: $MapPath")
[void]$sb.AppendLine(("Time: {0}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss")))
[void]$sb.AppendLine('')

# Section summary (best-effort: from header sizes we accumulated)
[void]$sb.AppendLine('Section summary')
[void]$sb.AppendLine('==============')
$want = @('CODE','.rodata','.data','.bss','CUSTOM(name)','CUSTOM(producers)','CUSTOM(target_features)','TYPE','IMPORT','EXPORT','ELEM','FUNCTION','TABLE','MEMORY','GLOBAL','DATA')
foreach ($k in $want) {
  if ($sectionSizes.ContainsKey($k)) {
    $n = [Int64]$sectionSizes[$k]
    [void]$sb.AppendLine(("{0,-22} {1,8}  {2,9}" -f $k, (Bytes $n), (KB $n)))
  }
}
[void]$sb.AppendLine('')

Write-TopList $sb "Top CODE symbols" ($topCodeFns | ForEach-Object { [pscustomobject]@{ Size=$_.Size; Label=$_.Label } }) $TopN
Write-TopList $sb "Top .rodata symbols" ($topRodataSyms | ForEach-Object { [pscustomobject]@{ Size=$_.Size; Label=$_.Label } }) $TopN
Write-TopList $sb "Top .data symbols" ($topDataSyms | ForEach-Object { [pscustomobject]@{ Size=$_.Size; Label=$_.Label } }) $TopN
Write-TopList $sb "Top .bss symbols" ($topBssSyms | ForEach-Object { [pscustomobject]@{ Size=$_.Size; Label=$_.Label } }) $TopN
Write-TopList $sb "CODE by object file" ($codeByObj | ForEach-Object { [pscustomobject]@{ Size=$_.Size; Label=$_.Label } }) $TopN

$dir = Split-Path -Parent $OutPath
if (-not (Test-Path -LiteralPath $dir)) {
  New-Item -ItemType Directory -Force -Path $dir | Out-Null
}

$sb.ToString() | Out-File -LiteralPath $OutPath -Encoding utf8
