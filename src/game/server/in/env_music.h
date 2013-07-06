//=====================================================================================//
//
// Controla música.
//
//=====================================================================================//

#ifndef ENV_MUSIC_H

#define ENV_MUSIC_H

#ifdef _WIN32
#pragma once
#endif

//#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

class EnvMusic
{
public:
	EnvMusic(const char *pName, CBasePlayer *pPlayer = NULL)
	{
		pSoundName		= pName;
		pSoundPlayer	= pPlayer;
		bPlaying		= false;
		pPatch			= NULL;

		Init();
	}

	virtual void Init();
	virtual bool IsPlaying() { return bPlaying;  }

	virtual void Play();
	virtual void Stop();

	virtual void Volume(float volume);
	virtual void Fadeout(float range = 2.0f, bool destroy = false);

private:
	CSoundPatch *pPatch;
	CSoundParameters pParams;

	CBasePlayer *pSoundPlayer;

	const char *pSoundName;
	bool bPlaying;
};

#endif // ENV_MUSIC_H