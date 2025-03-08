/****************************/
/*   METAOBJECTS.C		    */
/* (c)2000 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "stb_image.h"

extern	Boolean			gMyState_Lighting;

/****************************/
/*    PROTOTYPES            */
/****************************/

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type, uintptr_t subType);
static void SetMetaObjectToGroup(MOGroupObject *groupObj);
static void SetMetaObjectToGeometry(MetaObjectPtr mo, uintptr_t subType, void *data);
static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData);
static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data);
static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData);
static void MO_DetachFromLinkedList(MetaObjectPtr obj);
static void MO_DisposeObject_Group(MOGroupObject *group);
static void MO_DisposeObject_Material(MOMaterialObject *obj);
static void MO_GetNewGroupReference(MOGroupObject *obj);
static void MO_GetNewVertexArrayReference(MOVertexArrayObject *obj);
static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox);
static void SetMetaObjectToPicture(MOPictureObject *pictObj, const char *inData, int destPixelFormat);
static void MO_DisposeObject_Picture(MOPictureObject *obj);


/****************************/
/*    CONSTANTS             */
/****************************/





/*********************/
/*    VARIABLES      */
/*********************/

MetaObjectHeader	*gFirstMetaObject = nil;
MetaObjectHeader 	*gLastMetaObject = nil;
int					gNumMetaObjects = 0;

float				gGlobalTransparency = 1;			// 0 == clear, 1 = opaque
OGLColorRGB			gGlobalColorFilter = {1,1,1};
uint32_t				gGlobalMaterialFlags = 0;

MOMaterialObject	*gMostRecentMaterial;

/***************** INIT META OBJECT HANDLER ******************/

void MO_InitHandler(void)
{
	gFirstMetaObject = nil;				// no meta object nodes yet
	gLastMetaObject = nil;
	gNumMetaObjects = 0;
}


#pragma mark -

/******************** GET NEW REFERENCE *********************/

MetaObjectPtr MO_GetNewReference(MetaObjectPtr mo)
{
MetaObjectHeader *h = mo;

	h->refCount++;
	GAME_ASSERT(h->refCount < 1000000);


	return(mo);
}


/**************** MO: GET NEW GROUP REFERENCE ******************/
//
// Recursively increment ref counts of everything inside the group
//

static void MO_GetNewGroupReference(MOGroupObject *obj)
{
MOGroupData	*data = &obj->objectData;
int			i;

	for (i = 0; i < data->numObjectsInGroup; i++)
	{
		MO_GetNewReference(data->groupContents[i]);
	}
}


/***************** MO: GET NEW VERTEX ARRAY REFERENCE *************/
//
// Recursively increment ref counts of all texture objects in geometry
//

static void MO_GetNewVertexArrayReference(MOVertexArrayObject *obj)
{
MOVertexArrayData	*data = &obj->objectData;
int					i;

	for (i = 0; i < data->numMaterials; i++)
	{
		MO_GetNewReference(data->materials[i]);
	}
}

#pragma mark -

/****************** MO: CREATE NEW OBJECT OF TYPE ****************/
//
// INPUT:	type = type of mo to create
//			subType = subtype to create (optional)
//			data = pointer to any data needed to create the mo (optional)
//


MetaObjectPtr	MO_CreateNewObjectOfType(uint32_t type, uintptr_t subType, void *data)
{
MetaObjectPtr	mo;

			/* ALLOCATE EMPTY OBJECT */

	mo = AllocateEmptyMetaObject(type,subType);
	if (mo == nil)
		return(nil);


			/* SET OBJECT INFO */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				SetMetaObjectToGroup(mo);
				break;

		case	MO_TYPE_GEOMETRY:
				SetMetaObjectToGeometry(mo, subType, data);
				break;

		case	MO_TYPE_MATERIAL:
				SetMetaObjectToMaterial(mo, data);
				break;

		case	MO_TYPE_MATRIX:
				SetMetaObjectToMatrix(mo, data);
				break;

		case	MO_TYPE_PICTURE:
		{
				// In the PPC Mac original, background pictures don't appear to be gamma-corrected.
				// This way, wallfont pops against the main menu's background.
				int oldTextureFlags = gLoadTextureFlags;
				gLoadTextureFlags |= kLoadTextureFlags_NoGammaFix;

				SetMetaObjectToPicture(mo, data, GL_RGBA);

				gLoadTextureFlags = oldTextureFlags;
				break;
		}

		default:
				DoFatalAlert("MO_CreateNewObjectOfType: object type not recognized");
	}

	return(mo);
}


/****************** ALLOCATE EMPTY META OBJECT *****************/
//
// allocates an empty meta object and connects it to the linked list
//

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type, uintptr_t subType)
{
MetaObjectHeader	*mo;
int					size;

			/* DETERMINE SIZE OF DATA TO ALLOC */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				size = sizeof(MOGroupObject);
				break;

		case	MO_TYPE_GEOMETRY:
				switch(subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							size = sizeof(MOVertexArrayObject);
							break;

					default:
							DoFatalAlert("AllocateEmptyMetaObject: object subtype not recognized");
				}
				break;

		case	MO_TYPE_MATERIAL:
				size = sizeof(MOMaterialObject);
				break;

		case	MO_TYPE_MATRIX:
				size = sizeof(MOMatrixObject);
				break;

		case	MO_TYPE_PICTURE:
				size = sizeof(MOPictureObject);
				break;

		default:
				DoFatalAlert("AllocateEmptyMetaObject: object type not recognized");
	}


			/* ALLOC MEMORY FOR META OBJECT */

	mo = AllocPtrClear(size); //AllocPtr(size);
	if (mo == nil)
		DoFatalAlert("AllocateEmptyMetaObject: AllocPtr failed!");


			/* INIT STRUCTURE */

	mo->cookie 		= MO_COOKIE;
	mo->type 		= type;
	mo->subType 	= subType;
	mo->data 		= nil;
	mo->nextNode 	= nil;
	mo->refCount	= 1;							// initial reference count is always 1
	mo->parentGroup = nil;


		/***************************/
		/* ADD NODE TO LINKED LIST */
		/***************************/

		/* SEE IF IS ONLY NODE */

	if (gFirstMetaObject == nil)
	{
		if (gLastMetaObject)
			DoFatalAlert("AllocateEmptyMetaObject: gFirstMetaObject & gLastMetaObject should be nil");

		mo->prevNode = nil;
		gFirstMetaObject = gLastMetaObject = mo;
		gNumMetaObjects = 1;
	}

		/* ADD TO END OF LINKED LIST */

	else
	{
		mo->prevNode = gLastMetaObject;		// point new prev to last
		gLastMetaObject->nextNode = mo;		// point old last to new
		gLastMetaObject = mo;				// set new last
		gNumMetaObjects++;
	}

	return(mo);
}

/******************* SET META OBJECT TO GROUP ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGroup(MOGroupObject *groupObj)
{

			/* INIT THE DATA */

	groupObj->objectData.numObjectsInGroup = 0;
}


/***************** SET META OBJECT TO GEOMETRY ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGeometry(MetaObjectPtr mo, uintptr_t subType, void *data)
{
	switch(subType)
	{
		case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
				SetMetaObjectToVertexArrayGeometry(mo, data);
				break;

		default:
				DoFatalAlert("SetMetaObjectToGeometry: unknown subType");

	}
}


/*************** SET META OBJECT TO VERTEX ARRAY GEOMETRY ***************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.  It also boosts the ref count of
// any referenced items.
//

static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data)
{
int	i;

			/* INIT THE DATA */

	geoObj->objectData = *data;									// copy from input data


		/* INCREASE MATERIAL REFERENCE COUNTS */

	for (i = 0; i < data->numMaterials; i++)
	{
		if (data->materials[i] != nil)				// make sure this material ref is valid
			MO_GetNewReference(data->materials[i]);
	}

}


/******************* SET META OBJECT TO MATERIAL ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData)
{

		/* COPY INPUT DATA */

	matObj->objectData = *inData;
}


/******************* SET META OBJECT TO MATRIX ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData)
{

		/* COPY INPUT DATA */

	matObj->matrix = *inData;
}


/******************* SET META OBJECT TO PICTURE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToPicture(MOPictureObject *pictObj, const char *inData, int destPixelFormat)
{
int			width,height;
MOPictureData	*picData = &pictObj->objectData;
MOMaterialData	matData;

			/* LOAD PICTURE FILE */

	GLuint textureName = OGL_TextureMap_LoadImageFile(inData, &width, &height);

			/***************************/
			/* CREATE A TEXTURE OBJECT */
			/***************************/

