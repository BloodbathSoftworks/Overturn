//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe
	BULLET_PLAYER_KNIFE, // knife swipe
	BULLET_PLAYER_556, // saw
	BULLET_PLAYER_762, //sniper
	BULLET_PLAYER_EAGLE, //eagle

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

enum stakeout_e {
	STAKEOUT_IDLE1 = 0,
	STAKEOUT_IDLE2,
	STAKEOUT_FIRE,
	STAKEOUT_FIRE_SEMI,
	STAKEOUT_RELOAD,
	STAKEOUT_RELOAD_PUMP,
	STAKEOUT_RELOAD_START,
	STAKEOUT_RELOAD_END,
	STAKEOUT_DEPLOY,
	STAKEOUT_DEPLOY_FIRST,
	STAKEOUT_HOLSTER
};

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_HOLSTER,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};

enum m16a2_e
{
	M16A2_LONGIDLE = 0,
	M16A2_IDLE1,
	M16A2_RELOAD,
	M16A2_DEPLOY,
	M16A2_HOLSTER,
	M16A2_FIRE1,
	M16A2_FIRE2,
	M16A2_FIRE3,
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

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e {
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_IDLE5,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_NOT_EMPTY,
	DEAGLE_DRAW,
	DEAGLE_HOLSTER,
};

enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE1,
	M249_RELOAD,
	M249_RELOAD2,
	M249_HOLSTER,
	M249_DRAW,
	M249_FIRE1,
	M249_FIRE2,
	M249_FIRE3,
};
enum sniper_e {
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE1,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER,
};

enum displacer_e 
{
    DISPLACER_IDLE1 = 0,
    DISPLACER_IDLE2,
    DISPLACER_SPINUP,
    DISPLACER_SPIN,
    DISPLACER_FIRE,
    DISPLACER_DRAW,
    DISPLACER_HOLSTER,
};

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );

#endif // EV_HLDMH