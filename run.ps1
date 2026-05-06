# Создаём папки если нет
if (!(Test-Path "obj")) { New-Item -ItemType Directory -Force -Path "obj" }
if (!(Test-Path "esp\EFI\BOOT")) { New-Item -ItemType Directory -Force -Path "esp\EFI\BOOT" }

$flags = "-target x86_64-unknown-windows -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -I./include"

# Компиляция
echo "Compiling..."

$files = @("kernel/xorn", "kernel/display", "kernel/keyboard", "kernel/shell", "kernel/utils", "kernel/xorn-bare", "kernel/memory", "kernel/loader", "kernel/disk", "kernel/fat32", "kernel/config", "kernel/uefi_config")

foreach ($f in $files) {
    $name = Split-Path $f -Leaf
    echo "  $name.c..."
    Invoke-Expression "clang $flags -c $f.c -o obj/$name.o"
    if ($LASTEXITCODE -ne 0) { echo "Compile Error! ($name.c)"; pause; exit }
}

# Линковка
echo "Linking..."
lld-link -subsystem:efi_application `
         -entry:efi_main `
         -nodefaultlib `
         obj/xorn.o obj/display.o obj/keyboard.o obj/shell.o `
         obj/memory.o obj/utils.o obj/xorn-bare.o obj/loader.o `
         obj/disk.o obj/fat32.o obj/config.o obj/uefi_config.o `
         -out:xorn.efi

if ($LASTEXITCODE -ne 0) { echo "Link Error!"; pause; exit }

# Копируем загрузчик
Copy-Item "xorn.efi" "esp\EFI\BOOT\BOOTX64.EFI" -Force

# Обновляем disk.img — копируем файлы из vdisk
# Обновляем disk.img — копируем файлы из to-disk
echo "Updating disk..."
if (Test-Path "disk.img") {
    Get-ChildItem -Recurse -File "to-disk" | ForEach-Object {
        $rel = $_.FullName.Substring((Resolve-Path "to-disk").Path.Length + 1)
        $dir = [System.IO.Path]::GetDirectoryName($rel).Replace("\", "/")
        $name = $_.Name.ToUpper()
        
        $diskPath = if ($dir) { "::/$dir/$name" } else { "::/$name" }
        
        C:\msys64\mingw64\bin\mdel.exe -i C:\xorn-os\disk.img $diskPath 2>$null
        C:\msys64\mingw64\bin\mcopy.exe -i C:\xorn-os\disk.img -n $_.FullName $diskPath
    }
} else {
    echo "disk.img not found! Run disk.ps1 first."
}
# Запуск QEMU
echo "Starting QEMU..."
qemu-system-x86_64 `
    -bios ./OVMF.fd `
    -drive file=fat:rw:esp,format=raw,if=ide,index=0 `
    -drive file=disk.img,format=raw,if=ide,index=1 `
    -m 256M `
    -net none