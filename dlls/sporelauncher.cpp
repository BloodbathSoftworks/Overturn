//========= Copyright © 2004-2008, Raven City Team, All rights reserved. ============//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

#ifdef PARTIFLE_FUSION

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "animation.h"
#include "monsters.h"
#include "weapons.h"
#include "decals.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "ofentities.h"

//=========================================================
// Opposing Forces Sporelauncher
//=========================================================
enum sporelauncher_e {
	SPORELAUNCHER_IDLE = 0,
	SPORELAUNCHER_FIDGET,
	SPORELAUNCHER_RELOAD_REACH,		// reload prepare
	SPORELAUNCHER_RELOAD_LOAD,		// reload in progress
	SPORELAUNCHER_RELOAD_AIM,		// reload end
	SPORELAUNCHER_FIRE,				// fire rocket
	SPORELAUNCHER_HOLSTER,			// holster
	SPORELAUNCHER_DRAW,				// draw
	SPORELAUNCHER_IDLE2,			// idle 2
};

LINK_ENTITY_TO_CLASS( weapon_sporelauncher, CSporeLauncher );

BOOL CSporeLauncher::IsUseable( void )
{
	return TRUE;
}


void CSporeLauncher::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SPORELAUNCHER;
	SET_MODEL(ENT(pev), "models/w_spore_launcher.mdl");
	pev->sequence		= 0;
	pev->framerate		= 1.0;

	m_iDefaultAmmo = SPORE_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CSporeLauncher::Precache( void )
{
	PRECACHE_MODEL("models/w_spore_launcher.mdl");
	PRECACHE_MODEL("models/v_spore_launcher.mdl");
	PRECACHE_MODEL("models/p_spore_launcher.mdl");


	PRECACHE_SOUND("weapons/splauncher_fire.wav");				// main fire sound
	PRECACHE_SOUND("weapons/splauncher_altfire.wav");			// alternative fire sound
	PRECACHE_SOUND("weapons/splauncher_pet.wav");				// idle weapon sound
	PRECACHE_SOUND("weapons/splauncher_reload.wav");			// reload weapon sound

	m_usSporeLauncher = PRECACHE_EVENT ( 1, "events/sporerocket.sc" );
	m_usSporeLauncherGrenade = PRECACHE_EVENT ( 1, "events/sporegrenade.sc" );

	UTIL_PrecacheOther( "spore" );
}


int CSporeLauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "spore";
	p->iMaxAmmo1 = SPORE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;//SPORE_MAX_CLIP;
	p->iSlot = 6;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_SPORELAUNCHER;
	p->iFlags = 0;
	p->iWeight = SPORELAUNCHER_WEIGHT;

	return 1;
}

int CSporeLauncher::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CSporeLauncher::Deploy( )
{
	return DefaultDeploy( "models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPORELAUNCHER_DRAW, "rpg" );
}

void CSporeLauncher::Holster( int skiplocal  )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( SPORELAUNCHER_HOLSTER );
}



void CSporeLauncher::PrimaryAttack()
{
	if ( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CSpore *pSpore = CSpore::CreateSporeRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1500;

		int flags;
		flags = 0;

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usSporeLauncher );

		m_iClip--; 
				
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.01;
	}
	m_fInSpecialReload = 0;
}


void CSporeLauncher::SecondaryAttack()
{
	if ( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CSpore *pSpore = CSpore::CreateSporeGrenade( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 700;

		int flags;
		flags = 0;

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usSporeLauncherGrenade );

		m_iClip--; 

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.01;
	}
	m_fInSpecialReload = 0;
}

void CSporeLauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != SPORE_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				SendWeaponAnim( SPORELAUNCHER_RELOAD_AIM );

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else 
		{
			int iAnim;
			switch ( RANDOM_LONG( 0, 2 ) )
			{
			case 0:	
				iAnim = SPORELAUNCHER_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0 / 30.0;
				break;
			
			case 1:	
				iAnim = SPORELAUNCHER_IDLE2;	
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 121.0 / 30.0;
				break;

			default:
			case 2:
				iAnim = SPORELAUNCHER_FIDGET;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 121.0 / 30.0;
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/splauncher_pet.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			}
			SendWeaponAnim( iAnim );

		}
	}
}
#endif