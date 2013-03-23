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

ConVar in_player_model		("in_player_model",		"models/abigail.mdl",	FCVAR_REPLICATED,	"Define el modelo del jugador");
ConVar in_beginner_weapon	("in_beginner_weapon",	"0",					0,					"Al estar activado se hacen los efectos de ser principante manejando un arma.");

// Linterna
ConVar in_flashlight				("in_flashlight",				"1", 0, "Activa o desactiva el uso de la linterna.");
ConVar in_flashlight_require_suit	("in_flashlight_require_suit",	"0", 0, "Activa o desactiva el requerimiento del traje de protección para la linterna.");

// Regeneración de salud.
ConVar in_regeneration("in_regeneration",						"1",	FCVAR_REPLICATED | FCVAR_ARCHIVE,	"Estado de la regeneracion de salud");
ConVar in_regeneration_wait_time("in_regeneration_wait_time",	"5.0",  FCVAR_REPLICATED,	"Tiempo de espera en segundos al regenerar salud. (Aumenta según el nivel de dificultad)");
ConVar in_regeneration_rate("in_regeneration_rate",				"3",	FCVAR_REPLICATED,	"Cantidad de salud a regenerar (Disminuye según el nivel de dificultad)");

// Efecto de cansancio.
ConVar in_tired_effect("in_tired_effect",						"1",	FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de cansancio al perder salud.");

// Camara lenta.
ConVar in_timescale_effect("in_timescale_effect",				"1",	 FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de camara lenta al perder salud.");

//=========================================================
// Configuración
//=========================================================

#define HEALTH_REGENERATION_MEDIUM_WAITTIME 2.0		// Dificultad Medio: Tardara 2 segundos más.
#define HEALTH_REGENERATION_MEDIUM_RATE		1		// Dificultad Medio: 1% salud menos.

#define HEALTH_REGENERATION_HARD_WAITTIME	5.0		// Dificultad Dificil: Tardara 5 segundos más.
#define HEALTH_REGENERATION_HARD_RATE		2		// Dificultad Dificil: 2% salud menos.

#define INCLUDE_DIRECTOR	true					// ¿Incluir a InDirector? (Solo para juegos de zombis)

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( player, CIn_Player );

PRECACHE_REGISTER( player );

BEGIN_DATADESC( CIn_Player )
	DEFINE_FIELD( NextHealthRegeneration,	FIELD_INTEGER ),
	DEFINE_FIELD( BodyHurt,					FIELD_INTEGER ),
	DEFINE_FIELD( TasksTimer,				FIELD_INTEGER ),
END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CIn_Player::CIn_Player()
{
	NextHealthRegeneration	= gpGlobals->curtime;	// Regeneración de salud
	BodyHurt				= 0;					// Efecto de muerte.
	TasksTimer				= 0;					// Tareas.
}

//=========================================================
// Destructor
//=========================================================
CIn_Player::~CIn_Player()
{
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CIn_Player::Precache()
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
void CIn_Player::Spawn()
{
	// Establecemos el modelo del jugador.
	SetModel(in_player_model.GetString());

	ConVarRef host_timescale("host_timescale");
	ConVarRef indirector_enabled("indirector_enabled");

	// Resetear el tiempo.
	if ( host_timescale.GetInt() != 1 )
		engine->ClientCommand(edict(), "host_timescale 1");

	BaseClass::Spawn();
}

//=========================================================
// Crear y empieza el InDirector.
//=========================================================
void CIn_Player::StartDirector()
{
	ConVarRef indirector_enabled("indirector_enabled");
	CInDirector *pDirector	= (CInDirector *)gEntList.FindEntityByClassname(NULL, "info_director");

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
// Bucle de ejecución de tareas. "Pre"
//=========================================================
void CIn_Player::PreThink()
{
	HandleSpeedChanges();
	BaseClass::PreThink();
}

//=========================================================
// Bucle de ejecución de tareas. "Post"
//=========================================================
void CIn_Player::PostThink()
{
	// Si seguimos vivos.
	if ( IsAlive() )
	{
		ConVarRef mat_yuv("mat_yuv");
		ConVarRef host_timescale("host_timescale");

		/*
			Regeneración de salud
		*/
		if ( m_iHealth < GetMaxHealth() && in_regeneration.GetBool() )
		{
			// ¡Hora de regenerar!
			if ( gpGlobals->curtime > NextHealthRegeneration )
			{
				int RegenerationHealth = in_regeneration_rate.GetInt();
				float RegenerationWait = in_regeneration_wait_time.GetFloat();
				
				// Más o menos salud dependiendo del nivel de dificultad.
				switch ( GameRules()->GetSkillLevel() )
				{
					// Medio
					case SKILL_MEDIUM:
						RegenerationHealth	= RegenerationHealth - HEALTH_REGENERATION_MEDIUM_WAITTIME;
						RegenerationWait	= RegenerationWait - HEALTH_REGENERATION_MEDIUM_RATE;
					break;

					// Dificil
					case SKILL_HARD:
						RegenerationHealth	= RegenerationHealth - HEALTH_REGENERATION_HARD_WAITTIME;
						RegenerationWait	= RegenerationWait - HEALTH_REGENERATION_HARD_RATE;
					break;
				}

				// ¿Quieres quitarle vida o que?
				// TODO: Esto sería una buena característica... quizá en un futuro, por ahora NO.
				if ( RegenerationHealth < 1 )
					RegenerationHealth = in_regeneration_rate.GetInt();

				TakeHealth(RegenerationHealth, DMG_GENERIC);
				NextHealthRegeneration = gpGlobals->curtime + RegenerationWait;
			}
		}

		/*
			Efectos - Cansancio y muerte.
		*/
		if ( in_tired_effect.GetBool() )
		{
			// Desactivar escala de grises y parar el sonido de corazon latiendo.
			// Si, me estoy reponiendo.
			if ( m_iHealth > 10 && mat_yuv.GetInt() == 1 )
			{
				mat_yuv.SetValue(0);
				StopSound("Player.Music.Dying");
			}

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

	BaseClass::PostThink();
}

//=========================================================
// Bucle de ejecución de tareas al morir.
//=========================================================
void CIn_Player::PlayerDeathThink()
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
//=========================================================
// FUNCIONES RELACIONADAS AL MOVIMIENTO
//=========================================================
//=========================================================

//=========================================================
// HandleSpeedChanges()
// Controlar los cambios de velocidad.
//=========================================================
void CIn_Player::HandleSpeedChanges()
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
void CIn_Player::StartSprinting()
{
	BaseClass::StartSprinting();
	SetMaxSpeed(CalcWeaponSpeed());
}

//=========================================================
// Paramos de correr
//=========================================================
void CIn_Player::StopSprinting()
{
	ConVarRef hl2_walkspeed("hl2_walkspeed");
	ConVarRef hl2_normspeed("hl2_normspeed");

	BaseClass::StopSprinting();

	// Ajustar la velocidad dependiendo si tenemos el traje o no.
	if ( IsSuitEquipped() )
		SetMaxSpeed(CalcWeaponSpeed(NULL, hl2_walkspeed.GetFloat()));
	else
		SetMaxSpeed(hl2_normspeed.GetFloat());
}

//=========================================================
// Empezar a caminar
//=========================================================
void CIn_Player::StartWalking()
{
	SetMaxSpeed(CalcWeaponSpeed());
	BaseClass::StartWalking();
}

//=========================================================
// Paramos de caminar
//=========================================================
void CIn_Player::StopWalking()
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
void CIn_Player::FlashlightTurnOn()
{
	// Linterna desactivada.
	if ( in_flashlight.GetInt() == 0 )
		return;

	// Es necesario el traje de protección.
	if ( in_flashlight_require_suit.GetBool() && !IsSuitEquipped() )
		return;

	BaseClass::FlashlightTurnOn();
}

//=========================================================
// Calcula la nueva velocidad del jugador dependiendo del
// peso de la arma.
//=========================================================
float CIn_Player::CalcWeaponSpeed(CBaseCombatWeapon *pWeapon, float speed)
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
bool CIn_Player::Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)
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
int CIn_Player::OnTakeDamage(const CTakeDamageInfo &inputInfo)
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
	if ( !ftrivial && fmajor && flHealthPrev >= 75 ) 
	{
		SetSuitUpdate("!HEV_MED2", false, SUIT_NEXT_IN_30MIN);	// Administrando atención medica.

		// 15+ de salud por daños.
		TakeHealth(15, DMG_GENERIC);

		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);	// Administrado morfina.
	}

	if ( !ftrivial && fcritical && flHealthPrev < 75 )
	{
		if ( m_iHealth <= 5 )
			SetSuitUpdate("!HEV_HLTH6", false, SUIT_NEXT_IN_10MIN);	// ¡Emergencia! Muerte inminente del usuario, evacuar zona de inmediato.

		else if ( m_iHealth <= 10 )
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);	// ¡Emergencia! Muerte inminente del usuario.

		else if ( m_iHealth < 20 )
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);	// Precaución, signos vitales criticos.

		// Alertas al azar.
		if ( !random->RandomInt(0,3) && flHealthPrev < 50 )
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);	// Cuidados medicos necesarios.
	}

	if ( g_pGameRules->Damage_IsTimeBased(info.GetDamageType()) && flHealthPrev < 75 )
	{
		if ( flHealthPrev < 50 )
		{
			if ( !random->RandomInt(0,3) )
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);	// Cuidados medicos necesarios.
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN);		// Signos vitales debilitandose.
	}

	// Si seguimos vivos y los efectos estan activados.
	if ( IsAlive() && in_tired_effect.GetBool() )
	{
		ConVarRef mat_yuv("mat_yuv");

		// Salud menor a 10% y escala de grises desactivado.
		if ( m_iHealth < 10 && mat_yuv.GetInt() == 0 )
		{
			// Activamos escala de grises.
			mat_yuv.SetValue(1);

			// ¡Estamos muriendo!
			EmitSound("Player.Music.Dying");
		}

		// Salud menor a 5%
		// Oh man... pobre de ti.
		//if(m_iHealth < 5)
		//	EmitSound("Player.Music.Puddle");
	}

	return fTookDamage;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
