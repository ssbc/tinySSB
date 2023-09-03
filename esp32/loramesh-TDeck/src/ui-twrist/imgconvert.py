#!python3

from PIL import Image, ImageOps
from argparse import ArgumentParser
import sys

SCREEN_WIDTH =  200
SCREEN_HEIGHT = 200 # 825

if SCREEN_WIDTH % 2:
    print("image width must be even!", file=sys.stderr)
    sys.exit(1)

parser = ArgumentParser()
parser.add_argument('-i', action="store", dest="inputfile")
parser.add_argument('-n', action="store", dest="name")
parser.add_argument('-o', action="store", dest="outputfile")

args = parser.parse_args()

im = Image.open(args.inputfile)
im = im.resize((SCREEN_WIDTH, SCREEN_HEIGHT), Image.LANCZOS)
# im = im.convert(mode='L') # to grayscale
im = im.convert(mode='1') # , dither=Image.Dither.FLOYDSTEINBERG)

# im.save("test.png")

with open(args.outputfile, 'w') as f:
    f.write(f"const uint8_t {args.name}[] = {{\n")
    for y in range(0, im.size[1]):
        for x in range(im.size[0]//8):
            byte = 0
            for i in range(8):
                val = im.getpixel((8*x+i,y))
                byte = (byte << 1) | (val >> 7)
            f.write(" 0x{:02X},".format(byte))
        f.write("\n");
    f.write("};\n")

# eof
