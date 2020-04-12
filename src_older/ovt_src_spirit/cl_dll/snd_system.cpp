//================Copyright © 2013 Andrew Lucas, All rights reserved.================//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

//==============================
// Reckoning Sound Engine
//
// Programmed by Andrew Lucas
// Contains code from Xash and Richard Rohac, and Matthew Lapointe
//==============================

#include "windows.h"
#include "hud.h"
#include "cl_util.h"
#include <gl/glu.h>

#include "const.h"
#include "studio.h"
#include "entity_state.h"
#include "triangleapi.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_movevars.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <fstream> 
#include <iostream>

#include "com_model.h"
#include "r_efx.h"
#include "r_studioint.h"
#include "studio_util.h"
#include "snd_system.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
extern CGameStudioModelRenderer g_StudioRenderer;
extern engine_studio_api_t IEngineStudio;
#pragma comment(lib, "../common/al/openal32.lib")
#pragma comment(lib, "../common/vorbis/libvorbis.lib")
#pragma comment(lib, "../common/vorbis/libogg.lib")
// reverb set
EFXEAXREVERBPROPERTIES pEAXEffects [] = 
{
	EFX_REVERB_PRESET_ROOM, EFX_REVERB_PRESET_ROOM, 
	EFX_REVERB_PRESET_SEWERPIPE, EFX_REVERB_PRESET_SEWERPIPE, EFX_REVERB_PRESET_SEWERPIPE,
	EFX_REVERB_PRESET_HANGAR, EFX_REVERB_PRESET_HANGAR, EFX_REVERB_PRESET_HANGAR,
	EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR,
	EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR,
	EFX_REVERB_PRESET_UNDERWATER, EFX_REVERB_PRESET_UNDERWATER, EFX_REVERB_PRESET_UNDERWATER,
	EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR, EFX_REVERB_PRESET_STONECORRIDOR,
	EFX_REVERB_PRESET_OUTDOORS_VALLEY, EFX_REVERB_PRESET_OUTDOORS_VALLEY, EFX_REVERB_PRESET_OUTDOORS_VALLEY,
	EFX_REVERB_PRESET_ARENA, EFX_REVERB_PRESET_ARENA, EFX_REVERB_PRESET_ARENA,
	EFX_REVERB_PRESET_SEWERPIPE, EFX_REVERB_PRESET_SEWERPIPE, EFX_REVERB_PRESET_SEWERPIPE
};

/*
========================================
WatchThread

Code by Richard Rohac
========================================
*/

//Global thread handle
HANDLE g_hThreadHandle;

//Global event signalizing exit of thread
HANDLE g_hExitEvent;

//Global critical section
CRITICAL_SECTION g_CS;

//Global Half-Life's window handle
HWND g_hWnd = 0;

//Pointer structure passed to new thread
typedef struct thread_s
{
	s_active_t *psounds;
	void (*remove)( s_active_t *psound );

	s_music_t *music;
} thread_t;

thread_t sThreadData;

DWORD WINAPI WatchThread( LPVOID lpParam )
{
	thread_t *pThreadData = (thread_t*)lpParam;
	bool bAppStatus, bCleaned;

	while(1)
	{
		if(WaitForSingleObject(g_hExitEvent, 0) == WAIT_OBJECT_0)
		{
			ExitThread(0);
			return 0; //let's be sure
		}

		g_hWnd = FindWindow("Half-Life", NULL);
		if(!g_hWnd)
		{
			g_hWnd = FindWindow("Valve001", NULL);
			if(!g_hWnd)
				return 0;
		}

		LONG nStyle = GetWindowLongPtr(g_hWnd, GWL_STYLE);

		if(!(nStyle & WS_VISIBLE) || (nStyle & WS_MINIMIZE))
		{
			bAppStatus = false;
		}
		else
		{
			bCleaned = false;
			bAppStatus = true;
		}

		if(bAppStatus == false && bCleaned == false)
		{
			EnterCriticalSection(&g_CS);
			for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
			{
				if(pThreadData->psounds[i].playing)
				{
					pThreadData->remove(&pThreadData->psounds[i]);
				}
			}
			if(pThreadData->music->used)
			{
				ALenum state;
				alGetSourcei(pThreadData->music->source, AL_SOURCE_STATE, &state);
				if(state == AL_PLAYING)
				{
					alSourcePause(pThreadData->music->source);
				}
			}

			LeaveCriticalSection(&g_CS);
			bCleaned = true;
		}
	}

	return 0;
}

/*
====================
RemovePlayingWT

====================
*/
void RemovePlayingWT( s_active_t *sound )
{
	gSoundSystem.RemovePlaying(sound);
}
/*
====================
ScaleByte

====================
*/
inline void ScaleByte( DWORD *nInput )
{
	if(*nInput % 2 != 0)
	{
		*nInput += 1;
	}
}

/*
====================
ByteToShort

====================
*/
extern byte bLittleEndian[2];
short ByteToShort( byte *byte )
{
	if(*(short*)bLittleEndian == 1)
		return (byte[0]+(byte[1]<<8));
	else
		return (byte[1]+(byte[0]<<8));
}

// Detect litte/big endian
byte bLittleEndian[2] = {1, 0};
unsigned short ByteToUShort( byte *byte )
{
	if(*(short*)bLittleEndian == 1)
		return (byte[0]+(byte[1]<<8));
	else
		return (byte[1]+(byte[0]<<8));
}

unsigned int ByteToUInt( byte *byte )
{
	unsigned int iValue = byte[0];
	iValue += (byte[1]<<8);
	iValue += (byte[2]<<16);
	iValue += (byte[3]<<24);

	return iValue;
}

int ByteToInt( byte *byte )
{
	int iValue = byte[0];
	iValue += (byte[1]<<8);
	iValue += (byte[2]<<16);
	iValue += (byte[3]<<24);

	return iValue;
}

//==========================
//	MOD_PointInLeaf
//
//==========================
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t *node;
	float d;
	mplane_t *plane;

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}

/*
====================
Init

====================
*/
void CSoundSystem :: Init ( void )
{
	//
	// Load dll and create context
	//
	char szPath[64];
	strcpy(szPath, gEngfuncs.pfnGetGameDirectory());
	strcat(szPath, "/cl_dlls/openal32.dll");
	m_hOpenALDll = LoadLibrary(szPath);

	if(!m_hOpenALDll)
	{
		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, "FATAL ERROR: Failed to load cl_dlls/openal32.dll!.\n", "ERROR", MB_OK);
		exit(-1);
	}

	m_pDevice = alcOpenDevice(NULL);
	if(!m_pDevice)
	{
		FreeLibrary(m_hOpenALDll);
		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, "FATAL ERROR: Failed to open device!.\n", "ERROR", MB_OK);
		exit(-1);
	}

    m_pContext = alcCreateContext(m_pDevice, NULL);
    if(!m_pContext || !alcMakeContextCurrent(m_pContext))
    {
        if(m_pContext)
		{
			alcDestroyContext(m_pContext);
		}

		alcCloseDevice(m_pDevice);

		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, "FATAL ERROR: Failed to create context!.\n", "ERROR", MB_OK);
		exit(-1);
    }

	//
	// Load extended functions
	//

	alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
	alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
	alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
	alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
	alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");

	alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
	alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
	alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
	alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");

	if(!alGenEffects || !alDeleteEffects || !alEffecti || !alEffectf 
		|| !alEffectfv || !alGenAuxiliaryEffectSlots || !alDeleteAuxiliaryEffectSlots
		|| !alAuxiliaryEffectSloti || !alAuxiliaryEffectSlotf)
	{
		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, "FATAL ERROR: Failed to load OpenAL functions!.\n", "ERROR", MB_OK);
		exit(-1);
	}

	//
	// Everything else
	//
	m_pCvarVolume = gEngfuncs.pfnGetCvarPointer("volume");
	m_pCvarMusicVolume = gEngfuncs.pfnGetCvarPointer("MP3Volume");
	m_pCvarDebug = CVAR_CREATE("al_debug", "0", 0);

	LoadSentences();

	m_bReset = false;

	// Set distance model
	alDistanceModel(AL_LINEAR_DISTANCE);

	// Create the reverb slot
	alGenEffects(1, &m_uiReverbEffect);
	alEffecti(m_uiReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	alGenAuxiliaryEffectSlots(1, &m_uiEffectSlot);
	alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, m_uiReverbEffect);
	alAuxiliaryEffectSlotf(m_uiEffectSlot, AL_EFFECTSLOT_GAIN, 1.0);

	//
	// Watcher thread
	//
	// Setup watch thread
	InitializeCriticalSection(&g_CS);

	g_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!g_hExitEvent)
	{
		FreeLibrary(m_hOpenALDll);
		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, "FATAL ERROR: Failed to create thread.\n", "ERROR", MB_OK);
		exit(-1);
	}

	sThreadData.psounds = m_pActiveSounds;
	sThreadData.remove = RemovePlayingWT;
	sThreadData.music = &m_pMusicTrack;

	DWORD dwThreadID;
	g_hThreadHandle = CreateThread(NULL, 0, WatchThread, &sThreadData, 0, &dwThreadID);
}