//=========================================================
//=========================================================

//=========================================================
// Crea un cadaver
//=========================================================
void CIn_Player::CreateRagdollEntity()
{
	// Ya hay un cadaver.
	if ( m_hRagdoll )
	{
		// Removerlo.
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll	= NULL;
	}

	// Obtenemos el cadaver.
	CHL2Ragdoll *pRagdoll = dynamic_cast< CHL2Ragdoll* >(m_hRagdoll.Get());
	
	// Al parecer no hay ninguno, crearlo.
	if ( !pRagdoll )
		pRagdoll = dynamic_cast< CHL2Ragdoll* >(CreateEntityByName("hl2_ragdoll"));

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer				= this;
		pRagdoll->m_vecRagdollOrigin	= GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity	= GetAbsVelocity();
		pRagdoll->m_nModelIndex			= m_nModelIndex;
		pRagdoll->m_nForceBone			= m_nForceBone;
		pRagdoll->SetAbsOrigin(GetAbsOrigin());
	}

	m_hRagdoll	= pRagdoll;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL SONIDO/MUSICA
//=========================================================
//=========================================================

//=========================================================
// Inicia una música de fondo
//=========================================================
CSoundPatch *CIn_Player::EmitMusic(const char *pName)
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
void CIn_Player::StopMusic(CSoundPatch *pMusic)
{
	if ( !pMusic )
		return;

	VolumeMusic(pMusic, 0);
}

//=========================================================
// Cambia el volumen de una música de fondo
//=========================================================
void CIn_Player::VolumeMusic(CSoundPatch *pMusic, float newVolume)
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
void CIn_Player::FadeoutMusic(CSoundPatch *pMusic, float range)
{
	if ( !pMusic )
		return;

	ENVELOPE_CONTROLLER.SoundFadeOut(pMusic, range, true);
}