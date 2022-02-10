//
// sprites.h
//



enum
{
	SPRITE_FLAG_GLOW = (1)
};

#define	FONT_WIDTH	.15f

typedef struct
{
	int32_t 		width;
	int32_t 		height;
	float 			aspectRatio;  // h/w
	int32_t 		srcFormat;
	int32_t 		destFormat;
	int32_t 		bufferSize;
}File_SpriteType;


typedef struct
{
	int32_t			width,height;
	float			aspectRatio;			// h/w
	GLint			srcFormat, destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteFile(FSSpec *spec, int groupNum, OGLSetupOutputType *setupInfo);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type, OGLSetupOutputType *setupInfo);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags, const OGLSetupOutputType *setupInfo);
void BlendASprite(int group, int type);

ObjNode *MakeFontStringObject(const char* s, NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo, Boolean center);
