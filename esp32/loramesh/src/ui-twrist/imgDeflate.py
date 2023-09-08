#!/usr/bin/env python3

# imgDeflate.py

import argparse
import zlib
import PIL, PIL.Image
import sys

SCREEN_WIDTH =  200
SCREEN_HEIGHT = 200

p = argparse.ArgumentParser()
p.add_argument('-i', action="store", dest="inputfile")
p.add_argument('-n', action="store", dest="name")
p.add_argument('-o', action="store", dest="outputfile")

args = p.parse_args()

im = PIL.Image.open(args.inputfile)
im = im.resize((SCREEN_WIDTH, SCREEN_HEIGHT), PIL.Image.LANCZOS)
im = im.convert(mode='1') # , dither=Image.Dither.FLOYDSTEINBERG)

# im.save("test.png")

lst = [ im.getpixel((i%200,i//200)) \
        for i in range(SCREEN_WIDTH * SCREEN_HEIGHT) ]
data = b''
for i in range(len(lst)//8):
    b = 0
    for j in range(8):
        b <<= 1
        if lst[8*i + j] > 0:
            b |=  1
    data += bytes([b])

z = zlib.compress(data)
print(f"compressed {args.inputfile} from {len(data)} to {len(z)} bytes");

with open(args.outputfile, 'w') as f:
    cnt = 0
    f.write(f"const uint8_t {args.name}[{len(z)}] = {{\n")
    for b in z:
        f.write(" 0x{:02x},".format(b))
        cnt += 1
        if (cnt % 12) == 0:
            f.write("\n")
    f.write("\n};\n")

# eof
