// TEXT MESH.C
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <string.h>
#include <stdio.h>

/****************************/
/*    CONSTANTS             */
/****************************/

// This covers the basic multilingual plane (0000-FFFF)
#define MAX_CODEPOINT_PAGES 256

#define TAB_STOP 60.0f

#define MAX_KERNPAIRS		256

#define MAX_LINEBREAKS_PER_OBJNODE	16


/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	float x;
	float y;
	float w;
	float h;
	float xoff;
	float yoff;
	float xadv;

	uint16_t	kernTableOffset;
	int8_t		numKernPairs;
} AtlasGlyph;

typedef struct Atlas
{
	// The font material must be reloaded everytime a new GL context is created
	MOMaterialObject* material;
	int textureWidth;
	int textureHeight;
	float invTextureWidth;
	float invTextureHeight;
	float lineHeight;
	AtlasGlyph* glyphPages[MAX_CODEPOINT_PAGES];

	uint16_t kernPairs[MAX_KERNPAIRS];
	uint8_t kernTracking[MAX_KERNPAIRS];
} Atlas;

/****************************/
/*    VARIABLES             */
/****************************/

static Atlas* gAtlases[MAX_SPRITE_GROUPS];

#pragma mark -

/***************************************************************/
/*                         UTF-8                               */
/***************************************************************/

static AtlasGlyph* GetGlyphFromCodepoint(const Atlas* atlas, uint32_t c)
{
	uint32_t page = c >> 8;

	if (page >= MAX_CODEPOINT_PAGES)
	{
		page = 0; // ascii
		c = '?';
	}

	if (!atlas->glyphPages[page])
	{
		page = 0; // ascii
		c = '#';
	}

	return &atlas->glyphPages[page][c & 0xFF];
}

