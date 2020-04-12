//========= Copyright © 2004-2006, Raven City Team, All rights reserved. ============
//
// Purpose:	Opposing Forces M249, Copyright Highlander
//
// $NoKeywords: $
//===================================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE1,
	M249_RELOAD1,
	M249_RELOAD2,
	M249_HOLSTER,
	M249_DRAW,
	M249_FIRE1,
	M249_FIRE2,
	M249_FIRE3,
};

LINK_ENTITY_TO_CLASS( weapon_saw, CM249 );

//=========================================================
//=========================================================

void CM249::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_saw"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_saw.mdl");
	m_iId = WEAPON_M249;

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CM249::Precache( void )
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL ("models/saw_shell.mdl");// saw shellTE_MODEL
	m_iLink = PRECACHE_MODEL ("models/saw_link.mdl");//  saw linkTE_MODEL

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/ammopickup2.wav");              

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND ("weapons/saw_fire1.wav");
	PRECACHE_SOUND ("weapons/saw_fire2.wav");
	PRECACHE_SOUND ("weapons/saw_fire3.wav");

	PRECACHE_SOUND ("weapons/dryfire1.wav");

	m_usM249 = PRECACHE_EVENT( 1, "events/m249.sc" );
}

int CM249::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;//M249_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return 1;
}

int CM249::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CM249::Deploy( )
{
	UpdateClip();
	return DefaultDeploy( "models/v_saw.mdl", "models/p_saw.mdl", M249_DRAW, "mp5", UseDecrement() ? 1 : 0, pev->body );
}
void CM249::Holster( int skiplocal /* = 0 */)
{
	UpdateClip();
	m_fInReload = FALSE;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}
void CM249::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		UpdateClip();
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	UpdateClip();

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
	flags = 0;

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM249, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.0675;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.0675;

	if ( !FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
	{
		float flOldPlayerVel = m_pPlayer->pev->velocity.z;
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + (50 * -gpGlobals->v_forward);
		m_pPlayer->pev->velocity.z = flOldPlayerVel;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}


void CM249::ItemPostFrame( void )
{
	if(m_fInSpecialReload)
	{
	//	int iClip = M249_MAX_CLIP - m_iClip;
	//	if(iClip > m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		int iClip = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

		iClip += m_iClip;

		if (iClip >= 8) pev->body = 0;	
		if (iClip == 7) pev->body = 1;
		if (iClip == 6) pev->body = 2;
		if (iClip == 5) pev->body = 3;
		if (iClip == 4) pev->body = 4;
		if (iClip == 3) pev->body = 5;
		if (iClip == 2) pev->body = 6;
		if (iClip == 1) pev->body = 7;
		if (iClip == 0) pev->body = 8;

		DefaultReload(M249_MAX_CLIP, M249_RELOAD2, 2.46, pev->body);
		m_fInSpecialReload = FALSE;
		return;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CM249::UpdateClip( void )
{
	if (m_iClip >= 8) pev->body = 0;	
	if (m_iClip == 7) pev->body = 1;
	if (m_iClip == 6) pev->body = 2;
	if (m_iClip == 5) pev->body = 3;
	if (m_iClip == 4) pev->body = 4;
	if (m_iClip == 3) pev->body = 5;
	if (m_iClip == 2) pev->body = 6;
	if (m_iClip == 1) pev->body = 7;
	if (m_iClip == 0) pev->body = 8;
}
void CM249::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		ResetEmptySound( );
		UpdateClip();

		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.8)
		{
			iAnim = M249_SLOWIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/7.0);
		}
		else
		{
			iAnim = M249_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (65.0/10.0)*3;// * RANDOM_LONG(2, 5);
		}
		SendWeaponAnim( iAnim , UseDecrement() ? 1 : 0, pev->body );
	}
}

class CM249AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_saw_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_556_GIVE, "556", _556_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_AMBIENT_SOUND(ENT(pOther->pev), pev->origin, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_556, CM249AmmoClip )