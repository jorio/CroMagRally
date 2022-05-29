// QUAD MESH
// (C) 2022 Iliyas Jorio
// This file is part of Cro-Mag Rally. https://github.com/jorio/cromagrally

#include "game.h"

void ReallocateQuadMesh(MOVertexArrayData* mesh, int numQuads)
{
	if (mesh->points)
	{
		SafeDisposePtr((Ptr) mesh->points);
		mesh->points = nil;
		mesh->pointCapacity = 0;
		mesh->numPoints = 0;
	}

	if (mesh->uvs)
	{
		SafeDisposePtr((Ptr) mesh->uvs);
		mesh->uvs = nil;
	}

	if (mesh->triangles)
	{
		SafeDisposePtr((Ptr) mesh->triangles);
		mesh->triangles = nil;
		mesh->triangleCapacity = 0;
		mesh->numTriangles = 0;
	}

	int numPoints = numQuads * 4;
	int numTriangles = numQuads * 2;

	if (numQuads != 0)
	{
		mesh->points = (OGLPoint3D *) AllocPtr(sizeof(OGLPoint3D) * numPoints);
		mesh->uvs = (OGLTextureCoord *) AllocPtr(sizeof(OGLTextureCoord) * numPoints);
		mesh->triangles = (MOTriangleIndecies *) AllocPtr(sizeof(MOTriangleIndecies) * numTriangles);

		mesh->pointCapacity = numPoints;
		mesh->triangleCapacity = numTriangles;

		// Prep triangle-vertex assignments, which never change
		int t = 0;
		for (int p = 0; p < numPoints; p += 4, t += 2)
		{
			mesh->triangles[t + 0].vertexIndices[0] = p + 0;
			mesh->triangles[t + 0].vertexIndices[1] = p + 1;
			mesh->triangles[t + 0].vertexIndices[2] = p + 2;
			mesh->triangles[t + 1].vertexIndices[0] = p + 2;
			mesh->triangles[t + 1].vertexIndices[1] = p + 3;
			mesh->triangles[t + 1].vertexIndices[2] = p + 0;
		}
	}
}

MOVertexArrayData* GetQuadMeshWithin(ObjNode* theNode)
{
	GAME_ASSERT(theNode->Genre == TEXTMESH_GENRE || theNode->Genre == QUADMESH_GENRE);
	GAME_ASSERT(theNode->BaseGroup);
	GAME_ASSERT(theNode->BaseGroup->objectData.numObjectsInGroup >= 2);

	void*					metaObject			= theNode->BaseGroup->objectData.groupContents[1];
	MetaObjectHeader*		metaObjectHeader	= metaObject;
	MOVertexArrayObject*	vertexObject		= metaObject;
	MOVertexArrayData*		mesh				= &vertexObject->objectData;

	GAME_ASSERT(metaObjectHeader->type == MO_TYPE_GEOMETRY);
	GAME_ASSERT(metaObjectHeader->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY);

	return mesh;
}

ObjNode* MakeQuadMeshObject(NewObjectDefinitionType* newObjDef, int quadCapacity, MOMaterialObject* material)
{
	// If no material was given, make a blank material
	bool ownMaterial = false;
	if (material == NULL)
	{
		MOMaterialData matData =
		{
			.flags				= 0,		// not textured
			.numMipmaps			= 0,
			.diffuseColor		= (OGLColorRGBA) {1, 1, 1, 1},
		};
		material = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
		ownMaterial = true;
	}

	if (newObjDef->genre == ILLEGAL_GENRE)
		newObjDef->genre = QUADMESH_GENRE;	// Force genre to QUADMESH

	// Create object
	ObjNode* textNode = MakeNewObject(newObjDef);

	// Create mesh
	MOVertexArrayData mesh =
	{
		.numMaterials	= 1,
		.materials		= {material},
	};
	ReallocateQuadMesh(&mesh, quadCapacity);
	MetaObjectPtr meshMO = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &mesh);

	// Attach color mesh
	CreateBaseGroup(textNode);
	AttachGeometryToDisplayGroupObject(textNode, meshMO);

	// Dispose of extra reference to mesh
	MO_DisposeObjectReference(meshMO);
	meshMO = NULL;

	// Dispose of extra reference to material that we've created
	if (ownMaterial)
	{
		MO_DisposeObjectReference(material);
		material = NULL;
	}

	UpdateObjectTransforms(textNode);

	return textNode;
}
