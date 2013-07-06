//=====================================================================================//
//
// Controla música.
//
//=====================================================================================//

#include "cbase.h"
#include "env_music.h"

#include "in_utils.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

void EnvMusic::Init()
{
	if ( !soundemitterbase->GetParametersForSound(pSoundName, pParams, GENDER_NONE) )
		return;

	if ( pSoundPlayer )
	{
		CSingleUserRecipientFilter filter(pSoundPlayer);
		filter.MakeReliable();

		pPatch = ENVELOPE_CONTROLLER.SoundCreate(filter, pSoundPlayer->entindex(), pParams.channel, pParams.soundname, pParams.soundlevel);
	}
	else if ( Director() )
	{
		CReliableBroadcastRecipientFilter filter;
		pPatch = ENVELOPE_CONTROLLER.SoundCreate(filter, Director()->entindex(), pParams.channel, pParams.soundname, pParams.soundlevel);
	}

	// BUG!: Es necesario reproducir el sonido aquí o de otra forma no funcionará después.
	ENVELOPE_CONTROLLER.Play(pPatch, 0.0, pParams.pitch);
}

void EnvMusic::Play()
{
	//Assert( !pPatch );
	if ( !pPatch )
		return;

	if ( IsPlaying() )
		return;

	Msg("[ENV MUSIC] Reproduciendo: %s <%f>:<%s> \r\n", pSoundName, pParams.volume, pParams.soundname);

	ENVELOPE_CONTROLLER.Play(pPatch, 100, pParams.pitch);
	bPlaying = true;
}

void EnvMusic::Stop()
{
	//Assert( !pPatch );
	if ( !pPatch )
		return;

	if ( !IsPlaying() )
		return;

	Msg("[ENV MUSIC] Parando: %s <%f> \r\n", pSoundName, pParams.volume);
	Volume(0.0f);
}

void EnvMusic::Volume(float volume)
{
	//Assert( !pPatch );
	if ( !pPatch )
		return;

	if ( !IsPlaying() )
		return;

	ENVELOPE_CONTROLLER.SoundChangeVolume(pPatch, volume, 0);

	if ( volume <= 0.0f )
	{
		ENVELOPE_CONTROLLER.Shutdown(pPatch);
		bPlaying = false;
	}
}

void EnvMusic::Fadeout(float range, bool destroy)
{
	//Assert( !pPatch );
	if ( !pPatch )
		return;

	if ( !IsPlaying() )
		return;

	Msg("[ENV MUSIC] Fadeout: %s \r\n", pSoundName);

	ENVELOPE_CONTROLLER.SoundFadeOut(pPatch, range, destroy);
	bPlaying = false;
}