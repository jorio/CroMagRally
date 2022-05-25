import struct, sys, os, binascii

INLINE_COMMENTS = True  # output byte positions of each vertex/UV

GL_RGB = 0x1907
GL_RGBA = 0x1908
GL_RGB5_A1_EXT = 0x8057
GL_UNSIGNED_SHORT_1_5_5_5_REV = 0x8366

BG3D_TAGTYPE_MATERIALFLAGS				=	0
BG3D_TAGTYPE_MATERIALDIFFUSECOLOR		=	1
BG3D_TAGTYPE_TEXTUREMAP					=	2
BG3D_TAGTYPE_GROUPSTART					=	3
BG3D_TAGTYPE_GROUPEND					=	4
BG3D_TAGTYPE_GEOMETRY					=	5
BG3D_TAGTYPE_VERTEXARRAY				=	6
BG3D_TAGTYPE_NORMALARRAY				=	7
BG3D_TAGTYPE_UVARRAY					=	8
BG3D_TAGTYPE_COLORARRAY					=	9
BG3D_TAGTYPE_TRIANGLEARRAY				= 	10
BG3D_TAGTYPE_ENDFILE					=	11
BG3D_TAGTYPE_BOUNDINGBOX				=	12
BG3D_TAGTYPE_JPEGTEXTURE				=	13
BG3D_TAGTYPE_PVRTCTEXTURE				=	14

BG3D_MATERIALFLAG_TEXTURED		= 	(1<<0)
BG3D_MATERIALFLAG_ALWAYSBLEND	=	(1<<1)	# set if always want to GL_BLEND this texture when drawn
BG3D_MATERIALFLAG_CLAMP_U		=	(1<<2)
BG3D_MATERIALFLAG_CLAMP_V		=	(1<<3)

def unpack_from_file(format, file):
    size = struct.calcsize(format)
    data = file.read(size)
    assert(data)
    assert(len(data) == size)
    return struct.unpack(format, data)

def hexlify_f32be(f):
    return binascii.hexlify(struct.pack(">f", f)).decode('ascii')

