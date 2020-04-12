#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( weapon_powerglove, CPowerGlove );

enum gauss_e {
	POWERGLOVE_IDLE = 0,
	POWERGLOVE_FIDGET1,
	POWERGLOVE_FIDGET2,
	POWERGLOVE_PUNCHRIGHT,
	POWERGLOVE_PUNCHLEFT,
	POWERGLOVE_PUNCHBOTH,
	POWERGLOVE_DEPLOY,
	POWERGLOVE_HOLSTER
};


void CPowerGlove::Spawn( )
{
	Precache( );

	// No model or physics
	m_iId = WEAPON_POWERGLOVE;
	m_iClip = -1;

	SetTouch(&CBasePlayerItem :: DefaultTouch);
}


void CPowerGlove::Precache( void )
{
	PRECACHE_MODEL("models/v_powerglove.mdl");

	PRECACHE_SOUND("weapons/fists_miss1.wav");
	PRECACHE_SOUND("weapons/fists_miss2.wav");
	PRECACHE_SOUND("weapons/fists_miss3.wav");
	PRECACHE_SOUND("weapons/fists_hit_r1.wav");
	PRECACHE_SOUND("weapons/fists_hit_r2.wav");
	PRECACHE_SOUND("weapons/fists_hit_r3.wav");
	PRECACHE_SOUND("weapons/fists_hit_l1.wav");
	PRECACHE_SOUND("weapons/fists_hit_l2.wav");
	PRECACHE_SOUND("weapons/fists_hit_l3.wav");
	PRECACHE_SOUND("weapons/fists_idle.wav");
}

int CPowerGlove::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_POWERGLOVE;
	p->iWeight = POWERGLOVE_WEIGHT;
	return 1;
}

int CPowerGlove::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CPowerGlove::Deploy( )
{
	BOOL bRet = DefaultDeploy( "models/v_powerglove.mdl", NULL, POWERGLOVE_DEPLOY, "crowbar" );

	if(bRet)
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.46;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.46;
	}

	return bRet;
}

void CPowerGlove::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.4;
	SendWeaponAnim( POWERGLOVE_HOLSTER );
}

void CPowerGlove::PrimaryAttack()
{
	switch(RANDOM_LONG(0, 2))
	{
		case 0: 
			SendWeaponAnim(POWERGLOVE_PUNCHRIGHT);
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
			m_flPunchTime = UTIL_WeaponTimeBase() + 0.064;
			m_iFistOrder = 1;
			break;
		case 1:
			SendWeaponAnim(POWERGLOVE_PUNCHLEFT);
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
			m_flPunchTime = UTIL_WeaponTimeBase() + 0.09;
			m_iFistOrder = 2;
			break;
		case 2:
			SendWeaponAnim(POWERGLOVE_PUNCHBOTH);
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
			m_flPunchTime = UTIL_WeaponTimeBase() + 0.17;
			m_iFistOrder = 3;
			break;
	}

	switch(RANDOM_LONG(0, 2))
	{
		case 0: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_miss1.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
		case 1: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_miss2.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
		case 2: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_miss3.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CPowerGlove::ItemPostFrame( void )
{
	if(m_iFistOrder && (m_flPunchTime <= UTIL_WeaponTimeBase()))
	{
		TraceResult tr;

		UTIL_MakeVectors (m_pPlayer->pev->v_angle);
		Vector vecSrc	= m_pPlayer->GetGunPosition( );
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

		if ( tr.flFraction >= 1.0 )
		{
			UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
			if ( tr.flFraction < 1.0 )
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
				if ( !pHit || pHit->IsBSPModel() )
					FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
				vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
			}
		}

		if(tr.flFraction != 1.0)
		{
			ClearMultiDamage( );
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgPowerGlove, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if(m_iFistOrder == 1 || m_iFistOrder == 3)
			{
				switch(RANDOM_LONG(0, 2))
				{
					case 0: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_r1.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
					case 1: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_r2.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
					case 2: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_r3.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
				}
			}
			else
			{
				switch(RANDOM_LONG(0, 2))
				{
					case 0: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_l1.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
					case 1: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_l2.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
					case 2: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_hit_l3.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
				}
			}

		}

		// next is left punch
		if( m_iFistOrder == 3 )
		{
			switch(RANDOM_LONG(0, 2))
			{
				case 0: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_BODY, "weapons/fists_miss1.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
				case 1: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_BODY, "weapons/fists_miss2.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
				case 2: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_BODY, "weapons/fists_miss3.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM); break;
			}

			m_flPunchTime = UTIL_WeaponTimeBase() + 0.27;
			m_iFistOrder = 2;
		}
		else
		{
			m_iFistOrder = NULL;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CPowerGlove::WeaponIdle( void )
{
	int iAnim;
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.5)
		{
			iAnim = POWERGLOVE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
		}
		else if (flRand > 0.5 && flRand <= 0.75)
		{
			iAnim = POWERGLOVE_FIDGET1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);
		}
		else
		{
			iAnim = POWERGLOVE_FIDGET2;
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/fists_idle.wav", VOL_NORM, ATTN_NORM, NULL, PITCH_NORM);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSequenceTime(iAnim);

		}
		SendWeaponAnim( iAnim );
	}
}