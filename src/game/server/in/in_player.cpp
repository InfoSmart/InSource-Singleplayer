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

#include "ammodef.h"
#include "items.h"

#include "tier0/memdbgon.h"

// Necesario para la música de fondo.
extern ISoundEmitterSystemBase *soundemitterbase;

enum
{
	PLAYER_MALE = 1,
	PLAYER_FEMALE
};

#define DMG_BLOODLOST  (1<<30)

//=========================================================
// Definición de variables de configuración.
//=========================================================

// Modelo
ConVar cl_sp_playermodel		("cl_sp_playermodel",		"models/abigail.mdl",	FCVAR_ARCHIVE,						"Define el modelo del jugador en Singleplayer");
ConVar in_beginner_weapon		("in_beginner_weapon",		"0",					FCVAR_NOTIFY | FCVAR_REPLICATED,	"Al estar activado se hacen los efectos de ser principante manejando un arma.");

// Efecto de cansancio.
ConVar in_tired_effect("in_tired_effect",				"1",	FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Activa o desactiva el efecto de cansancio al perder salud.");

// Camara lenta.
ConVar in_timescale_effect("in_timescale_effect",		"1",	FCVAR_ARCHIVE, "Activa o desactiva el efecto de camara lenta al perder salud.");

// Linterna
ConVar in_flashlight				("in_flashlight",				"1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Activa o desactiva el uso de la linterna.");
ConVar in_flashlight_require_suit	("in_flashlight_require_suit",	"0", FCVAR_ARCHIVE, "Activa o desactiva el requerimiento del traje de protección para la linterna.");

// Regeneración de salud.
ConVar in_regeneration("in_regeneration",						"1",	FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_ARCHIVE,	"Estado de la regeneracion de salud");
ConVar in_regeneration_wait_time("in_regeneration_wait_time",	"10",	FCVAR_NOTIFY | FCVAR_REPLICATED,	"Tiempo de espera en segundos al regenerar salud. (Aumenta según el nivel de dificultad)");
ConVar in_regeneration_rate("in_regeneration_rate",				"3",	FCVAR_NOTIFY | FCVAR_REPLICATED,	"Cantidad de salud a regenerar (Disminuye según el nivel de dificultad)");

ConVar sv_in_blood("sv_in_blood",	"5600",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de sangre.");

ConVar sv_in_hunger("sv_in_hunger",				"1000",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de hambre.");
ConVar sv_in_hunger_rate("sv_in_hunger_rate",	"15",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo en minutos para disminuir la cantidad de hambre.");

ConVar sv_in_thirst("sv_in_thirst",				"500",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de sed.");
ConVar sv_in_thirst_rate("sv_in_thirst_rate",	 "10",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo en minutos para disminuir la cantidad de sed.");

ConVar sv_max_inventory("sv_max_inventory", "10",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Número de objetos máximo en el inventario.");

ConVar sv_flashlight_weapon("sv_flashlight_weapon", "0", FCVAR_REPLICATED, "Acoplar las linternas en la boca de las armas?");

//=========================================================
// Configuración
//=========================================================

#define HEALTH_REGENERATION_MEDIUM_WAITTIME 2		// Dificultad Medio: Tardara 2 segundos más.
#define HEALTH_REGENERATION_MEDIUM_RATE		1		// Dificultad Medio: 1% salud menos.

#define HEALTH_REGENERATION_HARD_WAITTIME	5		// Dificultad Dificil: Tardara 5 segundos más.
#define HEALTH_REGENERATION_HARD_RATE		2		// Dificultad Dificil: 2% salud menos.

#define INCLUDE_DIRECTOR	true					// ¿Incluir a InDirector? (Solo para juegos de zombis)

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS( player, CIN_Player );
PRECACHE_REGISTER(player);

BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_INLocalPlayerExclusive )
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_INNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CIN_Player, DT_IN_Player)
	SendPropDataTable( "inlocaldata", 0, &REFERENCE_SEND_TABLE(DT_INLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "innonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_INNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),
END_SEND_TABLE()

BEGIN_DATADESC( CIN_Player )
	DEFINE_FIELD( NextHealthRegeneration,	FIELD_INTEGER ),
	DEFINE_FIELD( BodyHurt,					FIELD_INTEGER ),
	DEFINE_FIELD( TasksTimer,				FIELD_INTEGER ),
	DEFINE_FIELD( m_bBloodWound,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBlood,					FIELD_FLOAT ),
END_DATADESC()

const char *PlayerModels[] =
{
	"models/humans/group03/male_01.mdl",
	"models/humans/group03/male_02.mdl",
	"models/abigail.mdl",
	"models/humans/group03/male_03.mdl",
	"models/humans/group03/female_02.mdl",
	"models/humans/group03/male_04.mdl",
	"models/humans/group03/female_03.mdl",
	"models/humans/group03/male_05.mdl",
	"models/humans/group03/female_04.mdl",
	"models/humans/group03/male_06.mdl",
	"models/humans/group03/female_06.mdl",
	"models/humans/group03/male_07.mdl",
	"models/humans/group03/female_07.mdl",
	"models/humans/group03/male_08.mdl",
	"models/humans/group03/male_09.mdl",
};

//=========================================================
// Constructor
//=========================================================
CIN_Player::CIN_Player()
{
	NextPainSound			= gpGlobals->curtime;	// Sonido de dolor.
	NextHealthRegeneration	= gpGlobals->curtime;	// Regeneración de salud
	BodyHurt				= 0;					// Efecto de muerte.
	TasksTimer				= 0;					// Tareas.

	m_angEyeAngles.Init();

	// Esto solo aplicado al modo supervivencia.
	if ( g_pGameRules->IsMultiplayer() )
	{
		m_bBloodWound		= false;
		m_iBloodSpawn		= gpGlobals->curtime;

		m_HL2Local.m_iBlood		= m_iBlood = sv_in_blood.GetFloat();
		m_HL2Local.m_iHunger	= m_iHunger = sv_in_hunger.GetFloat();
		m_HL2Local.m_iThirst	= m_iThirst = sv_in_thirst.GetFloat();

		m_iHungerTime = gpGlobals->curtime;
		m_iThirstTime = gpGlobals->curtime;
	}
}

//=========================================================
// Destructor
//=========================================================
CIN_Player::~CIN_Player()
{
}

//=========================================================
// Forma más fácil y corta de acceder a un comando del
// lado del cliente.
//=========================================================
const char *CIN_Player::GetConVar(const char *pConVar)
{
	if ( strncmp(pConVar, "cl_", 3) == 0 || pConVar == "crosshair" )
		return engine->GetClientConVarValue(engine->IndexOfEdict(edict()), pConVar);

	ConVarRef pVar(pConVar);

	if ( pVar.IsValid() )
		return pVar.GetString();
	else
		return engine->GetClientConVarValue(engine->IndexOfEdict(edict()), pConVar);
}

//=========================================================
// Forma más fácil y corta de ejecutar un comando.
//=========================================================
void CIN_Player::ExecCommand(const char *pCommand)
{
	// @TODO: ¿Que pasa con los comandos de servidor?
	engine->ClientCommand(edict(), pCommand);
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CIN_Player::Precache()
{
	BaseClass::Precache();

	PrecacheModel("sprites/glow_test02.vmt");
	PrecacheModel("sprites/light_glow03.vmt");
	PrecacheModel("sprites/glow01.vmt");

	PrecacheScriptSound("Player.Pain.Female");
	PrecacheScriptSound("Player.Pain.Male");
	PrecacheScriptSound("Player.Music.Dying");
	PrecacheScriptSound("Player.Music.Dead");
	PrecacheScriptSound("Player.Music.Puddle");

	// En Multiplayer guardamos en caché todos los modelos para los jugadores.
	if ( g_pGameRules->IsMultiplayer() )
	{
		int pModels = ARRAYSIZE(PlayerModels);
		int i;

		for ( i = 0; i < pModels; ++i )
			PrecacheModel(PlayerModels[i]);
	}

	// En Singleplayer Abigaíl es la que debe sufrir.
	else
		PrecacheModel(cl_sp_playermodel.GetString());
}

//=========================================================
// Crear al jugador.
//=========================================================
void CIN_Player::Spawn()
{
	// En Multiplayer no podemos cambiar el tiempo solo por salud.
	if ( !g_pGameRules->IsMultiplayer() )
	{
		const char *host_timescale = GetConVar("host_timescale");

		// Resetear el tiempo.
		if ( host_timescale != "1" )
			ExecCommand("host_timescale 1");

		// Establecemos el modelo para SinglePlayer
		SetModel(cl_sp_playermodel.GetString());
	}
	else
	{
		// Obtenemos el modelo del jugador.
		const char *pPlayerModel = GetConVar("cl_playermodel");

		// Por alguna razón el modelo esta vacio, usar el de Abigaíl (¿Hacker?)
		if ( pPlayerModel == "" || pPlayerModel == NULL )
			pPlayerModel = "models/abigail.mdl";

		// Establecemos el modelo del jugador.
		SetModel(pPlayerModel);

		//if ( !IsObserver() )
		//{
			pl.deadflag = false;
			RemoveSolidFlags(FSOLID_NOT_SOLID);
			RemoveEffects(EF_NODRAW);
		//}

		RemoveEffects(EF_NOINTERP);
		m_nRenderFX			= kRenderNormal;
		m_Local.m_iHideHUD	= 0;

		AddFlag(FL_ONGROUND);
		m_Local.m_bDucked = false;
		SetPlayerUnderwater(false);

		// Reseteamos la sangre.
		m_bBloodWound		= false;
		m_HL2Local.m_iBlood		= m_iBlood = sv_in_blood.GetFloat();
		m_HL2Local.m_iHunger	= m_iHunger = sv_in_hunger.GetFloat();
		m_HL2Local.m_iThirst	= m_iThirst = sv_in_thirst.GetFloat();
	}

	BaseClass::Spawn();
}

//=========================================================
// Crear y empieza el InDirector.
//=========================================================
void CIN_Player::StartDirector()
{
	// @TODO: ¡Mover a un lugar más apropiado!
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
// Selecciona un info_player_start donde aparecer.
//=========================================================
CBaseEntity* CIN_Player::EntSelectSpawnPoint()
{
	// El siguiente código solo es válido en Multiplayer, si estamos en Singleplayer
	// usar el código original.
	if ( !g_pGameRules->IsMultiplayer() )
		return BaseClass::EntSelectSpawnPoint();

	CBaseEntity *pSpot				= NULL;
	CBaseEntity *pLastSpawnPoint	= LastSpawn;
	edict_t	*player					= edict();
	const char *pSpawnPointName		= "info_player_start";

	for ( int i = random->RandomInt(1, 5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnPointName);

	if ( !pSpot )
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnPointName);

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		 if ( pSpot )
		 {
			 if ( g_pGameRules->IsSpawnPointValid(pSpot, this) )
			 {
				 if ( pSpot->GetLocalOrigin() == vec3_origin )
				 {
					 pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnPointName);
					 continue;
				 }

				 goto ReturnSpot;
			 }
		 }

		 pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnPointName);
	} while ( pSpot != pFirstSpot );

	if ( pSpot )
	{
		CBaseEntity *Ent = NULL;

		for ( CEntitySphereQuery sphere(pSpot->GetAbsOrigin(), 128); (Ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			if ( Ent->IsPlayer() && !(Ent->edict() == player) )
				Ent->TakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC));

			goto ReturnSpot;
		}
	}

	if ( !pSpot )
	{
		pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_start");

		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:

	LastSpawn = pSpot;
	return pSpot;
}

//=========================================================
// Bucle de ejecución de tareas. "Pre"
//=========================================================
void CIN_Player::PreThink()
{
	HandleSpeedChanges();

	// Esto solo funciona en modo Survival.
	if ( g_pGameRules->IsMultiplayer() )
	{
		BloodThink();
		HungerThink();
		ThirstThink();
	}

	BaseClass::PreThink();
}

//=========================================================
// Bucle de ejecución de tareas. "Post"
//=========================================================
void CIN_Player::PostThink()
{
	// Si seguimos vivos.
	if ( IsAlive() )
	{
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
		if ( GetConVar("in_tired_effect") == "1" )
		{
			//if ( !g_pGameRules->IsMultiplayer() )
			//{
				const char *mat_yuv	= GetConVar("mat_yuv");

				// Salud menor a 10% y escala de grises desactivado.
				if ( m_iHealth < 10 && mat_yuv == "0" )
				{
					// Activamos escala de grises.
					ExecCommand("mat_yuv 1");

					// ¡Estamos muriendo!
					if ( !g_pGameRules->IsMultiplayer() )
						EmitSound("Player.Music.Dying");
				}

				// Desactivar escala de grises y parar el sonido de corazon latiendo.
				// Si, me estoy reponiendo.
				if ( m_iHealth >= 10 && mat_yuv == "1" )
				{
					ExecCommand("mat_yuv 0");

					if ( !g_pGameRules->IsMultiplayer() )
						StopSound("Player.Music.Dying");
				}
			//}

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
			if ( m_iHealth < 15 )
				UTIL_ScreenShakePlayer(this, 1.0, 1.0, 1.0, SHAKE_START, true);

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
		if ( GetConVar("in_timescale_effect") == "1" && !g_pGameRules->IsMultiplayer() )
		{
			const char *host_timescale	= GetConVar("host_timescale");

			// Salud menor a 10%
			// Escala de tiempo: 0.6
			if ( m_iHealth < 10 && host_timescale != "0.7" )
				ExecCommand("host_timescale 0.7");

			// Salud menor a 15%
			// Escala de tiempo: 0.8
			else if ( m_iHealth < 15 && host_timescale != "0.9" )
				ExecCommand("host_timescale 0.9");

			// Salud mayor a 15%
			// Escala de tiempo: 1 (Real)
			else if ( m_iHealth > 15 && host_timescale != "1" )
				ExecCommand("host_timescale 1");
		}
	}

	m_angEyeAngles = EyeAngles();
	BaseClass::PostThink();
}

//=========================================================
// Bucle de ejecución de tareas al morir.
//=========================================================
void CIN_Player::PlayerDeathThink()
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	// Efectos - Camara lenta.
	if ( GetConVar("in_timescale_effect") == "1" && !g_pGameRules->IsMultiplayer() )
	{
		const char *host_timescale = GetConVar("host_timescale");

		if( host_timescale != "0.5" )
			ExecCommand("host_timescale 0.5");
	}

	// Perdida de luz.
	color32 black = {0, 0, 0, 210};
	UTIL_ScreenFade(this, black, 1.0f, 0.1f, FFADE_IN | FFADE_STAYOUT);

	BaseClass::PlayerDeathThink();
}

//=========================================================
// Bucle de ejecución de tareas para la sangre del jugador.
//=========================================================
void CIN_Player::BloodThink()
{
	// La sangre solo existe en el modo supervivencia.
	if ( !g_pGameRules->IsMultiplayer() )
		return;

	// Tenemos una "herida de sangre"
	if ( m_bBloodWound && m_iBlood > 0 )
	{
		m_iBlood	= m_iBlood - random->RandomFloat(0.1, 1.5);
		int pRand	= random->RandomInt(1, 900);

		if ( m_iBloodSpawn < (gpGlobals->curtime - 3) )
		{
			Vector vecSpot = WorldSpaceCenter();
			vecSpot.x += random->RandomFloat( -1, 1 );
			vecSpot.y += random->RandomFloat( -1, 1 );

			// Lanzar sangre.
			// FIXME: La sangre no se ve desde la vista del jugador pero si desde los otros.
			SpawnBlood(vecSpot, vec3_origin, BLOOD_COLOR_RED, 1);
			m_iBloodSpawn = gpGlobals->curtime;
		}

		// ¡Ha cicatrizado! Suertudo.
		if ( pRand == 843 && m_iBloodTime < (gpGlobals->curtime - 60) )
			m_bBloodWound = false;
	}
	else
	{
		if ( random->RandomInt(1, 100) == 2 && m_iBlood < sv_in_blood.GetFloat() )
			m_iBlood = m_iBlood + 0.1;
	}

	// Ha perdido más del 45% de sangre.
	if ( m_iBlood < 2520 && GetLastDamageTime() < (gpGlobals->curtime - 5) )
	{
		CTakeDamageInfo damage;

		damage.SetAttacker(this);
		damage.SetInflictor(this);
		damage.SetDamageType(DMG_BLOODLOST);

		// ¿100 de sangre? De aquí no pasas.
		if ( m_iBlood < 100 )
			damage.SetDamage(GetHealth() + 5);

		else if ( m_iBlood < 500 )
			damage.SetDamage(5);

		else if ( m_iBlood < 1100 )
			damage.SetDamage(4);

		else if ( m_iBlood < 1400 )
			damage.SetDamage(3);

		else if ( m_iBlood < 1700 )
			damage.SetDamage(3);

		else if ( m_iBlood < 1900 )
			damage.SetDamage(2);

		else
			damage.SetDamage(1);

		TakeDamage(damage);
	}

	m_HL2Local.m_iBlood = GetBlood();
}

void CIN_Player::HungerThink()
{
	// El hambre solo existe en el modo supervivencia.
	if ( !g_pGameRules->IsMultiplayer() )
		return;

	// ¡Tenemos mucha hambre! Empezar a disminuir la sangre.
	if ( m_iHunger < 50 && m_iBlood > 0 )
		m_iBlood = m_iBlood - random->RandomFloat(0.1, 1.5);

	// Aún no toca disminuir el hambre.
	if ( m_iHungerTime >= (gpGlobals->curtime - (sv_in_hunger_rate.GetInt() + 60)) )
		return;

	m_iHunger		= m_iHunger - random->RandomFloat(1.0, 3.0);
	m_iHungerTime	= gpGlobals->curtime;

	m_HL2Local.m_iHunger = GetHunger(); // Lo enviamos al cliente.
}

void CIN_Player::ThirstThink()
{
	// La sed solo existe en el modo supervivencia.
	if ( !g_pGameRules->IsMultiplayer() )
		return;

	// ¡Tenemos mucha sed! Empezar a disminuir la sangre.
	if ( m_iThirst < 30 && m_iBlood > 0 )
		m_iBlood = m_iBlood - random->RandomFloat(0.1, 1.5);

	// Aún no toca disminuir el hambre.
	if ( m_iThirstTime >= (gpGlobals->curtime - (sv_in_thirst_rate.GetInt() + 60)) )
		return;

	m_iThirst		= m_iThirst - random->RandomFloat(1.0, 2.0);
	m_iThirstTime	= gpGlobals->curtime;

	m_HL2Local.m_iThirst = m_iThirst; // Lo enviamos al cliente.
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

	// Al parecer el jugador desea correr.
	if ( bIsSprinting != bWantSprint && (buttonsChanged & IN_SPEED) )
	{
		// En esta sección se verifica que el jugador realmente esta presionando el boton indicado para correr.
		// Ten en cuenta que el comando "sv_stickysprint" sirve para activar el modo de "correr sin mantener presionado el boton"
		// Por lo tanto tambien hay que verificar que el usuario disminuya su velocidad para detectar que desea desactivar este modo.

		if ( bWantSprint )
		{
			// Correr sin mantener presionado el boton.
			//if ( sv_stickysprint.GetBool() )
			//	StartAutoSprint();
			//else
				StartSprinting();
		}
		else
		{
			//if ( sv_stickysprint.GetBool() )
			StopSprinting();

			// Quitar el estado de "presionado" a la tecla de correr.
			m_nButtons &= ~IN_SPEED;
		}
	}

	bool bIsWalking		= IsWalking();
	bool bWantWalking;

	// Tenemos el traje de protección y no estamos ni corriendo ni agachados.
	if ( IsSuitEquipped() )
		bWantWalking = (m_nButtons & IN_WALK) && !IsSprinting() && !(m_nButtons & IN_DUCK);
	else
		bWantWalking = true;

	// Iván: Creo que esto no funciona... StartWalking() jamas es llamado ¿Solución?
	if ( bIsWalking != bWantWalking )
	{
		if ( bWantWalking )
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
	if ( IsSuitEquipped() )
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

	if ( IsSprinting() )
		SetMaxSpeed(CalcWeaponSpeed(pWeapon));

	// Debido a que StartWalking() jamas es llamado (ver la función HandleSpeedChanges())
	// una solución temporal es verificar que no estemos corriendo...
	if ( !IsSprinting() )
		SetMaxSpeed(CalcWeaponSpeed(pWeapon));

	bool Result = BaseClass::Weapon_CanSwitchTo(pWeapon);

	if ( Result )
	{
		if ( pWeapon->GetWpnData().m_expOffset.x == 0 && pWeapon->GetWpnData().m_expOffset.y == 0 )
			ExecCommand("crosshair 1");
		else
		{
			// @TODO: Hacer funcionar
			CBaseViewModel *pVm = GetViewModel();

			if ( pVm->m_bExpSighted == true )
				ExecCommand("crosshair 0");
		}
	}

	return Result;
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

	if ( !fTookDamage )
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

	// Ouch!
	if ( gpGlobals->curtime >= NextPainSound && info.GetDamageType() != DMG_BLOODLOST )
	{
		if ( PlayerGender() == PLAYER_FEMALE )
			EmitSound("Player.Pain.Female");
		else
			EmitSound("Player.Pain.Male");

		NextPainSound = gpGlobals->curtime + random->RandomFloat(0.5, 3.0);
	}

	// Solo en Survival.
	if ( g_pGameRules->IsMultiplayer() )
	{
		// ¡Herido!
		// El inflictor fue un zombi con una probabilidad alta.
		// El daño sufrido fue mayor a 10
		// El inflictor es un Grunt
		// El daño es de bala con una probabilidad alta.
		if ( info.GetInflictor()->Classify() == CLASS_ZOMBIE && random->RandomInt(1, 5) == 2 ||
			m_lastDamageAmount > 10 ||
			info.GetInflictor()->Classify() == CLASS_GRUNT ||
			info.GetDamageType() == DMG_BULLET && random->RandomInt(1, 5) == 2 )
		{
			m_bBloodWound	= true;
			m_iBloodTime	= gpGlobals->curtime;

			m_iBlood = m_iBlood - (m_lastDamageAmount * random->RandomInt(1, 3));
		}
	}

	// Efectos
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade(this, white, 0.2, 0, FFADE_IN);

	return fTookDamage;
}

//=========================================================
// Añade una cantidad de sangre al jugador.
//=========================================================
int CIN_Player::TakeBlood(float flBlood)
{
	int iMax = sv_in_blood.GetInt();

	if ( m_iBlood >= iMax )
		return 0;

	const int oldBlood = m_iBlood;
	m_iBlood += flBlood;

	if ( m_iBlood > iMax )
		m_iBlood = iMax;

	return m_iBlood - oldBlood;
}

//=========================================================
// Cura una herida
//=========================================================
bool CIN_Player::ScarredBloodWound()
{
	if ( !m_bBloodWound )
		return false;

	// Así de sencillo, demasiado...
	m_bBloodWound = false;
	return true;
}

//=========================================================
// Añade una cantidad de hambre al jugador.
//=========================================================
int CIN_Player::TakeHunger(float flHunger)
{
	int iMax = sv_in_hunger.GetInt();

	if ( m_iHunger >= iMax )
		return 0;

	const int oldHunger = m_iHunger;
	m_iHunger += flHunger;

	if ( m_iHunger > iMax )
		m_iHunger = iMax;

	return m_iHunger - oldHunger;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
//=========================================================
//=========================================================

//=========================================================
// Crea un cadaver
//=========================================================
void CIN_Player::CreateRagdollEntity()
{
	// Ya hay un cadaver.
	if ( m_hRagdoll )
	{
		// Removerlo.
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll	= NULL;
	}

	// Obtenemos el cadaver.
	CHL2Ragdoll *pRagdoll = dynamic_cast<CHL2Ragdoll* >(m_hRagdoll.Get());

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
// FUNCIONES DE UTILIDAD
//=========================================================
//=========================================================

//=========================================================
// Dependiendo del modelo, devuelve si el jugador es hombre
// o mujer. (Util para las voces de dolor)
//=========================================================
int CIN_Player::PlayerGender()
{
	// En modo Historia jugamos como Abigaíl (Mujer)
	if ( !g_pGameRules->IsMultiplayer() )
		return PLAYER_FEMALE;

	// Obtenemos el modelo del jugador.
	const char *pPlayerModel = GetConVar("cl_playermodel");

	if ( pPlayerModel == "" || pPlayerModel == NULL )
		return PLAYER_FEMALE;

	if ( Q_stristr(pPlayerModel, "female") || Q_stristr(pPlayerModel, "abigail") )
		return PLAYER_FEMALE;

	return PLAYER_MALE;
}

//=========================================================
// Ejecuta un comando desde el lado del cliente.
//=========================================================
bool CIN_Player::ClientCommand(const CCommand &args)
{
	// Inventario: Soltar objeto
	if ( Q_stristr(args[0], "dropitem") )
	{
		int pFrom = INVENTORY_POCKET;

		if ( Q_stristr(args[1], "backpack") )
			pFrom = INVENTORY_BACKPACK;

		Inventory_DropItemByName(args[2], pFrom);
		return true;
	}

	// Inventario: Usar objeto.
	if ( Q_stristr(args[0], "useitem") )
	{
		int pFrom = INVENTORY_POCKET;

		if ( Q_stristr(args[1], "backpack") )
			pFrom = INVENTORY_BACKPACK;

		Inventory_UseItemByName(args[2], pFrom);
		return true;
	}

	// Inventario: Mover objeto.
	if ( Q_stristr(args[0], "moveitem") )
	{
		int pFrom	= INVENTORY_POCKET;
		int pTo		= INVENTORY_BACKPACK;

		if ( Q_stristr(args[1], "backpack") )
			pFrom = INVENTORY_BACKPACK;

		if ( Q_stristr(args[2], "pocket") )
			pTo = INVENTORY_POCKET;

		Inventory_MoveItemByName(args[3], pFrom, pTo);
		return true;
	}

	return BaseClass::ClientCommand(args);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA MUSICA
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

	ENVELOPE_CONTROLLER.SoundFadeOut(pMusic, range, false);
}





//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL INVENTARIO
//=========================================================
// !!!NOTE: Esto es una implementación muy básica (y asquerosa)
// de un inventario, si alguien más experimentado en esto
// puede mejorarlo ¡bienvenid@!
// PD: Me siento desarrollador de Wikipedia.
//=========================================================
//=========================================================

//=========================================================
// Aplica la escala de munición a partir del nivel de dificultad.
//=========================================================
int InITEM_GiveAmmo(CBasePlayer *pPlayer, float flCount, const char *pszAmmoName, bool bSuppressSound = false)
{
	int iAmmoType = GetAmmoDef()->Index(pszAmmoName);

	if ( iAmmoType == -1 )
	{
		Msg("ERROR: Attempting to give unknown ammo type (%s)\n", pszAmmoName);
		return 0;
	}

	flCount *= g_pGameRules->GetAmmoQuantityScale(iAmmoType);

	// Don't give out less than 1 of anything.
	flCount = max(1.0f, flCount);
	return pPlayer->GiveAmmo(flCount, iAmmoType, bSuppressSound);
}

//=========================================================
// Obtiene la ID de un objeto usable para el inventario.
//=========================================================
int CIN_Player::Inventory_GetItemID(const char *pName)
{
	if ( Q_stristr(pName, "item_blood") )
		return 1;

	if ( Q_stristr(pName, "bandage") )
		return 2;

	if ( Q_stristr(pName, "battery") )
		return 3;

	if ( Q_stristr(pName, "healthkit") )
		return 4;

	if ( Q_stristr(pName, "healthvial") )
		return 5;

	if ( Q_stristr(pName, "ammo_pistol") )
		return 6;

	if ( Q_stristr(pName, "pistol_large") )
		return 7;

	if ( Q_stristr(pName, "ammo_smg1") )
		return 8;

	if ( Q_stristr(pName, "smg1_large") )
		return 9;

	if ( Q_stristr(pName, "ammo_ar2") )
		return 10;

	if ( Q_stristr(pName, "ar2_large") )
		return 11;

	if ( Q_stristr(pName, "ammo_357") )
		return 12;

	if ( Q_stristr(pName, "ammo_357_large") )
		return 13;

	if ( Q_stristr(pName, "ammo_crossbow") )
		return 14;

	if ( Q_stristr(pName, "flare_round") )
		return 15;

	if ( Q_stristr(pName, "box_flare_rounds") )
		return 16;

	if ( Q_stristr(pName, "rpg_round") )
		return 17;

	if ( Q_stristr(pName, "ar2_grenade") )
		return 18;

	if ( Q_stristr(pName, "smg1_grenade") )
		return 19;

	if ( Q_stristr(pName, "box_buckshot") )
		return 20;

	if ( Q_stristr(pName, "ar2_altfire") )
		return 21;

	if ( Q_stristr(pName, "empty_bloodkit") )
		return 22;

	return 0;
}

//=========================================================
// Verifica si el jugador tiene un objeto con esta ID.
//=========================================================
bool CIN_Player::Inventory_HasItem(int pEntity, int pSection)
{
	// Bolsillo
	if ( pSection == INVENTORY_POCKET || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.PocketItems[i] == pEntity )
				return true;
		}
	}

	// Mochila
	if ( pSection == INVENTORY_BACKPACK || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.BackpackItems[i] == pEntity )
				return true;
		}
	}

	return false;
}

//=========================================================
// Agrega un nuevo objeto al inventario.
//=========================================================
int CIN_Player::Inventory_AddItem(const char *pName, int pSection)
{
	// Esto no es válido, mochila o bolsillo.
	if ( pSection == INVENTORY_ALL )
		return 0;

	// Inventario lleno.
	if ( Inventory_GetItemCount(pSection) >= sv_max_inventory.GetInt() )
	{
		ClientPrint(this, HUD_PRINTCENTER, "#Inventory_HUD_Full");
		return 0;
	}

	int pEntity = Inventory_GetItemID(pName);

	// ¡Este objeto no esta registrado!
	if ( pEntity == 0 )
		return 0;

	// No hay ningún objeto con esta ID en la lista, ajustarlo.
	if ( Items[pEntity] == "" || !Items[pEntity] )
		Items[pEntity] = pName;

	if ( pSection == INVENTORY_POCKET )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.PocketItems[i] == 0 || !m_HL2Local.PocketItems[i] )
			{
				// Establecemos el objeto en este slot y actualizamos el inventario.
				m_HL2Local.PocketItems.Set(i, pEntity);
				ExecCommand("cl_update_inventory 1");

				Msg("[INVENTARIO] %s ha recogido %s y lo ha puesto en su bolsillo. \r\n", GetPlayerName(), Inventory_GetItemName(pEntity));
				return i;
			}
		}
	}

	if ( pSection == INVENTORY_BACKPACK )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.BackpackItems[i] == 0 || !m_HL2Local.BackpackItems[i] )
			{
				// Establecemos el objeto en este slot y actualizamos el inventario.
				m_HL2Local.BackpackItems.Set(i, pEntity);
				ExecCommand("cl_update_inventory 1");

				Msg("[INVENTARIO] %s ha recogido %s y lo ha puesto en su mochila. \r\n", GetPlayerName(), Inventory_GetItemName(pEntity));
				return i;
			}
		}
	}

	return 0;
}

