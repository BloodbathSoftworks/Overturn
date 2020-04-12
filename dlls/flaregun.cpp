//Coded by Jay

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum flare_e {
	FGUN_IDLE1 = 0,
	FGUN_IDLE2,
	FGUN_FIDGET,
	FGUN_SHOOT,
	FGUN_RELOAD,
	FGUN_DRAW,
	FGUN_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_flaregun, CFlareGun );

class CFlareBall : public CBaseAnimating
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) {return CLASS_NONE;}

	void EXPORT FlareTouch( CBaseEntity *pOther );
	void EXPORT Ignite(void);
	void EXPORT Explode( void );

	void EXPORT FlareThink(void);

	static CFlareBall *ShootFlareGun( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );

	//virtual BOOL IsProjectile(void) {return TRUE;}

	int m_iTrail;
	float m_flDie;
};

void CFlareGun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_flaregun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_FLAREGUN;
	SET_MODEL(ENT(pev), "models/w_flaregun.mdl");

	m_iDefaultAmmo = FLARE_DEFAULT_GIVE;

	ResetSequenceInfo();
	FallInit();// get ready to fall down.
}


void CFlareGun::Precache( void )
{
	PRECACHE_MODEL("models/v_flaregun.mdl");
	PRECACHE_MODEL("models/w_flaregun.mdl");
	PRECACHE_MODEL("models/p_flaregun.mdl");
	PRECACHE_MODEL("sprites/playerflame.spr");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND("items/ammopickup2.wav");

	UTIL_PrecacheOther("flareball");

	m_usFlareGun = PRECACHE_EVENT( 1, "events/flaregun.sc" );
}

int CFlareGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hornets";//HACK!
	p->iMaxAmmo1 = FLARE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FLARE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FLAREGUN;
	p->iWeight = FLARE_WEIGHT;

	return 1;
}

BOOL CFlareGun::Deploy( )
{
	return DefaultDeploy( "models/v_flaregun.mdl", "models/p_flaregun.mdl", FGUN_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CFlareGun::SecondaryAttack( void )
{

}

void CFlareGun::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == FLARE_MAX_CLIP)
		return;

	int iResult;
	iResult = DefaultReload( FLARE_MAX_CLIP, FGUN_RELOAD, 3 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CFlareGun::PrimaryAttack( void )
{	
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFlareGun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	//
	Vector vecAng = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	vecAng.x = -vecAng.x;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16;
	CFlareBall::ShootFlareGun( m_pPlayer->pev, vecSrc, gpGlobals->v_forward * 1600 );
	//

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;//25;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CFlareGun::WeaponIdle( void )
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

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = FGUN_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);//49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = FGUN_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);//60.0 / 16.0;
		}
		else
		{
			iAnim = FGUN_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);//40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}







/*
class CFlareGunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/ammopickup2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_FLARECLIP_GIVE, "Hornets", FLARE_MAX_CARRY ) != -1)
		{
			EMIT_AMBIENT_SOUND(ENT(pOther->pev), pev->origin, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_flarebox, CFlareGunAmmo );*/










//FLARE BALL!
LINK_ENTITY_TO_CLASS( flareball, CFlareBall );

CFlareBall * CFlareBall:: ShootFlareGun( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CFlareBall *pProj = GetClassPtr( (CFlareBall *)NULL );
	pProj->Spawn();

	pProj->pev->classname = MAKE_STRING("flareball");

	UTIL_SetOrigin( pProj->pev, vecStart );
	pProj->pev->velocity = vecVelocity;
	pProj->pev->angles = UTIL_VecToAngles(pProj->pev->velocity);
	pProj->pev->owner = ENT(pevOwner);
		
	return pProj;
}


TYPEDESCRIPTION	CFlareBall::m_SaveData[] = 
{
	DEFINE_FIELD( CFlareBall, m_flDie, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CFlareBall, CBaseEntity );

void CFlareBall::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/flareball.spr");
	pev->rendercolor = Vector(255,0,0);
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 0.4;
	pev->gravity = 0.05;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	SetTouch( &CFlareBall::FlareTouch );
	SetThink( &CFlareBall::FlareThink );

	pev->dmg = gSkillData.plrDmgFlaregun;
	m_flDie = -1;

	UTIL_MakeAimVectors( pev->angles );

	Ignite();

	pev->nextthink = gpGlobals->time + 0.1;
}


void CFlareBall::Precache( )
{
	PRECACHE_MODEL("sprites/flareball.spr");
	m_iTrail = PRECACHE_MODEL("sprites/flaretrail.spr");

	PRECACHE_SOUND("weapons/flarehit.wav");
	PRECACHE_SOUND("flare/flareburn.wav");
}

void CFlareBall::Ignite( void )
{
//	pev->effects |= EF_LIGHT;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 4 );  // width
		WRITE_BYTE( pev->rendercolor.x );   // r, g, b
		WRITE_BYTE( pev->rendercolor.y );   // r, g, b
		WRITE_BYTE( pev->rendercolor.z );   // r, g, b
		WRITE_BYTE( 180 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "flare/flareburn.wav", 0.6, ATTN_NORM, 0, 100);
}

void CFlareBall::FlareTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	//SetThink( NULL );

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

//	UTIL_EmitAmbientSound(ENT(pev), porg, "weapons/flarehit.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(110,130));

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, porg );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(porg.x);	// X
		WRITE_COORD(porg.y);	// Y
		WRITE_COORD(porg.z);	// Z
		WRITE_BYTE( 12 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 0 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 20 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

	if (pOther->pev->takedamage)
	{
		// UNDONE: this needs to call TraceAttack instead
		/*ClearMultiDamage( );
		pOther->TraceAttack(pevOwner, pev->dmg, pev->velocity.Normalize(), &tr, DMG_BURN); 
		ApplyMultiDamage( pev, pevOwner );
*/
		/*if ( pOther->pev->flags & FL_MONSTER | pOther->IsPlayer() )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor = pev->rendercolor;
			pOther->pev->renderamt = 1;
			bShockedEnt = pOther;
		}*/
	}
//	else
//		::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg*2, 64, CLASS_NONE, DMG_BLAST | DMG_BURN | DMG_ALWAYSGIB );


	pev->movetype = MOVETYPE_NONE;
	pev->velocity = g_vecZero;
	pev->gravity = 0;
	UTIL_SetOrigin(this, porg);

	//pev->effects |= EF_NODRAW;

	//UTIL_Remove( this );
	m_flDie = gpGlobals->time + 0.2;
}

