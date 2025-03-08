// SPRITE ATLAS / TEXT MESHES
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <ctype.h>

/****************************/
/*    CONSTANTS             */
/****************************/

#define TAB_STOP 128.0f

#define MAX_LINEBREAKS_PER_OBJNODE	16

#define MAX_IMMEDIATEMODE_QUADS		1024

#define SUBSCRIPT_SCALE				0.66f

enum
{
	kControlChar_LineBreak				= '\n',
	kControlChar_Tab					= '\t',
	kControlChar_Subscript				= '\v',
	kControlChar_ResetInlineFormatting	= '\r',
};

/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	int numQuads;
	int numLines;
	float bbWidth;
	float bbHeight;
	float lineWidths[MAX_LINEBREAKS_PER_OBJNODE];
	float lineHeights[MAX_LINEBREAKS_PER_OBJNODE];
	float lineOffsetX[MAX_LINEBREAKS_PER_OBJNODE];
	float lineOffsetY[MAX_LINEBREAKS_PER_OBJNODE];
} TextMetrics;

/****************************/
/*    VARIABLES             */
/****************************/

static OGLPoint3D gImmediateModePoints[MAX_IMMEDIATEMODE_QUADS * 4];
static OGLTextureCoord gImmediateModeUVs[MAX_IMMEDIATEMODE_QUADS * 4];

#pragma mark -

/***************************************************************/
/*                         UTF-8                               */
/***************************************************************/

static uint32_t ReadNextCodepointFromUTF8(const char** utf8TextPtr, int flags)
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

	if (flags & kTextMeshAllCaps)
	{
		codepoint = toupper(codepoint);
	}

	return codepoint;

#undef TRY_ADVANCE
}

/***************************************************************/
/*                    GET/SET GLYPHS                           */
/***************************************************************/

static AtlasGlyph* Atlas_GetGlyphPtr(const Atlas* atlas, uint32_t codepoint)
{
	uint32_t page = codepoint >> 8;

	if (page >= atlas->maxPages
		|| NULL == atlas->glyphPages[page])
	{
		return NULL;
	}

	return &atlas->glyphPages[page][codepoint & 0xFF];
}

const AtlasGlyph* Atlas_GetGlyph(const Atlas* atlas, uint32_t codepoint)
{
	return Atlas_GetGlyphPtr(atlas, codepoint);
}

static void Atlas_SetGlyph(Atlas* atlas, uint32_t codepoint, AtlasGlyph* src)
{
	// Compute page for codepoint
	uint32_t page = codepoint >> 8;
	if (page >= atlas->maxPages)
	{
		SDL_Log("WARNING: codepoint 0x%x exceeds supported maximum (0x%x)", codepoint, atlas->maxPages * 256 - 1);
		return;
	}

	// Allocate codepoint page
	if (atlas->glyphPages[page] == NULL)
	{
		atlas->glyphPages[page] = AllocPtrClear(sizeof(AtlasGlyph) * 256);
	}

	// Store glyph
	atlas->glyphPages[page][codepoint & 0xFF] = *src;
}

/***************************************************************/
/*                   PARSE METRICS FILE                        */
/***************************************************************/

static void ParseAtlasMetrics_SkipLine(const char** dataPtr)
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

	*dataPtr = data;
}