//=========================================================
// Agrega un nuevo objeto (desde su ID) al inventario.
//=========================================================
int CIN_Player::Inventory_AddItemByID(int pEntity, int pSection)
{
	const char *pName = Inventory_GetItemName(pEntity); 
	return Inventory_AddItem(pName, pSection);
}

//=========================================================
// Mueve un objeto de una sección a otra.
//=========================================================
bool CIN_Player::Inventory_MoveItem(int pEntity, int pFrom, int pTo)
{
	// Al parecer no tenemos este objeto en nuestro inventario.
	if ( !Inventory_HasItem(pEntity, pFrom) )
		return false;

	// Bolsillo.
	if ( pFrom == INVENTORY_POCKET )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.PocketItems[i] == pEntity )
			{
				Inventory_RemoveItem(pEntity, pFrom);
				Inventory_AddItemByID(pEntity, pTo);

				return true;
			}
		}
	}

	// Mochila.
	if ( pFrom == INVENTORY_BACKPACK )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.BackpackItems[i] == pEntity )
			{
				Inventory_RemoveItem(pEntity, pFrom);
				Inventory_AddItemByID(pEntity, pTo);

				return true;
			}
		}
	}

	return false;
}

//=========================================================
// Mueve un objeto con su nombre desde una sección a otra.
//=========================================================
bool CIN_Player::Inventory_MoveItemByName(const char *pName, int pFrom, int pTo)
{
	int pEntity = Inventory_GetItemID(pName);
	return Inventory_MoveItem(pEntity, pFrom, pTo);
}

