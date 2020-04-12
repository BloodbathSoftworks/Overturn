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

#ifndef OT_HUD_H
#define OT_HUD_H

#include "windows.h"
#include "gl/gl.h"
#include "glext.h"
#include "hud.h"

#define MAX_WEAPON_POSITIONS		MAX_WEAPON_SLOTS
#define MAX_WEAPON_NAME 128
#define WEAPON_FLAGS_SELECTONEMPTY	1
#define WEAPON_IS_ONTARGET 0x40

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;

	int		iCount;		// # of itesm in plist
};

typedef int AMMO;

struct tga_header_t
{
	byte	idlength;
	byte	colourmaptype;
	byte	datatypecode;
	byte	colourmaporigin[2]; //how come you have short ints there?
	byte	colourmaplength[2];
	byte	colourmapdepth;
	byte	x_origin[2];
	byte	y_origin[2];
	byte	width[2];
	byte	height[2];
	byte	bitsperpixel;
	byte	imagedescriptor;
};

struct hudelement_t
{
	int xoffset;
	int yoffset;
	int xendoffset;
	int yendoffset;
};

struct cl_texture_t
{
	char szName[64];

	GLuint iIndex;

	int iBpp;
	unsigned int iWidth;
	unsigned int iHeight;
};

/*
=================
 COverturnHUD

=================
*/
class COverturnHUD
{
public:
	void Init ( void );
	void VidInit( void );
	void Shutdown( void );

	void Draw( void );
	void Think( void );

	int MsgFunc_Health( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_Medkit( const char*pszName, int iSize, void *pbuf );
	int MsgFunc_Batteries( const char*pszName, int iSize, void *pbuf );
	int MsgFunc_WeaponList( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_CurWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoX( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_LongJump( const char *pszName, int iSize, void *pbuf );

	void UserCmd_Slot1( void );
	void UserCmd_Slot2( void );
	void UserCmd_Slot3( void );
	void UserCmd_Slot4( void );
	void UserCmd_Slot5( void );
	void UserCmd_Slot6( void );
	void UserCmd_Slot7( void );
	void UserCmd_Slot8( void );
	void UserCmd_Slot9( void );
	void UserCmd_Slot10( void );
	void UserCmd_Close( void );
	void UserCmd_NextWeapon( void );
	void UserCmd_PrevWeapon( void );

	void SlotInput( int iSlot );

private:
	void DrawElement( cl_texture_t *ptex, int coordx, int coordy, hudelement_t pelement, double xfrac = 1, int internx = 0, int interny = 0 );
	cl_texture_t *LoadTexture( char *path );

private:
	cl_texture_t *m_pHUDTexture1;
	cl_texture_t *m_pHUDTexture2;

	int m_iHugeNumber;

private:
	WEAPON *GetWeapon( int iId ) { return &m_pWeapons[iId]; }
	void AddWeapon( WEAPON *wp ) 
	{ 
		m_pWeapons[ wp->iId ] = *wp;
	}

	void PickupWeapon( WEAPON *wp )
	{
		m_pSlots[ wp->iSlot ][ wp->iSlotPos ] = wp;
	}

	void DropWeapon( WEAPON *wp )
	{
		m_pSlots[ wp->iSlot ][ wp->iSlotPos ] = NULL;
	}

	void DropAllWeapons( void )
	{
		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			if ( m_pWeapons[i].iId )
				DropWeapon( &m_pWeapons[i] );
		}
	}

	WEAPON* GetFirstPos( int iSlot );
	void SelectSlot( int iSlot, int fAdvance, int iDirection );

	WEAPON* GetNextActivePos( int iSlot, int iSlotPos );
	WEAPON* GetWeaponSlot( int slot, int pos ) { return m_pSlots[slot][pos]; }

	int HasAmmo( WEAPON *p );
	AMMO GetAmmo( int iId ) { return iId; }
	void SetAmmo( int iId, int iCount ) { m_iAmmo[ iId ] = iCount; }
	int CountAmmo( int iId );

public:
	int m_iHealth;

private:
	int m_iBattery;

	int m_iMedkits;
	int m_iBatteries;

	bool m_bLongJump;

	PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;

private:
	int			m_iWeaponBits;
	int			m_iOldWeaponBits;
public:
	// Information about weapons & ammo
	WEAPON		m_pWeapons[MAX_WEAPONS];	// Weapons Array
	WEAPON		*m_pWeapon;
private:
	// counts of weapons * ammo
	WEAPON*		m_pSlots[MAX_WEAPON_SLOTS+1][MAX_WEAPON_POSITIONS+1];	// The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's NULL, no weapon is there
	int			m_iAmmo[MAX_AMMO_TYPES]; // count of each ammo type

	WEAPON		*m_pActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
								// this points to the active weapon menu item
	WEAPON		*m_pLastSel;	// Last weapon menu selection 

public:
	int			m_iWeaponSelect;
};
extern COverturnHUD gOverturnHUD;
#endif