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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( python_laser, CPythonLaser );
//=========================================================
//=========================================================
CPythonLaser *CPythonLaser::CreateSpot( void )
{
	CPythonLaser *pPythonLaser = GetClassPtr( (CPythonLaser *)NULL );
	pPythonLaser->Spawn();

	pPythonLaser->pev->classname = MAKE_STRING("python_laser");

	return pPythonLaser;
}

//=========================================================
//=========================================================
void CPythonLaser::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	pev->scale = 0.40;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CPythonLaser::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	SetThink( &CPythonLaser::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CPythonLaser::Revive( void )
{
	pev->effects &= ~EF_NODRAW;
	SetThink( NULL );
}
void CPythonLaser::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
	PRECACHE_MODEL("sprites/laserbeam.spr");
};

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython );
LINK_ENTITY_TO_CLASS( weapon_357, CPython );

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

int CPython::AddToPlayer( CBasePlayer *pPlayer )
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

void CPython::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PYTHON;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CPython::Precache( void )
{
	UTIL_PrecacheOther( "python_laser" );
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/ammopickup1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/357_shot1.wav");
	PRECACHE_SOUND ("weapons/357_shot2.wav");

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

BOOL CPython::Deploy( )
{
	pev->body = 1;
	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", UseDecrement(), pev->body );
}


void CPython::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_pPythonLaser)
	{
		m_pPythonLaser->Killed( NULL, GIB_NEVER );
		m_pPythonLaser = NULL;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( PYTHON_HOLSTER );
}

void CPython::SecondaryAttack( void )
{
	m_fLaserActive = ! m_fLaserActive;
	if (!m_fLaserActive && m_pPythonLaser)
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
		m_pPythonLaser->Killed( NULL, GIB_NORMAL );
		m_pPythonLaser = NULL;
	}
	else
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4;
}

void CPython::PrimaryAttack()
{
	PythonFire();
}

void CPython::PythonFire()
{
	if (m_iClip <= 0)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
	flags = 0;

		UTIL_ScreenShake( m_pPlayer->pev->origin, 8.0, 80.0, 0.3, 64 );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_fLaserActive && m_pPythonLaser)
		m_flNextPrimaryAttack = gpGlobals->time + 0.75;
	else
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CPython::UpdateSpot( void )
{
	if(!m_fLaserActive)
		return;
		
	if (!m_pPythonLaser)
		m_pPythonLaser = CPythonLaser::CreateSpot();

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );;
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
	
	UTIL_SetOrigin( m_pPythonLaser->pev, tr.vecEndPos );

	if ( UTIL_PointContents(tr.vecEndPos) == CONTENT_SKY )
	{
		UTIL_Remove( m_pPythonLaser );
		m_pPythonLaser = FALSE;
	}
}

void CPython::ItemPostFrame( void )
{
	UpdateSpot();
	CBasePlayerWeapon::ItemPostFrame();
}

void CPython::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == PYTHON_MAX_CLIP)
		return;

	int iResult;
	iResult = DefaultReload( 6, PYTHON_RELOAD, 3.09 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}

	if(m_pPythonLaser)
		m_pPythonLaser->Suspend(3.09);
}

void CPython::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + GetSequenceTime(iAnim);
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + GetSequenceTime(iAnim);
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + GetSequenceTime(iAnim);
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + GetSequenceTime(iAnim);
	}
	
	int bUseScope = FALSE;

	bUseScope = g_pGameRules->IsMultiplayer();
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, bUseScope );
}



class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357ammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_357ammo.mdl");
		PRECACHE_SOUND("items/ammopickup1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pOther->pev), CHAN_ITEM, "items/ammopickup1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo );