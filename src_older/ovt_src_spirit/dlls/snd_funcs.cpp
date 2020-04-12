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

#define MAX_CACHED_SOUNDS 4096
char	g_szPrecacheList[MAX_CACHED_SOUNDS][64];
int		g_iNumCached = NULL;
BOOL	g_bUpdateList = FALSE;

extern int gmsgEmitSound;
void EXEmitSound( edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{
	MESSAGE_BEGIN(MSG_ALL, gmsgEmitSound);
		WRITE_STRING(sample);
		WRITE_SHORT(ENTINDEX(entity));
		WRITE_COORD(volume*100);
		WRITE_COORD(attenuation*100);
		WRITE_BYTE(channel);
		WRITE_SHORT(fFlags);
		WRITE_BYTE(pitch);
	MESSAGE_END();
};

extern int gmsgEmitASound;
void EXEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch )
{
	MESSAGE_BEGIN(MSG_ALL, gmsgEmitASound, pos);
		WRITE_STRING(samp);
		WRITE_SHORT(ENTINDEX(entity));
		WRITE_COORD(vol*100);
		WRITE_COORD(attenuation*100);
		WRITE_SHORT(fFlags);
		WRITE_BYTE(pitch);
		WRITE_COORD(pos[0]);
		WRITE_COORD(pos[1]);
		WRITE_COORD(pos[2]);
	MESSAGE_END();
};

void SE_WorldInit( void )
{
	g_bUpdateList = FALSE;
	char szBspFilename[MAX_PATH];
	char szListFilename[MAX_PATH];
	
	strcpy ( szBspFilename, "maps/" );
	strcat ( szBspFilename, STRING(gpGlobals->mapname) );
	strcat ( szBspFilename, ".bsp" );

	strcpy ( szListFilename, "maps/" );
	strcat ( szListFilename, STRING(gpGlobals->mapname) );
	strcat ( szListFilename, "_soundlist.txt" );

	int iCompare;
	if (COMPARE_FILE_TIME(szBspFilename, szListFilename, &iCompare))
	{
		if(iCompare <= 0)
			return;
	}

	g_bUpdateList = TRUE;
}

void EXPrecacheSound( char *szFile ) 
{
	if(!g_bUpdateList)
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
	if(g_bUpdateList)
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
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgSEPrecache, NULL, pplayer->pev);
	MESSAGE_END();
}