#include "Sound.h"
#include    <vorbis/codec.h>            // from the vorbis sdk
#include    <vorbis/vorbisfile.h>       // also :)
#include "EngineFiles\Game.h"

CAudio *g_Audio = NULL;
char *gSoundExtentions[] = { ".mp3", ".wav", ".midi", ".ogg" };



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CSoundResource/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CSoundResource::CSoundResource(const std::string fileName)
: m_PCMBuffer(NULL), m_PCMBufferSize(0), m_SoundType(SOUND_TYPE_UNKNOWN),
m_Resource(fileName), m_Initialized(false), m_LengthMilli(0)
{
}

CSoundResource::~CSoundResource()
{
	SAFE_DELETE_ARRAY(m_PCMBuffer);
}

bool CSoundResource::VInitialize()
{
	if (!m_Initialized)
	{
		m_SoundType = CAudio::FindSoundTypeFromFile(m_Resource.m_name.c_str());
		
		int m_PCMBufferSize = g_App->m_ResCache->Create(m_Resource);

		assert(m_PCMBufferSize);
		char *soundbuffer = (char *)g_App->m_ResCache->Get(m_Resource);

		if (soundbuffer == NULL)
			return false;

		switch(m_SoundType)
		{
		case SOUND_TYPE_WAVE:
			ParseWave(soundbuffer, m_PCMBufferSize);
			break;
		case SOUND_TYPE_OGG:
			ParseOgg(soundbuffer, m_PCMBufferSize);
			break;

		default:
			assert(0 && _T("Sound not supported"));
		}

		m_Initialized = true;
		m_LengthMilli = (GetPCMBufferSize() * 1000) / GetFormat()->nAvgBytesPerSec;

	}
	return true;
}

// slightly modified version from Game Coding Complet
bool CSoundResource::ParseWave(char *buffer, int size)
{
	DWORD		file = 0; 
	DWORD		fileEnd = 0; 
	
	DWORD		length = 0;     
	DWORD		type = 0;									

	// mmioFOURCC -- converts four chars into a 4 byte integer code.
	// The first 4 bytes of a valid .wav file is 'R','I','F','F'
	//fread(&type, 1, sizeof(DWORD), fd);
	memcpy(&type, buffer, sizeof(DWORD));
	buffer += sizeof(DWORD);
	DWORD riff = mmioFOURCC('R', 'I', 'F', 'F');
	if(type != riff )
		return false;	
	
	//fread(&length, 1, sizeof(DWORD), fd); // The first integer after RIFF is the length
	memcpy(&length, buffer, sizeof(DWORD));
	buffer += sizeof(DWORD);
	//fread(&type, 1, sizeof(DWORD), fd);   // The second is the block type - another code, expect.
							// 'W','A','V','E' for a legal .wav file
	memcpy(&type, buffer, sizeof(DWORD));
	buffer += sizeof(DWORD);

	if(type != mmioFOURCC('W', 'A', 'V', 'E'))
		return false;		//not a WAV

	// Find the end of the file
	fileEnd = length - 4;
	
	// Load the .wav format and the .wav data
	// Note that these blocks can be in either order.
	while(file < fileEnd)
	{
		//fread(&type, 1, sizeof(DWORD), fd);   
		file += sizeof(DWORD);
		memcpy(&type, buffer, sizeof(DWORD));
		buffer += sizeof(DWORD);

		//fread(&length, 1, sizeof(DWORD), fd);   
		file += sizeof(DWORD);
		memcpy(&length, buffer, sizeof(DWORD));
		buffer += sizeof(DWORD);

		switch(type)
		{
			case mmioFOURCC('f', 'm', 't', ' '):
			{
				//fread(&m_WavFormatEx, 1, length, fd);   
				memcpy(&m_WavFormatEx, buffer, length);
				buffer += length;
				m_WavFormatEx.cbSize = length;
				break;
			}

			case mmioFOURCC('d', 'a', 't', 'a'):
			{
				m_PCMBuffer = SAFE_NEW char[length];
				m_PCMBufferSize = length;
				//size_t bytesRead = fread(m_PCMBuffer, 1, (unsigned int)length, fd); 
				memcpy(m_PCMBuffer, buffer, length);
				int bytesRead = length;
				if (bytesRead < (int)length)
				{
					assert(0 && _T("Couldn't read the sound data!"));
					return false;
				}
				break;
			}
		} 

		file += length;

		// If both blocks have been seen, we can return true.
		if (m_PCMBuffer && m_PCMBufferSize!=0)
			return true;

		// Increment the pointer past the block we just read,
		// and make sure the pointer is byte aliged.
		if (length & 1)
		{
			buffer++;
			file ++;
		}
	} 

	// If we get to here, the .wav file didn't contain all the right pieces.
	return false; 
}

