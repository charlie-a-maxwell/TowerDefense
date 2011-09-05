#pragma once

#include "StdHeader.h"
#include "Process.h"
#include "ResourceCache\ResCache2.h"
#include <dsound.h>
#include <mmsystem.h>

enum SoundType
{
	SOUND_TYPE_FIRST,
	SOUND_TYPE_MP3 = SOUND_TYPE_FIRST,
	SOUND_TYPE_WAVE,
	SOUND_TYPE_MIDI,
	SOUND_TYPE_OGG,

	// This needs to be the last sound type
	SOUND_TYPE_COUNT,
	SOUND_TYPE_UNKNOWN,
};

extern char *gSoundExtentions[];

class CSoundResource
{
public:
	CSoundResource(const std::string fileName);
	virtual ~CSoundResource();

	char const *GetPCMBuffer() const{return m_PCMBuffer;}
	int GetPCMBufferSize() const{return m_PCMBufferSize;}
	SoundType GetSoundType() const {return m_SoundType;}
	WAVEFORMATEX const *GetFormat() {return &m_WavFormatEx;}
	int GetLengthMilli() const {return m_LengthMilli;}

	virtual bool VInitialize();

protected:
	CSoundResource();
	CSoundResource(const CSoundResource& r);
	CSoundResource& operator=(const CSoundResource& r);

	Resource m_Resource;
	char *m_PCMBuffer;
	int m_PCMBufferSize;
	WAVEFORMATEX m_WavFormatEx;
	int m_LengthMilli;
	SoundType m_SoundType;
	bool m_Initialized;

	bool ParseWave(char *buffer, int size);
	bool ParseOgg(char *buffer, int size);

};

class CSoundProcess : public Process
{
public:
	CSoundProcess(std::string fileName, int typeOfSound=2, int volume=100, bool looping=false);
	virtual ~CSoundProcess();

	virtual void OnUpdate(int deltaMS);
	virtual void OnInitialize();
	virtual void Kill();

	virtual void TogglePause();

	void Play(const int volume, const bool looping);
	void Stop();
	bool IsPlaying();
	bool IsLooping() {return m_isLooping;}

protected:
	CSoundProcess();
	void Replay() { m_bInitialUpdate = true; };

	shared_ptr<CSoundResource> m_SoundResource;
	shared_ptr<IAudioBuffer> m_AudioBuffer;

	int m_Volume;
	bool m_isLooping;
};

class CAudioBuffer: public IAudioBuffer
{
public:
	virtual shared_ptr<CSoundResource> const VGetResource() {return m_Resource;}
	virtual bool VIsLooping() {return m_isLooping;}
	virtual int VGetVolume() {return m_Volume;}

protected:
	CAudioBuffer(shared_ptr<CSoundResource> resource)
	{
		m_Resource = resource;
		m_isPaused = false;
		m_isLooping = false;
		m_Volume = 0;
	}

	shared_ptr<CSoundResource> m_Resource;
	bool m_isPaused;
	bool m_isLooping;
	int m_Volume;
};

typedef std::list<IAudioBuffer *> AudioBufferList;

class CAudio : public IAudio
{
public:
	CAudio();
	virtual void VStopAllSounds();
	virtual void VPauseAllSounds();
	virtual void VResumeAllSounds();

	virtual void VShutdown();
	static char const * const FindExtFromSoundType(SoundType type)
		{ return gSoundExtentions[type]; }
	static SoundType FindSoundTypeFromFile( char const * const ext);
	static bool HasSoundCard(void);
	shared_ptr<CSoundResource> GetResource(std::string fileName);

protected:
	AudioBufferList m_AllSamples;
	bool m_AllPaused;
	bool m_Initialized;
};

class CDirectSoundAudio : public CAudio
{
public:
	CDirectSoundAudio() {m_pDS = NULL;}
	virtual bool VActive() {return m_pDS != NULL;}

	virtual IAudioBuffer *VInitAudioBuffer(shared_ptr<CSoundResource> soundResource);
	virtual void VReleaseAudioBuffer(IAudioBuffer* audioBuffer);

	virtual void VShutdown();
	virtual bool VInitialize(HWND hWnd);
protected:
	IDirectSound8 *m_pDS;

	HRESULT SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate);
};

class CDirectSoundAudioBuffer : public CAudioBuffer
{
public:
	CDirectSoundAudioBuffer(LPDIRECTSOUNDBUFFER sample, shared_ptr<CSoundResource> resource);
	~CDirectSoundAudioBuffer();
	virtual void *VGet();
	virtual bool VOnRestore();
	virtual bool VPlay(int volume, bool looping);
	virtual bool VStop();
	virtual bool VResume();
	virtual bool VTogglePause();
	virtual bool VIsPlaying();
	virtual void VSetVolume(int volume);
	virtual float VGetProgress();

private:
	HRESULT FillBufferWithSound();
	HRESULT RestoreBuffer( BOOL* pbWasRestored);

protected:
	LPDIRECTSOUNDBUFFER m_Sample;
};


extern CAudio *g_Audio;