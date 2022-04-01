// TEXT MESH.C
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

/****************************/
/*    EXTERNALS             */
/****************************/

#include	"globals.h"
#include	"atlas.h"
#include 	"objects.h"
#include	"misc.h"
#include	"bg3d.h"
#include	"sprites.h"
#include	"3dmath.h"
#include	"file.h"
#include <string.h>
#include <stdio.h>

extern FSSpec gDataSpec;
extern	float					gCurrentAspectRatio;
extern	int						gPolysThisFrame;

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

/****************************/
/*    CONSTANTS             */
/****************************/

// This covers the basic multilingual plane (0000-FFFF)
#define MAX_CODEPOINT_PAGES 256

#define TAB_STOP 60.0f

#define MAX_KERNPAIRS		256

#define MAX_LINEBREAKS_PER_OBJNODE	16

/****************************/
/*    VARIABLES             */
/****************************/

// The font material must be reloaded everytime a new GL context is created
static MetaObjectPtr gFontMaterial = NULL;
static int gFontAtlasWidth = 0;
static int gFontAtlasHeight = 0;

static bool gFontMetricsLoaded = false;
static float gFontLineHeight = 0;

static AtlasGlyph* gAtlasGlyphsPages[MAX_CODEPOINT_PAGES];

static uint16_t gKernPairs[MAX_KERNPAIRS];
static uint8_t gKernTracking[MAX_KERNPAIRS];

#pragma mark -

/***************************************************************/
/*                         UTF-8                               */
/***************************************************************/

static AtlasGlyph* GetGlyphFromCodepoint(uint32_t c)
{
	uint32_t page = c >> 8;

	if (page >= MAX_CODEPOINT_PAGES)
	{
		page = 0; // ascii
		c = '?';
	}

	if (!gAtlasGlyphsPages[page])
	{
		page = 0; // ascii
		c = '#';
	}

	return &gAtlasGlyphsPages[page][c & 0xFF];
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
static void ParseSFL(const char* data)
{
	int nArgs = 0;
	int nGlyphs = 0;
	int junk = 0;

	ParseSFL_SkipLine(&data);	// Skip font name

	nArgs = sscanf(data, "%d %f", &junk, &gFontLineHeight);
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

		if (gAtlasGlyphsPages[codePointPage] == NULL)
		{
			gAtlasGlyphsPages[codePointPage] = AllocPtrClear(sizeof(AtlasGlyph) * 256);
		}

		gAtlasGlyphsPages[codePointPage][codePoint & 0xFF] = newGlyph;

		ParseSFL_SkipLine(&data);  // Skip rest of line
	}

	// Force monospaced numbers
	AtlasGlyph* asciiPage = gAtlasGlyphsPages[0];
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

static void ParseKerningFile(const char* data)
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

		AtlasGlyph* g = GetGlyphFromCodepoint(codepoint1);

		if (g->numKernPairs == 0)
		{
			GAME_ASSERT(g->kernTableOffset == 0);
			g->kernTableOffset = kernTableOffset;
		}

		GAME_ASSERT_MESSAGE(g->numKernPairs == kernTableOffset - g->kernTableOffset, "kern pair blocks aren't contiguous!");

		gKernPairs[kernTableOffset] = codepoint2;
		gKernTracking[kernTableOffset] = tracking;
		kernTableOffset++;
		GAME_ASSERT(kernTableOffset <= MAX_KERNPAIRS);
		g->numKernPairs++;

		SkipWhitespace(&data);
	}
}

/***************************************************************/
/*                       INIT/SHUTDOWN                         */
/***************************************************************/

void TextMesh_LoadFont(OGLSetupOutputType* setupInfo, const char* fontName)
{
	char pathBuf[256];

	snprintf(pathBuf, sizeof(pathBuf), ":fonts:%s.sfl", fontName);
	TextMesh_LoadMetrics(pathBuf);

	snprintf(pathBuf, sizeof(pathBuf), ":fonts:%s.png", fontName);
	TextMesh_InitMaterial(setupInfo, pathBuf);
}

