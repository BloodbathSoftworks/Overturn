//===========Copyright © 2010 - 2011 Richard Roh·Ë, All rights reserved.=============//
//																					 //
// Purpose:																			 //
//																					 //
// $NoKeywords: $																	 //
//===================================================================================//

//===============================================
// Amnesia Sound Engine Class 
//
// Created by Richard Roh·Ë
// Thanks Andrew Lucas for sentences.txt parser
//
//===============================================

#include "hud.h"
#include "cl_util.h"

#include "const.h"
#include "entity_state.h"

#include "soundengine.h"

extern "C"
{
	#include "pm_shared.h"
}

#pragma warning( disable: 4018 )

void StopAudio ( void )
{
	if(gSoundEngine.m_bPlayingMusic)
	{
		gSoundEngine._FMOD_Channel_Stop(gSoundEngine.m_pMusicChannel);
		gSoundEngine._FMOD_Sound_Release(gSoundEngine.m_pMusicSound);
		gSoundEngine.m_bPlayingMusic = false;
	}
}

#define MUSIC_FLAG_LOOP 2

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
//	stristr
//
//==========================
char *stristr( const char *string, const char *string2 )
{
	int c, len;
	c = tolower( *string2 );
	len = strlen( string2 );

	while (string) {
		for (; *string && tolower( *string ) != c; string++);
		if (*string) {
			if (strnicmp( string, string2, len ) == 0) {
				break;
			}
			string++;
		}
		else {
			return NULL;
		}
	}
	return (char *)string;
}

/*
========================================
WatchThread

Watch Half-Life's window for minimized
and inactive states. If one of these
window styles are used, stop all the
sounds. Values passed in must be thread
safe and their change must be executed
in critical section.
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
	sound_t *pSounds;
} thread_t;

thread_t sThreadData;

FMOD_RESULT (_stdcall *SetPaused) ( FMOD_CHANNEL *channel, FMOD_BOOL paused );

DWORD WINAPI WatchThread( LPVOID lpParam )
{
	thread_t *pThreadData = (thread_t*)lpParam;
	bool bAppStatus, bCleaned;

	HMODULE hFmodDll = GetModuleHandle("fmodex.dll");
	if(!hFmodDll)
	{
		return 0;
	}

	(FARPROC&)SetPaused = GetProcAddress(hFmodDll, "FMOD_Channel_SetPaused");
	if(!SetPaused)
	{
		return 0;
	}

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
				if(pThreadData->pSounds[i].pSound)
				{
					SetPaused(pThreadData->pSounds[i].pChannel, true);
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
GetDllPointers

====================
*/
void CSoundEngine::GetDllPointers( void )
{
	char szPath[256];

	strcpy(szPath, gEngfuncs.pfnGetGameDirectory());
	strcat(szPath, "/dlls/fmodex.dll");

	m_hFmodDll = LoadLibrary(szPath);
	if(!m_hFmodDll)
		FatalError("Couldn't load %s!\n", szPath);

	(FARPROC&)_FMOD_System_Create = GetProcAddress(m_hFmodDll, "FMOD_System_Create");
	(FARPROC&)_FMOD_System_GetVersion = GetProcAddress(m_hFmodDll, "FMOD_System_GetVersion");
	(FARPROC&)_FMOD_System_GetNumDrivers = GetProcAddress(m_hFmodDll, "FMOD_System_GetNumDrivers");
	(FARPROC&)_FMOD_System_SetOutput = GetProcAddress(m_hFmodDll, "FMOD_System_SetOutput");
	(FARPROC&)_FMOD_System_GetDriverCaps = GetProcAddress(m_hFmodDll, "FMOD_System_GetDriverCaps");
	(FARPROC&)_FMOD_System_SetSpeakerMode = GetProcAddress(m_hFmodDll, "FMOD_System_SetSpeakerMode");
	(FARPROC&)_FMOD_System_SetDSPBufferSize = GetProcAddress(m_hFmodDll, "FMOD_System_SetDSPBufferSize");
	(FARPROC&)_FMOD_System_GetDriverInfo = GetProcAddress(m_hFmodDll, "FMOD_System_GetDriverInfo");
	(FARPROC&)_FMOD_System_SetSoftwareFormat = GetProcAddress(m_hFmodDll, "FMOD_System_SetSoftwareFormat");
	(FARPROC&)_FMOD_System_Init = GetProcAddress(m_hFmodDll, "FMOD_System_Init");
	(FARPROC&)_FMOD_System_Set3DSettings = GetProcAddress(m_hFmodDll, "FMOD_System_Set3DSettings");
	(FARPROC&)_FMOD_System_Release = GetProcAddress(m_hFmodDll, "FMOD_System_Release");
	(FARPROC&)_FMOD_Channel_Stop = GetProcAddress(m_hFmodDll, "FMOD_Channel_Stop");
	(FARPROC&)_FMOD_Sound_Release = GetProcAddress(m_hFmodDll, "FMOD_Sound_Release");
	(FARPROC&)_FMOD_System_SetReverbAmbientProperties = GetProcAddress(m_hFmodDll, "FMOD_System_SetReverbAmbientProperties");
	(FARPROC&)_FMOD_Channel_SetPaused = GetProcAddress(m_hFmodDll, "FMOD_Channel_SetPaused");
	(FARPROC&)_FMOD_System_Update = GetProcAddress(m_hFmodDll, "FMOD_System_Update");
	(FARPROC&)_FMOD_System_Set3DListenerAttributes = GetProcAddress(m_hFmodDll, "FMOD_System_Set3DListenerAttributes");
	(FARPROC&)_FMOD_Channel_IsPlaying = GetProcAddress(m_hFmodDll, "FMOD_Channel_IsPlaying");
	(FARPROC&)_FMOD_Channel_GetPaused = GetProcAddress(m_hFmodDll, "FMOD_Channel_GetPaused");
	(FARPROC&)_FMOD_Channel_Set3DAttributes = GetProcAddress(m_hFmodDll, "FMOD_Channel_Set3DAttributes");
	(FARPROC&)_FMOD_Channel_GetPosition = GetProcAddress(m_hFmodDll, "FMOD_Channel_GetPosition");
	(FARPROC&)_FMOD_Channel_SetVolume = GetProcAddress(m_hFmodDll, "FMOD_Channel_SetVolume");
	(FARPROC&)_FMOD_Channel_SetFrequency = GetProcAddress(m_hFmodDll, "FMOD_Channel_SetFrequency");
//	(FARPROC&)_FMOD_Channel_GetFrequency = GetProcAddress(m_hFmodDll, "FMOD_Channel_GetFrequency");
	(FARPROC&)_FMOD_System_CreateSound = GetProcAddress(m_hFmodDll, "FMOD_System_CreateSound");
	(FARPROC&)_FMOD_System_PlaySound = GetProcAddress(m_hFmodDll, "FMOD_System_PlaySound");
	(FARPROC&)_FMOD_Channel_Set3DMinMaxDistance = GetProcAddress(m_hFmodDll, "FMOD_Channel_Set3DMinMaxDistance");
	(FARPROC&)_FMOD_System_CreateStream = GetProcAddress(m_hFmodDll, "FMOD_System_CreateStream");
	(FARPROC&)_FMOD_Channel_SetDelay = GetProcAddress(m_hFmodDll, "FMOD_Channel_SetDelay");
	(FARPROC&)_FMOD_Channel_SetPosition = GetProcAddress(m_hFmodDll, "FMOD_Channel_SetPosition");
	//FGP
	(FARPROC&)_FMOD_Sound_GetLength = GetProcAddress(m_hFmodDll, "FMOD_Sound_GetLength");

	if(!_FMOD_System_Create ||
		!_FMOD_System_GetVersion ||
		!_FMOD_System_GetNumDrivers ||
		!_FMOD_System_SetOutput ||
		!_FMOD_System_GetDriverCaps ||
		!_FMOD_System_SetSpeakerMode ||
		!_FMOD_System_SetDSPBufferSize ||
		!_FMOD_System_GetDriverInfo ||
		!_FMOD_System_SetSoftwareFormat ||
		!_FMOD_System_Init ||
		!_FMOD_System_Set3DSettings ||
		!_FMOD_System_Release ||
		!_FMOD_Channel_Stop ||
		!_FMOD_Sound_Release ||
		!_FMOD_System_SetReverbAmbientProperties ||
		!_FMOD_Channel_SetPaused ||
		!_FMOD_System_Update ||
		!_FMOD_System_Set3DListenerAttributes ||
		!_FMOD_Channel_IsPlaying ||
		!_FMOD_Channel_GetPaused ||
		!_FMOD_Channel_Set3DAttributes ||
		!_FMOD_Channel_GetPosition ||
		!_FMOD_Channel_SetVolume ||
		!_FMOD_Channel_SetFrequency ||
	//	!_FMOD_Channel_GetFrequency ||
		!_FMOD_System_CreateSound ||
		!_FMOD_System_PlaySound ||
		!_FMOD_Channel_Set3DMinMaxDistance ||
		!_FMOD_System_CreateStream ||
		!_FMOD_Channel_SetDelay ||
		!_FMOD_Channel_SetPosition ||//)
		//FGP
		!_FMOD_Sound_GetLength)
	{
		FatalError("Couldn't load functions from %s\n", szPath);
	}
}