typedef struct OggFile
{
	unsigned char * dataPtr;
	size_t dataSize;
	size_t dataRead;
} OGG_MEMORY_FILE;

size_t vorbis_read(void* data_ptr, size_t byteSize, size_t sizeToRead, void* data_src)
{
	OGG_MEMORY_FILE *oggFile = static_cast<OGG_MEMORY_FILE *> (data_src);
	if (oggFile == NULL)
		return -1;

	size_t sizeToCopy = byteSize*sizeToRead;

	if (sizeToCopy > oggFile->dataSize-oggFile->dataRead)
	{
		sizeToCopy = oggFile->dataSize-oggFile->dataRead;
	}

	if (sizeToCopy)
	{
		memcpy(data_ptr, oggFile->dataPtr+oggFile->dataRead, sizeToCopy);
		oggFile->dataRead += sizeToCopy;
	}

	return sizeToCopy;
}

int vorbis_seek(void *data_src, ogg_int64_t offset, int whence)
{
	OGG_MEMORY_FILE *oggFile = static_cast<OGG_MEMORY_FILE *> (data_src);
	if (oggFile == NULL)
		return -1;

	switch (whence)
	{
	case SEEK_SET:
		{
			ogg_int64_t realOffSet;
			realOffSet = (oggFile->dataSize >= offset) ? offset : oggFile->dataSize;
			oggFile->dataRead = static_cast<size_t>(realOffSet);
			break;
		}

	case SEEK_CUR:
		{
			ogg_int64_t realOffSet;
			realOffSet = (oggFile->dataSize-oggFile->dataRead >= offset) ? offset : oggFile->dataSize-oggFile->dataRead;
			oggFile->dataRead = static_cast<size_t>(realOffSet);
			break;
		}

	case SEEK_END:
		oggFile->dataRead = oggFile->dataSize+1;
		break;
	}

	return 0;
}

int vorbis_close(void *data_src)
{
	return 0;
}

long vorbis_tell(void *data_src)
{
	OGG_MEMORY_FILE *oggFile = static_cast<OGG_MEMORY_FILE *> (data_src);
	if (oggFile == NULL)
		return -1;

	return oggFile->dataRead;
}	

