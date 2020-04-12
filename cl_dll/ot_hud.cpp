/***
*
*	Copyright (c) 2013, Overturn Team. All rights reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from the Overturn Team.
*
****/

#include "ot_hud.h"
#include "hud.h"

#include "cl_util.h"
#include "parsemsg.h"
#include "usercmd.h"
#include "vgui_TeamFortressViewport.h"
#include "snd_system.h"

#include <string.h>
#include <stdio.h>

hudelement_t ICON_IVAN_GREEN = { 0, 0, 38, 33 };
hudelement_t ICON_IVAN_YELLOW = { 39, 0, 77, 33 };
hudelement_t ICON_IVAN_RED = { 78, 0, 116, 33 };
hudelement_t ICON_IVAN_DARK = { 117, 0,155, 33 };

hudelement_t BAR_LONG_BRIGHT = { 0, 34, 132, 48 };
hudelement_t BAR_LONG_DARK = { 0, 49, 132, 64 };

hudelement_t BAR_MEDIUM_BRIGHT = { 0, 64, 110, 77 };
hudelement_t BAR_MEDIUM_DARK = { 0, 79, 110, 93 };

hudelement_t BAR_SMALL_BRIGHT = { 0, 94, 77, 105 };
hudelement_t BAR_SMALL_DARK = { 0, 106, 77, 117 };

hudelement_t MV_ICON_BRIGHT_STAND = { 156, 0, 188, 32 };
hudelement_t MV_ICON_BRIGHT_RUN = { 188, 0, 220, 32 };
hudelement_t MV_ICON_BRIGHT_CROUCH = { 156, 32, 188, 64 };
hudelement_t MV_ICON_BRIGHT_JUMP = { 188, 32, 220, 64 };

hudelement_t MV_ICON_DARK_STAND = { 221, 0, 253, 32 };
hudelement_t MV_ICON_DARK_RUN = { 253, 0, 285, 32 };
hudelement_t MV_ICON_DARK_CROUCH = { 221, 32, 253, 64 };
hudelement_t MV_ICON_DARK_JUMP = { 253, 32, 285, 64 };

hudelement_t IT_ICON_BRIGHT_LONGJUMP = { 1, 121, 16, 144 };
hudelement_t IT_ICON_BRIGHT_BATTERY = { 20, 120, 37, 144 };
hudelement_t IT_ICON_BRIGHT_HEALTHKIT = { 43, 121, 66, 144 };

hudelement_t IT_ICON_DARK_LONGJUMP = { 68, 121, 83, 144 };
hudelement_t IT_ICON_DARK_BATTERY = { 87, 120, 104, 144 };
hudelement_t IT_ICON_DARK_HEALTHKIT = { 110, 121, 133, 144 };

hudelement_t DIGIT_X = { 112, 96, 116, 100 };
hudelement_t DIGIT_0 = { 117, 93, 122, 100 };
hudelement_t DIGIT_1 = { 123, 93, 128, 100 };
hudelement_t DIGIT_2 = { 128, 93, 133, 100 };
hudelement_t DIGIT_3 = { 134, 93, 139, 100 };
hudelement_t DIGIT_4 = { 140, 93, 146, 100 };
hudelement_t DIGIT_5 = { 147, 93, 152, 100 };

hudelement_t MAIN_WEAPON_BAR = { 6, 5, 506, 39 };
hudelement_t MAIN_HEALTH_BAR = { 15, 54, 218, 136 };
hudelement_t MAIN_AMMO_BAR = { 15, 160, 180, 230 };
hudelement_t MAIN_STOCK_BAR = { 245, 54, 300, 185 };
hudelement_t MAIN_STATUS_BAR = { 313, 54, 365, 110 };