static void ParseAtlasMetrics(Atlas* atlas, const char* data, int imageWidth, int imageHeight)
{
	int nArgs = 0;
	int nGlyphs = 0;

	nArgs = SDL_sscanf(data, "%d %f", &nGlyphs, &atlas->lineHeight);
	GAME_ASSERT(nArgs == 2);
	ParseAtlasMetrics_SkipLine(&data);  // Skip rest of line (name)

	for (int i = 0; i < nGlyphs; i++)
	{
		AtlasGlyph newGlyph;
		SDL_memset(&newGlyph, 0, sizeof(newGlyph));

		uint32_t codepoint = 0;
		float x, y;

		nArgs = SDL_sscanf(
				data,
				"%d %f %f %f %f %f %f %f %f",
				&codepoint,
				&x,
				&y,
				&newGlyph.w,
				&newGlyph.h,
				&newGlyph.xoff,
				&newGlyph.yoff,
				&newGlyph.xadv,
				&newGlyph.yadv);
		GAME_ASSERT(nArgs == 9);

		ParseAtlasMetrics_SkipLine(&data);  // Skip rest of line

		newGlyph.u1 =  x               / (float)imageWidth;
		newGlyph.u2 = (x + newGlyph.w) / (float)imageWidth;
		newGlyph.v1 =  y               / (float)imageHeight;
		newGlyph.v2 = (y + newGlyph.h) / (float)imageHeight;

		Atlas_SetGlyph(atlas, codepoint, &newGlyph);
	}

	// Force monospaced numbers
	if (atlas->isASCIIFont)
	{
		AtlasGlyph* asciiPage = atlas->glyphPages[0];
		AtlasGlyph referenceNumber = asciiPage['4'];
		for (int c = '0'; c <= '9'; c++)
		{
			asciiPage[c].xoff += (referenceNumber.w - asciiPage[c].w) / 2.0f;
			asciiPage[c].xadv = referenceNumber.xadv;
		}
	}
}

/***************************************************************/
/*                 PARSE KERNING TABLE                         */
/***************************************************************/

static void SkipWhitespace(const char** data)
{
	while (**data && SDL_strchr("\t\r\n ", **data))
	{
		(*data)++;
	}
}

