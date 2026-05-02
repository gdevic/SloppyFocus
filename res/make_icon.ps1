Add-Type -AssemblyName System.Drawing

function New-IconBitmap {
    param([int]$Size)

    $bmp = New-Object System.Drawing.Bitmap $Size, $Size
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode    = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.PixelOffsetMode  = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    # Rounded blue square — the "focused window" indicator.
    $pad    = [Math]::Max(1, [int]($Size * 0.06))
    $left   = $pad
    $top    = $pad
    $right  = $Size - $pad
    $bottom = $Size - $pad
    $r      = [Math]::Max(2, [int]($Size * 0.18))

    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc($left,              $top,                ($r * 2), ($r * 2), 180, 90)
    $path.AddArc(($right - $r * 2),  $top,                ($r * 2), ($r * 2), 270, 90)
    $path.AddArc(($right - $r * 2),  ($bottom - $r * 2),  ($r * 2), ($r * 2),   0, 90)
    $path.AddArc($left,              ($bottom - $r * 2),  ($r * 2), ($r * 2),  90, 90)
    $path.CloseFigure()

    $bg = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(60, 120, 220))
    $g.FillPath($bg, $path)
    $bg.Dispose()
    $path.Dispose()

    # Classic Windows arrow cursor: white fill, black outline (outline skipped
    # at small sizes where it would smear into the white interior).
    $tipX = ($Size * 0.22)
    $tipY = ($Size * 0.16)
    $h    = ($Size * 0.70)

    [System.Drawing.PointF[]]$pts = @(
        [System.Drawing.PointF]::new($tipX,                 $tipY),
        [System.Drawing.PointF]::new($tipX,                 ($tipY + $h)),
        [System.Drawing.PointF]::new(($tipX + (0.29 * $h)), ($tipY + (0.71 * $h))),
        [System.Drawing.PointF]::new(($tipX + (0.47 * $h)), ($tipY + $h)),
        [System.Drawing.PointF]::new(($tipX + (0.59 * $h)), ($tipY + (0.94 * $h))),
        [System.Drawing.PointF]::new(($tipX + (0.41 * $h)), ($tipY + (0.65 * $h))),
        [System.Drawing.PointF]::new(($tipX + (0.71 * $h)), ($tipY + (0.65 * $h)))
    )

    if ($Size -ge 32) {
        $penW = ($Size * 0.05)
        $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::Black), $penW
        $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
        $g.DrawPolygon($pen, $pts)
        $pen.Dispose()
    }

    $white = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $g.FillPolygon($white, $pts)
    $white.Dispose()

    $g.Dispose()
    return $bmp
}

$outPath = Join-Path $PSScriptRoot 'app.ico'
$sizes   = @(16, 24, 32, 48, 64, 128, 256)

$pngs = @()
foreach ($s in $sizes) {
    $bmp = New-IconBitmap -Size $s
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += , $ms.ToArray()
    $ms.Dispose()
    $bmp.Dispose()
}

$out = New-Object System.IO.MemoryStream
$bw  = New-Object System.IO.BinaryWriter $out

# ICONDIR
$bw.Write([UInt16]0)
$bw.Write([UInt16]1)
$bw.Write([UInt16]$sizes.Count)

# ICONDIRENTRY[]
$dataOffset = (6 + (16 * $sizes.Count))
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $dim = if ($s -ge 256) { 0 } else { $s }
    $bw.Write([Byte]$dim)
    $bw.Write([Byte]$dim)
    $bw.Write([Byte]0)
    $bw.Write([Byte]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]$pngs[$i].Length)
    $bw.Write([UInt32]$dataOffset)
    $dataOffset += $pngs[$i].Length
}

foreach ($p in $pngs) { $bw.Write($p) }

$bw.Flush()
[System.IO.File]::WriteAllBytes($outPath, $out.ToArray())
$bw.Dispose()
$out.Dispose()

"Wrote $outPath ($((Get-Item $outPath).Length) bytes, $($sizes.Count) sizes: $($sizes -join ', '))"

# Dump preview PNGs for visual check
foreach ($s in @(16, 32, 256)) {
    $idx = $sizes.IndexOf($s)
    [System.IO.File]::WriteAllBytes((Join-Path $PSScriptRoot "app_preview_$s.png"), $pngs[$idx])
}
