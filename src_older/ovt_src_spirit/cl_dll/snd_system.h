//================Copyright © 2013 Andrew Lucas, All rights reserved.================//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

#ifndef SND_SYSTEM_H
#define SND_SYSTEM_H

//==============================
// Reckoning Sound Engine
//
// Programmed by Andrew Lucas
// Contains code from Xash and Richard Rohac, and Matthew Lapointe
//==============================

#include "ref_params.h"
#include "parsemsg.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "const.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include <windows.h>

#include "al/al.h"
#include "al/alc.h"
#include "al/alext.h"
#include "al/efx.h"
#include "al/efx-creative.h"
#include "al/efx-presets.h"
#include "vorbis/vorbisfile.h"

#include "com_model.h"

#define MAX_ACTIVE_SOUNDS		128
#define MAX_PLAYING_SOUNDS		64
#define MAX_ACTIVE_TEMP_SOUNDS	4
#define MIN_DISTANCE			32
#define MAX_DISTANCE			1024
#define MAX_MUSIC_TRACKS		4
#define MAX_MSG_CACHE			16
#define MAX_SENTENCES			4096
#define MAX_CACHED_SOUNDS		2048

#define NUM_REVERBS				29
#define REVERB_BLEND_TIME		2
#define SE_ROLLOFF_FACTOR		1
#define BUFFER_SIZE				(4096 * 32)

#define AVERAGE_SAMPLES_8BIT	40
#define AVERAGE_SAMPLES_16BIT	80

// Channel defines
#define CHAN_AUTO				0			// low priority, common sound
#define CHAN_WEAPON				1			// highest priority, weapon sound
#define	CHAN_VOICE				2			// highest priority, sentence sound
#define CHAN_ITEM				3			// low priority
#define	CHAN_BODY				4			// highest priority, replacement of snd_2d
#define CHAN_STREAM				5			// allocate stream channel from the static or dynamic area
#define CHAN_STATIC				6			// allocate channel from the static area 
#define CHAN_NETWORKVOICE_BASE	7			// voice data coming across the network
#define CHAN_NETWORKVOICE_END	500			// network voice data reserves slots (CHAN_NETWORKVOICE_BASE through CHAN_NETWORKVOICE_END).

#define SND_SPAWNING		(1<<8)		// duplicated in protocol.h we're spawing, used in some cases for ambients 
#define SND_STOP			(1<<5)		// duplicated in protocol.h stop sound
#define SND_CHANGE_VOL		(1<<6)		// duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

// Custom flags
#define SND_2D				(1<<9)
#define SND_TEMPENT			(1<<10)
#define SND_RADIO			(1<<11)
#define SND_KILLME			(1<<12)
#define SND_OCCLUSIONLESS	(1<<13)
#define SND_REVERBLESS		(1<<14)

#define MUSIC_LOOP			(1<<1)	

struct s_cache_t
{
	char name[64];
	long length;

	int samplerate;
	int dataoffset;
	short bitspersample;
	short channels;
	long loopbegin;

	ALuint bufferindex;
	byte *data;
};

struct sent_chunk_t
{
	char name[64];
	float delay;

	int pitch; // Frequency
	int start; // Start sound at iStart % of whole length
	int end; // Stop sound at iEnd % of whole length
	int volume; // Play sound at iVolume % of default folume (1.0)
	int time; // Delay between words (0 = full delay, 100 = no delay)

	sent_chunk_t *next;
};

struct sent_def_t
{
	char id[64];
	char dir[64];

	sent_chunk_t *chunks;
};

struct s_active_t
{
	bool used;

	int leafnum;
	float lasttime;

	s_cache_t *cache;

	sent_def_t *sent;
	sent_chunk_t *chunk;

	cl_entity_t *entity;
	int entindex;

	int datapos;

	byte channel;
	byte pitch;
	int flags;

	float volume;
	float radius;
	
	vec3_t origin;

	struct s_playing_t *playing;
};

struct s_playing_t
{
	s_active_t *sound;
	ALuint sourceindex;
};

struct oggcache_t
{
	char name[64];
	FILE *file;
};

struct s_music_t
{
	bool used;
	ALuint source;
	int flags;

	ALuint buffers[2];
	oggcache_t *cache;

	OggVorbis_File stream;
	vorbis_info	*info;

	ALenum format;

	float starttime;
};

struct s_msg_cache_t
{
	bool used;
	char sample[64];
	int entindex;
	int channel;
	int flags;
	float volume;
	float pitch;
	float atten;
};

