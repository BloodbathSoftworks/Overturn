/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "usercmd.h"

#include <string.h>
#include <stdio.h>

#define BAT_NAME "sprites/hud_mode.spr"

int CHudMovement::Init(void)
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

void CHudMovement::Reset(void)
{
}

int CHudMovement::VidInit(void)
{
	int HUD_mode_stand = gHUD.GetSpriteIndex( "mode_stand" );
	int HUD_mode_run = gHUD.GetSpriteIndex( "mode_run" );
	int HUD_mode_crouch = gHUD.GetSpriteIndex( "mode_crouch" );
	int HUD_mode_jump = gHUD.GetSpriteIndex( "mode_jump" );

	m_hSpriteStand = gHUD.GetSprite(HUD_mode_stand);
	m_hSpriteRun = gHUD.GetSprite(HUD_mode_run);
	m_hSpriteCrouch = gHUD.GetSprite(HUD_mode_crouch);
	m_hSpriteJump = gHUD.GetSprite(HUD_mode_jump);

	m_hRCStand = &gHUD.GetSpriteRect(HUD_mode_stand);
	m_hRCRun = &gHUD.GetSpriteRect(HUD_mode_run);
	m_hRCCrouch = &gHUD.GetSpriteRect(HUD_mode_crouch);
	m_hRCJump = &gHUD.GetSpriteRect(HUD_mode_jump);

	return 1;
};

int CHudMovement::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) )
		return 1;

	int r, g, b;
	wrect_t *rc;
	HSPRITE sprite;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	vec3_t vSimvel;
	VectorCopy(gHUD.m_pParams.simvel, vSimvel);

	if(gHUD.m_pParams.simvel[2])
	{
		sprite = m_hSpriteJump;
		rc = m_hRCJump;
	}
	else if(gHUD.m_pParams.cmd->buttons & IN_DUCK)
	{
		sprite = m_hSpriteCrouch;
		rc = m_hRCCrouch;
	}
	else if(vSimvel.Length2D())
	{
		sprite = m_hSpriteRun;
		rc = m_hRCRun;
	}
	else
	{
		sprite = m_hSpriteStand;
		rc = m_hRCStand;
	}

	UnpackRGB(r,g,b, gHUD.m_iHUDColor);
	ScaleColors(r, g, b, 255);

	int x = ScreenWidth - (m_hRCStand->right)*1.5;
	int y = (m_hRCStand->bottom+m_hRCStand->top)*2;

	SPR_Set(sprite, r, g, b );
	SPR_Draw( 0,  x, y, rc);

	return 1;
}