//=========================================================
// Remueve un objeto del inventario.
//=========================================================
bool CIN_Player::Inventory_RemoveItem(int pEntity, int pSection)
{
	if ( pSection == INVENTORY_POCKET )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.PocketItems[i] == pEntity )
			{
				// La establecemos en 0 para removerlo y actualizamos el inventario.
				m_HL2Local.PocketItems.Set(i, 0);
				ExecCommand("cl_update_inventory 1");

				return true;
			}
		}
	}

	if ( pSection == INVENTORY_BACKPACK )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
		{
			// Este slot contiene exactamente la ID del objeto.
			if ( m_HL2Local.BackpackItems[i] == pEntity )
			{
				// La establecemos en 0 para removerlo y actualizamos el inventario.
				m_HL2Local.BackpackItems.Set(i, 0);
				ExecCommand("cl_update_inventory 1");

				return true;
			}
		}	
	}

	return false;
}

//=========================================================
// Remueve el objeto que se encuentre en esta posición.
//=========================================================
bool CIN_Player::Inventory_RemoveItemByPos(int Position, int pSection)
{
	if ( pSection == INVENTORY_POCKET )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.PocketItems[Position] == 0 || !m_HL2Local.PocketItems[Position] )
			return false;

		// La establecemos en 0 para remover el objeto que estaba aquí.
		m_HL2Local.PocketItems.Set(Position, 0);
	}

	if ( pSection == INVENTORY_BACKPACK )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.BackpackItems[Position] == 0 || !m_HL2Local.BackpackItems[Position] )
			return false;

		// La establecemos en 0 para remover el objeto que estaba aquí.
		m_HL2Local.BackpackItems.Set(Position, 0);
	}

	ExecCommand("cl_update_inventory 1");
	return true;
}

