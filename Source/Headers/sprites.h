//
// sprites.h
//

#include "atlas.h"

void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void LoadSpriteGroup(int groupNum, const char* atlasName, int flags);
const AtlasGlyph* GetSpriteInfo(int groupNum, int spriteNum);

void DrawSprite2(
		int groupNum,
		int spriteNum,
		float x,
		float y,
		float scaleX,
		float scaleY,
		float rot,
		uint32_t flags);

#define DrawSprite(group, sprite, x, y, scale, flags) \
	DrawSprite2(group, sprite, x, y, scale, scale, 0, flags)
