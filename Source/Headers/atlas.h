#pragma once

// This covers the basic multilingual plane (0000-FFFF)
#define MAX_CODEPOINT_PAGES 256
#define MAX_KERNPAIRS		256

enum
{
	kTextMeshAlignCenter				= 0,		// default if horizontal alignment not set
	kTextMeshAlignLeft					= 1<<1,
	kTextMeshAlignRight					= 1<<2,
	kTextMeshAlignMiddle				= 0,		// default if vertical alignment not set
	kTextMeshAlignTop					= 1<<3,
	kTextMeshAlignBottom				= 1<<4,
	kTextMeshGlow						= 1<<6,
	kTextMeshProjectionOrthoFullRect	= 1<<7,
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
	float yadv;

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

	bool isASCIIFont;
} Atlas;

Atlas* Atlas_Load(const char* atlasName, int flags, OGLSetupOutputType* setupInfo);
void Atlas_Dispose(Atlas* atlas);

const AtlasGlyph* Atlas_GetGlyph(const Atlas* atlas, uint32_t codepoint);

void LoadSpriteGroup(int groupNum, const char* atlasName, int flags, OGLSetupOutputType* setupInfo);
void DisposeSpriteGroup(int groupNum);
void DisposeAllSpriteGroups(void);
const AtlasGlyph* GetSpriteInfo(int groupNum, int spriteNum);

ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int align, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int align, ObjNode* textNode);
float TextMesh_GetCharX(const char* text, int n);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);

void Atlas_DrawString(
	int groupNum,
	const char* text,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo);

void DrawSprite(
	int groupNum,
	int spriteNum,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo);
