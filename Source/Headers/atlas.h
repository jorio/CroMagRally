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
};

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
	MOMaterialObject* material;
	int textureWidth;
	int textureHeight;
	float lineHeight;
	AtlasGlyph* glyphPages[MAX_CODEPOINT_PAGES];

	uint16_t kernPairs[MAX_KERNPAIRS];
	uint8_t kernTracking[MAX_KERNPAIRS];
} Atlas;

void Atlas_LoadSlot(int slot, const char* atlasName, OGLSetupOutputType* setupInfo);
void Atlas_DisposeSlot(int slot);
void Atlas_DisposeAllSlots(void);

struct Atlas* Atlas_Load(const char* fontName, OGLSetupOutputType* setupInfo);
void Atlas_Dispose(struct Atlas* atlas);

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

void Atlas_DrawQuad(
	int slot,
	int spriteNo,
	float x,
	float y,
	float scale,
	float rot,
	uint32_t flags,
	const OGLSetupOutputType *setupInfo);