hudelement_t pWeapons [] = { 
	{ 156, 65, 239, 76 }, { 156, 78, 239, 89 }, { 156, 91, 239, 102 }, { 156, 104, 239, 115 }, { 156, 117, 239, 128 }, { 156, 130, 239, 141 }, { 156, 143, 239, 154 }, { 156, 156, 239, 167 }, { 156, 169, 239, 180 }, { 156, 182, 239, 193 },
	{ 242, 65, 325, 76 }, { 242, 78, 325, 89 }, { 242, 91, 325, 102 }, { 242, 104, 325, 115 }, { 242, 117, 325, 128 }, { 242, 130, 325, 141 }, { 242, 143, 325, 154 }, { 242, 156, 325, 167 }, { 242, 169, 325, 180 }
};

int gSlotOffsets [] = { 9, 107, 207, 307, 405 };

extern unsigned short ByteToUShort( byte *byte );

int __MsgFunc_Health(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_Health(pszName, iSize, pbuf);
}

int __MsgFunc_Battery(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_Battery(pszName, iSize, pbuf);
}

int __MsgFunc_Batteries(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_Batteries(pszName, iSize, pbuf);
}

int __MsgFunc_Medkit(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_Medkit(pszName, iSize, pbuf);
}

int __MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_WeaponList(pszName, iSize, pbuf);
}

int __MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_CurWeapon(pszName, iSize, pbuf);
}

int __MsgFunc_HideWeapon(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_HideWeapon(pszName, iSize, pbuf);
}

int __MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_AmmoX(pszName, iSize, pbuf);
}

int __MsgFunc_LongJump(const char *pszName, int iSize, void *pbuf )
{
	return gOverturnHUD.MsgFunc_LongJump(pszName, iSize, pbuf);
}

void __CmdFunc_Slot1( void )
{
	gOverturnHUD.UserCmd_Slot1( );
}

void __CmdFunc_Slot2( void )
{
	gOverturnHUD.UserCmd_Slot2( );
}

void __CmdFunc_Slot3( void )
{
	gOverturnHUD.UserCmd_Slot3( );
}

void __CmdFunc_Slot4( void )
{
	gOverturnHUD.UserCmd_Slot4( );
}

void __CmdFunc_Slot5( void )
{
	gOverturnHUD.UserCmd_Slot5( );
}

void __CmdFunc_Slot6( void )
{
	gOverturnHUD.UserCmd_Slot6( );
}

void __CmdFunc_Slot7( void )
{
	gOverturnHUD.UserCmd_Slot7( );
}
void __CmdFunc_Slot8( void )
{
	gOverturnHUD.UserCmd_Slot8( );
}

void __CmdFunc_Slot9( void )
{
	gOverturnHUD.UserCmd_Slot9( );
}

void __CmdFunc_Slot10( void )
{
	gOverturnHUD.UserCmd_Slot10( );
}

void __CmdFunc_Close( void )
{
	gOverturnHUD.UserCmd_Close( );
}

void __CmdFunc_PrevWeapon( void )
{
	gOverturnHUD.UserCmd_PrevWeapon( );
}

void __CmdFunc_NextWeapon( void )
{
	gOverturnHUD.UserCmd_NextWeapon( );
}

/*
=========================
 COverturnHUD :: Init

=========================
*/
void COverturnHUD::Init( void )
{
	HOOK_MESSAGE( Battery );
	HOOK_MESSAGE( Health );
	HOOK_MESSAGE( Medkit );
	HOOK_MESSAGE( Batteries );
	HOOK_MESSAGE( WeaponList );
	HOOK_MESSAGE( CurWeapon );
	HOOK_MESSAGE( AmmoX );
	HOOK_MESSAGE( HideWeapon );
	HOOK_MESSAGE( LongJump );

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);

	m_iHugeNumber = 100000;
	m_pHUDTexture1 = LoadTexture("gfx/hud/hud_main1.tga");
	m_pHUDTexture2 = LoadTexture("gfx/hud/hud_main2.tga");

	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
}