/*
FGP - USER MESSAGES
*/
/*MUSIC*/
int Msg_PlayMusic( const char *pszName, int iSize, void *pBuf )
{
	BEGIN_READ(pBuf, iSize);
	char *pszFile = READ_STRING();
	int iFlags = READ_BYTE();

	gSoundEngine.PlayMusic(pszFile, iFlags);
	return 1;
}

/*SOUNDS*/
int Msg_EmitSound( const char *pszName, int iSize, void *pBuf )
{
	BEGIN_READ(pBuf, iSize);

	int index = READ_SHORT();
	int channel = READ_BYTE();

	int flags = READ_LONG();

	//STOP THE SOUND
	if(flags & SND_STOP)
	{
		gSoundEngine.StopSound(index, channel);
		return 1;
	}

	char *sample = READ_STRING();

	Vector pos = Vector(0, 0, 0);
	for (int i=0 ; i<3 ; i++)
		pos[i] = READ_COORD();

	if(!index && (pos == Vector(0, 0, 0)))
	{
		gEngfuncs.Con_Printf("SOUND ERROR! %s PLAYED WITHOUT ENTITY OR VECTOR!\n", sample);
		return 1;
	}

	float attn = READ_COORD();
	float volume = READ_COORD();
	int pitch = READ_BYTE();
	
	float syncpoint = READ_FLOAT();

	//dont precache here, do it propery
	if((sample[0] != '!') && (sample[0] != '*'))
	{
		char szPath[256];
		strcpy(szPath, "sound/");
		strcat(szPath, sample);
		gSoundEngine.PrecacheSound(szPath);
	}

	gSoundEngine.PlaySound(sample, pos, flags, channel, volume, pitch, attn, 0, index, syncpoint);

	return 1;
}

/*ROOMTYPE*/
int Msg_UpdateRType( const char *pszName, int iSize, void *pBuf )
{
	BEGIN_READ(pBuf, iSize);

	gSoundEngine.m_iFGPRoomType = READ_BYTE();
	gSoundEngine.m_bFGPUpdateRoomType = TRUE;
	return 1;
}

/*
FGP - USER MESSAGES
*/

/*void DisconnectSoundEngine( void )//FGP
{
	gSoundEngine.ResetEngine();
	gEngfuncs.pfnClientCmd("disconnect");
}*/


/*
====================
Init

====================
*/
void CSoundEngine::Init( void )
{
	//Parse fmodex.dll
	GetDllPointers();

	m_hResult = _FMOD_System_Create(&m_pSystem);
	ErrorCheck(true);

	m_hResult = _FMOD_System_GetVersion(m_pSystem, &m_iVersion);
	ErrorCheck(true);

	if(m_iVersion != FMOD_VERSION)
		FatalError("Engine detected wrong version of FMOD Ex library loaded!\nPress OK to exit the game.");

	m_hResult = _FMOD_System_GetNumDrivers(m_pSystem, &m_iNumDrivers);
	ErrorCheck(true);

	if(m_iNumDrivers == 0)
	{
		m_hResult = _FMOD_System_SetOutput(m_pSystem, FMOD_OUTPUTTYPE_NOSOUND);
		ErrorCheck(true);
	}
	else
	{
		m_hResult = _FMOD_System_GetDriverCaps(m_pSystem, 0, &m_sCaps, 0, 0, &m_sSpeakerMode);
		ErrorCheck(true);

		m_hResult = _FMOD_System_SetSpeakerMode(m_pSystem, m_sSpeakerMode);
		ErrorCheck(true);

		if(m_sCaps & FMOD_CAPS_HARDWARE_EMULATED)
		{
			gEngfuncs.pfnClientCmd("escape\n");	
			MessageBox(NULL, "Engine detected sound device acceleration turned off!\nThis may cause sound issues in game!\nPress OK to continue.", "Warning", MB_OK | MB_SETFOREGROUND | MB_ICONWARNING);

			m_hResult = _FMOD_System_SetDSPBufferSize(m_pSystem, 1024, 10);
			ErrorCheck(true);
		}

		m_hResult = _FMOD_System_GetDriverInfo(m_pSystem, 0, m_szDeviceName, 256, 0);
		ErrorCheck(true);

		if(strstr(m_szDeviceName, "SigmaTel"))
		{
			m_hResult = _FMOD_System_SetSoftwareFormat(m_pSystem, 48000, FMOD_SOUND_FORMAT_PCMFLOAT, 0,0, FMOD_DSP_RESAMPLER_LINEAR);
			ErrorCheck(true);
		}
	}

	m_hResult = _FMOD_System_Init(m_pSystem, MAX_ACTIVE_SOUNDS, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0);
	if(m_hResult == FMOD_ERR_OUTPUT_CREATEBUFFER)
	{
		m_hResult = _FMOD_System_SetSpeakerMode(m_pSystem, FMOD_SPEAKERMODE_STEREO);
		ErrorCheck(true);

		m_hResult = _FMOD_System_Init(m_pSystem, MAX_ACTIVE_SOUNDS, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0);
		ErrorCheck(true);
	}

	/*FGP*/
	gEngfuncs.pfnHookUserMsg("PlayMusic", Msg_PlayMusic);
	gEngfuncs.pfnHookUserMsg("EmitSound", Msg_EmitSound);
	gEngfuncs.pfnHookUserMsg("UpdateRType", Msg_UpdateRType);

	m_iFGPRoomType = 0;
	m_bFGPUpdateRoomType = FALSE;
	m_bRobotniksPINGAS = FALSE;

	//gEngfuncs.pfnAddCommand ("disconnectme", DisconnectSoundEngine );//kills the sound engine and disconnects properly
	/*FGP*/

	m_bReset = true;

	// 0.3048 / 16 = 0.01905
	m_hResult = _FMOD_System_Set3DSettings(m_pSystem, 0.0f, 0.01905f, 0.0f);
	ErrorCheck(true);

	// Parse sentences.txt
	LoadSentences();

	// Get convars
	m_pCvarRoomType	= gEngfuncs.pfnGetCvarPointer("room_type");
	m_pCvarVolume	= CVAR_CREATE("rn_volume", "1", FCVAR_ARCHIVE);

	m_bPlayingMusic = false;

	// Setup watch thread
	InitializeCriticalSection(&g_CS);

	g_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!g_hExitEvent)
		FatalError("Couldn't create thread exit event!\n");

	sThreadData.pSounds = m_sSounds;

	DWORD dwThreadID;
	g_hThreadHandle = CreateThread(NULL, 0, WatchThread, &sThreadData, 0, &dwThreadID);

	gEngfuncs.pfnAddCommand("stopaudio", StopAudio);
}

/*
====================
Shutdown

====================
*/
void CSoundEngine::Shutdown( void )
{
	SetEvent(g_hExitEvent);
	WaitForSingleObject(g_hThreadHandle, 500);
	CloseHandle(g_hThreadHandle);
	CloseHandle(g_hExitEvent);

	ResetEngine();
	_FMOD_System_Release(m_pSystem);

	DeleteCriticalSection(&g_CS);

	if(m_hFmodDll)
		FreeLibrary(m_hFmodDll);
}