//	matData.setupInfo		= setupInfo;
	matData.flags			= BG3D_MATERIALFLAG_TEXTURED|BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
	matData.diffuseColor	= (OGLColorRGBA) {1,1,1,1};
	matData.numMipmaps		= 1;
	matData.width			= width;
	matData.height			= height;
	matData.pixelSrcFormat	= GL_RGBA;
	matData.pixelDstFormat 	= destPixelFormat;
	matData.texturePixels[0]= nil;						// we're going to preload
	matData.textureName[0]	= textureName;

	picData->material = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
}



#pragma mark -


/********************** MO: APPEND TO GROUP ****************/
//
// Attach new object to end of group
//

void MO_AppendToGroup(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("MO_AppendToGroup: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("MO_AppendToGroup: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object
	group->objectData.groupContents[i] = newObject;	// save object reference in group
}

/**************** MO: ATTACH TO GROUP START **************/
//
// Attach new object to START of group
//

void MO_AttachToGroupStart(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i,j;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("MO_AttachToGroupStart: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("MO_AttachToGroupStart: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object


			/* SHIFT ALL EXISTING OBJECTS UP */

	for (j = i; j > 0; j--)
		group->objectData.groupContents[j] = group->objectData.groupContents[j-1];

	group->objectData.groupContents[0] = newObject;	// save object ref into group
}


#pragma mark -


/******************** MO: DRAW OBJECT ***********************/
//
// This recursive function will draw any objects submitted and parses groups.
//

void MO_DrawObject(const MetaObjectPtr object)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_DrawObject: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							MO_DrawGeometry_VertexArray(&vObj->objectData);
							break;

					default:
						DoFatalAlert("MO_DrawObject: unknown sub-type!");
				}
				break;

		case	MO_TYPE_MATERIAL:
				MO_DrawMaterial(object);
				break;

		case	MO_TYPE_GROUP:
				MO_DrawGroup(object);
				break;

		case	MO_TYPE_MATRIX:
				MO_DrawMatrix(object);
				break;

		case	MO_TYPE_PICTURE:
				MO_DrawPicture(object);
				break;

		default:
			DoFatalAlert("MO_DrawObject: unknown type!");
	}
}


/******************** MO_DRAW GROUP *************************/

void MO_DrawGroup(const MOGroupObject *object)
{
int	numChildren,i;

				/* VERIFY OBJECT TYPE */

	if (object->objectHeader.type != MO_TYPE_GROUP)
		DoFatalAlert("MO_DrawGroup: this isnt a group!");


			/*************************************/
			/* PUSH CURRENT STATE ON STATE STACK */
			/*************************************/


	OGL_PushState();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group

	for (i = 0; i < numChildren; i++)
	{
		MO_DrawObject(object->objectData.groupContents[i]);
	}


			/******************************/
			/* POP OLD STATE OFF OF STACK */
			/******************************/

	OGL_PopState();
}


/******************** MO: DRAW GEOMETRY - VERTEX ARRAY *************************/

void MO_DrawGeometry_VertexArray(const MOVertexArrayData *data)
{
Boolean		useTexture = false;

			/**********************/
			/* SETUP VERTEX ARRAY */
			/**********************/

	glEnableClientState(GL_VERTEX_ARRAY);				// enable vertex arrays
	glVertexPointer(3, GL_FLOAT, 0, data->points);		// point to point array

	if (OGL_CheckError())
		DoFatalAlert("MO_DrawGeometry_VertexArray: glVertexPointer!");

			/************************/
			/* SETUP VERTEX NORMALS */
			/************************/

	if (data->normals && gMyState_Lighting)				// do we have normals & lighting
	{
		glNormalPointer(GL_FLOAT, 0, data->normals);
		glEnableClientState(GL_NORMAL_ARRAY);			// enable normal arrays
	}
	else
		glDisableClientState(GL_NORMAL_ARRAY);			// disable normal arrays

	if (OGL_CheckError())
		DoFatalAlert("MO_DrawGeometry_VertexArray: normals!");

			/****************************/
			/* SEE IF ACTIVATE MATERIAL */
			/****************************/
			//
			// For now, I'm just looking at material #0.
			//

	if (data->numMaterials < 0)							// if (-), then assume texture has been manually set
		goto use_current;

	if (data->numMaterials > 0)									// are there any materials?
	{
		MO_DrawMaterial(data->materials[0]);					// submit material #0

			/* IF TEXTURED, THEN ALSO ACTIVATE UV ARRAY */

		if (data->materials[0]->objectData.flags & BG3D_MATERIALFLAG_TEXTURED)
		{
use_current:
			if (data->uvs)
			{
				if (OGL_CheckError())
					DoFatalAlert("MO_DrawGeometry_VertexArray: preeeeee uv!");

				glTexCoordPointer(2, GL_FLOAT, 0,data->uvs);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);	// enable uv arrays
				useTexture = true;

				if (OGL_CheckError())
					DoFatalAlert("MO_DrawGeometry_VertexArray: uv!");
			}
		}
	}
	else
		glDisable(GL_TEXTURE_2D);						// no materials, thus no texture, thus turn this off


		/* WE DONT HAVE ENOUGH INFO TO DO TEXTURES, SO DISABLE */

	if (!useTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (OGL_CheckError())
			DoFatalAlert("MO_DrawGeometry_VertexArray: glDisableClientState(GL_TEXTURE_COORD_ARRAY)!");
	}


			/***********************/
			/* SETUP VERTEX COLORS */
			/***********************/

			/* IF LIGHTING, THEN USE COLOR FLOATS */

	if (gMyState_Lighting)
	{
		if (data->colorsFloat)									// do we have colors?
		{
			glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// disable color arrays
	}

			/* IF NOT LIGHTING, THEN USE COLOR BYTES */

	else
	{
		if (data->colorsByte)									// do we have colors?
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, data->colorsByte);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// disable color arrays
	}

	if (OGL_CheckError())
		DoFatalAlert("MO_DrawGeometry_VertexArray: color!");


			/***********/
			/* DRAW IT */
			/***********/