/*
=========================
 COverturnHUD :: VidInit

=========================
*/
void COverturnHUD::VidInit( void )
{
	// Clear weapon info
	memset(m_pWeapons, 0, sizeof(m_pWeapons));
	memset(m_pSlots, 0, sizeof(m_pSlots));
	memset(m_iAmmo, 0, sizeof(m_iAmmo));

	// Clear stuff
	m_iOldWeaponBits = NULL;
	m_iWeaponBits = NULL;
	m_pWeapon = NULL;

	// Reset
	gHUD.m_iHideHUDDisplay = 0;
}

/*
=========================
 COverturnHUD :: Shutdown

=========================
*/
void COverturnHUD::Shutdown( void )
{
	glDeleteTextures(1, &m_pHUDTexture1->iIndex);
	delete [] m_pHUDTexture1;

	glDeleteTextures(1, &m_pHUDTexture2->iIndex);
	delete [] m_pHUDTexture2;
}

/*
=========================
 COverturnHUD :: Think

=========================
*/
void COverturnHUD::Think( void )
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != m_iOldWeaponBits )
	{
		m_iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
					PickupWeapon( p );
				else
					DropWeapon( p );
			}
		}
	}

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) || gHUD.m_iHideHUDDisplay)
		return;

	if (!m_pActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (m_pActiveSel != (WEAPON *)1)
		{
			ServerCmd(m_pActiveSel->szName);
			m_iWeaponSelect = m_pActiveSel->iId;
		}

		m_pLastSel = m_pActiveSel;
		m_pActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}
}

