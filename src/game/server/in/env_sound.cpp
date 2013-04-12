//=====================================================================================//
//
// env_sound
//
// Entidad encargada de reproducir sonidos y musica en el mapa.
// Una versión arreglada y minimizada de ambient_generic
//
//=====================================================================================//

#include "cbase.h"
#include "soundenvelope.h"

#include "env_sound.h"

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

//=========================================================
// Definición de variables de configuración.
//=========================================================

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( env_sound, CEnvSound );

BEGIN_DATADESC( CEnvSound )
	DEFINE_KEYFIELD( SoundName,		FIELD_SOUNDNAME,	"SoundName" ),
	DEFINE_KEYFIELD( SoundVolume,	FIELD_FLOAT,		"SoundVolume" ),

	DEFINE_KEYFIELD( SoundPitch,		FIELD_INTEGER,		"Pitch" ),
	DEFINE_KEYFIELD( Radius,			FIELD_FLOAT,		"Radius" ),
	DEFINE_KEYFIELD( SourceEntityName,	FIELD_STRING,		"SourceEntityName" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,	"PlaySound",	InputPlaySound ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StopSound",	InputStopSound ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ToggleSound",	InputToggleSound ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"Pitch",		InputPitch ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"Volume",		InputVolume ),
	//DEFINE_INPUTFUNC( FIELD_FLOAT,	"FadeIn",		InputFadeIn ), //@ @TODO
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"FadeOut",		InputFadeOut ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetSource",	InputSetSource ),
END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CEnvSound::CEnvSound()
{
	pSound				= NULL;
	//SourceEntity		= NULL;
	SoundLevel			= SNDLVL_NONE;
	PlayingSound		= false;
}

//=========================================================
// Creación
//=========================================================
void CEnvSound::Spawn()
{
	BaseClass::Spawn();

	// Somos invisibles.
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);

	// Caché
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CEnvSound::Precache()
{
	char *SoundFile = (char *)STRING(SoundName);

	if ( SoundName != NULL_STRING && strlen(SoundFile) > 1 )
	{
		if ( *SoundFile != '!' )
			PrecacheScriptSound(SoundFile);
	}
}

//=========================================================
// Activa la entidad.
//=========================================================
void CEnvSound::Activate()
{
	BaseClass::Activate();

	// Actualizamos la información de la entidad de donde proviene.
	UpdateEntity();

	// Reproducir al crearse.
	if ( HasSpawnFlags(SF_AMBIENT_START_SPAWN) )
		Play();
}

void CEnvSound::UpdateEntity()
{
	// La entidad de donde proviene el sonido es nulo.
	if ( SourceEntity == NULL )
	{
		// Hay un nombre para la entidad de donde proviene.
		if ( SourceEntityName != NULL_STRING )
		{
			// Lo buscamos.
			SourceEntity = gEntList.FindEntityByName(NULL, SourceEntityName);

			if ( SourceEntity == NULL )
				SourceEntity = gEntList.FindEntityByClassname(NULL, (char *)STRING(SourceEntityName));

			if ( SourceEntity != NULL )
				SourceEntityIndex = SourceEntity->entindex();
		}

		// Sigue sin haber una entidad, usar este env_sound
		if ( SourceEntity == NULL )
		{
			SourceEntity		= this;
			SourceEntityIndex	= entindex();
		}
	}

	// No reproducir en todos lados, debe ser desde la entidad especifica.
	if ( !HasSpawnFlags(SF_AMBIENT_SOUND_EVERYWHERE) )
	{
		// Computamos el nivel de sonido y la máxima distancia de audición.
		SoundLevel = ComputeSoundlevel(Radius);
		ComputeMaxAudibleDistance();
	}
}

//=========================================================
// Computa el nivel de sonido.
//=========================================================
soundlevel_t CEnvSound::ComputeSoundlevel(float radius)
{
	soundlevel_t soundlevel = SNDLVL_NONE;

	if ( radius > 0 && !HasSpawnFlags(SF_AMBIENT_SOUND_EVERYWHERE) )
	{
		float dB_loss	= 20 * log10(radius / 36.0);
		soundlevel		= (soundlevel_t)(int)(40 + dB_loss); // sound at 40dB at reference distance
	}

	return soundlevel;
}

