/***************************/
/*   	CHECKPOINTS.C      */
/***************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/




/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

long				gNumCheckpoints;
CheckpointDefType	gCheckpointList[MAX_CHECKPOINTS];

short				gNumLapsThisRace = 3;

short				gWorstHumanPlace;


/*********************** UPDATE PLAYER CHECKPOINTS ***************************/

void UpdatePlayerCheckpoints(short p)
{
float	playerFromX, playerFromZ, playerToX, playerToZ;
short	c,i;
float	intersectX, intersectZ;
short	oldCheckpoint,newCheckpoint,nextCheckpoint;
float	x1,z1,x2,z2,rot;
OGLVector2D	checkToCheck,aim,deltaVec;


		/* SEE IF NEED TO DO THIS */

	switch(gGameMode)
	{
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
		case	GAME_MODE_SURVIVAL:
		case	GAME_MODE_CAPTUREFLAG:
				return;
	}


			/* SEE IF THIS PLAYER HAS ALREADY COMPLETED THE RACE */

	if (gPlayerInfo[p].raceComplete)
	{



	}

			/* GET PLAYER'S MOVEMENT LINE SEGMENT */

	playerFromX = gPlayerInfo[p].objNode->OldCoord.x;
	playerFromZ = gPlayerInfo[p].objNode->OldCoord.z;
	playerToX 	= gCoord.x;	//gPlayerInfo[p].objNode->Coord.x;
	playerToZ 	= gCoord.z;	//gPlayerInfo[p].objNode->Coord.z;


			/**********************************/
			/* SEE IF CROSSED ANY CHECKPOINTS */
			/**********************************/

	for (c = 0; c < gNumCheckpoints; c++)
	{
		x1 = gCheckpointList[c].x[0];													// get endpoints of checkpoint
		z1 = gCheckpointList[c].z[0];
        x2 = gCheckpointList[c].x[1];
        z2 = gCheckpointList[c].z[1];


					/*************************************/
					/* DO LINE SEGMENT INTERSECTION TEST */
					/*************************************/

		if (IntersectLineSegments(playerFromX, playerFromZ, playerToX, playerToZ,x1,z1,x2,z2,&intersectX, &intersectZ))
    	{
			oldCheckpoint = gPlayerInfo[p].checkpointNum;								// get old checkpoint #

					/* SEE IF CROSSED FINISH LINE */
					//
					// This can happen by going forward or backward over it, so
					// we need to handle it carefully.
					//

			if (c == 0)
			{
						/* SEE IF WENT FORWARD THRU FINISH LINE */

				if (oldCheckpoint == (gNumCheckpoints - 1))
				{
					short	count = 0;

					for (i = 0; i < gNumCheckpoints; i++)								// count # of checkpoints tagged
					{
						if (gPlayerInfo[p].checkpointTagged[i])
							count++;
					}

							/* SEE IF WE DID A NEW LAP */

					if (count > (gNumCheckpoints / 2))									// if crossed at least 50% of the checkpoints then assume we did a full lap
					{
						gPlayerInfo[p].lapNum++;

						if (gPlayerInfo[p].lapNum >= gNumLapsThisRace)					// see if completed race
							PlayerCompletedRace(p);
						else
							ShowLapNum(p);
					}
				}
						/* RESET ALL CHECKPOINT TAGS WHENEVER WE CROSS THE FINISH LINE*/

				for (i = 0; i < gNumCheckpoints; i++)
					gPlayerInfo[p].checkpointTagged[i] = false;

				newCheckpoint = c;
			}

					/* SEE IF FORWARD */
			else
			if (c > oldCheckpoint)
			{
				newCheckpoint = c;
				gPlayerInfo[p].checkpointTagged[c] = true;
			}

					/* SEE IF WENT BACK */
			else
			if (c <= oldCheckpoint)
			{
				if (c == 0)																// if went back over finish line then dec lap counter
				{
					if (gPlayerInfo[p].lapNum >= 0)
						gPlayerInfo[p].lapNum--;										// just lost a lap
					newCheckpoint = gNumCheckpoints-1;
					for (i = 0; i < gNumCheckpoints; i++)								// set all tags so can go back thru finish line for credit
						gPlayerInfo[p].checkpointTagged[i] = true;

				}
				else
				{
					newCheckpoint = c-1;
					gPlayerInfo[p].checkpointTagged[c] = false;							// untag the other checkpoint
				}
			}

				/* THIS SHOULD ONLY HAPPEN WHEN LAPPED AROUND TO 1ST CHECKPOINT AGAIN */
				//
				// This happens anytime the finish line is crossed, but remember that
				// it does not guarantee that the player did a lap - they could have
				// just cheated by backing up and re-crossing the finish line.  So,
				// we have to check that they went all the way around the track and
				// didnt skip any checkpoints.
				//

			else
			{
				newCheckpoint = c;

				for (i = 0; i < gNumCheckpoints; i++)									// verify that all checkpoints were tagged
				{
					if (!gPlayerInfo[p].checkpointTagged[i])							// if this checkpoint was not tagged then they didnt lap
						goto no_lap;
				}
				gPlayerInfo[p].lapNum++;												// yep, we lapped because all the checkpoints were tagged

					/* SEE IF COMPLETED THE RACE */

				if (gPlayerInfo[p].lapNum >= gNumLapsThisRace)
					PlayerCompletedRace(p);
				else
					ShowLapNum(p);


no_lap:;
				for (i = 0; i < gNumCheckpoints; i++)									// reset all tags
					gPlayerInfo[p].checkpointTagged[i] = false;
				gPlayerInfo[p].checkpointTagged[c] = true;								// except the one we passed thru
			}

			gPlayerInfo[p].checkpointNum = newCheckpoint;								// update player's current ckpt #
			break;
		}
	}

			/**************************************/
			/* SEE HOW FAR TO THE NEXT CHECKPOINT */
			/**************************************/

	newCheckpoint = gPlayerInfo[p].checkpointNum;

				/* GET NEXT CKP # */

	if (newCheckpoint == (gNumCheckpoints-1))						// see if wrap around
		nextCheckpoint = 0;
	else
		nextCheckpoint = newCheckpoint+1;


		/* GET CENTERPOINT OF THE NEXT CHECKPOINT */

	x1 = (gCheckpointList[nextCheckpoint].x[0] + gCheckpointList[nextCheckpoint].x[1]) * .5f;
	z1 = (gCheckpointList[nextCheckpoint].z[0] + gCheckpointList[nextCheckpoint].z[1]) * .5f;


			/* CALC DIST TO NEXT CHECKPOINT */

	gPlayerInfo[p].distToNextCheckpoint = CalcDistance(x1, z1, playerToX, playerToZ);


				/*********************************/
				/* SEE IF WE'RE AIMING BACKWARDS */
				/*********************************/


				/* GET CENTERPOINT OF CURRENT CHECKPOINT */

	x2 = (gCheckpointList[newCheckpoint].x[0] + gCheckpointList[newCheckpoint].x[1]) * .5f;
	z2 = (gCheckpointList[newCheckpoint].z[0] + gCheckpointList[newCheckpoint].z[1]) * .5f;


					/* CALC VECTOR FROM NEXT CHECKPOINT TO CURRENT */

	x1 = x2 - x1;
	z1 = z2 - z1;
	FastNormalizeVector2D(x1, z1, &checkToCheck, false);


				/* ALSO CALC DELTA VECTOR */

	FastNormalizeVector2D(gDelta.x, gDelta.z, &deltaVec, true);


				/* SEE IF AIM VECTOR IS CLOSE TO PARALLEL TO THAT VEC */

	rot = gPlayerInfo[p].objNode->Rot.y;
	aim.x = -sin(rot);
	aim.y = -cos(rot);

	if ((OGLVector2D_Dot(&aim, &checkToCheck) > .6f) && (OGLVector2D_Dot(&deltaVec, &checkToCheck) > .6f))
		gPlayerInfo[p].wrongWay = true;
	else
		gPlayerInfo[p].wrongWay = false;

}