/*
====================
VidInit

====================
*/
void CSoundSystem :: VidInit ( void )
{
	// Watcher functions
	m_bGotFrame = false;
	m_bReset = false;
	m_bGotEnts = false;
	m_bGotRender = false;

	// Set us up next frame
	m_bReloaded = true;
}

/*
====================
Shutdown

====================
*/
void CSoundSystem :: Shutdown ( void )
{
	// delete PHS data
	if(m_pPHS)
	{
		delete [] m_pPHS;
		m_pPHS = NULL;
	}

	// Kill music if it's playing
	if(m_pMusicTrack.used)
	{
		// clear everything
		alSourceStop(m_pMusicTrack.source);
		alDeleteSources(1, &m_pMusicTrack.source);
		alDeleteBuffers(2, m_pMusicTrack.buffers);

		memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
		ov_clear(&m_pMusicTrack.stream);
	}

	// Have the engine reset itself
	ResetEngine();

	// Clear reverb effect
	alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
	alDeleteAuxiliaryEffectSlots(1, &m_uiEffectSlot);
	alDeleteEffects(1, &m_uiReverbEffect);

#if 0 // OpenAL-Soft bug
	if(m_pContext)
	{
		alcMakeContextCurrent(NULL);
		alcDestroyContext(m_pContext);
	}

	if(m_pDevice)
	{
		alcCloseDevice(m_pDevice);
	}
#endif

	if(m_hOpenALDll)
	{
		FreeLibrary(m_hOpenALDll);
	}

	if(m_iNumSentences)
	{
		for(int i = 0; i < m_iNumSentences; i++)
		{
			sent_chunk_t *next = m_pSentences[i].chunks;
			while(next)
			{
				sent_chunk_t *free = next;
				next = free->next;
				delete [] free;
			}
		}
	}

	SetEvent(g_hExitEvent);
	WaitForSingleObject(g_hThreadHandle, 500);
	CloseHandle(g_hThreadHandle);
	CloseHandle(g_hExitEvent);

	DeleteCriticalSection(&g_CS);
}

/*
====================
ResetEngine

====================
*/
void CSoundSystem :: ResetEngine ( void )
{
	if(g_hThreadHandle)
	{
		EnterCriticalSection(&g_CS);
		// sources MUST be cleared BEFORE the cache can
		for(int i = 0; i < MAX_PLAYING_SOUNDS; i++)
		{
			if(m_pPlayingSounds[i].sound)
			{
				RemovePlaying(m_pPlayingSounds[i].sound);
			}
		}

		memset(m_pPlayingSounds, 0, sizeof(m_pPlayingSounds));
		memset(m_pActiveSounds, 0, sizeof(m_pActiveSounds));
		LeaveCriticalSection(&g_CS);
	}

	if(m_iNumCached)
	{
		for(int i = 0; i < m_iNumCached; i++)
		{
			alDeleteBuffers(1, &m_pCachedSounds[i].bufferindex);
			if(m_pCachedSounds[i].data)
			{
				delete [] m_pCachedSounds[i].data;
			}

		}
		memset(&m_pCachedSounds, 0, sizeof(m_pCachedSounds));
		m_iNumCached = 0;
	}

	if(m_pMusicTrack.used)
	{
		if(m_pMusicTrack.flags & MUSIC_LOOP)
		{
			alSourceStop(m_pMusicTrack.source);
			alDeleteSources(1, &m_pMusicTrack.source);
			alDeleteBuffers(2, m_pMusicTrack.buffers);

			memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
			ov_clear(&m_pMusicTrack.stream);
		}
		else
		{
			m_pMusicTrack.starttime = 0;
			alSourcePause(m_pMusicTrack.source);
		}
	}

	for(int i = 0; i < MAX_MUSIC_TRACKS; i++)
	{
		if(!m_pMusicCache[i].file)
			continue;

		if(m_pMusicTrack.used && (m_pMusicTrack.cache == &m_pMusicCache[i]))
			continue;

		fclose(m_pMusicCache[i].file);
		memset(&m_pMusicCache[i], 0, sizeof(oggcache_t));
	}
		

	m_iActiveReverb = 0;

	m_bReset = true;
	m_bGotFrame = false;
	m_bGotEnts = false;
	m_bGotRender = false;
}

/*
====================
W_HUDRame

====================
*/

void CSoundSystem::W_HUDFrame( void )
{
	if(!m_bReset && m_bGotFrame)
	{
		if(!m_bGotRender)
		{
			gEngfuncs.Con_DPrintf("Resetting from W_HUDRame\n");
			ResetEngine();
			return;
		}
		m_bGotRender = false;
	}
}

/*
====================
W_CreateEnts

====================
*/
void CSoundSystem::W_CreateEnts( void )
{
	m_bGotEnts = true;
	m_bGotRender = true;
}

/*
====================
W_DNormal

====================
*/
void CSoundSystem::W_DNormal( void )
{
	m_bGotFrame = true;
	m_bGotRender = true;

	// This can only happen during a levelchange
	if(!m_bReset && !m_bGotEnts)
	{
		gEngfuncs.Con_DPrintf("Resetting from W_DNormal\n");
		ResetEngine();
		return;
	}

	m_bGotEnts = false;

	if(m_pCvarDebug->value >= 1)
	{
		glDisable(GL_FOG);
		glDisable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);

		glEnable(GL_POINT_SMOOTH);
		glPointSize(15);

		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_pActiveSounds[i].used)
				continue;

			if(m_pActiveSounds[i].playing)
				glColor4f(0.0, 1.0, 0.0, 1.0);
			else
				glColor4f(1.0, 0.0, 0.0, 1.0);

			glBegin(GL_POINTS);
			glVertex3fv(m_pActiveSounds[i].origin);
			glEnd();
		}

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
	}
}

/*
====================
SetupEngine

====================
*/
void CSoundSystem::SetupEngine( void )
{
	if(!m_bReloaded)
		return;

	//
	// Delete previous data
	//
	if(m_pPHS)
	{
		delete [] m_pPHS;
		m_pPHS = NULL;
	}

	//
	// Set up PAS
	//
	model_t *world = IEngineStudio.GetModelByIndex(1);
	int num = world->numleafs + 1;
	int rowwords = (num + 31)>>5;
	int rowbytes = rowwords * 4;

	int *visofs = new int[num];
	byte *uncompressed_vis = new byte[rowbytes*num];
	byte *uncompressed_pas = new byte[rowbytes*num];
	byte *compressed_pas = new byte[rowbytes*num*4];

	byte *vismap, *vismap_p;
	vismap = vismap_p = compressed_pas;
	byte *scan = uncompressed_vis;

	for( int i = 0; i < num; i++, scan += rowbytes )
	{
		memcpy( scan, SE_LeafPVS( world->leafs + i, world ), sizeof(byte)*rowbytes );
	}

	int rowsize, total_size = 0;
	unsigned int *dest = (unsigned int *)uncompressed_pas;
	scan = uncompressed_vis;

	for( int i = 0; i < num; i++, dest += rowwords, scan += rowbytes )
	{
		memcpy( dest, scan, sizeof(byte)*rowbytes );

		for( int j = 0; j < rowbytes; j++ )
		{
			int bitbyte = scan[j];
			if( !bitbyte ) 
				continue;

			for( int k = 0; k < 8; k++ )
			{
				if(!( bitbyte & ( 1<<k )))
					continue;

				int index = ((j<<3) + k + 1);
				if( index >= num ) 
					continue;

				unsigned int *src = (unsigned int *)uncompressed_vis + index * rowwords;
				for( int l = 0; l < rowwords; l++ )
				{
					dest[l] |= src[l];
				}
			}
		}

		byte *comp = SE_CompressVis( (byte *)dest, &rowsize );
		visofs[i] = vismap_p - vismap; 
		total_size += rowsize;

		memcpy( vismap_p, comp, sizeof(byte)*rowsize );
		vismap_p += rowsize;
	}

	// Allocate final data array
	m_pPHS = new byte[total_size];
	memcpy(m_pPHS, compressed_pas, sizeof(byte)*total_size);

	// Use the unused "key" variable
	for( int i = 0; i < world->numleafs; i++ )
	{
		world->leafs[i].key = (int)(m_pPHS + visofs[i]);
	}

	delete [] compressed_pas;
	delete [] uncompressed_vis;
	delete [] visofs;

	//
	// Precache sounds from cached models
	//
	char szFile[64];
	for(int i = 0; i < 512; i++)
	{
		model_t *pmodel = IEngineStudio.GetModelByIndex(i+1);

		if(!pmodel)
			break;

		if(pmodel->type != mod_studio)
			continue;

		studiohdr_t *pHdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(pmodel);
		if(!pHdr)
			continue;

		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pHdr+pHdr->seqindex);
		for(int j = 0; j < pHdr->numseq; j++, pseqdesc++)
		{
			if(!pseqdesc->numevents)
				continue;

			mstudioevent_t *pevent = (mstudioevent_t *)((byte *)pHdr+pseqdesc->eventindex);
			for(int k = 0; k < pseqdesc->numevents; k++, pevent++)
			{
				if(pevent->event == 5004)
				{
					strcpy(szFile, "sound/");
					strcat(szFile, pevent->options);
					PrecacheSound(szFile);
				}
			}
		}
	}

	// done
	m_bReloaded = false;
}