extern inline void PlaySound( char *szSound, float vol = VOL_NORM );
extern inline void PlaySound( int iSound, float vol = VOL_NORM );
extern inline void EV_PlaySound ( int ent, float *origin, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch );
/*
====================
CSoundSystem

====================
*/
class CSoundSystem
{
public:
	void Init( void );
	void VidInit( void );
	void Shutdown( void );

	void W_HUDFrame( void );
	void W_CreateEnts( void );
	void W_DNormal( void );

	void CalcRefDef( ref_params_t *pparams );

	void PlaySound( char *sample, float *origin = NULL, int flags = 0, int channel = CHAN_AUTO, float volume = VOL_NORM, int pitch = PITCH_NORM, float attenuation = ATTN_NORM, cl_entity_t *entity = 0, int entindex = 0 );
	void TempEntPlaySound( TEMPENTITY *tempent, float volume );
	void StopSound( int entindex, int channel );
	void RemovePlaying( s_active_t *sound );

	void MsgFunc_PlayAudio( const char *pszName, int iSize, void *pbuf );
	void MsgFunc_EmitSound( const char *pszName, int iSize, void *pbuf );
	void MsgFunc_EmitASound( const char *pszName, int iSize, void *pbuf );
	void MsgFunc_Precache( const char *pszName, int iSize, void *pbuf );
	void MsgFunc_RoomType( const char *pszName, int iSize, void *pbuf );

private:
	void LoadSentences( void );
	void ResetEngine( void );
	void SetupEngine( void );
	void UpdateReverb( ref_params_t *pparams );

	void CacheMessage( char *sample, int channel, int flags, int entindex, int pitch, float vol, float attn );

	float CalcGain( vec3_t vieworg, s_active_t *sound, float multval );
	bool ShouldPlay( s_active_t *sound, vec3_t vieworg, float frametime );
	s_active_t *AllocSound( const char *sample, int entindex, int flags, int channel );
	void KillSound( s_active_t *sound );

	void CalcMouth8( s_active_t *sound );
	void CalcMouth16( s_active_t *sound );

	void PlayOgg( char *sample, int flags );
	bool Stream( ALuint buffer );

	oggcache_t* PrecacheOgg( char *sample );
	s_cache_t* PrecacheSound( char *sample );

	byte *SE_DecompressVis( byte *in, model_t *model );
	byte *SE_LeafPAS( mleaf_t *leaf, model_t *model );
	byte *SE_LeafPVS( mleaf_t *leaf, model_t *model );
	byte *SE_CompressVis( const byte *in, int *size );

private:
	bool			m_bReset;
	bool			m_bGotFrame;
	bool			m_bGotEnts;
	bool			m_bGotRender;

	bool			m_bReloaded;

	s_playing_t		m_pPlayingSounds[MAX_PLAYING_SOUNDS];
	s_active_t		m_pActiveSounds[MAX_ACTIVE_SOUNDS];

	sent_def_t		m_pSentences[MAX_SENTENCES];
	int				m_iNumSentences;

	char			m_szMissing[MAX_CACHED_SOUNDS][64];
	int				m_iNumMissing;

	byte			*m_pPHS;
	byte			*m_pPAS;

	ALuint			m_uiReverbEffect;
	ALuint			m_uiEffectSlot;
	int				m_iActiveReverb;
	int				m_iLastActiveReverb;
	int				m_iIdealReverb;

	cvar_t			*m_pCvarVolume;
	cvar_t			*m_pCvarMusicVolume;
	cvar_t			*m_pCvarDebug;

	oggcache_t		m_pMusicCache[MAX_MUSIC_TRACKS];
	s_music_t		m_pMusicTrack;

	s_msg_cache_t	m_pMSGCache[MAX_MSG_CACHE];

public:
	s_cache_t		m_pCachedSounds[MAX_CACHED_SOUNDS];
	int				m_iNumCached;

private:
	HMODULE			m_hOpenALDll;

	ALCdevice		*m_pDevice;
	ALCcontext		*m_pContext;

	LPALGENEFFECTS					alGenEffects;
	LPALDELETEEFFECTS				alDeleteEffects;
	LPALEFFECTI						alEffecti;
	LPALEFFECTF						alEffectf;
	LPALEFFECTFV					alEffectfv;

	LPALGENAUXILIARYEFFECTSLOTS		alGenAuxiliaryEffectSlots;
	LPALDELETEAUXILIARYEFFECTSLOTS	alDeleteAuxiliaryEffectSlots;
	LPALAUXILIARYEFFECTSLOTI		alAuxiliaryEffectSloti;
	LPALAUXILIARYEFFECTSLOTF		alAuxiliaryEffectSlotf;
};
extern CSoundSystem gSoundSystem;
#endif