/************************** CALC PLAYER PLACES **************************/
//
// Determine placing by counting how many players are in front of each player.
//

void CalcPlayerPlaces(void)
{
short	p,place,i;

		/* SEE IF NEED TO DO THIS */

	switch(gGameMode)
	{
		case	GAME_MODE_TAG1:
		case	GAME_MODE_TAG2:
		case	GAME_MODE_SURVIVAL:
		case	GAME_MODE_CAPTUREFLAG:
				return;
	}

	if (gIsSelfRunningDemo)
		gWorstHumanPlace = 5;										// no humans in demo, so trick so that CPU cars will all attack each other
	else
		gWorstHumanPlace = 0;										// also calc which human player is in last place

	for (p = 0; p < gNumTotalPlayers; p++)
	{
		if (gPlayerInfo[p].raceComplete)							// if player already done, then dont do anything
			continue;

		place = 0;													// assume 1st place

		for (i = 0; i < gNumTotalPlayers; i++)						// check place with other players
		{
			if (p == i)												// dont compare against self
				continue;

			if (gPlayerInfo[i].raceComplete)						// skip players that have completed race
				goto next;


						/* CHECK LAPS */

			if (gPlayerInfo[p].lapNum > gPlayerInfo[i].lapNum)					// see if I'm more laps than other guy
				continue;

			if (gPlayerInfo[p].lapNum < gPlayerInfo[i].lapNum)					// see if I'm less laps than other guy
				goto next;


					/* SAME LAP, SO CHECK CHECKPOINT */

			if (gPlayerInfo[p].checkpointNum > gPlayerInfo[i].checkpointNum)	// see if I'm more cp's than other guy
				continue;

			if (gPlayerInfo[p].checkpointNum < gPlayerInfo[i].checkpointNum)	// see if I'm less cp's than other guy
				goto next;


					/* SAME LAP & CHECKPOINT, SO CHECK DIST TO NEXT CHECKPOINT */

			if (gPlayerInfo[p].distToNextCheckpoint < gPlayerInfo[i].distToNextCheckpoint)
				continue;

next:
			place++;
		}


		gPlayerInfo[p].place = place;

					/* FOR HUMAN PLAYERS, SEE WHO IS FARTHEST BEHIND */

		if (!gPlayerInfo[p].isComputer)
		{
			if (place > gWorstHumanPlace)
				gWorstHumanPlace = place;
		}
	}
}




/****************** PLAYER COMPLETED RACE ************************/
//
// Called when player crosses the finish line on the final lap.
//

void PlayerCompletedRace(short playerNum)
{
	gPlayerInfo[playerNum].raceComplete = true;

			/* TELL WINNER IN MULTIPLAYER RACE */
			//
			// Game ends as soon as 1st player finishes the race
			//

	if (gGameMode == GAME_MODE_MULTIPLAYERRACE)
	{
		short	i;

		if (!gTrackCompleted)									// only if this is the 1st guy to win
		{
			for (i = 0; i < gNumTotalPlayers; i++)				// see which player Won (was not eliminated)
			{
				if (i != playerNum)
				{
					ShowWinLose(i, 2, playerNum);					// lost
				}
				else
					ShowWinLose(i, 1, playerNum);					// won!
			}

			gTrackCompleted = true;
			gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;
		}
	}

				/* WINNER IN 1-PLAYER RACE */

	else
	{
		ShowFinalPlace(playerNum);

		if (!gPlayerInfo[playerNum].isComputer)
		{
			gTrackCompleted = true;
			gTrackCompletedCoolDownTimer = TRACK_COMPLETE_COOLDOWN_TIME;
		}
	}
}