void CFlareBall::FlareThink( void )
{
	if (gpGlobals->time >= m_flDie
		&& m_flDie != -1)
	{
		m_flDie = -1;

		Explode();
	//	SetThink( &CFlareBall::Explode );
	//	pev->nextthink = gpGlobals->time + 1.0;
		return;
	}

	pev->frame += 1; //animate flareball

	if(pev->frame > 10)
		pev->frame = fmod( pev->frame, 10 );


	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 6 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 0 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 5 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );


	pev->nextthink = gpGlobals->time + 0.08;
}

void CFlareBall::Explode( void )
{
	pev->effects |= EF_NODRAW;
	pev->renderamt = 0;

	/*MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION2 );		// This just makes a dynamic light now
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z);
		WRITE_BYTE( 247 );//color
		WRITE_BYTE( 1 );
	MESSAGE_END();*/

	PLAYBACK_EVENT_FULL( 0, edict(), m_usFlareBurst, 0, (float *)&pev->origin, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	/*MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 20 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 60 );		// g
		WRITE_BYTE( 15 );		// b
		WRITE_BYTE( 40 );		// time * 10
		WRITE_BYTE( 40 );		// decay * 0.1
	MESSAGE_END( );*/

	entvars_t *pevOwner = VARS(pev->owner);
	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 100, CLASS_NONE, (DMG_BLAST | DMG_BURN | DMG_IGNITE | DMG_ALWAYSGIB) );

	UTIL_EmitAmbientSound(ENT(pev), pev->origin, "weapons/flarehit.wav", 1.0, ATTN_NORM, 0, 100);//RANDOM_LONG(80,90));

	STOP_SOUND(ENT(pev),CHAN_VOICE,"flare/flareburn.wav");

	SetThink(&CFlareBall :: SUB_Remove );
	pev->nextthink = gpGlobals->time + 1.0;
}