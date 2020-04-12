//===========Copyright © 2010 - 2011, Richard Roh·Ë All rights reserved.=============//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

#if !defined ( SOUNDENGINE_H )
#define SOUNDENGINE_H
#if defined( _WIN32 )
#pragma once
#endif

#include <windows.h>
#undef SND_LOOP

#include <iostream>
#include <vector>

#include "fmod/fmod.h"
#include "fmod/fmod_errors.h"

#include "ref_params.h"
#include "parsemsg.h"
#include "cl_entity.h"
#include "r_efx.h"

#define MAX_ACTIVE_SOUNDS		128
#define	MAX_CACHED_SOUNDS		4096
#define MAX_ACTIVE_TEMP_SOUNDS	5
#define MAX_SENTENCES			4096

// Default rolloff factors
#define MIN_DISTANCE	32
#define MAX_DISTANCE	1024

// Extended sound defines
#define SND_STOP			(1<<5)
#define SND_CHANGE_VOL		(1<<6)
#define SND_CHANGE_PITCH	(1<<7)
#define SND_SPAWNING		(1<<8)
#define SND_LOOP			(1<<9)
#define SND_AMBIENT			(1<<10)
#define SND_SENTENCE		(1<<11)
#define SND_2D				(1<<12)
#define SND_HUD				(1<<13)
#define SND_MUSIC			(1<<14)
#define SND_TEMPENT			(1<<15)
#define SND_RELATIVE		(1<<16)
#define SND_RADIO			(1<<17)

// Colliding temp entity sounds

#ifndef BOUNCE_GLASS
#define BOUNCE_GLASS		0x01
#define	BOUNCE_METAL		0x02
#define BOUNCE_FLESH		0x04
#define BOUNCE_WOOD			0x08
#define BOUNCE_SHRAP		0x10
#define BOUNCE_SHELL		0x20
#define	BOUNCE_CONCRETE		0x40
#define BOUNCE_SHOTSHELL	0x80
#endif

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

struct scache_t
{
	byte *pFile;
	int	  iSize;
	char  szFile[256];

	int iSampleRate;
	int iBitCount;
	int pDataOffset;

	int iLoopStart;
	int iLoopEnd;
};

struct soption_t
{
	char szFile[64];
	float flDelay;

	int iPitch; // Frequency
	int iStart; // Start sound at iStart % of whole length
	int iEnd; // Stop sound at iEnd % of whole length
	int iVolume; // Play sound at iVolume % of default folume (1.0)
	int iTime; // Delay between words (0 = full delay, 100 = no delay)
};

struct sentence_t
{
	char szID[64];
	char szParentDir[64];
	std::vector<soption_t> pOptions;
};

struct sound_t
{
	FMOD_SOUND*		pSound;
	FMOD_CHANNEL*	pChannel;

	scache_t*		pCache;
	//edict_t*		pEdict;
	//TEMPENTITY*		pTemp;
	cl_entity_t*	pEntity;//FGP

	vec3_t			vOrigin;

	int				iFlags;
	int				iPitch;
	int				iEntIndex;
	int				iChannel;

	float			flVolume;
	float			flRadius;

	sentence_t*		pSentence;
	int				iCurChunk;

	int				iStopPos;
};

/*
====================
VectorCopyFM

====================
*/
inline void VectorCopyFM( float *vecIn, FMOD_VECTOR &vecOut )
{
	_asm {
		mov eax, vecIn;
		mov ebx, vecOut;

		movss xmm0, [eax];
		movss xmm1, [eax+4];
		movss xmm2, [eax+8];

		movss [ebx],	xmm0;
		movss [ebx+4],	xmm1;
		movss [ebx+8],	xmm2;
	}
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
FFT

====================
*/
inline short FFT(short int dir,long m,double *x,double *y)
{
   long n,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1;
   for (i=0;i<m;i++) 
      n *= 2;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }
   
   return(TRUE);
}

//FFT defines
inline double GetFrequencyIntensity(double re, double im)
{
    return sqrt((re*re)+(im*im));
}
#define mag_sqrd(re,im) (re*re+im*im)
#define Decibels(re,im) ((re == 0 && im == 0) ? (0) : 10.0 * log10(double(mag_sqrd(re,im))))
#define Amplitude(re,im,len) (GetFrequencyIntensity(re,im)/(len))
#define AmplitudeScaled(re,im,len,scale) ((int)Amplitude(re,im,len)%scale)
//FFT defines