//=========================================================
// Suelta todos los objetos del inventario.
//=========================================================
void CIN_Player::Inventory_DropAll()
{
	// Verificamos cada slot del inventario.
	for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
	{
		// Este slot no tiene ningún objeto.
		if ( m_HL2Local.PocketItems[i] == 0 || !m_HL2Local.PocketItems[i] )
			continue;

		Inventory_DropItem(m_HL2Local.PocketItems[i], INVENTORY_POCKET, true);
	}

	// Verificamos cada slot del inventario.
	for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
	{
		// Este slot no tiene ningún objeto.
		if ( m_HL2Local.BackpackItems[i] == 0 || !m_HL2Local.BackpackItems[i] )
			continue;

		Inventory_DropItem(m_HL2Local.BackpackItems[i], INVENTORY_BACKPACK, true);
	}
}

//=========================================================
// Suelta un objeto del inventario.
//=========================================================
bool CIN_Player::Inventory_DropItem(int pEntity, int pSection, bool dropRandom)
{
	// Al parecer no tenemos este objeto en nuestro inventario.
	if ( !Inventory_HasItem(pEntity, pSection) )
		return false;

	// Obtenemos el objeto.
	const char *pItemName = Inventory_GetItemName(pEntity);

	// ¿Se ha eliminado/bugeado?
	if ( pItemName == "" )
		return false;

	CBaseEntity *pEnt = CreateEntityByName(pItemName);

	if ( !pEnt )
		return false;

	// Lo creamos en nuestra ubicación.
	Vector vecForward;
	AngleVectors(EyeAngles(), &vecForward);

	Vector vecOrigin;
	QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

	if ( dropRandom )
		vecOrigin = GetAbsOrigin() + vecForward * 5 + Vector(random->RandomInt(1, 20),random->RandomInt(1, 20),random->RandomInt(1, 20)); // Al azar
	else
		vecOrigin = GetAbsOrigin() + vecForward * 56 + Vector(0,0,64); // Justo delante de nosotros.

	pEnt->SetAbsOrigin(vecOrigin);
	pEnt->SetAbsAngles(vecAngles);

	DispatchSpawn(pEnt);
	pEnt->Activate();

	// Lo removemos del inventario.
	Inventory_RemoveItem(pEntity, pSection);
	return true;
}