static void ParseKerningFile(Atlas* atlas, const char* data)
{
	int kernTableOffset = 0;

	while (*data)
	{
		uint32_t codepoint1 = ReadNextCodepointFromUTF8(&data, 0);
		GAME_ASSERT(codepoint1);
		
		uint32_t codepoint2 = ReadNextCodepointFromUTF8(&data, 0);
		GAME_ASSERT(codepoint2);

		SkipWhitespace(&data);
		GAME_ASSERT(*data);

		int tracking = 0;
		int scannedChars = 0;
		int scannedTokens = SDL_sscanf(data, "%d%n", &tracking, &scannedChars);
		GAME_ASSERT(scannedTokens == 1);
		data += scannedChars;

		AtlasGlyph* g = (AtlasGlyph*) Atlas_GetGlyph(atlas, codepoint1);

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

Atlas* Atlas_Load(const char* fontName, int flags)
{
	Atlas* atlas = AllocPtrClear(sizeof(Atlas));

	if (flags & kAtlasLoadFont)
	{
		atlas->isASCIIFont = true;

		atlas->maxPages = MAX_CODEPOINT_PAGES;
		atlas->glyphPages = AllocPtrClear(sizeof(AtlasGlyph*) * atlas->maxPages);

		atlas->kernPairs = AllocPtrClear(sizeof(atlas->kernPairs[0]) * MAX_KERNPAIRS);
		atlas->kernTracking = AllocPtrClear(sizeof(atlas->kernTracking[0]) * MAX_KERNPAIRS);
	}
	else
	{
		atlas->maxPages = 1;
		atlas->glyphPages = AllocPtrClear(sizeof(AtlasGlyph*) * atlas->maxPages);

		atlas->kernPairs = nil;
		atlas->kernTracking = nil;
	}

	SDL_snprintf(atlas->name, sizeof(atlas->name), "%s", fontName);

	char pathBuf[256];
	SDL_snprintf(pathBuf, sizeof(pathBuf), ":Sprites:%s.png", fontName);
#if _DEBUG
	SDL_Log("Atlas_Load: %s", pathBuf);
#endif

	{
		// Create font material
		const char* texturePath = pathBuf;
		GLuint textureName = 0;
		textureName = OGL_TextureMap_LoadImageFile(texturePath, &atlas->textureWidth, &atlas->textureHeight);

		GAME_ASSERT(atlas->textureWidth != 0);
		GAME_ASSERT(atlas->textureHeight != 0);

		GAME_ASSERT_MESSAGE(!atlas->material, "atlas material already created");
		MOMaterialData matData;
		SDL_memset(&matData, 0, sizeof(matData));
//		matData.setupInfo		= setupInfo;
		matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND | BG3D_MATERIALFLAG_TEXTURED | BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V;
		matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
		matData.numMipmaps		= 1;
		matData.width			= atlas->textureWidth;
		matData.height			= atlas->textureHeight;
		matData.textureName[0]	= textureName;
		atlas->material = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
	}

	if (!(flags & kAtlasLoadAsSingleSprite))
	{
		SDL_snprintf(pathBuf, sizeof(pathBuf), ":Sprites:%s.txt", fontName);
		// Parse metrics from SFL file
		const char* sflPath = pathBuf;
		char* data = LoadTextFile(sflPath, NULL);
		GAME_ASSERT(data);
		ParseAtlasMetrics(atlas, data, atlas->textureWidth, atlas->textureHeight);
		SafeDisposePtr(data);
	}
	else
	{
		// Create single glyph #1
		float w = atlas->material->objectData.width;
		float h = atlas->material->objectData.height;
		AtlasGlyph newGlyph =
		{
			.xadv = w,
			.yadv = h,
			.w = w,
			.h = h,
			.u2 = 1,
			.v2 = 1,
			.xoff = 0,
			.yoff = 0,
		};
		Atlas_SetGlyph(atlas, 1, &newGlyph);
	}

	if (flags & kAtlasLoadFont)
	{
		// Parse kerning table
		char* data = LoadTextFile(":System:kerning.txt", NULL);
		GAME_ASSERT(data);
		ParseKerningFile(atlas, data);
		SafeDisposePtr(data);

	}

	return atlas;
}

void Atlas_Dispose(Atlas* atlas)
{
	MO_DisposeObjectReference(atlas->material);
	atlas->material = NULL;

	for (size_t i = 0; i < atlas->maxPages; i++)
	{
		if (atlas->glyphPages[i])
		{
			SafeDisposePtr((Ptr) atlas->glyphPages[i]);
			atlas->glyphPages[i] = NULL;
		}
	}

	SafeDisposePtr((Ptr) atlas->glyphPages);
	atlas->glyphPages = nil;

	if (atlas->kernPairs)
	{
		SafeDisposePtr((Ptr) atlas->kernPairs);
		atlas->kernPairs = nil;
	}

	if (atlas->kernTracking)
	{
		SafeDisposePtr((Ptr) atlas->kernTracking);
		atlas->kernTracking = nil;
	}

	SafeDisposePtr((Ptr) atlas);
}

/***************************************************************/
/*                MESH ALLOCATION/LAYOUT                       */
/***************************************************************/

static float Kern(const Atlas* font, const AtlasGlyph* glyph, const char* utftext, int flags)
{
	if (!glyph || !glyph->numKernPairs)
		return 1;

	uint32_t buddy = ReadNextCodepointFromUTF8(&utftext, flags);

	for (int i = glyph->kernTableOffset; i < glyph->kernTableOffset + glyph->numKernPairs; i++)
	{
		if (font->kernPairs[i] == buddy)
			return font->kernTracking[i] * .01f;
	}

	return 1;
}

static void ComputeMetrics(const Atlas* atlas, const char* text, int flags, TextMetrics* m)
{
	int currentLine = 0;
	float glyphScale = 1;

	// Compute number of quads and line width
	m->numLines = 1;
	m->numQuads = 0;
	m->lineWidths[0] = 0;
	m->lineHeights[0] = 0;
	m->bbWidth = 0;
	m->bbHeight = 0;

	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext, flags);

		if (atlas->isASCIIFont)			// Parse control characters if it's a font
		{
			switch (codepoint)
			{
				case kControlChar_LineBreak:
					m->bbWidth = GAME_MAX(m->bbWidth, m->lineWidths[currentLine]);
					m->bbHeight += m->lineHeights[currentLine];

					currentLine++;
					GAME_ASSERT(currentLine < MAX_LINEBREAKS_PER_OBJNODE);

					m->lineWidths[currentLine] = 0;  // init next line
					m->lineHeights[currentLine] = 0;
					continue;

				case kControlChar_Tab:
					m->lineWidths[currentLine] = TAB_STOP * ceilf((m->lineWidths[currentLine] + 1.0f) / TAB_STOP);
					continue;

				case kControlChar_Subscript:
					glyphScale *= SUBSCRIPT_SCALE;
					continue;

				case kControlChar_ResetInlineFormatting:
					glyphScale = 1;
					continue;

				default:
					break;
			}
		}

		const AtlasGlyph* glyph = Atlas_GetGlyph(atlas, codepoint);
		if (!glyph)
			continue;

		float kernFactor;
		float glyphHeight;

		if (atlas->isASCIIFont)
		{
			kernFactor = Kern(atlas, glyph, utftext, flags);
			glyphHeight = atlas->lineHeight;
		}
		else
		{
			kernFactor = 1;
			glyphHeight = glyph->yadv;
		}

		m->lineWidths[currentLine] += glyphScale * (glyph->xadv * kernFactor);
		m->lineHeights[currentLine] = GAME_MAX(m->lineHeights[currentLine], glyphHeight);

		if (glyph->w > 0)		// zero-width glyphs don't produce a quad (e.g. space)
		{
			m->numQuads++;
		}
	}

	// Commit last line
	m->bbWidth = GAME_MAX(m->bbWidth, m->lineWidths[currentLine]);
	m->bbHeight += m->lineHeights[currentLine];

	// Commit line count
	m->numLines = currentLine+1;

	//---------------------------------

	switch (flags & (kTextMeshAlignCenter | kTextMeshAlignLeft | kTextMeshAlignRight))
	{
		case kTextMeshAlignLeft:
			SDL_memset(m->lineOffsetX, 0, sizeof(m->lineOffsetX));
			break;

		case kTextMeshAlignRight:
			for (int i = 0; i < m->numLines; i++)
				m->lineOffsetX[i] = -m->lineWidths[i];
			break;

		default:
			for (int i = 0; i < m->numLines; i++)
				m->lineOffsetX[i] = -m->lineWidths[i] * .5f;
			break;
	}

	//---------------------------------


	float startY = 0;
	switch (flags & (kTextMeshAlignMiddle | kTextMeshAlignTop | kTextMeshAlignBottom))
	{
		case kTextMeshAlignTop:
			startY = 0;
			break;

		case kTextMeshAlignBottom:
			startY = -m->bbHeight;
			break;

		default:
			startY = -m->bbHeight * .5f;
			break;
	}

	for (int i = 0; i < m->numLines; i++)
	{
		m->lineOffsetY[i] = startY;
		startY += m->lineHeights[i];
	}
}

