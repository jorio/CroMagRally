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
void UpdateCameras(Boolean priming);
void DrawLensFlare(OGLSetupOutputType *setupInfo);
