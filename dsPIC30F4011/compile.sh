#!/bin/bash
# This script compile file main.c into main.hex
# Use for XC16 and dsPIC30F4011.

/opt/microchip/xc16/v1.25/bin/xc16-gcc main.c -mcpu=30F4011 -Wl,--script=p30F4011.gld
/opt/microchip/xc16/v1.25/bin/xc16-bin2hex a.out
mv a.hex main.hex