static uint32_t ReadNextCodepointFromUTF8(const char** utf8TextPtr)
{
#define TRY_ADVANCE(t) do { if (!*t) return 0; else t++; } while(0)

	uint32_t codepoint = 0;
	const uint8_t* t = (const uint8_t*) *utf8TextPtr;

	if ((*t & 0b10000000) == 0)
	{
		// 1 byte code point, ASCII
		codepoint |= (*t & 0b01111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 1;
	}
	else if ((*t & 0b11100000) == 0b11000000)
	{
		// 2 byte code point
		codepoint |= (*t & 0b00011111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 2;
	}
	else if ((**utf8TextPtr & 0b11110000) == 0b11100000)
	{
		// 3 byte code point
		codepoint |= (*t & 0b00001111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);
		*utf8TextPtr += 3;
	}
	else
	{
		// 4 byte code point
		codepoint |= (*t & 0b00000111) << 18;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 4;
	}

	return codepoint;

#undef TRY_ADVANCE
}

/***************************************************************/
/*                       PARSE SFL                             */
/***************************************************************/

static void ParseSFL_SkipLine(const char** dataPtr)
{
	const char* data = *dataPtr;

	while (*data)
	{
		char c = data[0];
		data++;
		if (c == '\r' && *data != '\n')
			break;
		if (c == '\n')
			break;
	}

	GAME_ASSERT(*data);
	*dataPtr = data;
}

// Parse an SFL file produced by fontbuilder
static void ParseSFL(Atlas* atlas, const char* data)
{
	int nArgs = 0;
	int nGlyphs = 0;
	int junk = 0;

	ParseSFL_SkipLine(&data);	// Skip font name

	nArgs = sscanf(data, "%d %f", &junk, &atlas->lineHeight);
	GAME_ASSERT(nArgs == 2);
	ParseSFL_SkipLine(&data);  // Skip rest of line

	ParseSFL_SkipLine(&data);	// Skip image filename

	nArgs = sscanf(data, "%d", &nGlyphs);
	GAME_ASSERT(nArgs == 1);
	ParseSFL_SkipLine(&data);  // Skip rest of line

	for (int i = 0; i < nGlyphs; i++)
	{
		AtlasGlyph newGlyph;
		memset(&newGlyph, 0, sizeof(newGlyph));

		uint32_t codePoint = 0;
		nArgs = sscanf(
				data,
				"%d %f %f %f %f %f %f %f",
				&codePoint,
				&newGlyph.x,
				&newGlyph.y,
				&newGlyph.w,
				&newGlyph.h,
				&newGlyph.xoff,
				&newGlyph.yoff,
				&newGlyph.xadv);
		GAME_ASSERT(nArgs == 8);

		uint32_t codePointPage = codePoint >> 8;
		if (codePointPage >= MAX_CODEPOINT_PAGES)
		{
			printf("WARNING: codepoint 0x%x exceeds supported maximum (0x%x)\n", codePoint, MAX_CODEPOINT_PAGES * 256 - 1);
			continue;
		}

		// Allocate codepoint page
		if (atlas->glyphPages[codePointPage] == NULL)
		{
			atlas->glyphPages[codePointPage] = AllocPtrClear(sizeof(AtlasGlyph) * 256);
		}

		atlas->glyphPages[codePointPage][codePoint & 0xFF] = newGlyph;

		ParseSFL_SkipLine(&data);  // Skip rest of line
	}

	// Force monospaced numbers
	AtlasGlyph* asciiPage = atlas->glyphPages[0];
	AtlasGlyph referenceNumber = asciiPage['4'];
	for (int c = '0'; c <= '9'; c++)
	{
		asciiPage[c].xoff += (referenceNumber.w - asciiPage[c].w) / 2.0f;
		asciiPage[c].xadv = referenceNumber.xadv;
	}
}

/***************************************************************/
/*                 PARSE KERNING TABLE                         */
/***************************************************************/

static void SkipWhitespace(const char** data)
{
	while (**data && strchr("\t\r\n ", **data))
	{
		(*data)++;
	}
}

static void ParseKerningFile(Atlas* atlas, const char* data)
{
	int kernTableOffset = 0;

	while (*data)
	{
		uint32_t codepoint1 = ReadNextCodepointFromUTF8(&data);
		GAME_ASSERT(codepoint1);
		
		uint32_t codepoint2 = ReadNextCodepointFromUTF8(&data);
		GAME_ASSERT(codepoint2);

		SkipWhitespace(&data);
		GAME_ASSERT(*data);

		int tracking = 0;
		int scannedChars = 0;
		int scannedTokens = sscanf(data, "%d%n", &tracking, &scannedChars);
		GAME_ASSERT(scannedTokens == 1);
		data += scannedChars;

		AtlasGlyph* g = GetGlyphFromCodepoint(atlas, codepoint1);

		if (g)
		{
			if (g->numKernPairs == 0)
			{
				GAME_ASSERT(g->kernTableOffset == 0);
				g->kernTableOffset = kernTableOffset;
			}

			GAME_ASSERT_MESSAGE(g->numKernPairs == kernTableOffset - g->kernTableOffset, "kern pair blocks aren't contiguous!");

			atlas->kernPairs[kernTableOffset] = codepoint2;
			atlas->kernTracking[kernTableOffset] = tracking;
			kernTableOffset++;
			GAME_ASSERT(kernTableOffset <= MAX_KERNPAIRS);
			g->numKernPairs++;
		}

		SkipWhitespace(&data);
	}
}

/***************************************************************/
/*                       INIT/SHUTDOWN                         */
/***************************************************************/

void Atlas_LoadSlot(int slot, const char* atlasName, OGLSetupOutputType* setupInfo)
{
	GAME_ASSERT(!gAtlases[slot]);
	gAtlases[slot] = Atlas_Load(atlasName, setupInfo);
}

void Atlas_DisposeSlot(int slot)
{
	if (gAtlases[slot])
	{
		Atlas_Dispose(gAtlases[slot]);
		gAtlases[slot] = NULL;
	}
}

void Atlas_DisposeAllSlots(void)
{
	for (int i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		Atlas_DisposeSlot(i);
	}
}

Atlas* Atlas_Load(const char* fontName, OGLSetupOutputType* setupInfo)
{
	Atlas* atlas = AllocPtrClear(sizeof(Atlas));

	char pathBuf[256];

	snprintf(pathBuf, sizeof(pathBuf), ":sprites:%s.sfl", fontName);
	{
		// Parse metrics from SFL file
		const char* sflPath = pathBuf;
		char* data = LoadTextFile(sflPath, NULL);
		GAME_ASSERT(data);
		ParseSFL(atlas, data);
		SafeDisposePtr(data);
	}

	{
		// Parse kerning table
		char* data = LoadTextFile(":sprites:kerning.txt", NULL);
		GAME_ASSERT(data);
		ParseKerningFile(atlas, data);
		SafeDisposePtr(data);
	}

	snprintf(pathBuf, sizeof(pathBuf), ":sprites:%s.png", fontName);
	{
		// Create font material
		const char* texturePath = pathBuf;
		GLuint textureName = 0;
		textureName = OGL_TextureMap_LoadImageFile(texturePath, &atlas->textureWidth, &atlas->textureHeight);

		GAME_ASSERT(atlas->textureWidth != 0);
		GAME_ASSERT(atlas->textureHeight != 0);
		atlas->invTextureWidth = 1.0f / atlas->textureWidth;
		atlas->invTextureHeight = 1.0f / atlas->textureHeight;

		GAME_ASSERT_MESSAGE(!atlas->material, "atlas material already created");
		MOMaterialData matData;
		memset(&matData, 0, sizeof(matData));
		matData.setupInfo		= setupInfo;
		matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND | BG3D_MATERIALFLAG_TEXTURED | BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V;
		matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
		matData.numMipmaps		= 1;
		matData.width			= atlas->textureWidth;
		matData.height			= atlas->textureHeight;
		matData.textureName[0]	= textureName;
		atlas->material = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
	}

	return atlas;
}

void Atlas_Dispose(Atlas* atlas)
{
	MO_DisposeObjectReference(atlas->material);
	atlas->material = NULL;

	for (int i = 0; i < MAX_CODEPOINT_PAGES; i++)
	{
		if (atlas->glyphPages[i])
		{
			SafeDisposePtr((Ptr) atlas->glyphPages[i]);
			atlas->glyphPages[i] = NULL;
		}
	}

	SafeDisposePtr((Ptr) atlas);
}

/***************************************************************/
/*                MESH ALLOCATION/LAYOUT                       */
/***************************************************************/

static void TextMesh_ReallocateMesh(MOVertexArrayData* mesh, int numQuads)
{
	if (mesh->points)
	{
		SafeDisposePtr((Ptr) mesh->points);
		mesh->points = nil;
	}

	if (mesh->uvs)
	{
		SafeDisposePtr((Ptr) mesh->uvs);
		mesh->uvs = nil;
	}

	if (mesh->triangles)
	{
		SafeDisposePtr((Ptr) mesh->triangles);
		mesh->triangles = nil;
	}

	int numPoints = numQuads * 4;
	int numTriangles = numQuads * 2;

	if (numQuads != 0)
	{
		mesh->points = (OGLPoint3D *) AllocPtr(sizeof(OGLPoint3D) * numPoints);
		mesh->uvs = (OGLTextureCoord *) AllocPtr(sizeof(OGLTextureCoord) * numPoints);
		mesh->triangles = (MOTriangleIndecies *) AllocPtr(sizeof(MOTriangleIndecies) * numTriangles);
	}
}

static void TextMesh_InitMesh(MOVertexArrayData* mesh, int numQuads)
{
	memset(mesh, 0, sizeof(*mesh));
	
	GAME_ASSERT(gAtlases[0]);

	mesh->numMaterials = 1;
	mesh->materials[0] = gAtlases[0]->material;

	TextMesh_ReallocateMesh(mesh, numQuads);
}

static float Kern(const Atlas* font, const AtlasGlyph* glyph, const char* utftext)
{
	if (!glyph || !glyph->numKernPairs)
		return 1;

	uint32_t buddy = ReadNextCodepointFromUTF8(&utftext);

	for (int i = glyph->kernTableOffset; i < glyph->kernTableOffset + glyph->numKernPairs; i++)
	{
		if (font->kernPairs[i] == buddy)
			return font->kernTracking[i] * .01f;
	}

	return 1;
}

static void ComputeCounts(const Atlas* font, const char* text, int* numQuadsOut, int* numLinesOut, float* lineWidths, int maxLines)
{
	float spacing = 0;

	// Compute number of quads and line width
	int numLines = 1;
	int numQuads = 0;
	lineWidths[0] = 0;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t c = ReadNextCodepointFromUTF8(&utftext);

		if (c == '\n')
		{
			GAME_ASSERT(numLines < maxLines);
			numLines++;
			lineWidths[numLines-1] = 0;  // init next line
			continue;
		}

		if (c == '\t')
		{
			lineWidths[numLines-1] = TAB_STOP * ceilf((lineWidths[numLines-1] + 1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph* glyph = GetGlyphFromCodepoint(font, c);
		float kernFactor = Kern(font, glyph, utftext);
		lineWidths[numLines-1] += (glyph->xadv * kernFactor + spacing);
		if (c != ' ')
			numQuads++;
	}

	*numQuadsOut = numQuads;
	*numLinesOut = numLines;
}

static float GetLineStartX(int align, float lineWidth)
{
	if (align == kTextMeshAlignCenter)
		return -(lineWidth * .5f);
	else if (align == kTextMeshAlignRight)
		return -(lineWidth);
	else
		return 0;
}

void TextMesh_Update(const char* text, int align, ObjNode* textNode)
{
	const Atlas* font = gAtlases[0];
	GAME_ASSERT(font);

	//-----------------------------------
	// Get mesh from ObjNode

	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);
	GAME_ASSERT(textNode->BaseGroup);
	GAME_ASSERT(textNode->BaseGroup->objectData.numObjectsInGroup >= 2);

	MetaObjectPtr			metaObject			= textNode->BaseGroup->objectData.groupContents[1];
	MetaObjectHeader*		metaObjectHeader	= metaObject;
	MOVertexArrayObject*	vertexObject		= metaObject;
	MOVertexArrayData*		mesh				= &vertexObject->objectData;

	GAME_ASSERT(metaObjectHeader->type == MO_TYPE_GEOMETRY);
	GAME_ASSERT(metaObjectHeader->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY);

	//-----------------------------------

	float x = 0;
	float y = 0;
	float z = 0;
	float spacing = 0;

	const float invAtlasW = 1.0f / font->textureWidth;
	const float invAtlasH = 1.0f / font->textureHeight;

	// Compute number of quads and line width
	float lineWidths[MAX_LINEBREAKS_PER_OBJNODE];
	int numLines = 0;
	int numQuads = 0;
	ComputeCounts(font, text, &numQuads, &numLines, lineWidths, MAX_LINEBREAKS_PER_OBJNODE);

	// Compute longest line width
	float longestLineWidth = 0;
	for (int i = 0; i < numLines; i++)
	{
		if (lineWidths[i] > longestLineWidth)
			longestLineWidth = lineWidths[i];
	}

	// Adjust y for ascender
	y += 0.5f * font->lineHeight;

	// Center vertically
	y -= 0.5f * font->lineHeight * (numLines - 1);

	// Save extents
	textNode->LeftOff	= GetLineStartX(align, longestLineWidth);
	textNode->RightOff	= textNode->LeftOff + longestLineWidth;
	textNode->TopOff	= y - font->lineHeight;
	textNode->BottomOff	= textNode->TopOff + font->lineHeight * numLines;

	// Ensure mesh has capacity for quads
	if (textNode->TextQuadCapacity < numQuads)
	{
		textNode->TextQuadCapacity = numQuads * 2;		// avoid reallocating often if text keeps growing
		TextMesh_ReallocateMesh(mesh, textNode->TextQuadCapacity);
	}

	// Set # of triangles and points
	mesh->numTriangles = numQuads*2;
	mesh->numPoints = numQuads*4;

	GAME_ASSERT(mesh->numTriangles >= numQuads*2);
	GAME_ASSERT(mesh->numPoints >= numQuads*4);

	if (numQuads == 0)
		return;

	GAME_ASSERT(mesh->uvs);
	GAME_ASSERT(mesh->triangles);
	GAME_ASSERT(mesh->numMaterials == 1);
	GAME_ASSERT(mesh->materials[0]);

	// Create a quad for each character
	int t = 0;		// triangle counter
	int p = 0;		// point counter
	int currentLine = 0;
	x = GetLineStartX(align, lineWidths[0]);
	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext);

		if (codepoint == '\n')
		{
			currentLine++;
			x = GetLineStartX(align, lineWidths[currentLine]);
			y += font->lineHeight;
			continue;
		}

		if (codepoint == '\t')
		{
			x = TAB_STOP * ceilf((x+1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph g = *GetGlyphFromCodepoint(font, codepoint);

		if (codepoint == ' ')
		{
			x += (g.xadv + spacing);
			continue;
		}

		float hw = .5f * g.w;
		float hh = .5f * g.h;
		float qx = x + (g.xoff + hw);
		float qy = y - (g.yoff + hh);

		mesh->triangles[t + 0].vertexIndices[0] = p + 0;
		mesh->triangles[t + 0].vertexIndices[1] = p + 2;
		mesh->triangles[t + 0].vertexIndices[2] = p + 1;
		mesh->triangles[t + 1].vertexIndices[0] = p + 0;
		mesh->triangles[t + 1].vertexIndices[1] = p + 3;
		mesh->triangles[t + 1].vertexIndices[2] = p + 2;
		mesh->points[p + 0] = (OGLPoint3D) { qx - hw, qy - hh, z };
		mesh->points[p + 1] = (OGLPoint3D) { qx + hw, qy - hh, z };
		mesh->points[p + 2] = (OGLPoint3D) { qx + hw, qy + hh, z };
		mesh->points[p + 3] = (OGLPoint3D) { qx - hw, qy + hh, z };
		mesh->uvs[p + 0] = (OGLTextureCoord) { invAtlasW * g.x,			invAtlasH * g.y };
		mesh->uvs[p + 1] = (OGLTextureCoord) { invAtlasW * (g.x+g.w),	invAtlasH * g.y };
		mesh->uvs[p + 2] = (OGLTextureCoord) { invAtlasW * (g.x+g.w),	invAtlasH * (g.y+g.h) };
		mesh->uvs[p + 3] = (OGLTextureCoord) { invAtlasW * g.x,			invAtlasH * (g.y+g.h) };

		float kernFactor = Kern(font, &g, utftext);
		x += (g.xadv*kernFactor + spacing);
		t += 2;
		p += 4;
	}

	GAME_ASSERT(p == mesh->numPoints);
}

/***************************************************************/
/*                    API IMPLEMENTATION                       */
/***************************************************************/

ObjNode *TextMesh_NewEmpty(int capacity, NewObjectDefinitionType* newObjDef)
{
	MOVertexArrayData mesh;
	TextMesh_InitMesh(&mesh, capacity);

	newObjDef->genre = TEXTMESH_GENRE;
	newObjDef->flags |= STATUS_BITS_FOR_2D;
	ObjNode* textNode = MakeNewObject(newObjDef);

	textNode->Projection = kProjectionType2DOrthoCentered;

	// Attach color mesh
	MetaObjectPtr meshMO = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &mesh);

	CreateBaseGroup(textNode);
	AttachGeometryToDisplayGroupObject(textNode, meshMO);

	textNode->TextQuadCapacity = capacity;

	// Dispose of extra reference to mesh
	MO_DisposeObjectReference(meshMO);

	UpdateObjectTransforms(textNode);

	return textNode;
}

ObjNode *TextMesh_New(const char *text, int align, NewObjectDefinitionType* newObjDef)
{
	ObjNode* textNode = TextMesh_NewEmpty(0, newObjDef);
	TextMesh_Update(text, align, textNode);
	return textNode;
}

#if 0 // REWRITE ME for multi-line strings
float TextMesh_GetCharX(const char* text, int n)
{
	float x = 0;
	for (const char *utftext = text;
		*utftext && n;
		n--)
	{
		uint32_t c = ReadNextCodepointFromUTF8(&utftext);

		if (c == '\n')		// TODO: line widths for strings containing line breaks aren't supported yet
			continue;

		const AtlasGlyph* glyph = GetGlyphFromCodepoint(c);
		float kernFactor = Kern(glyph, utftext);
		x += glyph->xadv * kernFactor;
	}
	return x;
}
#endif

OGLRect TextMesh_GetExtents(ObjNode* textNode)
{
	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);

	return (OGLRect)
	{
		.left		= textNode->Coord.x + textNode->Scale.x * textNode->LeftOff,
		.right		= textNode->Coord.x + textNode->Scale.x * textNode->RightOff,
		.top		= textNode->Coord.y + textNode->Scale.y * textNode->TopOff,
		.bottom		= textNode->Coord.y + textNode->Scale.y * textNode->BottomOff,
	};
}

void TextMesh_DrawExtents(ObjNode* textNode)
{
	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);

	OGL_PushState();								// keep state
//	SetInfobarSpriteState(true);
	glDisable(GL_TEXTURE_2D);

	OGLRect extents = TextMesh_GetExtents(textNode);
	float z = textNode->Coord.z;

	glColor4f(1,1,1,1);
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents.left,		extents.top,	z);
	glVertex3f(extents.right,		extents.top,	z);
	glColor4f(0,.5,1,1);
	glVertex3f(extents.right,		extents.bottom,	z);
	glVertex3f(extents.left,		extents.bottom,	z);
	glEnd();

	OGL_PopState();
}

