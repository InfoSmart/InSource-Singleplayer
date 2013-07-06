//=====================================================================================//
//
// Sistema encargado de constrolar al personaje.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "in_player.h"

#include "director.h"
#include "in_buttons.h"

#include "ammodef.h"
#include "items.h"
#include "item_loot.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "bone_setup.h"

#include "basehlcombatweapon_shared.h"
#include "ilagcompensationmanager.h"

#include "steam/steam_api.h"

#include "tier0/memdbgon.h"

#define DMG_BLOODLOST  (1<<30)

//=========================================================
// Definición de comandos de consola.
//=========================================================

// Modelo
ConVar cl_sp_playermodel("cl_sp_playermodel", "models/abigail.mdl", FCVAR_ARCHIVE, "Define el modelo del jugador en Singleplayer");

ConVar cl_playermodel_color_r("cl_playermodel_color_r", "255", FCVAR_ARCHIVE, "Define el color del modelo del jugador.");
ConVar cl_playermodel_color_g("cl_playermodel_color_g", "255", FCVAR_ARCHIVE, "Define el color del modelo del jugador.");
ConVar cl_playermodel_color_b("cl_playermodel_color_b", "255", FCVAR_ARCHIVE, "Define el color del modelo del jugador.");

// Camara lenta.
ConVar in_timescale_effect("in_timescale_effect", "1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Activa o desactiva el efecto de camara lenta al perder salud.");

// Efecto de cansancio.
ConVar in_tired_effect("in_tired_effect", "1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Activa o desactiva el efecto de cansancio al perder salud.");

// Linterna
ConVar in_flashlight				("in_flashlight",				"1", FCVAR_NOTIFY | FCVAR_REPLICATED,	"Activa o desactiva el uso de la linterna.");
ConVar in_flashlight_require_suit	("in_flashlight_require_suit",	"0", FCVAR_ARCHIVE,						"Activa o desactiva el requerimiento del traje de protección para la linterna.");

// Regeneración de salud.
ConVar in_regeneration("in_regeneration",						"1",	FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_ARCHIVE,	"Estado de la regeneracion de salud");
ConVar in_regeneration_wait_time("in_regeneration_wait_time",	"10",	FCVAR_NOTIFY | FCVAR_REPLICATED,	"Tiempo de espera en segundos al regenerar salud. (Aumenta según el nivel de dificultad)");
ConVar in_regeneration_rate("in_regeneration_rate",				"3",	FCVAR_NOTIFY | FCVAR_REPLICATED,	"Cantidad de salud a regenerar (Disminuye según el nivel de dificultad)");

//=========================================================
// SERVIDOR: Definición de comandos de consola.
//=========================================================

