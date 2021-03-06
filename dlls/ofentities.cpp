//========= Copyright � 2004-2008, Raven City Team, All rights reserved. ============//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

#ifdef PARTIFLE_FUSION

#include "extdll.h"
#include "util.h"
#include "const.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "ofentities.h"
#include "customentity.h"
#include "decals.h"
#include "func_break.h"
#include "explode.h"
#include "trains.h"

float VectorNormalize (vec3_t v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


//=================================
// Opposing Forces spore ammo plant 
// Copyright Demiurge, Copyright Highlander
//=================================

LINK_ENTITY_TO_CLASS( spore, CSpore );
CSpore *CSpore::CreateSporeGrenade( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CSpore *pSpore = GetClassPtr( (CSpore *)NULL );
	UTIL_SetOrigin( pSpore->pev, vecOrigin );
	pSpore->pev->angles = vecAngles;
	pSpore->m_iPrimaryMode = FALSE;
	pSpore->pev->angles = vecAngles;
	pSpore->pev->owner = pOwner->edict();
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->Spawn();

	return pSpore;
}

//=========================================================
//=========================================================
CSpore *CSpore::CreateSporeRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CSpore *pSpore = GetClassPtr( (CSpore *)NULL );
	UTIL_SetOrigin( pSpore->pev, vecOrigin );
	pSpore->pev->angles = vecAngles;
	pSpore->m_iPrimaryMode = TRUE;
	pSpore->pev->angles = vecAngles;
	pSpore->pev->owner = pOwner->edict();
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->Spawn();
 	pSpore->pev->angles = vecAngles;

	return pSpore;
}
//=========================================================
//=========================================================

void CSpore :: Spawn( void )
{
	Precache( );
	// motor
	if (m_iPrimaryMode)
		pev->movetype = MOVETYPE_FLY;
	else
		pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, -4), Vector(4, 4, 4));
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_MakeVectors( pev->angles );

	pev->gravity = 0.5;
	Glow ();


	if (m_iPrimaryMode)
	{
		SetThink( &CSpore::FlyThink );
		SetTouch( &CSpore::ExplodeThink );
		pev->velocity = gpGlobals->v_forward * 250;
	}
	else
	{
		SetThink( &CSpore::FlyThink );
		SetTouch( &CSpore::BounceThink );
	}

	pev->dmgtime = gpGlobals->time + 2;
	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = gSkillData.plrDmgSpore;
}
//=========================================================

void CSpore :: Precache( void )
{
	PRECACHE_MODEL("models/spore.mdl");
	m_iDrips = PRECACHE_MODEL("sprites/tinyspit.spr");
	m_iGlow = PRECACHE_MODEL("sprites/glow02.spr");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_01.spr");
	m_iExplodeC = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND ("weapons/splauncher_impact.wav");
	PRECACHE_SOUND ("weapons/spore_hit1.wav");//Bounce grenade
	PRECACHE_SOUND ("weapons/spore_hit2.wav");//Bounce grenade
	PRECACHE_SOUND ("weapons/spore_hit3.wav");//Bounce grenade
}
//=========================================================

void CSpore :: Glow( void )
{
	m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.75;
	m_pSprite->SetTransparency( kRenderTransAdd, 150, 158, 19, 155, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
}
//=========================================================

void CSpore :: FlyThink( void  )
{
	pev->nextthink = gpGlobals->time + 0.001;
 
	TraceResult tr;
 
	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x + RANDOM_LONG(-5,5));	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y + RANDOM_LONG(-5,5) );
		WRITE_COORD( pev->origin.z + RANDOM_LONG(-5,5));
		WRITE_COORD( tr.vecPlaneNormal.x );
		WRITE_COORD( tr.vecPlaneNormal.y );
		WRITE_COORD( tr.vecPlaneNormal.z );
		WRITE_SHORT( m_iDrips );
		WRITE_BYTE( 1  ); // count
		WRITE_BYTE( 18  ); // speed
		WRITE_BYTE( 1000 );
	MESSAGE_END();

	if (!m_iPrimaryMode)
	{
		if (pev->dmgtime <= gpGlobals->time)
			Explode ();
	}
}
//=========================================================

