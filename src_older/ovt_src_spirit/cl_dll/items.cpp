//========= Copyright © 2011, Half-Screwed Team, Released under the "Do whatever you want" license. ============//
//																									  //
// Purpose:																							  //
//																									  //
// $NoKeywords: $																					  //
//====================================================================================================//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Items, Items)
DECLARE_MESSAGE(m_Items, Medkit);
DECLARE_MESSAGE(m_Items, Batteries);

int CHudItems::Init(void)
{
	HOOK_MESSAGE( Items ); 
	HOOK_MESSAGE( Medkit );
	HOOK_MESSAGE( Batteries );

	medkits=batteries=0;//RESET

	gHUD.AddHudElem(this); 
	return 1; 
};

int CHudItems::VidInit(void)
{
	m_HUD_items = gHUD.GetSpriteIndex( "keyitemsbar" );
	m_HUD_rcard = gHUD.GetSpriteIndex( "rcard" );
	m_HUD_bcard = gHUD.GetSpriteIndex( "bcard" );
	m_HUD_ycard = gHUD.GetSpriteIndex( "ycard" );
	m_HUD_healthkit = gHUD.GetSpriteIndex( "medkit" );
	m_HUD_healthkitx = gHUD.GetSpriteIndex( "medkitx" );
	m_HUD_health1 = gHUD.GetSpriteIndex( "medkit1" );
	m_HUD_health2 = gHUD.GetSpriteIndex( "medkit2" );
	m_HUD_health3 = gHUD.GetSpriteIndex( "medkit3" );
	m_HUD_health4 = gHUD.GetSpriteIndex( "medkit4" );
	m_HUD_health5 = gHUD.GetSpriteIndex( "medkit5" );

	m_HUD_battery = gHUD.GetSpriteIndex( "batteryot" );


 	return 1;
};

int CHudItems::MsgFunc_Items( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_LONG();

	if (x == 1)
	{
		item_rcard = 1;	
	}
	else if (x == 2)
	{
		item_bcard = 1;
	}
	else if (x == 3)
	{
		item_ycard = 1;
	}

	m_iFlags |= HUD_ACTIVE;
 	return 1;
}

int CHudItems::MsgFunc_Medkit(const char*pszName, int iSize, void *pbuf)
{
   BEGIN_READ(pbuf,iSize);

   int x = READ_BYTE();

   medkits = x;

   return 1;
}

int CHudItems::MsgFunc_Batteries(const char*pszName, int iSize, void *pbuf)
{
   BEGIN_READ(pbuf,iSize);

   int x = READ_BYTE();

   batteries = x;

   return 1;
}



int CHudItems::Draw( float flTime )
{	
	int x,y;

	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
	return 1; 

	if (!(gHUD.m_iWeaponBits & (1 <<(WEAPON_SUIT)) ))
	return 1; 

	int itemsheight = gHUD.GetSpriteRect(m_HUD_items).bottom - gHUD.GetSpriteRect(m_HUD_items).top;
	int itemswidth = gHUD.GetSpriteRect(m_HUD_items).right - gHUD.GetSpriteRect(m_HUD_items).left;

	int cardheight = gHUD.GetSpriteRect(m_HUD_ycard).bottom - gHUD.GetSpriteRect(m_HUD_ycard).top;
	int cardwidth = (gHUD.GetSpriteRect(m_HUD_ycard).right - gHUD.GetSpriteRect(m_HUD_ycard).left)+5;

	x = ScreenWidth - itemswidth;
	y = ScreenHeight - itemsheight;

	SPR_Set(gHUD.GetSprite(m_HUD_items), 255, 255, 255);
	SPR_Draw(0, x, y, &gHUD.GetSpriteRect(m_HUD_items));

	if (item_rcard)
	{
		x = ScreenWidth - (cardwidth*3);
		y = ScreenHeight - (itemsheight+cardheight+5);

		SPR_Set(gHUD.GetSprite(m_HUD_rcard), 255, 255, 255);
		SPR_Draw(0, x, y, &gHUD.GetSpriteRect(m_HUD_rcard));
	}

	if (item_bcard)
	{
		x = ScreenWidth - (cardwidth*2);
		y = ScreenHeight - (itemsheight+cardheight+5);

		SPR_Set(gHUD.GetSprite(m_HUD_bcard), 255, 255, 255);
		SPR_Draw(0, x, y, &gHUD.GetSpriteRect(m_HUD_bcard));
	}

	if (item_ycard)
	{
		x = ScreenWidth - cardwidth;
		y = ScreenHeight - (itemsheight+cardheight+5);

		SPR_Set(gHUD.GetSprite(m_HUD_ycard), 255, 255, 255);
		SPR_Draw(0, x, y, &gHUD.GetSpriteRect(m_HUD_ycard));
	}

	/*MEDKITS*/
	int medheight = gHUD.GetSpriteRect(m_HUD_healthkit).bottom - gHUD.GetSpriteRect(m_HUD_healthkit).top;
	int medwidth = (gHUD.GetSpriteRect(m_HUD_healthkit).right - gHUD.GetSpriteRect(m_HUD_healthkit).left)+2;
	int medxwidth = gHUD.GetSpriteRect(m_HUD_healthkitx).right - gHUD.GetSpriteRect(m_HUD_healthkitx).left;
	if (medkits)
	{

		x = ScreenWidth - itemswidth - medwidth - (medxwidth*2);
		y = ScreenHeight - medheight;

		SPR_Set(gHUD.GetSprite(m_HUD_healthkit), 255, 255, 255);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_healthkit));

		x = x + medwidth;

		SPR_Set(gHUD.GetSprite(m_HUD_healthkitx), 255,255,255);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_healthkitx));

		x = x + medxwidth;

		if (medkits == 1)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health1), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health1));
		}
		else if (medkits == 2)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health2), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health2));
		}
		else if (medkits == 3)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health3), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health3));
		}
		else if (medkits == 4)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health4), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health4));
		}
		else if (medkits == 5)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health5), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health5));
		}
	}
	/*MEDKITS*/

	/*BATTERIES*/
	if (batteries)
	{
		int batheight = gHUD.GetSpriteRect(m_HUD_battery).bottom - gHUD.GetSpriteRect(m_HUD_battery).top;
		int batwidth = (gHUD.GetSpriteRect(m_HUD_battery).right - gHUD.GetSpriteRect(m_HUD_battery).left)+2;

		x = ScreenWidth - itemswidth - batwidth - (medxwidth*2);
		if (medkits)
			x = x - medwidth - (medxwidth*2) - medwidth - medxwidth*2;

		y = ScreenHeight - medheight;

		SPR_Set(gHUD.GetSprite(m_HUD_battery), 255, 255, 255);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_battery));

		x = x + batwidth;

		SPR_Set(gHUD.GetSprite(m_HUD_healthkitx), 255,255,255);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_healthkitx));

		x = x + medxwidth;

		if (batteries == 1)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health1), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health1));
		}
		else if (batteries == 2)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health2), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health2));
		}
		else if (batteries == 3)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health3), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health3));
		}
		else if (batteries == 4)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health4), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health4));
		}
		else if (batteries == 5)
		{
			SPR_Set(gHUD.GetSprite(m_HUD_health5), 255,255,255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_health5));
		}
	}
	/*BATTERIES*/

	return 1;
}