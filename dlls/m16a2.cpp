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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum m16a2_e
{
	M16A2_LONGIDLE = 0,
	M16A2_IDLE1,
	M16A2_DEPLOY,
	M16A2_FIRE1,
	M16A2_FIRE2,
	M16A2_FIRE3,
};



LINK_ENTITY_TO_CLASS( weapon_m16a2, CM16A2 );


//=========================================================
//=========================================================
void CM16A2::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m16a2"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_m16.mdl");
	m_iId = WEAPON_M16A2;

	m_iDefaultAmmo = M16A2_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CM16A2::Precache( void )
{
	PRECACHE_MODEL("models/v_m16.mdl");
	PRECACHE_MODEL("models/w_m16.mdl");
	PRECACHE_MODEL("models/p_m16.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_m16clip.mdl");
	PRECACHE_SOUND("items/ammopickup1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/colts1.wav");// H to the K
	PRECACHE_SOUND ("weapons/colts2.wav");// H to the K
	PRECACHE_SOUND ("weapons/colts3.wav");// H to the K

	PRECACHE_SOUND ("weapons/dryfire1.wav");

	m_usM16 = PRECACHE_EVENT( 1, "events/m16.sc" );
	m_usM162 = PRECACHE_EVENT( 1, "events/m162.sc" );
}

int CM16A2::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "10mm";
	p->iMaxAmmo1 = _10MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 0;//3
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M16A2;
	p->iWeight = M16A2_WEIGHT;

	return 1;
}

int CM16A2::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CM16A2::Deploy( )
{
	m_fInSpecialReload = FALSE;
	return DefaultDeploy( "models/v_m16.mdl", "models/p_m16.mdl", M16A2_DEPLOY, "mp5" );
}


void CM16A2::PrimaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 1 )
	{
		m_fInSpecialReload = 0;
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		m_fInSpecialReload = 0;
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	if ( !g_pGameRules->IsMultiplayer() )
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
	flags = 0;

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM16, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	UTIL_ScreenShake( m_pPlayer->pev->origin, 8.0, 80.0, 0.2, 64 );

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

}


void CM16A2::SecondaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 1 )
	{
		m_fInSpecialReload = 0;
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		m_fInSpecialReload = 0;
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	if ( !g_pGameRules->IsMultiplayer() )
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
	flags = 0;

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM16, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	UTIL_ScreenShake( m_pPlayer->pev->origin, 8.0, 80.0, 0.2, 64 );

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

		if(m_fInSpecialReload == 0)
		{
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.08;
			m_fInSpecialReload = 2;
		}
		else if(m_fInSpecialReload == 2)
		{
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.08;
			m_fInSpecialReload = 1;
		}
		else if(m_fInSpecialReload == 1)
		{
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.5;
			m_fInSpecialReload = 0;
		}
}

void CM16A2::ItemPostFrame( void )
{
//	if(m_pPlayer->m_pNextItem)
//		return;

	if(!m_iClip)
		m_fInSpecialReload = 0;

	if(m_fInSpecialReload)
	{
		if(m_flNextPrimaryAttack <= gpGlobals->time)
			SecondaryAttack();

		return;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CM16A2::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M16A2_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = M16A2_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
}


class CM16A2AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m16clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m16clip.mdl");
		PRECACHE_SOUND("items/ammopickup1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M16CLIP_GIVE, "10mm", _10MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pOther->pev), CHAN_ITEM, "items/ammopickup1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_m16clip, CM16A2AmmoClip );


