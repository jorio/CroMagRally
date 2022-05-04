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
#include <string.h>


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
	memset(gAtlases, 0, sizeof(gAtlases));
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

void LoadSpriteGroup(int groupNum, const char* atlasName, int flags, OGLSetupOutputType* setupInfo)
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
	gAtlases[groupNum] = Atlas_Load(atlasName, flags, setupInfo);
}



/************* MAKE NEW SRITE OBJECT *************/

ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo)
{
ObjNode				*newObj;
MOSpriteObject		*spriteMO;
MOSpriteSetupData	spriteData;

			/* ERROR CHECK */

//	GAME_ASSERT_MESSAGE(newObjDef->type < gNumSpritesInGroupList[newObjDef->group], "illegal type");


			/* MAKE OBJNODE */

	newObjDef->genre = SPRITE_GENRE;
	newObjDef->flags |= STATUS_BITS_FOR_2D;

	newObj = MakeNewObject(newObjDef);
	if (newObj == nil)
		return(nil);

	newObj->Projection = kProjectionType2DOrthoCentered;

			/* MAKE SPRITE META-OBJECT */

	spriteData.group	= newObjDef->group;
	spriteData.type		= newObjDef->type;

	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, (uintptr_t) setupInfo, &spriteData);
	GAME_ASSERT(spriteMO);


			/* SET SPRITE MO INFO */

	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = newObj->Scale.x;
	spriteMO->objectData.coord = newObj->Coord;


			/* ATTACH META OBJECT TO OBJNODE */

	newObj->SpriteMO = spriteMO;

	return(newObj);
}

/*********************** MODIFY SPRITE OBJECT IMAGE ******************************/

void ModifySpriteObjectFrame(ObjNode *theNode, short type, OGLSetupOutputType *setupInfo)
{
MOSpriteSetupData	spriteData;
MOSpriteObject		*spriteMO;


	if (type == theNode->Type)										// see if it is the same
		return;

		/* DISPOSE OF OLD TYPE */

	MO_DisposeObjectReference(theNode->SpriteMO);


		/* MAKE NEW SPRITE MO */

	spriteData.group	= theNode->Group;							// set group
	spriteData.type 	= type;										// set group subtype

	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, (uintptr_t) setupInfo, &spriteData);
	GAME_ASSERT(spriteMO);


			/* SET SPRITE MO INFO */

	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = theNode->Scale.x;
	spriteMO->objectData.coord = theNode->Coord;


			/* ATTACH META OBJECT TO OBJNODE */

	theNode->SpriteMO = spriteMO;
	theNode->Type = type;
}

/************************** DRAW SPRITE ************************/

void DrawSprite(int group, int spriteNo, float x, float y, float scale, float rot, uint32_t flags, const OGLSetupOutputType *setupInfo)
{
	char text[2] = { spriteNo, 0 };
	Atlas_DrawString(group, text, x, y, scale, rot, flags, setupInfo);
}