/*
====================
CSoundEngine

====================
*/
class CSoundEngine
{
public:
	void	GetDllPointers( void );

	void	Init( void );
	void	Shutdown( void );

	void	ResetEngine( void );
	void	VidInit( void );

	void	ErrorCheck( bool bInit = false );
	void	FatalError( char *pszFormat, ... );
	void	PrintMissing( char *szPath );

	void	SetupFrame( ref_params_t *pparams );
	void    SetupListener( void );
	void	SetupSounds( void );
	void	SetupMusic( void );
	void	SetupGeometry( void );
	void	SetupReverbation( void );

	void	CalcMouth8( cl_entity_t *pEntity, sound_t *pSound, int iChanPos );
	void	CalcMouth16( cl_entity_t *pEntity, sound_t *pSound, int iChanPos );

	sound_t* PrepareSound( const char *szFile, int iEntIndex, int iFlags, int iChannel );
	void	 PlaySound( const char *szFile, vec3_t vOrigin = Vector(0, 0, 0), int iFlags = NULL, int iChannel = CHAN_AUTO, float fVolume = VOL_NORM, int iPitch = PITCH_NORM, float flAttenuation = ATTN_NORM, edict_t *pEdict = NULL, int iEntIndex = NULL, float flSyncPoint = 0);//FGP
	void	 StopSound( int iEntIndex, int iChannel );
	void	 PlaySentenceChunk( sound_t *pSound, sentence_t *pSentence, soption_t *pChunk );
	void	 PlayMusic( char *pszFile, int iFlags );

	void		GetWavInfo( scache_t *pCache );
	scache_t*	PrecacheSound( char *szFile );
	scache_t*	GetSoundFromCache( char *szFile );

	void	TempEntPlaySound( TEMPENTITY *pTemp, float flVolume );
	void	LoadSentences( void );
	int		MsgFunc_PlayAudio( const char *pszName, int iSize, void *pBuf );

public:
	HMODULE				m_hFmodDll;

	FMOD_SYSTEM*		m_pSystem;
	FMOD_RESULT			m_hResult;
	FMOD_CAPS			m_sCaps;
	FMOD_SPEAKERMODE	m_sSpeakerMode;

	FMOD_SOUND*			m_pMusicSound;
	FMOD_CHANNEL*		m_pMusicChannel;

	FMOD_GEOMETRY*		m_pGeometry;

	unsigned int		m_iVersion;
	int					m_iNumDrivers;
	int					m_iCurrentRoomType;

	bool				m_bPlayingMusic;
	bool				m_bReloaded;

	char				m_szDeviceName[256];
	char				m_szMapName[64];

	sound_t	    		m_sSounds[MAX_ACTIVE_SOUNDS];
	int					m_iNumActiveSounds;

	scache_t			m_sSoundCache[MAX_CACHED_SOUNDS];
	int					m_iNumCachedSounds;

	sentence_t			m_sSentences[MAX_SENTENCES];
	int					m_iNumSentences;

	cvar_t*				m_pCvarRoomType;
	cvar_t*				m_pCvarVolume;

	vec3_t				m_vViewOrigin;
	vec3_t				m_vViewAngles;

	char				m_szMissing[MAX_CACHED_SOUNDS][64];
	int					m_iNumMissing;

