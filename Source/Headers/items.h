//
// items.h
//


extern	void InitItemsManager(void);
void CreateCyclorama(void);
Boolean AddTree(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddFinishLine(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddEasterHead(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddPillar(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPylon(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBoat(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddStatue(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSphinx(TerrainItemEntryType *itemPtr, long  x, long z);


Boolean AddSign(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddRock(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddBrontoNeck(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRockOverhang(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddVine(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRickshaw(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddAztecHead(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCastleTower(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddHouse(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWell(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddVolcano(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddClock(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddStoneHenge(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddColiseum(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddStump(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBaracade(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddVikingFlag(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddTorchPot(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddClam(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddVikingGong(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWeaponsRack(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddFlagPole(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean PrimePolarBear(long splineNum, SplineItemType *itemPtr);

Boolean AddFlower(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean PrimeViking(long splineNum, SplineItemType *itemPtr);