void CSpore::BounceThink(  CBaseEntity *pOther  )
{
	if ( pOther->pev->flags & FL_MONSTER || pOther->IsPlayer()  )
	{
		if ( !FClassnameIs( pOther->pev, "monster_maria" ) 
			&& !FClassnameIs( pOther->pev, "monster_boris" ) )
		{
			Explode ();
		}
		else
		{
			UTIL_Remove( m_pSprite );
			UTIL_Remove(this);
		}
	}

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove( this );
		return;
	}

#ifndef CLIENT_DLL
	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_GENERIC ); 
			ApplyMultiDamage( pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}
#endif

	Vector vecTestVelocity;

	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;

}
//=========================================================

void CSpore :: BounceSound( void )
{
	switch (RANDOM_LONG (0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit1.wav", 0.25, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit2.wav", 0.25, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit3.wav", 0.25, ATTN_NORM); break;
	}
}
//=========================================================

void CSpore::ExplodeThink(  CBaseEntity *pOther  )
{
	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove( this );
		return;
	}

	if ( FClassnameIs( pOther->pev, "monster_maria" ) 
		|| FClassnameIs( pOther->pev, "monster_boris" ) )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove(this);
	}


	Explode ();
}
//=========================================================
void CSpore::Explode( void )
{
	SetTouch( NULL );
	SetThink( NULL );
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/splauncher_impact.wav", 1, ATTN_NORM);

	TraceResult tr;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		switch ( RANDOM_LONG( 0, 1 ) )
		{
			case 0:	
				WRITE_SHORT( m_iExplode );
				break;

			default:
			case 1:
				WRITE_SHORT( m_iExplodeC );
				break;
		}
		WRITE_BYTE( 25  ); // scale * 10
		WRITE_BYTE( 155  ); // framerate
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( tr.vecPlaneNormal.x );
		WRITE_COORD( tr.vecPlaneNormal.y );
		WRITE_COORD( tr.vecPlaneNormal.z );
		WRITE_SHORT( m_iDrips );
		WRITE_BYTE( 50  ); // count
		WRITE_BYTE( 30  ); // speed
		WRITE_BYTE( 640 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_BYTE( 12 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 20 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

    	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set


	::RadiusDamage ( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_PLAYER_BIOWEAPON, DMG_GENERIC );

	if (m_iPrimaryMode)
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		//UTIL_CustomDecal ( &tr, "brains" );
	}

	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
	UTIL_Remove ( this );
	UTIL_Remove( m_pSprite );
	m_pSprite = NULL;
}
//=================================
// Opposing Forces spore ammo plant 
// Copyright Demiurge
//=================================
typedef enum
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDOWN,
	SPOREAMMO_SNATCHDOWN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2,
} SPOREAMMO;

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo );

TYPEDESCRIPTION	CSporeAmmo::m_SaveData[] = 
{
	DEFINE_FIELD( CSporeAmmo, m_flTimeSporeIdle, FIELD_TIME ),
	DEFINE_FIELD( CSporeAmmo, borntime, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CSporeAmmo, CBaseEntity );

void CSporeAmmo :: Precache( void )
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/spore_ammo.wav");
	UTIL_PrecacheOther ( "spore" );
}
//=========================================================
// Spawn
//=========================================================
void CSporeAmmo :: Spawn( void )
{
	TraceResult tr;
	Precache( );
	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	UTIL_SetSize(pev, Vector( -20, -20, -8 ), Vector( 20, 20, 16 ));
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;
	borntime = 1;

	pev->sequence = SPOREAMMO_IDLE;
	pev->body = 0;
	SetThink (&CSporeAmmo::BornThink);
	SetTouch (&CSporeAmmo::AmmoTouch);
	
	m_flTimeSporeIdle = gpGlobals->time + 22;
	pev->nextthink = gpGlobals->time + 0.1;
}
	
