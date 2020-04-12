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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	int HUD_power = gHUD.GetSpriteIndex( "power" );

	m_HUD_powerbar = gHUD.GetSpriteIndex( "powerbar" );
	m_HUD_roundfix = gHUD.GetSpriteIndex( "roundbar" );

	m_prc3 = &gHUD.GetSpriteRect(m_HUD_powerbar);
	m_iWidth = m_prc3->right - m_prc3->left;

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_power );
	m_iHeight = m_prc1->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
};

int CHudBattery:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
		m_flBat = ((float)x)/100.0;
	}

	return 1;
}


int CHudBattery::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	UnpackRGB(r,g,b, gHUD.m_iHUDColor);

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	a = MIN_ALPHA;

	ScaleColors(r, g, b, a );
	
	int iOffset = (m_prc1->bottom - m_prc1->top)/6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = ScreenWidth/5;

	// make sure we have the right sprite handles
	if ( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "power" ) );

	SPR_Set(m_hSprite1, 255, 255, 255 );
	SPR_Draw( 0,  x, y, m_prc1);

		int iOffset2 = m_iWidth * (1.0 - m_flBat);	//32 * (1 - 1) = 0

		if (iOffset2 < m_iWidth)
		{
			rc = *m_prc3;
			rc.left += iOffset2;

			y = y - (gHUD.GetSpriteRect(m_HUD_powerbar).bottom - gHUD.GetSpriteRect(m_HUD_powerbar).top) - 5;
			x = x + 7;

			SPR_Set(gHUD.GetSprite(m_HUD_powerbar), 255, 255, 255 );
			SPR_Draw(0, x, y, &rc);

			SPR_Set(gHUD.GetSprite(m_HUD_roundfix), 255, 255, 255 );
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_roundfix));
		}

	return 1;
}
