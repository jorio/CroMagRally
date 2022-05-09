//
// sprites.h
//

void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, uint32_t flags);