//=========================================================
// Suelta el objeto que se encuentre en esta posición.
//=========================================================
bool CIN_Player::Inventory_DropItemByPos(int Position, int pSection, bool dropRandom)
{
	int pEntity = Inventory_GetItem(Position);
	return Inventory_DropItem(pEntity, pSection, dropRandom);
}

//=========================================================
// Suelta un objeto con este nombre.
//=========================================================
bool CIN_Player::Inventory_DropItemByName(const char *pName, int pSection, bool dropRandom)
{
	int pEntity = Inventory_GetItemID(pName);
	return Inventory_DropItem(pEntity, pSection, dropRandom);
}

//=========================================================
// Devuelve la cantidad de objetos en el inventario.
//=========================================================
int CIN_Player::Inventory_GetItemCount(int pSection)
{
	int pTotal = 0;

	if ( pSection == INVENTORY_POCKET || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.PocketItems.Count(); i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.PocketItems[i] == 0 || !m_HL2Local.PocketItems[i] )
				continue;

			pTotal++;
		}
	}

	if ( pSection == INVENTORY_BACKPACK || pSection == INVENTORY_ALL )
	{
		// Verificamos cada slot del inventario.
		for ( int i = 1; i < m_HL2Local.BackpackItems.Count(); i++ )
		{
			// Este slot no tiene ningún objeto.
			if ( m_HL2Local.BackpackItems[i] == 0 || !m_HL2Local.BackpackItems[i] )
				continue;

			pTotal++;
		}
	}

	return pTotal;
}

