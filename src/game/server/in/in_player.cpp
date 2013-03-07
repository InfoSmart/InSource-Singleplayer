//=====================================================================================//
//
// Sistema encargado de constrolar al personaje.
//
//=====================================================================================//

#include "cbase.h"
#include "in_player.h"

#include "npcevent.h"
#include "eventlist.h"

#include "director.h"
#include "scripted.h"
#include "in_buttons.h"

#include "tier0/memdbgon.h"

// Necesario para la música de fondo.
extern ISoundEmitterSystemBase *soundemitterbase;

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar in_player_model		("in_player_model", "models/olivia_citizen.mdl", FCVAR_REPLICATED, "Define el modelo del jugador");
ConVar in_beginner_weapon	("in_beginner_weapon", "0", 0, "Al estar activado se hacen los efectos de ser principante manejando un arma.");

// Linterna
ConVar in_flashlight				("in_flashlight", "1", 0, "Activa o desactiva el uso de la linterna.");
ConVar in_flashlight_require_suit	("in_flashlight_require_suit", "0", 0, "Activa o desactiva el requerimiento del traje de protección para la linterna.");

// Regeneración de salud.
ConVar in_regeneration("in_regeneration", "1", FCVAR_REPLICATED, "Estado de la regeneracion de salud");
ConVar in_regeneration_wait_time("in_regeneration_wait_time", "10.0", FCVAR_REPLICATED, "Tiempo de espera para regenerar");
ConVar in_regeneration_rate("in_regeneration_rate", "0.1", FCVAR_REPLICATED, "Rango de tiempo para regenerar");

// Recarga automatica de arma.
//ConVar in_automatic_reload("in_automatic_reload", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Recarga automatica del arma");

// Efecto de cansancio.
ConVar in_tired_effect("in_tired_effect", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de cansancio al perder salud.");

// Camara lenta.
ConVar in_timescale_effect("in_timescale_effect", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de camara lenta al perder salud.");

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( player, CIN_Player );

PRECACHE_REGISTER( player );

BEGIN_DATADESC(CIN_Player)
	DEFINE_FIELD( RegenRemander,	FIELD_INTEGER ),
	DEFINE_FIELD( BodyHurt,			FIELD_INTEGER ),
	DEFINE_FIELD( TasksTimer,		FIELD_INTEGER ),
END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CIN_Player::CIN_Player()
{
	RegenRemander	= 0; // Regeneración de salud
	BodyHurt		= 0; // Efecto de muerte.
	TasksTimer		= 0; // Tareas.
}

//=========================================================
// Destructor
//=========================================================
CIN_Player::~CIN_Player()
{
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CIN_Player::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound("Player.Pain");
	PrecacheScriptSound("Player.Music.Dying");
	PrecacheScriptSound("Player.Music.Dead");
	PrecacheScriptSound("Player.Music.Puddle");

	PrecacheModel(in_player_model.GetString());
}

//=========================================================
// Crear al jugador.
//=========================================================
void CIN_Player::Spawn()
{
	// Establecemos el modelo del jugador.
	SetModel(in_player_model.GetString());

	ConVarRef host_timescale("host_timescale");
	ConVarRef indirector_enabled("indirector_enabled");

	// Resetear el tiempo.
	if( host_timescale.GetInt() != 1 )
		engine->ClientCommand(edict(), "host_timescale 1");

	BaseClass::Spawn();
}

//=========================================================
// Bucle de ejecución de tareas. "Pre"
//=========================================================
void CIN_Player::PreThink()
{
	HandleSpeedChanges();
	BaseClass::PreThink();
}

//=========================================================
// Bucle de ejecución de tareas. "Post"
//=========================================================
void CIN_Player::PostThink()
{
	// El juego no ha acabado.
	if ( !g_fGameOver && !IsPlayerLockedInPlace() )
	{
		// Si seguimos vivos.
		if ( IsAlive() )
		{
			ConVarRef mat_yuv("mat_yuv");
			ConVarRef host_timescale("host_timescale");
			
			// Regeneración de salud
			// TODO: Esto es un código de ejemplo para "empezar con Source Engine", debemos hacer algo mejor y más sencillo.
			if( m_iHealth < GetMaxHealth() && in_regeneration.GetBool() )
			{
				if ( gpGlobals->curtime > (LastDamageTime() + in_regeneration_wait_time.GetFloat()) )
				{
					RegenRemander += in_regeneration_rate.GetFloat() * gpGlobals->frametime;

					if ( RegenRemander >= 1 )
					{
						TakeHealth(RegenRemander, DMG_GENERIC);
						RegenRemander = 0;
					}
				}
			}

			// Efectos - Cansancio y muerte.
			if ( in_tired_effect.GetBool() )
			{
				// Desactivar escala de grises y parar el sonido de corazon latiendo.
				// Si, me estoy reponiendo.
				if ( m_iHealth > 10 && mat_yuv.GetInt() == 1 )
				{
					mat_yuv.SetValue(0);
					StopSound("Player.Music.Dying");
				}

				// Parar el sonido de "pobre de ti"
				// Si, me estoy reponiendo.
				if ( m_iHealth > 5 )
					StopSound("Player.Music.Puddle");

				// Tornamos la pantalla oscura.
				// Oh, me duele tanto que no veo bien.
				if ( m_iHealth <= 30 )
				{
					int DeathFade = (230 - (6 * m_iHealth));
					color32 black = {0, 0, 0, DeathFade};

					UTIL_ScreenFade(this, black, 1.0f, 0.1f, FFADE_IN);
				}

				// Hacemos que la camara tiemble.
				// Encerio, me duele tanto que no puedo agarrar bien mi arma.
				if ( m_iHealth < 10 )
					UTIL_ScreenShake(GetAbsOrigin(), 1.0, 1.0, 1.0, 750, SHAKE_START, true);

				// Efectos - Dolor del cuerpo.
				if ( m_iHealth < 60 && gpGlobals->curtime > BodyHurt )
				{
					int AngHealth = (100 - m_iHealth) / 8;
					ViewPunch(QAngle(random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth)));

					// Proximo efecto dentro de 10 a 20 segundos.
					BodyHurt = gpGlobals->curtime + (random->RandomInt(10, 20));
				}
			}

			// Efectos - Camara lenta.
			if ( in_timescale_effect.GetBool() )
			{
				// Salud menor a 10%
				// Escala de tiempo: 0.6
				if ( m_iHealth < 10 && host_timescale.GetFloat() != 0.6 )
					engine->ClientCommand(edict(), "host_timescale 0.6");

				// Salud menor a 15%
				// Escala de tiempo: 0.8
				else if ( m_iHealth < 15 && host_timescale.GetFloat() != 0.8 )
					engine->ClientCommand(edict(), "host_timescale 0.8");

				// Salud mayor a 15%
				// Escala de tiempo: 1 (Real)
				else if ( m_iHealth > 15 && host_timescale.GetFloat() != 1 )
					host_timescale.SetValue(1);
			}
		}
	}

	BaseClass::PostThink();
}

//=========================================================
// Bucle de ejecución de tareas al morir.
//=========================================================
void CIN_Player::PlayerDeathThink()
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	// Efectos - Camara lenta.
	if ( in_timescale_effect.GetBool() )
	{
		ConVarRef host_timescale("host_timescale");

		if( host_timescale.GetFloat() != 0.5 )
			engine->ClientCommand(edict(), "host_timescale 0.5");
	}

	BaseClass::PlayerDeathThink();
}

