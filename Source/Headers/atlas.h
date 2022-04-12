#pragma once

enum
{
	kTextMeshAlignCenter = 0,
	kTextMeshAlignLeft = 1,
	kTextMeshAlignRight = 2,
};

struct Atlas* Atlas_Load(const char* fontName, OGLSetupOutputType* setupInfo);
void Atlas_Dispose(struct Atlas* atlas);

struct Atlas* TextMesh_LoadDefaultFont(const char* fontName, OGLSetupOutputType* setupInfo);
void TextMesh_DisposeDefaultFont(void);

ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int align, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int align, ObjNode* textNode);
float TextMesh_GetCharX(const char* text, int n);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);
void TextMesh_DrawImmediate(const char* text, float x, float y, float scale, float rot, uint32_t flags, const OGLSetupOutputType *setupInfo);