/*
=========================
 COverturnHUD :: Draw

=========================
*/
void COverturnHUD::Draw( void )
{
	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) || gHUD.m_iHideHUDDisplay)
		return;

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return;

	// make sure this is disabled
	glDisable(GL_DEPTH_TEST);

	// Set up basic texturing
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Use alpha blend instead of alpha test
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render main HUD elements
	glBindTexture(GL_TEXTURE_2D, m_pHUDTexture2->iIndex);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	glOrtho(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO, 0.1, 100);

	// Draw main HUD elements
	DrawElement( m_pHUDTexture2, 10, 250, MAIN_STOCK_BAR );
	DrawElement( m_pHUDTexture2, 10, 390, MAIN_HEALTH_BAR );
	DrawElement( m_pHUDTexture2, 580, 52, MAIN_STATUS_BAR );
	DrawElement( m_pHUDTexture2, 460, 390, MAIN_AMMO_BAR );

	if(m_pActiveSel)
		DrawElement( m_pHUDTexture2, 70, 10, MAIN_WEAPON_BAR );

	// Draw secondary HUD elements
	glBindTexture(GL_TEXTURE_2D, m_pHUDTexture1->iIndex);

	// Draw status bar
	vec3_t vSimvel;
	VectorCopy(gHUD.m_pParams.simvel, vSimvel);

	if(m_iHealth)
	{
		if(vSimvel[2]) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_BRIGHT_JUMP, 1, 10, 12);
		else if(gHUD.m_pParams.cmd->buttons & IN_DUCK) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_BRIGHT_CROUCH, 1, 10, 12);
		else if(vSimvel.Length2D()) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_BRIGHT_RUN, 1, 10, 12);
		else DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_BRIGHT_STAND, 0.8, 14, 12);
	}
	else
	{
		if(vSimvel[2]) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_DARK_JUMP, 1, 10, 12);
		else if(gHUD.m_pParams.cmd->buttons & IN_DUCK) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_DARK_CROUCH, 1, 10, 12);
		else if(vSimvel.Length2D()) DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_DARK_RUN, 1, 10, 12);
		else DrawElement(m_pHUDTexture2, 580, 52, MV_ICON_DARK_STAND, 0.8, 14, 12);
	}

	// Draw health icon
	if(m_iHealth > 66) DrawElement(m_pHUDTexture2, 10, 390, ICON_IVAN_GREEN, 1, 12, 38);
	else if (m_iHealth > 33) DrawElement(m_pHUDTexture2, 10, 390, ICON_IVAN_YELLOW, 1, 12, 38);
	else if (m_iHealth > 0) DrawElement(m_pHUDTexture2, 10, 390, ICON_IVAN_RED, 1, 12, 38);
	else DrawElement(m_pHUDTexture2, 10, 390, ICON_IVAN_DARK, 1, 12, 38);

	// Draw health bar
	if(m_iHealth != 100) DrawElement(m_pHUDTexture2, 10, 390, BAR_LONG_DARK, 1, 54, 45);
	if(m_iHealth != 0) DrawElement(m_pHUDTexture2, 10, 390, BAR_LONG_BRIGHT, (double)m_iHealth/100.0f, 54, 45);

	// Draw power bar
	if(m_iBattery != 100) DrawElement(m_pHUDTexture2, 10, 390, BAR_SMALL_DARK, 1, 86, 26);
	if(m_iBattery != 0) DrawElement(m_pHUDTexture2, 10, 390, BAR_SMALL_BRIGHT, (double)m_iBattery/100.0f, 86, 26);

	// Draw the stock items
	if(m_iMedkits) DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_BRIGHT_HEALTHKIT, 1, 13, 13);
	else DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_DARK_HEALTHKIT, 1, 13, 13);

	// 10 250
	// Draw the numbers
	DrawElement(m_pHUDTexture2, 10, 250, DIGIT_X, 1, 34, 32);
	switch(m_iMedkits)
	{
		case 0: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_0, 1, 39, 29); break;
		case 1: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_1, 1, 39, 29); break;
		case 2: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_2, 1, 39, 29); break;
		case 3: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_3, 1, 39, 29); break;
		case 4: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_4, 1, 39, 29); break;
		case 5: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_5, 1, 39, 29); break;
	}

	// Draw batteries
	if(m_iBatteries) DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_BRIGHT_BATTERY, 1, 15, 39);
	else DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_DARK_BATTERY, 1, 15, 39);

	// Draw the numbers
	DrawElement(m_pHUDTexture2, 10, 250, DIGIT_X, 1, 34, 59);
	switch(m_iBatteries)
	{
		case 0: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_0, 1, 39, 56); break;
		case 1: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_1, 1, 39, 56); break;
		case 2: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_2, 1, 39, 56); break;
		case 3: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_3, 1, 39, 56); break;
		case 4: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_4, 1, 39, 56); break;
		case 5: DrawElement(m_pHUDTexture2, 10, 250, DIGIT_5, 1, 39, 56); break;
	}

	// Draw longjump icon
	if(m_bLongJump) DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_BRIGHT_LONGJUMP, 1, 16, 66);
	else DrawElement(m_pHUDTexture2, 10, 250, IT_ICON_DARK_LONGJUMP, 1, 16, 66);

	// Draw current weapon info
	if(m_pWeapon)
	{
		// Render the weapon icon
		DrawElement(m_pHUDTexture2, 460, 390, pWeapons[m_pWeapon->iId], 1, 40, 47);

		// Draw primary ammo bar
		int iAmmoCount = CountAmmo(m_pWeapon->iAmmoType);
		if(iAmmoCount != m_pWeapon->iMax1) DrawElement(m_pHUDTexture2, 460, 390, BAR_MEDIUM_DARK, 1, 17, 24);
		if(iAmmoCount != 0) DrawElement(m_pHUDTexture2, 460, 390, BAR_MEDIUM_BRIGHT, (float)iAmmoCount/(float)m_pWeapon->iMax1, 17, 24);

		if(m_pWeapon->iAmmo2Type != -1)
		{
			int iCount = CountAmmo(m_pWeapon->iAmmo2Type);
			DrawElement(m_pHUDTexture2, 460, 390, DIGIT_X, 1, 135, 31);
			switch(iCount)
			{
				case 0: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_0, 1, 140, 28); break;
				case 1: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_1, 1, 140, 28); break;
				case 2: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_2, 1, 140, 28); break;
				case 3: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_3, 1, 140, 28); break;
				case 4: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_4, 1, 140, 28); break;
				case 5: DrawElement(m_pHUDTexture2, 460, 390, DIGIT_5, 1, 140, 28); break;
			}
		}
	}

	// Render the HUD selection screen
	if(m_pActiveSel)
	{
		// Draw every weapon in every slot
		for(int i = 0; i < MAX_WEAPON_SLOTS; i++)
		{
			int yoffset = 0;
			for(int j = 0; j < MAX_WEAPON_POSITIONS; j++)
			{
				if(!m_pSlots[i][j])
					continue;

				// Draw main icon
				DrawElement(m_pHUDTexture2, 70, 10, pWeapons[m_pSlots[i][j]->iId], 1, gSlotOffsets[i], 16+yoffset);

				// Draw color
				if(m_pActiveSel == m_pSlots[i][j] || !HasAmmo(m_pSlots[i][j]) )
				{
					glDisable(GL_TEXTURE_2D);
					if(m_pActiveSel == m_pSlots[i][j])
					{
						float alpha = abs(sin(gEngfuncs.GetClientTime()*3))*0.5;
						glColor4f(0, 0.5, 0, alpha);
					}
					else
					{
						glColor4f(1, 0, 0, 0.5);
					}
					DrawElement(m_pHUDTexture2, 70, 10, pWeapons[m_pSlots[i][j]->iId], 1, gSlotOffsets[i], 16+yoffset);
					glEnable(GL_TEXTURE_2D);
				}

				yoffset += pWeapons[m_pSlots[i][j]->iId].yendoffset-pWeapons[m_pSlots[i][j]->iId].yoffset;
			}
		}
	}

	// restore us
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_BLEND);
}

