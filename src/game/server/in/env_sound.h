#ifndef ENV_SOUND_H

#define ENV_SOUND_H

#ifdef _WIN32
#pragma once
#endif

#define SF_AMBIENT_SOUND_EVERYWHERE			4
#define SF_AMBIENT_START_SPAWN				8
//#define SF_AMBIENT_SOUND_NOT_LOOPING		16

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

class CEnvSound : public CPointEntity
{
public:
	DECLARE_CLASS(CEnvSound, CPointEntity);
	DECLARE_DATADESC();

	CEnvSound();

	void Spawn();
	void Precache();
	void Activate();

	void UpdateEntity();
	soundlevel_t ComputeSoundlevel(float radius);
	void ComputeMaxAudibleDistance();
	void SetTransmit(CCheckTransmitInfo *pInfo, bool bAlways);
	void UpdateOnRemove();

	void Play();
	void PlayManual(float Volume, int Pitch);
	void Stop(bool destroy = false);
	void Volume(float volume);
	void Pitch(float pitch);
	//void FadeIn();
	void FadeOut(int seconds);

	void SetSourceEntityName(string_t pEntity) { SourceEntityName = pEntity; };
	void SetSourceEntity(CBaseEntity *pEntity) { SourceEntity = pEntity; };
	void SetSoundName(string_t pName) { SoundName = pName; };
	void SetRadius(float pRadius) { Radius = pRadius; };
	void SetPitch(int pPitch) { SoundPitch = pPitch; };
	void SetVolume(float pVolume) { SoundVolume = pVolume; };

	// Input handlers
	void InputPlaySound(inputdata_t &inputdata);
	void InputStopSound(inputdata_t &inputdata);
	void InputToggleSound(inputdata_t &inputdata);
	void InputPitch(inputdata_t &inputdata);
	void InputVolume(inputdata_t &inputdata);
	//void InputFadeIn(inputdata_t &inputdata);
	void InputFadeOut(inputdata_t &inputdata);
	void InputSetSource(inputdata_t &inputdata);

private:
	CSoundPatch *pSound;

	string_t SoundName;
	float SoundVolume;
	soundlevel_t SoundLevel;
	bool PlayingSound;

	int SoundPitch;
	float Radius;
	float MaxRadius;

	string_t SourceEntityName;
	EHANDLE SourceEntity;
	int SourceEntityIndex;
};

#endif // ENV_SOUND_H