/*
====================
ResetEngine

====================
*/
void CSoundEngine::ResetEngine( void )
{
	EnterCriticalSection(&g_CS);
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		sound_t *pSound = &m_sSounds[i];
		if(!pSound->pSound)
			continue;

		_FMOD_Channel_Stop(pSound->pChannel);
		_FMOD_Sound_Release(pSound->pSound);
	}

	memset(m_sSounds, 0, sizeof(m_sSounds));
	LeaveCriticalSection(&g_CS);

	if(m_iNumCachedSounds != 0)
	{
		for(int i = 0; i < m_iNumCachedSounds; i++)
		{
			delete [] m_sSoundCache[i].pFile;
		}

		memset(m_sSoundCache, 0, sizeof(m_sSoundCache));
		m_iNumCachedSounds = 0;
	}

	memset(m_szMissing, NULL, sizeof(m_szMissing));
	m_iNumMissing = NULL;

	FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_OFF;
	_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);

	m_iCurrentRoomType = 0;

	/*FGP - TEST*/
	m_iFGPRoomType = 0;
	m_bFGPUpdateRoomType = FALSE;
	m_bRobotniksPINGAS = FALSE;
	/*FGP - TEST*/

	if(m_bPlayingMusic)
	{
		if((m_iMusicFlags & MUSIC_FLAG_LOOP))
		{
			_FMOD_Channel_Stop(m_pMusicChannel);
			_FMOD_Sound_Release(m_pMusicSound);
			m_bPlayingMusic = false;
			m_iMusicFlags = NULL;
		}
		else
		{
			_FMOD_Channel_SetPaused(m_pMusicChannel, true);
		}
	}

	m_bReset = true;
	m_bGotFrame = false;
	m_bGotEnts = false;
	m_bGotRender = false;
}

/*
====================
VidInit

====================
*/
void CSoundEngine::VidInit( void )
{
	if(gEngfuncs.pfnGetCvarFloat("s_eax"))
	{
		gEngfuncs.pfnClientCmd("escape\n");	
		MessageBox(NULL, 
			"Amnesia Sound Engine detected EAX enabled by Half-Life.\nThis mod isn't compatible with this setting.\nIn order to continue, turn off EAX in Half-Life, and restart the game.\n", 
			"Sound engine warning", 
			MB_OK | MB_SETFOREGROUND | MB_ICONWARNING); 
	}

	m_bGotFrame = false;
	m_bReset = false;
	m_bGotEnts = false;
	m_bGotRender = false;
}

/*
====================
ErrorCheck

====================
*/
void CSoundEngine::ErrorCheck( bool bInit )
{
	if(m_hResult != FMOD_OK)
	{
		if(bInit)
			FatalError("FMOD error! (%d) %s\n", m_hResult, FMOD_ErrorString(m_hResult));
		else
			gEngfuncs.Con_Printf("FMOD error! (%d) %s\n", m_hResult, FMOD_ErrorString(m_hResult));
	}
};

/*
====================
FatalError

====================
*/
void CSoundEngine::FatalError( char *pszFormat, ... )
{
	va_list pszArgList;
	char szText[1024];

	gEngfuncs.pfnClientCmd("escape\n");	

	va_start(pszArgList, pszFormat);
	vsprintf(szText, pszFormat, pszArgList);
	va_end(pszArgList);

	MessageBox(NULL, szText, "Sound engine error!", MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
	
	gEngfuncs.pfnClientCmd("quit\n");
};

/*
====================
SetupFrame

====================
*/
void CSoundEngine::SetupFrame( ref_params_t *pparams )
{
	VectorCopy(pparams->vieworg, m_vViewOrigin);
	VectorCopy(pparams->viewangles, m_vViewAngles);

	/*FGP - new reverb*/
	if(/*pmove*/pparams->waterlevel == 3 && m_bRobotniksPINGAS == FALSE)
	{
		FMOD_REVERB_PROPERTIES waterReverb = FMOD_PRESET_UNDERWATER;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &waterReverb);
		m_bRobotniksPINGAS = TRUE;
	}
	if(/*pmove*/pparams->waterlevel != 3)
	{
		if(m_bRobotniksPINGAS == TRUE)
		{
			m_bFGPUpdateRoomType = TRUE;
			m_bRobotniksPINGAS = FALSE;
		}
		SetupReverbation();
	}
	/*FGP*/

	// Update sound elements
	SetupSounds();
	SetupListener();
	SetupMusic();
	_FMOD_System_Update(m_pSystem);
}

/*
====================
SetupListener

====================
*/
void CSoundEngine::SetupListener( void )
{
	vec3_t vForward, vUp;
	FMOD_VECTOR vOriginFM, vForwardFM, vUpFM;
	gEngfuncs.pfnAngleVectors(m_vViewAngles, vForward, NULL, vUp);

	VectorCopyFM(m_vViewOrigin, vOriginFM);
	VectorCopyFM(vForward, vForwardFM);
	VectorCopyFM(vUp, vUpFM);

	m_hResult = _FMOD_System_Set3DListenerAttributes(m_pSystem, 0, &vOriginFM, 0, &vForwardFM, &vUpFM);
	ErrorCheck();
}

/*
====================
SetupSounds

====================
*/