// Survival: Nivel de Sangre
ConVar sv_in_blood("sv_in_blood",	"5600",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de sangre.");

// Survival: Nivel de Hambre
ConVar sv_in_hunger("sv_in_hunger",				"300",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de hambre.");
ConVar sv_in_hunger_rate("sv_in_hunger_rate",	"1",		FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo en minutos para disminuir la cantidad de hambre.");

// Survival: Nivel de sed
ConVar sv_in_thirst("sv_in_thirst",				"100",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Cantidad de sed.");
ConVar sv_in_thirst_rate("sv_in_thirst_rate",	 "1",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Tiempo en minutos para disminuir la cantidad de sed.");

// Survival: Inventario
ConVar sv_max_inventory_backpack("sv_max_inventory_backpack",	"10",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Número de objetos máximo en la mochila.");
ConVar sv_max_inventory_pocket("sv_max_inventory_pocket",		"5",	FCVAR_NOTIFY | FCVAR_REPLICATED, "Número de objetos máximo en el bolsillo.");

// Linterna
ConVar sv_flashlight_weapon("sv_flashlight_weapon", "1", FCVAR_REPLICATED, "Acoplar las linternas en la boca de las armas?");
ConVar sv_flashlight_realistic("sv_flashlight_realistic", "1", FCVAR_REPLICATED, "¿Usar linternas reales?");

//ConVar sv_realistic_weapons("sv_realistic_weapons", "1", FCVAR_REPLICATED, "Activa o desactiva el modo de disparo realistico (y dificil) de las armas.");

//=========================================================
// Configuración
//=========================================================

#define HEALTH_REGENERATION_MEDIUM_WAITTIME 2		// Dificultad Medio: Tardara 2 segundos más.
#define HEALTH_REGENERATION_MEDIUM_RATE		1		// Dificultad Medio: 1% salud menos.

#define HEALTH_REGENERATION_HARD_WAITTIME	5		// Dificultad Dificil: Tardara 5 segundos más.
#define HEALTH_REGENERATION_HARD_RATE		2		// Dificultad Dificil: 2% salud menos.

//=========================================================
// Guardado y definición de datos
//=========================================================
LINK_ENTITY_TO_CLASS(in_ragdoll, CINRagdoll);

IMPLEMENT_SERVERCLASS_ST_NOBASE(CINRagdoll, DT_INRagdoll)
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CINRagdoll )

	DEFINE_FIELD( m_vecRagdollOrigin,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hPlayer,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecRagdollVelocity, FIELD_VECTOR ),

END_DATADESC()

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEPlayerAnimEvent, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent(const char *name) : CBaseTempEntity(name)
	{
	}

	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )

	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 ),

END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent("PlayerAnimEvent");

void TE_PlayerAnimEvent(CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData)
{
	CPVSFilter filter((const Vector&)pPlayer->EyePosition());

	g_TEPlayerAnimEvent.m_hPlayer	= pPlayer;
	g_TEPlayerAnimEvent.m_iEvent	= event;
	g_TEPlayerAnimEvent.m_nData		= nData;
	g_TEPlayerAnimEvent.Create(filter, 0);
}

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef APOCALYPSE
	LINK_ENTITY_TO_CLASS( player, CIN_Player );
	PRECACHE_REGISTER(player);
#endif

BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_INLocalPlayerExclusive )
	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_INNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CIN_Player, DT_IN_Player)
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropDataTable( "inlocaldata", 0, &REFERENCE_SEND_TABLE(DT_INLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "innonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_INNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropEHandle( SENDINFO(m_hRagdoll) ),
	SendPropBool( SENDINFO(m_fIsWalking) ),
END_SEND_TABLE()

BEGIN_DATADESC( CIN_Player )
	DEFINE_SOUNDPATCH ( pDyingSound ),

	DEFINE_FIELD( m_fNextPainSound,				FIELD_FLOAT ),
	DEFINE_FIELD( m_fNextHealthRegeneration,	FIELD_FLOAT ),
	DEFINE_FIELD( m_fBodyHurt,					FIELD_FLOAT ),
	DEFINE_FIELD( m_iStressLevel,				FIELD_INTEGER ),

	DEFINE_FIELD( m_bBloodWound,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBloodTime,					FIELD_INTEGER ),
	DEFINE_FIELD( m_iBloodSpawn,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iBlood,						FIELD_FLOAT ),

	DEFINE_FIELD( m_iHungerTime,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iHunger,					FIELD_FLOAT ),

	DEFINE_FIELD( m_iThirstTime,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iThirst,					FIELD_FLOAT ),

#ifdef APOCALYPSE
	DEFINE_FIELD( m_bGruntAttack,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fRecoverGruntAttack,		FIELD_TIME ),
#endif

END_DATADESC()

//=========================================================
// Modelos
//=========================================================

const char *PlayerModels[] =
{
	// Todas estas conversiones son gracias a GarrysMOD
	"models/player/alyx.mdl",
	"models/player/barney.mdl",
	"models/player/breen.mdl",
	"models/player/charple.mdl",
	"models/player/combine_soldier.mdl",
	"models/player/combine_super_soldier.mdl",
	"models/player/corpse1.mdl",
	"models/player/eli.mdl",
	"models/player/gman_high.mdl",
	"models/player/kleiner.mdl",
	"models/player/magnusson.mdl",
	"models/player/monk.mdl",
	"models/player/mossman.mdl",
	"models/player/odessa.mdl",
	"models/player/police.mdl",
	"models/player/police_fem.mdl"
};

#pragma warning( disable : 4355 )

//=========================================================
// Constructor
//=========================================================
CIN_Player::CIN_Player()
{
	m_fNextPainSound			= gpGlobals->curtime;	// Sonido de dolor.
	m_fNextHealthRegeneration	= gpGlobals->curtime;	// Regeneración de salud.
	m_fBodyHurt					= 0.0f;					// Efecto de muerte.
	m_iStressLevel				= 0;					// Estres del jugador.
	pDyingSound					= NULL;					// Sonido "Estamos muriendo"

#ifdef APOCALYPSE
	m_bGruntAttack				= false;				// ¿Hemos sufrido un ataque del Grunt?
	m_fRecoverGruntAttack		= 0;
#endif

	m_PlayerAnimState = CreatePlayerAnimationState(this);	// Creamos la instancia para la animación y el movimiento.

	UseClientSideAnimation();	// Usar animaciones del lado del cliente.
	//CreateExpresser();			// Creamos la instancia encargada de las respuestas y expresiones.

	m_angEyeAngles.Init();
	m_pPlayerModels.Purge();

	// Solo en modo Supervivencia.
	if ( InGameRules()->IsSurvival() )
	{
		m_bBloodWound		= false;				// No tenemos una herida de sangre.
		m_iBloodSpawn		= gpGlobals->curtime;	// Última vez que derramamos sangre.

		// Transmitimos esta información al cliente...
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
	// Eliminamos la instancia de animaciones.
	if ( m_PlayerAnimState )
		m_PlayerAnimState->Release();
}

//=========================================================
// Forma más fácil y corta de acceder a un comando del
// lado del cliente/servidor.
//=========================================================
const char *CIN_Player::GetConVar(const char *pConVar)
{
	// Comando de cliente.
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
	engine->ClientCommand(edict(), pCommand);
}

//=========================================================
// Actualizaciones al remover al jugador.
//=========================================================
void CIN_Player::UpdateOnRemove()
{
	// Eliminamos su cadaver.
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate(m_hRagdoll);
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
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
	if ( InGameRules()->IsMultiplayer() )
	{
		// 4 grupos de humanos.
		// @TODO: Algo más flexible.
		for ( int g = 1; g <= 4; ++g )
		{
			int fm = 6;		// Mujeres
			int mm = 9;		// Hombres

			// Grupo 2
			if ( g == 2 )
			{
				fm = 0;
				mm = 8;
			}

			// Guardamos a las mujeres.
			if ( fm > 0 )
			{
				for ( int f = 1; f <= fm; ++f )
				{
					char path[100];

					// Grupo 4 (Grupo 3 medico)
					if ( g == 4 )
						Q_snprintf(path, sizeof(path), "models/player/group0%im/female_0%i.mdl", g, f);
					else
						Q_snprintf(path, sizeof(path), "models/player/group0%i/female_0%i.mdl", g, f);

					PrecacheModel(path);
					DevMsg("[CACHE] <Mujer> Guardando modelo: %s \r\n", path);

					// Lo guardamos en la  lista de modelos válidos.
					m_pPlayerModels.AddToTail(path);
				}
			}

			// Guardamos a los hombres.
			if ( mm > 0 )
			{
				for ( int m = 1; m <= mm; ++m )
				{
					// Grupo 2
					if ( g == 2 )
					{
						// Estos modelos no existen.
						if ( m == 1 || m == 3 || m == 5 || m == 7 )
							continue;
					}

					char path[100];

					if ( g == 4 )
						Q_snprintf(path, sizeof(path), "models/player/group0%im/male_0%i.mdl", g, m);
					else
						Q_snprintf(path, sizeof(path), "models/player/group0%i/male_0%i.mdl", g, m);

					PrecacheModel(path);
					DevMsg("[CACHE] <Hombre> Guardando modelo: %s \r\n", path);

					// Lo guardamos en la  lista de modelos válidos.
					m_pPlayerModels.AddToTail(path);
				}
			}
		}

		// Los demás modelos conocidos.
		for ( int i = 0; i <= ARRAYSIZE(PlayerModels)-1; ++i )
		{
			PrecacheModel(PlayerModels[i]);
			DevMsg("[CACHE] Guardando modelo: %s \r\n", PlayerModels[i]);

			// Lo guardamos en la  lista de modelos válidos.
			m_pPlayerModels.AddToTail(PlayerModels[i]);
		}
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
	BaseClass::Spawn();

	// En Multiplayer no podemos cambiar el tiempo solo por salud.
	if ( !InGameRules()->IsMultiplayer() )
	{
		ConVarRef host_timescale("host_timescale");

		// Resetear el tiempo.
		if ( host_timescale.GetInt() != 1 )
			ExecCommand("host_timescale 1");

		// Establecemos el modelo para SinglePlayer
		SetModel(cl_sp_playermodel.GetString());
	}
	else
	{
		// Obtenemos el modelo del jugador.
		const char *pPlayerModel = GetConVar("cl_playermodel");

		// Este modelo no es válido, seleccionamos un modelo válido al azar.
		//if ( !IsModelValid(pPlayerModel) )
			//pPlayerModel = GetRandomModel();

		// Establecemos el modelo del jugador.
		SetModel(pPlayerModel);

		// Estaba en modo espectador.
		if ( !IsObserver() )
		{
			// Reiniciamos todo y volvamos a la vida.
			pl.deadflag = false;
			RemoveSolidFlags(FSOLID_NOT_SOLID);
			RemoveEffects(EF_NODRAW);
			StopObserverMode();
		}

		RemoveEffects(EF_NOINTERP);
		m_nRenderFX			= kRenderNormal;
		m_Local.m_iHideHUD	= 0;

		// Estamos en el suelo y ya no estamos congelados.
		AddFlag(FL_ONGROUND);
		RemoveFlag(FL_FROZEN);

		m_Local.m_bDucked = false;
		SetPlayerUnderwater(false);

		// SPAWN!
		DoAnimationEvent(PLAYERANIMEVENT_SPAWN);

		// Reseteamos los recursos.
		if ( InGameRules()->IsSurvival() )
		{
			m_bBloodWound			= false;
			m_HL2Local.m_iBlood		= m_iBlood = sv_in_blood.GetFloat();
			m_HL2Local.m_iHunger	= m_iHunger = sv_in_hunger.GetFloat();
			m_HL2Local.m_iThirst	= m_iThirst = sv_in_thirst.GetFloat();
		}
	}
}

//=========================================================
// POST Constructor
//=========================================================
void CIN_Player::PostConstructor(const char *szClassname)
{
	BaseClass::PostConstructor(szClassname);
}

//=========================================================
// Selecciona un info_player_start donde aparecer.
//=========================================================
CBaseEntity* CIN_Player::EntSelectSpawnPoint()
{
	// El siguiente código solo es válido en Survival, si estamos en Singleplayer
	// usar el código original.
	if ( !InGameRules()->IsSurvival() )
		return BaseClass::EntSelectSpawnPoint();

	CBaseEntity *pSpot				= NULL;
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
			 if ( InGameRules()->IsSpawnPointValid(pSpot, this) )
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

	m_LastSpawn = pSpot;
	return pSpot;
}

//=========================================================
// Evento: ¡Nos han matado1
//=========================================================
void CIN_Player::Event_Killed(const CTakeDamageInfo &info)
{
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce(m_vecTotalBulletForce);


	// En Multiplayer
	/*if ( InGameRules()->IsMultiplayer() )
	{
		// Dirijimos la camara al jugador/npc que nos ha matado.
		if ( info.GetAttacker() )
			m_hObserverTarget.Set(info.GetAttacker());
		else
			m_hObserverTarget.Set(NULL);
	}*/

#ifdef APOCALYPSE
	EnableControl(true);
	EmitMusic("Player.Music.Dead", this); // Que descanses en paz, mi abigaíl...
#endif

	// Este juego incluye al director
	// y no es Multiplayer.
	/*if ( InGameRules()->IncludeDirector() && !GameRules()->IsMultiplayer() )
	{
		CDirector *pDirector = GetDirector();

		if ( pDirector  )
		{
			//engine->FadeClientVolume(edict(), 90, 2, 10, 2);
			pDirector->StopMusic();
			pDirector->MuteOtherSounds = true;
		}
	}*/

	// Creamos nuestro cadaver.
	CreateRagdollEntity(info);

	BaseClass::Event_Killed(info);

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}

	// Tenemos a alguien en la camara de espectador.
	if ( GetObserverTarget() )
	{
		StartReplayMode(3, 3, GetObserverTarget()->entindex());
		StartObserverMode(OBS_MODE_DEATHCAM);
	}

	FlashlightTurnOff();
	m_lifeState = LIFE_DEAD;

	RemoveEffects(EF_NODRAW);
	StopZooming();
}

//=========================================================
// Evento: Estas muerto, no es una gran sorpresa...
//=========================================================
void CIN_Player::Event_Dying()
{
	BaseClass::Event_Dying();
}

//=========================================================
// Compensación de Lagg
//=========================================================
bool CIN_Player::WantsLagCompensationOnEntity(const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const
{
	return BaseClass::WantsLagCompensationOnEntity(pEntity, pCmd, pEntityTransmitBits);
}

//=========================================================
// Pensamiento: Bucle de ejecución de tareas.
//=========================================================
void CIN_Player::PreThink()
{
	QAngle vOldAngles	= GetLocalAngles();
	QAngle vTempAngles	= GetLocalAngles();
	vTempAngles			= EyeAngles();

	if ( vTempAngles[PITCH] > 180.0f )
		vTempAngles[PITCH] -= 360.0f;

	SetLocalAngles(vTempAngles);

	BaseClass::PreThink();

	SetLocalAngles(vOldAngles);
	HandleSpeedChanges();

	// @DEBUG: Muestra el número de entidades en el HUD.
	m_HL2Local.m_iEntities = engine->GetEntityCount();

#ifdef APOCALYPSE
	// Hemos sufrido un ataque de Grunt, tenemos un arma y la misma esta invisible.
	if ( m_bGruntAttack && GetActiveWeapon() && !GetActiveWeapon()->IsWeaponVisible() )
	{
		// Ya estamos en el suelo
		if ( (GetFlags() & FL_ONGROUND) )
		{
			// Aquí esperamos 3 segundos después de estar en el suelo para reactivar el arma.
			// @TODO: Algo mejor sería hacer la animación de "reactivar arma y levantarse"
			if ( m_fRecoverGruntAttack == 0 )
				m_fRecoverGruntAttack = gpGlobals->curtime + 2;

			// Ya activamos nuestra arma, hacerla visible de nuevo.
			if ( m_fRecoverGruntAttack <= gpGlobals->curtime )
			{
				EnableControl(true);
				GetActiveWeapon()->Deploy();

				m_bGruntAttack			= false;
				m_fRecoverGruntAttack	= 0;
			}
		}
		else
			EnableControl(false);
	}
#endif

	// Esto solo funciona en modo Survival.
	if ( InGameRules()->IsSurvival() )
	{
		BloodThink();
		HungerThink();
		ThirstThink();
	}
}

//=========================================================
// Bucle de ejecución de tareas. "Post"
//=========================================================
void CIN_Player::PostThink()
{
	m_angEyeAngles = EyeAngles();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles(angles);

	// Enviamos la ubicación de los ojos.
	m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);

	// Si seguimos vivos.
	if ( IsAlive() )
	{
		/*
			Regeneración de salud
		*/
		if ( AutoHealthRegeneration() )
			HealthRegeneration();

		/*
			Efectos - Cansancio y muerte.
		*/
		TiredThink();

		/*
			Efectos - Camara lenta.
		*/
		TimescaleThink();
	}

	BaseClass::PostThink();

	if ( GetFlags() & FL_DUCKING )
		SetCollisionBounds(VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX);
}

//=========================================================
// Bucle de ejecución de tareas al morir.
//=========================================================
void CIN_Player::PlayerDeathThink()
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	/*
		Efectos - Camara lenta.
	*/
	if ( in_timescale_effect.GetBool() && !InGameRules()->IsMultiplayer() )
	{
		ConVarRef host_timescale("host_timescale");

		if ( host_timescale.GetFloat() != 0.5 )
			ExecCommand("host_timescale 0.5");
	}

	// Mientras no seamos espectador.
	if ( !IsObserver() )
	{
		// Perdida de luz.
		color32 black = {0, 0, 0, 210};
		UTIL_ScreenFade(this, black, 1.0f, 0.1f, FFADE_IN | FFADE_STAYOUT);
	}		

	BaseClass::PlayerDeathThink();
}

//=========================================================
// Pensamiento: Efectos de cansancio.
//=========================================================
void CIN_Player::TiredThink()
{
	if ( !in_tired_effect.GetBool() )
		return;

	// No estamos en Multiplayer.
	if ( !InGameRules()->IsMultiplayer() )
	{
		ConVarRef mat_yuv("mat_yuv");

		// Salud menor a 10% y escala de grises desactivado.
		if ( m_iHealth < 10 && !mat_yuv.GetBool() )
		{
			// Activamos escala de grises.
			ExecCommand("mat_yuv 1");

			// ¡Estamos muriendo!
			if ( !InGameRules()->IsMultiplayer() && !PlayingDyingSound() )
				pDyingSound = EmitMusic("Player.Music.Puddle", this);
		}

		// Desactivar escala de grises y parar el sonido de "estamos muriendo".
		// Si, me estoy reponiendo.
		if ( m_iHealth >= 10 && mat_yuv.GetBool() )
		{
			ExecCommand("mat_yuv 0");

			if ( !InGameRules()->IsMultiplayer() && PlayingDyingSound() )
				FadeoutMusic(pDyingSound, 1.5f, true);
		}
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
	if ( m_iHealth < 15 )
		UTIL_ScreenShakePlayer(this, 1.0, 1.0, 1.0, SHAKE_START, true);

	// Efectos - Dolor del cuerpo.
	if ( m_iHealth < 60 && gpGlobals->curtime > m_fBodyHurt )
	{
		int AngHealth = (100 - m_iHealth) / 8;
		ViewPunch(QAngle(random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth)));

		// Proximo efecto dentro de 5 a 20 segundos.
		m_fBodyHurt = gpGlobals->curtime + (random->RandomInt(5, 20));
	}
}

//=========================================================
// Pensamiento: Efectos de escala de tiempo.
//=========================================================
void CIN_Player::TimescaleThink()
{
	if ( !in_timescale_effect.GetBool() || InGameRules()->IsMultiplayer() )
		return;

	ConVarRef host_timescale("host_timescale");

	// Salud menor a 10%
	// Escala de tiempo: 0.6
	if ( m_iHealth < 10 && host_timescale.GetFloat() != 0.7 )
		ExecCommand("host_timescale 0.7");

	// Salud menor a 15%
	// Escala de tiempo: 0.8
	else if ( m_iHealth < 15 && host_timescale.GetFloat() != 0.9 )
		ExecCommand("host_timescale 0.9");

	// Salud mayor a 15%
	// Escala de tiempo: 1 (Real)
	else if ( m_iHealth > 15 && host_timescale.GetFloat() != 1 )
		ExecCommand("host_timescale 1");
}

//=========================================================
// Bucle de ejecución de tareas para la sangre del jugador.
//=========================================================
void CIN_Player::BloodThink()
{
	// La sangre solo existe en el modo supervivencia.
	if ( !InGameRules()->IsSurvival() )
		return;

	// Tenemos una "herida de sangre"
	if ( m_bBloodWound && m_iBlood > 0 )
	{
		m_iBlood = m_iBlood - random->RandomFloat(0.1, 1.0);

		if ( m_iBloodSpawn < (gpGlobals->curtime - 3) )
		{
			Vector vecSpot = WorldSpaceCenter();
			vecSpot.x += random->RandomFloat( -5, 5 );
			vecSpot.y += random->RandomFloat( -5, 5 );

			// Lanzar sangre.
			// FIXME: La sangre no se ve desde la vista del jugador pero si desde los otros.
			SpawnBlood(vecSpot, vec3_origin, BLOOD_COLOR_RED, 1);
			m_iBloodSpawn = gpGlobals->curtime;
		}

		if ( m_iBloodTime < (gpGlobals->curtime - 60) )
		{
			int pRand = random->RandomInt(1, 900);

			// ¡Ha cicatrizado! Suertudo.
			if ( pRand == 843 )
				m_bBloodWound = false;
		}
	}
	else
	{
		if ( m_iBlood < sv_in_blood.GetFloat() )
		{
			if ( random->RandomInt(1, 100) == 2 )
				m_iBlood = m_iBlood + 0.1;
		}
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

//=========================================================
// Bucle de ejecución de tareas para el hambre.
//=========================================================
void CIN_Player::HungerThink()
{
	// El hambre solo existe en el modo supervivencia.
	if ( !InGameRules()->IsSurvival() )
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

//=========================================================
// Bucle de ejecución de tareas para la sed.
//=========================================================
void CIN_Player::ThirstThink()
{
	// La sed solo existe en el modo supervivencia.
	if ( !InGameRules()->IsSurvival() )
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
// Controla los cambios de velocidad.
//=========================================================
void CIN_Player::HandleSpeedChanges()
{
	int buttonsChanged	= m_afButtonPressed | m_afButtonReleased;
	bool bCanSprint		= CanSprint();
	bool bIsSprinting	= IsSprinting();
	bool bWantSprint	= ( bCanSprint && IsSuitEquipped() && (m_nButtons & IN_SPEED) );

	// Al parecer el jugador desea correr.
	if ( bIsSprinting != bWantSprint && (buttonsChanged & IN_SPEED) )
	{
		// En esta sección se verifica que el jugador realmente esta presionando el boton indicado para correr.
		// Ten en cuenta que el comando "sv_stickysprint" sirve para activar el modo de "correr sin mantener presionado el boton"
		// Por lo tanto tambien hay que verificar que el usuario disminuya su velocidad para detectar que desea desactivar este modo.

		if ( bWantSprint )
			StartSprinting();
		
		else
		{
			StopSprinting();

			// Quitar el estado de "presionado" a la tecla de correr.
			m_nButtons &= ~IN_SPEED;
		}
	}

	bool bIsWalking = IsWalking();
	bool bWantWalking;

	// Tenemos el traje de protección y no estamos ni corriendo ni agachados.
	if ( IsSuitEquipped() )
		bWantWalking = ( m_nButtons & IN_WALK ) && !IsSprinting() && !( m_nButtons & IN_DUCK );
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
	SetMaxSpeed(CalculateSpeed());
}

//=========================================================
// Paramos de correr
//=========================================================
void CIN_Player::StopSprinting()
{
	ConVarRef hl2_walkspeed("hl2_walkspeed"); // Velocidad con traje de protección.
	ConVarRef hl2_normspeed("hl2_normspeed"); // Velocidad sin traje de protección.

	BaseClass::StopSprinting();

	// Ajustar la velocidad dependiendo si tenemos el traje o no.
	if ( IsSuitEquipped() )
		SetMaxSpeed(CalculateSpeed(NULL, hl2_walkspeed.GetFloat()));
	else
		SetMaxSpeed(hl2_normspeed.GetFloat());
}

//=========================================================
// Empezar a caminar
//=========================================================
void CIN_Player::StartWalking()
{
	SetMaxSpeed(CalculateSpeed());
	BaseClass::StartWalking();
}

//=========================================================
// Paramos de caminar
//=========================================================
void CIN_Player::StopWalking()
{
	SetMaxSpeed(CalculateSpeed());
	BaseClass::StopWalking();
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ARMAS
//=========================================================
//=========================================================

int CIN_Player::Accuracy()
{
	CBaseViewModel *pVm		= GetViewModel();
	const char *crosshair	= GetConVar("crosshair");

	// !!!FIXME: No puedo detectar el IronSight del lado del servidor.
	//DevMsg("Iron: %i \r\n", pVm->m_bExpSighted);

	// Esta agachado y con la mira puesta. Las balas se esparcirán al minimo.
	if ( IsDucked() && crosshair == "0" )
		return ACCURACY_HIGH;

	// Esta con la mira.
	else if ( crosshair == "0" )
		return ACCURACY_MEDIUM;

	// Esta agachado.
	else if ( IsDucked() )
		return ACCURACY_LOW;

	return ACCURACY_NONE;
}

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
// Verifica si es posible cambiar a determinada arma.
//=========================================================
bool CIN_Player::Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)
{
	// Cuando el arma cambia también hay que actualizar la velocidad del jugador con el peso de la misma.
	// Puedes cambiar el peso en los scripts de las armas, variable: "slow_speed"
	// @TODO: Cambiar a un lugar más apropiado

	if ( IsSprinting() )
		SetMaxSpeed(CalculateSpeed(pWeapon));

	// Debido a que StartWalking() jamas es llamado (ver la función HandleSpeedChanges())
	// una solución temporal es verificar que no estemos corriendo...
	if ( !IsSprinting() )
		SetMaxSpeed(CalculateSpeed(pWeapon));

	bool Result = BaseClass::Weapon_CanSwitchTo(pWeapon);

	if ( Result )
	{
		if ( pWeapon->GetWpnData().m_expOffset.x == 0 && pWeapon->GetWpnData().m_expOffset.y == 0 )
			ExecCommand("crosshair 1");
		else
		{
			// @TODO: Hacer funcionar
			CBaseViewModel *pVm = GetViewModel();

			if ( pVm->m_bExpSighted )
				ExecCommand("crosshair 0");
		}
	}

	return Result;
}

//=========================================================
// Disparar
//=========================================================
void CIN_Player::FireBullets(const FireBulletsInfo_t &info)
{
	BaseClass::FireBullets(info);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL DAÑO/SALUD
//=========================================================
//=========================================================

//=========================================================
// Regeneración de salud.
//=========================================================
void CIN_Player::HealthRegeneration()
{
	// Tiene toda la salud o la regeneración esta desactivada.
	if ( m_iHealth >= GetMaxHealth() || !in_regeneration.GetBool() )
		return;
	
	// Aún no toca regenerar.
	if ( gpGlobals->curtime <= m_fNextHealthRegeneration )
		return;

	int RegenerationHealth = in_regeneration_rate.GetInt();
	float RegenerationWait = in_regeneration_wait_time.GetFloat();

	// Dependiendo del nivel de dificultad daremos una cantidad de salud.
	switch ( InGameRules()->GetSkillLevel() )
	{
		// Normal
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
	// @TODO: Esto sería una buena característica... quizá en un futuro, por ahora NO.
	if ( RegenerationHealth < 1 )
		RegenerationHealth = in_regeneration_rate.GetInt();

	TakeHealth(RegenerationHealth, DMG_GENERIC);
	m_fNextHealthRegeneration = gpGlobals->curtime + RegenerationWait;
}

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

	if ( InGameRules()->Damage_IsTimeBased(info.GetDamageType()) && flHealthPrev < 75 )
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
	if ( gpGlobals->curtime >= m_fNextPainSound && info.GetDamageType() != DMG_BLOODLOST )
	{
		if ( Gender() == PLAYER_FEMALE )
			EmitSound("Player.Pain.Female");
		else
			EmitSound("Player.Pain.Male");

		m_fNextPainSound = gpGlobals->curtime + random->RandomFloat(0.5, 3.0);
	}

	// Solo en Survival.
	if ( InGameRules()->IsSurvival() )
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

			m_iBlood = m_iBlood - (m_lastDamageAmount * random->RandomInt(1, 6));
		}
	}

	// Efectos
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade(this, white, 0.2, 0, FFADE_IN);

	SetMaxSpeed(CalculateSpeed());
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

	// Así de sencillo...
	m_bBloodWound = false;

	// Efectos
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade(this, white, 0.8, 0, FFADE_IN);

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

	m_HL2Local.m_iHunger = m_iHunger;
	return m_iHunger - oldHunger;
}

//=========================================================
// Añade una cantidad de sed al jugador.
//=========================================================
int CIN_Player::TakeThirst(float flThirst)
{
	int iMax = sv_in_thirst.GetInt();

	if ( m_iThirst >= iMax )
		return 0;

	const int oldThirst = m_iThirst;
	m_iThirst += flThirst;

	if ( m_iThirst > iMax )
		m_iThirst = iMax;

	m_HL2Local.m_iThirst = m_iThirst;
	return m_iThirst - oldThirst;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LAS ANIMACIONES Y MODELO
//=========================================================
//=========================================================

int CIN_Player::Restore( IRestore &restore )
{
	int status = BaseClass::Restore(restore);

	if ( GetFlags() & FL_DUCKING ) 
	{
		UTIL_SetSize(this, InGameRules()->GetViewVectors()->m_vDuckHullMin, InGameRules()->GetViewVectors()->m_vDuckHullMax);
	}
	else
	{
		UTIL_SetSize(this, InGameRules()->GetViewVectors()->m_vHullMin, InGameRules()->GetViewVectors()->m_vHullMax);
	}

	return status;
}

//=========================================================
// Realiza un evento de animación.
//=========================================================
void CIN_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	m_PlayerAnimState->DoAnimationEvent(event, nData);
	TE_PlayerAnimEvent(this, event, nData);
}

//=========================================================
// Establece la animación actual.
//=========================================================
void CIN_Player::SetAnimation(PLAYER_ANIM playerAnim)
{
	// Las animaciones son procesadas y calculadas por "in_playeranimstate"
	// El siguiente código solo sirve para "redireccionar" las animaciones de
	// disparo y recarga.

	PlayerAnimEvent_t pAnim = PLAYERANIMEVENT_CANCEL;

	switch ( playerAnim )
	{
		// Disparo
		case PLAYER_ATTACK1:
			pAnim = PLAYERANIMEVENT_ATTACK_PRIMARY;
		break;

		// Recarga
		case PLAYER_RELOAD:
			pAnim = PLAYERANIMEVENT_RELOAD;
		break;
	}

	if ( pAnim != PLAYERANIMEVENT_CANCEL )
		DoAnimationEvent(pAnim);
}

//=========================================================
// Transformarse en cadaver en el lado del cliente.
//=========================================================
bool CIN_Player::BecomeRagdollOnClient(const Vector &force)
{
	return true;
}

//=========================================================
// Crea al cadaver del jugador.
//=========================================================
void CIN_Player::CreateRagdollEntity(const CTakeDamageInfo &info)
{
	// Ya hay un cadaver, removerlo.
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate(m_hRagdoll);
		m_hRagdoll = NULL;
	}

	// Obtenemos el cadaver.
	CINRagdoll *pRagdoll = dynamic_cast<CINRagdoll*>(m_hRagdoll.Get());

	// Al parecer no hay ninguno, crearlo.
	if ( !pRagdoll )
		pRagdoll = dynamic_cast<CINRagdoll*>(CreateEntityByName("in_ragdoll"));

	// Se ha creado el cadaver.
	if ( pRagdoll )
	{
		// Heredamos toda la información posible.

		pRagdoll->m_hPlayer				= this;
		pRagdoll->m_vecRagdollOrigin		= GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity	= GetAbsVelocity();
		pRagdoll->m_nModelIndex			= m_nModelIndex;
		pRagdoll->m_nForceBone			= m_nForceBone;

		pRagdoll->m_vecForce				= m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );

		/*pRagdoll->CopyAnimationDataFrom(this);
		pRagdoll->SetOwnerEntity(this);

		pRagdoll->m_flAnimTime		= gpGlobals->curtime;
		pRagdoll->m_flPlaybackRate	= 0.0;
		pRagdoll->SetCycle(0);
		pRagdoll->ResetSequence(0);

		float fSequenceDuration = SequenceDuration( GetSequence() );
		float fPreviousCycle	= clamp(GetCycle()-( 0.1 * ( 1 / fSequenceDuration ) ),0.f,1.f);
		float fCurCycle			= GetCycle();

		matrix3x4_t pBoneToWorld[MAXSTUDIOBONES], pBoneToWorldNext[MAXSTUDIOBONES];
		SetupBones(pBoneToWorldNext, BONE_USED_BY_ANYTHING);
		SetCycle(fPreviousCycle);
		SetupBones(pBoneToWorld, BONE_USED_BY_ANYTHING);
		SetCycle(fCurCycle);

		pRagdoll->SetMoveType(MOVETYPE_VPHYSICS);
		pRagdoll->SetSolid(SOLID_VPHYSICS);

		if ( IsDissolving() )
			pRagdoll->TransferDissolveFrom(this);

		Vector mins, maxs;
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
		pRagdoll->CollisionProp()->SetCollisionBounds( mins, maxs );
		pRagdoll->SetCollisionGroup(COLLISION_GROUP_INTERACTIVE_DEBRIS);*/
	}

	m_hRagdoll	= pRagdoll;
}

//=========================================================
// Establece los huesos.
//=========================================================
void CIN_Player::SetupBones(matrix3x4_t *pBoneToWorld, int boneMask)
{
	VPROF_BUDGET("CIN_Player::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM);

	// Set the mdl cache semaphore.
	MDLCACHE_CRITICAL_SECTION();

	// Get the studio header.
	Assert(GetModelPtr());
	CStudioHdr *pStudioHdr = GetModelPtr();

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// Adjust hit boxes based on IK driven offset.
	Vector adjOrigin = GetAbsOrigin() + Vector( 0, 0, m_flEstIkOffset );

	// FIXME: pass this into Studio_BuildMatrices to skip transforms
	CBoneBitList boneComputed;
	if ( m_pIk )
	{
		m_iIKCounter++;
		m_pIk->Init( pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask );
		GetSkeleton( pStudioHdr, pos, q, boneMask );

		m_pIk->UpdateTargets( pos, q, pBoneToWorld, boneComputed );
		CalculateIKLocks( gpGlobals->curtime );
		m_pIk->SolveDependencies( pos, q, pBoneToWorld, boneComputed );
	}
	else
	{
		GetSkeleton(pStudioHdr, pos, q, boneMask);
	}

	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >(GetMoveParent());

	if ( pParent )
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if ( pParentCache )
		{
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				m_PlayerAnimState->GetRenderAngles(),
				adjOrigin, 
				pos, 
				q, 
				pBoneToWorld, 
				pParent, 
				pParentCache );

			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr, 
		m_PlayerAnimState->GetRenderAngles(),
		adjOrigin, 
		pos, 
		q, 
		-1,
		pBoneToWorld,
		boneMask );
}

//=========================================================
// Devuelve si el modelo especificado es válido.
//=========================================================
bool CIN_Player::IsModelValid(const char *pModel)
{
	return m_pPlayerModels.HasElement(pModel);
}

//=========================================================
// Devuelve un modelo válido al azar.
//=========================================================
const char *CIN_Player::GetRandomModel()
{
	// ¿Que? Al parecer no hay modelos...
	if ( m_pPlayerModels.Count() == 0 )
	{
		Warning("[PLAYER] ******************************** \r\n No hay modelos en la lista!!!!! \r\n ******************************** \r\n");
		return "models/player/alyx.mdl";
	}

	int iRand = RandomInt(0, m_pPlayerModels.Count()-1);
	return m_pPlayerModels[iRand];
}

//=========================================================
// Crea a la instancia de administración de respuestas y expresiones.
//=========================================================
/*
CAI_Expresser *CIN_Player::CreateExpresser()
{
	if ( m_pExpresser )
		delete m_pExpresser;

	m_pExpresser = new CAI_Expresser(this);

	if ( !m_pExpresser )
		return NULL;

	m_pExpresser->Connect(this);
	return m_pExpresser;
}

//=========================================================
// Devuelve la instancia de administración de respuestas y expresiones.
//=========================================================
CAI_Expresser *CIN_Player::GetExpresser()
{
	if ( m_pExpresser )
		m_pExpresser->Connect(this);
	
	return m_pExpresser; 
}
*/

//=========================================================
// 
//=========================================================
void CIN_Player::ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet)
{
	BaseClass::ModifyOrAppendCriteria(criteriaSet);
	ModifyOrAppendPlayerCriteria(criteriaSet);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA VOZ
//=========================================================
//=========================================================

bool CIN_Player::SpeakIfAllowed(AIConcept_t concept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
{
	if ( !IsAlive() )
		return false;

	return false;
	//return Speak(concept, modifiers, pszOutResponseChosen, bufsize, filter);
}

bool CIN_Player::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

		// can't hear dead players if we're alive
	if ( pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE )
		return false;

	return true;
}

//=========================================================
//=========================================================
// FUNCIONES DE UTILIDAD
//=========================================================
//=========================================================

//=========================================================
// Devuelve si el jugador es hombre
// o mujer. (Util para las voces de dolor)
//=========================================================
PlayerGender CIN_Player::Gender()
{
	// En modo Historia jugamos como Abigaíl (Mujer)
	if ( !InGameRules()->IsMultiplayer() )
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

	// Inventario: Mover objeto de un Loot
	if ( Q_stristr(args[0], "lootitem") )
	{
		Msg("[Loot] Se ha recibido el comando: lootitem con %i y %s \r\n", atoi(args[1]), args[2]);

		Inventory_LootItemByName(atoi(args[1]), args[2]);
		return true;
	}

	// Chat: Ignorar mensajes de...
	if ( FStrEq( args[0], "ignoremsg" ) )
	{
		//if ( ShouldRunRateLimitedCommand( args ) )
		//{
			m_iIgnoreGlobalChat = (m_iIgnoreGlobalChat + 1) % 3;

			switch( m_iIgnoreGlobalChat )
			{
				case CHAT_IGNORE_NONE:
					ClientPrint(this, HUD_PRINTTALK, "#Accept_All_Messages");
				break;

				case CHAT_IGNORE_ALL:
					ClientPrint(this, HUD_PRINTTALK, "#Ignore_Broadcast_Messages");
				break;

				
				default:
				break;
			}
		//}

		return true;
	}
	

	return BaseClass::ClientCommand(args);
}

//=========================================================
// Ejecuta un comando de tipo impulse <iImpulse>
//=========================================================
void CIN_Player::CheatImpulseCommands(int iImpulse)
{
	if ( !sv_cheats->GetBool() )
		return;

	switch ( iImpulse )
	{
		case 102:
		{
			CBaseCombatCharacter::GiveAmmo(150,		"GaussEnergy1");	// Munición: Gauss
			CBaseCombatCharacter::GiveAmmo(150,		"AlyxGun");			// Municación: Pistola automatica.

			GiveNamedItem("weapon_slam");		// S.L.A.M.
			GiveNamedItem("weapon_gauss");		// Gauss
			GiveNamedItem("weapon_alyxgun");	// Pistola automatica.

			if ( GetHealth() < GetMaxHealth() )
				TakeHealth(GetMaxHealth(), DMG_GENERIC);

			BaseClass::CheatImpulseCommands(iImpulse);
			break;
		}

		default:
			BaseClass::CheatImpulseCommands(iImpulse);
	}
}

//=========================================================
//=========================================================
// FUNCIONES A STEAM
//=========================================================
//=========================================================

#ifndef NO_STEAM

bool CIN_Player::GetSteamID(CSteamID *pID)
{
	const CSteamID *pClientID = engine->GetClientSteamID(edict());

	if ( pClientID )
	{
		*pID = *pClientID;
		return true;
	}

	return false;
}

uint64 CIN_Player::GetSteamIDAsUInt64()
{
	CSteamID steamIDForPlayer;

	if ( GetSteamID(&steamIDForPlayer) )
		return steamIDForPlayer.ConvertToUint64();

	return 0;
}

#endif // NO_STEAM

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

	flCount *= InGameRules()->GetAmmoQuantityScale(iAmmoType);

	// Don't give out less than 1 of anything.
	flCount = max(1.0f, flCount);
	return pPlayer->GiveAmmo(flCount, iAmmoType, bSuppressSound);
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
		
	int MaxItems = 0;

	if ( pSection == INVENTORY_BACKPACK )
		MaxItems = sv_max_inventory_backpack.GetInt();
	if ( pSection == INVENTORY_POCKET )
		MaxItems = sv_max_inventory_pocket.GetInt();

	// Inventario lleno.
	if ( Inventory_GetItemCount(pSection) >= MaxItems )
	{
		ClientPrint(this, HUD_PRINTCENTER, "#Inventory_HUD_Full");
		return 0;
	}

	int pEntity = InGameRules()->Inventory_GetItemID(pName);

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
				// Agregamos el objeto ahora a la mochila.
				int pPosition = Inventory_AddItemByID(pEntity, pTo);

				// Se agrego el objeto sin problemas, ahora lo eliminamos del bolsillo.
				if ( pPosition != 0 )
					Inventory_RemoveItem(pEntity, pFrom);				

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
				// Agregamos el objeto ahora en el bolsillo.
				int pPosition = Inventory_AddItemByID(pEntity, pTo);

				// Se agrego el objeto sin problemas, ahora lo eliminamos de la mochila.
				if ( pPosition != 0 )
					Inventory_RemoveItem(pEntity, pFrom);

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
	int pEntity = InGameRules()->Inventory_GetItemID(pName);
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
	int pEntity = InGameRules()->Inventory_GetItemID(pName);
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

	// Soda
	// Por obvias razones, un objeto para aliviar la sed.
	if ( Q_stristr(pItemName, "item_soda") )
	{
		ConVarRef sk_soda("sk_soda");

		if ( TakeThirst(sk_soda.GetFloat()) != 0 )
		{
			CSingleUserRecipientFilter user(this);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
				WRITE_STRING(pItemName);
			MessageEnd();

			CPASAttenuationFilter filter(this, "Player.Drink");
			EmitSound(filter, entindex(), "Player.Drink");

			Inventory_AddItem("item_empty_soda", pSection);
		}
		else
			pDelete = false;
	}

	if ( Q_stristr(pItemName, "empty_soda") )
		pDelete = false;

	// Lata de comida
	// Todos tenemos hambre.
	if ( Q_stristr(pItemName, "item_food") )
	{
		ConVarRef sk_food("sk_food");

		if ( TakeHunger(sk_food.GetFloat()) != 0 )
		{
			CSingleUserRecipientFilter user(this);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
				WRITE_STRING(pItemName);
			MessageEnd();

			CPASAttenuationFilter filter(this, "Player.Eat");
			EmitSound(filter, entindex(), "Player.Eat");

			Inventory_AddItem("item_empty_food", pSection);
		}
		else
			pDelete = false;
	}

	if ( Q_stristr(pItemName, "empty_food") )
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
	int pEntity = InGameRules()->Inventory_GetItemID(pName);
	Inventory_UseItem(pEntity, pSection);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL LOOT
//=========================================================
//=========================================================

void CIN_Player::Inventory_LootItemByName(int pLootID, const char *pName)
{
	Msg("[LOOT] Looteando objeto: %s del Loot %i \r\n", pName, pLootID);

	CEntInfo *pEntInfo	= (CEntInfo *)gEntList.GetEntInfoPtrByIndex(pLootID);
	CLoot *pLoot		= (CLoot *)pEntInfo->m_pEntity;

	// Al parecer este Loot no existe ¿lol?
	if ( !pLoot )
	{
		Msg("[LOOT] Invalido con ID %i \r\n", pLootID); // DEBUG
		return;
	}

	int pEntity = InGameRules()->Inventory_GetItemID(pName);

	// Al parecer el Loot no tiene este objeto.
	if ( !pLoot->HasItem(pEntity) )
	{
		Msg("[LOOT] No tiene el objeto: %i \r\n", pEntity); // DEBUG
		return;
	}

	// Agregamos el objeto a la mochila.
	int pPosition = Inventory_AddItem(pName, INVENTORY_BACKPACK);

	Msg("[LOOT] Posicion del objeto: %i - Objeto: %s \r\n", pPosition, pName);

	// Se agrego el objeto sin problemas, ahora lo eliminamos del Loot.
	if ( pPosition != 0 )
		pLoot->RemoveItem(pEntity, this);
}