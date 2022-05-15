#pragma once

ObjNode* MakeQuadMeshObject(NewObjectDefinitionType* newObjDef, int capacity, MOMaterialObject* material);
void ReallocateQuadMesh(MOVertexArrayData* mesh, int numQuads);
MOVertexArrayData* GetQuadMeshWithin(ObjNode* theNode);