/*
====================
ShouldPlay

====================
*/
bool CSoundSystem::ShouldPlay( s_active_t *sound, vec3_t vieworg, float frametime )
{
	if(!frametime)
		return false;

	if(!(sound->flags & SND_2D))
	{
		float dist = (vieworg-sound->origin).Length2D();
		if(dist > sound->radius)
			return false;

		if(sound->leafnum)
		{
			if(!(m_pPAS[sound->leafnum >> 3] & (1 << (sound->leafnum&7))))
				return false;
		}
	}

	return true;
}

/*
====================
AllocSound

====================
*/
s_active_t* CSoundSystem::AllocSound( const char *sample, int entindex, int flags, int channel )
{
	// Don't let tempents clutter up or more than one hud sound play
	if((flags & SND_TEMPENT) || (flags & SND_RADIO))
	{
		int numfound = 0;
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_pActiveSounds[i].used)
				continue;

			if(m_pActiveSounds[i].flags & SND_KILLME)
				continue;

			if(m_pActiveSounds[i].flags & (SND_TEMPENT|SND_RADIO))
			{
				if((m_pActiveSounds[i].flags & SND_TEMPENT) && (flags & SND_TEMPENT))
				{
					if(++numfound == MAX_ACTIVE_TEMP_SOUNDS)
					{
						return NULL;
					}
				}
				else if((m_pActiveSounds[i].flags & SND_RADIO) && (flags & SND_RADIO))
				{
					m_pActiveSounds[i].flags |= SND_KILLME;
					break;
				}
			}
		}
	}

	// See if we can clear on this ent
	if(entindex)
	{
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_pActiveSounds[i].used)
				continue;

			if(m_pActiveSounds[i].flags & SND_KILLME)
				continue;

			if(m_pActiveSounds[i].flags & SND_RADIO)
				continue;

			if((m_pActiveSounds[i].entindex == entindex) && (m_pActiveSounds[i].channel == channel))
			{
				m_pActiveSounds[i].flags |= SND_KILLME;
				break;
			}
		}
	}

	// Find a free slot
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_pActiveSounds[i].used)
		{
			return &m_pActiveSounds[i];
		}
	}

	// Clear an unimportant slot
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(m_pActiveSounds[i].flags & SND_RADIO)
			continue;

		if(m_pActiveSounds[i].channel == CHAN_VOICE)
			continue;

		KillSound(&m_pActiveSounds[i]);
		return &m_pActiveSounds[i];
	}

	return NULL;
}

/*
====================
RemovePlaying

====================
*/
void CSoundSystem::RemovePlaying( s_active_t *sound )
{
	if(!sound->playing)
		return;

	// Clear from OpenAL
	alSourceStop(sound->playing->sourceindex);
	alDeleteSources(1, &sound->playing->sourceindex);

	// Clear from system
	memset(sound->playing, 0, sizeof(s_playing_t));
	sound->playing = NULL;
}

/*
====================
KillSound

====================
*/
void CSoundSystem::KillSound( s_active_t *sound )
{
	// Shut mouth
	if(sound->sent)
	{
		sound->entity->mouth.mouthopen = 0;
	}

	// Remove from openal
	if(sound->playing)
	{
		RemovePlaying(sound);
	}

	// Clear data
	memset(sound, 0, sizeof(s_active_t));
}

/*
====================
CalcMouth8

====================
*/
void CSoundSystem::CalcMouth8( s_active_t *sound )
{
	if(sound->cache->channels > 1)
		return;

	s_cache_t *pcache = sound->cache;
	mouth_t *pmouth = &sound->entity->mouth;

	byte *pdata = pcache->data+pcache->dataoffset+sound->datapos;
	byte *pend = pcache->data+pcache->dataoffset+pcache->length;

	int mouthavg = 0;
	for(int i = 0; i < AVERAGE_SAMPLES_8BIT; i++)
	{
		if(pdata >= pend)
		{
			continue;
		}

		mouthavg += abs((*pdata)-127);
		pdata++;
	}

	mouthavg = (mouthavg-10)/AVERAGE_SAMPLES_8BIT;
	if(mouthavg < 0) mouthavg = 0;
	if(mouthavg > 255) mouthavg = 255;

	int lastval = pmouth->mouthopen;
	pmouth->mouthopen = ((lastval+mouthavg)/2);
}

/*
====================
CalcMouth16

====================
*/
void CSoundSystem::CalcMouth16( s_active_t *sound )
{
	if(sound->cache->channels > 1)
		return;

	s_cache_t *pcache = sound->cache;
	mouth_t *pmouth = &sound->entity->mouth;

	byte *pdata = pcache->data+pcache->dataoffset+sound->datapos;
	byte *pend = pcache->data+pcache->dataoffset+pcache->length;

	int mouthavg = 0;
	for(int i = 0; i < AVERAGE_SAMPLES_16BIT; i++)
	{
		if(pdata >= pend)
		{
			continue;
		}

		mouthavg += abs(ByteToShort(pdata));
		mouthavg = mouthavg*0.2;
		pdata+=2;
	}

	mouthavg = (mouthavg-10)/AVERAGE_SAMPLES_16BIT;
	if(mouthavg < 0) mouthavg = 0;
	if(mouthavg > 255) mouthavg = 255;

	int lastval = pmouth->mouthopen;
	pmouth->mouthopen = (lastval+mouthavg)/2;
}

/*
====================
PrecacheOgg

====================
*/
oggcache_t* CSoundSystem::PrecacheOgg( char *sample )
{
	for(int i = 0; i < MAX_MUSIC_TRACKS; i++)
	{
		if(!m_pMusicCache[i].file)
			continue;

		if(!strcmp(m_pMusicCache[i].name, sample))
			return &m_pMusicCache[i];
	}



	char szpath[128];
	strcpy(szpath, gEngfuncs.pfnGetGameDirectory());
	strcat(szpath, "/");
	strcat(szpath, sample);

	FILE *pfile = fopen(szpath, "rb");
	if(!pfile)
	{
		gEngfuncs.Con_Printf("PlayAudio: Failed to load %s.\n", sample);
		return NULL;
	}

	oggcache_t *pcache = NULL;
	for(int i = 0; i < MAX_MUSIC_TRACKS; i++)
	{
		if(!m_pMusicCache[i].file)
		{
			pcache = &m_pMusicCache[i];
			break;
		}
	}

	if(!pcache)
	{
		gEngfuncs.Con_Printf("PlayAudio: Cache full!\n");
		return NULL;
	}

	pcache->file = pfile;
	strcpy(pcache->name, sample);
	return pcache;
}

/*
====================
PrecacheSound

====================
*/
s_cache_t* CSoundSystem::PrecacheSound( char *sample )
{
	for(int i = 0; i < m_iNumCached; i++)
	{
		if(!strcmp(sample, m_pCachedSounds[i].name))
			return &m_pCachedSounds[i];
	}

	if(m_iNumCached == MAX_CACHED_SOUNDS)
	{
		gEngfuncs.Con_Printf("Sound cache full!\n");
		return NULL;
	}

	int isize = NULL;
	byte *pfile = gEngfuncs.COM_LoadFile(sample, 5, &isize);

	if(!pfile)
	{
		for(int i = 0; i < m_iNumMissing; i++)
		{
			if(!strcmp(sample, m_szMissing[i]))
				return NULL;
		}

		strcpy(m_szMissing[m_iNumMissing], sample);
		m_iNumMissing++;

		gEngfuncs.Con_Printf("Playsound: Could not precache: %s.\n", sample);
		return NULL;
	}

	if(strncmp((const char *)pfile, "RIFF", 4))
	{
		gEngfuncs.Con_Printf("Playsound: %s is not a valid .wav file!\n", sample);
		gEngfuncs.COM_FreeFile(pfile);
		return NULL;
	}

	s_cache_t *pcache = &m_pCachedSounds[m_iNumCached];
	m_iNumCached++;

	pcache->data = new byte[isize];
	memcpy(pcache->data, pfile, sizeof(byte)*isize);
	pcache->loopbegin = -1;
	strcpy(pcache->name, sample);

	byte *pbegin = (byte*)pfile + 12;
	byte *pend = (byte*)pfile + isize;

	while(1)
	{
		if(pbegin >= pend)
			break;

		DWORD ilength = ByteToInt(pbegin+4);
		ScaleByte(&ilength);

		if(!strncmp((const char*)pbegin, "fmt ", 4))
		{
			pcache->channels = ByteToUShort(pbegin+10);
			pcache->samplerate = ByteToInt(pbegin+12);
			pcache->bitspersample = ByteToUShort(pbegin+22);
		}

		if(!strncmp((const char*)pbegin, "cue ", 4))
		{
			pcache->loopbegin = ByteToInt(pbegin+32);
		}

		if(!strncmp((const char*)pbegin, "data", 4))
		{
			pcache->dataoffset = (pbegin+8)-pfile;
			pcache->length = ByteToInt(pbegin+4);
		}

		pbegin = pbegin + 8 + ilength;
	}

	int format = 0;
	if(pcache->channels == 1)
	{
		if(pcache->bitspersample == 8)
			format = AL_FORMAT_MONO8;
		else
			format = AL_FORMAT_MONO16;
	}
	else
	{
		if(pcache->bitspersample == 8)
			format = AL_FORMAT_STEREO8;
		else
			format = AL_FORMAT_STEREO16;
	}
		
	alGenBuffers(1, &pcache->bufferindex);
	alBufferData(pcache->bufferindex, format, pcache->data+pcache->dataoffset, pcache->length, pcache->samplerate);

	gEngfuncs.COM_FreeFile(pfile);
	return pcache;
}