//=========================================================
// Override all damage
//=========================================================
int CSporeAmmo :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return TRUE;
}

void CSporeAmmo::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (!borntime) // rigth '!borntime'  // blast in anytime 'borntime || !borntime'
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * -20;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( vecSrc.x );	// Send to PAS because of the sound
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
		MESSAGE_END();

			
		ALERT( at_aiconsole, "angles %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );

				Vector angles = pev->angles + gpGlobals->v_up;
		if (abs(angles.x) != 0)
				angles.x = angles.x + 90;
		if (abs(angles.y) != 0)
				angles.y = angles.y + 90;
		if (abs(angles.z) != 0)
				angles.y = angles.y + 90;
		ALERT( at_aiconsole, "angles %f %f %f\n", angles.x, angles.y, angles.z );

		CSpore *pSpore = CSpore::CreateSporeGrenade( vecSrc, angles, this );
		pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1000;

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence		= SPOREAMMO_SNATCHDOWN;
		pev->body			= 0;
		borntime			= 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
	}
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

//=========================================================
// Thinking begin
//=========================================================
void CSporeAmmo :: BornThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	pev->sequence = 3;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;
	pev->body = 1;
	borntime = 0;
	SetThink (&CSporeAmmo::IdleThink);
	
	m_flTimeSporeIdle = gpGlobals->time + 16;
}

void CSporeAmmo :: IdleThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	if (borntime)
	{
		pev->sequence = SPOREAMMO_IDLE;

		m_flTimeSporeIdle = gpGlobals->time + 18;
		SetThink(&CSporeAmmo::BornThink);
		return;
	}
	else
	{
		pev->sequence = SPOREAMMO_IDLE1;
	}
}

void CSporeAmmo :: AmmoTouch ( CBaseEntity *pOther )
{
	Vector		vecSpot;
	TraceResult	tr;

	if ( pOther->pev->velocity == g_vecZero || !pOther->IsPlayer() )
		return;

	if (borntime)
		return;

	int bResult = (pOther->GiveAmmo( AMMO_SPORE_GIVE, "spore", SPORE_MAX_CARRY) != -1);
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence = SPOREAMMO_SNATCHDOWN;
		pev->body = 0;
		borntime = 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
	}
}
//=================================
// Opposing Forces shock beam
// Copyright Demiurge, Copyright Highlander
//=================================
LINK_ENTITY_TO_CLASS( shock_beam, CShockBeam );
//=========================================================
//=========================================================
void CShockBeam :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}
CShockBeam *CShockBeam::ShockCreate( void )
{
	// Create a new entity with CShockBeam private data
	CShockBeam *pShock = GetClassPtr( (CShockBeam *)NULL );
	pShock->pev->classname = MAKE_STRING("shock_beam");
	pShock->Spawn();

	return pShock;
}

void CShockBeam::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/shock_effect.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	BlastOn();
	Glow ();

	SetTouch( &CShockBeam::ShockTouch );

	if ( g_pGameRules->IsMultiplayer() )
		pev->dmg = gSkillData.plrDmgShockm;
	else
		pev->dmg = gSkillData.plrDmgShocks;

	UTIL_MakeAimVectors( pev->angles );

	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1;
}


void CShockBeam::Precache( )
{
	PRECACHE_MODEL("models/shock_effect.mdl");

	PRECACHE_SOUND( "weapons/shock_fire.wav" );
	PRECACHE_SOUND( "weapons/shock_impact.wav" );

	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/glow02.spr" );
}

int	CShockBeam :: Classify ( void )
{
	return	CLASS_NONE;
}