//=========================================================
// Crear y empieza el InDirector.
//=========================================================
void CIN_Player::StartDirector()
{
	ConVarRef indirector_enabled("indirector_enabled");
	CInDirector *pDirector = (CInDirector *)gEntList.FindEntityByClassname(NULL, "info_director");

	// Si el Director esta activado
	// y no se ha creado la entidad info_director
	if ( indirector_enabled.GetBool() && !pDirector )
	{
		Vector vecForward;
		AngleVectors(EyeAngles(), &vecForward);

		Vector vecOrigin = GetAbsOrigin() + vecForward;
		QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

		CInDirector *mDirector = (CInDirector *)CBaseEntity::Create("info_director", vecOrigin, vecAngles, NULL);
		mDirector->SetName(MAKE_STRING("director"));
	}
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL MOVIMIENTO
//=========================================================
//=========================================================

//=========================================================
// HandleSpeedChanges()
// Controlar los cambios de velocidad.
//=========================================================
void CIN_Player::HandleSpeedChanges()
{
	int buttonsChanged	= m_afButtonPressed | m_afButtonReleased;
	bool bCanSprint		= CanSprint();
	bool bIsSprinting	= IsSprinting();
	bool bWantSprint	= (bCanSprint && IsSuitEquipped() && (m_nButtons & IN_SPEED));

	ConVarRef sv_stickysprint("sv_stickysprint");

	// Al parecer el jugador desea correr.
	if (bIsSprinting != bWantSprint && (buttonsChanged & IN_SPEED))
	{
		// En esta sección se verifica que el jugador realmente esta presionando el boton indicado para correr.
		// Ten en cuenta que el comando "sv_stickysprint" sirve para activar el modo de "correr sin mantener presionado el boton"
		// Por lo tanto tambien hay que verificar que el usuario disminuya su velocidad para detectar que desea desactivar este modo.

		if (bWantSprint)
		{
			// Correr sin mantener presionado el boton.
			if (sv_stickysprint.GetBool())
				StartAutoSprint();
			else
				StartSprinting();
		}
		else
		{
			if (!sv_stickysprint.GetBool())
				StopSprinting();
			
			// Quitar el estado de "presionado" a la tecla de correr.
			m_nButtons &= ~IN_SPEED;
		}
	}

	bool bIsWalking		= IsWalking();
	bool bWantWalking;	
	
	// Tenemos el traje de protección y no estamos ni corriendo ni agachados.
	if(IsSuitEquipped())
		bWantWalking = (m_nButtons & IN_WALK) && !IsSprinting() && !(m_nButtons & IN_DUCK);
	else
		bWantWalking = true;
	
	// Iván: Creo que esto no funciona... StartWalking() jamas es llamado ¿Solución?
	if(bIsWalking != bWantWalking)
	{
		if (bWantWalking)
			StartWalking();
		else
			StopWalking();
	}

	BaseClass::HandleSpeedChanges();
}

//=========================================================
// Empezar a correr
//=========================================================
void CIN_Player::StartSprinting()
{
	BaseClass::StartSprinting();
	SetMaxSpeed(CalcWeaponSpeed());
}

//=========================================================
// Paramos de correr
//=========================================================
void CIN_Player::StopSprinting()
{
	ConVarRef hl2_walkspeed("hl2_walkspeed");
	ConVarRef hl2_normspeed("hl2_normspeed");

	BaseClass::StopSprinting();

	// Ajustar la velocidad dependiendo si tenemos el traje o no.
	if(IsSuitEquipped())
		SetMaxSpeed(CalcWeaponSpeed(NULL, hl2_walkspeed.GetFloat()));
	else
		SetMaxSpeed(hl2_normspeed.GetFloat());
}

//=========================================================
// Empezar a caminar
//=========================================================
void CIN_Player::StartWalking()
{
	SetMaxSpeed(CalcWeaponSpeed());
	BaseClass::StartWalking();
}

//=========================================================
// Paramos de caminar
//=========================================================
void CIN_Player::StopWalking()
{
	SetMaxSpeed(CalcWeaponSpeed());
	BaseClass::StopWalking();
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ARMAS
//=========================================================
//=========================================================

//=========================================================
// Encender nuestra linterna.
//=========================================================
void CIN_Player::FlashlightTurnOn()
{
	// Linterna desactivada.
	if(in_flashlight.GetInt() == 0)
		return;

	// Es necesario el traje de protección.
	if(in_flashlight_require_suit.GetBool() && !IsSuitEquipped())
		return;

	BaseClass::FlashlightTurnOn();
}

//=========================================================
// Calcula la nueva velocidad del jugador dependiendo del
// peso de la arma.
//=========================================================
float CIN_Player::CalcWeaponSpeed(CBaseCombatWeapon *pWeapon, float speed)
{
	if (pWeapon == NULL)
		pWeapon = dynamic_cast<CBaseCombatWeapon *>(GetActiveWeapon());

	if (!pWeapon)
		return 0;

	ConVarRef hl2_sprintspeed("hl2_sprintspeed");
	ConVarRef hl2_walkspeed("hl2_walkspeed");

	if (speed == 0 && IsSprinting())
		speed = hl2_sprintspeed.GetFloat();

	if (speed == 0 && !IsSprinting())
		speed = hl2_walkspeed.GetFloat();

	float newSpeed = pWeapon->GetWpnData().m_WeaponWeight;

	if(newSpeed < 0)
	{
		newSpeed = fabs(newSpeed);
		return speed + newSpeed;
	}

	return speed - newSpeed;
}

//=========================================================
// Verifica si es posible cambiar a determinada arma.
//=========================================================
bool CIN_Player::Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)
{
	// Cuando el arma cambia también hay que actualizar la velocidad del jugador con el peso de la misma.
	// Puedes cambiar el peso en los scripts de las armas, variable: "slow_speed"
	// TODO: Cambiar a un lugar más apropiado	

	if (IsSprinting())
		SetMaxSpeed(CalcWeaponSpeed(pWeapon));

	// Debido a que StartWalking() jamas es llamado (ver la función HandleSpeedChanges())
	// una solución temporal es verificar que no estemos corriendo...
	if (!IsSprinting())
		SetMaxSpeed(CalcWeaponSpeed(pWeapon));

	return BaseClass::Weapon_CanSwitchTo(pWeapon);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL DAÑO/SALUD
//=========================================================
//=========================================================

//=========================================================
// Al sufrir daño.
//=========================================================
int CIN_Player::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	int fTookDamage		= BaseClass::OnTakeDamage(inputInfo);
	int fmajor			= (m_lastDamageAmount > 25);
	int fcritical		= (m_iHealth < 30);
	int ftrivial		= (m_iHealth > 75 || m_lastDamageAmount < 5);
	float flHealthPrev	= m_iHealth;

	CTakeDamageInfo info = inputInfo;

	if (!fTookDamage)
		return 0;

	/*
		SONIDOS
		Aquí solo reproducimos los sonidos del traje de protección dependiendo del daño causado.
	*/

	// Sonidos del traje realisticos.
	if (!ftrivial && fmajor && flHealthPrev >= 75) 
	{
		SetSuitUpdate("!HEV_MED2", false, SUIT_NEXT_IN_30MIN);	// Administrando atención medica.

		// 15+ de salud por daños.
		TakeHealth(15, DMG_GENERIC);

		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);	// Administrado morfina.
	}

	if (!ftrivial && fcritical && flHealthPrev < 75)
	{
		if (m_iHealth <= 5)
			SetSuitUpdate("!HEV_HLTH6", false, SUIT_NEXT_IN_10MIN);	// ¡Emergencia! Muerte inminente del usuario, evacuar zona de inmediato.

		else if (m_iHealth <= 10)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);	// ¡Emergencia! Muerte inminente del usuario.

		else if (m_iHealth < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);	// Precaución, signos vitales criticos.

		// Alertas al azar.
		if (!random->RandomInt(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);	// Cuidados medicos necesarios.
	}

	if (g_pGameRules->Damage_IsTimeBased(info.GetDamageType()) && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!random->RandomInt(0,3))
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);	// Cuidados medicos necesarios.
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN);		// Signos vitales debilitandose.
	}

	// Si seguimos vivos y los efectos estan activados.
	if(IsAlive() && in_tired_effect.GetBool())
	{
		ConVarRef mat_yuv("mat_yuv");

		// Salud menor a 10% y escala de grises desactivado.
		if(m_iHealth < 10 && mat_yuv.GetInt() == 0)
		{
			// Activamos escala de grises.
			mat_yuv.SetValue(1);

			// ¡Estamos muriendo!
			EmitSound("Player.Music.Dying");
		}

		// Salud menor a 5%
		// Oh man... pobre de ti.
		if(m_iHealth < 5)
			EmitSound("Player.Music.Puddle");
	}

	return fTookDamage;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
