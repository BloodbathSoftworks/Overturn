
//
// healthkit.cpp
//
// implementation of CHudHealthkit class
//
// Coded By: ash_link

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Medkit, Medkit);

int CHudHealthkit::Init(void)
{
   HOOK_MESSAGE(Medkit);

   gHUD.AddHudElem(this);

   return 1;
}

int CHudHealthkit::VidInit(void)
{

   return 1;
}

int CHudHealthkit::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
	return 1; 

	if (!(gHUD.m_iWeaponBits & (1 <<(WEAPON_SUIT)) ))
	return 1; 

   return 1;
}