void CShockBeam::ShockTouch( CBaseEntity *pOther )
{
	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;

		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;

		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;

		UTIL_Remove( this );
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		if ( g_pGameRules->IsMultiplayer() )
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgShockm, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM );
		else
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgShocks, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM );

		ApplyMultiDamage( pev, pevOwner );

		if ( pOther->pev->flags & FL_MONSTER | pOther->IsPlayer() )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor.x = 0; // R
			pOther->pev->rendercolor.y = 255; // G
			pOther->pev->rendercolor.z = 255; // B
			pOther->pev->renderamt = 1;
			pShockedEnt = pOther;
		}

		SetThink ( &CShockBeam::FadeShock );
		pev->nextthink = gpGlobals->time + 1.0;

		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);
	}
	UTIL_Sparks( pev->origin );
	ExplodeThink();
}

void CShockBeam::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	BlastOff();
	iScale = 10;

	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 10 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );


	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	TraceResult tr;

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
	//UTIL_CustomDecal ( &tr, "scorch" );
}
void CShockBeam::BlastOff ( void )
{
	UTIL_Remove( m_pBeam );
	m_pBeam = NULL;

	UTIL_Remove( m_pNoise );
	m_pNoise = NULL;

	UTIL_Remove( m_pSprite );
	m_pSprite = NULL;

}
void CShockBeam::FadeShock ( void )
{
	if ( pShockedEnt )
	{
		pShockedEnt->pev->renderfx = kRenderFxNone;
		pShockedEnt->pev->rendercolor.x = 0; // R
		pShockedEnt->pev->rendercolor.y = 0; // G
		pShockedEnt->pev->rendercolor.z = 0; // B
		pShockedEnt->pev->renderamt = 255;
	}
	UTIL_Remove( this );
}
void CShockBeam :: Glow( void )
{
	Vector		posGun, angleGun;
	int m_iAttachment = 1;
	GetAttachment( m_iAttachment, posGun, angleGun );

	m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), m_iAttachment );
	m_pSprite->pev->scale = 0.25;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 125, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
}
void CShockBeam::BlastOn ( void )
{
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	
	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	Vector vecEnd = (gpGlobals->v_forward * 40) + posGun;
	UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

	m_pBeam->EntsInit( entindex(), entindex() );
	m_pBeam->SetStartAttachment( 1 );
	m_pBeam->SetEndAttachment( 2 );
	m_pBeam->SetBrightness( 190 );
	m_pBeam->SetScrollRate( 20 );
	m_pBeam->SetNoise( 20 );
	m_pBeam->DamageDecal( 1 );
	m_pBeam->SetFlags( BEAM_FSHADEOUT );
	m_pBeam->SetColor( 0, 255, 255 );
	m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

	m_pNoise->EntsInit( entindex(), entindex() );
	m_pNoise->SetStartAttachment( 1 );
	m_pNoise->SetEndAttachment( 2 );
	m_pNoise->SetBrightness( 190 );
	m_pNoise->SetScrollRate( 20 );
	m_pNoise->SetNoise( 65 );
	m_pNoise->DamageDecal( 1 );
	m_pNoise->SetFlags( BEAM_FSHADEOUT );
	m_pNoise->SetColor( 255, 255, 173 );
	m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;

//	EXPORT RelinkBeam();
}
//=================================================
// Displacer Targets	
//=================================================
// Xen Target
LINK_ENTITY_TO_CLASS( info_displacer_xen_target, CPointEntity );
// Earth Target
LINK_ENTITY_TO_CLASS( info_displacer_earth_target, CPointEntity );

//=================================================
// Displacer Ball	
//=================================================

LINK_ENTITY_TO_CLASS( displacer_ball, CDisplacerBall );