//=========================================================
// Obtiene la ID del objeto que se encuentra en esta posición.
//=========================================================
int CIN_Player::Inventory_GetItem(int Position, int pSection)
{
	if ( pSection == INVENTORY_POCKET )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.PocketItems[Position] == 0 || !m_HL2Local.PocketItems[Position] )
			return 0;

		return m_HL2Local.PocketItems.Get(Position);
	}

	if ( pSection == INVENTORY_BACKPACK )
	{
		// No hay ningún objeto en este slot.
		if ( m_HL2Local.BackpackItems[Position] == 0 || !m_HL2Local.BackpackItems[Position] )
			return 0;

		return m_HL2Local.BackpackItems.Get(Position);
	}

	return 0;
}

//=========================================================
// Obtiene el nombre clase de un objeto en el inventario.
//=========================================================
const char *CIN_Player::Inventory_GetItemName(int pEntity)
{
	// No hay ningún objeto con esta ID en la lista.
	if ( Items[pEntity] == "" || !Items[pEntity] )
		return "";

	return Items[pEntity];
}

//=========================================================
// Obtiene el nombre de un objeto de una posición en el inventario.
//=========================================================
const char *CIN_Player::Inventory_GetItemNameByPos(int Position, int pSection)
{
	int pEntity = Inventory_GetItem(Position, pSection);
	return Inventory_GetItemName(pEntity);
}