/*
====================
CalcRefDef

====================
*/
void CSoundSystem::CalcRefDef( ref_params_t *pparams )
{
	//
	// Set up basics
	//
	SetupEngine();

	//
	// Set reverb
	//
	UpdateReverb(pparams);

	//
	// attempt to add any cached sounds
	//
	for(int i = 0; i < MAX_MSG_CACHE; i++)
	{
		if(!m_pMSGCache[i].used)
			continue;

		s_msg_cache_t *pmsg = &m_pMSGCache[i];
		cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(pmsg->entindex);
		if(!pEntity)
			continue;

		if(!pEntity->model)
			continue;

		PlaySound(pmsg->sample, NULL, pmsg->flags, pmsg->channel, pmsg->volume, pmsg->pitch, pmsg->atten, pEntity, pmsg->entindex);
		memset(&m_pMSGCache[i], 0, sizeof(s_msg_cache_t));
	}

	//
	// Set listener properties
	//
	float flOrientation[6];
	AngleVectors(pparams->viewangles, &flOrientation[0], NULL, &flOrientation[3]);
	alListenerfv(AL_ORIENTATION, flOrientation);
	alListenerfv(AL_POSITION, pparams->vieworg);
	alListenerf(AL_METERS_PER_UNIT, 0.01905f);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListenerf(AL_GAIN, 1);

	//
	// Set PHS/PAS
	//
	model_t *world = IEngineStudio.GetModelByIndex(1);
	mleaf_t *pleaf = Mod_PointInLeaf(pparams->vieworg, world); 
	m_pPAS = SE_LeafPAS(pleaf, world);

	//
	// Manage sounds
	//

	static float fllasttime = 0;
	float fltime = gEngfuncs.GetClientTime();
	float flframetime = fltime - fllasttime;
	fllasttime = fltime;

	if (flframetime > 1) flframetime = 1;
	if (flframetime < 0) flframetime = 0;

	cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
	int imsgnum = pplayer->curstate.messagenum;

	float flmultval = 1.0;
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_pActiveSounds[i].used)
			continue;

		if(m_pActiveSounds[i].flags & SND_RADIO)
		{
			flmultval = 0.1;
			break;
		}
	}

	EnterCriticalSection(&g_CS);
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_pActiveSounds[i].used)
			continue;

		if(m_pActiveSounds[i].flags & SND_KILLME)
		{
			KillSound(&m_pActiveSounds[i]);
			continue;
		}

		s_active_t *psound = &m_pActiveSounds[i];
		s_cache_t *pcache = psound->cache;
		float pitch = max(min(psound->pitch/(float)PITCH_NORM, 5.0), 0.5);

		//
		// advance if not paused
		if(flframetime)
		{
			if(psound->lasttime == -1)
			{
				psound->lasttime = fltime;
			}

			float flsndframetime = fltime - psound->lasttime;
			if (flsndframetime > 1) flsndframetime = 1;
			if (flsndframetime < 0) flsndframetime = 0;
			psound->lasttime = fltime;

			// Advance offset
			int bytepersec = pcache->channels * (pcache->samplerate*pitch) * (pcache->bitspersample>>3);
			psound->datapos += bytepersec*flsndframetime;
			int endpos = pcache->length;

			if(psound->sent && psound->chunk->end)
			{
				endpos = endpos*(psound->chunk->end/100.0f);
			}

			if(psound->datapos >= endpos)
			{
				if(pcache->loopbegin != -1)
				{
					// set position in openal too
					psound->datapos = pcache->loopbegin;
				}
				else
				{
					if(psound->chunk && psound->chunk->next)
					{
						// TODO: implement delay
						if(psound->playing)
						{
							RemovePlaying(psound);
						}

						psound->chunk = psound->chunk->next;
						psound->pitch = psound->pitch*(psound->chunk->pitch/100.0f);
						psound->volume = psound->volume*(psound->chunk->volume/100.0f);
						psound->datapos = pcache->length*(psound->chunk->start/100.0f);

						char sz[64];
						strcpy(sz, "sound/");
						strcat(sz, psound->sent->dir);
						strcat(sz, psound->chunk->name);
						strcat(sz, ".wav");
						pcache = PrecacheSound(sz);
						psound->cache = pcache;
					}
					else
					{
						// over, kill it
						KillSound(psound);
						continue;
					}
				}
			}
		}

		if(!psound->entity && !(psound->flags & SND_2D) && !(psound->flags & SND_OCCLUSIONLESS) && psound->leafnum == 0)
		{
			// update leaf we're on
			model_t *pworld = IEngineStudio.GetModelByIndex(1);
			mleaf_t *pleaf = Mod_PointInLeaf(psound->origin, pworld);

			if(pleaf->contents != CONTENTS_SOLID)
			{
				psound->leafnum = pleaf-pworld->leafs-1;
			}
			else
			{
				psound->leafnum = 0;
			}
		}

		//
		// update origin and leaf for entity tied sounds
		if(psound->entity && !(psound->flags & SND_2D) && psound->entity->curstate.messagenum == imsgnum)
		{
			model_t *pmodel = psound->entity->model;
			if(pmodel->type == mod_brush)
			{
				vec3_t vmins = psound->entity->origin + pmodel->mins;
				vec3_t vmaxs = psound->entity->origin + pmodel->maxs;
				VectorScale((vmins+vmaxs), 0.5, psound->origin);
			}
			else if(psound->entity == pplayer)
			{
				VectorCopy(pparams->vieworg, psound->origin);
			}
			else
			{
				if(psound->sent)
				{
					if(pcache->bitspersample == 16)
					{
						CalcMouth16(psound);
					}
					else
					{
						CalcMouth8(psound);
					}
				}

				VectorCopy(psound->entity->origin, psound->origin);
			}

			if(psound->entity != pplayer)
			{
				// update leaf we're on
				model_t *pworld = IEngineStudio.GetModelByIndex(1);
				mleaf_t *pleaf = Mod_PointInLeaf(psound->origin, pworld);

				if(pleaf->contents != CONTENTS_SOLID)
				{
					psound->leafnum = pleaf-pworld->leafs-1;
				}
				else
				{
					psound->leafnum = 0;
				}
			}
		}

		if(psound->playing)
		{
			if(ShouldPlay(psound, pparams->vieworg, flframetime))
			{
				if(!(psound->flags & SND_2D))
				{
					// only update for non-2d sounds
					alSourcefv(psound->playing->sourceindex, AL_POSITION, psound->origin);
					
				}

				if(m_iIdealReverb != m_iLastActiveReverb && !(psound->flags & SND_REVERBLESS))
				{
					if(m_iIdealReverb)
					{
						alSource3i(psound->playing->sourceindex, AL_AUXILIARY_SEND_FILTER, m_uiEffectSlot, 0, NULL);
					}
					else
					{
						alSource3i(psound->playing->sourceindex, AL_AUXILIARY_SEND_FILTER, AL_EFFECT_NULL, 0, NULL);
					}
				}

				float flgain = CalcGain(pparams->vieworg, psound, flmultval);
				alSourcef(psound->playing->sourceindex, AL_GAIN, flgain);
				alSourcef(psound->playing->sourceindex, AL_PITCH, pitch);
			}
			else
			{	
				RemovePlaying(psound);
				continue;
			}
		}
		else if(ShouldPlay(psound, pparams->vieworg, flframetime))
		{
			s_playing_t *pplaying = NULL;
			for(int i = 0; i < MAX_PLAYING_SOUNDS; i++)
			{
				if(!m_pPlayingSounds[i].sound)
				{
					pplaying = &m_pPlayingSounds[i];
					break;
				}
			}

			if(!pplaying)
			{
				gEngfuncs.Con_Printf("Sound system overflow!\n");
				continue;
			}

			// Tie to sound
			psound->playing = pplaying;
			pplaying->sound = psound;

			alGenSources(1, &pplaying->sourceindex);
			alSourcei(pplaying->sourceindex, AL_BUFFER, pcache->bufferindex);

			alSourcei(pplaying->sourceindex, AL_MAX_DISTANCE, psound->radius);
			alSourcei(pplaying->sourceindex, AL_REFERENCE_DISTANCE, psound->radius*0.2);
			alSourcei(pplaying->sourceindex, AL_BYTE_OFFSET, psound->datapos);
			alSourcef(pplaying->sourceindex, AL_PITCH, pitch);
			alSourcei(pplaying->sourceindex, AL_LOOPING, (pcache->loopbegin == -1) ? AL_FALSE : AL_TRUE);
			
			if(!(psound->flags & SND_REVERBLESS))
			{
				if(m_iIdealReverb)
				{
					alSource3i(psound->playing->sourceindex, AL_AUXILIARY_SEND_FILTER, m_uiEffectSlot, 0, NULL);
				}
				else
				{
					alSource3i(psound->playing->sourceindex, AL_AUXILIARY_SEND_FILTER, AL_EFFECT_NULL, 0, NULL);
				}
			}

			if(psound->flags & SND_2D)
			{
				alSourcei(pplaying->sourceindex, AL_ROLLOFF_FACTOR, 0);
				alSource3f(pplaying->sourceindex, AL_POSITION, 0, 0, 0);
				alSourcei(pplaying->sourceindex, AL_SOURCE_RELATIVE, AL_TRUE);
			}
			else
			{
				alSourcei(pplaying->sourceindex, AL_ROLLOFF_FACTOR, SE_ROLLOFF_FACTOR);
				alSourcefv(pplaying->sourceindex, AL_POSITION, psound->origin);
				alSourcei(pplaying->sourceindex, AL_SOURCE_RELATIVE, AL_FALSE);
			}

			float flgain = CalcGain(pparams->vieworg, psound, flmultval);
			alSourcef(pplaying->sourceindex, AL_GAIN, flgain);

			// Lastly tell it to play
			alSourcePlay(pplaying->sourceindex);
		}
	}
	LeaveCriticalSection(&g_CS);

	//	
	// Set this at end
	//
	if(m_iLastActiveReverb != m_iIdealReverb)
	{
		m_iLastActiveReverb = m_iIdealReverb;
	}
		
	//
	// Set up music
	//
	if(m_pMusicTrack.used)
	{
		if(!m_pMusicTrack.source)
		{
			if(ov_open_callbacks(m_pMusicTrack.cache->file, &m_pMusicTrack.stream, NULL, NULL, OV_CALLBACKS_NOCLOSE) < 0)
			{
				gEngfuncs.Con_Printf("PlayAudio: Decode error on %s.\n", m_pMusicTrack.cache->name);
				memset(&m_pMusicTrack, 0, sizeof(s_music_t));
				return;
			}

			m_pMusicTrack.starttime = fltime;
			m_pMusicTrack.info = ov_info(&m_pMusicTrack.stream, -1);
			if(m_pMusicTrack.info->channels == 1)
			{
				m_pMusicTrack.format = AL_FORMAT_MONO16;
			}
			else
			{
				m_pMusicTrack.format = AL_FORMAT_STEREO16;
			}

			// allocate source and buffers
			alGenSources(1, &m_pMusicTrack.source);
			alGenBuffers(2, m_pMusicTrack.buffers);

			alSource3f(m_pMusicTrack.source, AL_POSITION, 0.0, 0.0, 0.0);
			alSource3f(m_pMusicTrack.source, AL_VELOCITY, 0.0, 0.0, 0.0);
			alSource3f(m_pMusicTrack.source, AL_DIRECTION, 0.0, 0.0, 0.0);
			alSourcef(m_pMusicTrack.source, AL_ROLLOFF_FACTOR, 0.0);
			alSourcei(m_pMusicTrack.source, AL_SOURCE_RELATIVE, AL_TRUE);
			alSourcei(m_pMusicTrack.source, AL_LOOPING, (m_pMusicTrack.flags & MUSIC_LOOP) ? AL_TRUE:AL_FALSE);
		
			if(!Stream(m_pMusicTrack.buffers[0]) || !Stream(m_pMusicTrack.buffers[1]))
			{
				alDeleteSources(1, &m_pMusicTrack.source);
				alDeleteBuffers(2, m_pMusicTrack.buffers);

				memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
				ov_clear(&m_pMusicTrack.stream);
				return;
			}

			alSourceQueueBuffers(m_pMusicTrack.source, 2, m_pMusicTrack.buffers);
			alSourcePlay(m_pMusicTrack.source);
		}

#if 0 // TODO: clear the music if we've save-reloaded back to before it was playing
		if(m_pMusicTrack.starttime < fltime)
		{
			// clear everything
			alSourceStop(m_pMusicTrack.source);
			alDeleteSources(1, &m_pMusicTrack.source);
			alDeleteBuffers(2, m_pMusicTrack.buffers);

			memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
			ov_clear(&m_pMusicTrack.stream);
			return;
		}
#endif

		alSourcef(m_pMusicTrack.source, AL_GAIN, m_pCvarMusicVolume->value*flmultval);

		if(!flframetime)
		{
			alSourcePause(m_pMusicTrack.source);
			return;
		}

		ALenum state;
		alGetSourcei(m_pMusicTrack.source, AL_SOURCE_STATE, &state);
		if(state == AL_PAUSED)
		{
			alSourcePlay(m_pMusicTrack.source);
		}
		else if(state != AL_PLAYING)
		{
			// clear everything
			alSourceStop(m_pMusicTrack.source);
			alDeleteSources(1, &m_pMusicTrack.source);
			alDeleteBuffers(2, m_pMusicTrack.buffers);

			memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
			ov_clear(&m_pMusicTrack.stream);
			return;
		}

		int processed;
		alGetSourcei(m_pMusicTrack.source, AL_BUFFERS_PROCESSED, &processed);
		while(processed--)
		{
			ALuint buffer;
			alSourceUnqueueBuffers(m_pMusicTrack.source, 1, &buffer);
			bool result = Stream(buffer);
			alSourceQueueBuffers(m_pMusicTrack.source, 1, &buffer);

			if(!result)
			{
				alSourceStop(m_pMusicTrack.source);
				alDeleteSources(1, &m_pMusicTrack.source);
				alDeleteBuffers(2, m_pMusicTrack.buffers);

				memset(&m_pMusicTrack, 0, sizeof(m_pMusicTrack));
				ov_clear(&m_pMusicTrack.stream);
				return;
			}
		}
	}
}