/*
=========================
 COverturnHUD :: DrawElement

=========================
*/
void COverturnHUD::DrawElement( cl_texture_t *ptex, int coordx, int coordy, hudelement_t pelement, double xfrac, int internx, int interny )
{
	double sizex = pelement.xendoffset-pelement.xoffset;
	double sizey = pelement.yendoffset-pelement.yoffset;

	// scale elements by the height ratio
	double sizemodx = (float)gHUD.m_scrinfo.iWidth/640.0f;
	double sizemody = (float)gHUD.m_scrinfo.iHeight/480.0f;

	double fsizex = sizex*sizemody;
	double fsizey = sizey*sizemody;

	double offsety = (coordy*sizemody)/(double)gHUD.m_scrinfo.iHeight+((interny*sizemody)/(double)gHUD.m_scrinfo.iHeight);
	double offsetx = (640.0f-(coordx+sizex))*sizemodx;
	offsetx = gHUD.m_scrinfo.iWidth-offsetx-(sizex*sizemodx);
	offsetx = offsetx + internx*sizemody;
	offsetx /= (double)gHUD.m_scrinfo.iWidth;

	fsizex /= (double)gHUD.m_scrinfo.iWidth;
	fsizey /= (double)gHUD.m_scrinfo.iHeight;

	double ftexcxsize = (double)pelement.xendoffset-(double)pelement.xoffset;
	double xendoffset = (double)pelement.xoffset+ftexcxsize*xfrac;

	glBegin(GL_TRIANGLES);
	glTexCoord2d(pelement.xoffset/(float)ptex->iWidth, pelement.yoffset/(float)ptex->iHeight);
	glVertex3d(offsetx, offsety, -1);
	glTexCoord2d(xendoffset/(float)ptex->iWidth, pelement.yoffset/(float)ptex->iHeight);
	glVertex3d(offsetx+fsizex*xfrac, offsety, -1);
	glTexCoord2d(pelement.xoffset/(float)ptex->iWidth, pelement.yendoffset/(float)ptex->iHeight);
	glVertex3d(offsetx, offsety+fsizey, -1);
	glTexCoord2d(xendoffset/(float)ptex->iWidth, pelement.yoffset/(float)ptex->iHeight);
	glVertex3d(offsetx+fsizex*xfrac, offsety, -1);
	glTexCoord2d(xendoffset/(float)ptex->iWidth, pelement.yendoffset/(float)ptex->iHeight);
	glVertex3d(offsetx+fsizex*xfrac, offsety+fsizey, -1);
	glTexCoord2d(pelement.xoffset/(float)ptex->iWidth, pelement.yendoffset/(float)ptex->iHeight);
	glVertex3d(offsetx, offsety+fsizey, -1);
	glEnd();
}