static void PrepVertices(
		const Atlas* atlas,
		const char* text,
		int flags,
		const TextMetrics* metrics,
		OGLPoint3D* points,
		OGLTextureCoord* uvs)
{
	float x = 0;
	float y = 0;
	float z = 0;

	// Get top-left corner of text
	x = metrics->lineOffsetX[0];
	y = metrics->lineOffsetY[0];

	int p = 0;		// point counter
	int currentLine = 0;
	float glyphScale = 1;

	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext, flags);

		if (atlas->isASCIIFont)			// Parse control characters if it's a font
		{
			switch (codepoint)
			{
				case kControlChar_LineBreak:
					currentLine++;
					x = metrics->lineOffsetX[currentLine];
					y = metrics->lineOffsetY[currentLine];
					glyphScale = 1;
					continue;

				case kControlChar_Tab:
					x = TAB_STOP * ceilf((x + 1.0f) / TAB_STOP);
					continue;

				case kControlChar_Subscript:
					y += atlas->lineHeight * (1.0f - SUBSCRIPT_SCALE * 1.05f);
					glyphScale *= SUBSCRIPT_SCALE;
					continue;

				case kControlChar_ResetInlineFormatting:
					y = metrics->lineOffsetY[currentLine];
					glyphScale = 1;
					continue;

				default:
					break;
			}
		}

		const AtlasGlyph* g = Atlas_GetGlyph(atlas, codepoint);
		if (!g)
			continue;

		if (g->w <= 0.0f)	// e.g. space codepoint
		{
			x += (g->xadv);
			continue;
		}

		float left   = x    + glyphScale * g->xoff;
		float top    = y    + glyphScale * g->yoff;
		float right  = left + glyphScale * g->w;
		float bottom = top  + glyphScale * g->h;

		uvs[p+0] = (OGLTextureCoord) {g->u1, g->v2};
		uvs[p+1] = (OGLTextureCoord) {g->u2, g->v2};
		uvs[p+2] = (OGLTextureCoord) {g->u2, g->v1};
		uvs[p+3] = (OGLTextureCoord) {g->u1, g->v1};
		points[p+0] = (OGLPoint3D) { left , bottom, z };
		points[p+1] = (OGLPoint3D) { right, bottom, z };
		points[p+2] = (OGLPoint3D) { right, top   , z };
		points[p+3] = (OGLPoint3D) { left , top   , z };

		float xadv = g->xadv;
		if (atlas->isASCIIFont)
			xadv *= Kern(atlas, g, utftext, flags);

		x += glyphScale * xadv;
		p += 4;			// 4 more vertices
	}
}