//	glLockArraysEXT(0, data->numPoints);
	glDrawElements(GL_TRIANGLES,data->numTriangles*3,GL_UNSIGNED_INT,&data->triangles[0]);
	if (OGL_CheckError())
		DoFatalAlert("MO_DrawGeometry_VertexArray: glDrawElements");
//	glUnlockArraysEXT();

	gPolysThisFrame += data->numPoints;					// inc poly counter
}


/************************ DRAW MATERIAL **************************/

void MO_DrawMaterial(MOMaterialObject *matObj)
{
MOMaterialData		*matData;
OGLColorRGBA		*diffuseColor,diffColor2;
Boolean				textureHasAlpha = false;
Boolean				alreadySet;
uint32_t				matFlags;

			/* SEE IF THIS MATERIAL IS ALREADY SET AS CURRENT */

	alreadySet = (matObj == gMostRecentMaterial);


	matData = &matObj->objectData;									// point to material data

	if (matObj->objectHeader.cookie != MO_COOKIE)					// verify cookie
		DoFatalAlert("MO_DrawMaterial: bad cookie!");


				/****************************/
				/* SEE IF TEXTURED MATERIAL */
				/****************************/

	matFlags = matData->flags | gGlobalMaterialFlags;				// check flags of material & global

	if (matData->flags & BG3D_MATERIALFLAG_TEXTURED)
	{
//		if (matData->setupInfo != setupInfo)						// make sure texture is loaded for this draw context
//			DoFatalAlert("MO_DrawMaterial: texture is not assigned to this draw context");


		if (matData->pixelDstFormat == GL_RGBA)						// if 32bit with alpha, then turn blending on (see below)
			textureHasAlpha = true;

					/* SET APPROPRIATE ALPHA MODE */

		if (alreadySet)												// see if even need to bother resetting this
		{
			glEnable(GL_TEXTURE_2D);								// just make sure textures are on
		}
		else
		{
			OGL_Texture_SetOpenGLTexture(matData->textureName[0]);	// set this texture active



			if (gDebugMode)
			{
				int	size;
				size = matData->width * matData->height;
				switch(matData->pixelDstFormat)
				{
					case	GL_RGBA:
					case	GL_RGB:
							gVRAMUsedThisFrame += size * 4;
							break;

					case	GL_RGB5_A1:
							gVRAMUsedThisFrame += size * 2;
							break;
				}

			}
		}


					/* SET TEXTURE WRAPPING MODE */

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_U)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_V)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
		glDisable(GL_TEXTURE_2D);									// not textured, so disable textures




			/********************/
			/* COLORED MATERIAL */
			/********************/

	diffuseColor = &matData->diffuseColor;			// point to diffuse color

	if (gGlobalTransparency != 1.0f)				// see if need to factor in global transparency
	{
		diffColor2.r = diffuseColor->r;
		diffColor2.g = diffuseColor->g;
		diffColor2.b = diffuseColor->b;
		diffColor2.a = diffuseColor->a * gGlobalTransparency;
	}
	else
		diffColor2 = *diffuseColor;					// copy to local so we can apply filter to it without munging original


			/* APPLY COLOR FILTER */

	diffColor2.r *= gGlobalColorFilter.r;
	diffColor2.g *= gGlobalColorFilter.g;
	diffColor2.b *= gGlobalColorFilter.b;

	OGL_FixColorGamma(&diffColor2);

	glColor4fv(&diffColor2.r);						// set current diffuse color


		/* SEE IF NEED TO ENABLE BLENDING */


	if (textureHasAlpha || (diffColor2.a != 1.0f) || (matFlags & BG3D_MATERIALFLAG_ALWAYSBLEND))		// if has alpha, then we need blending on
	    glEnable(GL_BLEND);
	else
	    glDisable(GL_BLEND);


			/* SAVE THIS STUFF */

	gMostRecentMaterial = matObj;
}