//=========================================================
// Computa la distancia máxima de audición.
//=========================================================
void CEnvSound::ComputeMaxAudibleDistance()
{
	if ( SoundLevel == SNDLVL_NONE || Radius == 0.0f )
	{
		MaxRadius = -1.0f;
		return;
	}

	float Gain = enginesound->GetDistGainFromSoundLevel(SoundLevel, Radius);

	if ( Gain <= 1.01e-3 )
	{
		MaxRadius = Radius;
		return;
	}

	float flMinRadius = Radius;
	float flMaxRadius = Radius * 2;

	while ( true )
	{
		float Gain = enginesound->GetDistGainFromSoundLevel(SoundLevel, flMaxRadius);

		if ( Gain <= 1.01e-3 )
			break;

		if ( flMaxRadius = 1e5 )
		{
			MaxRadius = -1.0f;
			return;
		}

		flMinRadius = flMaxRadius;
		flMaxRadius *= 2.0f;
	}

	// Now home in a little bit
	int Interations = 4;

	while ( --Interations >= 0 )
	{
		float TestRadius	= (flMinRadius + flMaxRadius) * 0.5f;
		float Gain			= enginesound->GetDistGainFromSoundLevel(SoundLevel, TestRadius);

		if ( Gain <= 1.01e-3 )
			flMaxRadius = TestRadius;
		else
			flMinRadius = TestRadius;
	}

	MaxRadius = flMaxRadius;
}

//=========================================================
//
//=========================================================
void CEnvSound::SetTransmit(CCheckTransmitInfo *pInfo, bool bAlways)
{
	// No hay entidad de origen, somos nosotros o no se esta reproduciendo el sonido.
	if ( !SourceEntity || SourceEntity == this || !PlayingSound )
		return;

	// Esta establecido reproducirse en todos lados.
	if ( HasSpawnFlags(SF_AMBIENT_SOUND_EVERYWHERE) )
		return;

	Assert(pInfo->m_pClientEnt);

	CBaseEntity *pClient = (CBaseEntity*)(pInfo->m_pClientEnt->GetUnknown());

	if ( !pClient )
		return;

	if ( MaxRadius < 0 || pClient->GetAbsOrigin().DistToSqr(SourceEntity->GetAbsOrigin()) <= (MaxRadius * MaxRadius) )
		SourceEntity->SetTransmit(pInfo, false);
}

//=========================================================
//
//=========================================================
void CEnvSound::UpdateOnRemove()
{
	if ( PlayingSound )
		Stop();

	BaseClass::UpdateOnRemove();
}

//=========================================================
// Reproduce el sonido.
//=========================================================
void CEnvSound::Play()
{
	char *pName = (char *)STRING(SoundName);

	Msg("[1] SE INTENTA REPRODUCIR: %s - Volume: %f - Pitch: %i - Level: %i - Radio: %i \r\n", pName, SoundVolume, SoundPitch, (int)SoundLevel, Radius);

	if ( SoundVolume > 1 || !SoundVolume )
		SoundVolume = 1;

	if ( SoundVolume < 0 )
		SoundVolume = 0;

	if ( SoundPitch > 255 || !SoundPitch )
		SoundPitch = 100;

	if ( SoundPitch < 0 )
		SoundPitch = 0;

	Msg("[2] SE INTENTA REPRODUCIR: %s - Volume: %f - Pitch: %i - Level: %i - Radio: %i \r\n", pName, SoundVolume, SoundPitch, (int)SoundLevel, Radius);

	// El audio es la ruta a un .wav o .mp3
	if ( strstr(STRING(SoundName), "mp3") || strstr(STRING(SoundName), "wav") )
	{
		CPASAttenuationFilter filter(SourceEntity->GetAbsOrigin());

		// Creamos el sonido.
		if ( pSound == NULL ) 
			pSound = ENVELOPE_CONTROLLER.SoundCreate(filter, SourceEntityIndex, CHAN_STATIC, pName, SoundLevel);

		// Lo reproducimos.
		ENVELOPE_CONTROLLER.Play(pSound, SoundVolume, SoundPitch);
	}

	// Se trata del nombre del audio.
	else
	{
		CSoundParameters params;

		// Obtenemos información del audio.
		if ( !soundemitterbase->GetParametersForSound(pName, params, GENDER_NONE) )
			return;

		CPASAttenuationFilter filter(SourceEntity->GetAbsOrigin());

		// Creamos el sonido.
		if ( pSound == NULL )
			pSound = ENVELOPE_CONTROLLER.SoundCreate(filter, SourceEntityIndex, params.channel, params.soundname, SoundLevel);

		// Lo reproducimos.
		ENVELOPE_CONTROLLER.Play(pSound, SoundVolume, SoundPitch);
	}
	
	PlayingSound = true;
}

