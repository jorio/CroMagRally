/****************************/
/*   	SPRITES.C			*/
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

Atlas* gAtlases[MAX_SPRITE_GROUPS];


/****************** INIT SPRITE MANAGER ***********************/

void InitSpriteManager(void)
{
	SDL_memset(gAtlases, 0, sizeof(gAtlases));
}

/******************* DISPOSE ALL SPRITE GROUPS ****************/

void DisposeAllSpriteGroups(void)
{
	for (int i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		DisposeSpriteGroup(i);
	}
}

/******************* DISPOSE SPRITE GROUP ****************/

void DisposeSpriteGroup(int groupNum)
{
	if (gAtlases[groupNum])
	{
		Atlas_Dispose(gAtlases[groupNum]);
		gAtlases[groupNum] = NULL;
	}
}


/******************* GET SPRITE INFO ****************/

const AtlasGlyph* GetSpriteInfo(int groupNum, int spriteNum)
{
	GAME_ASSERT(gAtlases[groupNum]);
	return Atlas_GetGlyph(gAtlases[groupNum], spriteNum);
}


/********************** LOAD SPRITE FILE **************************/
//
// NOTE:  	All sprite files must be imported AFTER the draw context has been created,
//			because all imported textures are named with OpenGL and loaded into OpenGL!
//

void LoadSpriteGroup(int groupNum, const char* atlasName, int flags)
{
	if (gAtlases[groupNum])
	{
		// Sprite group busy

		if (0 == strncmp(atlasName, gAtlases[groupNum]->name, sizeof(gAtlases[groupNum]->name)))
		{
			// This atlas is already loaded
			return;
		}
		else
		{
			// Make room for new atlas
			DisposeSpriteGroup(groupNum);
		}
	}

	GAME_ASSERT_MESSAGE(!gAtlases[groupNum], "Sprite group already loaded!");
	gAtlases[groupNum] = Atlas_Load(atlasName, flags);
}



/************* MAKE NEW SRITE OBJECT *************/

ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef)
{
ObjNode				*newObj;

			/* ERROR CHECK */

//	GAME_ASSERT_MESSAGE(newObjDef->type < gNumSpritesInGroupList[newObjDef->group], "illegal type");


			/* MAKE OBJNODE */

	newObjDef->genre = SPRITE_GENRE;
	newObjDef->flags |= STATUS_BITS_FOR_2D;

	if (newObjDef->projection == 0)
		newObjDef->projection = kProjectionType2DOrthoCentered;

	newObj = MakeNewObject(newObjDef);
	if (newObj == nil)
		return(nil);

	UpdateObjectTransforms(newObj);

	return(newObj);
}

/*********************** MODIFY SPRITE OBJECT IMAGE ******************************/

void ModifySpriteObjectFrame(ObjNode *theNode, short type)
{
	GAME_ASSERT(theNode->Genre == SPRITE_GENRE);

	if (type == theNode->Type)		// see if it is the same
		return;						// (Type should have been set in ObjNode ctor via NewObjectDefinitionType.type)

	theNode->Type = type;
}

/************************** DRAW SPRITE ************************/

void DrawSprite2(
		int group,
		int spriteNo,
		float x,
		float y,
		float scaleX,
		float scaleY,
		float rot,
		uint32_t flags)
{
	char text[2] = { spriteNo, 0 };
	Atlas_DrawString2(group, text, x, y, scaleX, scaleY, rot, flags);
}