CDisplacerBall *CDisplacerBall::BallCreate( void )
{
	CDisplacerBall *pBall = GetClassPtr( (CDisplacerBall *)NULL );
	pBall->pev->classname = MAKE_STRING("displacer_ball");
	pBall->m_fIsSelfTeleporter = FALSE;
	pBall->Spawn();

	return pBall;
}
CDisplacerBall *CDisplacerBall::SelfCreate( void )
{
	CDisplacerBall *pBall = GetClassPtr( (CDisplacerBall *)NULL );
	pBall->pev->classname = MAKE_STRING("displacer_ball");
	pBall->m_fIsSelfTeleporter = TRUE;
	pBall->Spawn();
	pBall->SelfRemove();

	return pBall;
}
void CDisplacerBall::Spawn( )
{
	Precache( );

	if ( !m_fIsSelfTeleporter )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->solid = SOLID_BBOX;
		pev->velocity = gpGlobals->v_forward * DISPLACER_BALL_SPEED;
	}

	SET_MODEL(ENT(pev), "sprites/exit1.spr");
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	pev->effects |= EF_BRIGHTLIGHT;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 180;
	pev->rendercolor.z = 28;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	if ( !m_fIsSelfTeleporter )
	{
		SetTouch( &CDisplacerBall::BallTouch );
		SetThink( &CDisplacerBall::FlyThink );
		pev->nextthink = gpGlobals->time + 0.05;
	}
}


