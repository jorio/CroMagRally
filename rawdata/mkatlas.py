# Cro-Mag Rally atlas/font packer
#
# This file is part of Cro-Mag Rally: https://github.com/jorio/cromagrally
# (C)2022 Iliyas Jorio
#
# Requirements: Python 3, Python Imaging Library, rectpack (https://github.com/secnot/rectpack)

from dataclasses import dataclass
from PIL import Image
import argparse, html, os,  rectpack, unicodedata

@dataclass
class Glyph:
    codepoint: int
    slice_image: Image
    xoff: int
    yoff: int
    xadv: int
    yadv: int
    alias_of: int = -1
    atlas_x: int = 0
    atlas_y: int = 0
    atlas_w: int = 0
    atlas_h: int = 0

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
DO_CROP = args.crop

if not IS_FONT:
    BASE_H = 0

if not outpath:
    outpath = os.path.join(os.path.dirname(fontname), os.path.basename(fontname).lower())
    outpath = os.path.abspath(outpath)

glyphs: dict[int, Glyph] = {}
num_master_glyphs = 0
atlas_needs_alpha = False

def get_raw_glyph_info(path):
    glyphstr = os.path.basename(path).removesuffix(".png")

    fixw = glyphstr.endswith(".FIXW")
    glyphstr = glyphstr.removesuffix(".FIXW")

    if glyphstr.startswith('#'):
        glyphstr = glyphstr.removeprefix('#')
        codepoint = int(glyphstr)
        glyphstr = chr(codepoint)
    elif len(glyphstr) > 1:
        glyphstr = html.unescape("&" + glyphstr + ";")
        codepoint = ord(glyphstr)
    else:
        codepoint = ord(glyphstr)
    assert len(glyphstr) == 1, glyphstr

    image = Image.open(F"{fontname}/{filename}")
    return image, codepoint, fixw

for filename in sorted(os.listdir(fontname)):
    if not filename.endswith(".png"):
        continue

    raw_image, codepoint, fixw = get_raw_glyph_info(os.path.join(fontname, filename))

    if raw_image.mode == 'RGBA':
        atlas_needs_alpha = True

    if DO_CROP:
        cropped_bbox = raw_image.getbbox()
        cx1, cy1, cx2, cy2 = cropped_bbox
        slice_image = raw_image.crop(cropped_bbox)
        del cropped_bbox
    else:
        slice_image = raw_image
        cx1, cy1, cx2, cy2 = 0, 0, slice_image.width, slice_image.height

    try:
        alias_of = next(k for k in glyphs if glyphs[k].slice_image == slice_image)
        print(f"[{fontname}] Codepoint {codepoint} aliases {alias_of}")
    except StopIteration:
        alias_of = -1
        num_master_glyphs += 1

    if IS_FONT:
        # The position of the baseline is defined by the bottom edge
        # of a 64x64 rectangle placed in the center of the image.
        baseline = BASE_H//2 + int(raw_image.height/2)

        ascender = cy1 - baseline
        descender = cy2 - baseline
        yoff = int(BASE_H + ascender)
        print(chr(codepoint), baseline, ascender, yoff)
        #yoff = ( BASE_H//2 + int(raw_image.height/2 - cy2) )
        yadv = raw_image.height  # perhaps BASE_H would be better here?  the game doesn't use yadv for fonts anyway
        if fixw:
            xoff = int(raw_image.width - slice_image.width)//2
            xadv = raw_image.width
        else:
            xoff = 0
            xadv = slice_image.width
        xadv += CHAR_SPACING
    else:
        xoff = cx1
        yoff = cy1
        xadv = raw_image.width
        yadv = raw_image.height
    
    glyphs[codepoint] = Glyph(codepoint, slice_image, xoff, yoff, xadv, yadv, alias_of)

def pack_the_glyphs(glyphs, atlas_w, atlas_h):
    # Prep the packer
    packer = rectpack.newPacker(rotation=False, pack_algo=rectpack.MaxRectsBl)
    packer.add_bin(atlas_w, atlas_h)

    for codepoint in glyphs:
        glyph = glyphs[codepoint]
        if glyph.alias_of < 0:  # only add rects for glyphs that are don't alias any other glyph (or are the master alias)
            padded_width = glyph.slice_image.width + PADDING*2
            padded_height = glyph.slice_image.height + PADDING*2
            packer.add_rect(padded_width, padded_height, codepoint)

    # Pack the rects
    packer.pack()
    all_rects = packer.rect_list()

    if len(all_rects) == num_master_glyphs:
        return all_rects
    else:
        # Not all glyphs accounted for. Try increasing atlas dimensions.
        return None

# Compose atlas image and fill out atlas positions in each glyph
atlas_mode = "RGBA" if atlas_needs_alpha else "RGB"
atlas = Image.new(atlas_mode, (ATLAS_W, ATLAS_H))

all_rects = pack_the_glyphs(glyphs, ATLAS_W, ATLAS_H)
assert all_rects, "Try increasing atlas dimensions"

for rect in all_rects:
    bin_no, x, y, w, h, codepoint = rect
    assert bin_no == 0, "Not enough bins!!"

    glyph = glyphs[codepoint]

    if w != 0 and h != 0:
        x += PADDING
        y += PADDING
        w -= 2*PADDING
        h -= 2*PADDING
        atlas.paste(glyph.slice_image, (x, y))

    glyph.atlas_x = x
    glyph.atlas_y = y
    glyph.atlas_w = w
    glyph.atlas_h = h

# Add fake space glyph
if IS_FONT and SPACE_W != 0:
    space_codepoint = ord(' ')
    assert space_codepoint not in glyphs, "There's an explicit space glyph in this font!"
    glyphs[space_codepoint] = Glyph(space_codepoint, None, 0, 0, SPACE_W, BASE_H)

# Generate atlas text
atlas_text = (
    f"{len(glyphs)}"
    f"\t{BASE_H}"
    f"\t{os.path.basename(fontname)}"
    f"\n")
for codepoint in sorted(glyphs):
    glyph = glyphs[codepoint]
    if glyph.alias_of >= 0:  # steal master alias's position in atlas
        atlas_glyph = glyphs[glyph.alias_of]
    else:
        atlas_glyph = glyph
    atlas_text += (
        f"{codepoint}"
        f"\t{atlas_glyph.atlas_x}"
        f"\t{atlas_glyph.atlas_y}"
        f"\t{atlas_glyph.atlas_w}"
        f"\t{atlas_glyph.atlas_h}"
        f"\t{glyph.xoff}"
        f"\t{glyph.yoff}"
        f"\t{glyph.xadv}"
        f"\t{glyph.yadv}"
        )
    if IS_FONT:
        atlas_text += F"\t{unicodedata.name(chr(codepoint))}"
    atlas_text += "\n"

# Save png & txt files
atlas.save(f"{outpath}.png")
open(f"{outpath}.txt", "wt").write(atlas_text)
print(F"Wrote {outpath}.{{png,txt}}")
