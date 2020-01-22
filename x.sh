#!/bin/bash
gcc -N  -nostdlib -T ./linker.lds a.S -o a
objdump -d a
objcopy -O binary a a.bin
hexdump -e '"%x\n"' a.biz | head -7