void TextMesh_DisposeFont(void)
{
	TextMesh_DisposeMaterial();
	TextMesh_DisposeMetrics();
}

void TextMesh_LoadMetrics(const char* sflPath)
{
	GAME_ASSERT_MESSAGE(!gFontMetricsLoaded, "Metrics already loaded");

	memset(gAtlasGlyphsPages, 0, sizeof(gAtlasGlyphsPages));

	// Parse metrics (gAtlasGlyphs) from SFL file
	char* data = LoadTextFile(sflPath, NULL);
	GAME_ASSERT(data);
	ParseSFL(data);
	SafeDisposePtr(data);

	// Parse kerning table
	data = LoadTextFile(":fonts:kerning.txt", NULL);
	GAME_ASSERT(data);
	ParseKerningFile(data);
	SafeDisposePtr(data);

	gFontMetricsLoaded = true;
}

void TextMesh_InitMaterial(OGLSetupOutputType* setupInfo, const char* texturePath)
{
	GLuint textureName = 0;
	textureName = OGL_TextureMap_LoadImageFile(texturePath, &gFontAtlasWidth, &gFontAtlasHeight);

		/* CREATE FONT MATERIAL */

	GAME_ASSERT_MESSAGE(!gFontMaterial, "gFontMaterial already created");
	MOMaterialData matData;
	memset(&matData, 0, sizeof(matData));
	matData.setupInfo		= setupInfo;
	matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND | BG3D_MATERIALFLAG_TEXTURED | BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V;
	matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
	matData.numMipmaps		= 1;
	matData.width			= gFontAtlasWidth;
	matData.height			= gFontAtlasHeight;
	matData.textureName[0]	= textureName;
	gFontMaterial = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
}

void TextMesh_DisposeMaterial(void)
{
	MO_DisposeObjectReference(gFontMaterial);
	gFontMaterial = NULL;
}

void TextMesh_DisposeMetrics(void)
{
	if (!gFontMetricsLoaded)
		return;

	gFontMetricsLoaded = false;

	for (int i = 0; i < MAX_CODEPOINT_PAGES; i++)
	{
		if (gAtlasGlyphsPages[i])
		{
			SafeDisposePtr((Ptr) gAtlasGlyphsPages[i]);
			gAtlasGlyphsPages[i] = NULL;
		}
	}
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
	
	GAME_ASSERT(gFontMaterial);
	mesh->numMaterials = 1;
	mesh->materials[0] = gFontMaterial;

	TextMesh_ReallocateMesh(mesh, numQuads);
}

static float Kern(const AtlasGlyph* glyph, const char* utftext)
{
	if (!glyph || !glyph->numKernPairs)
		return 1;

	uint32_t buddy = ReadNextCodepointFromUTF8(&utftext);

	for (int i = glyph->kernTableOffset; i < glyph->kernTableOffset + glyph->numKernPairs; i++)
	{
		if (gKernPairs[i] == buddy)
			return gKernTracking[i] * .01f;
	}

	return 1;
}