//=========================================================
// Usa un objeto del inventario.
// @TODO: Asquerosa esta implementación... Estaría mucho
// mejor tener el código de uso en el código
// del objeto (ejemplo: item_bandage.cpp). Con esto
// se repite el código 2 veces... -Iván
//=========================================================
void CIN_Player::Inventory_UseItem(int pEntity, int pSection)
{
	// Obtenemos el nombre clase del objeto.
	const char *pItemName	= Inventory_GetItemName(pEntity);
	bool pDelete			= true;

	if ( pItemName == "" )
		return;

	if ( !Inventory_HasItem(pEntity, pSection) )
	{
		Msg("[INVENTARIO] %s ha intentado usar %s. No esta en su inventario! \r\n", GetPlayerName(), pItemName);
		return;
	}

	// Benda
	// Objeto para dejar de sangrar.
	if ( Q_stristr(pItemName, "bandage") )
	{
		if ( ScarredBloodWound() )
		{
			CSingleUserRecipientFilter user(this);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
				WRITE_STRING(pItemName);
			MessageEnd();

			CPASAttenuationFilter filter(this, "Player.Bandage");
			EmitSound(filter, entindex(), "Player.Bandage");
		}
		else
			pDelete = false;
	}

	// Sangre
	// Todos necesitamos litros y litros de sangre para vivir.
	if ( Q_stristr(pItemName, "item_blood") )
	{
		ConVarRef sk_bloodkit("sk_bloodkit");

		if ( TakeBlood(sk_bloodkit.GetFloat()) != 0 )
		{
			CSingleUserRecipientFilter user(this);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
				WRITE_STRING(pItemName);
			MessageEnd();

			Inventory_AddItem("item_empty_bloodkit", pSection);
		}
		else
			pDelete = false;
	}

	// Bolsa de sangre vacia.
	if ( Q_stristr(pItemName, "empty_blood") )
		pDelete = false;

	// Bateria
	if ( Q_stristr(pItemName, "battery") )
		ApplyBattery();

	// Botiquin
	if ( Q_stristr(pItemName, "health") )
	{
		ConVarRef sk_healthkit("sk_healthkit");

		if ( TakeHealth(sk_healthkit.GetFloat(), DMG_GENERIC) )
		{
			CSingleUserRecipientFilter user(this);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
				WRITE_STRING(GetClassname());
			MessageEnd();

			CPASAttenuationFilter filter(this, "HealthKit.Touch");
			EmitSound(filter, entindex(), "HealthKit.Touch");
		}
		else
			pDelete = false;
	}


	// Munición: Pistola
	if ( Q_stristr(pItemName, "ammo_pistol") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_PISTOL, "Pistol") == 0 )
			pDelete = false;
	}

	// Munición: Pistola
	if ( Q_stristr(pItemName, "pistol_large") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_PISTOL_LARGE, "Pistol") == 0 )
			pDelete = false;
	}

	// Munición: SMG1
	if ( Q_stristr(pItemName, "ammo_smg1") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_SMG1, "SMG1") == 0 )
			pDelete = false;
	}

	// Munición: SMG1
	if ( Q_stristr(pItemName, "smg1_large") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_SMG1, "SMG1") == 0 )
			pDelete = false;
	}

	// Munición: AR2
	if ( Q_stristr(pItemName, "ammo_ar2") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_AR2, "AR2") == 0 )
			pDelete = false;
	}

	// Munición: AR2
	if ( Q_stristr(pItemName, "ar2_large") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_AR2_LARGE, "AR2") == 0 )
			pDelete = false;
	}

	// Munición: 357
	if ( Q_stristr(pItemName, "ammo_357") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_357, "357") == 0 )
			pDelete = false;
	}

	// Munición: 357
	if ( Q_stristr(pItemName, "ammo_357_large") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_357_LARGE, "357") == 0 )
			pDelete = false;
	}

	// Munición: Ballesta
	if ( Q_stristr(pItemName, "ammo_crossbow") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_CROSSBOW, "XBowBolt") == 0 )
			pDelete = false;
	}

	// Munición: Flareround
	if ( Q_stristr(pItemName, "flare_round") )
	{
		if ( InITEM_GiveAmmo(this, 1, "FlareRound") == 0 )
			pDelete = false;
	}

	// Munición: Flareround
	if ( Q_stristr(pItemName, "box_flare_rounds") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_BOX_FLARE_ROUNDS, "FlareRound") == 0 )
			pDelete = false;
	}

	// Munición: RPG
	if ( Q_stristr(pItemName, "rpg_round") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_RPG_ROUND, "RPG_Round") == 0 )
			pDelete = false;
	}

	// Munición: Granada AR2 y Granada SMG1
	if ( Q_stristr(pItemName, "ar2_grenade") ||  Q_stristr(pItemName, "smg1_grenade") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_SMG1_GRENADE, "SMG1_Grenade") == 0 )
			pDelete = false;
	}

	// Munición: Escopeta
	if ( Q_stristr(pItemName, "box_buckshot") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_BUCKSHOT, "Buckshot") == 0 )
			pDelete = false;
	}

	// Munición: AR2 Altfire
	if ( Q_stristr(pItemName, "ar2_altfire") )
	{
		if ( InITEM_GiveAmmo(this, SIZE_AMMO_AR2_ALTFIRE, "AR2AltFire") == 0 )
			pDelete = false;
	}

	// Todo salio correcto, eliminar del inventario.
	if ( pDelete )
	{
		Msg("[INVENTARIO] %s ha usado %s. \r\n", GetPlayerName(), pItemName);
		Inventory_RemoveItem(pEntity, pSection);
	}
}

//=========================================================
// Usa el objeto que este en esta posición.
//=========================================================
void CIN_Player::Inventory_UseItemByPos(int Position, int pSection)
{
	int pEntity = Inventory_GetItem(Position, pSection);
	Inventory_UseItem(pEntity, pSection);
}

//=========================================================
// Usa el objeto con este nombre.
//=========================================================
void CIN_Player::Inventory_UseItemByName(const char *pName, int pSection)
{
	int pEntity = Inventory_GetItemID(pName);
	Inventory_UseItem(pEntity, pSection);
}