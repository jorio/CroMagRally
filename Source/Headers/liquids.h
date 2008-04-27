//
// liquids.h
//


enum
{
	LIQUID_WATER,
	LIQUID_TAR
};


//==========================================

void UpdateLiquidAnimation(void);
float FindLiquidY(float x, float z);
Boolean AddWaterPatch(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWaterfall(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddTarPatch(TerrainItemEntryType *itemPtr, long  x, long z);