	int					m_iMusicFlags;
		
public:
	FMOD_RESULT			(_stdcall *_FMOD_System_Create) ( FMOD_SYSTEM **system );
	FMOD_RESULT			(_stdcall *_FMOD_System_GetVersion) ( FMOD_SYSTEM *system, unsigned int *  version );
	FMOD_RESULT			(_stdcall *_FMOD_System_GetNumDrivers) ( FMOD_SYSTEM *system, int *numdrivers );
	FMOD_RESULT			(_stdcall *_FMOD_System_SetOutput) ( FMOD_SYSTEM *system, FMOD_OUTPUTTYPE output );
	FMOD_RESULT			(_stdcall *_FMOD_System_GetDriverCaps) ( FMOD_SYSTEM *system, int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode );
	FMOD_RESULT			(_stdcall *_FMOD_System_SetSpeakerMode) ( FMOD_SYSTEM *system, FMOD_SPEAKERMODE speakermode );
	FMOD_RESULT			(_stdcall *_FMOD_System_SetDSPBufferSize) ( FMOD_SYSTEM *system, unsigned int bufferlength, int numbuffers );
	FMOD_RESULT			(_stdcall *_FMOD_System_GetDriverInfo) ( FMOD_SYSTEM *system, int id, char *name, int namelen, FMOD_GUID *guid );
	FMOD_RESULT			(_stdcall *_FMOD_System_SetSoftwareFormat) ( FMOD_SYSTEM *system, int samplerate, FMOD_SOUND_FORMAT format, int numoutputchannels, int maxinputchannels, FMOD_DSP_RESAMPLER resamplemethod );
	FMOD_RESULT			(_stdcall *_FMOD_System_Init) ( FMOD_SYSTEM *system, int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata );
	FMOD_RESULT			(_stdcall *_FMOD_System_Set3DSettings) ( FMOD_SYSTEM *system, float dopplerscale, float distancefactor, float rolloffscale );
	FMOD_RESULT			(_stdcall *_FMOD_System_Release) ( FMOD_SYSTEM *system );
	FMOD_RESULT	        (_stdcall *_FMOD_Channel_Stop) ( FMOD_CHANNEL *channel );
	FMOD_RESULT			(_stdcall *_FMOD_Sound_Release) ( FMOD_SOUND *sound );
	FMOD_RESULT			(_stdcall *_FMOD_System_SetReverbAmbientProperties) ( FMOD_SYSTEM *system, FMOD_REVERB_PROPERTIES *prop );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_SetPaused) ( FMOD_CHANNEL *channel, FMOD_BOOL paused );
	FMOD_RESULT			(_stdcall *_FMOD_System_Update) ( FMOD_SYSTEM *system );
	FMOD_RESULT			(_stdcall *_FMOD_System_Set3DListenerAttributes) ( FMOD_SYSTEM *system, int listener, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel, const FMOD_VECTOR *forward, const FMOD_VECTOR *up );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_IsPlaying) ( FMOD_CHANNEL *channel, FMOD_BOOL *isplaying );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_GetPaused) ( FMOD_CHANNEL *channel, FMOD_BOOL *paused );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_Set3DAttributes) ( FMOD_CHANNEL *channel, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_GetPosition) ( FMOD_CHANNEL *channel, unsigned int *position, FMOD_TIMEUNIT postype );

	FMOD_RESULT			(_stdcall *_FMOD_Channel_SetVolume) ( FMOD_CHANNEL *channel, float volume );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_SetFrequency) ( FMOD_CHANNEL *channel, float frequency );
	FMOD_RESULT			(_stdcall *_FMOD_System_CreateSound) ( FMOD_SYSTEM *system, const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, FMOD_SOUND **sound );
	FMOD_RESULT			(_stdcall *_FMOD_System_PlaySound) ( FMOD_SYSTEM *system, FMOD_CHANNELINDEX channelid, FMOD_SOUND *sound, FMOD_BOOL paused, FMOD_CHANNEL **channel );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_Set3DMinMaxDistance) ( FMOD_CHANNEL *channel, float mindistance, float maxdistance );
	FMOD_RESULT			(_stdcall *_FMOD_System_CreateStream) ( FMOD_SYSTEM *system, const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, FMOD_SOUND **sound );
	FMOD_RESULT			(_stdcall *_FMOD_Channel_SetDelay) ( FMOD_CHANNEL *  channel, FMOD_DELAYTYPE  delaytype,   unsigned int  delayhi,   unsigned int  delaylo);
	FMOD_RESULT			(_stdcall *_FMOD_Channel_SetPosition) ( FMOD_CHANNEL *  channel,unsigned int  position,   FMOD_TIMEUNIT  postype);

//FGP
public:
	FMOD_RESULT			(_stdcall *_FMOD_Sound_GetLength)(FMOD_SOUND *sound, unsigned int *length, FMOD_TIMEUNIT lengthtype);
public:
	void HUDFrame( void );
	void CreateEntities( void );
	void DrawNormalTriangles( void );
	bool m_bReset, m_bGotFrame, m_bGotEnts, m_bGotRender;

public:
	void	EV_PlaySound( int ent, float *origin, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, bool relative = TRUE );
	void	HUD_PlaySound( const char *sample, float volume, int channel = CHAN_AUTO );//FGP - added channel

public:
	int m_iFGPRoomType;
	bool m_bFGPUpdateRoomType;
	bool m_bRobotniksPINGAS;	
};

extern CSoundEngine gSoundEngine;

#endif