void CSoundEngine::SetupSounds( void )
{
	FMOD_BOOL	bState, bPaused;
	FMOD_VECTOR vPos;
	cl_entity_t *pEntity = NULL;
	uint iChanPos = NULL;
	float flVolumeScale = m_pCvarVolume->value;

	static float flLastTime = 0;
	float flTime = gEngfuncs.GetClientTime();
	float flFrameTime = flTime - flLastTime;
	flLastTime = flTime;

	//FGP
	cl_entity_t *pPlayer = gEngfuncs.GetLocalPlayer();
	int iMsg = pPlayer->curstate.messagenum;

	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		sound_t *pSound = &m_sSounds[i];
		if(!pSound->pSound)
			continue;

		if(pSound && pSound->pEntity/* && !(pSound->iFlags & SND_AMBIENT)*/ && (pSound->pEntity->curstate.messagenum != iMsg))
		{
			_FMOD_Channel_Stop(pSound->pChannel);
			_FMOD_Sound_Release(pSound->pSound);
			memset(&m_sSounds[i], 0, sizeof(sound_t));
			continue;
		}
	}
	//FGP

	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
			continue;

		if(m_sSounds[i].iFlags & SND_RADIO)
		{
			flVolumeScale = 0.1*m_pCvarVolume->value;
			break;
		}
	}

	// Be sure to set music too
	//_FMOD_Channel_SetVolume(m_pMusicChannel, 1.5f*flVolumeScale);

	EnterCriticalSection(&g_CS);
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		sound_t *pSound = &m_sSounds[i];
		if(!pSound->pSound)
			continue;

		_FMOD_Channel_IsPlaying(pSound->pChannel, &bState);
		_FMOD_Channel_GetPaused(pSound->pChannel, &bPaused);

		if(pSound->iFlags & SND_SENTENCE)
		{
			pEntity = gEngfuncs.GetEntityByIndex(pSound->iEntIndex);

			// Get playback position
			_FMOD_Channel_GetPosition(pSound->pChannel, &iChanPos, FMOD_TIMEUNIT_PCMBYTES);

			if(pSound->iStopPos > 0 && iChanPos >= pSound->iStopPos || !bState)
			{
				sentence_t *pSentence = pSound->pSentence;
				pEntity->mouth.mouthopen = 0;
				pEntity->mouth.sndavg = 0;
				pEntity->mouth.sndcount = 0;

				if(pSound->iCurChunk >= pSentence->pOptions.size())
				{
					_FMOD_Channel_Stop(pSound->pChannel);
					_FMOD_Sound_Release(pSound->pSound);

					memset(&m_sSounds[i], 0, sizeof(sound_t));
					continue;
				}
				else
				{
					_FMOD_Channel_Stop(pSound->pChannel);
					_FMOD_Sound_Release(pSound->pSound);

					PlaySentenceChunk(pSound, pSentence, &pSentence->pOptions[pSound->iCurChunk]);
					pSound->iCurChunk++;
				}
			}
		}

		if(!bState)
		{
			if(pSound->iFlags & SND_SENTENCE)
			{
				sentence_t *pSentence = pSound->pSentence;
				pEntity->mouth.mouthopen = 0;
				pEntity->mouth.sndavg = 0;
				pEntity->mouth.sndcount = 0;

				if(pSound->iCurChunk >= pSentence->pOptions.size())
				{
					_FMOD_Channel_Stop(pSound->pChannel);
					_FMOD_Sound_Release(pSound->pSound);

					memset(&m_sSounds[i], 0, sizeof(sound_t));
					continue;
				}
				else
				{
					_FMOD_Channel_Stop(pSound->pChannel);
					_FMOD_Sound_Release(pSound->pSound);

					PlaySentenceChunk(pSound, pSentence, &pSentence->pOptions[pSound->iCurChunk]);
					pSound->iCurChunk++;
				}
			}
			else
			{
				_FMOD_Channel_Stop(pSound->pChannel);
				_FMOD_Sound_Release(pSound->pSound);
				memset(&m_sSounds[i], 0, sizeof(sound_t));
				continue;
			}
		}

		if(flFrameTime <= 0 && !bPaused)
			_FMOD_Channel_SetPaused(pSound->pChannel, true);

		if(flFrameTime > 0 && bPaused)
			_FMOD_Channel_SetPaused(pSound->pChannel, false);

		if(!(pSound->iFlags & SND_RADIO))
			_FMOD_Channel_SetVolume(pSound->pChannel, pSound->flVolume*flVolumeScale);

		_FMOD_Channel_SetFrequency(pSound->pChannel, ((float)pSound->iPitch / 100) * pSound->pCache->iSampleRate);

		if(pSound->iFlags & SND_RELATIVE)
		{
			VectorCopyFM(m_vViewOrigin, vPos);
			_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
		}
		//FGP
		else if(pSound->pEntity && !(pSound->iFlags & SND_RELATIVE))
		{
			if(pSound->iFlags & SND_AMBIENT)//is this even nessicary if theres no need to update anyway?
			{
				VectorCopyFM(pSound->vOrigin, vPos);
				_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
			}
			else if(pSound->pEntity->curstate.modelindex)
			{
				vec3_t vRealMins = pSound->pEntity->curstate.maxs + pSound->pEntity->origin;
				vec3_t vRealMaxs = pSound->pEntity->curstate.mins + pSound->pEntity->origin;
				vec3_t vCenter = (vRealMins + vRealMaxs) * 0.5;

				VectorCopyFM(vCenter, vPos);
				_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
			}
			else
			{
				VectorCopyFM(pSound->pEntity->origin, vPos);
				_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
			}
		}
		//FGP

		/*else if(pSound->pEdict)
		{
			edict_t *pEdict = pSound->pEdict;
			if(pEdict->free)
				continue;

			if(pEdict->v.modelindex)
			{
				vec3_t vRealMins = pEdict->v.mins + pEdict->v.origin;
				vec3_t vRealMaxs = pEdict->v.maxs + pEdict->v.origin;
				vec3_t vCenter = (vRealMins + vRealMaxs) * 0.5;

				VectorCopyFM(vCenter, vPos);
				_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
			}
			else
			{
				VectorCopyFM(pEdict->v.origin, vPos);
				_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
			}
		}*/
		else if(!(pSound->iFlags & SND_2D))
		{
			VectorCopyFM(pSound->vOrigin, vPos);
			_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vPos, NULL);
		}


		if(pSound->iFlags & SND_SENTENCE && !bPaused && pEntity)
		{
			if(pSound->pCache->iBitCount == 8)
				CalcMouth8(pEntity, pSound, iChanPos);
			else if(pSound->pCache->iBitCount == 16)
				CalcMouth16(pEntity, pSound, iChanPos);
		}
	}
	LeaveCriticalSection(&g_CS);
}

/*
====================
CalcMouth8

====================
*/
void CSoundEngine::CalcMouth8( cl_entity_t *pEntity, sound_t *pSound, int iChanPos )
{
	int iMouthAvg = NULL;
	BYTE *pData, *pEnd;

	for(int i = 0; i < 3; i++)
	{
		double nInput[2] = { 0, 0 }, nOutput[2] = { 0, 0 };
		pData = (BYTE*)pSound->pCache->pFile + iChanPos;
		pEnd = (BYTE*)pSound->pCache->pFile + pSound->pCache->iSize;

		// Something can go wrong so to be sure
		if((pData+4) >= pEnd || (pData-2) < (pSound->pCache->pFile+pSound->pCache->pDataOffset))
		{
			pEntity->mouth.mouthopen = NULL;
			pEntity->mouth.sndcount = NULL;
			pEntity->mouth.sndavg = NULL;
			return;
		}

		// Average 3 samples
		pData += 2*(i-1);

		// Fill input array with sound data
		for(int i = 0; i < 2; i++)
		{
			pData++;
			nInput[i] = *pData;
		}

		FFT(0, 1, nInput, nOutput);

		// Calculate average value
		for(int i = 0; i < 2; i++)
		{
			int iReal = nInput[i];
			int iImaginary = nOutput[i];

			iMouthAvg += AmplitudeScaled(iReal, iImaginary, 1, 40);
		}
	}
	
	iMouthAvg = iMouthAvg/6;
	if(iMouthAvg < 10) iMouthAvg = 0;
	if(iMouthAvg > 255) iMouthAvg = 255;
	pEntity->mouth.mouthopen = (iMouthAvg+pEntity->mouth.mouthopen)/2;
}

/*
====================
CalcMouth16

====================
*/
void CSoundEngine::CalcMouth16( cl_entity_t *pEntity, sound_t *pSound, int iChanPos )
{
	int iMouthAvg = NULL;
	BYTE *pData, *pEnd;

	for(int i = 0; i < 3; i++)
	{
		double nInput[4] = { 0, 0, 0, 0 }, nOutput[4] = { 0, 0, 0, 0 };
		pData = (BYTE*)pSound->pCache->pFile + iChanPos;
		pEnd = (BYTE*)pSound->pCache->pFile + pSound->pCache->iSize;

		// Something can go wrong so to be sure
		if((pData+8) >= pEnd || (pData-4) < (pSound->pCache->pFile+pSound->pCache->pDataOffset))
		{
			pEntity->mouth.mouthopen = NULL;
			pEntity->mouth.sndcount = NULL;
			pEntity->mouth.sndavg = NULL;
			return;
		}

		// Average 3 samples
		pData += 4*(i-1);

		// Fill input array with sound data
		for(int i = 0; i < 4; i++)
		{
			pData++;
			nInput[i] = *pData;
		}

		FFT(0, 1, nInput, nOutput);

		// Calculate average value
		for(int i = 0; i < 4; i++)
		{
			int iReal = nInput[i];
			int iImaginary = nOutput[i];

			iMouthAvg += AmplitudeScaled(iReal, iImaginary, 1, 40);
		}
	}

	iMouthAvg = iMouthAvg/12;
	if(iMouthAvg < 10) iMouthAvg = 0;
	if(iMouthAvg > 255) iMouthAvg = 255;
	pEntity->mouth.mouthopen = (iMouthAvg+pEntity->mouth.mouthopen)/2;
}

/*
====================
SetupMusic

====================
*/
void CSoundEngine::SetupMusic( void )
{
	if(!m_bPlayingMusic)
		return;

	FMOD_BOOL bState, bPaused;
	_FMOD_Channel_IsPlaying(m_pMusicChannel, &bState);

	if(bState)
	{
		_FMOD_Channel_SetVolume(m_pMusicChannel, 1.5f*m_pCvarVolume->value);
	}
	else
	{
		_FMOD_Channel_Stop(m_pMusicChannel);
		_FMOD_Sound_Release(m_pMusicSound);
		m_bPlayingMusic = false;
	}

	static float flLastTime = 0;
	float flTime = gEngfuncs.GetClientTime();
	float flFrameTime = flTime - flLastTime;
	flLastTime = flTime;

	_FMOD_Channel_GetPaused(m_pMusicChannel, &bPaused);

	if(flFrameTime <= 0 && !bPaused)
		_FMOD_Channel_SetPaused(m_pMusicChannel, true);

	if(flFrameTime > 0 && bPaused)
		_FMOD_Channel_SetPaused(m_pMusicChannel, false);
}