void CDisplacerBall::Precache( )
{
	PRECACHE_MODEL("sprites/exit1.spr");
	m_iBeamSprite = PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/xflare1.spr");

	PRECACHE_SOUND("weapons/displacer_teleport.wav");
	PRECACHE_SOUND("weapons/displacer_teleport_player.wav");
	PRECACHE_SOUND("weapons/displacer_impact.wav");

	m_iDispRing = PRECACHE_MODEL ("sprites/disp_ring.spr");
}
int	CDisplacerBall :: Classify ( void )
{
	return	CLASS_PLAYER_BIOWEAPON;
}
void CDisplacerBall :: BallTouch ( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;
	Vector		vecSrc;
	pev->enemy = pOther->edict();
	CBaseEntity *pTarget = NULL;
	vecSpot = pev->origin - pev->velocity.Normalize() * 32;

	if ( FClassnameIs( pOther->pev, "monster_maria" ) 
		|| FClassnameIs( pOther->pev, "monster_boris" ) )
		UTIL_Remove(this);

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT(pev), &tr );
	if( !LockRing )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 840);
			WRITE_SHORT( m_iDispRing );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 10 );
			WRITE_BYTE( 3 );
			WRITE_BYTE( 20 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 0 );
		MESSAGE_END();


		if ( pOther )
		{
			if ( g_pGameRules->IsMultiplayer() )
			{
				if ( pOther->IsPlayer() )
				{
					for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
					pTarget = UTIL_FindEntityByClassname(pTarget, "info_player_deathmatch" );

					if (pTarget)
					{
						UTIL_ScreenFade( pOther, Vector(0, 160, 0), 0.5, 0.5, 255, FFADE_IN );

						Vector tmp = pTarget->pev->origin;
						tmp.z -= pOther->pev->mins.z;
						tmp.z++;
						UTIL_SetOrigin( pOther->pev, tmp );

						pOther->pev->angles = pTarget->pev->angles;
						pOther->pev->velocity = pOther->pev->basevelocity = g_vecZero;

						CSprite *pSpr = CSprite::SpriteCreate( "sprites/xflare1.spr", vecSrc, TRUE );
						pSpr->AnimateAndDie( 6 );
						pSpr->SetTransparency(kRenderGlow, 184, 250, 214, 255, kRenderFxNoDissipation);

						EMIT_SOUND(ENT(pOther->pev), CHAN_WEAPON, "weapons/displacer_teleport_player.wav", 1, ATTN_NORM);
						
						vecSrc = pTarget->pev->origin;
						MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
							WRITE_BYTE(TE_DLIGHT);
							WRITE_COORD( vecSrc.x );	// X
							WRITE_COORD( vecSrc.y );	// Y
							WRITE_COORD( vecSrc.z );	// Z
							WRITE_BYTE( 24 );		// radius * 0.1
							WRITE_BYTE( 255 );		// r
							WRITE_BYTE( 180 );		// g
							WRITE_BYTE( 96 );		// b
							WRITE_BYTE( 20 );		// time * 10
							WRITE_BYTE( 0 );		// decay * 0.1
						MESSAGE_END( );
					}
				}
				else
				{
					pOther->TakeDamage( pev, pev, 100, DMG_ENERGYBEAM | DMG_NEVERGIB );
				}
			}
			else if ( pOther->pev->flags & FL_MONSTER )
			{
				if ( pOther->pev->health < 200 && !FClassnameIs( pOther->pev, "monster_nihilanth" )
					&& !FClassnameIs( pOther->pev, "monster_apache" ) && !FClassnameIs( pOther->pev, "monster_osprey" )
					&& !FClassnameIs( pOther->pev, "monster_superapache" ) && !FClassnameIs( pOther->pev, "monster_gargantua" )
					&& !FClassnameIs( pOther->pev, "monster_bigmomma" ))
				{
					pOther->Killed( pev, GIB_NEVER );
					pRemoveEnt = pOther;
				}
				else
				{
					pOther->TakeDamage( pev, pev, 100, DMG_ENERGYBEAM | DMG_NEVERGIB );
				}

			}
		}

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(pev->origin.x);	// X
			WRITE_COORD(pev->origin.y);	// Y
			WRITE_COORD(pev->origin.z);	// Z
			WRITE_BYTE( 32 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 96 );		// b
			WRITE_BYTE( 60 );		// time * 10
			WRITE_BYTE( 20 );		// decay * 0.1
		MESSAGE_END( );

		SetThink ( &CDisplacerBall::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.6;
	}

	LockRing = TRUE;
	pev->velocity = g_vecZero;
}
void CDisplacerBall :: SelfRemove ( void )
{
	pev->velocity = g_vecZero;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 640);
		WRITE_SHORT( m_iDispRing );
		WRITE_BYTE( 0 );	// startframe
		WRITE_BYTE( 10 );	// framerate
		WRITE_BYTE( 3 );	// life
		WRITE_BYTE( 20 );	// width
		WRITE_BYTE( 0 );	// noise
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 0 );	// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 32 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( 60 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

	SetThink ( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.3;
}
void CDisplacerBall::FlyThink( void )
{
	CBeam *pBeam;
	TraceResult tr;
	Vector vecDest;
	float flDist = 1.0;

	for (int i = 0; i < 10; i++)
	{
		Vector vecDir = Vector( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		vecDir = vecDir.Normalize();
		TraceResult         tr1;
		UTIL_TraceLine( pev->origin, pev->origin + vecDir * 1024, ignore_monsters, ENT(pev), &tr1 );
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	};

	if ( flDist == 1.0 ) return;

	pBeam = CBeam::BeamCreate("sprites/lgtning.spr",200);
	pBeam->PointEntInit( tr.vecEndPos, entindex() );
	pBeam->SetStartPos( tr.vecEndPos );
	pBeam->SetEndEntity( entindex() );
	pBeam->SetColor( 96, 128, 16 );
	pBeam->SetNoise( 65 );
	pBeam->SetBrightness( 255 );
	pBeam->SetWidth( 30 );
	pBeam->SetScrollRate( 35 );
	pBeam->LiveForTime( 1 );

	pev->frame += 1; //animate teleball

	if(pev->frame > 24)
		pev->frame = fmod( pev->frame, 24 );

	pev->nextthink = gpGlobals->time + 0.05;

}

void CDisplacerBall::ExplodeThink( void )
{
	pev->effects |= EF_NODRAW;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/displacer_teleport.wav", VOL_NORM, ATTN_NORM);

	if ( pRemoveEnt )
	{
		UTIL_Remove( pRemoveEnt );
	}

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;
		pev->owner = NULL;

	UTIL_Remove( this );
	::RadiusDamage( pev->origin, pev, pevOwner, gSkillData.plrDmgDisplacer, gSkillData.plrDisplacerRadius , CLASS_NONE, DMG_ENERGYBEAM );
}

//=======================
// Opposing Force Rope
//=======================

LINK_ENTITY_TO_CLASS( env_rope, CRope );
void CRope :: Spawn ( void )
{
	pev->effects |= EF_NODRAW;
	pev->flags |= FL_ALWAYSTHINK;
	Precache();
	Initialize();

	SetThink(&CRope::RopeFrame);
	pev->nextthink = gpGlobals->time + 1;
}

// Centripetal acceleration :D ( I love physics, to some degree, yeah )
void CRope :: RopeFrame ( void )
{
	pev->nextthink =  pev->ltime + 0.1;

	for ( int i = 0; i < m_iSegments; i++ )
	{
		TraceResult tr;
		Vector angles, origin, endpos;
		CRopeSegment *pRope = (CRopeSegment *)m_hSegments[i];

		g_engfuncs.pfnGetAttachment(ENT(pRope->pev), 0, endpos, angles );
		UTIL_TraceLine ( pRope->pev->origin, endpos,  dont_ignore_monsters, ENT(pev), &tr);

		if(tr.flFraction != 1)
		{
			float dot = DotProduct(pRope->pev->velocity, tr.vecPlaneNormal);
			VectorMA(pRope->pev->velocity, -dot*2.0f, tr.vecPlaneNormal,pRope->pev->velocity);

			Vector diff = pRope->pev->origin-tr.vecEndPos;
			VectorNormalize(diff);

			pRope->pev->velocity = pRope->pev->velocity * 0.9;
			pRope->pev->origin = tr.vecEndPos+diff*2;
		}

		if(tr.flFraction == 1.0)
		{
			pev->angles.z += 2;
		}

		if ( i != 0 )
		{
			CRopeSegment *pRopeParent = (CRopeSegment *)m_hSegments[i-1];
			g_engfuncs.pfnGetAttachment(ENT(pRopeParent->pev), 0, origin, angles );
			pRope->pev->origin = origin;
		}
		else
		{
			pRope->pev->origin = pev->origin;
		}
	}
}
void CRope :: Precache ( void )
{
	PRECACHE_MODEL( m_szEndingModel );
	PRECACHE_MODEL( m_szBodyModel );
}
void CRope :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "segments"))
	{
		m_iSegments = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "bodymodel"))
	{
		strcpy(m_szBodyModel, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "endingmodel"))
	{
		strcpy(m_szEndingModel, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "disable"))
	{
		m_fDisabled = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}
