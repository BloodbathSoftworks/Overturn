//Coded by Jay

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum gl_e {
	GL_IDLE1 = 0,
	GL_IDLE2,
	GL_SHOOT1,
	GL_SHOOT2,
	GL_RELOAD,
	GL_DRAW,
	GL_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_glauncher, CGLauncher );

void CGLauncher::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glauncher"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_RPG;
	SET_MODEL(ENT(pev), "models/w_glauncher.mdl");

	m_iDefaultAmmo = RPG_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CGLauncher::Precache( void )
{
	PRECACHE_MODEL("models/v_glauncher.mdl");
	PRECACHE_MODEL("models/w_glauncher.mdl");
	PRECACHE_MODEL("models/p_glauncher.mdl");
	PRECACHE_MODEL("models/rocket.mdl");
	PRECACHE_MODEL("models/grenade.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND("items/ammopickup2.wav");

	UTIL_PrecacheOther("quake_rocket");

	m_usGLauncher = PRECACHE_EVENT( 1, "events/rpg.sc" );
}

int CGLauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rockets";//HACK!
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iWeight = RPG_WEIGHT;

	return 1;
}

BOOL CGLauncher::Deploy( )
{
	return DefaultDeploy( "models/v_glauncher.mdl", "models/p_glauncher.mdl", GL_DRAW, "rpg", /*UseDecrement() ? 1 : 0*/ 0 );
}



void CGLauncher::Reload( void )
{
	return;/*
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == RPG_MAX_CLIP)
		return;

	int iResult;
	iResult = DefaultReload( RPG_MAX_CLIP, GL_RELOAD, 2.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}*/
}

void CGLauncher::SecondaryAttack( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 2 )
	{
		m_fInSpecialReload = 0;
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}


//	m_iClip--;
	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] -= 2;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGLauncher, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );



	// Create the rocket
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	//Vector vecOrg = m_pPlayer->pev->origin + (gpGlobals->v_forward * 8);// + Vector(0,0,16);
	Vector vecOrg = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16/* + gpGlobals->v_right * 8*/ + gpGlobals->v_up * -8;
	Vector vecDir = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	CQuakeRocket *pRocket = CQuakeRocket::CreateRocket( vecOrg, vecDir, m_pPlayer );
	//

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;//25;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGLauncher::PrimaryAttack( void )
{	
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 1 )
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	//m_iClip--;
	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGLauncher, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );



	// Create the grenade
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16;
	CQuakeRocket *pRocket = CQuakeRocket::CreateGrenade( vecSrc, gpGlobals->v_forward *1200, m_pPlayer );
	//

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;//25;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGLauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	//if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (RANDOM_LONG(0,1) == 0)
		{
			iAnim = GL_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);//60.0 / 16.0;
		}
		else
		{
			iAnim = GL_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);//40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}








class CGLauncherAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpgammo.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_RPGCLIP_GIVE, "rockets", ROCKET_MAX_CARRY ) != -1)
		{
			EMIT_AMBIENT_SOUND(ENT(pOther->pev), pev->origin, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_grenades, CGLauncherAmmo );