/*
======================
SetupGeometry

======================
*/
void CSoundEngine::SetupGeometry( void )
{
}

/*
====================
SetupReverbation

====================
*/
void CSoundEngine::SetupReverbation( void )
{
	//FGP - everything here has been reworked for my liking

	if(m_bFGPUpdateRoomType == FALSE)
		return;

	if(m_iFGPRoomType == 0)
	{
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_OFF;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 1)
	{
		// Room
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_ROOM;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 2 || m_iFGPRoomType == 3 || m_iFGPRoomType == 4)
	{
		// Metal small/medium/large
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_SEWERPIPE;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 5 || m_iFGPRoomType == 6 || m_iFGPRoomType == 7)
	{
		// Tunnel small/large/medium
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_HANGAR;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 8 || m_iFGPRoomType == 9 || m_iFGPRoomType == 10)
	{
		// Chamber small/large/medium
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_STONECORRIDOR;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 11 || m_iFGPRoomType == 12 || m_iFGPRoomType == 13)
	{
		// Bright small/large/medium
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_STONECORRIDOR;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 14 || m_iFGPRoomType == 15 || m_iFGPRoomType == 16)
	{
		// Underwater
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_UNDERWATER;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 17 || m_iFGPRoomType == 18 || m_iFGPRoomType == 19)
	{
		//Concrete small/large/medium
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_STONECORRIDOR;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 20 || m_iFGPRoomType == 21 || m_iFGPRoomType == 22)
	{
		// Big 1/2/3
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_MOUNTAINS;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 23 || m_iFGPRoomType == 24 || m_iFGPRoomType == 25)
	{
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_ARENA;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}
	else if(m_iFGPRoomType == 26 || m_iFGPRoomType == 27 || m_iFGPRoomType == 28)
	{
		FMOD_REVERB_PROPERTIES sReverb = FMOD_PRESET_SEWERPIPE;
		_FMOD_System_SetReverbAmbientProperties(m_pSystem, &sReverb);
	}

	m_bFGPUpdateRoomType = FALSE;
}

/*
====================
PrepareSound

====================
*/
sound_t *CSoundEngine::PrepareSound( const char *szFile, int iEntIndex, int iFlags, int iChannel )
{	
	EnterCriticalSection(&g_CS);
	// Make sure not to let more than one HUD sound play
	// or let tempent noises clutter up

	if((iFlags & SND_HUD) || (iFlags & SND_TEMPENT))
	{
		int iNumTempSounds = 0;
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_sSounds[i].pSound)
				continue;

			if((m_sSounds[i].iFlags & SND_HUD) && (iFlags & SND_HUD)
				|| (m_sSounds[i].iFlags & SND_TEMPENT) && (iFlags & SND_TEMPENT)
				|| (m_sSounds[i].iFlags & SND_RADIO) && (iFlags & SND_RADIO))
			{
				if(m_sSounds[i].iFlags & SND_TEMPENT)
				{
					if(iNumTempSounds < MAX_ACTIVE_TEMP_SOUNDS)
					{
						iNumTempSounds++;
						continue;
					}

					LeaveCriticalSection(&g_CS);
					return NULL;
				}
				else
				{
					_FMOD_Channel_Stop(m_sSounds[i].pChannel);
					_FMOD_Sound_Release(m_sSounds[i].pSound);
					memset(&m_sSounds[i], 0, sizeof(sound_t));

					LeaveCriticalSection(&g_CS);
					return &m_sSounds[i];
				}
			}
		}
	}

	// See if the entity index we're using is already
	// playing something
	if(iEntIndex)
	{
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_sSounds[i].pSound)
				continue;

			if(m_sSounds[i].iFlags & SND_HUD)
				continue;

			if(m_sSounds[i].iFlags & SND_RADIO)
				continue;

			if(m_sSounds[i].iEntIndex == 0)
				continue;

			if(m_sSounds[i].iEntIndex == iEntIndex && m_sSounds[i].iChannel == iChannel)
			{
				_FMOD_Channel_Stop(m_sSounds[i].pChannel);
				_FMOD_Sound_Release(m_sSounds[i].pSound);
				memset(&m_sSounds[i], 0, sizeof(sound_t));

				LeaveCriticalSection(&g_CS);
				return &m_sSounds[i];
			}
		}
	}

	// Then find a free slot
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
		{
			LeaveCriticalSection(&g_CS);
			return &m_sSounds[i];
		}
	}

	// Didn't find free slot, clear some unimportant one
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
			continue;

		if(m_sSounds[i].iFlags & SND_AMBIENT)
			continue;

		if(m_sSounds[i].iFlags & SND_SENTENCE)
			continue;

		if(m_sSounds[i].iFlags & SND_RADIO)
			continue;

		if(m_sSounds[i].iChannel == CHAN_WEAPON)
			continue;

		_FMOD_Channel_Stop(m_sSounds[i].pChannel);
		_FMOD_Sound_Release(m_sSounds[i].pSound);
		memset(&m_sSounds[i], 0, sizeof(sound_t));
		break;
	}

	LeaveCriticalSection(&g_CS);
	return 0;
}