/*
=========================
 COverturnHUD :: MsgFunc_Health

=========================
*/
int COverturnHUD::MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iHealth = READ_BYTE();
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_Battery

=========================
*/
int COverturnHUD::MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iBattery = READ_SHORT();
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_Medkit

=========================
*/
int COverturnHUD::MsgFunc_Medkit( const char*pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iMedkits = READ_BYTE();
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_Batteries

=========================
*/
int COverturnHUD::MsgFunc_Batteries( const char*pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iBatteries = READ_BYTE();
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_WeaponList

=========================
*/
int COverturnHUD::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	WEAPON pWeapon;
	strcpy( pWeapon.szName, READ_STRING() );
	pWeapon.iAmmoType = (int)READ_CHAR();	
	
	pWeapon.iMax1 = READ_BYTE();
	if (pWeapon.iMax1 == 255) 
		pWeapon.iMax1 = -1;

	pWeapon.iAmmo2Type = READ_CHAR();
	pWeapon.iMax2 = READ_BYTE();
	if (pWeapon.iMax2 == 255) 
		pWeapon.iMax2 = -1;

	pWeapon.iSlot = READ_CHAR();
	pWeapon.iSlotPos = READ_CHAR();
	pWeapon.iId = READ_CHAR();
	pWeapon.iFlags = READ_BYTE();
	pWeapon.iClip = 0;

	memcpy(&m_pWeapons[pWeapon.iId], &pWeapon, sizeof(WEAPON));
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_CurWeapon

=========================
*/
int COverturnHUD::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	if ( iId < 1 )
	{
		m_pWeapon = NULL; //LRC
		return 0;
	}

	WEAPON *pWeapon = &m_pWeapons[iId];

	if ( iClip < -1 ) pWeapon->iClip = abs(iClip);
	else pWeapon->iClip = iClip;

	if ( iState == 0 )
		return 1;

	m_pWeapon = pWeapon;
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_AmmoX

=========================
*/
int COverturnHUD::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	m_iAmmo[iIndex] = iCount;
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_HideWeapon

=========================
*/
int COverturnHUD::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	gHUD.m_iHideHUDDisplay = READ_BYTE();
	return 1;
}

/*
=========================
 COverturnHUD :: MsgFunc_LongJump

=========================
*/
int COverturnHUD::MsgFunc_LongJump( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_bLongJump = READ_BYTE() ? true:false;
	return 1;
}

/*
=========================
 COverturnHUD :: LoadTexture

=========================
*/
cl_texture_t *COverturnHUD::LoadTexture( char *path )
{
	byte *pFile = gEngfuncs.COM_LoadFile(path, 5, NULL);
	if(!pFile)
	{
		gEngfuncs.Con_Printf("Error! Failed to load: %s.\n", path);
		return NULL;
	}

	// Set basic information
	tga_header_t *pHeader = (tga_header_t *)pFile;
	if(pHeader->datatypecode != 2 && pHeader->datatypecode != 10
		|| pHeader->bitsperpixel != 24 && pHeader->bitsperpixel != 32)
	{
		gEngfuncs.Con_Printf("Error! %s is using a non-supported format. Only 24 bit and 32 bit true color formats are supported.\n", path);
		return false;
	}

	cl_texture_t *pTexture = new cl_texture_t;
	memset(pTexture, 0, sizeof(cl_texture_t));

	pTexture->iWidth = ByteToUShort(pHeader->width);
	pTexture->iHeight = ByteToUShort(pHeader->height);

	// Allocate data
	pTexture->iBpp = pHeader->bitsperpixel/8;
	int iSize = pTexture->iWidth*pTexture->iHeight;
	int iImageSize = iSize*pTexture->iBpp;
	int iInternalSize = iSize*4;

	byte *pOriginal = new byte[iInternalSize];
	memset(pOriginal, 0, sizeof(byte)*iInternalSize);

	// Load based on type
	byte *pCurrent = pFile+18;
	if(pHeader->datatypecode == 2)
	{
		//Uncompressed TGA
		if(pTexture->iBpp == 3)
		{
			for(int i = 0, j = 0; i < iImageSize; i += 3, j += 4)
			{
				pOriginal[j] = pCurrent[i+2];
				pOriginal[j+1] = pCurrent[i+1];
				pOriginal[j+2] = pCurrent[i];
				pOriginal[j+3] = 255;
			}
		}
		else if(pTexture->iBpp == 4)
		{
			for(int i = 0; i < iImageSize; i += 4)
			{
				pOriginal[i] = pCurrent[i+2];
				pOriginal[i+1] = pCurrent[i+1];
				pOriginal[i+2] = pCurrent[i];
				pOriginal[i+3] = pCurrent[i+3];
			}
		}
	}
	else
	{
		// RLE Compression
		int i = 0, k = 0;
		if(pTexture->iBpp == 3)
		{
			while(i < iImageSize)
			{
				if(*pCurrent & 0x80)
				{
					byte bLength = *pCurrent-127;
					pCurrent++;

					for(int j = 0; j < bLength; j++, i+= pTexture->iBpp, k+= 4)
					{
						pOriginal[k] = pCurrent[2];
						pOriginal[k+1] = pCurrent[1];
						pOriginal[k+2] = pCurrent[0];
						pOriginal[k+3] = 255;
					}
					
					pCurrent += pTexture->iBpp;
				}
				else
				{
					byte bLength = *pCurrent+1;
					pCurrent++;

					for(int j = 0; j < bLength; j++, i+= pTexture->iBpp, pCurrent += pTexture->iBpp, k += 4)
					{
						pOriginal[k] = pCurrent[2];
						pOriginal[k+1] = pCurrent[1];
						pOriginal[k+2] = pCurrent[0];
						pOriginal[k+3] = 255;
					}
				}
			}
		}
		else
		{
			while(i < iImageSize)
			{
				if(*pCurrent & 0x80)
				{
					byte bLength = *pCurrent-127;
					pCurrent++;

					for(int j = 0; j < bLength; j++, i+= pTexture->iBpp)
					{
						pOriginal[k] = pCurrent[2];
						pOriginal[i+1] = pCurrent[1];
						pOriginal[i+2] = pCurrent[0];
						pOriginal[i+3] = pCurrent[3];
					}
					
					pCurrent += pTexture->iBpp;
				}
				else
				{
					byte bLength = *pCurrent+1;
					pCurrent++;

					for(int j = 0; j < bLength; j++, i+= pTexture->iBpp, pCurrent += pTexture->iBpp)
					{
						pOriginal[i] = pCurrent[2];
						pOriginal[i+1] = pCurrent[1];
						pOriginal[i+2] = pCurrent[0];
						pOriginal[i+3] = pCurrent[3];
					}
				}
			}
		}
	}

	// Flip vertically
	byte *pFlipped = new byte[iInternalSize];
	for(int i = 0; i < (int)pTexture->iHeight; i++)
	{
		GLubyte *dst = pFlipped + i*pTexture->iWidth*4;
		GLubyte *src = pOriginal + (pTexture->iHeight-i-1)*pTexture->iWidth*4;
		memcpy(dst, src, sizeof(GLubyte)*pTexture->iWidth*4);
	}

	pTexture->iIndex = m_iHugeNumber;
	m_iHugeNumber++;

	glBindTexture(GL_TEXTURE_2D, pTexture->iIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, pTexture->iWidth, pTexture->iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pFlipped );

	return pTexture;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------

/*
=========================
 COverturnHUD :: DrawElement

=========================
*/
void COverturnHUD::SlotInput( int iSlot )
{
	// Let the Viewport use it first, for menus
	if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;

	SelectSlot(iSlot, FALSE, 1);
}

/*
=========================
 COverturnHUD :: UserCmd_Slot1

=========================
*/
void COverturnHUD::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot2

=========================
*/
void COverturnHUD::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot3

=========================
*/
void COverturnHUD::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot4

=========================
*/
void COverturnHUD::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot5

=========================
*/
void COverturnHUD::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot6

=========================
*/
void COverturnHUD::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot7

=========================
*/
void COverturnHUD::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot8

=========================
*/
void COverturnHUD::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot9

=========================
*/
void COverturnHUD::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

/*
=========================
 COverturnHUD :: UserCmd_Slot10

=========================
*/
void COverturnHUD::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

/*
=========================
 COverturnHUD :: UserCmd_Close

=========================
*/
void COverturnHUD::UserCmd_Close(void)
{
	if (m_pActiveSel)
	{
		m_pLastSel = m_pActiveSel;
		m_pActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		ClientCmd("escape");
}

/*
=========================
 COverturnHUD :: UserCmd_NextWeapon

=========================
*/
void COverturnHUD::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !m_pActiveSel || m_pActiveSel == (WEAPON*)1 )
		m_pActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( m_pActiveSel )
	{
		pos = m_pActiveSel->iSlotPos + 1;
		slot = m_pActiveSel->iSlot;
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = GetWeaponSlot( slot, pos );

				if ( wsp && HasAmmo(wsp) )
				{
					m_pActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	m_pActiveSel = NULL;
}

/*
=========================
 COverturnHUD :: UserCmd_PrevWeapon

=========================
*/
void COverturnHUD::UserCmd_PrevWeapon( void )
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !m_pActiveSel || m_pActiveSel == (WEAPON*)1 )
		m_pActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( m_pActiveSel )
	{
		pos = m_pActiveSel->iSlotPos - 1;
		slot = m_pActiveSel->iSlot;
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = GetWeaponSlot( slot, pos );

				if ( wsp && HasAmmo(wsp) )
				{
					m_pActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	m_pActiveSel = NULL;
}

/*
=========================
 COverturnHUD :: SelectSlot

=========================
*/
void COverturnHUD :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return;

	if ( ! ( gHUD.m_iWeaponBits & ~(1<<(WEAPON_SUIT)) ))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( (m_pActiveSel == NULL) || (m_pActiveSel == (WEAPON *)1) || (iSlot != m_pActiveSel->iSlot) )
	{
		PlaySound("common/wpn_select.wav", 1);
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				m_iWeaponSelect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( m_pActiveSel )
			p = GetNextActivePos( m_pActiveSel->iSlot, m_pActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			m_pActiveSel = (WEAPON *)1;
		else
			m_pActiveSel = NULL;
	}
	else 
		m_pActiveSel = p;
}

/*
=========================
 COverturnHUD :: CountAmmo

=========================
*/
int COverturnHUD :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return m_iAmmo[iId];
}

/*
=========================
 COverturnHUD :: HasAmmo

=========================
*/
int COverturnHUD :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}

/*
=========================
 COverturnHUD :: HasAmmo

=========================
*/
WEAPON *COverturnHUD :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( m_pSlots[iSlot][i] && HasAmmo( m_pSlots[iSlot][i] ) )
		{
			pret = m_pSlots[iSlot][i];
			break;
		}
	}

	return pret;
}

/*
=========================
 COverturnHUD :: HasAmmo

=========================
*/
WEAPON* COverturnHUD :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = m_pSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}