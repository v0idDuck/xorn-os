# disk.ps1 — создание образа диска
# Запускать от администратора!

$imgPath = "C:\xorn-os\disk.img"

# удаляем старый если есть
if (Test-Path $imgPath) { Remove-Item $imgPath }

# создаём пустой образ 64MB
$img = New-Object byte[] (64 * 1024 * 1024)
[IO.File]::WriteAllBytes($imgPath, $img)

# форматируем в FAT32
C:\msys64\mingw64\bin\mformat.exe -i $imgPath -F -c 1 -v XORN ::

if ($LASTEXITCODE -ne 0) {
    echo "mformat failed! trying alternative..."
    C:\msys64\mingw64\bin\mformat.exe -i "${imgPath}@@0" -F -v XORN ::
}

mformat -i disk.img -F -v "XORNOS" ::
& "$mtools\mlabel.exe" -i "$imgPath" "::XORNOS"
# проверяем содержимое bin
echo "Contents of bin:"
C:\msys64\mingw64\bin\mdir.exe -i $imgPath ::/bin
echo "Done! Listing disk contents:"
C:\msys64\mingw64\bin\mdir.exe -i $imgPath ::