/*
====================
PlaySound

====================
*/
void CSoundEngine::PlaySound( const char *szFile, vec3_t vOrigin, int iFlags, int iChannel, float fVolume, int iPitch, float flAttenuation, edict_t *pEdict, int iEntIndex, float flSyncPoint)//FGP - added sync pt
{
	char szPath[256];
	sentence_t *pSentence = NULL;
	FMOD_CREATESOUNDEXINFO sSoundInfo;
	int iSoundFlags = NULL;

	if(szFile[0] == '!')
	{
		iFlags |= SND_SENTENCE;
		int iID = atoi(&szFile[1]);
		pSentence = &m_sSentences[iID];

		strcpy(szPath, "sound/");
		strcat(szPath, pSentence->szParentDir);

		if(pSentence->pOptions[0].szFile[strlen(pSentence->pOptions[0].szFile)-1] == '.')
			pSentence->pOptions[0].szFile[strlen(pSentence->pOptions[0].szFile)-1] = '\0';

		strcat(szPath, pSentence->pOptions[0].szFile);
		strcat(szPath, ".wav");
	}
	else if(szFile[0] == '*')
	{
		strcpy(szPath, "sound/");
		strcat(szPath, &szFile[1]);
	}
	else
	{
		strcpy(szPath, "sound/");
		strcat(szPath, szFile);
	}

	float flVolumeScale = m_pCvarVolume->value;
	if(!(iFlags & SND_RADIO))
	{
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_sSounds[i].pSound)
				continue;

			if(m_sSounds[i].iFlags & SND_RADIO)
			{
				flVolumeScale = 0.1*m_pCvarVolume->value;
				break;
			}
		}
	}

	EnterCriticalSection(&g_CS);
	if((iFlags & SND_STOP) || (iFlags & SND_CHANGE_VOL) || (iFlags & SND_CHANGE_PITCH))
	{
		for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
		{
			if(!m_sSounds[i].pSound)
				continue;

			if(m_sSounds[i].iEntIndex == 0)
				continue;

			if(!strcmp(m_sSounds[i].pCache->szFile, szPath)
				&& m_sSounds[i].iEntIndex == iEntIndex)
			{
				if(iFlags & SND_STOP)
				{
					_FMOD_Channel_Stop(m_sSounds[i].pChannel);
					_FMOD_Sound_Release(m_sSounds[i].pSound);
					memset(&m_sSounds[i], 0, sizeof(sound_t));

					LeaveCriticalSection(&g_CS);
					return;
				}
				else if(iFlags & SND_CHANGE_VOL)
				{
					m_sSounds[i].flVolume = fVolume;
					if(m_sSounds[i].flVolume > 1)
						m_sSounds[i].flVolume = 1;

					_FMOD_Channel_SetVolume(m_sSounds[i].pChannel, m_sSounds[i].flVolume*flVolumeScale);

					LeaveCriticalSection(&g_CS);
					return;
				}
				else if(iFlags & SND_CHANGE_PITCH)
				{

					m_sSounds[i].iPitch = iPitch;
					_FMOD_Channel_SetFrequency(m_sSounds[i].pChannel, ((float)m_sSounds[i].iPitch / 100) * m_sSounds[i].pCache->iSampleRate);

					LeaveCriticalSection(&g_CS);
					return;
				}
			}
		}
		LeaveCriticalSection(&g_CS);
		return;
	}
	LeaveCriticalSection(&g_CS);

	scache_t *pSoundData = GetSoundFromCache(szPath);
	if(!pSoundData)
		return;

	sound_t *pSound = PrepareSound(szPath, iEntIndex, iFlags, iChannel);
	if(!pSound)
		return;

	pSound->pCache = pSoundData;

	//FGP
	if(gEngfuncs.GetLocalPlayer()->index == pSound->iEntIndex)
		pSound->iFlags |= SND_RELATIVE;

	if(iEntIndex > 0 && (!(iFlags & SND_RELATIVE) && !(iFlags & SND_2D)) && !(iFlags & SND_AMBIENT))
		pSound->pEntity = gEngfuncs.GetEntityByIndex(iEntIndex);
	else
		pSound->pEntity = NULL;
	//FGP


	pSound->iFlags = iFlags;
	pSound->iEntIndex = iEntIndex;
	pSound->iChannel = iChannel;
	pSound->iStopPos = -1;
	
	if(pSentence && (iFlags & SND_SENTENCE))
	{
	//	pSound->iPitch = pSentence->pOptions[0].iPitch;//FGP
		pSound->iPitch = iPitch;
		pSound->flVolume = (float)pSentence->pOptions[0].iVolume / 100;
	}
	else
	{
		pSound->iPitch = iPitch;
		pSound->flVolume = fVolume;
	}

	if(pSentence && (iFlags & SND_SENTENCE))
	{
		pSound->pSentence = pSentence;
		pSound->iCurChunk++;
	}

	if(!iEntIndex)
		pSound->pEntity = NULL;//FGP//pEdict = 0;

	//FGP
	if(pSound->pEntity && (!(pSound->iFlags & SND_AMBIENT) && !(pSound->iFlags & SND_RELATIVE)))
	{
		if(pSound->pEntity->curstate.modelindex)
		{
			vec3_t vRealMins = pSound->pEntity->curstate.maxs + pSound->pEntity->origin;
			vec3_t vRealMaxs = pSound->pEntity->curstate.mins + pSound->pEntity->origin;
			pSound->vOrigin = (vRealMins + vRealMaxs) * 0.5;
		}
		else
			VectorCopy(pSound->pEntity->origin, pSound->vOrigin);
	}
	else
		VectorCopy(vOrigin, pSound->vOrigin);
	//FGP


	if(flAttenuation != ATTN_NONE)
	{
		pSound->flRadius = MAX_DISTANCE + (1.0 - flAttenuation) * (0.5*MAX_DISTANCE);//FGP
	/*	if(flAttenuation >= ATTN_NORM)
			pSound->flRadius = MAX_DISTANCE + (1.0 - flAttenuation) * (0.5*MAX_DISTANCE);
		else
			pSound->flRadius = MAX_DISTANCE + (1.0 - flAttenuation) * (4.0*MAX_DISTANCE);*/
	
		if(pSound->flRadius < 0)
			pSound->flRadius = 0;
	}
	else
	{
		pSound->iFlags |= SND_RELATIVE;
	}

	if(!vOrigin.Length() && (pSound->iEntIndex >= 0) && !iEntIndex || !pSound->flRadius)//FGP
	{
		if(!(pSound->iFlags & SND_RELATIVE))
			pSound->iFlags |= SND_2D;

	}

	memset(&sSoundInfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	sSoundInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	sSoundInfo.length = pSoundData->iSize;

	iSoundFlags = (FMOD_SOFTWARE | FMOD_OPENMEMORY);
	if(pSound->iFlags & SND_2D)
		iSoundFlags |= FMOD_2D;
	else
		iSoundFlags |= (FMOD_3D | FMOD_3D_LINEARROLLOFF);

	if(pSound->pCache->iLoopStart != -1)// || pSound->iFlags & SND_LOOP)// && pSound->pEntity)//FGP// && pSound->pEdict)
		iSoundFlags |= FMOD_LOOP_NORMAL;

	//FGP - loop non ".wav" sound files that contain "_lp"
	if(!stristr(szFile, ".wav") && stristr(szFile, "_lp"))
		iSoundFlags |= FMOD_LOOP_NORMAL;

	m_hResult = _FMOD_System_CreateSound(m_pSystem, (const char*)pSound->pCache->pFile, iSoundFlags, &sSoundInfo, &pSound->pSound);

	ErrorCheck();

	if(pSound->pEntity)//FGP//pEdict)
		m_hResult = _FMOD_System_PlaySound(m_pSystem, FMOD_CHANNEL_FREE, pSound->pSound, true, &pSound->pChannel);
	else
		m_hResult = _FMOD_System_PlaySound(m_pSystem, FMOD_CHANNEL_FREE, pSound->pSound, false, &pSound->pChannel);

	ErrorCheck();

	if(pSentence && (iFlags & SND_SENTENCE))
	{
		unsigned int iDelay = (pSentence->pOptions[0].flDelay*250) + (pSentence->pOptions[0].iTime);
		m_hResult = _FMOD_Channel_SetDelay(pSound->pChannel, FMOD_DELAYTYPE_END_MS, iDelay, 0);
		ErrorCheck();

		float flStartPos = ((pSound->pCache->iSize - 44) * (pSentence->pOptions[0].iStart)) / 100;
		m_hResult = _FMOD_Channel_SetPosition(pSound->pChannel, (int)flStartPos, FMOD_TIMEUNIT_PCMBYTES);
		ErrorCheck();

		if(pSentence->pOptions[0].iEnd > 0)
			pSound->iStopPos = (int)((pSound->pCache->iSize - 44) * (pSentence->pOptions[0].iEnd)) / 100;
	}
	//FGP - sound syncing crap that barely works propely but bleargh
	else if(flSyncPoint > 0.0 && (pSound->iFlags & SND_AMBIENT) && iSoundFlags & FMOD_LOOP_NORMAL)
	{
		unsigned int fmlength;
		m_hResult = _FMOD_Sound_GetLength(pSound->pSound, &fmlength, FMOD_TIMEUNIT_MS);
		ErrorCheck(true);

		int iSyncTime = ((int)flSyncPoint*1000)%fmlength;//(int)(flSyncPoint*1000);
		gEngfuncs.Con_Printf("%s - length (ms): %d - sync time (ms): %d\n", szFile, fmlength, iSyncTime);
		m_hResult = _FMOD_Channel_SetPosition(pSound->pChannel, iSyncTime, FMOD_TIMEUNIT_MS);
		ErrorCheck(true);
	}
	//FGP

	if(!(pSound->iFlags & SND_2D))
	{
		FMOD_VECTOR vOriginFM;
		VectorCopyFM(pSound->vOrigin, vOriginFM);

		_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vOriginFM, NULL);
		_FMOD_Channel_Set3DMinMaxDistance(pSound->pChannel, MIN_DISTANCE, pSound->flRadius);
	}

	if(pSound->flVolume > 1)
		pSound->flVolume = 1;

	_FMOD_Channel_SetVolume(pSound->pChannel, pSound->flVolume*flVolumeScale);

	_FMOD_Channel_SetFrequency(pSound->pChannel, ((float)pSound->iPitch / 100) * pSound->pCache->iSampleRate);

}

/*
====================
StopSound

====================
*/
void CSoundEngine::StopSound( int iEntIndex, int iChannel )
{
	EnterCriticalSection(&g_CS);
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
			continue;

		if(m_sSounds[i].iEntIndex == iEntIndex
			&& m_sSounds[i].iChannel == iChannel)
		{
			_FMOD_Channel_Stop(m_sSounds[i].pChannel);
			_FMOD_Sound_Release(m_sSounds[i].pSound);
			memset(&m_sSounds[i], 0, sizeof(sound_t));

			LeaveCriticalSection(&g_CS);
			return;
		}
	}
	LeaveCriticalSection(&g_CS);
}