static void ComputeCounts(const char* text, int* numQuadsOut, int* numLinesOut, float* lineWidths, int maxLines)
{
	const float S = 2.0f;//.5f;
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

		const AtlasGlyph* glyph = GetGlyphFromCodepoint(c);
		float kernFactor = Kern(glyph, utftext);
		lineWidths[numLines-1] += S * (glyph->xadv * kernFactor + spacing);
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
	GAME_ASSERT_MESSAGE(gFontMaterial, "Can't lay out text if the font material isn't loaded!");
	GAME_ASSERT_MESSAGE(gFontMetricsLoaded, "Can't lay out text if the font metrics aren't loaded!");

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

	const float S = 2.0f;//.5f;
	float x = 0;
	float y = 0;
	float z = 0;
	float spacing = 0;

	const float invAtlasW = 1.0f / gFontAtlasWidth;
	const float invAtlasH = 1.0f / gFontAtlasHeight;

	// Compute number of quads and line width
	float lineWidths[MAX_LINEBREAKS_PER_OBJNODE];
	int numLines = 0;
	int numQuads = 0;
	ComputeCounts(text, &numQuads, &numLines, lineWidths, MAX_LINEBREAKS_PER_OBJNODE);

	// Compute longest line width
	float longestLineWidth = 0;
	for (int i = 0; i < numLines; i++)
	{
		if (lineWidths[i] > longestLineWidth)
			longestLineWidth = lineWidths[i];
	}

	// Adjust y for ascender
	y -= 0.5f * S * gFontLineHeight;

	// Center vertically
	y += 0.5f * S * gFontLineHeight * (numLines - 1);

	// Save extents
	textNode->LeftOff	= GetLineStartX(align, longestLineWidth);
	textNode->RightOff	= textNode->LeftOff + longestLineWidth;
	textNode->TopOff	= y + S * gFontLineHeight;// * 0.5f;
	textNode->BottomOff	= textNode->TopOff - S * gFontLineHeight * numLines;

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
			y -= gFontLineHeight * S;
			continue;
		}

		if (codepoint == '\t')
		{
			x = TAB_STOP * ceilf((x+1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph g = *GetGlyphFromCodepoint(codepoint);

		if (codepoint == ' ')
		{
			x += S*(g.xadv + spacing);
			continue;
		}

		float qx = x + S*(g.xoff + g.w*.5f);
		float qy = y + S*(g.yoff + g.h*.5f);

		mesh->triangles[t + 0].vertexIndices[0] = p + 0;
		mesh->triangles[t + 0].vertexIndices[1] = p + 1;
		mesh->triangles[t + 0].vertexIndices[2] = p + 2;
		mesh->triangles[t + 1].vertexIndices[0] = p + 0;
		mesh->triangles[t + 1].vertexIndices[1] = p + 2;
		mesh->triangles[t + 1].vertexIndices[2] = p + 3;
		mesh->points[p + 0] = (OGLPoint3D) { qx - S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh->points[p + 1] = (OGLPoint3D) { qx + S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh->points[p + 2] = (OGLPoint3D) { qx + S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh->points[p + 3] = (OGLPoint3D) { qx - S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh->uvs[p + 0] = (OGLTextureCoord) { invAtlasW * g.x,			invAtlasH * (g.y+g.h) };
		mesh->uvs[p + 1] = (OGLTextureCoord) { invAtlasW * (g.x+g.w),	invAtlasH * (g.y+g.h) };
		mesh->uvs[p + 2] = (OGLTextureCoord) { invAtlasW * (g.x+g.w),	invAtlasH * g.y };
		mesh->uvs[p + 3] = (OGLTextureCoord) { invAtlasW * g.x,			invAtlasH * g.y };

		float kernFactor = Kern(&g, utftext);
		x += S*(g.xadv*kernFactor + spacing);
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
	newObjDef->flags = STATUS_BIT_NOFOG | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_NOZBUFFER
		| STATUS_BIT_DONTCULL;   // TODO: make it so we don't need DONTCULL
	ObjNode* textNode = MakeNewObject(newObjDef);

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

void TextMesh_DrawImmediate(const char* text, float x, float y, float scale, float rot, uint32_t flags, const OGLSetupOutputType *setupInfo)
{
			/* SET STATE */

	OGL_PushState();								// keep state
	glMatrixMode(GL_PROJECTION);					// init projection matrix
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();								// init MODELVIEW matrix

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

	MO_DrawMaterial(gFontMaterial, setupInfo);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);						// set clamp mode after each texture set since OGL just likes it that way
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


			/* DRAW IT */

	const float invAtlasW = 1.0f / gFontAtlasWidth;
	const float invAtlasH = 1.0f / gFontAtlasHeight;

	glBegin(GL_QUADS);
	float cx = -32;  // hack to make text origin fit where CMR infobar expects it
	float cy = -32;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t codepoint = ReadNextCodepointFromUTF8(&utftext);
		const AtlasGlyph* glyph = GetGlyphFromCodepoint(codepoint);

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

		cx += glyph->xadv * Kern(glyph, utftext);

		gPolysThisFrame += 2;						// 2 tris drawn
	}
	glEnd();

		/* CLEAN UP */

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);						// set this back to normal
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	OGL_PopState();									// restore state
}