/************************ DRAW MATRIX **************************/

void MO_DrawMatrix(const MOMatrixObject *matObj)
{
const OGLMatrix4x4		*m;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */

	glMultMatrixf((GLfloat *)m);

	if (OGL_CheckError())
		DoFatalAlert("MO_DrawMatrix: glMultMatrixf!");

}

/************************ MO: DRAW PICTURE **************************/

void MO_DrawPicture(const MOPictureObject *picObj)
{
const MOPictureData	*picData = &picObj->objectData;

			/* INIT MATRICES */

	OGL_PushState();

	// Projection should be kProjectionType2DNDC
	// 2D state should be set by STATUS_BITS_FOR_2D

	float z = 0;

			/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(picData->material);							// submit material

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);	glVertex3f(-1, -1, z);
	glTexCoord2f(1, 1);	glVertex3f( 1, -1, z);
	glTexCoord2f(1, 0);	glVertex3f( 1,  1, z);
	glTexCoord2f(0, 0);	glVertex3f(-1,  1, z);
	glEnd();

	gPolysThisFrame += 2;										// 2 more triangles

			/* RESTORE STATE */

	OGL_PopState();
}



#pragma mark -

/********************** MO DISPOSE OBJECT REFERENCE ******************************/
//
// NOTE: this is a recursive function in the case of group objects
//
// This will dispose of a single reference to the input object and any sub-objects in the
// case of a group.  If the refCount is 0, then the object is freed.
//

void MO_DisposeObjectReference(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MOVertexArrayObject	*vObj;

	if (obj == nil)
		DoFatalAlert("MO_DisposeObjectReference: obj == nil");

	if (header->cookie != MO_COOKIE)					// verify cookie
		DoFatalAlert("MO_DisposeObjectReference: bad cookie!");

			/**************************************/
			/* DEC REFERENCE COUNT OF THIS OBJECT */
			/**************************************/

	header->refCount--;									// dec ref count

	if (header->refCount < 0)							// see if over did it
		DoFatalAlert("MO_DisposeObjectReference: refcount < 0!");

	if (header->refCount == 0)							// see if we can DELETE this node
	{
			/* DELETE OBJECT'S SPECIFIC INFO */

		switch(header->type)
		{
			case	MO_TYPE_GROUP:
					MO_DisposeObject_Group(obj);
					break;

			case	MO_TYPE_GEOMETRY:
					GAME_ASSERT_MESSAGE(header->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY, "unknown geometry subtype");
					vObj = obj;
					MO_DisposeObject_Geometry_VertexArray(&vObj->objectData);
					break;

			case	MO_TYPE_MATERIAL:
					MO_DisposeObject_Material(obj);
					break;

			case	MO_TYPE_PICTURE:
					MO_DisposeObject_Picture(obj);
					break;
		}

			/* DELETE THE OBJECT NODE */

		MO_DetachFromLinkedList(obj);					// detach from linked list

		header->cookie = 0xdeadbeef;					// devalidate cookie
		SafeDisposePtr(obj);								// free memory used by object
		return;
	}
}



/***************** DETACH FROM LINKED LIST *********************/