/*
====================
PlaySentenceChunk

====================
*/
void CSoundEngine::PlaySentenceChunk( sound_t *pSound, sentence_t *pSentence, soption_t *pChunk )
{
	char szPath[256];
	FMOD_CREATESOUNDEXINFO sSoundInfo;

	strcpy(szPath, "sound/");
	strcat(szPath, pSentence->szParentDir);

	if(pChunk->szFile[strlen(pChunk->szFile)-1] == '.')
		pChunk->szFile[strlen(pChunk->szFile)-1] = '\0';

	strcat(szPath, pChunk->szFile);
	strcat(szPath, ".wav");

	scache_t *pSoundData = GetSoundFromCache(szPath);
	if(!pSoundData)
		return;

	pSound->pCache = pSoundData;
	pSound->iPitch = pChunk->iPitch;
	pSound->flVolume = (float)pChunk->iVolume / 100;

	memset(&sSoundInfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
	sSoundInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	sSoundInfo.length = pSoundData->iSize;

	FMOD_VECTOR vOriginFM;
	VectorCopyFM(pSound->vOrigin, vOriginFM);

	_FMOD_System_CreateSound(m_pSystem, (const char *)pSoundData->pFile, 
		(FMOD_SOFTWARE | FMOD_OPENMEMORY | FMOD_3D | FMOD_3D_LINEARROLLOFF), 
		&sSoundInfo, &pSound->pSound);

	_FMOD_System_PlaySound(m_pSystem, FMOD_CHANNEL_FREE, pSound->pSound, false, &pSound->pChannel);

	float flVolumeScale = 1.0;
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
			continue;

		if(m_sSounds[i].iFlags & SND_RADIO)
		{
			flVolumeScale = 0.1;
			break;
		}
	}

	unsigned int iDelay = (pChunk->flDelay*250) + (pChunk->iTime);
	m_hResult = _FMOD_Channel_SetDelay(pSound->pChannel, FMOD_DELAYTYPE_END_MS, iDelay, 0);
	ErrorCheck();

	float flStartPos = ((pSound->pCache->iSize - 44) * (pChunk->iStart)) / 100;
	m_hResult = _FMOD_Channel_SetPosition(pSound->pChannel, (int)flStartPos, FMOD_TIMEUNIT_PCMBYTES);
	ErrorCheck();

	if(pChunk->iEnd > 0)
		pSound->iStopPos = (int)((pSound->pCache->iSize - 44) * (pChunk->iEnd)) / 100;
	else
		pSound->iStopPos = -1;

	if(pSound->flVolume > 1)
		pSound->flVolume = 1;

	_FMOD_Channel_SetVolume(pSound->pChannel, pSound->flVolume*flVolumeScale);

	_FMOD_Channel_SetFrequency(pSound->pChannel, ((float)pSound->iPitch / 100) * pSound->pCache->iSampleRate);

	_FMOD_Channel_Set3DAttributes(pSound->pChannel, &vOriginFM, NULL);
	_FMOD_Channel_Set3DMinMaxDistance(pSound->pChannel, MIN_DISTANCE, pSound->flRadius);
}

/*
====================
PlayMusic

====================
*/
void CSoundEngine::PlayMusic( char *pszFile, int iFlags )
{
	if(m_bPlayingMusic && (iFlags & MUSIC_FLAG_LOOP) && (m_iMusicFlags & MUSIC_FLAG_LOOP))
		return;

	if(m_bPlayingMusic)
	{
		_FMOD_Channel_Stop(m_pMusicChannel);
		_FMOD_Sound_Release(m_pMusicSound);
	}

	float flVolumeScale = 1.0;
	for(int i = 0; i < MAX_ACTIVE_SOUNDS; i++)
	{
		if(!m_sSounds[i].pSound)
			continue;

		if(m_sSounds[i].iFlags & SND_RADIO)
		{
			flVolumeScale = 0.1;
			break;
		}
	}

	char szPath[256];
	sprintf(szPath, "%s/media/%s", gEngfuncs.pfnGetGameDirectory(), pszFile);

	int iSoundFlags = (FMOD_SOFTWARE | FMOD_2D);
	if(iFlags & MUSIC_FLAG_LOOP) iSoundFlags |= FMOD_LOOP_NORMAL;
	_FMOD_System_CreateStream(m_pSystem, szPath, iSoundFlags, NULL, &m_pMusicSound);
	_FMOD_System_PlaySound(m_pSystem, FMOD_CHANNEL_FREE, m_pMusicSound, true, &m_pMusicChannel);
	_FMOD_Channel_SetVolume(m_pMusicChannel, 1.5f*flVolumeScale);

	m_iMusicFlags = iFlags;
	m_bPlayingMusic = true;
	strcpy(m_szMapName, gEngfuncs.pfnGetLevelName());
}
/*
====================
GetWavInfo

====================
*/
void CSoundEngine::GetWavInfo( scache_t *pCache )
{
	DWORD iLength;

	BYTE *pFile = (BYTE*)pCache->pFile + 12;
	BYTE *pEnd = (BYTE*)pCache->pFile + pCache->iSize;

	while(1)
	{
		if(pFile >= pEnd)
			break;

		iLength = ByteToInt(pFile+4);
		ScaleByte(&iLength);

		if(!strncmp((const char*)pFile, "fmt ", 4))
		{
			pCache->iSampleRate = ByteToInt(pFile+12);
			pCache->iBitCount = ByteToUShort(pFile+22);
		}

		if(!strncmp((const char*)pFile, "cue ", 4))
		{
			pCache->iLoopStart = ByteToInt(pFile+32);
		}

		if(!strncmp((const char*)pFile, "data", 4))
		{
			pCache->pDataOffset = (pFile+8)-pCache->pFile;
		}

		pFile = pFile + 8 + iLength;
	}
}

/*
====================
PrintMissing

====================
*/
void CSoundEngine::PrintMissing( char *szPath )
{
	for(int i = 0; i < m_iNumMissing; i++)
	{
		if(!strcmp(m_szMissing[i], szPath))
			return;
	}

	gEngfuncs.Con_Printf("Could not precache: %s.\n", szPath);
	strcpy(m_szMissing[m_iNumMissing], szPath); m_iNumMissing++;
}


/*
====================
PrecacheSound

====================
*/
scache_t* CSoundEngine::PrecacheSound( char *szFile )
{
	char szPath[256];
	if(szFile[0] == '!')
	{
		int iID = atoi(&szFile[1]);
		sentence_t *pSentence = &m_sSentences[iID];
		strcpy(szPath, pSentence->szParentDir);
		if(pSentence->pOptions[0].szFile[strlen(pSentence->pOptions[0].szFile)-1] == '.')
			pSentence->pOptions[0].szFile[strlen(pSentence->pOptions[0].szFile)-1] = '\0';
		strcat(szPath, pSentence->pOptions[0].szFile);
		strcat(szPath, ".wav");
	}
	else
	{
		strcpy(szPath, szFile);
	}

	for(int i = 0; i < m_iNumCachedSounds; i++)
	{
		if(!strcmp(m_sSoundCache[i].szFile, szPath))
		{
			return &m_sSoundCache[i];
		}
	}

	if(m_iNumCachedSounds == MAX_CACHED_SOUNDS)
	{
		gEngfuncs.Con_Printf("Sound cache is full! Sound (%s) won't be part of playback!\n", szPath);
		return 0;
	}

	int	iSize = 0;
	byte *pData = (byte *)gEngfuncs.COM_LoadFile(szPath, 5, &iSize);

	if(!pData)
	{
		PrintMissing(szPath);
		return 0;
	}

	scache_t *pCache = &m_sSoundCache[m_iNumCachedSounds];
	
	pCache->pFile = new byte[iSize];
	memcpy(pCache->pFile, pData, sizeof(byte)*iSize);
	pCache->iSize = iSize;

	strcpy(pCache->szFile, szPath);
	gEngfuncs.COM_FreeFile(pData);

	pCache->iLoopStart = -1;
	pCache->iLoopEnd = -1;

	GetWavInfo(pCache);

	m_iNumCachedSounds++;

	return pCache;
}

