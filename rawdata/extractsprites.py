import sys, os, struct
from PIL import Image

knownformats = {
    0x1907: "GL_RGB",
    0x1908: "GL_RGBA",
    0x8057: "GL_RGB5_A1_EXT",
    0x8366: "GL_UNSIGNED_SHORT_1_5_5_5_REV",
}

def funpack(file, format):
    length = struct.calcsize(format)
    blob = file.read(length)
    return struct.unpack(format, blob)

def get_format_name(f):
    try:
        return knownformats[f]
    except KeyError:
        return "UNKNOWN"

inpath = sys.argv[1]
outpath = sys.argv[2]


filesize = os.path.getsize(inpath)
basename = os.path.splitext(os.path.basename(inpath))[0]
print(basename)

with open(inpath, 'rb') as file:
    numsprites = funpack(file, ">L")[0]
    assert numsprites <= 65535, ("This looks like a little-endian sprite file, probably from the iOS version. "
        "This script does not support sprite files from the iOS version.")

    for i in range(numsprites):
        width, height, aspect, srcformat, dstformat, bufsize = funpack(file, ">iifiii")

        if dstformat != srcformat:
            print("[fmt mismatch!]  ", end='')

        print(F"sprite {i}:\tAR={aspect}\t{width}x{height}\tformat: {get_format_name(srcformat)} -> {get_format_name(dstformat)}\tbytes={bufsize}")
        assert aspect == height / width, "weird aspect ratio!"

        buffer = file.read(bufsize)
        if get_format_name(srcformat) == 'GL_RGB':
            img = Image.frombytes('RGB', (width,height), buffer)
        elif get_format_name(srcformat) == 'GL_RGBA':
            img = Image.frombytes('RGBA', (width,height), buffer)
        else:
            raise F"Unsupported srcFormat 0x{srcFormat:X}"

        img = img.transpose(Image.Transpose.FLIP_TOP_BOTTOM)

        os.makedirs(outpath, exist_ok=True)
        img.save(os.path.join(outpath, F"{basename}{i:03}.png"))