static void MO_DetachFromLinkedList(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MetaObjectHeader	*prev,*next;


			/* VERIFY COOKIE */

	if (header->cookie != MO_COOKIE)
		DoFatalAlert("MO_DetachFromLinkedList: cookie is invalid!");


	prev = header->prevNode;
	next = header->nextNode;

			/* SEE IF WAS 1ST NODE IN LIST */

	if (prev == nil)
	{
		gFirstMetaObject = next;
		if (gFirstMetaObject)
			gFirstMetaObject->prevNode = nil;
	}

			/* SEE IF WAS LAST NODE IN LIST */

	if (next == nil)
	{
		gLastMetaObject = prev;
		if (gLastMetaObject)
			gLastMetaObject->nextNode = nil;
	}

			/* SOMEWHERE IN THE MIDDLE */

	else
	if (prev != nil)
	{
		prev->nextNode = next;
		next->prevNode = prev;
	}

	gNumMetaObjects--;

	if (gNumMetaObjects < 0)
		DoFatalAlert("MO_DetachFromLinkedList: counter mismatch");

	if (gNumMetaObjects == 0)
	{
		if (prev || next)							// if all gone, then prev & next should be nil
			DoFatalAlert("MO_DetachFromLinkedList: prev/next should be nil!");

		if (gFirstMetaObject != nil)
			DoFatalAlert("MO_DetachFromLinkedList: gFirstMetaObject should be nil!");

		if (gLastMetaObject != nil)
			DoFatalAlert("MO_DetachFromLinkedList: gLastMetaObject should be nil!");
	}
}


/**************** DISPOSE OBJECT:  GROUP ************************/
//
// Decrement the references of all objects in the group.
//

static void MO_DisposeObject_Group(MOGroupObject *group)
{
int	i,n;

	n = group->objectData.numObjectsInGroup;				// get # objects in group

	for (i = 0; i < n; i++)
	{
		MO_DisposeObjectReference(group->objectData.groupContents[i]);	// dispose of this object's ref
	}
}

/****************** DISPOSE OBJECT: PICTURE *******************/
//
// Decrement references to all of the materials used in this picture.
//

static void MO_DisposeObject_Picture(MOPictureObject *obj)
{
MOPictureData *data = &obj->objectData;

	MO_DisposeObjectReference(data->material);
	data->material = NULL;
}




/****************** DELETE OBJECT INFO: GEOMETRY : VERTEX ARRAY *******************/
//
// NOTE: this function is not static because the Skeleton code calls it for stuff.
//

void MO_DisposeObject_Geometry_VertexArray(MOVertexArrayData *data)
{
		/* DEREFERENCE ANY MATERIALS */

	int numMaterials = data->numMaterials;
	for (int i = 0; i < numMaterials; i++)
	{
		MO_DisposeObjectReference(data->materials[i]);	// dispose of this object's ref
	}

		/* DISPOSE OF VARIOUS ARRAYS */

	if (data->points)
	{
		SafeDisposePtr((Ptr)data->points);
		data->points = nil;
	}

	if (data->normals)
	{
		SafeDisposePtr((Ptr)data->normals);
		data->normals = nil;
	}

	if (data->uvs)
	{
		SafeDisposePtr((Ptr)data->uvs);
		data->uvs = nil;
	}

	if (data->colorsByte)
	{
		SafeDisposePtr((Ptr)data->colorsByte);
		data->colorsByte = nil;
	}

	if (data->colorsFloat)
	{
		SafeDisposePtr((Ptr)data->colorsFloat);
		data->colorsFloat = nil;
	}

	if (data->triangles)
	{
		SafeDisposePtr((Ptr)data->triangles);
		data->triangles = nil;
	}
}



/****************** DELETE OBJECT INFO: MATERIAL *******************/

static void MO_DisposeObject_Material(MOMaterialObject *obj)
{
MOMaterialData		*data = &obj->objectData;

		/* DISPOSE OF TEXTURE NAMES */

	if (data->numMipmaps > 0)
	{
		glDeleteTextures(data->numMipmaps, &data->textureName[0]);

		gNumTexturesAllocated -= data->numMipmaps;
	}
}



#pragma mark -

/******************** MO_DUPLICATE VERTEX ARRAY DATA *********************/

