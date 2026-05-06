# xpil.ps1 - компилятор программ для Xorn OS

param(
    [string]$src = "",
    [string]$dst = "",
    [switch]$objs
)

if (!$src) {
    echo "Xpiler v0.1.0"
    echo "Usage: xpiler -src file.c -dst output.xe -arg(obj)"
    exit
}

if (!(Test-Path $src)) {
    echo "Error: file '$src' not found!"
    exit 1
}

if (!$dst) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $dst = "to-disk/bin/$name.xe"
}

# создаём папку если нужно
$dstDir = [System.IO.Path]::GetDirectoryName($dst)
if ($dstDir -and !(Test-Path $dstDir)) {
    New-Item -ItemType Directory -Force -Path $dstDir
}

$objFile = "obj_temp_$name.o"  # временный, в корне проекта
$elf = "$dst.elf"

$compileFlags = "-target x86_64-pc-windows-msvc " +
                "-ffreestanding " +
                "-fno-stack-protector " +
                "-mno-red-zone " +
                "-nostdlib " +
                "-I./sdk"
echo "Xpiler v0.1.0"
echo "  input:  $src"
echo "  output: $dst"

# компиляция
echo "  compiling..."
Invoke-Expression "clang $compileFlags -c $src -o $objFile"
if ($LASTEXITCODE -ne 0) {
    echo "  Error: compile failed!"
    Remove-Item $objFile -ErrorAction SilentlyContinue
    exit 1
}

# линковка в ELF
echo "  linking..."
# создаём линкер скрипт
$linkerScript = @"
SECTIONS {
    . = 0x1000000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
"@
$linkerScript | Out-File -FilePath "xorn.ld" -Encoding ASCII

# линкуем с скриптом
& clang -target x86_64-pc-windows-msvc `
        -ffreestanding `
        -fno-stack-protector `
        -mno-red-zone `
        -nostdlib `
        "-Wl,-entry:entry,-subsystem:console" `
        -o $elf `
        $objFile


if ($LASTEXITCODE -ne 0) {
    echo "  Error: objcopy failed!"
    Remove-Item $obj, $elf -ErrorAction SilentlyContinue
    exit 1
}
# читаем e_entry прямо из ELF-заголовка (offset 0x18, 8 байт, little-endian)
$elfBytes    = [System.IO.File]::ReadAllBytes($elf)
$entryVma    = [BitConverter]::ToUInt64($elfBytes, 0x18)
$baseAddr    = [uint64]0x1000000
$entryOffset = if ($entryVma -ge $baseAddr) { [uint32]($entryVma - $baseAddr) } else { 0 }
echo "  entry vma=0x$($entryVma.ToString('X'))  offset=$entryOffset"

echo "  extracting binary..."
& llvm-objcopy --dump-section .text=$dst $elf
if ($LASTEXITCODE -ne 0) {
    echo "  Error: extract failed!"
    Remove-Item $obj, $elf -ErrorAction SilentlyContinue
    exit 1
}

Remove-Item $elf -ErrorAction SilentlyContinue

# добавляем заголовок VD
echo "  packing .xe..."
$code = [System.IO.File]::ReadAllBytes($dst)

echo "  code size: $($code.Length) bytes"

$header = [byte[]]@(
    0x56, 0x44,
    0x01, 0x00,
    [byte]($code.Length -band 0xFF),
    [byte](($code.Length -shr 8) -band 0xFF),
    [byte](($code.Length -shr 16) -band 0xFF),
    [byte](($code.Length -shr 24) -band 0xFF),
    [byte]($entryOffset -band 0xFF),
    [byte](($entryOffset -shr 8) -band 0xFF),
    [byte](($entryOffset -shr 16) -band 0xFF),
    [byte](($entryOffset -shr 24) -band 0xFF)
)

$xe = $header + $code
[System.IO.File]::WriteAllBytes($dst, $xe)


# удаляем .o или перемещаем в obj/
if ($objs) {
    if (!(Test-Path "obj")) { New-Item -ItemType Directory -Force -Path "obj" }
    $objName = [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
    Move-Item $objFile "obj/obj_$objName" -Force
    echo "  obj saved: obj/obj_$objName"
} else {
    Remove-Item $objFile -ErrorAction SilentlyContinue
}

echo "  done! $dst ($($xe.Length) bytes)"