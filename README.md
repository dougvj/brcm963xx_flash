# brcm963xx\_flash

Flash utility for bcrm963xx SoCs which were used in many routers.

Many expose SSH access so you can copy the binary to the device and run it to
dump the flash, etc

## Building

You need a mips32 cross compiler, use [crosstool-ng](https://crosstool-ng.github.io/).
A configuration to create a cross compiler is provided and symlinked to
.config, so you can just run `ct-ng build` and it will produce a cross compiler
in your `~/x-tools/` directory.

Export the cross compiler to your path with `export
$PATH:~/x-tools/mips-linux-uclibc/bin`

Then run:

`meson --cross=./build-mips32.txt build`

`cd build`

`ninja build`

The binary `brcm963xx_flash` will be created which can run on the SoC (assuming
libc etc isn't broken)

## Usage

```
./brcm963xx_flash <-r/-w> <image-file>
```
Use -r to read flash, -w to write flash

If you want to dump to stdout, use -stdout as the image file name


## Writing

The utility supports writing flash but it's extremely dangerous, no checking is
performed. main.c needs to be modified to allow this by commenting out the
line:

`#define DISABLE_WRITE`