/*
====================
GetSoundFromCache

====================
*/
scache_t* CSoundEngine::GetSoundFromCache( char *szFile )
{
	for(int i = 0; i < m_iNumCachedSounds; i++)
	{
		if(!strcmp(szFile, m_sSoundCache[i].szFile))
			return &m_sSoundCache[i];
	}

	return PrecacheSound(szFile);
}

/*
====================
TempEntPlaySound

====================
*/
void CSoundEngine::TempEntPlaySound( TEMPENTITY *pTemp, float flVolume )
{
	if(pTemp->entity.origin == pTemp->entity.prevstate.origin)
		return;

	pTemp->entity.prevstate.origin = pTemp->entity.origin;

	// Glass impact
	if(pTemp->hitSound & BOUNCE_GLASS)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1:
			PlaySound("debris/glass1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("debris/glass2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("debris/glass3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}
	
	// Metal Impact
	if(pTemp->hitSound & BOUNCE_METAL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 5))
		{
		case 1:
			PlaySound("debris/metal1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("debris/metal2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 3:
			PlaySound("debris/metal3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 4:
			PlaySound("debris/metal4.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("debris/metal5.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}

	// Flesh impact
	if(pTemp->hitSound & BOUNCE_FLESH)
	{
		switch(gEngfuncs.pfnRandomLong(0, 6))
		{
		case 1:
			PlaySound("debris/flesh1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2: 
			PlaySound("debris/flesh2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 3:
			PlaySound("debris/flesh3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 4:
			PlaySound("debris/flesh4.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 5:
			PlaySound("debris/flesh5.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 6:
			PlaySound("debris/flesh6.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("debris/flesh7.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}

	// Wood impact
	if(pTemp->hitSound & BOUNCE_WOOD)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1:
			PlaySound("debris/wood1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("debris/wood2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("debris/wood3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}

	// Shell impact
	if(pTemp->hitSound & BOUNCE_SHELL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1:
			PlaySound("player/pl_shell2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("player/pl_shell3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("player/pl_shell1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}

	// Concrete impact
	if(pTemp->hitSound & BOUNCE_CONCRETE)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1:
			PlaySound("debris/concrete1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("debris/concrete2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("debris/concrete3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}

	// Shotgun shell impact
	if(pTemp->hitSound & BOUNCE_SHOTSHELL)
	{
		switch(gEngfuncs.pfnRandomLong(0, 2))
		{
		case 1:
			PlaySound("weapons/sshell2.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		case 2:
			PlaySound("weapons/sshell3.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		default:
			PlaySound("weapons/sshell1.wav", pTemp->entity.origin, SND_TEMPENT, CHAN_AUTO, flVolume, PITCH_NORM);
		}
	}
}

/*
====================
LoadSentences

====================
*/
void CSoundEngine::LoadSentences( void )
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
		sentence_t *pSentence = &m_sSentences[m_iNumSentences];
		sprintf(pSentence->szID, "!%i", m_iNumSentences); //Set ID
		m_iNumSentences++;

		int iDefaultPitch = 100;
		int iDefaultTime = 0;
		int iDefaultStart = 0;
		int iDefaultEnd = 0;
		int iDefaultVol = 100;

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
				strcpy(pSentence->szParentDir, "vox/");
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

					pSentence->szParentDir[j] = pFile[i];
					i++; j++;

					if(pFile[(i - 1)] == '\\' || pFile[(i - 1)] == '/')
					{
						pSentence->szParentDir[i] = 0;//terminate
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

			pSentence->pOptions.resize((pSentence->pOptions.size()+1));
			soption_t *pOption = &pSentence->pOptions[(pSentence->pOptions.size()-1)];
			memset(pOption, 0, sizeof(soption_t));

			// Set defaults
			pOption->iPitch = iDefaultPitch;
			pOption->iStart = iDefaultStart;
			pOption->iEnd = iDefaultEnd;
			pOption->iTime = iDefaultTime;
			pOption->iVolume = iDefaultVol;
			pOption->flDelay = 0.0f;

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

					char szValue[5];
					char *pName = &pFile[i]; 
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
							szValue[j] = 0;
							break;
						}

						szValue[j] = pFile[i];
						i++; j++;
					}
					
					if(*pName == 'p')
						pOption->iPitch = iDefaultPitch = atoi(szValue);
					else if(*pName == 't')
						pOption->iTime = iDefaultTime = atoi(szValue);
					else if(*pName == 's')
						pOption->iStart = iDefaultStart = atoi(szValue);
					else if(*pName == 'e')
						pOption->iEnd = iDefaultEnd = atoi(szValue);
					else if(*pName == 'v')
						pOption->iVolume = iDefaultVol = atoi(szValue);
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
					pOption->szFile[j] = 0;
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
								pOption->flDelay = 0.5; i++;
	
							break;
						}

						char szValue[5];
						char *pName = &pFile[i]; 
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
								szValue[j] = 0;
								break;
							}

							szValue[j] = pFile[i];
							i++; j++;
						}
						
						if(*pName == 'p')
							pOption->iPitch = atoi(szValue);
						else if(*pName == 't')
							pOption->iTime = atoi(szValue);
						else if(*pName == 's')
							pOption->iStart = atoi(szValue);
						else if(*pName == 'e')
							pOption->iEnd = atoi(szValue);
						else if(*pName == 'v')
							pOption->iVolume = atoi(szValue);
					}

					// Break main loop
					break;
				}

				if(pFile[i] == ',')
				{
					pOption->flDelay = 0.5; i++;
					continue;
				}

				if(i >= iSize)
				{
					gEngfuncs.Con_Printf("Sentences.txt error: Unexpected end!\n");
					gEngfuncs.COM_FreeFile(pFile);
					return;
				}

				pOption->szFile[j] = pFile[i];
				i++; j++;
			}
		}
	}
	
	// Free file
	gEngfuncs.COM_FreeFile(pFile);
}

/*
===================================
PM_PlaySample


===================================
*/
//extern "C" void PM_PlaySample( const char *szFile, float fVolume, int iPitch, float *origin )

extern "C" void PM_PlaySound( int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch )
{
	cl_entity_t *pPlayer = gEngfuncs.GetLocalPlayer();
	gSoundEngine.PlaySound( sample, pPlayer->origin, SND_RELATIVE, channel, volume, pitch, attenuation );//FGP
}

//FGP - support for old EV_PlaySound func's
void CSoundEngine::EV_PlaySound( int ent, float *origin, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, bool relative )
{
	if(ent == gEngfuncs.GetLocalPlayer()->index)//make relative if player is local ent
		fFlags |= (SND_RELATIVE);// | SND_2D);

	PlaySound( sample, origin, fFlags, channel, volume, pitch, attenuation, NULL, ent );
}

//FGP - support for old HUD PlaySound func's
void CSoundEngine::HUD_PlaySound( const char *sample, float volume, int channel )//FGP - added channel
{
	PlaySound(sample, Vector(0, 0, 0), SND_HUD, channel);//FGP - added channel
}



/*
====================
HUDFrame

====================
*/

void CSoundEngine::HUDFrame( void )
{
	//gEngfuncs.Con_Printf("HUDFrame\n");
	if(!m_bReset && m_bGotFrame)
	{
		if(!m_bGotRender)
		{
			gEngfuncs.Con_Printf("Resetting Sound Engine from HUDFrame\n");
			ResetEngine();
			return;
		}
		m_bGotRender = false;
	}
}

/*
====================
CreateEntities

====================
*/
void CSoundEngine::CreateEntities( void )
{
	//gEngfuncs.Con_Printf("CreateEntities\n");
	m_bGotEnts = true;
	m_bGotRender = true;
}

/*
====================
DrawNormalTriangles

====================
*/
void CSoundEngine::DrawNormalTriangles( void )
{
	//gEngfuncs.Con_Printf("DrawNormalTriangles\n");
	m_bGotFrame = true;
	m_bGotRender = true;

	// This can only happen during a levelchange
	if(!m_bReset && !m_bGotEnts)
	{
		gEngfuncs.Con_Printf("Resetting Engine from DrawNormalTriangles\n");
		ResetEngine();
		return;
	}

	m_bGotEnts = false;
}