def read_bg3d(filepath):
    model_name = os.path.splitext(os.path.basename(filepath))[0]

    mtl = F"# Converted from {os.path.basename(filepath)}\n"

    obj = F"# Converted from {os.path.basename(filepath)}\n"
    obj += F"mtllib {model_name}.mtl\n"

    group_depth = 0
    group_count = 0
    mtl_count = 0
    current_mtl_name = None
    has_uvs = False

    geom_count = 0

    prev_geometry_num_vertices = 0
    vertex_offset = 0

    with open(filepath, 'rb') as f:
        header = f.read(20)
        assert header == b'BG3D 1.0        \1\0\0\0'

        while True:
            # Read a tag
            tag = unpack_from_file(">L", f)[0]

            if tag == BG3D_TAGTYPE_MATERIALFLAGS:
                print(F"# @{f.tell():08x} ", end='')
                flags = unpack_from_file(">L", f)[0]
                print(F"# Flags: ${flags:08x}")
                mtl_count += 1
                current_mtl = F"{model_name}-{mtl_count}"
                #obj += F"usemtl {current_mtl}\n"
                mtl += F"newmtl {current_mtl}\n"
                if 0 != (flags & BG3D_MATERIALFLAG_TEXTURED):
                    mtl += F"\tmap_Kd {current_mtl}.tga\n"

            elif tag == BG3D_TAGTYPE_MATERIALDIFFUSECOLOR:
                r, g, b, a = unpack_from_file(">4f", f)
                mtl += F"\tKd {r} {g} {b}\n"
                mtl += F"\td {a}\n"

            elif tag == BG3D_TAGTYPE_TEXTUREMAP:
                width, height, srcPixelFormat, dstPixelFormat, bufferSize = unpack_from_file(">LLiiL16x", f)
                mtl += F"\t# Texture: {width}x{height}, format 0x{srcPixelFormat:x}\n"
                pixels = f.read(bufferSize)

                bpp = 0
                if srcPixelFormat == GL_RGBA:
                    bpp = 4
                elif srcPixelFormat == GL_RGB:
                    bpp = 3
                #elif srcPixelFormat == GL_RGB5_A1_EXT:
                elif srcPixelFormat == GL_RGB5_A1_EXT:
                    bpp = 2
                elif srcPixelFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV:
                    bpp = 2
                else:
                    assert False, F"Unsupported srcPixelFormat 0x{srcPixelFormat:x}"

                with open(F"{current_mtl}.tga", "wb") as tga:
                    tga.write(struct.pack("<BBBHHBHHHHBB",
                        0, #idFieldLength
                        0, #colorMapType
                        2, #imageType: BGR
                        0, #palette origin
                        0, #palette color count
                        0, #palette bits per color
                        0, #x origin
                        0, #y origin
                        width,
                        height,
                        8*bpp,
                        0 #image descriptor - set 1<<5 to NOT flip the image vertically
                    ))
                    assert len(pixels) == bpp*width*height
                    if srcPixelFormat == GL_RGB:
                        for pix in range(width*height):
                            r,g,b = struct.unpack_from(">BBB", pixels, bpp*pix)
                            tga.write(struct.pack("<BBB", b,g,r))
                    elif srcPixelFormat == GL_RGBA:
                        for pix in range(width*height):
                            r,g,b,a = struct.unpack_from(">BBBB", pixels, bpp*pix)
                            tga.write(struct.pack("<BBBB", b,g,r,a))
                    elif srcPixelFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV:
                        for pix in range(width*height):
                            blob = struct.unpack_from(">H", pixels, bpp*pix)[0]
                            tga.write(struct.pack("<H", blob))
                    else:
                        assert False, F"Unsupported srcPixelFormat 0x{srcPixelFormat:x}"

            elif tag == BG3D_TAGTYPE_GROUPSTART:
                print("# Group Start")
                group_depth += 1
                group_count += 1
                assert group_depth == 1
                obj += F"o {model_name}_Group{group_count}\n"

            elif tag == BG3D_TAGTYPE_GROUPEND:
                group_depth -= 1
                assert group_depth == 0
                obj += "\n"

            elif tag == BG3D_TAGTYPE_GEOMETRY:
                if group_depth == 0:
                    group_count += 1
                    obj += F"o {model_name}_Group{group_count}  # top-level geometry forming its own group\n"

                type, numMaterials, mat0, mat1, mat2, mat3, flags, numPoints, numTriangles = unpack_from_file(">Li4LLLL16x", f)

                assert numMaterials == 1
                assert type == 0, "only BG3D_GEOMETRYTYPE_VERTEXELEMENTS is supported"

                geom_count += 1
                obj += F"g {model_name}_Geometry{geom_count}\n"
                obj += F"usemtl {model_name}-{mat0+1}\n"
                
                vertex_offset += prev_geometry_num_vertices
                prev_geometry_num_vertices = numPoints

            elif tag == BG3D_TAGTYPE_VERTEXARRAY:
                for _ in range(numPoints):
                    pos = f.tell()
                    x, y, z = unpack_from_file(">3f", f)
                    obj += F"v {x:.6f} {y:.6f} {z:.6f}"
                    if INLINE_COMMENTS:
                    	obj += F"\t# @{pos:08x}: {hexlify_f32be(x)} {hexlify_f32be(y)} {hexlify_f32be(z)}\n"
                    else:
                    	obj += F"\n"

            elif tag == BG3D_TAGTYPE_NORMALARRAY:
                for _ in range(numPoints):
                    x, y, z = unpack_from_file(">3f", f)
                    obj += F"vn {x:.6f} {y:.6f} {z:.6f}\n"

            elif tag == BG3D_TAGTYPE_UVARRAY:
                for _ in range(numPoints):
                    pos = f.tell()
                    u, v = unpack_from_file(">2f", f)
                    obj += F"vt {u:.6f} {v:.6f}"
                    if INLINE_COMMENTS:
                    	obj += F"\t# @{pos:08x}: {hexlify_f32be(u)} {hexlify_f32be(v)}\n"
                    else:
                    	obj += F"\n"

            elif tag == BG3D_TAGTYPE_TRIANGLEARRAY:
                for _ in range(numTriangles):
                    p0, p1, p2 = unpack_from_file(">3L", f)
                    p0 += 1 + vertex_offset
                    p1 += 1 + vertex_offset
                    p2 += 1 + vertex_offset
                    obj += F"f {p0}/{p0}/{p0} {p1}/{p1}/{p1} {p2}/{p2}/{p2}\n"

            elif tag == BG3D_TAGTYPE_BOUNDINGBOX:
                print("# skipping bounding box!!!")
                unpack_from_file(">ffffff?xxx", f)

            elif tag == BG3D_TAGTYPE_ENDFILE:
                break

            else:
                raise BaseException(F"Unrecognized tag {tag}")

    with open(F"{model_name}.mtl", "w") as mtlfile:
        mtlfile.write(mtl)

    with open(F"{model_name}.obj", "w") as objfile:
        objfile.write(obj)

read_bg3d(sys.argv[1])
