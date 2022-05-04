// SPRITE ATLAS / TEXT MESHES
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

/****************************/
/*    CONSTANTS             */
/****************************/

#define TAB_STOP 60.0f

#define MAX_LINEBREAKS_PER_OBJNODE	8

/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	int numQuads;
	int numLines;
	float lineWidths[MAX_LINEBREAKS_PER_OBJNODE];
	float lineHeights[MAX_LINEBREAKS_PER_OBJNODE];
	float longestLineWidth;
} TextMetrics;

/****************************/
/*    VARIABLES             */
/****************************/

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

	if (page >= MAX_CODEPOINT_PAGES
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
	if (page >= MAX_CODEPOINT_PAGES)
	{
		printf("WARNING: codepoint 0x%x exceeds supported maximum (0x%x)\n", codepoint, MAX_CODEPOINT_PAGES * 256 - 1);
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

	nArgs = sscanf(data, "%d %f", &nGlyphs, &atlas->lineHeight);
	GAME_ASSERT(nArgs == 2);
	ParseAtlasMetrics_SkipLine(&data);  // Skip rest of line (name)

	for (int i = 0; i < nGlyphs; i++)
	{
		AtlasGlyph newGlyph;
		memset(&newGlyph, 0, sizeof(newGlyph));

		uint32_t codepoint = 0;
		float x, y;

		nArgs = sscanf(
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

#if 0
	// Force monospaced numbers
	AtlasGlyph* asciiPage = atlas->glyphPages[0];
	AtlasGlyph referenceNumber = asciiPage['4'];
	for (int c = '0'; c <= '9'; c++)
	{
		asciiPage[c].xoff += (referenceNumber.w - asciiPage[c].w) / 2.0f;
		asciiPage[c].xadv = referenceNumber.xadv;
	}
#endif
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
		uint32_t codepoint1 = ReadNextCodepointFromUTF8(&data, 0);
		GAME_ASSERT(codepoint1);
		
		uint32_t codepoint2 = ReadNextCodepointFromUTF8(&data, 0);
		GAME_ASSERT(codepoint2);

		SkipWhitespace(&data);
		GAME_ASSERT(*data);

		int tracking = 0;
		int scannedChars = 0;
		int scannedTokens = sscanf(data, "%d%n", &tracking, &scannedChars);
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

Atlas* Atlas_Load(const char* fontName, int flags, OGLSetupOutputType* setupInfo)
{
	Atlas* atlas = AllocPtrClear(sizeof(Atlas));

	snprintf(atlas->name, sizeof(atlas->name), "%s", fontName);

	char pathBuf[256];

	snprintf(pathBuf, sizeof(pathBuf), ":sprites:%s.png", fontName);
	printf("Atlas_Load: %s\n", pathBuf);
	{
		// Create font material
		const char* texturePath = pathBuf;
		GLuint textureName = 0;
		textureName = OGL_TextureMap_LoadImageFile(texturePath, &atlas->textureWidth, &atlas->textureHeight);

		GAME_ASSERT(atlas->textureWidth != 0);
		GAME_ASSERT(atlas->textureHeight != 0);

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

	if (!(flags & kAtlasLoadAsSingleSprite))
	{
		snprintf(pathBuf, sizeof(pathBuf), ":sprites:%s.txt", fontName);
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
		char* data = LoadTextFile(":system:kerning.txt", NULL);
		GAME_ASSERT(data);
		ParseKerningFile(atlas, data);
		SafeDisposePtr(data);

		atlas->isASCIIFont = true;
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
	
	GAME_ASSERT(gAtlases[SPRITE_GROUP_FONT]);

	mesh->numMaterials = 1;
	mesh->materials[0] = gAtlases[SPRITE_GROUP_FONT]->material;

	TextMesh_ReallocateMesh(mesh, numQuads);
}

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

static void ComputeMetrics(const Atlas* atlas, const char* text, int flags, TextMetrics* metrics)
{
	float spacing = 0;

	// Compute number of quads and line width
	metrics->numLines = 1;
	metrics->numQuads = 0;
	metrics->lineWidths[0] = 0;
	metrics->lineHeights[0] = 0;
	metrics->longestLineWidth = 0;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext, flags);

		if (atlas->isASCIIFont)
		{
			if (codepoint == '\n')
			{
				GAME_ASSERT(metrics->numLines < MAX_LINEBREAKS_PER_OBJNODE);

				if (metrics->lineWidths[metrics->numLines - 1] > metrics->longestLineWidth)
					metrics->longestLineWidth = metrics->lineWidths[metrics->numLines - 1];

				metrics->numLines++;

				metrics->lineWidths[metrics->numLines - 1] = 0;  // init next line
				metrics->lineHeights[metrics->numLines - 1] = 0;
				continue;
			}
			else if (codepoint == '\t')
			{
				metrics->lineWidths[metrics->numLines - 1] = TAB_STOP * ceilf((metrics->lineWidths[metrics->numLines - 1] + 1.0f) / TAB_STOP);
				continue;
			}
		}

		const AtlasGlyph* glyph = Atlas_GetGlyph(atlas, codepoint);
		if (!glyph)
			continue;

		float kernFactor = 1;

		if (atlas->isASCIIFont)
		{
			kernFactor = Kern(atlas, glyph, utftext, flags);
		}

		metrics->lineWidths[metrics->numLines-1] += (glyph->xadv * kernFactor + spacing);

		float gh = glyph->yadv;
		if (gh > metrics->lineHeights[metrics->numLines-1])
			metrics->lineHeights[metrics->numLines-1] = gh;

		if (glyph->w > 0)
			metrics->numQuads++;
	}

	if (metrics->lineWidths[metrics->numLines - 1] > metrics->longestLineWidth)
		metrics->longestLineWidth = metrics->lineWidths[metrics->numLines - 1];
}

static float GetLineStartX(int align, float lineWidth)
{
	switch (align & (kTextMeshAlignCenter | kTextMeshAlignLeft | kTextMeshAlignRight))
	{
		case kTextMeshAlignLeft:
			return 0;

		case kTextMeshAlignRight:
			return -lineWidth;
		
		default:
			return -lineWidth * .5f;
	}
}

static float GetLineStartY(int align, float lineHeight)
{
	switch (align & (kTextMeshAlignMiddle | kTextMeshAlignTop | kTextMeshAlignBottom))
	{
		case kTextMeshAlignTop:
			return 0;

		case kTextMeshAlignBottom:
			return -lineHeight;
		
		default:
			return -lineHeight * .5f;
	}
}

void TextMesh_Update(const char* text, int flags, ObjNode* textNode)
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

	// Compute number of quads and line width
	TextMetrics metrics;
	ComputeMetrics(font, text, flags, &metrics);

	// Adjust y for ascender
	y += 0.5f * font->lineHeight;

	// Center vertically
	y -= 0.5f * font->lineHeight * (metrics.numLines - 1);

	// Save extents
	textNode->LeftOff	= GetLineStartX(flags, metrics.longestLineWidth);
	textNode->RightOff	= textNode->LeftOff + metrics.longestLineWidth;
	textNode->TopOff	= y - font->lineHeight;
	textNode->BottomOff	= textNode->TopOff + font->lineHeight * metrics.numLines;

	// Ensure mesh has capacity for quads
	if (textNode->TextQuadCapacity < metrics.numQuads)
	{
		textNode->TextQuadCapacity = metrics.numQuads * 2;		// avoid reallocating often if text keeps growing
		TextMesh_ReallocateMesh(mesh, textNode->TextQuadCapacity);
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

	// Create a quad for each character
	int t = 0;		// triangle counter
	int p = 0;		// point counter
	int currentLine = 0;
	x = GetLineStartX(flags, metrics.lineWidths[0]);
	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext, flags);

		if (font->isASCIIFont)
		{
			if (codepoint == '\n')
			{
				currentLine++;
				x = GetLineStartX(flags, metrics.lineWidths[currentLine]);
				y += font->lineHeight;
				continue;
			}
			else if (codepoint == '\t')
			{
				x = TAB_STOP * ceilf((x + 1.0f) / TAB_STOP);
				continue;
			}
		}

		if (flags & kTextMeshAllCaps)
		{
			codepoint = toupper(codepoint);
		}

		const AtlasGlyph* gptr = Atlas_GetGlyph(font, codepoint);
		if (!gptr)
			continue;
		const AtlasGlyph g = *gptr;

		if (g.w <= 0.0f)	// e.g. space codepoint
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
		mesh->uvs[p + 0] = (OGLTextureCoord) { g.u1, g.v1 };
		mesh->uvs[p + 1] = (OGLTextureCoord) { g.u2, g.v1 };
		mesh->uvs[p + 2] = (OGLTextureCoord) { g.u2, g.v2 };
		mesh->uvs[p + 3] = (OGLTextureCoord) { g.u1, g.v2 };

		float kernFactor = Kern(font, &g, utftext, flags);
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

void Atlas_DrawString(
	int groupNum,
	const char* text,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo)
{
	GAME_ASSERT((size_t)groupNum < (size_t)MAX_SPRITE_GROUPS);

	const Atlas* font = gAtlases[groupNum];
	GAME_ASSERT(font);

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
	glScalef(scale, scale, 1);						// Assume ortho projection

	if (rot != 0.0f)
		glRotatef(OGLMath_RadiansToDegrees(rot), 0, 0, 1);											// remember:  rotation is in degrees, not radians!


		/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(font->material, setupInfo);


			/* DRAW IT */

	glBegin(GL_QUADS);

	TextMetrics metrics;
	ComputeMetrics(font, text, flags, &metrics);

	float cx = GetLineStartX(flags, metrics.longestLineWidth);
	float cy = GetLineStartY(flags, metrics.lineHeights[0]);	// single-quad hack...

	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext, flags);
		const AtlasGlyph* gptr = Atlas_GetGlyph(font, codepoint);
		if (!gptr)
			continue;
		const AtlasGlyph g = *gptr;

		float halfw = .5f * g.w;
		float halfh = .5f * g.h;
		float qx = cx + (g.xoff + halfw);
		float qy = cy + (g.yoff + halfh);

		glTexCoord2f(g.u1, g.v2);	glVertex3f(qx - halfw, qy + halfh, 0);
		glTexCoord2f(g.u2, g.v2);	glVertex3f(qx + halfw, qy + halfh, 0);
		glTexCoord2f(g.u2, g.v1);	glVertex3f(qx + halfw, qy - halfh, 0);
		glTexCoord2f(g.u1, g.v1);	glVertex3f(qx - halfw, qy - halfh, 0);

		cx += g.xadv * Kern(font, &g, utftext, flags);

		gPolysThisFrame += 2;						// 2 tris drawn
	}
	glEnd();

		/* CLEAN UP */

	OGL_PopState();									// restore state
}