bool CSoundResource::ParseOgg(char *buffer, int length)
{
	OggVorbis_File vf;

	ov_callbacks oggCallBacks;
	OGG_MEMORY_FILE *oggFile = SAFE_NEW OGG_MEMORY_FILE;
	oggFile->dataPtr = (unsigned char *)buffer;
	oggFile->dataSize = length;
	oggFile->dataRead = 0;
	
	oggCallBacks.read_func = vorbis_read;
	oggCallBacks.close_func = vorbis_close;
	oggCallBacks.seek_func = vorbis_seek;
	oggCallBacks.tell_func = vorbis_tell;

	int ov_ret = ov_open_callbacks(oggFile, &vf, NULL, 0, oggCallBacks);

	assert(ov_ret>=0);

	vorbis_info *vi = ov_info(&vf, -1);

	memset(&m_WavFormatEx, 0, sizeof(m_WavFormatEx));

	m_WavFormatEx.cbSize = sizeof(m_WavFormatEx);
	m_WavFormatEx.nChannels = vi->channels;
	m_WavFormatEx.wBitsPerSample = 16;
	m_WavFormatEx.nSamplesPerSec = vi->rate;
	m_WavFormatEx.nAvgBytesPerSec = m_WavFormatEx.nSamplesPerSec*m_WavFormatEx.nChannels*2;
	m_WavFormatEx.nBlockAlign = 2*m_WavFormatEx.nChannels;
	m_WavFormatEx.wFormatTag = 1;

	DWORD size = 4096*16;
	DWORD pos = 0;
	int sec = 0;
	int ret = 1;

	DWORD bytes = (DWORD)ov_pcm_total(&vf,-1);
	bytes *= 2 * vi->channels;
	m_PCMBuffer = SAFE_NEW char[bytes];
	m_PCMBufferSize = bytes;

	while (ret && pos<bytes)
	{
		ret = ov_read(&vf, m_PCMBuffer+pos, size, 0, 2, 1, &sec);
		pos += ret;
		if (bytes - pos < size)
		{
			size = bytes - pos;
		}
	}

	m_LengthMilli = 1000.0f * ov_time_total(&vf, -1);
	ov_clear(&vf);
	delete oggFile;

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CAudio/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CAudio::CAudio():
	m_Initialized(false), m_AllPaused(false)
{
}

void CAudio::VShutdown()
{
	IAudioBuffer *buffer = NULL;
	AudioBufferList::iterator it;
	it = m_AllSamples.begin();
	while(it != m_AllSamples.end())
	{
		buffer = (*it);
		buffer->VStop();
		++it;
		m_AllSamples.pop_front();
	}
}

void CAudio::VStopAllSounds()
{
	IAudioBuffer *buffer = NULL;
	AudioBufferList::iterator it;
	for (it = m_AllSamples.begin(); it != m_AllSamples.end(); it++)
	{
		buffer = (*it);
		buffer->VStop();
	}
	m_AllPaused = true;
}

void CAudio::VPauseAllSounds()
{
	IAudioBuffer *buffer = NULL;
	AudioBufferList::iterator it;
	for (it = m_AllSamples.begin(); it != m_AllSamples.end(); it++)
	{
		buffer = (*it);
		buffer->VStop();
	}
	m_AllPaused = true;
}

void CAudio::VResumeAllSounds()
{
	IAudioBuffer *buffer = NULL;
	AudioBufferList::iterator it;
	for (it = m_AllSamples.begin(); it != m_AllSamples.end(); it++)
	{
		buffer = (*it);
		//buffer->VResume();
	}
	m_AllPaused = false;
}

SoundType CAudio::FindSoundTypeFromFile(char const * const fileName)
{
	int type = SOUND_TYPE_FIRST;

	while (type!=SOUND_TYPE_COUNT)
	{
		if (strstr(fileName, gSoundExtentions[type]))
			return SoundType(type);
		type++;
	}
	return SOUND_TYPE_UNKNOWN;
}

bool CAudio::HasSoundCard(void)
{
	return (g_Audio && g_Audio->VActive());
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CDirectSoundAudio//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


bool CDirectSoundAudio::VInitialize(HWND hWnd)
{
	if (m_Initialized)
		return true;

	m_Initialized = false;

	SAFE_RELEASE( m_pDS);

	HRESULT hr;
	
	if ( FAILED( hr = DirectSoundCreate8(NULL, &m_pDS, NULL)))
		return false;

	if ( FAILED( hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
		return false;

	m_Initialized = true;

	m_AllSamples.clear();
	return true;
}

HRESULT CDirectSoundAudio::SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate)
{
	HRESULT hr;
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	if (m_pDS == NULL)
		return CO_E_NOTINITIALIZED;

	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	if ( FAILED( hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL)))
		return DXUT_ERR(L"CreateSoundBuffer", hr);

	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = (WORD) WAVE_FORMAT_PCM;
	wfx.nChannels = (WORD) dwPrimaryChannels;
	wfx.nSamplesPerSec = (DWORD) dwPrimaryFreq;
	wfx.wBitsPerSample = (WORD) dwPrimaryBitRate;
	wfx.nBlockAlign = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);

	if ( FAILED( hr = pDSBPrimary->SetFormat(&wfx)))
		return DXUT_ERR(L"SetFormat", hr);

	SAFE_RELEASE(pDSBPrimary);
	return S_OK;
}

void CDirectSoundAudio::VShutdown()
{
	if (m_Initialized)
	{
		CAudio::VShutdown();
		SAFE_RELEASE(m_pDS);
		m_Initialized = false;
	}
}

IAudioBuffer *CDirectSoundAudio::VInitAudioBuffer(boost::shared_ptr<CSoundResource> soundResource)
{
	const char* fileExtension = CAudio::FindExtFromSoundType(soundResource->GetSoundType());

	if (m_pDS == NULL)
		return NULL;

	switch (soundResource->GetSoundType())
	{
	case SOUND_TYPE_OGG:
	case SOUND_TYPE_WAVE:
		break;
	default:
		assert(0 && "We don't support that type");
		return NULL;
	}
	
	LPDIRECTSOUNDBUFFER sampleHandle;

	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = 0;
	dsbd.dwBufferBytes = soundResource->GetPCMBufferSize();
	dsbd.lpwfxFormat = const_cast<WAVEFORMATEX *>(soundResource->GetFormat());
	dsbd.guid3DAlgorithm = GUID_NULL;

	HRESULT hr;
	if ( FAILED( hr = m_pDS->CreateSoundBuffer(&dsbd, &sampleHandle, NULL)))
		return NULL;

	IAudioBuffer *audioBuffer = (IAudioBuffer *)(SAFE_NEW CDirectSoundAudioBuffer(sampleHandle, soundResource));
	m_AllSamples.insert(m_AllSamples.begin(), audioBuffer);

	return audioBuffer;
}

void CDirectSoundAudio::VReleaseAudioBuffer(IAudioBuffer *audioBuffer)
{
	if (audioBuffer)
	{
		audioBuffer->VStop();
		m_AllSamples.remove(audioBuffer);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CDirectAudioSoundBuffer////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CDirectSoundAudioBuffer::CDirectSoundAudioBuffer(LPDIRECTSOUNDBUFFER sample, shared_ptr<CSoundResource> resource):
CAudioBuffer(resource)
{
	m_Sample = sample;
	FillBufferWithSound();
}

CDirectSoundAudioBuffer::~CDirectSoundAudioBuffer()
{
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	if (!pDSB)
		return;
	pDSB->Release();
}

void *CDirectSoundAudioBuffer::VGet()
{
	if (!VOnRestore())
		return NULL;
	return m_Sample;
}

bool CDirectSoundAudioBuffer::VPlay(int volume, bool looping)
{
	VStop();

	m_Volume = volume;
	m_isLooping = looping;
	
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	if (!pDSB)
		return false;

	pDSB->SetVolume(volume);

	DWORD dwFlags = looping ? DSBPLAY_LOOPING: 0L;
	return (S_OK == pDSB->Play(0, 0, dwFlags));
}

bool CDirectSoundAudioBuffer::VStop()
{
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	
	if (!g_Audio->VActive())
		return false;
	if (pDSB == NULL)
		return false;

	m_isPaused = true;
	pDSB->Stop();
	return true;
}

bool CDirectSoundAudioBuffer::VResume()
{
	m_isPaused = false;
	return VPlay(VGetVolume(), VIsLooping());
}

bool CDirectSoundAudioBuffer::VTogglePause()
{
	if (!g_Audio->VActive())
		return false;

	if (m_isPaused)
		VResume();
	else
		VStop();

	return true;
}

bool CDirectSoundAudioBuffer::VIsPlaying()
{
	if (!g_Audio->VActive())
		return false;

	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	if (!pDSB)
		return false;

	DWORD dwStatus = 0;
	pDSB->GetStatus(&dwStatus);
	bool bIsPlaying = ( (dwStatus & DSBSTATUS_PLAYING) != 0);
	return bIsPlaying;
}

void CDirectSoundAudioBuffer::VSetVolume(int volume)
{
	if (!g_Audio->VActive())
		return;
	
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();
	if (!pDSB)
		return;

	assert(volume>=0 && volume<=100 && "Volume must be between 0 and 100");
	float coeff = (float) volume /100.0f;
	float range = (DSBVOLUME_MAX - DSBVOLUME_MIN);
	float fvolume = (range * coeff) + DSBVOLUME_MIN;
	pDSB->SetVolume( LONG(fvolume));
}

bool CDirectSoundAudioBuffer::VOnRestore()
{
	HRESULT hr;
	BOOL bRestored;

	if ( FAILED( hr = RestoreBuffer(&bRestored)))
		return NULL;

	if (bRestored)
	{
		if ( FAILED( hr = FillBufferWithSound() ) )
			return NULL;
	}

	return true;
}


HRESULT CDirectSoundAudioBuffer::RestoreBuffer(BOOL *pbWasRestored)
{
	HRESULT hr;

	if (m_Sample == NULL)
		return CO_E_NOTINITIALIZED;
	if (pbWasRestored)
		*pbWasRestored=FALSE;

	DWORD dwStatus;
	if ( FAILED( hr= m_Sample->GetStatus( &dwStatus)))
		return DXUT_ERR(L"GetStatus", hr);

	if (dwStatus & DSBSTATUS_BUFFERLOST)
	{
		do
		{
			hr = m_Sample->Restore();
			if (hr == DSERR_BUFFERLOST)
				Sleep(10);
		}while ( (hr = m_Sample->Restore() ) == DSERR_BUFFERLOST);

		if (pbWasRestored)
			*pbWasRestored=TRUE;

		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

HRESULT CDirectSoundAudioBuffer::FillBufferWithSound()
{
	HRESULT hr;
	VOID	*pDSLockedBuffer = NULL;
	DWORD	dwDSLockedBufferSize = 0;
	DWORD	dwWavDataRead = 0;

	if (m_Sample == NULL)
		return CO_E_NOTINITIALIZED;

	if ( FAILED(hr = RestoreBuffer(NULL) ) )
		return DXUT_ERR(L"RestoreBuffer", hr);

	int pcmBufferSize = m_Resource->GetPCMBufferSize();

	if ( FAILED( hr = m_Sample->Lock( 0, pcmBufferSize, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, NULL, 0L) ) )
		return DXUT_ERR(L"Lock", hr);


	if (pcmBufferSize == 0)
	{
		FillMemory( (BYTE*) pDSLockedBuffer, dwDSLockedBufferSize, (BYTE)(m_Resource->GetFormat()->wBitsPerSample == 8 ? 128 : 0) );
	}
	else
	{
		CopyMemory(pDSLockedBuffer, m_Resource->GetPCMBuffer(), pcmBufferSize);
		if (pcmBufferSize < (int) dwDSLockedBufferSize)
		{
			FillMemory( (BYTE*) pDSLockedBuffer + pcmBufferSize, 
                        dwDSLockedBufferSize - pcmBufferSize, 
                        (BYTE)(m_Resource->GetFormat()->wBitsPerSample == 8 ? 128 : 0 ) );
		}
	}

	m_Sample->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);

	return S_OK;
}

float CDirectSoundAudioBuffer::VGetProgress()
{
	LPDIRECTSOUNDBUFFER pDSB = (LPDIRECTSOUNDBUFFER)VGet();	
	DWORD progress = 0;

	pDSB->GetCurrentPosition(&progress, NULL);

	float length = (float)m_Resource->GetPCMBufferSize();

	return (float)progress / length;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////CSoundProcess//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

CSoundProcess::CSoundProcess(std::string fileName, int typeOfSound, int volume, bool looping):
Process(typeOfSound, 0), m_Volume(volume), m_isLooping(looping)
{
	m_SoundResource.reset(SAFE_NEW CSoundResource(fileName));
	//InitializeVolume();
}

CSoundProcess::~CSoundProcess()
{
	if (m_AudioBuffer)
	{
		g_Audio->VReleaseAudioBuffer(&*m_AudioBuffer);
	}
}

void CSoundProcess::OnInitialize()
{
	if (!m_SoundResource)
		return;

	m_SoundResource->VInitialize();

	IAudioBuffer *buffer = g_Audio->VInitAudioBuffer(m_SoundResource);

	if (!buffer)
	{
		Kill();
		return;
	}

	m_AudioBuffer.reset(buffer);
	Play(m_Volume, m_isLooping);
}

void CSoundProcess::OnUpdate(int deltaMS)
{
	Process::OnUpdate(deltaMS);

	if ( ! m_bInitialUpdate && !IsPlaying())
		Kill();

	if (IsDead() && IsLooping())
		Replay();
}

bool CSoundProcess::IsPlaying()
{
	if ( !m_SoundResource || !m_AudioBuffer)
		return false;

	return m_AudioBuffer->VIsPlaying();
}

void CSoundProcess::Kill()
{
	Process::Kill();
	if ( IsPlaying() )
		Stop();
}

void CSoundProcess::TogglePause()
{
	if(m_AudioBuffer)
	{
		m_AudioBuffer->VTogglePause();
	}
}

void CSoundProcess::Play(const int volume, const bool looping)
{
	assert(volume>=0 && volume<=100 && "Volume must be a number between 0 and 100");

	if(!m_AudioBuffer)
	{
		return;
	}
	
	m_AudioBuffer->VPlay(volume, looping);
}

void CSoundProcess::Stop()
{
	if(m_AudioBuffer)
	{
		m_AudioBuffer->VStop();
	}
}