static OGLRect GetExtentsFromMetrics(const TextMetrics* metrics)
{
	return (OGLRect)
	{
			.left	= metrics->lineOffsetX[0],
			.top	= metrics->lineOffsetY[0],
			.right	= metrics->lineOffsetX[0] + metrics->bbWidth,
			.bottom	= metrics->lineOffsetY[0] + metrics->bbHeight,
	};
}

void TextMesh_Update(const char* text, int flags, ObjNode* textNode)
{
	const Atlas* font = gAtlases[0];
	GAME_ASSERT(font);

	//-----------------------------------
	// Get mesh from ObjNode

	MOVertexArrayData*		mesh = GetQuadMeshWithin(textNode);

	// Compute number of quads and line width
	TextMetrics metrics;
	ComputeMetrics(font, text, flags, &metrics);

	// Save extents
	{
		OGLRect extents = GetExtentsFromMetrics(&metrics);
		textNode->LeftOff	= extents.left;
		textNode->TopOff	= extents.top;
		textNode->RightOff	= extents.right;
		textNode->BottomOff	= extents.bottom;
	}

	// Ensure mesh has capacity for quads
	int quadCapacity = mesh->triangleCapacity/2;
	if (quadCapacity < metrics.numQuads)
	{
		quadCapacity = metrics.numQuads * 2;		// avoid reallocating often if text keeps growing
		ReallocateQuadMesh(mesh, quadCapacity);
	}

	// Set # of triangles and points
	mesh->numTriangles = metrics.numQuads*2;
	mesh->numPoints = metrics.numQuads*4;

	GAME_ASSERT(mesh->numTriangles >= metrics.numQuads*2);
	GAME_ASSERT(mesh->numPoints >= metrics.numQuads*4);

	if (metrics.numQuads == 0)
		return;

	GAME_ASSERT(mesh->uvs);
	GAME_ASSERT(mesh->triangles);
	GAME_ASSERT(mesh->numMaterials == 1);
	GAME_ASSERT(mesh->materials[0]);

	// Lay out triangles
	PrepVertices(font, text, flags, &metrics, mesh->points, mesh->uvs);
}

/***************************************************************/
/*                    API IMPLEMENTATION                       */
/***************************************************************/

