/****************************/
/*   PILLARBOX.C            */
/* (c)2022 Iliyas Jorio     */
/****************************/

#include "game.h"

static MOMaterialObject*	gPillarboxMaterial = nil;
static float				gPillarboxBrightness = 0;

void DisposePillarboxMaterial(void)
{
	if (gPillarboxMaterial)
	{
		MO_DisposeObjectReference(gPillarboxMaterial);
		gPillarboxMaterial = nil;
	}
}

static void DrawPillarbox(ObjNode* objNode)
{
	// Get dimensions of 0th viewport
	int vpw = gGameView->panes[0].vpw;
	int vph = gGameView->panes[0].vph;

	// Don't draw if it's not gonna be visible
	if (gGameView->pillarboxRatio <= 0
		|| (vpw == gGameWindowWidth && vph == gGameWindowHeight))
	{
		return;
	}

	// 2D state should have been set for us by STATUS_BITS_FOR_2D and ObjNode::Projection.

	MO_DrawMaterial(gPillarboxMaterial);

	float shadowOpacity = 0.99f;
	float shadowThickness = 0.11f;
	float z = 0;
	float textureAR = 1024.0f / 768.0f;

	// See if pillarbox brightness should track global fade brightness
	// (Force tracking if we're not at full brightness already)
	if (gPillarboxBrightness < 1.0 || gGameView->fadePillarbox)
		gPillarboxBrightness = gGammaFadePercent;

	if (vph == gGameWindowHeight) // widescreen
	{
		float stripeW = 0.5f * (gGameWindowWidth - vpw);
		float stripeAR = stripeW / gGameWindowHeight;
		float texRegionAR = textureAR / 2;

		float qB = -1;
		float qT = 1;
		float qL1 = (1.0f - stripeW / (0.5f * gGameWindowWidth));
		float qR1 = 1.0f;
		float qR2 = -qL1;
		float qL2 = -1.0f;
		float qL3 = qR2 - shadowThickness;  // shadow
		float qR3 = qL1 + shadowThickness;

		float tcT, tcB, tcL1, tcR1, tcL2, tcR2;
		if (stripeAR <= texRegionAR)
		{
			// padding stripe is narrower than texture -- keep height, crop sides
			tcB = 1;
			tcT = 0;
			tcL1 = .5f;
			tcR1 = .5f + .5f * (stripeAR / texRegionAR);
			tcL2 = .5f - .5f * (stripeAR / texRegionAR);
			tcR2 = .5f;
		}
		else
		{
			// ultra-wide -- pin image to bottom and crop sides
			tcB = 1;
			tcT = 1 - (texRegionAR / stripeAR);
			tcL1 = .5f;
			tcR1 = 1;
			tcL2 = 0;
			tcR2 = .5f;
		}

		glColor4f(gPillarboxBrightness, gPillarboxBrightness, gPillarboxBrightness, 1);
		glBegin(GL_QUADS);
		glTexCoord2f(tcL1, tcB);		glVertex3f(qL1, qB, z);		// Quad 1 (right)
		glTexCoord2f(tcR1, tcB);		glVertex3f(qR1, qB, z);
		glTexCoord2f(tcR1, tcT);		glVertex3f(qR1, qT, z);
		glTexCoord2f(tcL1, tcT);		glVertex3f(qL1, qT, z);
		glTexCoord2f(tcL2, tcB);		glVertex3f(qL2, qB, z);		// Quad 2 (left)
		glTexCoord2f(tcR2, tcB);		glVertex3f(qR2, qB, z);
		glTexCoord2f(tcR2, tcT);		glVertex3f(qR2, qT, z);
		glTexCoord2f(tcL2, tcT);		glVertex3f(qL2, qT, z);
		glEnd();

		glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(0,0,0,shadowOpacity);
		glVertex3f(qL1, qT, z);		// Quad 1 (right)
		glVertex3f(qL1, qB, z);
		glColor4f(0,0,0,0);
		glVertex3f(qR3, qB, z);
		glVertex3f(qR3, qT, z);
		glVertex3f(qL3, qT, z);		// Quad 2 (left)
		glVertex3f(qL3, qB, z);
		glColor4f(0,0,0,shadowOpacity);
		glVertex3f(qR2, qB, z);
		glVertex3f(qR2, qT, z);
		glEnd();
	}
	else // tallscreen
	{
		float stripeH = 0.5f * (gGameWindowHeight - vph);
		float stripeAR = gGameWindowWidth / stripeH;
		float texRegionAR = textureAR * 2;

		float qL = -1.0f;
		float qR = 1.0f;
		float qB1 = (1.0f - stripeH / (0.5f * gGameWindowHeight));
		float qT1 = 1.0;
		float qB2 = -1.0;
		float qT2 = -qB1;
		float qB3 = qT2 - shadowThickness;  // shadow
		float qT3 = qB1 + shadowThickness;

		float tcL, tcR, tcT1, tcB1, tcT2, tcB2;
		if (stripeAR <= texRegionAR)
		{
			// keep height, crop left/right
			float invZoom = stripeAR / texRegionAR;
			tcL =     (1 - invZoom) * 0.5f;
			tcR = 1 - (1 - invZoom) * 0.5f;
			tcT1 = 0.0f; // pin top
			tcB1 = 0.5f; // pin bottom
			tcT2 = 0.5f;
			tcB2 = 1.0f;
		}
		else
		{
			// keep width, crop top/bottom
			float zoom = texRegionAR / stripeAR;
			tcL = 0;
			tcR = 1;
			tcT1 = 0.5f - .5f * zoom;
			tcB1 = 0.5f;
			tcT2 = 0.5f;
			tcB2 = 0.5f + .5f * zoom;
		}

		glColor4f(gPillarboxBrightness, gPillarboxBrightness, gPillarboxBrightness, 1);
		glBegin(GL_QUADS);
		glTexCoord2f(tcL, tcB1);		glVertex3f(qL, qB1, z);		// Quad 1 (top)
		glTexCoord2f(tcR, tcB1);		glVertex3f(qR, qB1, z);
		glTexCoord2f(tcR, tcT1);		glVertex3f(qR, qT1, z);
		glTexCoord2f(tcL, tcT1);		glVertex3f(qL, qT1, z);
		glTexCoord2f(tcL, tcB2);		glVertex3f(qL, qB2, z);		// Quad 2 (bottom)
		glTexCoord2f(tcR, tcB2);		glVertex3f(qR, qB2, z);
		glTexCoord2f(tcR, tcT2);		glVertex3f(qR, qT2, z);
		glTexCoord2f(tcL, tcT2);		glVertex3f(qL, qT2, z);
		glEnd();

		glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor4f(0,0,0,shadowOpacity);
		glVertex3f(qL, qB1, z);		// Quad 1 (top)
		glVertex3f(qR, qB1, z);
		glColor4f(0,0,0,0);
		glVertex3f(qR, qT3, z);
		glVertex3f(qL, qT3, z);
		glVertex3f(qL, qB3, z);
		glVertex3f(qR, qB3, z);
		glColor4f(0,0,0,shadowOpacity);
		glVertex3f(qR, qT2, z);		// Quad 2 (bottom)
		glVertex3f(qL, qT2, z);
		glEnd();
	}
}

static ObjNode* MakePillarboxObject(void)
{
	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BIT_OVERLAYPANE | STATUS_BITS_FOR_2D,
		.slot = PILLARBOX_SLOT,
		.scale = 1,
		.projection = kProjectionType2DNDC,
		.drawCall = DrawPillarbox,
	};

	return MakeNewObject(&def);
}

void InitPillarbox(void)
{
	if (gGameView->pillarboxRatio > 0)
	{
		if (gPillarboxMaterial == nil)
		{
			gPillarboxMaterial = MO_GetTextureFromFile(":Images:Pillarbox.jpg", GL_RGB);
		}

		MakePillarboxObject();
	}
	else
	{
		// This fullscreen scene doesn't need a pillarbox.
		// Next time we enter a scene needing a pillarbox, make it fade in.
		gPillarboxBrightness = 0;
	}
}
