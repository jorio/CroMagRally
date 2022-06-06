//
// effects.h
//

#pragma once

#define	MAX_PARTICLE_GROUPS		70
#define	MAX_PARTICLES			250		// (note change Byte below if > 255)


		/* FIRE & SMOKE */

#define	FireTimer			SpecialF[0]
#define	SmokeTimer			SpecialF[1]
#define	SmokeParticleGroup	Special[0]
#define	SmokeParticleMagic	Special[1]



typedef struct
{
	uint32_t		magicNum;
	Byte			isUsed[MAX_PARTICLES];
	Byte			type;
	Byte			flags;
	Byte			particleTextureNum;
	float			gravity;
	float			magnetism;
	float			baseScale;
	float			decayRate;						// shrink speed
	float			fadeRate;

	int				srcBlend,dstBlend;

	float			alpha[MAX_PARTICLES];
	float			scale[MAX_PARTICLES];
	float			rotZ[MAX_PARTICLES];
	float			rotDZ[MAX_PARTICLES];
	OGLPoint3D		coord[MAX_PARTICLES];
	OGLVector3D		delta[MAX_PARTICLES];

	MOVertexArrayObject	*geometryObj;

}ParticleGroupType;




enum
{
	PARTICLE_TYPE_FALLINGSPARKS,
	PARTICLE_TYPE_GRAVITOIDS
};

enum
{
	PARTICLE_FLAGS_BOUNCE = (1<<0),
	PARTICLE_FLAGS_HURTPLAYER = (1<<1),
	PARTICLE_FLAGS_HURTPLAYERBAD = (1<<2),	//combine with PARTICLE_FLAGS_HURTPLAYER
	PARTICLE_FLAGS_HURTENEMY = (1<<3),
	PARTICLE_FLAGS_DONTCHECKGROUND = (1<<4),
	PARTICLE_FLAGS_EXTINGUISH = (1<<5),
	PARTICLE_FLAGS_HOT = (1<<6)
};


/********** PARTICLE GROUP DEFINITION **************/

typedef struct
{
	uint32_t magicNum;
	Byte 	type;
	Byte 	flags;
	float 	gravity;
	float 	magnetism;
	float 	baseScale;
	float 	decayRate;
	float 	fadeRate;
	Byte 	particleTextureNum;
	int		srcBlend,dstBlend;
}NewParticleGroupDefType;

/*************** NEW PARTICLE DEFINITION *****************/

typedef struct
{
	short 		groupNum;
	OGLPoint3D 	*where;
	OGLVector3D *delta;
	float 		scale;
	float		rotZ,rotDZ;
	float 		alpha;
}NewParticleDefType;



#define	FULL_ALPHA	1.0f


void InitEffects(void);
ObjNode* InitParticleSystem(void);


void DeleteAllParticleGroups(void);
short NewParticleGroup(NewParticleGroupDefType *def);
Boolean AddParticleToGroup(NewParticleDefType *def);
void MoveParticleGroups(void);
Boolean VerifyParticleGroupMagicNum(short group, uint32_t magicNum);
Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags);

void MakePuff(float x, float y, float z);
void MakeSparkExplosion(float x, float y, float z, float force, short sparkTexture);

void MakeBombExplosion(ObjNode *theBomb, float x, float z, const OGLVector3D *delta);
void MakeShockwave(float x, float y, float z);
void MakeConeBlast(float x, float y, float z);
void MakeSplash(float x, float y, float z);

void MakeSnow(void);
void MakeSnowExplosion(const OGLPoint3D *where);
void MakeSnowShockwave(float x, float y, float z);

void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke, short particleType, float scale);

void MakeBubbles(ObjNode *theNode, const OGLPoint3D *coord, float fadeRate, float scale);

void MakeFireExplosion(float x, float z, OGLVector3D *delta);
Boolean AddBubbleGenerator(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddLavaGenerator(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_Lava(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

