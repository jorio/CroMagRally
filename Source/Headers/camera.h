//
// camera.h
//

enum
{
	CAMERA_MODE_NORMAL1,
	CAMERA_MODE_NORMAL2,
	CAMERA_MODE_NORMAL3,
	CAMERA_MODE_FIRSTPERSON,
	NUM_CAMERA_MODES
};

void InitCameras(void);
void SetDefaultCameraModeForAllPlayers(void);
void UpdateCameras(Boolean priming, Boolean forceRefreshMode);
void DrawLensFlare(void);