/*
====================
Stream

====================
*/
bool CSoundSystem::Stream( ALuint buffer )
{
	static char data[BUFFER_SIZE];
	int size = 0;
	int section;
	int result;

	while(size < BUFFER_SIZE)
	{
		result = ov_read(&m_pMusicTrack.stream, data+size, BUFFER_SIZE-size, 0, 2, 1, &section);
		if(result > 0)
		{
			size += result;
		}
		else
		{
			break;
		}
	}

	if(size == 0)
	{
		return false;
	}

	alBufferData(buffer, m_pMusicTrack.format, data, size, m_pMusicTrack.info->rate);
	return true;
}

/*
====================
UpdateReverb

====================
*/
void CSoundSystem::UpdateReverb( ref_params_t *pparams )
{
	// Check for underwater
	m_iIdealReverb = m_iActiveReverb;
	if(pparams->waterlevel == 3)
	{
		m_iIdealReverb = 15;
	}

	if(!m_iIdealReverb)
		return;

	if(m_iIdealReverb == m_iLastActiveReverb)
		return;

	alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_DENSITY, pEAXEffects[m_iIdealReverb].flDensity);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_DIFFUSION, pEAXEffects[m_iIdealReverb].flDiffusion);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_GAIN, pEAXEffects[m_iIdealReverb].flGain);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_GAINHF, pEAXEffects[m_iIdealReverb].flGainHF);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_GAINLF, pEAXEffects[m_iIdealReverb].flGainLF);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_DECAY_TIME, pEAXEffects[m_iIdealReverb].flDecayTime);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_DECAY_HFRATIO, pEAXEffects[m_iIdealReverb].flDecayHFRatio);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_DECAY_LFRATIO, pEAXEffects[m_iIdealReverb].flDecayLFRatio);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_REFLECTIONS_GAIN, pEAXEffects[m_iIdealReverb].flReflectionsGain);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_REFLECTIONS_DELAY, pEAXEffects[m_iIdealReverb].flReflectionsDelay);
	alEffectfv(m_uiReverbEffect, AL_EAXREVERB_REFLECTIONS_PAN, pEAXEffects[m_iIdealReverb].flReflectionsPan);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, pEAXEffects[m_iIdealReverb].flLateReverbGain);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_LATE_REVERB_DELAY, pEAXEffects[m_iIdealReverb].flLateReverbDelay);
	alEffectfv(m_uiReverbEffect, AL_EAXREVERB_LATE_REVERB_PAN, pEAXEffects[m_iIdealReverb].flLateReverbPan);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_ECHO_TIME, pEAXEffects[m_iIdealReverb].flEchoTime);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_ECHO_DEPTH, pEAXEffects[m_iIdealReverb].flEchoDepth);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_MODULATION_TIME, pEAXEffects[m_iIdealReverb].flModulationTime);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_MODULATION_DEPTH, pEAXEffects[m_iIdealReverb].flModulationDepth);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, pEAXEffects[m_iIdealReverb].flAirAbsorptionGainHF);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_HFREFERENCE, pEAXEffects[m_iIdealReverb].flHFReference);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_LFREFERENCE, pEAXEffects[m_iIdealReverb].flLFReference);
	alEffectf(m_uiReverbEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, pEAXEffects[m_iIdealReverb].flRoomRolloffFactor);
	alEffecti(m_uiReverbEffect, AL_EAXREVERB_DECAY_HFLIMIT, pEAXEffects[m_iIdealReverb].iDecayHFLimit);
	alAuxiliaryEffectSloti(m_uiEffectSlot, AL_EFFECTSLOT_EFFECT, m_uiReverbEffect);
}