void Atlas_DrawImmediate(
	int slot,
	const char* text,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo)
{
	GAME_ASSERT((size_t)slot < (size_t)MAX_SPRITE_GROUPS);

	const Atlas* font = gAtlases[slot];
	GAME_ASSERT(font);

			/* SET STATE */

	OGL_PushState();								// keep state
	OGL_SetProjection(kProjectionType2DNDC);

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	if (flags & SPRITE_FLAG_GLOW)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glTranslatef(x,y,0);

	float scaleBasis = 2.0f / SPRITE_SCALE_BASIS_DENOMINATOR;
	glScalef(scale * scaleBasis, scale * gCurrentAspectRatio * scaleBasis, 1);

	if (rot != 0.0f)
		glRotatef(OGLMath_RadiansToDegrees(rot), 0, 0, 1);											// remember:  rotation is in degrees, not radians!


		/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(font->material, setupInfo);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);						// set clamp mode after each texture set since OGL just likes it that way
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


			/* DRAW IT */

	const float invAtlasW = 1.0f / font->textureWidth;
	const float invAtlasH = 1.0f / font->textureHeight;

	glBegin(GL_QUADS);
	float cx = -32;  // hack to make text origin fit where CMR infobar expects it
	float cy = -32;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext);
		const AtlasGlyph* glyph = GetGlyphFromCodepoint(font, codepoint);

		const float gl = invAtlasW * glyph->x;
		const float gt = invAtlasH * glyph->y;
		const float gr = invAtlasW * (glyph->x + glyph->w);
		const float gb = invAtlasH * (glyph->y + glyph->h);

		float halfw = .5f * glyph->w;
		float halfh = .5f * glyph->h;
		float qx = cx + (glyph->xoff + halfw);
		float qy = cy + (glyph->yoff + halfh);

		glTexCoord2f(gl,gt);	glVertex3f(qx - halfw, qy + halfh, 0);
		glTexCoord2f(gr,gt);	glVertex3f(qx + halfw, qy + halfh, 0);
		glTexCoord2f(gr,gb);	glVertex3f(qx + halfw, qy - halfh, 0);
		glTexCoord2f(gl,gb);	glVertex3f(qx - halfw, qy - halfh, 0);

		cx += glyph->xadv * Kern(font, glyph, utftext);

		gPolysThisFrame += 2;						// 2 tris drawn
	}
	glEnd();

		/* CLEAN UP */

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);						// set this back to normal
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	OGL_PopState();									// restore state
}
