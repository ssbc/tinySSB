#!/usr/bin/env python3

# imgDeflate.py

import argparse
import zlib
import PIL, PIL.Image
import sys

SCREEN_WIDTH =  212
SCREEN_HEIGHT = 104

p = argparse.ArgumentParser()
p.add_argument('-i', action="store", dest="inputfile")
p.add_argument('-n', action="store", dest="name")
p.add_argument('-o', action="store", dest="outputfile")

args = p.parse_args()

im = PIL.Image.open(args.inputfile)
im = im.convert('L') # , dither=PIL.Image.Dither.FLOYDSTEINBERG,

lst = [ im.getpixel((i%SCREEN_WIDTH,i//SCREEN_WIDTH)) >> 6 \
        for i in range(SCREEN_WIDTH * SCREEN_HEIGHT) ]
bw_data = b''
gr_data = b''
for i in range(len(lst)//8):
    b = 0
    g = 0
    for j in range(8):
        b <<= 1
        g <<= 1
        if lst[8*i + j] == 0:
            g |=  1
            b |=  1
        elif lst[8*i + j] == 1:
            b |=  1
        elif lst[8*i + j] == 2:
            g |=  1
        elif lst[8*i + j] == 3:
            pass
    bw_data += bytes([b])
    gr_data += bytes([g])

with open(args.outputfile, 'w') as f:

    z = zlib.compress(bw_data)
    print(f"a) compressed {args.inputfile} from {len(bw_data)} to {len(z)} bytes");

    cnt = 0
    f.write(f"const uint8_t {args.name}_bw_z[{len(z)}] = {{\n")
    for b in z:
        f.write(" 0x{:02x},".format(b))
        cnt += 1
        if (cnt % 12) == 0:
            f.write("\n")
    f.write("\n};\n")

    z = zlib.compress(gr_data)
    print(f"b) compressed {args.inputfile} from {len(gr_data)} to {len(z)} bytes");

    cnt = 0
    f.write(f"const uint8_t {args.name}_gr_z[{len(z)}] = {{\n")
    for b in z:
        f.write(" 0x{:02x},".format(b))
        cnt += 1
        if (cnt % 12) == 0:
            f.write("\n")
    f.write("\n};\n")
    
# eof