/*
====================
CalcGain

====================
*/
float CSoundSystem::CalcGain( vec3_t vieworg, s_active_t *sound, float multval )
{
	if(sound->flags & SND_RADIO)
	{
		return sound->volume*m_pCvarVolume->value;
	}

	if(sound->flags & SND_2D)
	{
		return sound->volume*multval*m_pCvarVolume->value;
	}
	else
	{
		float refdist = sound->radius*0.2;
		float distance = min((vieworg-sound->origin).Length(), sound->radius);
		float gain = (1-SE_ROLLOFF_FACTOR*(distance-refdist)/(sound->radius-refdist));
		return sound->volume*gain*multval*m_pCvarVolume->value;
	}
}

/*
====================
TempEntPlaySound

====================
*/
void CSoundSystem::TempEntPlaySound( TEMPENTITY *tempent, float volume )
{
	if(tempent->entity.origin == tempent->entity.prevstate.origin)
		return;

	tempent->entity.prevstate.origin = tempent->entity.origin;

	// Glass impact
	if(tempent->hitSound & BOUNCE_GLASS)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1: PlaySound("debris/glass1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("debris/glass2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("debris/glass3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}
	
	// Metal Impact
	if(tempent->hitSound & BOUNCE_METAL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 5))
		{
		case 1: PlaySound("debris/metal1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("debris/metal2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 3: PlaySound("debris/metal3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 4: PlaySound("debris/metal4.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("debris/metal5.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}

	// Flesh impact
	if(tempent->hitSound & BOUNCE_FLESH)
	{
		switch(gEngfuncs.pfnRandomLong(0, 6))
		{
		case 1: PlaySound("debris/flesh1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("debris/flesh2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 3: PlaySound("debris/flesh3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 4: PlaySound("debris/flesh4.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 5: PlaySound("debris/flesh5.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 6: PlaySound("debris/flesh6.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("debris/flesh7.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}

	// Wood impact
	if(tempent->hitSound & BOUNCE_WOOD)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1: PlaySound("debris/wood1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("debris/wood2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("debris/wood3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}

	// Shell impact
	if(tempent->hitSound & BOUNCE_SHELL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1: PlaySound("player/pl_shell2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("player/pl_shell3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("player/pl_shell1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}

	// Concrete impact
	if(tempent->hitSound & BOUNCE_CONCRETE)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1: PlaySound("debris/concrete1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("debris/concrete2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("debris/concrete3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}

	// Shotgun shell impact
	if(tempent->hitSound & BOUNCE_SHOTSHELL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1: PlaySound("weapons/sshell2.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		case 2: PlaySound("weapons/sshell3.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		default: PlaySound("weapons/sshell1.wav", tempent->entity.origin, SND_TEMPENT, CHAN_AUTO, volume, PITCH_NORM); break;
		}
	}
}

/*
====================
PlayOgg

====================
*/
void CSoundSystem::PlayOgg( char *sample, int flags )
{
	if(m_pMusicTrack.used)
	{
		alSourceStop(m_pMusicTrack.source);
		alSourceUnqueueBuffers(m_pMusicTrack.source, 2, m_pMusicTrack.buffers);
		alDeleteSources(1, &m_pMusicTrack.source);
		alDeleteBuffers(2, m_pMusicTrack.buffers);
		memset(&m_pMusicTrack, 0, sizeof(s_music_t));
	}

	if(!sample || sample[0] == 0)
		return;

	char szpath[64];
	strcpy(szpath, "sound/");
	strcat(szpath, sample);

	oggcache_t *pcache = PrecacheOgg(szpath);
	if(!pcache)
		return;

	m_pMusicTrack.used = true;
	m_pMusicTrack.cache = pcache;
#if 0
	m_pMusicTrack.flags = flags;
#endif
}

/*
====================
PlaySound

====================
*/
void CSoundSystem::PlaySound( char *sample, float *origin, int flags, int channel, float volume, int pitch, float attenuation, cl_entity_t *entity, int entindex )
{
	char szfile[128];
	sent_def_t *sent = NULL;
	if(sample[0] == '!')
	{
		int id = atoi(&sample[1]);
		sent = &m_pSentences[id];

		strcpy(szfile, "sound/");
		strcat(szfile, sent->dir);
		strcat(szfile, sent->chunks->name);
		strcat(szfile, ".wav");
	}
	else if(sample[0] == '*')
	{
		strcpy(szfile, "sound/");
		strcat(szfile, &sample[1]);
	}
	else
	{
		strcpy(szfile, "sound/");
		strcat(szfile, sample);
	}

	EnterCriticalSection(&g_CS);
	if((flags & SND_STOP) || (flags & SND_CHANGE_VOL) || (flags & SND_CHANGE_PITCH))
	{
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_pActiveSounds[i].used)
				continue;

			if(m_pActiveSounds[i].entindex == entindex
				&& m_pActiveSounds[i].channel == channel
				&& !strcmp(m_pActiveSounds[i].cache->name, szfile))
			{
				if(flags & SND_STOP)
				{
					KillSound(&m_pActiveSounds[i]);
					LeaveCriticalSection(&g_CS);
					return;
				}
				else if(flags & SND_CHANGE_PITCH)
				{
					m_pActiveSounds[i].pitch = pitch;
					LeaveCriticalSection(&g_CS);
					return;
				}
				else if(flags & SND_CHANGE_VOL)
				{
					m_pActiveSounds[i].volume = volume;
					LeaveCriticalSection(&g_CS);
					return;
				}
			}
		}
		LeaveCriticalSection(&g_CS);
		return;
	}

	s_cache_t *psample = PrecacheSound(szfile);
	if(!psample)
	{
		LeaveCriticalSection(&g_CS);
		return;
	}

	s_active_t *psound = AllocSound(szfile, entindex, flags, channel);
	if(!psound)
	{
		LeaveCriticalSection(&g_CS);
		return;
	}

	//
	// Only put it in the list now
	// The sound is played only later
	psound->volume = volume;
	psound->cache = psample;
	psound->channel = channel;
	psound->entindex = entindex;
	psound->entity = entity;
	psound->flags = flags;
	psound->pitch = pitch;
	psound->used = true;
	psound->lasttime = -1;
	psound->sent = sent;

	if(psound->sent)
	{
		psound->chunk = sent->chunks;
		psound->datapos = psample->length*(psound->chunk->start/100.0f);
		psound->volume = (psound->chunk->volume/100.0f)*psound->volume;
		psound->pitch = (psound->chunk->pitch/100.0f)*psound->pitch;
	}

	if(attenuation != ATTN_NONE)
	{
		if(attenuation >= ATTN_NORM)
		{
			psound->radius = MAX_DISTANCE + (1.0 - attenuation) * (0.5*MAX_DISTANCE);
		}
		else
		{
			psound->radius = MAX_DISTANCE + (1.0 - attenuation) * (4.0*MAX_DISTANCE);
		}
	
		if(psound->radius < 0)
		{
			psound->radius = 0;
		}
	}

	if(origin && psound->radius)
	{
		VectorCopy(origin, psound->origin);
	}

	if(!entity && !origin || !psound->radius)
	{
		psound->flags |= SND_2D;
	}

	if(!entity && !(psound->flags & SND_2D) && !(psound->flags & SND_OCCLUSIONLESS))
	{
		model_t *pworld = IEngineStudio.GetModelByIndex(1);
		if(pworld)
		{
			mleaf_t *pleaf = Mod_PointInLeaf(origin, pworld);
			if(pleaf->contents != CONTENTS_SOLID)
			{
				psound->leafnum = pleaf - pworld->leafs - 1;
			}
		}
	}

	LeaveCriticalSection(&g_CS);
}

/*
====================
StopSound

====================
*/
void CSoundSystem::StopSound( int entindex, int channel )
{
	EnterCriticalSection(&g_CS);
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_pActiveSounds[i].used)
			continue;

		if(m_pActiveSounds[i].entindex == entindex && m_pActiveSounds[i].channel == channel)
		{
			KillSound(&m_pActiveSounds[i]);
			LeaveCriticalSection(&g_CS);
			return;
		}
	}
	LeaveCriticalSection(&g_CS);
}

/*
====================
LoadSentences

====================
*/
void CSoundSystem::LoadSentences( void )
{
	if(m_iNumSentences)
		return;

	int iSize = 0;
	char *pFile = (char *)gEngfuncs.COM_LoadFile("sound/sentences.txt", 5, &iSize);

	if(!pFile || !iSize)
	{
		gEngfuncs.Con_Printf("ERROR: Couldn't load sentences.txt!");
		return;
	}

	int i = 0;
	while(1)
	{
		if(i >= iSize)
			break;

		if(pFile[i] == '\n' || pFile[i] == '\r')
		{
			i++;

			if(i >= iSize)
				break;

			continue;
		}

		// Skip whitelines
		if(pFile[i] == ' ')
		{
			while(1)
			{
				if(pFile[i] != ' ')
					break;

				if(pFile[i] == '\n')
				{
					i++;
					break;
				}

				i++;
			}
			continue;
		}

		if(pFile[i] == '/' || pFile[i] == '\\')
		{
			// Skip comments
			while(1)
			{
				if(i >= iSize)
					break;

				if(pFile[i] == '\n')
				{
					i++;
					break;
				}

				i++;
			}

			// Begin from start
			continue;
		}

		// If we got here, it means we found a valid entry
		sent_def_t *sentence = &m_pSentences[m_iNumSentences];
		sprintf(sentence->id, "!%i", m_iNumSentences); //Set ID
		m_iNumSentences++;

		int defaultpitch = 100;
		int defaulttime = 0;
		int defaultstart = 0;
		int defaultend = 0;
		int defaultvol = 100;

		// Skip sentence name
		while(1)
		{
			if(i >= iSize)
			{
				gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
				gEngfuncs.COM_FreeFile(pFile);
				return;
			}

			if(pFile[i] == ' ')
			{
				// Skip to next token
				while(1)
				{
					if(i >= iSize)
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
						gEngfuncs.COM_FreeFile(pFile);
						return;
					}

					if(pFile[i] != ' ')
						break;

					i++;
				}
				break;
			}
			i++;
		}

		int j = i;
		while(1)
		{
			if(j >= iSize)
			{
				gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
				gEngfuncs.COM_FreeFile(pFile);
				return;
			}

			if(pFile[j] == '\n' || pFile[j] == '\r')
			{
				// Seems Valve takes a default as "vox/"
				strcpy(sentence->dir, "vox/");
				break;
			}

			if(pFile[j] == '\\' || pFile[j] == '/')
			{
				j = 0;
				while(1)
				{
					if(i >= iSize)
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
						gEngfuncs.COM_FreeFile(pFile);
						return;
					}

					sentence->dir[j] = pFile[i];
					i++; j++;

					if(pFile[(i - 1)] == '\\' || pFile[(i - 1)] == '/')
					{
						sentence->dir[i] = 0;//terminate
						break;
					}
				}
				break;
			}

			j++;
		}

		//Lets set options up
		while(1)
		{
			//Skip white spaces
			while(1)
			{
				if(i >= iSize)
				{
					// Reached EOF
					break;
				}

				if(pFile[i] != ' ')
					break;

				i++;
			}

			if(i >= iSize)
			{
				// Reached EOF
				break;
			}

			if(pFile[i] == '\n' || pFile[i] == '\r')
			{
				i++;
				break;
			}

			sent_chunk_t *chunk = new sent_chunk_t;
			memset(chunk, 0, sizeof(sent_chunk_t));

			if(!sentence->chunks)
			{
				sentence->chunks = chunk;
			}
			else
			{
				sent_chunk_t *next = sentence->chunks;
				while(1)
				{
					if(!next->next)
					{
						next->next = chunk;
						break;
					}

					// move forward
					next = next->next;
				}
			}

			// Set defaults
			chunk->pitch = defaultpitch;
			chunk->start = defaultstart;
			chunk->end = defaultend;
			chunk->time = defaulttime;
			chunk->volume = defaultvol;
			chunk->delay = 0.0f;

			while(1)
			{
				// Skip whitespace
				while(1)
				{
					if(i >= iSize)
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
						gEngfuncs.COM_FreeFile(pFile);
						return;
					}

					if(pFile[i] != ' ')
						break;

					i++;
				}

				if(pFile[i] != '(')
					break;

				//If there's a bracelet before any sentence chunk, it means that it's defaulted for forthcoming options.
				i++; // Go into bracelet
				while(1)
				{
					if(i >= iSize)
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
						gEngfuncs.COM_FreeFile(pFile);
						return;
					}

					if(pFile[i] != ' ')
						break;

					// Go ahead
					i++;
				}

				if(pFile[i] == '\n' || pFile[i] == '\r')
				{
					gEngfuncs.Con_Printf("Sentences.txt error: Unexpected newline\n");
					return;
				}

				//Extract option parameters
				j = 0;
				while(1)
				{
					// Yes, this sucks
					while(1)
					{
						if(i >= iSize)
						{
							gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
							gEngfuncs.COM_FreeFile(pFile);
							return;
						}

						if(pFile[i] != ' ')
							break;

						i++;
					}

					if(pFile[i] == ')')
					{
						i++;
						break;
					}

					if(i >= iSize)
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
						gEngfuncs.COM_FreeFile(pFile);
						return;
					}

					char value[5];
					char *name = &pFile[i]; 
					i++; //Skip to value

					// Skip any whitespace(who knows)
					while(1)
					{
						if(i >= iSize)
						{
							gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
							gEngfuncs.COM_FreeFile(pFile);
							return;
						}

						if(pFile[i] != ' ')
							break;

						i++;
					}

					int j = 0;
					while(1)
					{
						// Skip whitespace
						while(1)
						{
							if(i >= iSize)
							{
								gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
								gEngfuncs.COM_FreeFile(pFile);
								return;
							}

							if(pFile[i] != ' ')
								break;

							i++;
						}

						if(i >= iSize)
						{
							gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
							gEngfuncs.COM_FreeFile(pFile);
							return;
						}

						if(pFile[i] != '0' && pFile[i] != '1' &&
							pFile[i] != '2' && pFile[i] != '3' &&
							pFile[i] != '4' && pFile[i] != '5' &&
							pFile[i] != '6' && pFile[i] != '7' &&
							pFile[i] != '8' && pFile[i] != '9')
						{
							value[j] = 0;
							break;
						}

						value[j] = pFile[i];
						i++; j++;
					}
					
					if(*name == 'p') chunk->pitch = defaultpitch = atoi(value);
					else if(*name == 't') chunk->time = defaulttime = atoi(value);
					else if(*name == 's') chunk->start = defaultstart = atoi(value);
					else if(*name == 'e') chunk->end = defaultend= atoi(value);
					else if(*name == 'v') chunk->volume = defaultvol = atoi(value);
				}
			}

			j = 0;
			while(1)
			{
				if(i >= iSize)
				{
					// Reached EOF
					break;
				}

				if(pFile[i] == ' ')
				{
					i++;
					break;
				}

				if(pFile[i] == '\n' || pFile[i] == '\r')
				{
					chunk->name[j] = 0;
					break;
				}

				// Sound params
				if(pFile[i] == '(')
				{
					i++; // Go into bracelet
					while(1)
					{
						if(i >= iSize)
						{
							gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
							gEngfuncs.COM_FreeFile(pFile);
							return;
						}

						if(pFile[i] != ' ')
							break;

						i++;
					}

					if(pFile[i] == '\n' || pFile[i] == '\r')
					{
						gEngfuncs.Con_Printf("Sentences.txt error: Unexpected newline\n");
						return;
					}

					//Extract option parameters
					j = 0;
					while(1)
					{
						// Yes, this sucks
						while(1)
						{
							if(i >= iSize)
							{
								gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
								gEngfuncs.COM_FreeFile(pFile);
								return;
							}

							if(pFile[i] != ' ')
								break;

							i++;
						}

						if(pFile[i] == ')')
						{
							i++;

							if(pFile[i] == ',')
								chunk->delay = 0.5; i++;
	
							break;
						}

						char value[5];
						char *name = &pFile[i]; 
						i++; //Skip to value

						// Skip any whiteline(who knows)
						while(1)
						{
							if(i >= iSize)
							{
								gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
								gEngfuncs.COM_FreeFile(pFile);
								return;
							}

							if(pFile[i] != ' ')
								break;

							i++;
						}

						int j = 0;
						while(1)
						{
							// Skip whitespace
							while(1)
							{
								if(i >= iSize)
								{
									gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
									gEngfuncs.COM_FreeFile(pFile);
									return;
								}

								if(pFile[i] != ' ')
									break;

								i++;
							}

							if(i >= iSize)
							{
								gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
								gEngfuncs.COM_FreeFile(pFile);
								return;
							}

							if(pFile[i] != '0' && pFile[i] != '1' &&
								pFile[i] != '2' && pFile[i] != '3' &&
								pFile[i] != '4' && pFile[i] != '5' &&
								pFile[i] != '6' && pFile[i] != '7' &&
								pFile[i] != '8' && pFile[i] != '9')
							{
								value[j] = 0;
								break;
							}

							value[j] = pFile[i];
							i++; j++;
						}
						
						if(*name == 'p') chunk->pitch = atoi(value);
						else if(*name == 't') chunk->time = atoi(value);
						else if(*name == 's') chunk->start = atoi(value);
						else if(*name == 'e') chunk->end = atoi(value);
						else if(*name == 'v') chunk->volume = atoi(value);
					}

					// Break main loop
					break;
				}

				if(pFile[i] == ',')
				{
					chunk->delay = 0.5; i++;
					continue;
				}

				if(i >= iSize)
				{
					gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
					gEngfuncs.COM_FreeFile(pFile);
					return;
				}

				chunk->name[j] = pFile[i];
				i++; j++;
			}
		}
	}
	
	// Free file
	gEngfuncs.COM_FreeFile(pFile);
}

/*
====================
CacheMessage

====================
*/
void CSoundSystem::CacheMessage( char *sample, int channel, int flags, int entindex, int pitch, float vol, float attn )
{
	char szfile[128];
	strcpy(szfile, "sound/");
	strcat(szfile, sample);
	s_cache_t *pcache = PrecacheSound(szfile);

	if(!pcache)
		return;

	if(pcache->loopbegin == -1)
		return;

	if(flags & SND_STOP)
	{
		for(int i = 0; i < MAX_MSG_CACHE; i++)
		{
			if(m_pMSGCache[i].channel == channel 
				&& m_pMSGCache[i].entindex == entindex
				&& !strcmp(m_pMSGCache[i].sample, sample))
			{
				memset(&m_pMSGCache[i], 0, sizeof(s_msg_cache_t));
				return;
			}
		}
		return;
	}

	s_msg_cache_t *pmsg = NULL;
	for(int i = 0; i < MAX_MSG_CACHE; i++)
	{
		if(!m_pMSGCache[i].used)
		{
			pmsg = &m_pMSGCache[i];
			break;
		}	
	}

	if(!pmsg)
		return;

	// add to cache
	pmsg->atten = attn;
	pmsg->channel = channel;
	pmsg->flags = flags;
	pmsg->entindex = entindex;
	pmsg->pitch = pitch;
	pmsg->volume = vol;
	pmsg->used = true;
	strcpy(pmsg->sample, sample);
}

/*
====================
MsgFunc_EmitSound

====================
*/
void CSoundSystem::MsgFunc_EmitSound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	
	char *sample = READ_STRING();
	int entindex = READ_SHORT();
	float volume = READ_COORD()/100.0f;
	float atten = READ_COORD()/100.0f;
	int channel = READ_BYTE();
	int flags = READ_SHORT();
	int pitch = READ_BYTE();

	cl_entity_t *entity = gEngfuncs.GetEntityByIndex(entindex);
	if(!entity || !entity->model)
	{	
		CacheMessage(sample, channel, flags, entindex, pitch, volume, atten);
		return;
	}

	PlaySound(sample, NULL, flags, channel, volume, pitch, atten, entity, entindex);
}


/*
====================
MsgFunc_EmitASound

====================
*/
void CSoundSystem::MsgFunc_EmitASound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);

	vec3_t origin;
	char *sample = READ_STRING();
	int entindex = READ_SHORT();
	float volume = READ_COORD()/100.0f;
	float atten = READ_COORD()/100.0f;
	int flags = READ_SHORT();
	int pitch = READ_BYTE();
	origin[0] = READ_COORD();
	origin[1] = READ_COORD();
	origin[2] = READ_COORD();

	PlaySound(sample, origin, flags, CHAN_AUTO, volume, pitch, atten, NULL, entindex);

	if(m_pCvarDebug->value >= 1)
	{
		gEngfuncs.Con_Printf("Called to play %s, vol: %f\n", sample, volume);
	}
}

/*
====================
MsgFunc_Precache

====================
*/
void CSoundSystem::MsgFunc_Precache( const char *pszName, int iSize, void *pbuf )
{
	char szfile[64];
	strcpy(szfile, gEngfuncs.pfnGetLevelName());
	strcpy(&szfile[(strlen(szfile)-4)], "_soundlist.txt");

	gEngfuncs.Con_Printf("Loading soundlist: %s.\n", szfile);

	int isize = 0;
	char *pfile = (char *)gEngfuncs.COM_LoadFile(szfile, 5, &isize);

	if(!pfile)
		return;

	char szsample[64];

	int length = 0;
	int offset = 0;
	while(1)
	{
		if(offset >= isize)
			break;

		if(pfile[offset] == '!')
		{
			length = 0;
			while(1)
			{
				szsample[length] = pfile[offset];
				offset++; length++;

				if(pfile[offset] == ';')
				{
					szsample[length] = 0;
					offset++;
					break;
				}
			}

			int id = atoi(&szsample[1]);
			sent_def_t *sent = &m_pSentences[id];
			sent_chunk_t *chunk = sent->chunks;
			while(chunk)
			{
				strcpy(szsample, "sound/");
				strcat(szsample, sent->dir);
				strcat(szsample, chunk->name);
				strcat(szsample, ".wav");
				PrecacheSound(szsample);
				chunk = chunk->next;
			}
		}
		else
		{
			int length = 6;
			strcpy(szsample, "sound/");
			while(1)
			{
				szsample[length] = pfile[offset];
				offset++; length++;

				if(pfile[offset] == ';')
				{
					szsample[length] = 0;
					offset++;
					break;
				}
			}

			if(!strcmp(&szsample[strlen(szsample)-4], ".ogg"))
			{
				PrecacheOgg(szsample);
			}
			else
			{
				PrecacheSound(szsample);
			}
		}
	}

	gEngfuncs.COM_FreeFile(pfile);
}

/*
====================
MsgFunc_PlayAudio

====================
*/
void CSoundSystem::MsgFunc_PlayAudio( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	char *string = READ_STRING();
	byte looped = READ_BYTE();

	PlayOgg(string, looped);
}

/*
====================
MsgFunc_RoomType

====================
*/
void CSoundSystem::MsgFunc_RoomType( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);
	m_iActiveReverb = READ_SHORT();
}

/*
===================
SE_DecompressVis
===================
*/
#define MAX_MAP_LEAFS			131068
byte *CSoundSystem :: SE_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->numleafs+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
===================
SE_LeafPAS
===================
*/
byte *CSoundSystem :: SE_LeafPAS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
		return SE_DecompressVis(NULL, model);

	return SE_DecompressVis ((byte *)leaf->key, model);
}

/*
===================
SE_LeafPVS
===================
*/
byte *CSoundSystem :: SE_LeafPVS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
		return SE_DecompressVis(NULL, model);

	return SE_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
SE_CompressVis
===================
*/
byte *CSoundSystem :: SE_CompressVis( const byte *in, int *size )
{
	int	j, rep;
	int	visrow;
	byte	*dest_p;
	static byte	visdata[MAX_MAP_LEAFS/8];

	model_t *world = IEngineStudio.GetModelByIndex(1);
	
	dest_p = visdata;
	visrow = (world->numleafs + 7)>>3;
	
	for( j = 0; j < visrow; j++ )
	{
		*dest_p++ = in[j];
		if( in[j] ) continue;

		rep = 1;
		for( j++; j < visrow; j++ )
		{
			if( in[j] || rep == 255 )
				break;
			else rep++;
		}
		*dest_p++ = rep;
		j--;
	}

	if( size ) *size = dest_p - visdata;

	return visdata;
}

/*
===================================
PM_PlaySample

===================================
*/
extern "C" void PM_PlaySound( int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{ 
	vec3_t origin = gEngfuncs.GetLocalPlayer()->origin;
	gSoundSystem.PlaySound( (char *)sample, origin, fFlags, channel, volume, pitch, attenuation );
}

/*
===================================
EV_PlaySound

===================================
*/
void EV_PlaySound ( int ent, float *origin, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{
	cl_entity_t *pentity = gEngfuncs.GetEntityByIndex(ent);
	gSoundSystem.PlaySound((char *)sample, origin, fFlags, channel, volume, pitch, attenuation, pentity, ent);
}

/*
===================================
PlaySound

===================================
*/
inline void PlaySound( char *szSound, float vol ) 
{
	cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
	gSoundSystem.PlaySound(szSound, NULL, NULL, CHAN_ITEM, vol, PITCH_NORM, ATTN_NONE, pplayer, pplayer->index); 
}

/*
===================================
PlaySound

===================================
*/
inline void PlaySound( int iSound, float vol ) 
{ 
	// oh fishsticks...
}