// Create all the rope segments.
void CRope :: Initialize ( void )
{
	for ( int i = 0; i < m_iSegments; i++ )
	{
		Vector	angles;
		int num = i-1;
		CRopeSegment *pSegment;
		angles.z = 270;
		if ( num = m_iSegments )
			pSegment = CRopeSegment::CreateSample( pev->origin, angles, m_szEndingModel, this );
		else
			pSegment = CRopeSegment::CreateSample( pev->origin, angles, m_szBodyModel, this );

		pSegment->pev->velocity.x = 0.5;
		m_hSegments[i] = pSegment;
	}
}
//=======================
// Opposing Force Rope
//=======================

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );
void CRopeSegment :: Spawn ( void )
{
	SET_MODEL(ENT(pev), m_szModel);
}
//=======================
// Opposing Force Rope Sample
//=======================
CRopeSegment *CRopeSegment::CreateSample( Vector vecOrigin, Vector vecAngles, char *m_BodyModel, CBaseEntity *pOwner )
{
	CRopeSegment *pSegment = GetClassPtr( (CRopeSegment *)NULL );

	UTIL_SetOrigin( pSegment->pev, vecOrigin );
	pSegment->pev->angles = vecAngles;
	pSegment->pev->owner = pOwner->edict();
	pSegment->m_szModel = m_BodyModel;
	pSegment->Spawn();

	return pSegment;
}
#endif