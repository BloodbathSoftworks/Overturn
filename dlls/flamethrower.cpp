//Coded by Jay

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum fthrow_e {
	FLAME_IDLE1 = 0,
	FLAME_FIDGET,
	FLAME_ALTFIREON,
	FLAME_ALTFIRECYCLE,
	FLAME_ALTFIREOFF,
	FLAME_FIRE1,
	FLAME_FIRE2,
	FLAME_FIRE3,
	FLAME_FIRE4,
	FLAME_DRAW,
	FLAME_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_flamethrower, CFlameThrower );

class CFlame : public CBaseAnimating
{
public:
/*	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];*/

	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) {return CLASS_NONE;}

	void EXPORT FlameTouch( CBaseEntity *pOther );
	void EXPORT FlameThink(void);

	static CFlame *ShootFlameThrower( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
};

void CFlameThrower::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_flamethrower"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_FLAMETHROWER;
	SET_MODEL(ENT(pev), "models/w_flamethrower.mdl");

	m_iDefaultAmmo = FLAME_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CFlameThrower::Precache( void )
{
	PRECACHE_MODEL("models/v_flamethrower.mdl");
	PRECACHE_MODEL("models/w_flamethrower.mdl");
	PRECACHE_MODEL("models/p_flamethrower.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND("items/ammopickup2.wav");

	UTIL_PrecacheOther("flame");

	m_usFlameThrower = PRECACHE_EVENT( 1, "events/flamethrower.sc" );
}

int CFlameThrower::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Snarks";//HACK!
	p->iMaxAmmo1 = FLAME_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FLAMETHROWER;
	p->iWeight = FLAME_WEIGHT;

	return 1;
}

BOOL CFlameThrower::Deploy( )
{
	return DefaultDeploy( "models/v_flamethrower.mdl", "models/p_flamethrower.mdl", FLAME_DRAW, "egon", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CFlameThrower::SecondaryAttack( void )
{

}

void CFlameThrower::PrimaryAttack( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 1 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFlameThrower, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );


	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	//
	Vector vecAng = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	vecAng.x = -vecAng.x;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16;
	CFlame::ShootFlameThrower( m_pPlayer->pev, vecSrc, gpGlobals->v_forward * 800 );
	//

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.15;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CFlameThrower::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (RANDOM_LONG(0,3) == 0)
		{
			iAnim = FLAME_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
		}
		else
		{
			iAnim = FLAME_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
		}
		SendWeaponAnim( iAnim, 1 );
	}
}







class CFlameThrowerAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_canister.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_canister.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_FLAME_GIVE, "Snarks", FLAME_MAX_CARRY ) != -1)
		{
			EMIT_AMBIENT_SOUND(ENT(pOther->pev), pev->origin, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_canister, CFlameThrowerAmmo );


//FLAME

LINK_ENTITY_TO_CLASS( flame, CFlame );

CFlame * CFlame:: ShootFlameThrower( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CFlame *pProj = GetClassPtr( (CFlame *)NULL );
	pProj->Spawn();

	pProj->pev->classname = MAKE_STRING("flame");

	UTIL_SetOrigin( pProj->pev, vecStart );
	pProj->pev->velocity = vecVelocity;
	pProj->pev->angles = UTIL_VecToAngles(pProj->pev->velocity);
	pProj->pev->angles.x += RANDOM_FLOAT(15,-15);
	pProj->pev->angles.y += RANDOM_FLOAT(30,-30);
	pProj->pev->owner = ENT(pevOwner);
		
	return pProj;
}


/*TYPEDESCRIPTION	CFlame::m_SaveData[] = 
{
	DEFINE_FIELD( CFlame, m_flDie, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CFlame, CBaseEntity );*/

void CFlame::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/fthrow.spr");
	pev->rendercolor = Vector(255,255,255);
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	pev->scale = RANDOM_FLOAT(0.7,1.2);
	pev->gravity = 0.0;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	SetTouch( &CFlame::FlameTouch );
	SetThink( &CFlame::FlameThink );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = 10;

	UTIL_MakeAimVectors( pev->angles );
}


void CFlame::Precache( )
{
	PRECACHE_MODEL("sprites/fthrow.spr");
}


void CFlame::FlameTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	if (pOther->edict() == pev->owner)
	{
		UTIL_Remove( this );
		return;
	}

	entvars_t *pevOwner = VARS(pev->owner);

	TraceResult tr;// XDM3035a: less hacky?
	Vector forward;
	UTIL_MakeVectorsPrivate(pev->angles, forward, NULL, NULL);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 8.0f, dont_ignore_monsters, ENT(pev), &tr);

	Vector porg = tr.vecEndPos + (tr.vecPlaneNormal * 2.0);

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 64, CLASS_NONE, (DMG_BURN|DMG_IGNITE) );

	UTIL_Remove( this );
}



void CFlame::FlameThink( void )
{
	pev->frame += 1; //animate flame

	if(pev->frame > 15)
	{
		UTIL_Remove( this );//remove after flame dissipates
		return;
	}

	pev->velocity = pev->velocity*0.8;//slow down
	pev->scale += RANDOM_FLOAT(0.05,0.07);//grow a bit bigger

	pev->nextthink = gpGlobals->time + 0.1;
}


