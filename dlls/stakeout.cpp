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
#include "gamerules.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.06105, 0.02618, 0.00  )// 7 degrees by 3 degrees
#define VECTOR_CONE_DM_SEMISHOTGUN Vector( 0.08716, 0.04362, 0.00 ) // 10 degrees by 5 degrees

enum stakeout_e {
	STAKEOUT_IDLE1 = 0,
	STAKEOUT_IDLE2,
	STAKEOUT_FIRE,
	STAKEOUT_FIRE_SEMI,
	STAKEOUT_DEPLOY,
	STAKEOUT_DEPLOY_FIRST
};

LINK_ENTITY_TO_CLASS( weapon_stakeout, CStakeout );

void CStakeout::Spawn( )
{
	Precache( );
	m_iId = WEAPON_STAKEOUT;
	SET_MODEL(ENT(pev), "models/w_stakeout.mdl");

	m_iDefaultAmmo = STAKEOUT_DEFAULT_GIVE;
	ResetSequenceInfo();

	FallInit();// get ready to fall
}


void CStakeout::Precache( void )
{
	PRECACHE_MODEL("models/v_stakeout.mdl");
	PRECACHE_MODEL("models/w_stakeout.mdl");
	PRECACHE_MODEL("models/p_stakeout.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/ammopickup2.wav");              

	PRECACHE_SOUND ("weapons/stakeout_fire1.wav");//shotgun
	PRECACHE_SOUND ("weapons/stakeout_fire2.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload

//	PRECACHE_SOUND ("weapons/sshell1.wav");	// shotgun reload - played on client
//	PRECACHE_SOUND ("weapons/sshell3.wav");	// shotgun reload - played on client
	
	PRECACHE_SOUND ("weapons/dryfire1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/stakeout_pump.wav");	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/sshotgun1.sc" );
	m_usSemiFire = PRECACHE_EVENT( 1, "events/sshotgun2.sc" );
}

int CStakeout::AddToPlayer( CBasePlayer *pPlayer )
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


int CStakeout::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "20buckshot";
	p->iMaxAmmo1 = _20BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_STAKEOUT;
	p->iWeight = STAKEOUT_WEIGHT;

	return 1;
}



BOOL CStakeout::Deploy( )
{
	return DefaultDeploy( "models/v_stakeout.mdl", "models/p_stakeout.mdl", STAKEOUT_DEPLOY_FIRST, "stakeout" );
}

void CStakeout::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	int flags;
	flags = 0;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	if ( g_pGameRules->IsMultiplayer() )
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_20BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// regular old, untouched spread. 
		vecDir = m_pPlayer->FireBulletsPlayer( 8, vecSrc, vecAiming, VECTOR_CONE_7DEGREES, 2048, BULLET_PLAYER_20BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	UTIL_ScreenShake( m_pPlayer->pev->origin, 8.0, 80.0, 0.25, 64 );

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;

}


void CStakeout::SecondaryAttack( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			return;
		}

		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;


		int flags;
		flags = 0;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector vecSrc	 = m_pPlayer->GetGunPosition( );
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

		Vector vecDir;
		
		if ( g_pGameRules->IsMultiplayer() )
		{
			// tuned for deathmatch
			vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_DM_SEMISHOTGUN, 2048, BULLET_PLAYER_20BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else
		{
			// untouched default single player
			vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_20BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
			
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSemiFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

UTIL_ScreenShake( m_pPlayer->pev->origin, 8.0, 80.0, 0.2, 64 );

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
}


void CStakeout::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/stakeout_pump.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.95)
			{
				iAnim = STAKEOUT_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
			}
			else
			{
				iAnim = STAKEOUT_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
			}
			SendWeaponAnim( iAnim );
	}
}



class CStakeoutAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shotshell.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shotshell.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_20BUCKSHOTBOX_GIVE, "20buckshot", _20BUCKSHOT_MAX_CARRY ) != -1)
		{
			EMIT_AMBIENT_SOUND(ENT(pOther->pev), pev->origin, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_20buckshot, CStakeoutAmmo );


