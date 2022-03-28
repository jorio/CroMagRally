#pragma once

enum
{
	kTextMeshAlignCenter = 0,
	kTextMeshAlignLeft = 1,
	kTextMeshAlignRight = 2,
};

void TextMesh_LoadFont(OGLSetupOutputType* setupInfo, const char* fontName);
void TextMesh_DisposeFont(void);
void TextMesh_LoadMetrics(const char* sflPath);
void TextMesh_DisposeMetrics(void);
void TextMesh_InitMaterial(OGLSetupOutputType* setupInfo, const char* pngPath);
void TextMesh_DisposeMaterial(void);
ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int align, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int align, ObjNode* textNode);
float TextMesh_GetCharX(const char* text, int n);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);
void TextMesh_DrawImmediate(const char* text, float x, float y, float scale, float rot, uint32_t flags, const OGLSetupOutputType *setupInfo);
