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
/*

===== h_export.cpp ========================================================

  Entity classes exported by Halflife.

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

#define MAX_CACHED_SOUNDS 4096
char	g_szPrecacheList[MAX_CACHED_SOUNDS][64];
int		g_iNumCached = NULL;
BOOL	g_bUpdateList = FALSE;

BOOL IsEntPlayer(edict_t *entity)//JIW
{
	CBaseEntity *pEntity = CBaseEntity::Instance( entity );
	if(entity && !entity->free && pEntity)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( ENTINDEX(entity) );
		if(pPlayer)
			return TRUE;
	}

	return FALSE;
}

void WRITE_FLOAT(float flFloat)
{
	if(flFloat == 0)
	{
		for (int i=0; i<4; i++)//4 empty bytes
		{
			WRITE_BYTE( 0 );
		}
		return;
	}

	BYTE pbytes[4];
	memcpy(pbytes, &flFloat, sizeof(flFloat));
	WRITE_BYTE( pbytes[0] );
	WRITE_BYTE( pbytes[1] );
	WRITE_BYTE( pbytes[2] );
	WRITE_BYTE( pbytes[3] );
}

extern int gmsgEmitSound;
void EXEmitSound( edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, int client )
{
	/*
	entindex
	channel
	flags

	sample

	vol
	attn
	pitch
	*/

	//JIW - make sure we're not sending a borked sample
	if(strlen( sample ) < 1)
		return;

	//ALERT(at_console,"s %s vol %f attn %f pitch %d\n",sample,volume,attenuation,pitch);

	CBaseEntity *pEntity = CBaseEntity::Instance( entity );

	if(fFlags & SND_STOP)
		MESSAGE_BEGIN(MSG_ALL, gmsgEmitSound);
	else if(client && IsEntPlayer(g_engfuncs.pfnPEntityOfEntIndex(client)))
		MESSAGE_BEGIN(MSG_ONE, gmsgEmitSound, NULL, g_engfuncs.pfnPEntityOfEntIndex(client));
//	else if(fFlags & SND_AMBIENT || fFlags & SND_2D)
//		MESSAGE_BEGIN(MSG_ALL, gmsgEmitSound);
//	else if(entity->v.flags & (FL_MONSTER | FL_CLIENT))//IsEntPlayer(entity))
//		MESSAGE_BEGIN(MSG_PVS, gmsgEmitSound, pEntity->Center() );
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgEmitSound);//MSG_PAS

		WRITE_SHORT(ENTINDEX(entity));
		WRITE_BYTE(channel);
		WRITE_SHORT(fFlags);

		if(fFlags & SND_STOP)//STOP THE SOUND
		{
			MESSAGE_END();
			return;
		}

		WRITE_STRING(sample);

	//	WRITE_COORD(volume*100);
	//	WRITE_COORD(attenuation*100);
		WRITE_FLOAT(volume);
		WRITE_FLOAT(attenuation);

		WRITE_BYTE(pitch);
	MESSAGE_END();
};

extern int gmsgEmitASound;
void EXEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch, int client )
{
	/*
	entindex
	flags

	sample

	vol
	attn
	pitch
	*/

	//JIW - make sure we're not sending a borked sample
	if(strlen( samp ) < 1)
		return;

	if(pos == g_vecZero)
		return;

	if(client && IsEntPlayer(g_engfuncs.pfnPEntityOfEntIndex(client)))
		MESSAGE_BEGIN(MSG_ONE, gmsgEmitASound, pos, g_engfuncs.pfnPEntityOfEntIndex(client));
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgEmitASound, pos);

//	ALERT(at_console,"%s %f %f %f\n", samp, pos[0],pos[1],pos[2]);

		WRITE_SHORT(ENTINDEX(entity));
		WRITE_SHORT(fFlags);

		if(fFlags & SND_STOP)//STOP THE SOUND
		{
			MESSAGE_END();
			return;
		}

		WRITE_STRING(samp);

	//	WRITE_COORD(vol*100);
	//	WRITE_COORD(attenuation*100);
		WRITE_FLOAT(vol);
		WRITE_FLOAT(attenuation);

		WRITE_BYTE(pitch);

		WRITE_COORD(pos[0]);
		WRITE_COORD(pos[1]);
		WRITE_COORD(pos[2]);
	MESSAGE_END();
};

//JIW - play footsteps for non-local players
extern "C" void PM_PlaySound( int idx, float *origin, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{ 
	idx = idx+1;//HACKHACK
	//Loop through all the players and send footstep sounds to all but the local player
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if(plr && (idx != plr->entindex()))
		{
			CBaseEntity *ent = NULL;
			while ( (ent = UTIL_FindEntityInSphere( ent, origin, 1024 )) != NULL )
			{
				if(ent == plr)
					EXEmitAmbientSound(g_engfuncs.pfnPEntityOfEntIndex(idx),origin,sample,volume,attenuation,fFlags,pitch,plr->entindex());
			}
		}
	}
}

void SE_WorldInit( void )
{
	g_iNumCached = NULL;
	//empty
}

void EXPrecacheSound( char *szFile ) 
{
	//JIW - make sure we're not sending a borked sample
	if(strlen( szFile ) < 1)
		return;

	for(int i = 0; i < g_iNumCached; i++)
	{
		if(!strcmp(g_szPrecacheList[i], szFile))
			return;
	}

	strcpy(g_szPrecacheList[g_iNumCached], szFile);
	g_iNumCached++;
}
extern int gmsgSEPrecache;
void SE_RunPrecache( CBaseEntity *pplayer )
{
	int filesize = NULL;
	for(int i = 0; i < g_iNumCached; i++)
		filesize += strlen(g_szPrecacheList[i])+1;

	char szListFilename[MAX_PATH];
	GET_GAME_DIR(szListFilename);
	strcat ( szListFilename, "/maps/" );
	strcat ( szListFilename, STRING(gpGlobals->mapname) );
	strcat ( szListFilename, "_soundlist.txt" );

	int offset = 0;
	byte *filedata = new byte[filesize];
	for(int i = 0; i < g_iNumCached; i++)
	{
		int length = strlen(g_szPrecacheList[i]);
		memcpy(filedata+offset, g_szPrecacheList[i], length);
		filedata[offset+length] = ';'; offset+=length+1;
	}

	FILE *pfile = fopen(szListFilename, "wb");
	fwrite(filedata, filesize, 1, pfile);
	fclose(pfile);

	g_iNumCached = NULL;
	memset(g_szPrecacheList, NULL, sizeof(g_szPrecacheList));

	MESSAGE_BEGIN(MSG_ONE, gmsgSEPrecache, NULL, pplayer->pev);
	MESSAGE_END();
}