//=========================================================
//=========================================================

//=========================================================
// Ejecuta una acción al momento que el modelo hace
// la animación correspondiente.
//=========================================================
void CIN_Player::HandleAnimEvent(animevent_t *pEvent)
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
		// Nos convertimos en un muñeco de trapo.
		if (pEvent->event == AE_RAGDOLL)
		{
			// Empezamos el bucle de tareas al morir.
			SetThink(&CIN_Player::PlayerDeathThink);
			SetNextThink(gpGlobals->curtime + 0.1f);
		}
	}

	BaseClass::HandleAnimEvent(pEvent);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL SONIDO/MUSICA
//=========================================================
//=========================================================

//=========================================================
// Inicia una música de fondo
//=========================================================
CSoundPatch *CIN_Player::EmitMusic(const char *pName)
{
	CSoundParameters params;

	if ( !soundemitterbase->GetParametersForSound(pName, params, GENDER_NONE) )
		return NULL;

	CPASAttenuationFilter filter(this);
	CSoundPatch *pMusic = ENVELOPE_CONTROLLER.SoundCreate(filter, GetSoundSourceIndex(), params.channel, params.soundname, params.soundlevel);
	ENVELOPE_CONTROLLER.Play(pMusic, params.volume, params.pitch);

	return pMusic;
}

//=========================================================
// Parar una música de fondo
//=========================================================
void CIN_Player::StopMusic(CSoundPatch *pMusic)
{
	if ( !pMusic )
		return;

	VolumeMusic(pMusic, 0);
}

//=========================================================
// Cambia el volumen de una música de fondo
//=========================================================
void CIN_Player::VolumeMusic(CSoundPatch *pMusic, float newVolume)
{
	if ( !pMusic )
		return;

	if ( newVolume > 0 )
		ENVELOPE_CONTROLLER.SoundChangeVolume(pMusic, newVolume, 0);
	else
		ENVELOPE_CONTROLLER.SoundDestroy(pMusic);
}

//=========================================================
// Realiza el efecto de "desaparecer poco a poco" en una
// música de fondo. Aceptando un rango de desaparición.
//=========================================================
void CIN_Player::FadeoutMusic(CSoundPatch *pMusic, float range)
{
	if ( !pMusic )
		return;

	ENVELOPE_CONTROLLER.SoundFadeOut(pMusic, range, true);
}