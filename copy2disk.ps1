$imgPath = "C:\xorn-os\disk.img"
$srcPath = "C:\xorn-os\to-disk"
$mtools  = "C:\msys64\mingw64\bin"

# пересоздаём образ
& "$mtools\mformat.exe" -i "$imgPath" -F -T 65536 ::
& "$mtools\mlabel.exe" -i "$imgPath" "::XORNOS"
# копируем всё рекурсивно одной командой
Get-ChildItem -Directory $srcPath | ForEach-Object {
    Write-Host "copy $($_.Name)"
    & "$mtools\mcopy.exe" -i "$imgPath" -s "$($_.FullName)" "::/"
}

Write-Host "Done!"
& "$mtools\mdir.exe" -i "$imgPath" -/ ::