void CEnvSound::PlayManual(float Volume, int Pitch)
{
	char *pName = (char *)STRING(SoundName);

	Msg("[1] SE INTENTA REPRODUCIR: %s - Volume: %f - Pitch: %i - Level: %i - Radio: %f \r\n", pName, Volume, Pitch, (int)SoundLevel, Radius);

	// El audio es la ruta a un .wav o .mp3
	if ( strstr(STRING(SoundName), "mp3") || strstr(STRING(SoundName), "wav") )
	{
		CPASAttenuationFilter filter(SourceEntity->GetAbsOrigin());

		// Creamos el sonido.
		if ( pSound == NULL ) 
			pSound = ENVELOPE_CONTROLLER.SoundCreate(filter, SourceEntityIndex, CHAN_STATIC, pName, SoundLevel);

		// Lo reproducimos.
		ENVELOPE_CONTROLLER.Play(pSound, Volume, Pitch);
	}

	// Se trata del nombre del audio.
	else
	{
		CSoundParameters params;

		// Obtenemos información del audio.
		if ( !soundemitterbase->GetParametersForSound(pName, params, GENDER_NONE) )
			return;

		CPASAttenuationFilter filter(SourceEntity->GetAbsOrigin());

		// Creamos el sonido.
		if ( pSound == NULL )
			pSound = ENVELOPE_CONTROLLER.SoundCreate(filter, SourceEntityIndex, params.channel, params.soundname, SoundLevel);

		// Lo reproducimos.
		ENVELOPE_CONTROLLER.Play(pSound, Volume, Pitch);
	}
	
	PlayingSound = true;
}

//=========================================================
// Para el sonido.
//=========================================================
void CEnvSound::Stop()
{
	if ( !pSound )
		return;

	ENVELOPE_CONTROLLER.Shutdown(pSound);
	PlayingSound = false;
}

//=========================================================
// Cambia el volumen del sonido.
//=========================================================
void CEnvSound::Volume(float volume)
{
	if ( !pSound )
		return;

	ENVELOPE_CONTROLLER.SoundChangeVolume(pSound, volume, 0);
}

//=========================================================
// Cambia el tono del sonido.
//=========================================================
void CEnvSound::Pitch(float pitch)
{
	if ( !pSound )
		return;

	ENVELOPE_CONTROLLER.SoundChangePitch(pSound, pitch, 0);
}

//=========================================================
// Para la música en un determinado tiempo.
//=========================================================
void CEnvSound::FadeOut(int seconds)
{
	if ( !pSound )
		return;

	ENVELOPE_CONTROLLER.SoundFadeOut(pSound, seconds);
	PlayingSound = false;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS)
//=========================================================
//=========================================================

void CEnvSound::InputPlaySound(inputdata_t &inputdata)
{
	Play();
}

void CEnvSound::InputStopSound(inputdata_t &inputdata)
{
	Stop();
}

void CEnvSound::InputToggleSound(inputdata_t &inputdata)
{
	if ( PlayingSound )
		Stop();
	else
		Play();
}

void CEnvSound::InputPitch(inputdata_t &inputdata)
{
	Pitch(inputdata.value.Float());
}

void CEnvSound::InputVolume(inputdata_t &inputdata)
{
	Volume(inputdata.value.Float());
}

void CEnvSound::InputFadeOut(inputdata_t &inputdata)
{
	FadeOut(inputdata.value.Float());
}

void CEnvSound::InputSetSource(inputdata_t &inputdata)
{
	// @TODO: Hacer funcionar.
	DevMsg("[ENV SOUND] Source: %s \r\n", inputdata.value.String());
	Stop();

	SourceEntity		= NULL;
	SourceEntityName	= MAKE_STRING(inputdata.value.String());

	UpdateEntity();
	Play();
}