void MO_DuplicateVertexArrayData(MOVertexArrayData *inData, MOVertexArrayData *outData)
{
int	i,n,s;
			/***********************************/
			/* GET NEW REFERENCES TO MATERIALS */
			/***********************************/

	outData->numMaterials = n = inData->numMaterials;

	for (i = 0; i < n; i++)
	{
		MO_GetNewReference(inData->materials[i]);
		outData->materials[i] = inData->materials[i];
	}

			/************************/
			/* DUPLICATE THE ARRAYS */
			/************************/

			/* POINTS */

	n = outData->numPoints = inData->numPoints;
	s = inData->numPoints * sizeof(OGLPoint3D);

	if (inData->points)
	{
		outData->points = AllocPtr(s);
		if (outData->points == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->points, outData->points, s);
	}
	else
		outData->points = nil;


			/* NORMALS */

	s = n * sizeof(OGLVector3D);

	if (inData->normals)
	{
		outData->normals = AllocPtr(s);
		if (outData->normals == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->normals, outData->normals, s);
	}
	else
		outData->normals = nil;


			/* UVS */

	s = n * sizeof(OGLTextureCoord);

	if (inData->uvs)
	{
		outData->uvs = AllocPtr(s);
		if (outData->uvs == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->uvs, outData->uvs, s);
	}
	else
		outData->uvs = nil;


			/* COLORS BYTE */

	s = n * sizeof(OGLColorRGBA_Byte);

	if (inData->colorsByte)
	{
		outData->colorsByte = AllocPtr(s);
		if (outData->colorsByte == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->colorsByte, outData->colorsByte, s);
	}
	else
		outData->colorsByte = nil;


			/* COLORS FLOAT */

	s = n * sizeof(OGLColorRGBA);

	if (inData->colorsFloat)
	{
		outData->colorsFloat = AllocPtr(s);
		if (outData->colorsFloat == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->colorsFloat, outData->colorsFloat, s);
	}
	else
		outData->colorsFloat = nil;


			/* TRIANGLES */

	n = outData->numTriangles = inData->numTriangles;
	s = n * sizeof(GLint) * 3;

	if (inData->triangles)
	{
		outData->triangles = AllocPtr(s);
		if (outData->triangles == nil)
			DoFatalAlert("MO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->triangles, outData->triangles, s);
	}
	else
		outData->triangles = nil;
}

#pragma mark -


/******************** MO: CALC BOUNDING BOX ***********************/

void MO_CalcBoundingBox(MetaObjectPtr object, OGLBoundingBox *bBox)
{
		/* INIT BBOX TO BOGUS VALUES */

	bBox->min.x =
	bBox->min.y =
	bBox->min.z = 100000000;

	bBox->max.x =
	bBox->max.y =
	bBox->max.z = -bBox->min.x;


			/* RECURSIVELY CALC BBOX */

	MO_CalcBoundingBox_Recurse(object, bBox);
}


/******************** MO: CALC BOUNDING BOX: RECURSE ***********************/

static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_CalcBoundingBox_Recurse: cookie is invalid!");


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */

		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							geoData = &vObj->objectData;

							for (i = 0; i < geoData->numPoints; i++)
							{
								if (geoData->points[i].x < bBox->min.x)
									bBox->min.x = geoData->points[i].x;
								if (geoData->points[i].x > bBox->max.x)
									bBox->max.x = geoData->points[i].x;

								if (geoData->points[i].y < bBox->min.y)
									bBox->min.y = geoData->points[i].y;
								if (geoData->points[i].y > bBox->max.y)
									bBox->max.y = geoData->points[i].y;

								if (geoData->points[i].z < bBox->min.z)
									bBox->min.z = geoData->points[i].z;
								if (geoData->points[i].z > bBox->max.z)
									bBox->max.z = geoData->points[i].z;
							}
							break;

					default:
						DoFatalAlert("MO_CalcBoundingBox_Recurse: unknown sub-type!");
				}
				break;


			/* RECURSE THRU GROUP */

		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingBox_Recurse(groupObject->objectData.groupContents[i], bBox);
				break;


		case	MO_TYPE_MATRIX:
				DoFatalAlert("MO_CalcBoundingBox_Recurse: why is there a matrix here?");
				break;
	}
}


#pragma mark -



/******************* MO: GET TEXTURE FROM FILE ************************/

