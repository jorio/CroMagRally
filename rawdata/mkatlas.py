import sys, os, html, rectpack, argparse
from PIL import Image

parser = argparse.ArgumentParser(description="Create atlas for CMR")
parser.add_argument('srcdir', type=str, help="path to dir of PNG files")
parser.add_argument('-o', metavar='OUTFILE', help="output path")
parser.add_argument('--atlas-size', '-a', type=str, metavar='WxH', help="atlas width x height", default="512x512")
parser.add_argument('--font', action=argparse.BooleanOptionalAction, default=False, help="enable font features (char spacing, space width)")
parser.add_argument('--line-height', type=int, default=64)
parser.add_argument('--space-width', type=int, default=28)
parser.add_argument('--char-spacing', type=int, default=0)
parser.add_argument('--crop', action=argparse.BooleanOptionalAction, default=True)
parser.add_argument('--padding', type=int, default=1)
args = parser.parse_args()

fontname = args.srcdir
outpath = args.o
ATLAS_W = int(args.atlas_size.split('x')[0])
ATLAS_H = int(args.atlas_size.split('x')[1])
BASE_H = args.line_height
SPACE_W = args.space_width
CHAR_SPACING = args.char_spacing
PADDING = args.padding
IS_FONT = args.font

if not IS_FONT:
    SPACE_W = 0
    CHAR_SPACING = 0
    BASE_H = 0

if not outpath:
    outpath = os.path.join(os.path.dirname(fontname), os.path.basename(fontname).lower())
    outpath = os.path.abspath(outpath)

packer = rectpack.newPacker(rotation=False, pack_algo=rectpack.MaxRectsBl)
packer.add_bin(ATLAS_W, ATLAS_H)

glyphs = {}
offsets = {}
atlas_needs_alpha = False

for filename in sorted(os.listdir(fontname)):
    if not filename.endswith(".png"):
        continue

    glyphstr = filename.removesuffix(".png")

    fixw = glyphstr.endswith(".FIXW")
    glyphstr = glyphstr.removesuffix(".FIXW")

    if glyphstr.startswith('#'):
        glyphstr = glyphstr.removeprefix('#')
        glyphstr = chr(int(glyphstr))
    elif len(glyphstr) > 1:
        glyphstr = html.unescape("&" + glyphstr + ";")
    assert len(glyphstr) == 1, glyphstr

    im = Image.open(F"{fontname}/{filename}")

    if im.mode == 'RGBA':
        atlas_needs_alpha = True

    if args.crop:
        cropped_bbox = im.getbbox()
        cx1, cy1, cx2, cy2 = cropped_bbox
        cropped_im = im.crop(cropped_bbox)
    else:
        cropped_bbox = None
        cropped_im = im
        cx1 = 0
        cy1 = 0
        cx2 = cropped_im.width
        cy2 = cropped_im.height

    packer.add_rect(cropped_im.width+PADDING*2, cropped_im.height+PADDING*2, glyphstr)
    glyphs[glyphstr] = cropped_im

    if IS_FONT:
        xoff = 0
        yoff = ( BASE_H//2 + int(im.height/2 - cy2) )
        xadv = cropped_im.width
        yadv = im.height #BASE_H
        if fixw:
            xoff = int(im.width - cropped_im.width)//2
            xadv = im.width
    else:
        xoff = cx1 #int(im.width/2 - cx2)
        yoff = cy1 #int(im.height/2 - cy2)
        xadv = im.width
        yadv = im.height

    offsets[glyphstr] = (xoff, yoff, xadv+CHAR_SPACING, yadv)
    #print(ord(glyphstr[0]), glyphstr, cropped_bbox, "-->", cropped_im.size, "offset:", offsets[glyphstr])

packer.pack()

all_rects = packer.rect_list()

assert len(all_rects) == len(glyphs), "Not all glyphs accounted for. Try increasing atlas dimensions."

# add space
if IS_FONT and SPACE_W != 0:
    assert ' ' not in glyphs
    assert ' ' not in offsets
    offsets[' '] = (0, 0, SPACE_W, BASE_H)
    all_rects.append( (0, 0, 0, SPACE_W, 0, ' '))

sfl = f"{len(all_rects)}\t{BASE_H}\t{os.path.basename(fontname)}\n"

atlas_mode = "RGBA" if atlas_needs_alpha else "RGB"

atlas = Image.new(atlas_mode, (ATLAS_W, ATLAS_H))

for rect in sorted(all_rects, key=lambda r: r[5]):
    bin_no, x, y, w, h, c = rect
    assert bin_no == 0, "Not enough bins!!"

    if w != 0 and h != 0:
        frame = glyphs[c]
        atlas.paste(frame, (x+PADDING, y+PADDING))

    xo, yo, xadv, yadv = offsets[c]
    sfl += F"{ord(c)}\t{x+PADDING}\t{y+PADDING}\t{w-2*PADDING}\t{h-2*PADDING}\t{xo}\t{yo}\t{xadv}\t{yadv}\n"

atlas.save(f"{outpath}.png")
open(f"{outpath}.txt", "w").write(sfl)
print(F"Wrote {outpath}.{{png,txt}}")
