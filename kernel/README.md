# My Portable Console Kernel

## Building
Before building there are a couple dependencies.
You can install these yourself or run the `setup.sh` (for Unix systems) or `setup.ps1` (for Windows systems) scripts to automatically install these.

### Unix dependencies
On Unix systems you will need:
 - `cmake`
 - `gcc-arm-none-eabi`
 - `libnewlib-arm-none-eabi`
 - `build-essential`
 - `ninja-build`
 - `python3`
 - `git`

To install these is super easy, just use your package manager:
```sh
sudo apt update
sudo apt install -y cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    ninja-build \
    python3 \
    git
```

### Windows dependencies
On Windows systems you will need:
 - `GnuArmEmbeddedToolchain`
 - `Ninja`
 - `CMake`
 - `Python3`
 - `Git`

You can install all of these using `winget`, which is what the `setup.ps1` script does for you (it's very short and simple so you can check it at a glance should you wish to).

### Build script
Use either of the build scripts, `build.sh` (for Unix systems) or `build.ps1` (for Windows systems).
These basically just configure and build the `CMakeLists.txt`, the Windows one also adds some searching for the compiler (since it often isn't automatically added to `PATH` permanently).

To use either script, simply:
```sh
./build.sh
```
Or:
```pwsh
./build.ps1
```
You can add the `clean` argument afterwards to automatically clean the `build` directory.
```sh
./build.sh clean
```

Once built, (if successful) you will find `my_console.uf2` in the `build` folder.
This is the kernel you can flash onto the Pico device.
Hold the `BOOTSEL` button on the Pico and plug in the cable.
Keep holding `BOOTSEL` down until the file explorer opens on your desktop (at least on Windows).
You can then let go and simply drag the kernel into said folder, it will close the file explorer automatically and restart the Pico with the new kernel!

## Datasheets
A couple of datasheets are necessary for reference when writing this driver, these can be found in the `datasheets` folder.
 - `ILI9341 Datasheet.pdf` - The Adafruit screen I used
 - `Part 1 Physical Layer Simplified Specification Ver 9.10.pdf` - The datasheet for Micro SD cards
 - `Raspberry Pi Pico 2W Datasheet.pdf` - Raspberry Pi Pico 2 W
