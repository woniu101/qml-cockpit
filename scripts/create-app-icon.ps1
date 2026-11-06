$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$project = Split-Path -Parent $PSScriptRoot
$simulator = Test-Path -LiteralPath (Join-Path $project 'src/SimServer.h')
$directory = Join-Path $project 'assets/branding'
New-Item -ItemType Directory -Path $directory -Force | Out-Null

function New-RoundedPath([single]$x, [single]$y, [single]$width, [single]$height, [single]$radius) {
    $path = [System.Drawing.Drawing2D.GraphicsPath]::new()
    $diameter = 2 * $radius
    $path.AddArc($x, $y, $diameter, $diameter, 180, 90)
    $path.AddArc($x + $width - $diameter, $y, $diameter, $diameter, 270, 90)
    $path.AddArc($x + $width - $diameter, $y + $height - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($x, $y + $height - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

$ink = [System.Drawing.ColorTranslator]::FromHtml('#24473e')
$accent = [System.Drawing.ColorTranslator]::FromHtml('#88b49f')
$master = [System.Drawing.Bitmap]::new(512, 512, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($master)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.ScaleTransform(2, 2)

$tile = New-RoundedPath 6 6 244 244 42
$tileFill = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
    [System.Drawing.Point]::new(0, 0), [System.Drawing.Point]::new(256, 256),
    [System.Drawing.ColorTranslator]::FromHtml('#e4ebe5'),
    [System.Drawing.ColorTranslator]::FromHtml('#cbdcd0'))
$rim = [System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml('#b9ccbf'), 2)
$graphics.FillPath($tileFill, $tile)
$graphics.DrawPath($rim, $tile)

$inkBrush = [System.Drawing.SolidBrush]::new($ink)
$inkPen = [System.Drawing.Pen]::new($ink, 14)
$inkPen.StartCap = $inkPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$inkPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$mark = [System.Drawing.Drawing2D.GraphicsPath]::new()

if ($simulator) {
    $chip = New-RoundedPath 62 60 132 136 26
    $graphics.FillPath($inkBrush, $chip)
    $chip.Dispose()
    $inkPen.Width = 22
    $graphics.DrawLine($inkPen, 45, 103, 211, 103)
    $graphics.DrawLine($inkPen, 45, 153, 211, 153)

    $signal = [System.Drawing.Pen]::new($accent, 14)
    $signal.StartCap = $signal.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $signal.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $points = [System.Drawing.Point[]]@(
        [System.Drawing.Point]::new(82, 128)
        [System.Drawing.Point]::new(106, 128)
        [System.Drawing.Point]::new(121, 98)
        [System.Drawing.Point]::new(142, 158)
        [System.Drawing.Point]::new(155, 128)
        [System.Drawing.Point]::new(174, 128)
    )
    $graphics.DrawLines($signal, $points)
    $signal.Dispose()
} else {
    $mark.AddBezier(26, 157, 31, 139, 39, 108, 46, 90)
    $mark.AddBezier(46, 90, 53, 69, 90, 66, 128, 66)
    $mark.AddBezier(128, 66, 166, 66, 203, 69, 210, 90)
    $mark.AddBezier(210, 90, 217, 108, 225, 139, 230, 157)
    $mark.AddBezier(230, 157, 236, 178, 217, 179, 201, 179)
    $mark.AddLine(201, 179, 55, 179)
    $mark.AddBezier(55, 179, 39, 179, 20, 178, 26, 157)
    $mark.CloseFigure()
    $graphics.DrawPath($inkPen, $mark)

    $mark.Reset()
    $mark.AddBezier(46, 147, 49, 136, 54, 124, 57, 116)
    $mark.AddBezier(57, 116, 60, 109, 66, 108, 74, 108)
    $mark.AddLine(74, 108, 182, 108)
    $mark.AddBezier(182, 108, 190, 108, 196, 109, 199, 116)
    $mark.AddBezier(199, 116, 202, 124, 207, 136, 210, 147)
    $mark.AddBezier(210, 147, 215, 161, 207, 165, 198, 165)
    $mark.AddLine(198, 165, 58, 165)
    $mark.AddBezier(58, 165, 49, 165, 41, 161, 46, 147)
    $mark.CloseFigure()
    $graphics.FillPath($inkBrush, $mark)

    $highlight = [System.Drawing.Pen]::new($accent, 14)
    $highlight.StartCap = $highlight.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $graphics.DrawLine($highlight, 171, 144, 184, 124)
    $highlight.Dispose()

    # The tile fill separates the steering wheel from the dashboard silhouette.
    $graphics.FillEllipse($tileFill, 76, 140, 104, 104)
    $inkPen.Width = 13
    $graphics.DrawArc($inkPen, 88, 152, 80, 80, 163, 214)
    $inkPen.Width = 10
    $graphics.DrawLine($inkPen, 97, 180, 126, 193)
    $graphics.DrawLine($inkPen, 159, 180, 130, 193)
    $graphics.FillEllipse($inkBrush, 113, 181, 30, 25)
}

$mark.Dispose()
$inkPen.Dispose()
$inkBrush.Dispose()
$rim.Dispose()
$tileFill.Dispose()
$tile.Dispose()
$graphics.Dispose()

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$frames = @()
foreach ($size in $sizes) {
    $bitmap = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $scaled = [System.Drawing.Graphics]::FromImage($bitmap)
    $scaled.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $scaled.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $scaled.DrawImage($master, 0, 0, $size, $size)
    $stream = [System.IO.MemoryStream]::new()
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $frames += ,$stream.ToArray()
    $stream.Dispose()
    $scaled.Dispose()
    $bitmap.Dispose()
}
$master.Dispose()

$file = [System.IO.File]::Create((Join-Path $directory 'app.ico'))
$writer = [System.IO.BinaryWriter]::new($file)
try {
    $writer.Write([uint16]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]$sizes.Count)
    $offset = 6 + 16 * $sizes.Count
    for ($index = 0; $index -lt $sizes.Count; $index++) {
        $dimension = if ($sizes[$index] -eq 256) { 0 } else { $sizes[$index] }
        $writer.Write([byte]$dimension)
        $writer.Write([byte]$dimension)
        $writer.Write([uint16]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$frames[$index].Length)
        $writer.Write([uint32]$offset)
        $offset += $frames[$index].Length
    }
    foreach ($frame in $frames) { $writer.Write([byte[]]$frame) }
} finally {
    $writer.Dispose()
}
