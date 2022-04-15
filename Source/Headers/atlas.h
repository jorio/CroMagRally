#pragma once

// This covers the basic multilingual plane (0000-FFFF)
#define MAX_CODEPOINT_PAGES 256
#define MAX_KERNPAIRS		256

enum
{
	kTextMeshAlignCenter = 0,
	kTextMeshAlignLeft = 1,
	kTextMeshAlignRight = 2,
	kTextMeshNoSpecialASCII = 4,
	kTextMeshGlow = 8,
};

enum
{
	kAtlasLoadAsSingleSprite	= 1,
	kAtlasLoadFont				= 2,
};

typedef struct
{
	float w;
	float h;
	float xoff;
	float yoff;
	float xadv;

	float u1;
	float v1;
	float u2;
	float v2;

	uint16_t	kernTableOffset;
	int8_t		numKernPairs;
} AtlasGlyph;

typedef struct Atlas
{
	MOMaterialObject* material;
	int textureWidth;
	int textureHeight;
	float lineHeight;
	AtlasGlyph* glyphPages[MAX_CODEPOINT_PAGES];

	uint16_t kernPairs[MAX_KERNPAIRS];
	uint8_t kernTracking[MAX_KERNPAIRS];
} Atlas;

Atlas* Atlas_Load(const char* atlasName, int flags, OGLSetupOutputType* setupInfo);
void Atlas_Dispose(Atlas* atlas);

void LoadSpriteGroup(int groupNum, const char* atlasName, int flags, OGLSetupOutputType* setupInfo);
void DisposeSpriteGroup(int groupNum);
void DisposeAllSpriteGroups(void);

ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int align, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int align, ObjNode* textNode);
float TextMesh_GetCharX(const char* text, int n);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);

void Atlas_DrawString(
	int slot,
	const char* text,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo);

void DrawSprite(
	int slot,
	int spriteNo,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo);