MOMaterialObject *MO_GetTextureFromFile(const char* path, int destPixelFormat)
{
MetaObjectPtr	obj;
MOMaterialData	matData;
int				width,height;
Ptr				buffer;
Ptr 			pictMapAddr;
uint32_t		pictRowBytes;

#if _DEBUG
	SDL_Log("MO_GetTextureFromFile: %s", path);
#endif

		/*******************************/
		/* CREATE TEXTURE PIXEL BUFFER */
		/*******************************/

		/* LOAD PICTURE FILE */

	{
		long imageLength = 0;
		Ptr imageData = LoadDataFile(path, &imageLength);
		GAME_ASSERT(imageData);

		pictMapAddr = (Ptr) stbi_load_from_memory((const stbi_uc*) imageData, imageLength, &width, &height, NULL, 4);
		GAME_ASSERT(pictMapAddr);

		SafeDisposePtr(imageData);
		imageData = NULL;
	}

	pictRowBytes = 4*width;


#if 0
			/* GET GWORLD INFO */

	if ((!IsPowerOf2(width)) || (!IsPowerOf2(height)))				// make sure its a power of 2
		DoFatalAlert("MO_GetTextureFromFile: dimensions not power of 2");
#endif

		/*************************************/
		/* CREATE TEXTURE OBJECT FROM PIXELS */
		/*************************************/

		/* COPY PIXELS FROM GWORLD INTO BUFFER */

	buffer = AllocPtr(width * height * 4);							// alloc enough for a 32bit texture
	if (buffer == nil)
		DoFatalAlert("MO_GetTextureFromResource: AllocPtr failed!");


		/* FLIP IMAGE VERTICALLY */

	{
		uint32_t	*dest, *src;

		src = (uint32_t *)(pictMapAddr + pictRowBytes * (height-1)); // start @ bottom to flip texture
		dest = (uint32_t *)buffer;

		for (int y = 0; y < height; y++)
		{
			SDL_memcpy(dest, src, width*4);
			dest += width;
			src -= pictRowBytes/4;
		}
	}

	stbi_image_free(pictMapAddr);
	pictMapAddr = nil;

			/* CREATE NEW TEXTURE OBJECT */

//	matData.setupInfo		= setupInfo;
	matData.flags			= BG3D_MATERIALFLAG_TEXTURED;
	matData.diffuseColor.r	= 1;
	matData.diffuseColor.g	= 1;
	matData.diffuseColor.b	= 1;
	matData.diffuseColor.a	= 1;

	matData.numMipmaps		= 1;
	matData.width			= width;
	matData.height			= height;
	matData.pixelSrcFormat	= GL_RGBA;
	matData.pixelDstFormat	= destPixelFormat;
	matData.texturePixels[0]= nil;						// we're going to preload
	matData.textureName[0] 	= OGL_TextureMap_Load(buffer, width, height,
												 matData.pixelSrcFormat,
												 destPixelFormat, GL_UNSIGNED_BYTE);

	obj = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);

	SafeDisposePtr(buffer);									// dispose of our copy of the buffer

	return(obj);
}



/******************* MO: OBJECT OFFSET UVS ************************/

void MO_Object_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOGroupData			*data;
int					i;
MOGroupObject		*group;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_Group_OffsetUVs: cookie is invalid!");


			/* HANDLE IT */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				MO_VertexArray_OffsetUVs(object, du, dv);
				break;

		case	MO_TYPE_GROUP:
				group = object;
				data = &group->objectData;

							/* PARSE OBJECTS IN GROUP */

				for (i = 0; i < data->numObjectsInGroup; i++)
				{
					switch(data->groupContents[i]->type)
					{
						case	MO_TYPE_GEOMETRY:
								MO_VertexArray_OffsetUVs(data->groupContents[i], du, dv);
								break;

						case	MO_TYPE_GROUP:
								MO_Object_OffsetUVs(data->groupContents[i], du, dv);		// recurse this sub-group
								break;

					}
				}
				break;


		default:
			DoFatalAlert("MO_Group_OffsetUVs: object type is not supported.");
	}

}


/******************* MO: VERTEX ARRAY, OFFSET UVS ************************/

void MO_VertexArray_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayData	*data;
int					numPoints,i;
OGLTextureCoord		*uvPtr;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_VertexArray_OffsetUVs: cookie is invalid!");


		/* MAKE SURE IT IS A VERTEX ARRAY */

	if ((objHead->type != MO_TYPE_GEOMETRY) || (objHead->subType != MO_GEOMETRY_SUBTYPE_VERTEXARRAY))
		DoFatalAlert("MO_VertexArray_OffsetUVs: object is not a Vertex Array!");

	vObj = object;
	data = &vObj->objectData;						// point to data


	uvPtr = data->uvs;								// point to uv list
	if (uvPtr == nil)
		return;

	numPoints = data->numPoints;					// get # points

			/* OFFSET THE UV'S */

	for (i = 0; i < numPoints; i++)
	{
		uvPtr[i].u += du;
		uvPtr[i].v += dv;
	}
}