ObjNode *TextMesh_NewEmpty(int capacity, NewObjectDefinitionType* newObjDef)
{
	// Patch newObjDef with bare minimum flags for TextMesh
	newObjDef->genre = TEXTMESH_GENRE;
	newObjDef->flags |= STATUS_BITS_FOR_2D;

	// Fall back to 2D projection if standard projection (3D) is set
	if (!newObjDef->projection)
	{
		newObjDef->projection = kProjectionType2DOrthoCentered;
	}

	GAME_ASSERT(gAtlases[SPRITE_GROUP_FONT]);
	MOMaterialObject* material = gAtlases[SPRITE_GROUP_FONT]->material;

	// Create mesh object
	return MakeQuadMeshObject(newObjDef, capacity, material);
}

ObjNode *TextMesh_New(const char *text, int align, NewObjectDefinitionType* newObjDef)
{
	ObjNode* textNode = TextMesh_NewEmpty(0, newObjDef);
	TextMesh_Update(text, align, textNode);
	return textNode;
}

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

static void DrawExtents(OGLRect extents, float z)
{
	OGL_PushState();								// keep state
//	SetInfobarSpriteState(true);
	glDisable(GL_TEXTURE_2D);

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

void Atlas_ImmediateDraw(int groupNum, const char* text, uint32_t flags)
{
	GAME_ASSERT((size_t)groupNum < (size_t)MAX_SPRITE_GROUPS);

	const Atlas* font = gAtlases[groupNum];
	GAME_ASSERT(font);

			/* GET TEXT METRICS*/

	TextMetrics metrics;
	ComputeMetrics(font, text, flags, &metrics);

	GAME_ASSERT_MESSAGE(metrics.numQuads < MAX_IMMEDIATEMODE_QUADS,
						"Can't draw this many quads in immediate mode!");

	PrepVertices(font, text, flags, &metrics, gImmediateModePoints, gImmediateModeUVs);

			/* DRAW BOUNDING RECT */

	if (gDebugMode >= 2)
	{
		OGLRect extents = GetExtentsFromMetrics(&metrics);
		DrawExtents(extents, 0);
	}

			/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(font->material);

			/* DRAW IT */

	glBegin(GL_QUADS);
	const OGLPoint3D* pt = gImmediateModePoints;
	const OGLTextureCoord* uv = gImmediateModeUVs;
	for (int p = 0; p < 4*metrics.numQuads; p += 4)
	{
		glTexCoord2f(uv[p+0].u, uv[p+0].v);	glVertex3f(pt[p+0].x, pt[p+0].y, 0);
		glTexCoord2f(uv[p+1].u, uv[p+1].v);	glVertex3f(pt[p+1].x, pt[p+1].y, 0);
		glTexCoord2f(uv[p+2].u, uv[p+2].v);	glVertex3f(pt[p+2].x, pt[p+2].y, 0);
		glTexCoord2f(uv[p+3].u, uv[p+3].v);	glVertex3f(pt[p+3].x, pt[p+3].y, 0);
	}
	glEnd();
	gPolysThisFrame += 2*metrics.numQuads;						// 2 tris drawn per quad

}

void Atlas_DrawString2(
	int groupNum,
	const char* text,
	float x,
	float y,
	float scaleX,
	float scaleY,
	float rot,
	uint32_t flags)
{
			/* SET STATE */

	OGL_PushState();								// keep state

	if (!(flags & kTextMeshKeepCurrentProjection))
		OGL_SetProjection(kProjectionType2DOrthoCentered);

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	if (flags & kTextMeshGlow)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glTranslatef(x,y,0);
	glScalef(scaleX, scaleY, 1);					// Assume ortho projection

	if (rot != 0.0f)
		glRotatef(OGLMath_RadiansToDegrees(rot), 0, 0, 1);											// remember:  rotation is in degrees, not radians!

	Atlas_ImmediateDraw(groupNum, text, flags);

		/* CLEAN UP */

	OGL_PopState();									// restore state
}
