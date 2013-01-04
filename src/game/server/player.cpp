//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Funciones y sistemas del jugador.
//
//===========================================================================//

#include "cbase.h"
#include "const.h"
#include "baseplayer_shared.h"
#include "trains.h"
#include "soundent.h"
#include "gib.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "entityapi.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "worldsize.h"
#include "isaverestore.h"
#include "globalstate.h"
#include "basecombatweapon.h"
#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_networkmanager.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "ndebugoverlay.h"
#include "baseviewmodel.h"
#include "in_buttons.h"
#include "client.h"
#include "team.h"
#include "particle_smokegrenade.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movehelper_server.h"
#include "igamemovement.h"
#include "saverestoretypes.h"
#include "iservervehicle.h"
#include "movevars_shared.h"
#include "vcollide_parse.h"
#include "player_command.h"
#include "vehicle_base.h"
#include "AI_Criteria.h"
#include "globals.h"
#include "usermessages.h"
#include "gamevars_shared.h"
#include "world.h"
#include "physobj.h"
#include "KeyValues.h"
#include "coordsize.h"
#include "vphysics/player_controller.h"
#include "saverestore_utlvector.h"
#include "hltvdirector.h"
#include "nav_mesh.h"
#include "env_zoom.h"
#include "rumble_shared.h"
#include "GameStats.h"
#include "npcevent.h"
#include "datacache/imdlcache.h"
#include "hintsystem.h"
#include "env_debughistory.h"
#include "fogcontroller.h"
#include "gameinterface.h"
#include "hl2orange.spa.h"

#ifdef HL2_DLL
	#include "combine_mine.h"
	#include "weapon_physcannon.h"
#endif

//----------------------------------------------------
// Definici�n de variables de configuraci�n.
//----------------------------------------------------

ConVar autoaim_max_dist("autoaim_max_dist", "2160");
ConVar autoaim_max_deflect("autoaim_max_deflect", "0.99");

ConVar spec_freeze_time("spec_freeze_time", "4.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Time spend frozen in observer freeze cam.");
ConVar spec_freeze_traveltime("spec_freeze_traveltime", "0.4", FCVAR_CHEAT | FCVAR_REPLICATED, "Time taken to zoom in to frame a target in observer freeze cam.", true, 0.01, false, 0);

ConVar sv_bonus_challenge("sv_bonus_challenge", "0", FCVAR_REPLICATED, "Set to values other than 0 to select a bonus map challenge type." );

#include "tier0/memdbgon.h"

static ConVar old_armor("player_old_armor", "0");
static ConVar physicsshadowupdate_render("physicsshadowupdate_render", "0");

static ConVar sv_clockcorrection_msecs("sv_clockcorrection_msecs", "60", 0, "The server tries to keep each player's m_nTickBase withing this many msecs of the server absolute tickcount");
static ConVar sv_playerperfhistorycount("sv_playerperfhistorycount", "20", 0, "Number of samples to maintain in player perf history", true, 1.0f, true, 128.0);

bool IsInCommentaryMode( void );
bool IsListeningToCommentary( void );

ConVar sv_noclipduringpause("sv_noclipduringpause", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If cheats are enabled, then you can noclip with the game paused (for doing screenshots, etc.).");
ConVar xc_crouch_debounce("xc_crouch_debounce", "0", FCVAR_NONE);

extern ConVar sv_maxunlag;
extern ConVar sv_turbophysics;
extern ConVar *sv_maxreplay;

extern CServerGameDLL g_ServerGameDLL;
extern CMoveData *g_pMoveData;

//----------------------------------------------------
// InSource - Definici�n de variables de configuraci�n.
//---------------------------------------------------

// Regeneraci�n de salud.
ConVar in_regeneration("in_regeneration", "1", FCVAR_REPLICATED, "Estado de la regeneracion de salud");
ConVar in_regeneration_wait_time("in_regeneration_wait_time", "10.0", FCVAR_REPLICATED, "Tiempo de espera para regenerar");
ConVar in_regeneration_rate("in_regeneration_rate", "0.1", FCVAR_REPLICATED, "Rango de tiempo para regenerar");

// Recarga automatica de arma.
ConVar in_automatic_reload("in_automatic_reload", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Recarga automatica del arma");

// Efecto de cansancio.
ConVar in_tired_effect("in_tired_effect", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de cansancio al perder salud.");

// Camara lenta.
ConVar in_timescale_effect("in_timescale_effect", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Activa o desactiva el efecto de camara lenta al perder salud.");

//----------------------------------------------------
// Tiempo para los tipos de da�os.
//----------------------------------------------------

#define PARALYZE_DURATION	2
#define PARALYZE_DAMAGE		1.0

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0

#define OLD_ARMOR_RATIO	 0.2
#define OLD_ARMOR_BONUS  0.5

#define ARMOR_RATIO	0.2
#define ARMOR_BONUS	1.0

#define MIN_SHOCK_AND_CONFUSION_DAMAGE	30.0f
#define MIN_EAR_RINGING_DISTANCE		240.0f

//----------------------------------------------------
// Agua
//----------------------------------------------------

#ifdef HL2_DLL
	#define AIRTIME						10
	#define DROWNING_DAMAGE_INITIAL		10
	#define DROWNING_DAMAGE_MAX			10
#else
	#define AIRTIME						12
	#define DROWNING_DAMAGE_INITIAL		2
	#define DROWNING_DAMAGE_MAX			5
#endif

//----------------------------------------------------
// Traje de protecci�n
//----------------------------------------------------

#define GEIGERDELAY 0.25

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

#define SMOOTHING_FACTOR 0.9

//----------------------------------------------------
// Sombra de la f�sica del jugador.
//----------------------------------------------------

#define VPHYS_MAX_DISTANCE		2.0
#define VPHYS_MAX_VEL			10
#define VPHYS_MAX_DISTSQR		(VPHYS_MAX_DISTANCE*VPHYS_MAX_DISTANCE)
#define VPHYS_MAX_VELSQR		(VPHYS_MAX_VEL*VPHYS_MAX_VEL)

//----------------------------------------------------
// Definici�n de variables globales.
//----------------------------------------------------

extern bool		g_fDrawLines;
int				gEvilImpulse101;

bool gInitHUD = true;

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
int MapTextureTypeStepType(char chTextureType);
extern void	SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);
extern void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );

#define CMD_MOSTRECENT 0

//----------------------------------------------------
// InSource - Definici�n de variables globales.
//----------------------------------------------------

float			m_fRegenRemander;	// Tiempo de la �ltima regeneraci�n de salud.
float			m_fDeadTimer;		// Tiempo para la ejecuci�n del efecto de muerte.
float			m_fTasksTimer;		// Tiempo para la ejecuci�n de tareas.

//----------------------------------------------------
// Definici�n de variables de da�o al jugador.
//---------------------------------------------------

ConVar	sk_player_head("sk_player_head","2");
ConVar	sk_player_chest("sk_player_chest","1");
ConVar	sk_player_stomach("sk_player_stomach","1");
ConVar	sk_player_arm("sk_player_arm","1");
ConVar	sk_player_leg("sk_player_leg","1");

ConVar  player_debug_print_damage("player_debug_print_damage", "0", FCVAR_CHEAT, "Activa o desactiva el envio de mensajes de da�o del usuario a la consola.");

CBaseEntity	*g_pLastSpawn = NULL;

/*----------------------------------------------------
	CC_GiveCurrentAmmo()
	Darle al usuario munici�n del arma actual.
----------------------------------------------------*/
void CC_GiveCurrentAmmo(void)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	// Algo anda mal con el usuario. ?
	if(!pPlayer)
		return;

	CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

	// Al parecer no hay ninguna arma activa.
	if(!pWeapon)
		return;

	if(pWeapon->UsesPrimaryAmmo())
	{
		int ammoIndex = pWeapon->GetPrimaryAmmoType();

		if( ammoIndex != -1 )
		{
			int giveAmount;

			giveAmount = GetAmmoDef()->MaxCarry(ammoIndex);
			pPlayer->GiveAmmo(giveAmount, GetAmmoDef()->GetAmmoOfIndex(ammoIndex)->pName);
		}
	}

	if(pWeapon->UsesSecondaryAmmo() && pWeapon->HasSecondaryAmmo())
	{
		int ammoIndex = pWeapon->GetSecondaryAmmoType();

		if( ammoIndex != -1 )
		{
			int giveAmount;

			giveAmount = GetAmmoDef()->MaxCarry(ammoIndex);
			pPlayer->GiveAmmo( giveAmount, GetAmmoDef()->GetAmmoOfIndex(ammoIndex)->pName );
		}
	}
}

// Comando para ejecutar la funci�n.
static ConCommand givecurrentammo("givecurrentammo", CC_GiveCurrentAmmo, "Obtener munici�n para el arma actual..\n", FCVAR_CHEAT);

//----------------------------------------------------
// Guardado y definici�n de datos
//----------------------------------------------------

BEGIN_SIMPLE_DATADESC( CPlayerState )
	DEFINE_FIELD( v_angle, FIELD_VECTOR ),
	DEFINE_FIELD( deadflag, FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_DATADESC( CBasePlayer )
	DEFINE_EMBEDDED( m_Local ),
	DEFINE_UTLVECTOR( m_hTriggerSoundscapeList, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( pl ),

	DEFINE_FIELD( m_StuckLast, FIELD_INTEGER ),

	DEFINE_FIELD( m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonReleased, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonDisabled, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonForced,	FIELD_INTEGER ),

	DEFINE_FIELD( m_iFOV,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iFOVStart,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flFOVTime,	FIELD_TIME ),
	DEFINE_FIELD( m_iDefaultFOV,FIELD_INTEGER ),
	DEFINE_FIELD( m_flVehicleViewFOV, FIELD_FLOAT ),

	DEFINE_FIELD( m_iObserverMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iObserverLastMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_hObserverTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bForcedObserverMode, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_szAnimExtension, FIELD_CHARACTER ),

	DEFINE_FIELD( m_nUpdateRate, FIELD_INTEGER ),
	DEFINE_FIELD( m_fLerpTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bLagCompensation, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPredictWeapons, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_vecAdditionalPVSOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecCameraPVSOrigin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_hUseEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( m_iRespawnFrames, FIELD_FLOAT ),
	DEFINE_FIELD( m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_hVehicle, FIELD_EHANDLE ),

	DEFINE_ARRAY( m_szNetworkIDString, FIELD_CHARACTER, MAX_NETWORKID_LENGTH ),	
	DEFINE_FIELD( m_oldOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecSmoothedVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgItems, FIELD_INTEGER ),

	DEFINE_FIELD( m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDuckJumpTime, FIELD_TIME ),

	DEFINE_FIELD( m_flSuitUpdate, FIELD_TIME ),
	DEFINE_AUTO_ARRAY( m_rgSuitPlayList, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgiSuitNoRepeat, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgflSuitNoRepeatTime, FIELD_TIME ),
	DEFINE_FIELD( m_bPauseBonusProgress, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBonusProgress, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBonusChallenge, FIELD_INTEGER ),
	DEFINE_FIELD( m_lastDamageAmount, FIELD_INTEGER ),
	DEFINE_FIELD( m_tbdPrev, FIELD_TIME ),
	DEFINE_FIELD( m_flLastDamageTime, FIELD_TIME ),
	DEFINE_FIELD( m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_ARRAY( m_szNetname, FIELD_CHARACTER, MAX_PLAYER_NAME_LENGTH ),

	DEFINE_FIELD( m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( m_idrownrestored, FIELD_INTEGER ),

	DEFINE_FIELD( m_nPoisonDmg, FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoisonRestored, FIELD_INTEGER ),

	DEFINE_FIELD( m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDeathTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDeathAnimTime, FIELD_TIME ),

	DEFINE_FIELD( m_iFrags, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDeaths, FIELD_INTEGER ),
	DEFINE_FIELD( m_bAllowInstantSpawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextDecalTime, FIELD_TIME ),

	DEFINE_FIELD( m_ArmorValue, FIELD_INTEGER ),
	DEFINE_FIELD( m_DmgOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_DmgTake, FIELD_FLOAT ),
	DEFINE_FIELD( m_DmgSave, FIELD_FLOAT ),
	DEFINE_FIELD( m_AirFinished, FIELD_TIME ),
	DEFINE_FIELD( m_PainFinished, FIELD_TIME ),
	
	DEFINE_FIELD( m_iPlayerLocked, FIELD_INTEGER ),

	DEFINE_AUTO_ARRAY( m_hViewModel, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_flMaxspeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flWaterJumpTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecWaterJumpVel, FIELD_VECTOR ),
	DEFINE_FIELD( m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( m_flSwimSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecLadderNormal, FIELD_VECTOR ),

	DEFINE_FIELD( m_flFlashTime, FIELD_TIME ),
	DEFINE_FIELD( m_nDrownDmgRate, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSuicideCustomKillFlags, FIELD_INTEGER ),

	DEFINE_FIELD( m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgbTimeBasedDamage, FIELD_CHARACTER ),
	DEFINE_FIELD( m_fLastPlayerTalkTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_hLastWeapon, FIELD_EHANDLE ),

	DEFINE_FIELD( m_flOldPlayerZ, FIELD_FLOAT ),
	DEFINE_FIELD( m_flOldPlayerViewOffsetZ, FIELD_FLOAT ),
	DEFINE_FIELD( m_bPlayerUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hViewEntity, FIELD_EHANDLE ),

	DEFINE_FIELD( m_hConstraintEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecConstraintCenter, FIELD_VECTOR ),
	DEFINE_FIELD( m_flConstraintRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flConstraintWidth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flConstraintSpeedFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_hZoomOwner, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_flLaggedMovementValue, FIELD_FLOAT ),

	DEFINE_FIELD( m_vNewVPhysicsPosition, FIELD_VECTOR ),
	DEFINE_FIELD( m_vNewVPhysicsVelocity, FIELD_VECTOR ),

	DEFINE_FIELD( m_bSinglePlayerGameEnding, FIELD_BOOLEAN ),
	DEFINE_ARRAY( m_szLastPlaceName, FIELD_CHARACTER, MAX_PLACE_NAME_LENGTH ),

	DEFINE_FIELD( m_autoKickDisabled, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_FUNCTION( PlayerDeathThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetHUDVisibility", InputSetHUDVisibility ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFogController", InputSetFogController ),

	DEFINE_FIELD( m_nNumCrouches, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDuckToggled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flForwardMove, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSideMove, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecPreviouslyPredictedOrigin, FIELD_POSITION_VECTOR ), 

	DEFINE_FIELD( m_nNumCrateHudHints, FIELD_INTEGER ),
END_DATADESC()

int giPrecacheGrunt = 0;
edict_t *CBasePlayer::s_PlayerEdict = NULL;

inline bool ShouldRunCommandsInContext( const CCommandContext *ctx )
{
	#ifdef NO_USERCMDS_DURING_PAUSE
		return !ctx->paused || sv_noclipduringpause.GetInt();
	#else
		return true;
	#endif
}

/*---------------------------------------------------
	GetViewModel()
	Obtener la vista del modelo.
-----------------------------------------------------
- index: 
---------------------------------------------------*/
CBaseViewModel *CBasePlayer::GetViewModel(int index)
{
	//Assert(index >= 0 && index < MAX_VIEWMODELS);
	try
	{
		return m_hViewModel[ index ].Get();
	}
	catch(char * str)
	{
		
	}
}

/*---------------------------------------------------
	CreateViewModel()
	Crear la vista del modelo.
-----------------------------------------------------
- index: 
---------------------------------------------------*/
void CBasePlayer::CreateViewModel(int index)
{
	Assert(index >= 0 && index < MAX_VIEWMODELS);

	if (GetViewModel(index))
		return;

	CBaseViewModel *vm = ( CBaseViewModel * )CreateEntityByName("viewmodel");

	if (vm)
	{
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		vm->SetIndex(index);
		DispatchSpawn(vm);
		vm->FollowEntity(this);
		m_hViewModel.Set(index, vm);
	}
}

/*---------------------------------------------------
	DestroyViewModels()
	Destruir las vistas de los modelos.
---------------------------------------------------*/
void CBasePlayer::DestroyViewModels( void )
{
	int i;

	for (i = MAX_VIEWMODELS - 1; i >= 0; i--)
	{
		CBaseViewModel *vm = GetViewModel(i);

		if (!vm)
			continue;

		UTIL_Remove(vm);
		m_hViewModel.Set( i, NULL );
	}
}


/*---------------------------------------------------
	CreatePlayer()
	Crear jugador de la clase especificada.
-----------------------------------------------------
- className: Nombre de la clase.
- ed:
---------------------------------------------------*/
CBasePlayer *CBasePlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CBasePlayer *player;
	CBasePlayer::s_PlayerEdict = ed;

	player = (CBasePlayer *)CreateEntityByName(className);

	return player;
}

/*---------------------------------------------------
	CBasePlayer
	Clase del jugador maestro (Usuario)
---------------------------------------------------*/
CBasePlayer::CBasePlayer()
{
	AddEFlags(EFL_NO_AUTO_EDICT_ATTACH);

	#ifdef _DEBUG
		m_vecAutoAim.Init();
		m_vecAdditionalPVSOrigin.Init();
		m_vecCameraPVSOrigin.Init();
		m_DmgOrigin.Init();
		m_vecLadderNormal.Init();

		m_oldOrigin.Init();
		m_vecSmoothedVelocity.Init();
	#endif

	if (s_PlayerEdict)
	{
		Assert(s_PlayerEdict != NULL);
		NetworkProp()->AttachEdict( s_PlayerEdict );
		s_PlayerEdict = NULL;
	}

	// Reseteo de variables globales.

	m_flFlashTime	= -1;
	pl.fixangle		= FIXANGLE_ABSOLUTE;
	pl.hltv			= false;
	pl.frags		= 0;
	pl.deaths		= 0;

	m_szNetname[0]	= '\0';

	m_iHealth			= 0; // Salud.

	Weapon_SetLast(NULL);
	m_bitsDamageType	= 0;

	m_bForceOrigin		= false;
	m_hVehicle			= NULL;
	m_pCurrentCommand	= NULL;
	
	m_iDefaultFOV		= g_pGameRules->DefaultFOV();
	m_hZoomOwner		= NULL;

	m_nUpdateRate			= 20;
	m_fLerpTime				= 0.1f;
	m_bPredictWeapons		= true;
	m_bLagCompensation		= false;
	m_flLaggedMovementValue = 1.0f;
	m_StuckLast				= 0;
	m_impactEnergyScale		= 1.0f;
	m_fLastPlayerTalkTime	= 0.0f;
	m_PlayerInfo.SetParent(this);

	ResetObserverMode();

	m_surfaceProps			= 0;
	m_pSurfaceData			= NULL;
	m_surfaceFriction		= 1.0f;
	m_chTextureType			= 0;
	m_chPreviousTextureType = 0;

	m_iSuicideCustomKillFlags	= 0;
	m_fDelay					= 0.0f;
	m_fReplayEnd				= -1;
	m_iReplayEntity				= 0;

	m_autoKickDisabled	= false;
	m_nNumCrouches		= 0;
	m_bDuckToggled		= false;
	m_bPhysicsWasFrozen = false;

	m_afButtonDisabled	= 0;
	m_afButtonForced	= 0;

	m_nBodyPitchPoseParam	= -1;
	m_flForwardMove			= 0;

	/* InSource */

	m_fRegenRemander	= 0; // Regeneraci�n de salud
	m_fDeadTimer		= 0; // Efecto de muerte.
	m_fTasksTimer		= 0; // Tareas.
}

CBasePlayer::~CBasePlayer( )
{
	VPhysicsDestroyObject();
}

/*---------------------------------------------------
	UpdateOnRemove()
	Actualizar al remover al jugador del juego.
	NOTA: No se sabe si hace eso exactamente...
---------------------------------------------------*/
void CBasePlayer::UpdateOnRemove(void)
{
	VPhysicsDestroyObject();

	// Al parecer esta en un equipo, removerlo del equipo
	if (GetTeam())
		GetTeam()->RemovePlayer(this);

	BaseClass::UpdateOnRemove();
}

/*---------------------------------------------------
	SetupVisibility()
	Ajustar visibilidad
	NOTA: No se sabe que hace exactamente...
---------------------------------------------------*/
void CBasePlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	if ( pViewEntity )
		return;

	Vector org;
	org = EyePosition();

	engine->AddOriginToPVS(org);
}

/*---------------------------------------------------
	UpdateTransmitState()
	Actualizar transmisi�n de estado.
	NOTA: No se sabe que hace exactamente...
---------------------------------------------------*/
int	CBasePlayer::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

/*---------------------------------------------------
	ShouldTransmit()
	Transmitir
	NOTA: No se sabe que hace exactamente...
---------------------------------------------------*/
int CBasePlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if (pInfo->m_pClientEnt == edict())
		return FL_EDICT_ALWAYS;

	if (HLTVDirector()->GetCameraMan() == entindex())
	{
		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );		
		Assert(pRecipientEntity->IsPlayer());
		
		CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );

		if (pRecipientPlayer->IsHLTV())
		{
			NetworkProp()->AreaNum();
			return FL_EDICT_ALWAYS;
		}
	}

	if ( IsEffectActive( EF_NODRAW ) || ( IsObserver() && ( gpGlobals->curtime - m_flDeathTime > 0.5 ) && 
		( m_lifeState == LIFE_DEAD ) && ( gpGlobals->curtime - m_flDeathAnimTime > 0.5 ) ) )
	{
		return FL_EDICT_DONTSEND;
	}

	return BaseClass::ShouldTransmit(pInfo);
}

/*---------------------------------------------------
	WantsLagCompensationOnEntity()
	Calcular si la entidad desea una compensaci�n de Lag.
	NOTA: Falta de investigaci�n.
---------------------------------------------------*/
bool CBasePlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	if (gpGlobals->teamplay)
	{
		if (!friendlyfire.GetInt() && pPlayer->GetTeamNumber() == GetTeamNumber())
			return false;
	}

	if ( pEntityTransmitBits && !pEntityTransmitBits->Get(pPlayer->entindex()))
		return false;

	const Vector &vMyOrigin		= GetAbsOrigin();
	const Vector &vHisOrigin	= pPlayer->GetAbsOrigin();

	// Calcular la m�xima distancia de compensaci�n.
	// 1.5 es lo ideal para evitar "zonas muertas".
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	if (vHisOrigin.DistTo(vMyOrigin) < maxDistance)
		return true;

	Vector vForward;
	AngleVectors(pCmd->viewangles, &vForward);
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize(vDiff);

	float flCosAngle = 0.707107f; // 45 grados.

	if (vForward.Dot( vDiff ) < flCosAngle)
		return false;

	return true;
}

/*---------------------------------------------------
	PauseBonusProgress()
	
	NOTA: Falta de investigaci�n.
---------------------------------------------------*/
void CBasePlayer::PauseBonusProgress(bool bPause)
{
	m_bPauseBonusProgress = bPause;
}

/*---------------------------------------------------
	SetBonusProgress()
	
	NOTA: Falta de investigaci�n.
---------------------------------------------------*/
void CBasePlayer::SetBonusProgress(int iBonusProgress)
{
	if ( !m_bPauseBonusProgress )
		m_iBonusProgress = iBonusProgress;
}

/*---------------------------------------------------
	SetBonusChallenge()
	
	NOTA: Falta de investigaci�n.
---------------------------------------------------*/
void CBasePlayer::SetBonusChallenge(int iBonusChallenge)
{
	m_iBonusChallenge = iBonusChallenge;
}


/*---------------------------------------------------
	SetBonusChallenge()
	Ajustar cambios en los grados del ojo.
---------------------------------------------------
*/
void CBasePlayer::SnapEyeAngles( const QAngle &viewAngles )
{
	pl.v_angle	= viewAngles;
	pl.fixangle = FIXANGLE_ABSOLUTE;
}


/*---------------------------------------------------
	TrainSpeed()
	Devolver INT dependiendo de la velocidad del objeto.
---------------------------------------------------*/
int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax	= (float)iMax;
	fSpeed	= iSpeed;

	fSpeed	= fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

/*---------------------------------------------------
	DeathSound()
	Reproducir sonido de muerte del jugador.
---------------------------------------------------*/
void CBasePlayer::DeathSound(const CTakeDamageInfo &info)
{
	// Muerte por caida.
	// Reproducir sonido de "rompedura de huesos"
	if ( m_bitsDamageType & DMG_FALL )
		EmitSound("Player.FallGib");

	else
		EmitSound("Player.Death");

	StopSound("Player.Music.Dying");
	
	// Reproducir sonido del traje de protecci�n.
	// bep, bep, bep, beeeeeeep...
	if (IsSuitEquipped())
		UTIL_EmitGroupnameSuit(edict(), "HEV_DEAD");

	//enginesound->StopAllSounds(true);
	engine->ClientCommand(edict(), "stopsound");

	EmitSound("Player.Music.Dead");
}

/*---------------------------------------------------
	TakeHealth()
	Obtener salud.
---------------------------------------------------*/
int CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	if (m_takedamage)
	{
		int bitsDmgTimeBased = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType	&= ~( bitsDamageType & ~bitsDmgTimeBased );
	}

	return BaseClass::TakeHealth(flHealth, bitsDamageType);
}

/*---------------------------------------------------
	DrawDebugGeometryOverlays()
	Dibuja todas las "superposiciones".
---------------------------------------------------*/
void CBasePlayer::DrawDebugGeometryOverlays(void) 
{
	if ((m_debugOverlays & OVERLAY_BUDDHA_MODE) && m_iHealth == 1)
	{
		Vector vBodyDir = BodyDirection2D();
		Vector eyePos	= EyePosition() + vBodyDir*10.0;
		Vector vUp		= Vector(0,0,8);
		Vector vSide;

		CrossProduct(vBodyDir, vUp, vSide);
		NDebugOverlay::Line(eyePos+vSide+vUp, eyePos-vSide-vUp, 255,0,0, false, 0);
		NDebugOverlay::Line(eyePos+vSide-vUp, eyePos-vSide+vUp, 255,0,0, false, 0);
	}

	BaseClass::DrawDebugGeometryOverlays();
}

/*---------------------------------------------------
	TraceAttack()
	Calcula, rastrea y administra un ataque.
---------------------------------------------------*/
void CBasePlayer::TraceAttack(const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr)
{
	// IF
	if (!m_takedamage)
		return;

	CTakeDamageInfo info = inputInfo;

	if (info.GetAttacker())
	{
		// Si el NPC detecta que el fuego amigo no esta permido.
		CAI_BaseNPC *pNPC = info.GetAttacker()->MyNPCPointer();

		if (pNPC && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) && pNPC->IRelationType(this) != D_HT)
			return;

		if (info.GetAttacker()->IsPlayer())
		{
			if (!g_pGameRules->FPlayerCanTakeDamage(this, info.GetAttacker()))
				return;
		}
	}

	// Ubicaci�n del ataque.
	SetLastHitGroup(ptr->hitgroup);
		
	switch (ptr->hitgroup)
	{
		case HITGROUP_GENERIC:	// Generico.
			break;
		case HITGROUP_HEAD:		// Justo en la cabeza.
			info.ScaleDamage(sk_player_head.GetFloat());
			break;
		case HITGROUP_CHEST:	// En el pecho.
			info.ScaleDamage(sk_player_chest.GetFloat());
			break;
		case HITGROUP_STOMACH:	// Estomago
			info.ScaleDamage(sk_player_stomach.GetFloat());
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:	// Brazo izquierdo o derecho.
			info.ScaleDamage(sk_player_arm.GetFloat());
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG: // Pierna izquierda o derecha.
			info.ScaleDamage(sk_player_leg.GetFloat());
			break;
		default:
			break;
	}

	#ifdef HL2_EPISODIC
		// Verificar si el da�o produce sangrado.
		bool bShouldBleed = !g_pGameRules->Damage_ShouldNotBleed(info.GetDamageType());
		if ( bShouldBleed )
	#endif
		{
			// Lanzar sangre.
			SpawnBlood(ptr->endpos, vecDir, BloodColor(), info.GetDamage());
			TraceBleed(info.GetDamage(), vecDir, ptr, info.GetDamageType());
		}

	AddMultiDamage(info, this);
}

/*---------------------------------------------------
	DamageEffect()
	Muestra el efecto en el indicar seg�n sea el da�o.
---------------------------------------------------*/
void CBasePlayer::DamageEffect(float flDamage, int fDamageType)
{
	// Aplastado - Rojo
	if (fDamageType & DMG_CRUSH)
	{
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}

	// Ahogandose - Azul
	else if (fDamageType & DMG_DROWN)
	{
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}

	// Cuchillo - Lanzar sangre.
	else if (fDamageType & DMG_SLASH)
		SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
	
	// Plasma - Azul
	else if (fDamageType & DMG_PLASMA)
	{
		color32 blue = {0,0,255,100};
		UTIL_ScreenFade(this, blue, 0.2, 0.4, FFADE_MODULATE);

		// Peque�a sacudida de la pantalla.
		ViewPunch(QAngle(random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1)));

		// Reproducir sonido de dolor por fuego.
		EmitSound("Player.PlasmaDamage");
	}

	// Caida - Reproducir sonido
	else if (fDamageType & DMG_SONIC)
		EmitSound("Player.SonicDamage");
	
	// Balas - Reproducir sonido
	else if (fDamageType & DMG_BULLET)
		EmitSound("Flesh.BulletImpact");
}

/*---------------------------------------------------
	ShouldTakeDamageInCommentaryMode()
	Calcular si se debe tomar da�o en el modo "Comentarios del desarrollador"
---------------------------------------------------*/
bool CBasePlayer::ShouldTakeDamageInCommentaryMode( const CTakeDamageInfo &inputInfo )
{
	// Al parecer no esta escuchando ning�n comentario, activar todo da�o.
	if (!IsListeningToCommentary())
		return true;

	if (inputInfo.GetInflictor() == this && inputInfo.GetAttacker() == this)
		return true;

	#ifdef PORTAL
		if (inputInfo.GetDamageType() & DMG_ACID)
			return true;
	#endif

	// Ignorar todos los tipos de da�os excepto: Quemaduras, Plasma, Caidas y Aplastados
	if (!(inputInfo.GetDamageType() & (DMG_BURN | DMG_PLASMA | DMG_FALL | DMG_CRUSH)) && inputInfo.GetDamageType() != DMG_GENERIC)
		return false;

	if ((inputInfo.GetDamageType() & DMG_CRUSH) && ( inputInfo.GetAttacker() == NULL || !inputInfo.GetAttacker()->IsBSPModel()))
		return false;

	return true;
}

/*---------------------------------------------------
	OnTakeDamage()
	Al recibir da�o.
---------------------------------------------------*/
int CBasePlayer::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	int bitsDamage		= inputInfo.GetDamageType();
	int ffound			= true;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev	= m_iHealth;

	CTakeDamageInfo info		= inputInfo;
	IServerVehicle *pVehicle	= GetVehicle();

	// Modo Dios - �Que hace dios teniendo da�o?
	if (GetFlags() & FL_GODMODE)
		return 0;

	// El jugador ya esta muerto
	if (!IsAlive())
		return 0;

	// Al parecer estamos en un veh�culo
	if (pVehicle)
	{
		if (pVehicle->PassengerShouldReceiveDamage(info) == false)
			return 0;
	}

	// Modo "comentarios del desarrollador"
 	if (IsInCommentaryMode())
	{
		if(!ShouldTakeDamageInCommentaryMode(info))
			return 0;
	}

	if (m_debugOverlays & OVERLAY_BUDDHA_MODE) 
	{
		if ((m_iHealth - info.GetDamage()) <= 0)
		{
			m_iHealth = 1;
			return 0;
		}
	}

	if (!info.GetDamage())
		return 0;

	if(old_armor.GetBool())
	{
		flBonus = OLD_ARMOR_BONUS;
		flRatio = OLD_ARMOR_RATIO;
	}
	else
	{
		flBonus = ARMOR_BONUS;
		flRatio = ARMOR_RATIO;
	}

	if ((info.GetDamageType() & DMG_BLAST) && g_pGameRules->IsMultiplayer())
		flBonus *= 2;
	
	if (!g_pGameRules->FPlayerCanTakeDamage(this, info.GetAttacker()))
		return 0;

	m_lastDamageAmount = info.GetDamage();

	// Tenemos traje de protecci�n
	// NOTA: El traje de protecci�n no te ayuda cuando estas: Cayendo, Ahogandote, Envenenado o en una zona de radiaci�n
	if (m_ArmorValue && !(info.GetDamageType() & (DMG_FALL | DMG_DROWN | DMG_POISON | DMG_RADIATION)))
	{
		float flNew = info.GetDamage() * flRatio;
		float flArmor;

		flArmor = (info.GetDamage() - flNew) * flBonus;

		if(!old_armor.GetBool())
		{
			if( flArmor < 1.0 )
				flArmor = 1.0;
		}

		if (flArmor > m_ArmorValue)
		{
			flArmor		= m_ArmorValue;
			flArmor		*= (1/flBonus);
			flNew		= info.GetDamage() - flArmor;

			m_DmgSave		= m_ArmorValue;
			m_ArmorValue	= 0;
		}
		else
		{
			m_DmgSave		= flArmor;
			m_ArmorValue	-= flArmor;
		}
		
		info.SetDamage(flNew);
	}

	fTookDamage = BaseClass::OnTakeDamage(info);

	if (!fTookDamage)
		return 0;

	if (info.GetInflictor() && info.GetInflictor()->edict())
		m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();

	m_DmgTake += (int)info.GetDamage();
	
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		int iDamage = ( DMG_PARALYZE << i );

		if ((info.GetDamageType() & iDamage) && g_pGameRules->Damage_IsTimeBased(iDamage))
			m_rgbTimeBasedDamage[i] = 0;
	}

	// Mostrar cualquier efecto relacionado al da�o
	DamageEffect(info.GetDamage(), bitsDamage);

	// Calcular el da�o producido
	// Sonidos del traje de protecci�n: �Qu� tan grave es el da�o doctor?
	ftrivial	= (m_iHealth > 75 || m_lastDamageAmount < 5);
	fmajor		= (m_lastDamageAmount > 25);
	fcritical	= (m_iHealth < 30);

	m_bitsDamageType |= bitsDamage;
	m_bitsHUDDamage = -1;

	while ((!ftrivial || g_pGameRules->Damage_IsTimeBased(bitsDamage)) && ffound && bitsDamage)
	{
		ffound = false;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // Fractura peque�a

			bitsDamage &= ~DMG_CLUB;
			ffound		= true;
		}

		// Caida o aplastado
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC); // Fractura peque�a
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // Fractura grande
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound		= true;
		}
		
		// Balas
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC); // Perdida de sangre

			// InSource - Sonidos extra
			if (m_iHealth <= 50)
			{
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_5MIN);	// Hemorragia interna
				SetSuitUpdate("!HEV_HEAL1", false, SUIT_NEXT_IN_5MIN);	// Hemorragia detenida
			}

			// InSource - Ouch!
			if(m_lastDamageAmount > 3)
				EmitSound("Player.Pain");
			
			bitsDamage &= ~DMG_BULLET;
			ffound		= true;
		}

		// Cortada, cuchillo...
		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC); // Laceraciones grandes
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC); // Laceraciones peque�as

			bitsDamage &= ~DMG_SLASH;
			ffound		= true;
		}
		
		// ?
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
			{
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN);	// Hemorragia interna
				SetSuitUpdate("!HEV_HEAL1", false, SUIT_NEXT_IN_1MIN);	// Hemorragia detenida
			}

			bitsDamage &= ~DMG_SONIC;
			ffound		= true;
		}

		// Envenenamiento
		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			if (bitsDamage & DMG_POISON)
			{
				m_nPoisonDmg								+= info.GetDamage();
				m_tbdPrev									= gpGlobals->curtime;
				m_rgbTimeBasedDamage[itbd_PoisonRecover]	= 0;
			}

			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN); // Niveles de toxinas en la sangre.

			bitsDamage &= ~( DMG_POISON | DMG_PARALYZE );
			ffound		= true;
		}

		// Acido
		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN); // Sustancia quimica peligrosa.

			bitsDamage &= ~DMG_ACID;
			ffound		= true;
		}

		// Gas nervioso
		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN); // Amenaza biologica.

			bitsDamage &= ~DMG_NERVEGAS;
			ffound		= true;
		}

		// Radiaci�n
		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN); // Niveles peligrosos de radiaci�n.

			bitsDamage &= ~DMG_RADIATION;
			ffound		= true;
		}

		// Electrocuci�n
		if (bitsDamage & DMG_SHOCK)
		{
			SetSuitUpdate("!HEV_SHOCK", false, SUIT_NEXT_IN_5MIN); // Aver�a en el sistema electrico.
			EmitSound("Player.Pain");

			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	float flPunch = -2;

	if(hl2_episodic.GetBool() && info.GetAttacker() && !FInViewCone(info.GetAttacker()))
	{
		if( info.GetDamage() > 10.0f )
			flPunch = -10;
		else
			flPunch = RandomFloat( -5, -7 );
	}

	m_Local.m_vecPunchAngle.SetX(flPunch);
	
	// InSource - Sonidos del traje realisticos.
	if (!ftrivial && fmajor && flHealthPrev >= 75) 
	{
		SetSuitUpdate("!HEV_MED2", false, SUIT_NEXT_IN_30MIN);	// Administrando atenci�n medica.

		// 15+ de salud por da�os.
		TakeHealth(15, DMG_GENERIC);

		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);	// Administrado morfina.
	}

	if (!ftrivial && fcritical && flHealthPrev < 75)
	{
		if (m_iHealth <= 5)
			SetSuitUpdate("!HEV_HLTH6", false, SUIT_NEXT_IN_10MIN);	// �Emergencia! Muerte inminente del usuario, evacuar zona de inmediato.

		else if (m_iHealth <= 10)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);	// �Emergencia! Muerte inminente del usuario.

		else if (m_iHealth < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);	// Precauci�n, signos vitales criticos.

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

	// Da�o por explosi�n.
	if (bitsDamage & DMG_BLAST)
		OnDamagedByExplosion(info);

	// �ltima vez que se ha sufrido da�o.
	m_flLastDamageTime = gpGlobals->curtime;

	/*
		InSource
	*/

	// Si seguimos vivos y los efectos estan activados.
	if(IsAlive() && in_tired_effect.GetInt() == 1)
	{
		// Efectos - cansancio/muerte
		ConVarRef mat_yuv("mat_yuv");

		if(m_iHealth < 10 && mat_yuv.GetInt() == 0)
		{
			mat_yuv.SetValue(1);

			// �Estamos muriendo!
			EmitSound("Player.Music.Dying");
		}
	}

	/*
		InSource
	*/

	// �ltima vez que se ha sufrido da�o.
	m_flLastDamageTime = gpGlobals->curtime;

	return fTookDamage;
}

/*---------------------------------------------------
	OnDamagedByExplosion()
	Al recibir da�o por explosi�n.
---------------------------------------------------*/
void CBasePlayer::OnDamagedByExplosion(const CTakeDamageInfo &info)
{
	float lastDamage			= info.GetDamage();
	float distanceFromPlayer	= 9999.0f;

	CBaseEntity *inflictor		= info.GetInflictor();

	// �Quien causo la explosi�n?
	if (inflictor)
	{
		Vector delta		= GetAbsOrigin() - inflictor->GetAbsOrigin();
		distanceFromPlayer	= delta.Length();
	}

	// Calcular la distancia de la explosi�n y ver si es adecuado hacer el sonido de zumbido.
	bool ear_ringing		= (distanceFromPlayer < MIN_EAR_RINGING_DISTANCE) ? true : false;
	// Calcular si el da�o causado merece darle al jugador el efecto de confusi�n.
	bool shock				= (lastDamage >= MIN_SHOCK_AND_CONFUSION_DAMAGE);

	// Ning�n efecto.
	if (!shock && !ear_ringing)
		return;

	int effect = (shock) ? 
		random->RandomInt( 35, 37 ) : 
		random->RandomInt( 32, 34 );

	// �Zumbido de oido!
	CSingleUserRecipientFilter user(this);
	enginesound->SetPlayerDSP(user, effect, false);
}

/*---------------------------------------------------
	PackDeadPlayerItems()
	Eliminar objetos del jugador que ha acaba de morir.
---------------------------------------------------*/
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;

	CBaseCombatWeapon *rgpPackWeapons[20];
	int iPackAmmo[MAX_AMMO_SLOTS + 1];

	int iPW = 0;
	int iPA = 0;

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons));
	memset(iPackAmmo, -1, sizeof(iPackAmmo));

	iWeaponRules	= g_pGameRules->DeadPlayerWeapons(this);
 	iAmmoRules		= g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		RemoveAllItems(true);
		return;
	}

	for (i = 0 ; i < WeaponCount(); i++)
	{
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );

		if (pPlayerItem)
		{
			switch(iWeaponRules)
			{
				case GR_PLR_DROP_GUN_ACTIVE:
					if ( GetActiveWeapon() && pPlayerItem == GetActiveWeapon() )
						rgpPackWeapons[ iPW++ ] = pPlayerItem;
				break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[ iPW++ ] = pPlayerItem;
				break;

				default:
					break;
			}
		}
	}

	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0 ; i < MAX_AMMO_SLOTS ; i++)
		{
			if (GetAmmoCount(i) > 0)
			{
				switch ( iAmmoRules )
				{
					case GR_PLR_DROP_AMMO_ALL:
						iPackAmmo[ iPA++ ] = i;
					break;

					default:
					break;
				}
			}
		}
	}

	RemoveAllItems(true);
}

/*---------------------------------------------------
	RemoveAllItems()
	Eliminar todos los objetos del jugador.
---------------------------------------------------*/
void CBasePlayer::RemoveAllItems( bool removeSuit )
{
	// Hay una arma activa.
	if (GetActiveWeapon())
	{
		ResetAutoaim();
		GetActiveWeapon()->Holster();
	}

	// Resetear todo.
	Weapon_SetLast(NULL);
	RemoveAllWeapons();
 	RemoveAllAmmo();

	// Quitar tambi�n el traje de protecci�n.
	// Pues ya que, de una vez los calzones �no?
	if (removeSuit)
		RemoveSuit();

	UpdateClientData();
}

/*---------------------------------------------------
	IsDead()
	Obtener estado: �El jugador esta muerto?
---------------------------------------------------*/
bool CBasePlayer::IsDead() const
{
	return m_lifeState != LIFE_ALIVE;
}

/*---------------------------------------------------
	DamageForce()
	Calcular fuerza del da�o.
---------------------------------------------------*/
static float DamageForce(const Vector &size, float damage)
{ 
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * 5;
	
	// HULK? Nah
	if (force > 1000.0) 
		force = 1000.0;

	return force;
}

const impactdamagetable_t &CBasePlayer::GetPhysicsImpactDamageTable()
{
	return gDefaultPlayerImpactDamageTable;
}

/*---------------------------------------------------
	OnTakeDamage_Alive()
	Al recibir da�o y estar a�n vivo.
---------------------------------------------------*/
int CBasePlayer::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	m_bitsDamageType |= info.GetDamageType();

	if (!BaseClass::OnTakeDamage_Alive(info))
		return 0;

	CBaseEntity * attacker = info.GetAttacker();

	if (!attacker )
		return 0;

	Vector vecDir = vec3_origin;

	if (info.GetInflictor())
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}

	if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK) && 
		( !attacker->IsSolidFlagSet(FSOLID_TRIGGER)) )
	{
		Vector force = vecDir * -DamageForce(WorldAlignSize(), info.GetBaseDamage());

		if (force.z > 250.0f)
			force.z = 250.0f;

		ApplyAbsVelocityImpulse(force);
	}

	// Enviar evento �me he da�ado!
	IGameEvent * event = gameeventmanager->CreateEvent("player_hurt");

	// Enviar m�s informaci�n al evento. Multiplayer?
	if (event)
	{
		event->SetInt("userid", GetUserID());
		event->SetInt("health", max(0, m_iHealth));
		event->SetInt("priority", 5);

		if (attacker->IsPlayer())
		{
			CBasePlayer *player = ToBasePlayer(attacker);
			event->SetInt("attacker", player->GetUserID());
		}
		else
			event->SetInt("attacker", 0);

        gameeventmanager->FireEvent(event);
	}
	
	if (attacker->IsNPC())
		CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 512, 0.5, this);

	return 1;
}

/*---------------------------------------------------
	Event_Killed()
	Muerte del usuario.
---------------------------------------------------*/
void CBasePlayer::Event_Killed(const CTakeDamageInfo &info)
{
	CSound *pSound;

	if (Hints())
		Hints()->ResetHintTimers();

	// Ejecutar tarea de muerte en las reglas del juego.
	g_pGameRules->PlayerKilled(this, info);
	gamestats->Event_PlayerKilled(this, info);

	// Detener la vibraci�n del control (En caso de estar en una consola)
	RumbleEffect(RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE);
	ClearUseEntity();
	
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
			pSound->Reset();
	}

	// Evitar glitches con la barra de salud.
	if (m_iHealth < -99)
		m_iHealth = 0;

	// Soltar la arma activa.
	if (GetActiveWeapon())
		GetActiveWeapon()->Holster();
	
	// Activar animaci�n de muerte.
	SetAnimation(PLAYER_DIE);

	SetViewOffset(VEC_DEAD_VIEWHEIGHT);

	m_lifeState		= LIFE_DYING;

	pl.deadflag = true;
	AddSolidFlags(FSOLID_NOT_SOLID);

	IPhysicsObject *pObject = VPhysicsGetObject();

	if (pObject)
		pObject->RecheckContactPoints();

	SetMoveType(MOVETYPE_FLYGRAVITY);
	SetGroundEntity(NULL);

	// Resetear traje de protecci�n y FOV.
	SetSuitUpdate(NULL, false, 0);
	SetFOV(this, 0);
	
	// Apagar linterna.
	if (FlashlightIsOn())
		 FlashlightTurnOff();

	// Murio a las...
	m_flDeathTime = gpGlobals->curtime;

	if (m_lastNavArea)
	{
		m_lastNavArea->DecrementPlayerCount( GetTeamNumber() );
		m_lastNavArea = NULL;
	}

	BaseClass::Event_Killed(info);
}

/*---------------------------------------------------
	Event_Dying()
	El jugador esta agonizando.
---------------------------------------------------*/
void CBasePlayer::Event_Dying()
{
	CTakeDamageInfo info;
	DeathSound(info);

	if (IsInAVehicle())
		LeaveVehicle();

	QAngle angles = GetLocalAngles();

	angles.x = 0;
	angles.z = 0;
	
	SetLocalAngles(angles);

	SetThink(&CBasePlayer::PlayerDeathThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	BaseClass::Event_Dying();
}

/*---------------------------------------------------
	SetAnimation()
	Ajustar animaci�n del jugador o usuario.
---------------------------------------------------*/
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	char szAnim[64];
	float speed;

	speed = GetAbsVelocity().Length2D();

	if (GetFlags() & (FL_FROZEN|FL_ATCONTROLS))
	{
		speed		= 0;
		playerAnim	= PLAYER_IDLE;
	}

	Activity idealActivity = ACT_WALK;

	
	if (playerAnim == PLAYER_JUMP)
		idealActivity = ACT_HOP;

	else if (playerAnim == PLAYER_SUPERJUMP)
		idealActivity = ACT_LEAP;

	else if (playerAnim == PLAYER_DIE)
	{
		if ( m_lifeState == LIFE_ALIVE )
			idealActivity = GetDeathActivity();
	}

	else if (playerAnim == PLAYER_ATTACK1)
	{
		if ( m_Activity == ACT_HOVER	|| 
			 m_Activity == ACT_SWIM		||
			 m_Activity == ACT_HOP		||
			 m_Activity == ACT_LEAP		||
			 m_Activity == ACT_DIESIMPLE )
				idealActivity = m_Activity;

		else
			idealActivity = ACT_RANGE_ATTACK1;
	}

	else if (playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK)
	{
		// Al parecer esta en el aire.
		if (!( GetFlags() & FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP))
			idealActivity = m_Activity;

		// Esta en el agua.
		else if (GetWaterLevel() > 1)
		{
			if (speed == 0)
				idealActivity = ACT_HOVER;	// Flotando
			else
				idealActivity = ACT_SWIM;	// Nadando
		}

		else
			idealActivity = ACT_WALK;
	}
	
	if (idealActivity == ACT_RANGE_ATTACK1)
	{
		// Agachado.
		if (GetFlags() & FL_DUCKING)
			Q_strncpy(szAnim, "crouch_shoot_" ,sizeof(szAnim));

		else
			Q_strncpy(szAnim, "ref_shoot_" ,sizeof(szAnim));

		Q_strncat(szAnim, m_szAnimExtension ,sizeof(szAnim), COPY_ALL_CHARACTERS);
		animDesired = LookupSequence(szAnim);

		if (animDesired == -1)
			animDesired = 0;

		if (GetSequence() != animDesired || !SequenceLoops())
			SetCycle( 0 );

		SetActivity(idealActivity);
		ResetSequence(animDesired);
	}

	else if (idealActivity == ACT_WALK)
	{
		if (GetActivity() != ACT_RANGE_ATTACK1 || IsActivityFinished())
		{
			// Agachado.
			if (GetFlags() & FL_DUCKING)
				Q_strncpy(szAnim, "crouch_aim_" ,sizeof(szAnim));
			else
				Q_strncpy(szAnim, "ref_aim_" ,sizeof(szAnim));

			Q_strncat(szAnim, m_szAnimExtension,sizeof(szAnim), COPY_ALL_CHARACTERS);

			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			SetActivity(ACT_WALK);
		}
		else
			animDesired = GetSequence();
	}

	else
	{
		if (GetActivity() == idealActivity)
			return;
	
		SetActivity(idealActivity);
		animDesired = SelectWeightedSequence(m_Activity);

		if (GetSequence() == animDesired)
			return;

		ResetSequence(animDesired);
		SetCycle(0);
		return;
	}

	if (GetSequence() == animDesired)
		return;

	ResetSequence( animDesired );
	SetCycle(0);
}

/*---------------------------------------------------
	WaterMove()
	Estamos en el agua.
---------------------------------------------------*/
void CBasePlayer::WaterMove()
{
	if ((GetMoveType() == MOVETYPE_NOCLIP ) && !GetMoveParent())
	{
		m_AirFinished = gpGlobals->curtime + AIRTIME;
		return;
	}

	if (m_iHealth < 0 || !IsAlive())
	{
		UpdateUnderwaterState();
		return;
	}

	// El agua esta cubriendo al jugador.
	if (GetWaterLevel() != WL_Eyes || CanBreatheUnderwater()) 
	{
		// �Se nos termino el aire! Me ahogo...
		if (m_AirFinished < gpGlobals->curtime)
			EmitSound("Player.DrownStart");

		m_AirFinished	= gpGlobals->curtime + AIRTIME;
		m_nDrownDmgRate = DROWNING_DAMAGE_INITIAL;

		if (m_idrowndmg > m_idrownrestored)
		{
			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{

		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		// �Me estoy ahogando!
		if (m_AirFinished < gpGlobals->curtime && !(GetFlags() & FL_GODMODE))
		{
			// Quitarle la vida poco a poco...
			if (m_PainFinished < gpGlobals->curtime)
			{
				m_nDrownDmgRate += 1;

				if (m_nDrownDmgRate > DROWNING_DAMAGE_MAX)
					m_nDrownDmgRate = DROWNING_DAMAGE_MAX;

				OnTakeDamage(CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), m_nDrownDmgRate, DMG_DROWN));
				m_PainFinished = gpGlobals->curtime + 1;
				
				m_idrowndmg += m_nDrownDmgRate;
			} 
		}
		else
			m_bitsDamageType &= ~DMG_DROWN;
	}

	UpdateUnderwaterState();
}

/*---------------------------------------------------
	IsOnLadder()
	�Estamos en una escalera?
---------------------------------------------------*/
bool CBasePlayer::IsOnLadder(void)
{ 
	return (GetMoveType() == MOVETYPE_LADDER);
}

/*---------------------------------------------------
	GetWaterJumpTime()
	
---------------------------------------------------*/
float CBasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}

/*---------------------------------------------------
	SetWaterJumpTime()
	
---------------------------------------------------*/
void CBasePlayer::SetWaterJumpTime( float flWaterJumpTime )
{
	m_flWaterJumpTime = flWaterJumpTime;
}

/*---------------------------------------------------
	GetSwimSoundTime()
	
---------------------------------------------------*/
float CBasePlayer::GetSwimSoundTime( void ) const
{
	return m_flSwimSoundTime;
}

/*---------------------------------------------------
	SetSwimSoundTime()
	
---------------------------------------------------*/
void CBasePlayer::SetSwimSoundTime( float flSwimSoundTime )
{
	m_flSwimSoundTime = flSwimSoundTime;
}

/*---------------------------------------------------
	ShowViewPortPanel()
	
---------------------------------------------------*/
void CBasePlayer::ShowViewPortPanel( const char * name, bool bShow, KeyValues *data )
{
	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	int count			= 0;
	KeyValues *subkey	= NULL;

	if (data)
	{
		subkey = data->GetFirstSubKey();

		while (subkey)
		{
			count++; 
			subkey = subkey->GetNextKey();
		}

		subkey = data->GetFirstSubKey();
	}

	UserMessageBegin(filter, "VGUIMenu");
		WRITE_STRING(name);
		WRITE_BYTE(bShow?1:0);
		WRITE_BYTE(count);
		
		while (subkey)
		{
			WRITE_STRING( subkey->GetName() );
			WRITE_STRING( subkey->GetString() );
			subkey = subkey->GetNextKey();
		}
	MessageEnd();
}

/*---------------------------------------------------
	PlayerDeathThink()
	Ejecuci�n de tareas al morir.
---------------------------------------------------*/
void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;
	SetNextThink(gpGlobals->curtime + 0.1f);

	/*
		InSource
	*/
	
	// Camara lenta al morir.
	ConVarRef host_timescale("host_timescale");

	if(host_timescale.GetFloat() != 0.3)
		engine->ClientCommand(edict(), "host_timescale 0.3");

	/*
		InSource
	*/

	if (GetFlags() & FL_ONGROUND)
	{
		flForward = GetAbsVelocity().Length() - 20;

		if (flForward <= 0)
			SetAbsVelocity(vec3_origin);

		else
		{
			Vector vecNewVelocity = GetAbsVelocity();
			VectorNormalize( vecNewVelocity );
			vecNewVelocity *= flForward;
			SetAbsVelocity( vecNewVelocity );
		}
	}

	// Empacar las armas que teniamos.
	if (HasWeapons())
		PackDeadPlayerItems();

	if (GetModelIndex() && (!IsSequenceFinished()) && (m_lifeState == LIFE_DYING))
	{
		StudioFrameAdvance( );

		m_iRespawnFrames++;

		if (m_iRespawnFrames < 60)
			return;
	}

	if (m_lifeState == LIFE_DYING)
	{
		m_lifeState			= LIFE_DEAD;
		m_flDeathAnimTime	= gpGlobals->curtime;
	}
	
	// Detener cualquier animaci�n activa
	StopAnimation();

	AddEffects(EF_NOINTERP);
	m_flPlaybackRate = 0.0;
	
	int fAnyButtonDown = (m_nButtons & ~IN_SCORE);
	
	if ((fAnyButtonDown & IN_DUCK) && GetToggledDuckState())
		fAnyButtonDown &= ~IN_DUCK;

	if (m_lifeState == LIFE_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if ( g_pGameRules->FPlayerCanRespawn(this))
			m_lifeState = LIFE_RESPAWNABLE;
		
		return;
	}

	//if (g_pGameRules->IsMultiplayer() && ( gpGlobals->curtime > (m_flDeathTime + DEATH_ANIMATION_TIME) ) && !IsObserver())
	//	StartObserverMode(m_iObserverLastMode);
	
	// Volvemos a la vida �yey!
	if (!fAnyButtonDown 
		&& !( g_pGameRules->IsMultiplayer() && forcerespawn.GetInt() > 0 && (gpGlobals->curtime > (m_flDeathTime + 5))))
		return;

	/*
		InSource
	*/

	// Volver al tiempo real
	engine->ClientCommand(edict(), "host_timescale 1");

	/*
		InSource
	*/

	m_nButtons			= 0;
	m_iRespawnFrames	= 0;

	// Respawn
	respawn(this, !IsObserver());
	SetNextThink(TICK_NEVER_THINK);
}

/*---------------------------------------------------
	Observer Mode
	POR AHORA NO NECESARIO
---------------------------------------------------*/
void CBasePlayer::StopObserverMode()
{
	m_bForcedObserverMode = false;
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;

	if ( m_iObserverMode == OBS_MODE_NONE )
		return;

	if ( m_iObserverMode  > OBS_MODE_DEATHCAM )
	{
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode.Set( OBS_MODE_NONE );

	ShowViewPortPanel( "specmenu", false );
	ShowViewPortPanel( "specgui", false );
	ShowViewPortPanel( "overview", false );
}

bool CBasePlayer::StartObserverMode(int mode)
{
	if ( !IsObserver() )
	{
		// set position to last view offset
		SetAbsOrigin( GetAbsOrigin() + GetViewOffset() );
		SetViewOffset( vec3_origin );
	}

	Assert( mode > OBS_MODE_NONE );
	
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	// Holster weapon immediately, to allow it to cleanup
    if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	// clear out the suit message cache so we don't keep chattering
    SetSuitUpdate(NULL, FALSE, 0);

	SetGroundEntity( (CBaseEntity *)NULL );
	
	RemoveFlag( FL_DUCKING );
	
    AddSolidFlags( FSOLID_NOT_SOLID );

	SetObserverMode( mode );

	if ( gpGlobals->eLoadType != MapLoad_Background )
	{
		ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(mode) );
	}
	
	// Setup flags
    m_Local.m_iHideHUD = HIDEHUD_HEALTH;
	m_takedamage = DAMAGE_NO;		

	// Become invisible
	AddEffects( EF_NODRAW );		

	m_iHealth = 1;
	m_lifeState = LIFE_DEAD; // Can't be dead, otherwise movement doesn't work right.
	m_flDeathAnimTime = gpGlobals->curtime;
	pl.deadflag = true;

	return true;
}

bool CBasePlayer::SetObserverMode(int mode )
{
	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;


	// check mp_forcecamera settings for dead players
	if ( mode > OBS_MODE_FIXED && GetTeamNumber() > TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )
		{
			case OBS_ALLOW_ALL	:	break;	// no restrictions
			case OBS_ALLOW_TEAM :	mode = OBS_MODE_IN_EYE;	break;
			case OBS_ALLOW_NONE :	mode = OBS_MODE_FIXED; break;	// don't allow anything
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	
	switch ( mode )
	{
		case OBS_MODE_NONE:
		case OBS_MODE_FIXED :
		case OBS_MODE_DEATHCAM :
			SetFOV( this, 0 );	// Reset FOV
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			break;

		case OBS_MODE_CHASE :
		case OBS_MODE_IN_EYE :	
			// udpate FOV and viewmodels
			SetObserverTarget( m_hObserverTarget );	
			SetMoveType( MOVETYPE_OBSERVER );
			break;
			
		case OBS_MODE_ROAMING :
			SetFOV( this, 0 );	// Reset FOV
			SetObserverTarget( m_hObserverTarget );
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_OBSERVER );
			break;

	}

	CheckObserverSettings();

	return true;	
}

int CBasePlayer::GetObserverMode()
{
	return m_iObserverMode;
}

void CBasePlayer::ForceObserverMode(int mode)
{
	int tempMode = OBS_MODE_ROAMING;

	if ( m_iObserverMode == mode )
		return;

	// don't change last mode if already in forced mode

	if ( m_bForcedObserverMode )
	{
		tempMode = m_iObserverLastMode;
	}
	
	SetObserverMode( mode );

	if ( m_bForcedObserverMode )
	{
		m_iObserverLastMode = tempMode;
	}

	m_bForcedObserverMode = true;
}

void CBasePlayer::CheckObserverSettings()
{
	// check if we are in forced mode and may go back to old mode
	if ( m_bForcedObserverMode )
	{
		CBaseEntity * target = m_hObserverTarget;

		if ( !IsValidObserverTarget(target) )
		{
			// if old target is still invalid, try to find valid one
			target = FindNextObserverTarget( false );
		}

		if ( target )
		{
				// we found a valid target
				m_bForcedObserverMode = false;	// disable force mode
				SetObserverMode( m_iObserverLastMode ); // switch to last mode
				SetObserverTarget( target ); // goto target
				
				// TODO check for HUD icons
				return;
		}
		else
		{
			// else stay in forced mode, no changes
			return;
		}
	}

	// make sure our last mode is valid
	if ( m_iObserverLastMode < OBS_MODE_FIXED )
	{
		m_iObserverLastMode = OBS_MODE_ROAMING;
	}

	// check if our spectating target is still a valid one
	
	if (  m_iObserverMode == OBS_MODE_IN_EYE || m_iObserverMode == OBS_MODE_CHASE || m_iObserverMode == OBS_MODE_FIXED )
	{
		ValidateCurrentObserverTarget();
				
		CBasePlayer *target = ToBasePlayer( m_hObserverTarget.Get() );

		// for ineye mode we have to copy several data to see exactly the same 

		if ( target && m_iObserverMode == OBS_MODE_IN_EYE )
		{
			int flagMask =	FL_ONGROUND | FL_DUCKING ;

			int flags = target->GetFlags() & flagMask;

			if ( (GetFlags() & flagMask) != flags )
			{
				flags |= GetFlags() & (~flagMask); // keep other flags
				ClearFlags();
				AddFlag( flags );
			}

			if ( target->GetViewOffset() != GetViewOffset()	)
			{
				SetViewOffset( target->GetViewOffset() );
			}
		}

		// Update the fog.
		if ( target )
		{
			if ( target->m_Local.m_PlayerFog.m_hCtrl.Get() != m_Local.m_PlayerFog.m_hCtrl.Get() )
			{
				m_Local.m_PlayerFog.m_hCtrl.Set( target->m_Local.m_PlayerFog.m_hCtrl.Get() );
			}
		}
	}
}

void CBasePlayer::ValidateCurrentObserverTarget( void )
{
	if ( !IsValidObserverTarget( m_hObserverTarget.Get() ) )
	{
		// our target is not valid, try to find new target
		CBaseEntity * target = FindNextObserverTarget( false );
		if ( target )
		{
			// switch to new valid target
			SetObserverTarget( target );	
		}
		else
		{
			// couldn't find new target, switch to temporary mode
			if ( mp_forcecamera.GetInt() == OBS_ALLOW_ALL )
			{
				// let player roam around
				ForceObserverMode( OBS_MODE_ROAMING );
			}
			else
			{
				// fix player view right where it is
				ForceObserverMode( OBS_MODE_FIXED );
				m_hObserverTarget.Set( NULL ); // no traget to follow
			}
		}
	}
}

void CBasePlayer::AttemptToExitFreezeCam( void )
{
	StartObserverMode( OBS_MODE_DEATHCAM );
}

bool CBasePlayer::StartReplayMode( float fDelay, float fDuration, int iEntity )
{
	if ( ( sv_maxreplay == NULL ) || ( sv_maxreplay->GetFloat() <= 0 ) )
		return false;

	m_fDelay = fDelay;
	m_fReplayEnd = gpGlobals->curtime + fDuration;
	m_iReplayEntity = iEntity;

	return true;
}

void CBasePlayer::StopReplayMode()
{
	m_fDelay = 0.0f;
	m_fReplayEnd = -1;
	m_iReplayEntity = 0;
}

int	CBasePlayer::GetDelayTicks()
{
	if ( m_fReplayEnd > gpGlobals->curtime )
	{
		return TIME_TO_TICKS( m_fDelay );
	}
	else
	{
		if ( m_fDelay > 0.0f )
			StopReplayMode();

		return 0;
	}
}

int CBasePlayer::GetReplayEntity()
{
	return m_iReplayEntity;
}

CBaseEntity * CBasePlayer::GetObserverTarget()
{
	return m_hObserverTarget.Get();
}

void CBasePlayer::ObserverUse( bool bIsPressed )
{
#ifndef _XBOX
	if ( !HLTVDirector()->IsActive() )	
		return;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
		return;	// only pure spectators can play cameraman

	if ( !bIsPressed )
		return;

	int iCameraManIndex = HLTVDirector()->GetCameraMan();

	if ( iCameraManIndex == 0 )
	{
		// turn camera on
		HLTVDirector()->SetCameraMan( entindex() );
	}
	else if ( iCameraManIndex == entindex() )
	{
		// turn camera off
		HLTVDirector()->SetCameraMan( 0 );
	}
	else
	{
		ClientPrint( this, HUD_PRINTTALK, "Camera in use by other player." );	
	}
#endif
}

void CBasePlayer::JumptoPosition(const Vector &origin, const QAngle &angles)
{
	SetAbsOrigin( origin );
	SetAbsVelocity( vec3_origin );	// stop movement
	SetLocalAngles( angles );
	SnapEyeAngles( angles );
}

bool CBasePlayer::SetObserverTarget(CBaseEntity *target)
{
	if ( !IsValidObserverTarget( target ) )
		return false;
	
	// set new target
	m_hObserverTarget.Set( target ); 

	// reset fov to default
	SetFOV( this, 0 );	
	
	if ( m_iObserverMode == OBS_MODE_ROAMING )
	{
		Vector	dir, end;
		Vector	start = target->EyePosition();
		
		AngleVectors( target->EyeAngles(), &dir );
		VectorNormalize( dir );
		VectorMA( start, -64.0f, dir, end );

		Ray_t ray;
		ray.Init( start, end, VEC_DUCK_HULL_MIN	, VEC_DUCK_HULL_MAX );

		trace_t	tr;
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, target, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

		JumptoPosition( tr.endpos, target->EyeAngles() );
	}
	
	return true;
}

bool CBasePlayer::IsValidObserverTarget(CBaseEntity * target)
{
	if ( target == NULL )
		return false;

	// MOD AUTHORS: Add checks on target here or in derived method

	if ( !target->IsPlayer() )	// only track players
		return false;

	CBasePlayer * player = ToBasePlayer( target );


	if( player == this )
		return false; // We can't observe ourselves.

	if ( player->IsEffectActive( EF_NODRAW ) ) // don't watch invisible players
		return false;

	if ( player->m_lifeState == LIFE_RESPAWNABLE ) // target is dead, waiting for respawn
		return false;

	if ( player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING )
	{
		if ( (player->m_flDeathTime + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
		{
			return false;	// allow watching until 3 seconds after death to see death animation
		}
	}
		
	// check forcecamera settings for active players
	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )	
		{
			case OBS_ALLOW_ALL	:	break;
			case OBS_ALLOW_TEAM :	if ( GetTeamNumber() != target->GetTeamNumber() )
										 return false;
									break;
			case OBS_ALLOW_NONE :	return false;
		}
	}
	
	return true;	// passed all test
}

int CBasePlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 

	int startIndex;

	if ( m_hObserverTarget )
	{
		// start using last followed player
		startIndex = m_hObserverTarget->entindex();	
	}
	else
	{
		// start using own player index
		startIndex = this->entindex();
	}

	startIndex += iDir;
	if (startIndex > gpGlobals->maxClients)
		startIndex = 1;
	else if (startIndex < 1)
		startIndex = gpGlobals->maxClients;

	return startIndex;
}

CBaseEntity * CBasePlayer::FindNextObserverTarget(bool bReverse)
{

	int startIndex = GetNextObserverSearchStartPoint( bReverse );
	
	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 
	
	do
	{
		CBaseEntity * nextTarget = UTIL_PlayerByIndex( currentIndex );

		if ( IsValidObserverTarget( nextTarget ) )
		{
			return nextTarget;	// found next valid player
		}

		currentIndex += iDir;

		// Loop through the clients
  		if (currentIndex > gpGlobals->maxClients)
  			currentIndex = 1;
		else if (currentIndex < 1)
  			currentIndex = gpGlobals->maxClients;

	} while ( currentIndex != startIndex );
		
	return NULL;
}

/*---------------------------------------------------
	Observer Mode
	POR AHORA NO NECESARIO
---------------------------------------------------*/

/*---------------------------------------------------
	IsUseableEntity()
	�El objeto puede ser usado por el jugador?
---------------------------------------------------*/
bool CBasePlayer::IsUseableEntity(CBaseEntity *pEntity, unsigned int requiredCaps)
{
	// Debe ser un objeto v�lido
	if (pEntity == NULL)
		return false;

	int caps = pEntity->ObjectCaps();

	if (caps & (FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE|FCAP_DIRECTIONAL_USE))
	{
		if ((caps & requiredCaps) == requiredCaps)
			return true;
	}

	return false;
}


/*---------------------------------------------------
	CanPickupObject()
	�El objeto se puede recojer?
---------------------------------------------------*/
bool CBasePlayer::CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit )
{
	#ifdef HL2_DLL

		// Debe ser un objeto v�lido
		if (pObject == NULL)
			return false;

		// Debe movible por las f�sicas
		if (pObject->GetMoveType() != MOVETYPE_VPHYSICS)
			return false;

		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = pObject->VPhysicsGetObjectList(pList, ARRAYSIZE(pList));

		if (!count)
			return false;

		float objectMass = 0;
		bool checkEnable = false;

		for (int i = 0; i < count; i++)
		{
			objectMass += pList[i]->GetMass();

			if (!pList[i]->IsMoveable())
				checkEnable = true;

			if (pList[i]->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP)
				return false;

			if (pList[i]->IsHinged())
				return false;
		}

		if (massLimit > 0 && objectMass > massLimit)
			return false;

		if (checkEnable)
		{
			CBounceBomb *pBomb = dynamic_cast<CBounceBomb*>(pObject);

			if(pBomb)
				return true;

			// Permitir si es un objeto con f�sicas o esta establecido que se puede recojer
			CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>(pObject);
			CPhysBox *pBox		= dynamic_cast<CPhysBox*>(pObject);

			if (!pProp && !pBox)
				return false;

			if (pProp && !(pProp->HasSpawnFlags(SF_PHYSPROP_ENABLE_ON_PHYSCANNON)))
				return false;

			if (pBox && !(pBox->HasSpawnFlags(SF_PHYSBOX_ENABLE_ON_PHYSCANNON)))
				return false;
		}

		if (sizeLimit > 0)
		{
			const Vector &size = pObject->CollisionProp()->OBBSize();

			if (size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit)
				return false;
		}

		return true;

	#else
		return false;
	#endif
}

/*---------------------------------------------------
	GetHeldObjectMass()
	
---------------------------------------------------*/
float CBasePlayer::GetHeldObjectMass( IPhysicsObject *pHeldObject )
{
	return 0;
}

/*---------------------------------------------------
	GetHeldObject()
	
---------------------------------------------------*/
CBaseEntity	*CBasePlayer::GetHeldObject( void )
{
	return NULL;
}


/*---------------------------------------------------
	Jump()
	Saltar
---------------------------------------------------*/
void CBasePlayer::Jump()
{
}

/*---------------------------------------------------
	Duck()
	Agachar
---------------------------------------------------*/
void CBasePlayer::Duck()
{
	if (m_nButtons & IN_DUCK) 
	{
		if (m_Activity != ACT_LEAP)
			SetAnimation( PLAYER_WALK );
	}
}

/*---------------------------------------------------
	Classify
---------------------------------------------------*/
Class_T  CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}

/*---------------------------------------------------
	ResetFragCount()
---------------------------------------------------*/
void CBasePlayer::ResetFragCount()
{
	m_iFrags = 0;
	pl.frags = m_iFrags;
}

/*---------------------------------------------------
	IncrementFragCount()
---------------------------------------------------*/
void CBasePlayer::IncrementFragCount( int nCount )
{
	m_iFrags += nCount;
	pl.frags = m_iFrags;
}

/*---------------------------------------------------
	ResetDeathCount()
	Resetear contador de muertes.
---------------------------------------------------*/
void CBasePlayer::ResetDeathCount()
{
	m_iDeaths = 0;
	pl.deaths = m_iDeaths;
}

/*---------------------------------------------------
	IncrementDeathCount()
	Incrementar una muerte al contador.
---------------------------------------------------*/
void CBasePlayer::IncrementDeathCount(int nCount)
{
	m_iDeaths += nCount;
	pl.deaths = m_iDeaths;
}

/*---------------------------------------------------
	AddPoints()
	Agregar puntos.
---------------------------------------------------*/
void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore)
{
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (m_iFrags < 0)	
				return;
			
			if (-score > m_iFrags)
				score = -m_iFrags;
		}
	}

	m_iFrags += score;
	pl.frags = m_iFrags;
}

/*---------------------------------------------------
	AddPointsToTeam()
	Agregar puntos al equipo.
---------------------------------------------------*/
void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore)
{
	if (GetTeam())
		GetTeam()->AddScore(score);
}

/*---------------------------------------------------
	GetCommandContextCount()
---------------------------------------------------*/
int CBasePlayer::GetCommandContextCount(void) const
{
	return m_CommandContext.Count();
}

/*---------------------------------------------------
	GetCommandContext()
---------------------------------------------------*/
CCommandContext *CBasePlayer::GetCommandContext(int index)
{
	if (index < 0 || index >= m_CommandContext.Count())
		return NULL;

	return &m_CommandContext[ index ];
}

/*---------------------------------------------------
	AllocCommandContext()
---------------------------------------------------*/
CCommandContext	*CBasePlayer::AllocCommandContext(void)
{
	int idx = m_CommandContext.AddToTail();

	if (m_CommandContext.Count() > 1000)
		Assert( 0 );

	return &m_CommandContext[ idx ];
}

/*---------------------------------------------------
	RemoveCommandContext()
---------------------------------------------------*/
void CBasePlayer::RemoveCommandContext(int index)
{
	m_CommandContext.Remove(index);
}

/*---------------------------------------------------
	RemoveAllCommandContexts()
---------------------------------------------------*/
void CBasePlayer::RemoveAllCommandContexts()
{
	m_CommandContext.RemoveAll();
}

/*---------------------------------------------------
	RemoveAllCommandContextsExceptNewest()
---------------------------------------------------*/
CCommandContext *CBasePlayer::RemoveAllCommandContextsExceptNewest(void)
{
	int count		= m_CommandContext.Count();
	int toRemove	= count - 1;

	if (toRemove > 0)
		m_CommandContext.RemoveMultiple( 0, toRemove );

	if (!m_CommandContext.Count())
	{
		Assert(0);
		CCommandContext *ctx = AllocCommandContext();
		Q_memset(ctx, 0, sizeof(*ctx));
	}

	return &m_CommandContext[ 0 ];
}

/*---------------------------------------------------
	ReplaceContextCommands()
---------------------------------------------------*/
void CBasePlayer::ReplaceContextCommands(CCommandContext *ctx, CUserCmd *pCommands, int nCommands)
{
	ctx->cmds.RemoveAll();

	ctx->numcmds			= nCommands;
	ctx->totalcmds			= nCommands;
	ctx->dropped_packets	= 0;

	for (int i = nCommands - 1; i >= 0; --i)
		ctx->cmds.AddToTail(pCommands[i]);
}

/*---------------------------------------------------
	DetermineSimulationTicks()
	Determina cuanto tiempo debe permanecer el Frame
---------------------------------------------------*/
int CBasePlayer::DetermineSimulationTicks(void)
{
	int command_context_count	= GetCommandContextCount();
	int simulation_ticks		= 0;
	int context_number;

	for (context_number = 0; context_number < command_context_count; context_number++)
	{
		CCommandContext const *ctx = GetCommandContext(context_number);

		Assert(ctx);
		Assert(ctx->numcmds > 0);
		Assert(ctx->dropped_packets >= 0);

		simulation_ticks += ctx->numcmds + ctx->dropped_packets;
	}

	return simulation_ticks;
}


/*---------------------------------------------------
	AdjustPlayerTimeBase()
	Ajusta el tiempo y el frame del jugador con los del servidor.
---------------------------------------------------*/
void CBasePlayer::AdjustPlayerTimeBase(int simulation_ticks)
{
	Assert(simulation_ticks >= 0);

	if (simulation_ticks < 0)
		return;

	CPlayerSimInfo *pi = NULL;

	if (sv_playerperfhistorycount.GetInt() > 0)
	{
		while (m_vecPlayerSimInfo.Count() > sv_playerperfhistorycount.GetInt())
			m_vecPlayerSimInfo.Remove(m_vecPlayerSimInfo.Head());

		pi = &m_vecPlayerSimInfo[ m_vecPlayerSimInfo.AddToTail() ];
	}

	if (gpGlobals->maxClients == 1)
		m_nTickBase = gpGlobals->tickcount - simulation_ticks + gpGlobals->simTicksThisFrame;
	
	else
	{
		float flCorrectionSeconds	= clamp(sv_clockcorrection_msecs.GetFloat() / 1000.0f, 0.0f, 1.0f);
		int nCorrectionTicks		= TIME_TO_TICKS(flCorrectionSeconds);

		int	nIdealFinalTick			= gpGlobals->tickcount + nCorrectionTicks;
		int nEstimatedFinalTick		= m_nTickBase + simulation_ticks;
		
		int	 too_fast_limit = nIdealFinalTick + nCorrectionTicks;
		int	 too_slow_limit = nIdealFinalTick - nCorrectionTicks;
			
		if ( nEstimatedFinalTick > too_fast_limit ||
			 nEstimatedFinalTick < too_slow_limit )
		{
			int nCorrectedTick = nIdealFinalTick - simulation_ticks + gpGlobals->simTicksThisFrame;

			if (pi)
				pi->m_nTicksCorrected = nCorrectionTicks;

			m_nTickBase = nCorrectedTick;
		}
	}

	if (pi)
		pi->m_flFinalSimulationTime = TICKS_TO_TIME(m_nTickBase + simulation_ticks + gpGlobals->simTicksThisFrame);
}

/*---------------------------------------------------
	RunNullCommand()
	Ejecuta un comando nulo.
---------------------------------------------------*/
void CBasePlayer::RunNullCommand()
{
	CUserCmd cmd;

	float flOldFrametime	= gpGlobals->frametime;
	float flOldCurtime		= gpGlobals->curtime;
	float flTimeBase		= gpGlobals->curtime;

	pl.fixangle		= FIXANGLE_NONE;
	cmd.viewangles	= EyeAngles();

	SetTimeBase(flTimeBase);

	MoveHelperServer()->SetHost(this);
	PlayerRunCommand(&cmd, MoveHelperServer());

	SetLastUserCommand(cmd);

	gpGlobals->frametime	= flOldFrametime;
	gpGlobals->curtime		= flOldCurtime;

	MoveHelperServer()->SetHost(NULL);
}

/*---------------------------------------------------
	PhysicsSimulate()
	Simulaci�n de f�sicas
---------------------------------------------------*/
void CBasePlayer::PhysicsSimulate( void )
{
	VPROF_BUDGET("CBasePlayer::PhysicsSimulate", VPROF_BUDGETGROUP_PLAYER);

	// Hay un movimiento padre
	CBaseEntity *pMoveParent = GetMoveParent();

	if (pMoveParent)
		pMoveParent->PhysicsSimulate();

	// No hay que simular dos veces por frame.
	if ( m_nSimulationTick == gpGlobals->tickcount )
		return;
	
	m_nSimulationTick = gpGlobals->tickcount;

	int simulation_ticks = DetermineSimulationTicks();

	if (simulation_ticks > 0)
		AdjustPlayerTimeBase(simulation_ticks);

	if (IsHLTV())
	{
		Assert (GetCommandContextCount() == 0);
		RunNullCommand();
		RemoveAllCommandContexts();
		return;
	}

	// Guardar fuera el tiempo real del servidor.
	float savetime		= gpGlobals->curtime;
	float saveframetime = gpGlobals->frametime;

	int command_context_count = GetCommandContextCount();	

	CUtlVector< CUserCmd >	vecAvailCommands;

	for (int context_number = 0; context_number < command_context_count; context_number++)
	{
		CCommandContext *ctx = GetCommandContext(context_number);

		if (!ShouldRunCommandsInContext(ctx))
			continue;

		if (!ctx->cmds.Count())
			continue;

		int numbackup = ctx->totalcmds - ctx->numcmds;

		if (ctx->dropped_packets < 24)                
		{
			int droppedcmds = ctx->dropped_packets;

			while (droppedcmds > numbackup)
			{
				m_LastCmd.tick_count++;
				vecAvailCommands.AddToTail( m_LastCmd );
				droppedcmds--;
			}

			while (droppedcmds > 0)
			{
				int cmdnum = ctx->numcmds + droppedcmds - 1;
				vecAvailCommands.AddToTail( ctx->cmds[cmdnum] );
				droppedcmds--;
			}
		}

		for (int i = ctx->numcmds - 1; i >= 0; i--)
			vecAvailCommands.AddToTail( ctx->cmds[i] );

		m_LastCmd = ctx->cmds[ CMD_MOSTRECENT ];
	}

	int commandLimit	= CBaseEntity::IsSimulatingOnAlternateTicks() ? 2 : 1;
	int commandsToRun	= vecAvailCommands.Count();

	if (gpGlobals->simTicksThisFrame >= commandLimit && vecAvailCommands.Count() > commandLimit)
	{
		int commandsToRollOver	= min(vecAvailCommands.Count(), ( gpGlobals->simTicksThisFrame - 1 ));
		commandsToRun			= vecAvailCommands.Count() - commandsToRollOver;

		Assert(commandsToRun >= 0);
		
		if (commandsToRollOver > 0)
		{
			CCommandContext *ctx = RemoveAllCommandContextsExceptNewest();
			ReplaceContextCommands( ctx, &vecAvailCommands[ commandsToRun ], commandsToRollOver );
		}
		else
			RemoveAllCommandContexts();
	}
	else
		RemoveAllCommandContexts();

	float vphysicsArrivalTime = TICK_INTERVAL;

	if (commandsToRun > 0)
	{
		MoveHelperServer()->SetHost(this);

		if (IsPredictingWeapons())
			IPredictionSystem::SuppressHostEvents(this);

		for (int i = 0; i < commandsToRun; ++i)
		{
			PlayerRunCommand(&vecAvailCommands[ i ], MoveHelperServer());

			if (m_pPhysicsController)
			{
				VPROF("CBasePlayer::PhysicsSimulate-UpdateVPhysicsPosition");

				UpdateVPhysicsPosition( m_vNewVPhysicsPosition, m_vNewVPhysicsVelocity, vphysicsArrivalTime );
				vphysicsArrivalTime += TICK_INTERVAL;
			}
		}

		IPredictionSystem::SuppressHostEvents(NULL);
		MoveHelperServer()->SetHost(NULL);

		CPlayerSimInfo *pi = NULL;

		if (m_vecPlayerSimInfo.Count() > 0)
		{
			pi							= &m_vecPlayerSimInfo[ m_vecPlayerSimInfo.Tail() ];
			pi->m_flTime				= gpGlobals->realtime;
			pi->m_vecAbsOrigin			= GetAbsOrigin();
			pi->m_flGameSimulationTime	= gpGlobals->curtime;
			pi->m_nNumCmds				= commandsToRun;
		}
	}

	gpGlobals->curtime		= savetime;
	gpGlobals->frametime	= saveframetime;	
}

/*---------------------------------------------------
	PhysicsSolidMaskForEntity()
---------------------------------------------------*/
unsigned int CBasePlayer::PhysicsSolidMaskForEntity() const
{
	return MASK_PLAYERSOLID;
}

/*---------------------------------------------------
	ForceSimulation()
	This will force usercmd processing to actually consume commands even if the global tick counter isn't incrementing
---------------------------------------------------*/
void CBasePlayer::ForceSimulation()
{
	m_nSimulationTick = -1;
}

/*---------------------------------------------------
	ProcessUsercmds()
---------------------------------------------------*/
void CBasePlayer::ProcessUsercmds(CUserCmd *cmds, int numcmds, int totalcmds,
	int dropped_packets, bool paused)
{
	CCommandContext *ctx = AllocCommandContext();
	Assert(ctx);

	int i;
	for (i = totalcmds - 1; i >= 0; i--)
		ctx->cmds.AddToTail(cmds[ totalcmds - 1 - i ]);

	ctx->numcmds			= numcmds;
	ctx->totalcmds			= totalcmds,
	ctx->dropped_packets	= dropped_packets;
	ctx->paused				= paused;
		
	if (ctx->paused)
	{
		bool clear_angles = true;

		if ( GetMoveType() == MOVETYPE_NOCLIP &&
			sv_cheats->GetBool() && 
			sv_noclipduringpause.GetBool() )
		{
			clear_angles = false;
		}

		for (i = 0; i < ctx->numcmds; i++)
		{
			ctx->cmds[ i ].buttons = 0;

			if (clear_angles)
			{
				ctx->cmds[ i ].forwardmove	= 0;
				ctx->cmds[ i ].sidemove		= 0;
				ctx->cmds[ i ].upmove		= 0;

				VectorCopy (pl.v_angle, ctx->cmds[ i ].viewangles);
			}
		}

		ctx->dropped_packets = 0;
	}

	m_bGamePaused = paused;

	if (paused)
	{
		ForceSimulation();
		PhysicsSimulate();
	}

	if (sv_playerperfhistorycount.GetInt() > 0)
	{
		CPlayerCmdInfo pi;

		pi.m_flTime				= gpGlobals->realtime;
		pi.m_nDroppedPackets	= dropped_packets;
		pi.m_nNumCmds			= numcmds;
	
		while (m_vecPlayerCmdInfo.Count() >= sv_playerperfhistorycount.GetInt())
			m_vecPlayerCmdInfo.Remove( m_vecPlayerCmdInfo.Head() );

		m_vecPlayerCmdInfo.AddToTail(pi);
	}
}

/*---------------------------------------------------
	DumpPerfToRecipient()
---------------------------------------------------*/
void CBasePlayer::DumpPerfToRecipient(CBasePlayer *pRecipient, int nMaxRecords)
{
	if (!pRecipient)
		return;

	char buf[ 256 ] = { 0 };
	int curpos		= 0;

	int nDumped = 0;
	float prevt = 0.0f;

	Vector prevo( 0, 0, 0 );

	for (int i = m_vecPlayerSimInfo.Tail(); i != m_vecPlayerSimInfo.InvalidIndex() ; i = m_vecPlayerSimInfo.Previous(i))
	{
		const CPlayerSimInfo *pi = &m_vecPlayerSimInfo[ i ];

		float vel = 0.0f;
		float dt = prevt - pi->m_flFinalSimulationTime;

		if (nDumped > 0 && dt > 0.0f)
		{
			Vector d	= pi->m_vecAbsOrigin - prevo;
			vel			= d.Length() / dt;
		}

		char line[ 128 ];

		int len = Q_snprintf( line, sizeof( line ), "%.3f %d %d %.3f %.3f %.3f [vel %.2f]\n",
			pi->m_flTime,
			pi->m_nNumCmds,
			pi->m_nTicksCorrected,
			pi->m_flFinalSimulationTime,
			pi->m_flGameSimulationTime,
			pi->m_flServerFrameTime, 
			vel );

		if (curpos + len > 200)
		{
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, (char const *)buf);
			buf[ 0 ]	= 0;
			curpos		= 0;
		}

		Q_strncpy(&buf[ curpos ], line, sizeof( buf ) - curpos);
		curpos += len;

		++nDumped;

		if (nMaxRecords != -1 && nDumped >= nMaxRecords)
			break;

		prevo = pi->m_vecAbsOrigin;
		prevt = pi->m_flFinalSimulationTime;
	}

	if (curpos > 0)
		ClientPrint(pRecipient, HUD_PRINTCONSOLE, buf);

	nDumped = 0;
	curpos	= 0;

	for (int i = m_vecPlayerCmdInfo.Tail(); i != m_vecPlayerCmdInfo.InvalidIndex() ; i = m_vecPlayerCmdInfo.Previous(i))
	{
		const CPlayerCmdInfo *pi = &m_vecPlayerCmdInfo[i];
		char line[ 128 ];

		int len = Q_snprintf( line, sizeof( line ), "%.3f %d %d\n",
			pi->m_flTime,
			pi->m_nNumCmds,
			pi->m_nDroppedPackets );

		if (curpos + len > 200)
		{
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, (char const *)buf);

			buf[ 0 ]	= 0;
			curpos		= 0;
		}

		Q_strncpy(&buf[ curpos ], line, sizeof( buf ) - curpos);
		curpos += len;

		++nDumped;

		if (nMaxRecords != -1 && nDumped >= nMaxRecords)
			break;
	}

	if (curpos > 0)
		ClientPrint(pRecipient, HUD_PRINTCONSOLE, buf);
}

/*---------------------------------------------------
	PlayerRunCommand()
---------------------------------------------------*/
void CBasePlayer::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	m_touchedPhysObject = false;

	if (pl.fixangle == FIXANGLE_NONE)
		VectorCopy (ucmd->viewangles, pl.v_angle);

	if(GetFlags() & FL_FROZEN || 
		(developer.GetInt() == 0 && gpGlobals->eLoadType == MapLoad_NewGame && gpGlobals->curtime < 3.0 ))
	{
		ucmd->forwardmove	= 0;
		ucmd->sidemove		= 0;
		ucmd->upmove		= 0;
		ucmd->buttons		= 0;
		ucmd->impulse		= 0;

		VectorCopy (pl.v_angle, ucmd->viewangles);
	}

	else
	{
		if (GetToggledDuckState())
		{
			if (xc_crouch_debounce.GetBool())
			{
				ToggleDuck();
				xc_crouch_debounce.SetValue(0);
			}
			else
				ucmd->buttons |= IN_DUCK;
		}
	}
	
	PlayerMove()->RunCommand(this, ucmd, moveHelper);
}

/*---------------------------------------------------
	DisableButtons()
	Desactivar ciertos botones del control del usuario.
---------------------------------------------------*/
void CBasePlayer::DisableButtons(int nButtons)
{
	m_afButtonDisabled |= nButtons;
}

/*---------------------------------------------------
	EnableButtons()
	Activa ciertos botones del control del usuario.
---------------------------------------------------*/
void CBasePlayer::EnableButtons(int nButtons)
{
	m_afButtonDisabled &= ~nButtons;
}

/*---------------------------------------------------
	ForceButtons()
	Desactivar ciertos botones del control del usuario.
---------------------------------------------------*/
void CBasePlayer::ForceButtons( int nButtons )
{
	m_afButtonForced |= nButtons;
}

/*---------------------------------------------------
	UnforceButtons()
	Activa ciertos botones del control del usuario.
---------------------------------------------------*/
void CBasePlayer::UnforceButtons( int nButtons )
{
	m_afButtonForced &= ~nButtons;
}

/*---------------------------------------------------
	HandleFuncTrain()
---------------------------------------------------*/
void CBasePlayer::HandleFuncTrain(void)
{
	if (m_afPhysicsFlags & PFLAG_DIROVERRIDE)
		AddFlag(FL_ONTRAIN);
	else 
		RemoveFlag(FL_ONTRAIN);

	if ((m_afPhysicsFlags & PFLAG_DIROVERRIDE) == 0)
	{
		if (m_iTrain & TRAIN_ACTIVE)
			m_iTrain = TRAIN_NEW;

		return;
	}

	CBaseEntity *pTrain = GetGroundEntity();
	float vel;

	if (pTrain)
	{
		if (!(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE))
			pTrain = NULL;
	}
	
	if (!pTrain)
	{
		if (GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE)
		{
			m_iTrain = TRAIN_ACTIVE | TRAIN_NEW;

			if ( m_nButtons & IN_FORWARD )
				m_iTrain |= TRAIN_FAST;

			else if ( m_nButtons & IN_BACK )
				m_iTrain |= TRAIN_BACK;

			else
				m_iTrain |= TRAIN_NEUTRAL;

			return;
		}
		else
		{
			trace_t trainTrace;

			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-38), 
				MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace );

			if (trainTrace.fraction != 1.0 && trainTrace.m_pEnt)
				pTrain = trainTrace.m_pEnt;

			if (!pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(this))
			{
				m_afPhysicsFlags	&= ~PFLAG_DIROVERRIDE;
				m_iTrain			= TRAIN_NEW|TRAIN_OFF;

				return;
			}
		}
	}

	else if (!(GetFlags() & FL_ONGROUND) || pTrain->HasSpawnFlags( SF_TRACKTRAIN_NOCONTROL ) || (m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT)))
	{
		m_afPhysicsFlags	&= ~PFLAG_DIROVERRIDE;
		m_iTrain			= TRAIN_NEW|TRAIN_OFF;

		return;
	}

	SetAbsVelocity(vec3_origin);
	vel = 0;

	if (m_afButtonPressed & IN_FORWARD)
	{
		vel = 1;
		pTrain->Use(this, this, USE_SET, (float)vel);
	}

	else if (m_afButtonPressed & IN_BACK)
	{
		vel = -1;
		pTrain->Use( this, this, USE_SET, (float)vel );
	}

	if (vel)
	{
		m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
		m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
	}
}

/*---------------------------------------------------
	CheckTimeBasedDamage()
	Calcular y checar el tiempo de da�o.
---------------------------------------------------*/
void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	byte bDuration			= 0;
	static float gtbdPrev	= 0.0;

	if (!g_pGameRules->Damage_IsTimeBased(m_bitsDamageType))
		return;

	if (abs(gpGlobals->curtime - m_tbdPrev ) < 2.0)
		return;
	
	m_tbdPrev = gpGlobals->curtime;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		int iDamage = (DMG_PARALYZE << i);

		if (!g_pGameRules->Damage_IsTimeBased(iDamage))
			continue;

		if ( m_bitsDamageType & iDamage )
		{
			switch (i)
			{
				case itbd_Paralyze:
					bDuration = PARALYZE_DURATION;
				break;

				case itbd_NerveGas:
					bDuration = NERVEGAS_DURATION;
				break;

				case itbd_Radiation:
					bDuration = RADIATION_DURATION;
				break;

				case itbd_DrownRecover:
					if (m_idrowndmg > m_idrownrestored)
					{
						int idif = min(m_idrowndmg - m_idrownrestored, 10);

						TakeHealth(idif, DMG_GENERIC);
						m_idrownrestored += idif;
					}

					bDuration = 4;
				break;

				case itbd_PoisonRecover:
					if (m_nPoisonDmg > m_nPoisonRestored)
					{
						int nDif = min(m_nPoisonDmg - m_nPoisonRestored, 10);

						TakeHealth(nDif, DMG_GENERIC);
						m_nPoisonRestored += nDif;
					}
				
					bDuration = 9;
				break;

				case itbd_Acid:
					bDuration = ACID_DURATION;
				break;

				case itbd_SlowBurn:
					bDuration = SLOWBURN_DURATION;
				break;

				case itbd_SlowFreeze:
					bDuration = SLOWFREEZE_DURATION;
				break;

				default:
					bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					m_bitsDamageType		&= ~(DMG_PARALYZE << i);	
				}
			}
			else
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
	EL TRAJE DE PROTECCI�N - INMARK 1.0

	El traje de protecci�n es originalmente creado por Valve para Half-Life y Half-Life 2.
	En la siguiente descripci�n se explica el uso y las funciones del traje de protecci�n
	de siempre as� como el "nuevo" traje de protecci�n desarrollado para Apocalypse.

	------------------------------------------------------------------------------------------

	El traje de protecci�n proporciona 2 funciones principales: PROTECCI�N Y NOTIFICACI�N.
	Algunas de estas funciones son activadas autom�ticamente, otras requieren energ�a.
	El traje de protecci�n permanece con el usuario en todo el juego.

	------------------------------------------------------------------------------------------

	PROTECCI�N

	Radiaci�n
		El usuario podr� ser inmune a los efectos de la radiaci�n.
		NOTA: Excepto cuando el usuario cae en aguas radioactivas.

	Envenenamiento
		Podr� curar al usuario cuando este sea envenando por alguno de sus enemigos.

	Salud
		Podr� curar al usuario cuando este adquiera botiquines o en periodos de tiempo largos.

	Inmunidad
		El usuario podr� ser inmune a cualquier da�o durante unos segundos.
		Cabe mencionar que esta funci�n usar� la energ�a del traje para poder ser activada.

	Aumento de poder
		Las armas del usuario podr�n aumentar su da�o durante unos segundos.
		Cabe mencionar que esta funci�n usar� la energ�a del traje para poder ser activada.

	Armadura
		La armadura proteger� al usuario de impactos, balas y explosiones, esto quiere decir
		que cierto porcentaje del da�o ser� anulado.
		Cabe mencionar que esta funci�n usar� la energ�a del traje para poder ser activada.

	Reanimaci�n
		Podr� revivir al usuario dependiendo de la situaci�n.

*/

/*---------------------------------------------------
	UpdateGeigerCounter()
	Actualizar el contador Geiger.
---------------------------------------------------*/
void CBasePlayer::UpdateGeigerCounter( void )
{
	byte range;

	// No queremos flood de advertencias.
	if (gpGlobals->curtime < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->curtime + GEIGERDELAY;
		
	range = (byte) clamp(m_flgeigerRange / 4, 0, 255);

	if (IsInAVehicle())
		range = clamp((int)range * 4, 0, 255);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		CSingleUserRecipientFilter user(this);

		user.MakeReliable();

		UserMessageBegin(user, "Geiger");
			WRITE_BYTE( range );
		MessageEnd();
	}

	// Resetear.
	if (!random->RandomInt(0,3))
		m_flgeigerRange = 1000;
}


/*---------------------------------------------------
	CheckSuitUpdate()
	Verifica si hay actualizaciones a aplicar en el traje de protecci�n.
	NOTA: M�s que nada sentencias a reproducir.
---------------------------------------------------*/
void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence	= 0;
	int isearch		= m_iSuitPlayNext;
	
	// Al parecer a�n no tenemos el traje de protecci�n.
	if (!IsSuitEquipped())
		return;

	// Actualizar el contador Geiger en caso de estar en una zona de radiaci�n.
	UpdateGeigerCounter();

	// No son muy comodas las distracciones en Multiplayer.
	if (g_pGameRules->IsMultiplayer())
		return;

	if (gpGlobals->curtime >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			if ((isentence = m_rgSuitPlayList[isearch]) != 0)
				break;
			
			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;

			if (isentence > 0)
			{
				char sentence[512];
				Q_snprintf(sentence, sizeof(sentence), "!%s", engine->SentenceNameFromIndex(isentence));

				UTIL_EmitSoundSuit( edict(), sentence );
			}
			else
				UTIL_EmitGroupIDSuit(edict(), -isentence);

			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME;
		}
		else
			m_flSuitUpdate = 0;
	}
}

/*---------------------------------------------------
	SetSuitUpdate()
	Reproduce una setencia del traje de protecci�n.
---------------------------------------------------*/
void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;	
	
	// Al parecer a�n no tenemos el traje de protecci�n.
	if (!IsSuitEquipped())
		return;
	
	// No son muy comodas las distracciones en Multiplayer.
	if (g_pGameRules->IsMultiplayer())
		return;

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;

		return;
	}

	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name);

		if (isentence < 0)
			return;
	}
	else
		isentence = -SENTENCEG_GetIndex(name);

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->curtime)
			{
				m_rgiSuitNoRepeat[i]		= 0;
				m_rgflSuitNoRepeatTime[i]	= 0.0;
				iempty						= i;

				break;
			}
			else
				return;
		}

		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = random->RandomInt(0, CSUITNOREPEAT-1);

		m_rgiSuitNoRepeat[iempty]		= isentence;
		m_rgflSuitNoRepeatTime[iempty]	= iNoRepeatTime + gpGlobals->curtime;
	}

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;

	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->curtime)
	{
		if (m_flSuitUpdate == 0)
			m_flSuitUpdate = gpGlobals->curtime + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME; 
	}

}

/*---------------------------------------------------
	UpdatePlayerSound()
	Actualiza la posici�n del slot de sonido del jugador.
---------------------------------------------------*/
void CBasePlayer::UpdatePlayerSound(void)
{
	int iBodyVolume;
	int iVolume;

	CSound *pSound;
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	// �Hemos perdido el sonido?
	if (!pSound)
		return;

	if (GetFlags() & FL_NOTARGET)
	{
		pSound->m_iVolume = 0;
		return;
	}

	if (GetFlags() & FL_ONGROUND)
	{	
		iBodyVolume = GetAbsVelocity().Length(); 

		if (iBodyVolume > 512)
			iBodyVolume = 512;
	}
	else
		iBodyVolume = 0;

	if (m_nButtons & IN_JUMP)
		iBodyVolume += 100;

	m_iTargetVolume = iBodyVolume;
	iVolume			= pSound->Volume();

	if (m_iTargetVolume > iVolume)
		iVolume = m_iTargetVolume;

	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
			iVolume = 0;
	}

	if (pSound)
	{
		pSound->SetSoundOrigin(GetAbsOrigin());
		pSound->m_iType		= SOUND_PLAYER;
		pSound->m_iVolume	= iVolume;
	}
}

/*---------------------------------------------------
	FixPlayerCrouchStuck()
	Soluci�n "temporal" para calcular el espacio libre
	del lugar donde el usuario se ha agachado.
---------------------------------------------------*/
void FixPlayerCrouchStuck( CBasePlayer *pPlayer )
{
	trace_t trace;

	int i;
	Vector org = pPlayer->GetAbsOrigin();;

	for (i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), 
			VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

		if (trace.startsolid)
		{
			Vector origin	= pPlayer->GetAbsOrigin();
			origin.z		+= 1.0f;

			pPlayer->SetLocalOrigin( origin );
		}
		else
			return;
	}

	pPlayer->SetAbsOrigin(org);

	for (i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), 
			VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

		if (trace.startsolid)
		{
			Vector origin	 = pPlayer->GetAbsOrigin();
			origin.z		-= 1.0f;

			pPlayer->SetLocalOrigin( origin );
		}
		else
			return;
	}
}

/*---------------------------------------------------
	IsRideablePhysics()
---------------------------------------------------*/
bool CBasePlayer::IsRideablePhysics(IPhysicsObject *pPhysics)
{
	if (pPhysics)
	{
		if (pPhysics->GetMass() > (VPhysicsGetObject()->GetMass()*2))
			return true;
	}

	return false;
}

/*---------------------------------------------------
	GetGroundVPhysics
---------------------------------------------------*/
IPhysicsObject *CBasePlayer::GetGroundVPhysics()
{
	CBaseEntity *pGroundEntity = GetGroundEntity();

	if (pGroundEntity && pGroundEntity->GetMoveType() == MOVETYPE_VPHYSICS)
	{
		IPhysicsObject *pPhysGround = pGroundEntity->VPhysicsGetObject();

		if (pPhysGround && pPhysGround->IsMoveable())
			return pPhysGround;
	}

	return NULL;
}


/*---------------------------------------------------
	ForceOrigin()
	Herramienta de prueba (debugging)
---------------------------------------------------*/
void CBasePlayer::ForceOrigin( const Vector &vecOrigin )
{
	m_bForceOrigin = true;
	m_vForcedOrigin = vecOrigin;
}

/*---------------------------------------------------
	PreThink()
	Ejecuci�n de tareas del jugador.
---------------------------------------------------*/
void CBasePlayer::PreThink(void)
{
	// El juego acabo.
	if (g_fGameOver || m_iPlayerLocked)
		return;

	if (Hints())
		Hints()->Update();

	ItemPreFrame();
	WaterMove();

	UpdateClientData();	
	CheckTimeBasedDamage();
	CheckSuitUpdate();

	//if (GetObserverMode() > OBS_MODE_FREEZECAM)
	//	CheckObserverSettings();

	if (m_lifeState >= LIFE_DYING)
		return;

	HandleFuncTrain();

	// �Saltar!
	if (m_nButtons & IN_JUMP)
		Jump();

	// Agacharse
	if ((m_nButtons & IN_DUCK) || (GetFlags() & FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	// Si estamos cayendo, calcular la velocidad de caida.
	if (!( GetFlags() & FL_ONGROUND))
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;

	#ifndef _XBOX
		CNavArea *area = TheNavMesh->GetNavArea(GetAbsOrigin());

		if (area && area != m_lastNavArea)
		{
			if (m_lastNavArea)
				m_lastNavArea->DecrementPlayerCount( GetTeamNumber() );

			area->IncrementPlayerCount(GetTeamNumber());
			m_lastNavArea = area;

			if (area->GetPlace() != UNDEFINED_PLACE)
			{
				const char *placeName = TheNavMesh->PlaceToName(area->GetPlace());

				if (placeName && *placeName)
					Q_strncpy(m_szLastPlaceName.GetForModify(), placeName, MAX_PLACE_NAME_LENGTH);
			}
		}
	#endif
}

/*---------------------------------------------------
	PostThink()
	Ejecuci�n de tareas del jugador.
---------------------------------------------------*/
void CBasePlayer::PostThink()
{
	m_vecSmoothedVelocity = m_vecSmoothedVelocity * SMOOTHING_FACTOR + GetAbsVelocity() * (1 - SMOOTHING_FACTOR);

	// El juego no ha acabado.
	if (!g_fGameOver && !m_iPlayerLocked)
	{
		// Si seguimos vivos.
		if (IsAlive())
		{
			VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-Bounds");

				if (GetFlags() & FL_DUCKING)
					SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );			
				else
					SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );

			VPROF_SCOPE_END();

			VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-Use");
			
				if (m_hUseEntity != NULL)
				{ 
					if ( m_hUseEntity->OnControls( this ) && 
						( !GetActiveWeapon() || GetActiveWeapon()->IsEffectActive( EF_NODRAW ) ||
						( GetActiveWeapon()->GetActivity() == ACT_VM_HOLSTER ) 			
						) )
					{  
						m_hUseEntity->Use(this, this, USE_SET, 2);
					}
					else
						ClearUseEntity();
				}

			VPROF_SCOPE_END();

			VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-ItemPostFrame");
				ItemPostFrame();
			VPROF_SCOPE_END();
			
			// Tenemos los pies sobre el suelo.
			if (GetFlags() & FL_ONGROUND)
			{		
				if (m_Local.m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
					CSoundEnt::InsertSound (SOUND_PLAYER, GetAbsOrigin(), m_Local.m_flFallVelocity, 0.2, this);

				m_Local.m_flFallVelocity = 0;
			}

			// Seleccionar la animaci�n apropiada para el jugador.
			VPROF("CBasePlayer::PostThink-Animation");

				if (IsInAVehicle())
					SetAnimation(PLAYER_IN_VEHICLE);
				else if (!GetAbsVelocity().x && !GetAbsVelocity().y)
					SetAnimation(PLAYER_IDLE);
				else if ((GetAbsVelocity().x || GetAbsVelocity().y) && (GetFlags() & FL_ONGROUND))
					SetAnimation(PLAYER_WALK);
				else if (GetWaterLevel() > 1)
					SetAnimation(PLAYER_WALK);

			/*
				InSource
			*/

			ConVarRef mat_yuv("mat_yuv");
			ConVarRef host_timescale("host_timescale");
			
			// Regeneraci�n de salud
			if(m_iHealth < GetMaxHealth() && (in_regeneration.GetInt() == 1))
			{
				if(gpGlobals->curtime > m_flLastDamageTime + in_regeneration_wait_time.GetFloat())
				{
					m_fRegenRemander += in_regeneration_rate.GetFloat() * gpGlobals->frametime;

					if(m_fRegenRemander >= 1)
					{
						TakeHealth(m_fRegenRemander, DMG_GENERIC);
						m_fRegenRemander = 0;
					}
				}
			}

			// Efectos de cansancio.
			if(in_tired_effect.GetInt() == 1)
			{
				// Desactivar escala de grises, me estoy reponiendo.
				if(m_iHealth > 10 && mat_yuv.GetInt() == 1)
				{
					mat_yuv.SetValue(0);
					StopSound("Player.Music.Dying");
				}

				if(m_iHealth <= 30)
				{
					int DeathFade = (230 - (6 * m_iHealth));
					color32 black = {0, 0, 0, DeathFade};

					UTIL_ScreenFade(this, black, 1.0f, 0.1f, FFADE_IN);
				}

				if(m_iHealth < 10)
					UTIL_ScreenShake(GetAbsOrigin(), 1.0, 1.0, 1.0, 750, SHAKE_START, true);
			}

			// Camara lenta.
			if(in_timescale_effect.GetInt() == 1)
			{
				if(m_iHealth < 10 && host_timescale.GetFloat() != 0.6)
					engine->ClientCommand(edict(), "host_timescale 0.6");

				else if(m_iHealth < 15 && host_timescale.GetFloat() != 0.8)
					engine->ClientCommand(edict(), "host_timescale 0.8");

				else if(m_iHealth > 15 && host_timescale.GetFloat() != 1)
					host_timescale.SetValue(1);
			}

			// Me duele el cuerpo, efecto de cansancio.
			if(m_iHealth < 60)
			{
				m_fDeadTimer += 0.2 * gpGlobals->frametime;

				if(m_fDeadTimer >= 1)
				{
					int AngHealth = (100 - m_iHealth) / 8;
					ViewPunch(QAngle(random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth), random->RandomInt(2.0, AngHealth)));
				}
			}

			if(m_fDeadTimer >= 1)
				m_fDeadTimer = 0;

			/*
				InSource
			*/
		}

		if (GetSequence() == -1)
			SetSequence(0);

		VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-StudioFrameAdvance");
			StudioFrameAdvance();
		VPROF_SCOPE_END();

		VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-DispatchAnimEvents");
			DispatchAnimEvents( this );
		VPROF_SCOPE_END();

		SetSimulationTime(gpGlobals->curtime);

		// Armas
		VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-Weapon_FrameUpdate");
			Weapon_FrameUpdate();
		VPROF_SCOPE_END();

		VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-UpdatePlayerSound");
			UpdatePlayerSound();
		VPROF_SCOPE_END();

		if (m_bForceOrigin)
		{
			SetLocalOrigin( m_vForcedOrigin );
			SetLocalAngles( m_Local.m_vecPunchAngle );
			m_Local.m_vecPunchAngle = RandomAngle( -25, 25 );
			m_Local.m_vecPunchAngleVel.Init();
		}

		VPROF_SCOPE_BEGIN("CBasePlayer::PostThink-PostThinkVPhysics");
			PostThinkVPhysics();
		VPROF_SCOPE_END();
	}

	#if !defined( NO_ENTITY_PREDICTION )
		SimulatePlayerSimulatedEntities();
	#endif
}

/*---------------------------------------------------
	Touch()
	Tocando un objeto.
---------------------------------------------------*/
void CBasePlayer::Touch( CBaseEntity *pOther )
{
	if (pOther == GetGroundEntity())
		return;

	if (pOther->GetMoveType() != MOVETYPE_VPHYSICS || pOther->GetSolid() != SOLID_VPHYSICS || (pOther->GetSolidFlags() & FSOLID_TRIGGER))
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();

	if (!pPhys || !pPhys->IsMoveable())
		return;

	SetTouchedPhysics(true);
}

/*---------------------------------------------------
	PostThinkVPhysics()
---------------------------------------------------*/
void CBasePlayer::PostThinkVPhysics( void )
{
	if ( !m_pPhysicsController )
		return;

	Vector newPosition	= GetAbsOrigin();
	float frametime		= gpGlobals->frametime;

	if (frametime <= 0 || frametime > 0.1f)
		frametime = 0.1f;

	IPhysicsObject *pPhysGround = GetGroundVPhysics();

	if (!pPhysGround && m_touchedPhysObject && g_pMoveData->m_outStepHeight <= 0.f && (GetFlags() & FL_ONGROUND))
	{
		newPosition = m_oldOrigin + frametime * g_pMoveData->m_outWishVel;
		newPosition = (GetAbsOrigin() * 0.5f) + (newPosition * 0.5f);
	}

	int collisionState = VPHYS_WALK;

	if (GetMoveType() == MOVETYPE_NOCLIP || GetMoveType() == MOVETYPE_OBSERVER)
		collisionState = VPHYS_NOCLIP;

	else if ( GetFlags() & FL_DUCKING )
		collisionState = VPHYS_CROUCH;

	if (collisionState != m_vphysicsCollisionState)
		SetVCollisionState(GetAbsOrigin(), GetAbsVelocity(), collisionState);

	if (!(TouchedPhysics() || pPhysGround))
	{
		float maxSpeed = (m_flMaxspeed > 0.0f) ? m_flMaxspeed : sv_maxspeed.GetFloat();
		g_pMoveData->m_outWishVel.Init(maxSpeed, maxSpeed, maxSpeed);
	}

	if (g_pMoveData->m_outStepHeight > 0.1f)
	{
		if (g_pMoveData->m_outStepHeight > 4.0f)
			VPhysicsGetObject()->SetPosition(GetAbsOrigin(), vec3_angle, true);
		else
		{
			Vector position, end;

			VPhysicsGetObject()->GetPosition(&position, NULL);
			end = position;
			end.z += g_pMoveData->m_outStepHeight;
			trace_t trace;

			UTIL_TraceEntity(this, position, end, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

			if (trace.DidHit())
				g_pMoveData->m_outStepHeight = trace.endpos.z - position.z;
			
			m_pPhysicsController->StepUp(g_pMoveData->m_outStepHeight);
		}

		m_pPhysicsController->Jump();
	}

	g_pMoveData->m_outStepHeight = 0.0f;
		
	m_vNewVPhysicsPosition = newPosition;
	m_vNewVPhysicsVelocity = g_pMoveData->m_outWishVel;

	m_oldOrigin = GetAbsOrigin();
}

/*---------------------------------------------------
	UpdateVPhysicsPosition()
---------------------------------------------------*/
void CBasePlayer::UpdateVPhysicsPosition( const Vector &position, const Vector &velocity, float secondsToArrival )
{
	bool onground = (GetFlags() & FL_ONGROUND) ? true : false;
	IPhysicsObject *pPhysGround = GetGroundVPhysics();
	
	if (!IsRideablePhysics(pPhysGround))
		pPhysGround = NULL;

	m_pPhysicsController->Update(position, velocity, secondsToArrival, onground, pPhysGround);
}

/*---------------------------------------------------
	UpdatePhysicsShadowToCurrentPosition()
---------------------------------------------------*/
void CBasePlayer::UpdatePhysicsShadowToCurrentPosition()
{
	UpdateVPhysicsPosition(GetAbsOrigin(), vec3_origin, gpGlobals->frametime);
}

/*---------------------------------------------------
	UpdatePhysicsShadowToPosition()
---------------------------------------------------*/
void CBasePlayer::UpdatePhysicsShadowToPosition(const Vector &vecAbsOrigin)
{
	UpdateVPhysicsPosition(vecAbsOrigin, vec3_origin, gpGlobals->frametime);
}

/*---------------------------------------------------
	GetSmoothedVelocity()
---------------------------------------------------*/
Vector CBasePlayer::GetSmoothedVelocity(void)
{ 
	if (IsInAVehicle())
		return GetVehicle()->GetVehicleEnt()->GetSmoothedVelocity();
	
	return m_vecSmoothedVelocity;
}

/*---------------------------------------------------
	FindPlayerStart()
	Encontrar la entidad donde el jugador comenzar�.
	NOTA: info_player_start, info_player_coop, info_player_deathmatch
---------------------------------------------------*/
CBaseEntity *FindPlayerStart(const char *pszClassName)
{
	#define SF_PLAYER_START_MASTER	1
	
	CBaseEntity *pStart			= gEntList.FindEntityByClassname(NULL, pszClassName);
	CBaseEntity *pStartFirst	= pStart;

	while (pStart != NULL)
	{
		if (pStart->HasSpawnFlags(SF_PLAYER_START_MASTER))
			return pStart;

		pStart = gEntList.FindEntityByClassname(pStart, pszClassName);
	}

	return pStartFirst;
}

/*---------------------------------------------------
	EntSelectSpawnPoint()
	Retorna el lugar donde se hara un Spawn.
---------------------------------------------------*/
CBaseEntity *CBasePlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = edict();

	// Buscar por info_player_coop
	if (g_pGameRules->IsCoOp())
	{
		pSpot = gEntList.FindEntityByClassname(g_pLastSpawn, "info_player_coop");

		if (pSpot)
			goto ReturnSpot;

		pSpot = gEntList.FindEntityByClassname(g_pLastSpawn, "info_player_start");

		if (pSpot) 
			goto ReturnSpot;
	}

	// Buscar por info_player_deathmatch
	else if (g_pGameRules->IsDeathmatch())
	{
		pSpot = g_pLastSpawn;

		// Seleccionar uno de los tantos que pudiera haber.
		for (int i = random->RandomInt(1,5); i > 0; i--)
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		
		if (!pSpot)
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if (pSpot)
			{
				// Checar si el Spot es v�lido.
				if (g_pGameRules->IsSpawnPointValid(pSpot, this))
				{
					if (pSpot->GetLocalOrigin() == vec3_origin)
					{
						pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_deathmatch");
						continue;
					}

					goto ReturnSpot;
				}
			}

			pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_deathmatch");
		} while (pSpot != pFirstSpot);

		// Si no se encontro un lugar v�lido para hacer Spawn, encontrar un lugar adecuado autom�ticamente.
		if (pSpot)
		{
			CBaseEntity *ent = NULL;

			for (CEntitySphereQuery sphere(pSpot->GetAbsOrigin(), 128); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
			{
				// Si hay una entidad (tipo jugador) en el Spawn �matarlo!
				if (ent->IsPlayer() && !(ent->edict() == player))
					ent->TakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC));
			}

			goto ReturnSpot;
		}
	}

	if (!gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = FindPlayerStart("info_player_start");

		if (pSpot)
			goto ReturnSpot;
	}
	else
	{
		pSpot = gEntList.FindEntityByName(NULL, gpGlobals->startspot);

		if (pSpot)
			goto ReturnSpot;
	}

ReturnSpot:
	if (!pSpot)
	{
		Warning("PutClientInServer: no info_player_start on level\n");
		return CBaseEntity::Instance(INDEXENT(0));
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}

/*---------------------------------------------------
	InitialSpawn()
	El jugador ser� "spawneado" por primera vez.
---------------------------------------------------*/
void CBasePlayer::InitialSpawn(void)
{
	m_iConnected = PlayerConnected;
	gamestats->Event_PlayerConnected(this);
}

/*---------------------------------------------------
	ClearTonemapParams()
---------------------------------------------------*/
void CBasePlayer::ClearTonemapParams(void)
{
	m_Local.m_TonemapParams.m_flAutoExposureMin = -1.0f;
	m_Local.m_TonemapParams.m_flAutoExposureMax = -1.0f;
	m_Local.m_TonemapParams.m_flTonemapScale	= -1.0f;
	m_Local.m_TonemapParams.m_flBloomScale		= -1.0f;
	m_Local.m_TonemapParams.m_flTonemapRate		= -1.0f;
}

/*---------------------------------------------------
	InputSetTonemapScale()
---------------------------------------------------*/
void CBasePlayer::InputSetTonemapScale(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flTonemapScale = inputdata.value.Float();
}

/*---------------------------------------------------
	InputSetTonemapRate()
---------------------------------------------------*/
void CBasePlayer::InputSetTonemapRate(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flTonemapRate = inputdata.value.Float();
}
void CBasePlayer::InputSetAutoExposureMin(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flAutoExposureMin = inputdata.value.Float();
}

/*---------------------------------------------------
	InputSetAutoExposureMax()
---------------------------------------------------*/
void CBasePlayer::InputSetAutoExposureMax(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flAutoExposureMax = inputdata.value.Float();
}

/*---------------------------------------------------
	InputSetBloomScale()
---------------------------------------------------*/
void CBasePlayer::InputSetBloomScale(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flBloomScale = inputdata.value.Float();
}

/*---------------------------------------------------
	InputUseDefaultAutoExposure()
---------------------------------------------------*/
void CBasePlayer::InputUseDefaultAutoExposure(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flAutoExposureMin = -1.0f;
	m_Local.m_TonemapParams.m_flAutoExposureMax = -1.0f;
	m_Local.m_TonemapParams.m_flTonemapRate		= -1.0f;
}

/*---------------------------------------------------
	InputUseDefaultBloomScale()
---------------------------------------------------*/
void CBasePlayer::InputUseDefaultBloomScale(inputdata_t &inputdata)
{
	m_Local.m_TonemapParams.m_flBloomScale = -1.0f;
}

/*---------------------------------------------------
	Spawn()
	Spawn al jugador.
---------------------------------------------------*/
void CBasePlayer::Spawn( void )
{
	if (Hints())
		Hints()->ResetHints();
	
	ClearTonemapParams();
	SetClassname("player");

	SharedSpawn();
	
	SetSimulatedEveryTick(true);
	SetAnimatedEveryTick(true);

	m_ArmorValue		= SpawnArmorValue();
	SetBlocksLOS(false);
	m_iMaxHealth		= m_iHealth;

	if (GetFlags() & FL_FAKECLIENT)
	{
		ClearFlags();
		AddFlag(FL_CLIENT | FL_FAKECLIENT);
	}
	else
	{
		ClearFlags();
		AddFlag(FL_CLIENT);
	}

	AddFlag(FL_AIMTARGET);

	m_AirFinished	= gpGlobals->curtime + AIRTIME;
	m_nDrownDmgRate	= DROWNING_DAMAGE_INITIAL;
	
	int effects = GetEffects() & EF_NOSHADOW;
	SetEffects(effects | EF_NOINTERP);

	InitFogController();

	m_DmgTake			= 0;
	m_DmgSave			= 0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;

	m_idrownrestored	= m_idrowndmg;

	SetFOV(this, 0);

	m_flNextDecalTime	= 0;							// Permitir pintar su logo.
	m_flgeigerDelay		= gpGlobals->curtime + 2.0;	
	m_flFieldOfView		= 0.766;						// Restaurar vista del usuario, para evitar que los NPC piensen que los estamos mirando.

	m_vecAdditionalPVSOrigin	= vec3_origin;
	m_vecCameraPVSOrigin		= vec3_origin;

	if (!m_fGameHUDInitialized)
		g_pGameRules->SetDefaultPlayerTeam(this);

	g_pGameRules->GetPlayerSpawnSpot(this);

	// Resetear variables para detecci�n del agachado.
	m_Local.m_bDucked	= false;
	m_Local.m_bDucking	= false;

    SetViewOffset(VEC_VIEW);
	Precache();
	
	m_bitsDamageType	= 0;
	m_bitsHUDDamage		= -1;
	m_iTrain			= TRAIN_NEW;	
	m_HackedGunPos		= Vector(0, 32, 0);

	SetPlayerUnderwater(false);

	m_iBonusChallenge = sv_bonus_challenge.GetInt();
	sv_bonus_challenge.SetValue(0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
		Msg("Couldn't alloc player sound slot!\n");

	SetThink(NULL);

	m_fInitHUD			= true;
	m_fWeapon			= false;
	m_iClientBattery	= -1;

	m_lastx = m_lasty	= 0;
	m_lastNavArea		= NULL;

	#ifndef _XBOX
		if (TheNavMesh)
			TheNavMesh->ClearPlayerCounts();
	#endif

	Q_strncpy(m_szLastPlaceName.GetForModify(), "", MAX_PLACE_NAME_LENGTH);
	
	CSingleUserRecipientFilter user(this);
	enginesound->SetPlayerDSP(user, 0, false);

	CreateViewModel();
	SetCollisionGroup(COLLISION_GROUP_PLAYER);

	// Si el jugador ha sido bloqueado, pues seguir� bloqueado
	if (m_iPlayerLocked)
	{
		m_iPlayerLocked = false;
		LockPlayerInPlace();
	}
		
	/*
	if (GetTeamNumber() != TEAM_SPECTATOR)
		StopObserverMode();

	else
		StartObserverMode( m_iObserverLastMode );

	StopReplayMode();
	*/

	// Limpiar cualquier efecto en la pantalla.
	color32 nothing = {0,0,0,255};
	UTIL_ScreenFade(this, nothing, 0, 0, FFADE_IN | FFADE_PURGE);

	g_pGameRules->PlayerSpawn(this);

	m_flLaggedMovementValue = 1.0f;
	m_vecSmoothedVelocity	= vec3_origin;

	InitVCollision(GetAbsOrigin(), GetAbsVelocity());

	/*
		InSource
	*/

	// Resetear el tiempo.
	ConVarRef host_timescale("host_timescale");

	if(host_timescale.GetInt() != 1)
		engine->ClientCommand(edict(), "host_timescale 1");

	/*
		InSource
	*/

	IGameEvent *event = gameeventmanager->CreateEvent("player_spawn");
	
	if (event)
	{
		event->SetInt("userid", GetUserID());
		gameeventmanager->FireEvent(event);
	}

	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );
	m_nVehicleViewSavedFrame = 0;
}

/*---------------------------------------------------
	Activate()
---------------------------------------------------*/
void CBasePlayer::Activate( void )
{
	BaseClass::Activate();

	AimTarget_ForceRepopulateList();
	RumbleEffect(RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE);

	m_iVehicleAnalogBias = VEHICLE_ANALOG_BIAS_NONE;
}

/*---------------------------------------------------
	Precache()
	Guardar en cach� objetos necesarios.
---------------------------------------------------*/
void CBasePlayer::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound("Player.FallGib");
	PrecacheScriptSound("Player.Death");
	PrecacheScriptSound("Player.PlasmaDamage");
	PrecacheScriptSound("Player.SonicDamage");
	PrecacheScriptSound("Player.DrownStart");
	PrecacheScriptSound("Player.DrownContinue");
	PrecacheScriptSound("Player.Wade");
	PrecacheScriptSound("Player.AmbientUnderWater");

	/*
		InSource
	*/

	PrecacheScriptSound("Player.Pain");
	PrecacheScriptSound("Player.Music.Dying");
	PrecacheScriptSound("Player.Music.Dead");

	enginesound->PrecacheSentenceGroup("HEV");

	#ifndef TF_DLL
		PrecacheParticleSystem("slime_splash_01");
		PrecacheParticleSystem("slime_splash_02");
		PrecacheParticleSystem("slime_splash_03");
	#endif

	// Resetear contador de Geiger.
	m_flgeigerRange		= 1000;
	m_igeigerRangePrev	= 1000;

	#if 0
		m_bitsDamageType	= 0;
		m_bitsHUDDamage		= -1;
		m_iTrain			= TRAIN_NEW;

		SetPlayerUnderwter(false);
	#endif

	m_iClientBattery	= -1;
	m_iUpdateTime		= 5;

	if (gInitHUD)
		m_fInitHUD = true;

}


/*---------------------------------------------------
	Precache()
	Forzar al jugador a hacer Spawn.
---------------------------------------------------*/
void CBasePlayer::ForceRespawn(void)
{
	RemoveAllItems(true);
	SetGroundEntity(NULL);

	// Resetear los botones que estan presionados y evitar bugs.
	m_nButtons = 0;
	Spawn();
}

/*---------------------------------------------------
	Save()
	Guardar
---------------------------------------------------*/
int CBasePlayer::Save(ISave &save)
{
	if (!BaseClass::Save(save))
		return 0;

	return 1;
}

/*---------------------------------------------------
	CPlayerRestoreHelper
	Clase amigable para acceder a objetos privados.
---------------------------------------------------*/
class CPlayerRestoreHelper
{
public:

	const Vector &GetAbsOrigin( CBaseEntity *pent )
	{
		return pent->m_vecAbsOrigin;
	}

	const Vector &GetAbsVelocity( CBaseEntity *pent )
	{
		return pent->m_vecAbsVelocity;
	}
};

/*---------------------------------------------------
	Restore()
---------------------------------------------------*/
int CBasePlayer::Restore( IRestore &restore )
{
	int status = BaseClass::Restore(restore);

	if (!status)
		return 0;

	CSaveRestoreData *pSaveData = gpGlobals->pSaveData;
	
	if (!pSaveData->levelInfo.fUseLandmark)
	{
		Msg("No Landmark:%s\n", pSaveData->levelInfo.szLandmarkName);

		CBaseEntity *pSpawnSpot = EntSelectSpawnPoint();
		SetLocalOrigin(pSpawnSpot->GetLocalOrigin() + Vector(0,0,1));
		SetLocalAngles(pSpawnSpot->GetLocalAngles());
	}

	QAngle newViewAngles	= pl.v_angle;
	newViewAngles.z			= 0;

	SetLocalAngles(newViewAngles);
	SnapEyeAngles(newViewAngles);

	SetBloodColor(BLOOD_COLOR_RED);
	
	m_afPhysicsFlags &= ~PFLAG_VPHYSICS_MOTIONCONTROLLER;

	if (GetFlags() & FL_DUCKING) 
	{
		FixPlayerCrouchStuck(this);
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);

		m_Local.m_bDucked = true;
	}
	else
	{
		m_Local.m_bDucked = false;
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	CPlayerRestoreHelper helper;
	InitVCollision(helper.GetAbsOrigin(this), helper.GetAbsVelocity(this));

	return 1;
}

/*---------------------------------------------------
	OnRestore()
---------------------------------------------------*/
void CBasePlayer::OnRestore( void )
{
	BaseClass::OnRestore();

	SetViewEntity(m_hViewEntity);
	SetDefaultFOV(m_iDefaultFOV);

	m_nVehicleViewSavedFrame	= 0;
	m_nBodyPitchPoseParam		= LookupPoseParameter("body_pitch");
}

/*---------------------------------------------------
	SetArmorValue()
---------------------------------------------------*/
void CBasePlayer::SetArmorValue( int value )
{
	m_ArmorValue = value;
}

/*---------------------------------------------------
	IncrementArmorValue()
---------------------------------------------------*/
void CBasePlayer::IncrementArmorValue( int nCount, int nMaxValue )
{ 
	m_ArmorValue += nCount;

	if (nMaxValue > 0)
	{
		if (m_ArmorValue > nMaxValue)
			m_ArmorValue = nMaxValue;
	}
}

/*---------------------------------------------------
	SetPhysicsFlag()
---------------------------------------------------*/
void CBasePlayer::SetPhysicsFlag(int nFlag, bool bSet)
{
	if (bSet)
		m_afPhysicsFlags |= nFlag;
	else
		m_afPhysicsFlags &= ~nFlag;
}

/*---------------------------------------------------
	NotifyNearbyRadiationSource()
	Notificar sobre una zona radioactiva.
---------------------------------------------------*/
void CBasePlayer::NotifyNearbyRadiationSource( float flRange )
{
	if (m_flgeigerRange >= flRange)
		m_flgeigerRange = flRange;
}

/*---------------------------------------------------
	AllowImmediateDecalPainting()
---------------------------------------------------*/
void CBasePlayer::AllowImmediateDecalPainting()
{
	m_flNextDecalTime = gpGlobals->curtime;
}

/*---------------------------------------------------
	CommitSuicide()
	Suicidarse
---------------------------------------------------*/
void CBasePlayer::CommitSuicide(bool bExplode, bool bForce)
{
	MDLCACHE_CRITICAL_SECTION();

	if(!IsAlive())
		return;
		
	if (m_fNextSuicideTime > gpGlobals->curtime && !bForce)
		return;

	m_fNextSuicideTime	= gpGlobals->curtime + 5;
	int fDamage			= DMG_PREVENT_PHYSICS_FORCE | ( bExplode ? ( DMG_BLAST | DMG_ALWAYSGIB ) : DMG_NEVERGIB );

	m_iHealth = 0;
	Event_Killed(CTakeDamageInfo(this, this, 0, fDamage, m_iSuicideCustomKillFlags));
	Event_Dying();
	m_iSuicideCustomKillFlags = 0;
}

/*---------------------------------------------------
	CommitSuicide()
	Suicidarse... con estilo.
---------------------------------------------------*/
void CBasePlayer::CommitSuicide(const Vector &vecForce, bool bExplode, bool bForce)
{
	MDLCACHE_CRITICAL_SECTION();

	if(!IsAlive())
		return;

	if (m_fNextSuicideTime > gpGlobals->curtime && !bForce)
		return;

	m_fNextSuicideTime	= gpGlobals->curtime + 5;  
	int nHealth			= GetHealth();

	CTakeDamageInfo info;

	info.SetDamage(nHealth + 10);
	info.SetAttacker(this);
	info.SetDamageType(bExplode ? DMG_ALWAYSGIB : DMG_GENERIC);
	info.SetDamageForce(vecForce);
	info.SetDamagePosition(WorldSpaceCenter());

	TakeDamage(info);
}

/*---------------------------------------------------
	HasWeapons()
	�Tengo armas?
---------------------------------------------------*/
bool CBasePlayer::HasWeapons(void)
{
	int i;

	for (i = 0 ; i < WeaponCount() ; i++)
	{
		if (GetWeapon(i))
			return true;
	}

	return false;
}

/*---------------------------------------------------
	VelocityPunch()
---------------------------------------------------*/
void CBasePlayer::VelocityPunch(const Vector &vecForce)
{
	SetGroundEntity(NULL);
	ApplyAbsVelocityImpulse(vecForce);
}


/*
	VEHICULOS
*/

/*---------------------------------------------------
	CanEnterVehicle()
	�Puedo entrar en el vehiculo?
---------------------------------------------------*/
bool CBasePlayer::CanEnterVehicle(IServerVehicle *pVehicle, int nRole)
{
	// Al parecer ya hay alguien conduciendo.
	if (pVehicle->GetPassenger(nRole))
		return false;

	// Al parecer el conductor tiene armas.
	if (pVehicle->IsPassengerUsingStandardWeapons(nRole) == false)
	{
		// Debe ser capaz de gurdar su arma actual.
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();

		if ((pWeapon != NULL) && (pWeapon->CanHolster() == false))
			return false;
	}

	// Obviamente debemos estar vivos.
	if (!IsAlive())
		return false;

	if (IsEFlagSet(EFL_IS_BEING_LIFTED_BY_BARNACLE))
		return false;

	return true;
}

/*---------------------------------------------------
	GetInVehicle()
	Entrar al vehiculo
---------------------------------------------------*/
bool CBasePlayer::GetInVehicle(IServerVehicle *pVehicle, int nRole)
{
	Assert(NULL == m_hVehicle.Get());
	Assert(nRole >= 0);
	
	// Primero debemos verificar que podemos entrar.
	if (CanEnterVehicle(pVehicle, nRole) == false)
		return false;

	CBaseEntity *pEnt = pVehicle->GetVehicleEnt();
	Assert(pEnt);

	// Intentando guardar las armas activas.
	if (pVehicle->IsPassengerUsingStandardWeapons(nRole) == false)
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();

		if (pWeapon != NULL)
			pWeapon->Holster( NULL );

		#ifndef HL2_DLL
				m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
		#endif
			
		m_Local.m_iHideHUD |= HIDEHUD_INVEHICLE;
	}

	if (!pVehicle->IsPassengerVisible(nRole))
		AddEffects(EF_NODRAW);

	// Definir el conductor del vehiculo.
	pVehicle->SetPassenger(nRole, this);

	ViewPunchReset();

	SetAbsVelocity(vec3_origin);
	SetMoveType(MOVETYPE_NOCLIP);

	gamestats->Event_DecrementPlayerEnteredNoClip(this);

	Vector vSeatOrigin;
	QAngle qSeatAngles;

	pVehicle->GetPassengerSeatPoint(nRole, &vSeatOrigin, &qSeatAngles);
	
	SetAbsOrigin(vSeatOrigin);
	SetAbsAngles(qSeatAngles);
	
	SetParent(pEnt);
	SetCollisionGroup(COLLISION_GROUP_IN_VEHICLE);
	
	RemoveFlag(FL_DUCKING);
	SetViewOffset(VEC_VIEW);

	m_Local.m_bDucked			= false;
	m_Local.m_bDucking			= false;
	m_Local.m_flDucktime		= 0.0f;
	m_Local.m_flDuckJumpTime	= 0.0f;
	m_Local.m_flJumpTime		= 0.0f;

	if (GetToggledDuckState())
		ToggleDuck();

	m_hVehicle = pEnt;

	// Enviar un evento notificando que el usuario ha entrado al vehiculo.
	g_pNotify->ReportNamedEvent(this, "PlayerEnteredVehicle");
	m_iVehicleAnalogBias = VEHICLE_ANALOG_BIAS_NONE;

	OnVehicleStart();
	return true;
}

/*---------------------------------------------------
	LeaveVehicle()
	Salir del vehiculo
---------------------------------------------------*/
void CBasePlayer::LeaveVehicle(const Vector &vecExitPoint, const QAngle &vecExitAngles)
{
	// Al parecer no estamos en ning�n vehiculo.
	if (m_hVehicle.Get() == NULL)
		return;

	IServerVehicle *pVehicle = GetVehicle();
	Assert(pVehicle);

	int nRole = pVehicle->GetPassengerRole(this);
	Assert(nRole >= 0);

	SetParent(NULL);

	// Encontrar una salida no bloqueada.
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();

	if (vecExitPoint == vec3_origin)
		pVehicle->GetPassengerExitPoint(nRole, &vNewPos, &qAngles);
	else
	{
		vNewPos = vecExitPoint;
		qAngles = vecExitAngles;
	}

	OnVehicleEnd(vNewPos);
	SetAbsOrigin(vNewPos);
	SetAbsAngles(qAngles);
	
	SetAbsVelocity(vec3_origin);

	qAngles[ROLL] = 0;
	SnapEyeAngles(qAngles);

	#ifndef HL2_DLL
		m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	#endif

	m_Local.m_iHideHUD &= ~HIDEHUD_INVEHICLE;

	RemoveEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_WALK);
	SetCollisionGroup(COLLISION_GROUP_PLAYER);

	if (VPhysicsGetObject())
		VPhysicsGetObject()->SetPosition(vNewPos, vec3_angle, true);

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	if (IsAlive())
	{
		// Mostrar nuestra arma.
		if (GetActiveWeapon() && GetActiveWeapon()->IsWeaponVisible() == false)
		{
			GetActiveWeapon()->Deploy();
			ShowCrosshair(true);
		}
	}

	RumbleEffect(RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE);
}


/*---------------------------------------------------
	CSprayCan
	NOTA: INCOMPLETO
---------------------------------------------------*/
class CSprayCan : public CPointEntity
{
public:
	DECLARE_CLASS( CSprayCan, CPointEntity );

	void	Spawn ( CBasePlayer *pOwner );
	void	Think( void );

	virtual void Precache();

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(spraycan, CSprayCan);
PRECACHE_REGISTER(spraycan);

void CSprayCan::Spawn ( CBasePlayer *pOwner )
{
	SetLocalOrigin(pOwner->WorldSpaceCenter() + Vector (0 , 0 , 32));
	SetLocalAngles(pOwner->EyeAngles());
	SetOwnerEntity(pOwner);
	SetNextThink(gpGlobals->curtime);

	EmitSound("SprayCan.Paint");
}

void CSprayCan::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound("SprayCan.Paint");
}

void CSprayCan::Think(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());

	if (pPlayer)
	{
       	int playernum = pPlayer->entindex();
		
		Vector forward;
		trace_t	tr;	

		AngleVectors(GetAbsAngles(), &forward);
		UTIL_TraceLine (GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
			MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

		UTIL_PlayerDecalTrace(&tr, playernum);
	}
	
	UTIL_Remove(this);
}

class	CBloodSplat : public CPointEntity
{
public:
	DECLARE_CLASS(CBloodSplat, CPointEntity);

	void	Spawn (CBaseEntity *pOwner);
	void	Think (void);
};

void CBloodSplat::Spawn ( CBaseEntity *pOwner )
{
	SetLocalOrigin(pOwner->WorldSpaceCenter() + Vector (0 , 0 , 32));
	SetLocalAngles(pOwner->GetLocalAngles());
	SetOwnerEntity(pOwner);

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CBloodSplat::Think( void )
{
	trace_t	tr;	
	
	if (g_Language.GetInt() != LANGUAGE_GERMAN)
	{
		CBasePlayer *pPlayer;
		pPlayer = ToBasePlayer(GetOwnerEntity());

		Vector forward;
		AngleVectors(GetAbsAngles(), &forward);

		UTIL_TraceLine (GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
			MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}

	UTIL_Remove(this);
}

/*---------------------------------------------------
	GiveNamedItem()
	Crear y otorgarle un objeto al usuario.
---------------------------------------------------*/
CBaseEntity	*CBasePlayer::GiveNamedItem(const char *pszName, int iSubType)
{
	// Al parecer el jugador ya tiene este objeto.
	if (Weapon_OwnsThisType(pszName, iSubType))
		return NULL;

	EHANDLE pent;
	pent = CreateEntityByName(pszName);

	if (pent == NULL)
		return NULL;

	pent->SetLocalOrigin(GetLocalOrigin());
	pent->AddSpawnFlags(SF_NORESPAWN);

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>((CBaseEntity*)pent);

	if (pWeapon)
		pWeapon->SetSubType( iSubType );

	DispatchSpawn(pent);

	if (pent != NULL && !(pent->IsMarkedForDeletion())) 
		pent->Touch( this );

	return pent;
}

/*---------------------------------------------------
	FindEntityClassForward()
	Returns the nearest COLLIBALE entity in front of the player
---------------------------------------------------*/
CBaseEntity *FindEntityClassForward( CBasePlayer *pMe, char *classname )
{
	trace_t tr;
	Vector forward;

	pMe->EyeVectors(&forward);

	UTIL_TraceLine(pMe->EyePosition(),
		pMe->EyePosition() + forward * MAX_COORD_RANGE,
		MASK_SOLID, pMe, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
	{
		CBaseEntity *pHit = tr.m_pEnt;

		if (FClassnameIs(pHit,classname))
			return pHit;
	}

	return NULL;
}

/*---------------------------------------------------
	FindEntityForward()
	Returns the nearest COLLIBALE entity in front of the player
	that has a clear line of sight. If HULL is true, the trace will
	hit the collision hull of entities. Otherwise, the trace will hit hitboxes
---------------------------------------------------*/
CBaseEntity *FindEntityForward(CBasePlayer *pMe, bool fHull)
{
	if (pMe)
	{
		trace_t tr;
		Vector forward;
		int mask;

		if( fHull )
			mask = MASK_SOLID;
		else
			mask = MASK_SHOT;

		pMe->EyeVectors(&forward);
		UTIL_TraceLine(pMe->EyePosition(),
			pMe->EyePosition() + forward * MAX_COORD_RANGE,
			mask, pMe, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
			return tr.m_pEnt;
	}

	return NULL;

}

/*---------------------------------------------------
	FindPickerEntityClass()
	Finds the nearest entity in front of the player of the given
	classname, preferring collidable entities, but allows selection of 
	enities that are on the other side of walls or objects
---------------------------------------------------*/
CBaseEntity *FindPickerEntityClass( CBasePlayer *pPlayer, char *classname )
{
	CBaseEntity *pEntity = FindEntityClassForward(pPlayer, classname);

	if (!pEntity) 
	{
		Vector forward;
		Vector origin;

		pPlayer->EyeVectors(&forward);

		origin = pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityClassNearestFacing( origin, forward,0.95,classname);
	}

	return pEntity;
}

/*---------------------------------------------------
	FindPickerEntityClass()
	Finds the nearest entity in front of the player, preferring
	collidable entities, but allows selection of enities that are
	on the other side of walls or objects
---------------------------------------------------*/
CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	CBaseEntity *pEntity = FindEntityForward( pPlayer, true );

	if (!pEntity) 
	{
		Vector forward;
		Vector origin;

		pPlayer->EyeVectors(&forward);

		origin	= pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityNearestFacing( origin, forward,0.95);
	}

	return pEntity;
}

/*---------------------------------------------------
	FindPickerAINode()
	Finds the nearest node in front of the player
---------------------------------------------------*/
CAI_Node *FindPickerAINode( CBasePlayer *pPlayer, NodeType_e nNodeType )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors(&forward);
	origin = pPlayer->EyePosition();

	return g_pAINetworkManager->GetEditOps()->FindAINodeNearestFacing(origin, forward,0.90, nNodeType);
}

/*---------------------------------------------------
	FindPickerAILink()
	Finds the nearest link in front of the player
---------------------------------------------------*/
CAI_Link *FindPickerAILink( CBasePlayer* pPlayer )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors(&forward);
	origin = pPlayer->EyePosition();

	return g_pAINetworkManager->GetEditOps()->FindAILinkNearestFacing(origin, forward,0.90);
}

/*---------------------------------------------------
	ForceClientDllUpdate()
	Actualizar el estado del lado de cliente para sincronizar correctamente los dos lados.
	�til para mantener el orden en una grabaci�n.
---------------------------------------------------*/
void CBasePlayer::ForceClientDllUpdate( void )
{
	m_iClientBattery = -1;
	m_iTrain		|= TRAIN_NEW;
	m_fWeapon		= false;
	m_fInitHUD		= true;

	UpdateClientData();
	UTIL_RestartAmbientSounds();
}

/*---------------------------------------------------
	ImpulseCommands()
	Ejecutar comandos del malvado Impulse
---------------------------------------------------*/
void CBasePlayer::ImpulseCommands()
{
	trace_t	tr;
		
	int iImpulse = (int)m_nImpulse;

	switch (iImpulse)
	{
		// impulse 100 - Encender o apagar la linterna.
		case 100:
		{
			if (FlashlightIsOn())
				FlashlightTurnOff();
			else 
				FlashlightTurnOn();
		}
		break;

		// impulse 200 - Mostrar o ocultar el arma activa.
		case 200:
		{
			if (!sv_cheats->GetBool())
				break;

			CBaseCombatWeapon *pWeapon;
			pWeapon = GetActiveWeapon();
			
			if(pWeapon->IsEffectActive(EF_NODRAW))
				pWeapon->Deploy();
			else
				pWeapon->Holster();
		}
		break;

		// impulse 201 - Pintar.
		case 201:	
		{
			if (gpGlobals->curtime < m_flNextDecalTime)
				break;

			Vector forward;
			EyeVectors(&forward);

				UTIL_TraceLine ( EyePosition(), 
					EyePosition() + forward * 128, 
					MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

			if (tr.fraction != 1.0)
			{
				m_flNextDecalTime = gpGlobals->curtime + decalfrequency.GetFloat();
				CSprayCan *pCan = CREATE_UNSAVED_ENTITY(CSprayCan, "spraycan");
				pCan->Spawn(this);
			}
		}
		break;

		// Reproducir sonido
		case 202:
		{
			if (gpGlobals->curtime < m_flNextDecalTime)
				break;

			EntityMessageBegin(this);
				WRITE_BYTE(PLAY_PLAYER_JINGLE);
			MessageEnd();

			m_flNextDecalTime = gpGlobals->curtime + decalfrequency.GetFloat();
		}
		break;
	
		default:
			CheatImpulseCommands(iImpulse);
		break;
	}
	
	m_nImpulse = 0;
}

#ifdef HL2_EPISODIC

	/*---------------------------------------------------
		CreateJalopy()
		Crear un Jeep enfrente del usuario.
	---------------------------------------------------*/
	static void CreateJalopy( CBasePlayer *pPlayer )
	{
		Vector vecForward;
		AngleVectors(pPlayer->EyeAngles(), &vecForward);

		CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName("prop_vehicle_jeep");

		if (pJeep)
		{
			Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
			QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );

			pJeep->SetAbsOrigin(vecOrigin);
			pJeep->SetAbsAngles(vecAngles);
			pJeep->KeyValue("model", "models/vehicle.mdl");
			pJeep->KeyValue("solid", "6");
			pJeep->KeyValue("targetname", "jeep");
			pJeep->KeyValue("vehiclescript", "scripts/vehicles/jalopy.txt");
			DispatchSpawn(pJeep);
			pJeep->Activate();
			pJeep->Teleport(&vecOrigin, &vecAngles, NULL);
		}
	}

	void CC_CH_CreateJalopy( void )
	{
		CBasePlayer *pPlayer = UTIL_GetCommandClient();

		if (!pPlayer)
			return;

		CreateJalopy(pPlayer);
	}

	static ConCommand ch_createjalopy("ch_createjalopy", CC_CH_CreateJalopy, "Spawn jalopy in front of the player.", FCVAR_CHEAT);

#endif // HL2_EPISODIC

/*---------------------------------------------------
	CreateJeep()
	Crear un Jeep enfrente del usuario.
---------------------------------------------------*/
static void CreateJeep( CBasePlayer *pPlayer )
{
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );

	#if defined ( SP_SDK )
		CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName("prop_vehicle_hl2buggy");
	#else
		CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName("prop_vehicle_jeep");
	#endif

	if (pJeep)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
		QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);

		pJeep->SetAbsOrigin(vecOrigin);
		pJeep->SetAbsAngles(vecAngles);
		pJeep->KeyValue("model", "models/buggy.mdl");
		pJeep->KeyValue("solid", "6");
		pJeep->KeyValue("targetname", "hl2buggy");
		pJeep->KeyValue("vehiclescript", "scripts/vehicles/jeep_test.txt");
		DispatchSpawn(pJeep);
		pJeep->Activate();
		pJeep->Teleport(&vecOrigin, &vecAngles, NULL);
	}
}

void CC_CH_CreateJeep(void)
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if (!pPlayer)
		return;

	CreateJeep( pPlayer );
}

static ConCommand ch_createjeep("ch_createjeep", CC_CH_CreateJeep, "Spawn jeep in front of the player.", FCVAR_CHEAT);


/*---------------------------------------------------
	CreateAirboat()
	Crear un bote enfrente del usuario.
---------------------------------------------------*/
static void CreateAirboat( CBasePlayer *pPlayer )
{
	Vector vecForward;
	AngleVectors(pPlayer->EyeAngles(), &vecForward);

	CBaseEntity *pJeep = (CBaseEntity* )CreateEntityByName("prop_vehicle_airboat");

	if (pJeep)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector( 0,0,64 );
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );

		pJeep->SetAbsOrigin(vecOrigin);
		pJeep->SetAbsAngles(vecAngles);
		pJeep->KeyValue("model", "models/airboat.mdl");
		pJeep->KeyValue("solid", "6");
		pJeep->KeyValue("targetname", "airboat");
		pJeep->KeyValue("vehiclescript", "scripts/vehicles/airboat.txt");
		DispatchSpawn(pJeep);
		pJeep->Activate();
	}
}

void CC_CH_CreateAirboat( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if (!pPlayer)
		return;

	CreateAirboat(pPlayer);
}

static ConCommand ch_createairboat( "ch_createairboat", CC_CH_CreateAirboat, "Spawn airboat in front of the player.", FCVAR_CHEAT );

/*---------------------------------------------------
	CheatImpulseCommands()
	Ejecutar comandos del malvado Impulse
---------------------------------------------------*/
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	#if !defined( HLDEMO_BUILD )
		if (!sv_cheats->GetBool())
			return;

		CBaseEntity *pEntity;
		trace_t tr;

		switch (iImpulse)
		{
			// Grunt-o-matic ?
			case 50:
			{
				if (!giPrecacheGrunt)
				{
					giPrecacheGrunt = 1;
					Msg( "You must now restart to use Grunt-o-matic.\n");
				}
				else
				{
					Vector forward = UTIL_YawToVector( EyeAngles().y );
					Create("NPC_human_grunt", GetLocalOrigin() + forward * 128, GetLocalAngles());
				}
			}
			break;

			// Obtener el objeto: weapon_cubemap
			case 60:
				GiveNamedItem("weapon_cubemap");
			break;

			// Crear un Jeep
			case 70:
				CreateJeep(this);
			break;

			// Crear un bote.
			case 80:
				CreateAirboat(this);
			break;

			// Ser Steve Jobs y obtener todas las armas.
			case 102:
			{
				gEvilImpulse101 = true;
				EquipSuit();

				GiveAmmo(255,	"Pistol");
				GiveAmmo(255,	"AR2");
				GiveAmmo(5,		"AR2AltFire");
				GiveAmmo(255,	"SMG1");
				GiveAmmo(255,	"Buckshot");
				GiveAmmo(3,		"smg1_grenade");
				GiveAmmo(3,		"rpg_round");
				GiveAmmo(5,		"grenade");
				GiveAmmo(32,	"357");
				GiveAmmo(16,	"XBowBolt");

				// InSource
				GiveAmmo(150,	"GaussEnergy1");
				GiveAmmo(150,	"AlyxGun");
					
				GiveNamedItem("weapon_smg1");
				GiveNamedItem("weapon_frag");
				GiveNamedItem("weapon_crowbar");
				GiveNamedItem("weapon_pistol");
				GiveNamedItem("weapon_ar2");
				GiveNamedItem("weapon_shotgun");
				GiveNamedItem("weapon_physcannon");
				GiveNamedItem("weapon_bugbait");
				GiveNamedItem("weapon_rpg");
				GiveNamedItem("weapon_357");
				GiveNamedItem("weapon_crossbow");

				// InSource
				GiveNamedItem("weapon_mp5");
				GiveNamedItem("weapon_slam");
				GiveNamedItem("weapon_gauss");
				GiveNamedItem("weapon_alyxgun");

				#ifdef HL2_EPISODIC
					GiveAmmo(5,	"Hopwire");
				#endif

				if (GetHealth() < 100)
					TakeHealth(50, DMG_GENERIC);
		
				gEvilImpulse101	= false;
			}
			break;

			// Hacer aparacer 10 craneos.
			case 110:
				CGib::SpawnRandomGibs(this, 10, GIB_HUMAN);
			break;

			// Llamar la atenci�n de los NPC ?
			case 120:
			{
				pEntity = FindEntityForward(this, true);

				if (pEntity)
				{
					CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

					if (pNPC)
						pNPC->ReportAIState();
				}
			}
			break;

			// Obtener informaci�n de la entidad.
			case 130:
			{
				pEntity = FindEntityForward(this, true);

				if (pEntity)
				{
					Msg("Classname: %s", pEntity->GetClassname());
			
					if (pEntity->GetEntityName() != NULL_STRING)
						Msg( " - Name: %s\n", STRING( pEntity->GetEntityName() ) );
					
					else
						Msg( " - Name: No Targetname\n" );

					if (pEntity->m_iParent != NULL_STRING)
						Msg( "Parent: %s\n", STRING(pEntity->m_iParent) );

					Msg("Model: %s\n", STRING( pEntity->GetModelName()));

					if (pEntity->m_iGlobalname != NULL_STRING)
						Msg( "Globalname: %s\n", STRING(pEntity->m_iGlobalname) );
				}
			}
			break;

			// Obtener la textura de la entidad.
			case 140:
			{
				trace_t tr;
				edict_t		*pWorld = engine->PEntityOfEntIndex( 0 );

				Vector start = EyePosition();
				Vector forward;

				EyeVectors( &forward );
				Vector end = start + forward * 1024;

				UTIL_TraceLine(start, end, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
				
				if (tr.m_pEnt)
					pWorld = tr.m_pEnt->edict();

				const char *pTextureName = tr.surface.name;

				if (pTextureName)
					Msg( "Texture: %s\n", pTextureName );
			}
			break;

			// Debugear un NPC.
			case 150:
			{
				pEntity = FindEntityForward(this, true);

				if (pEntity)
				{
					CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

					if (pNPC != NULL)
					{
						Msg("Debugging %s (0x%x)\n", pNPC->GetClassname(), pNPC);
						CAI_BaseNPC::SetDebugNPC( pNPC );
					}
				}
			}
			break;

			case 160:
				Create("node_viewer_fly", GetLocalOrigin(), GetLocalAngles());
			break;

			case 170:
				Create("node_viewer_large", GetLocalOrigin(), GetLocalAngles());
			break;

			case 180:
				Create("node_viewer_human", GetLocalOrigin(), GetLocalAngles());
			break;

			// Remover una entidad
			case 200:
			{
				pEntity = FindEntityForward(this, true);

				if (pEntity)
					UTIL_Remove(pEntity);
			}
			break;
		}
	#endif
}


/*---------------------------------------------------
	ClientCommand()
	Ejecutar comando.
---------------------------------------------------*/
bool CBasePlayer::ClientCommand(const CCommand &args)
{
	const char *cmd = args[0];

	#ifdef _DEBUG
		if(stricmp(cmd, "test_SmokeGrenade") == 0)
		{
			if ( sv_cheats && sv_cheats->GetBool() )
			{
				ParticleSmokeGrenade *pSmoke = dynamic_cast<ParticleSmokeGrenade*>( CreateEntityByName(PARTICLESMOKEGRENADE_ENTITYNAME) );
				if ( pSmoke )
				{
					Vector vForward;
					AngleVectors( GetLocalAngles(), &vForward );
					vForward.z = 0;
					VectorNormalize( vForward );

					pSmoke->SetLocalOrigin( GetLocalOrigin() + vForward * 100 );
					pSmoke->SetFadeTime(25, 30);	// Fade out between 25 seconds and 30 seconds.
					pSmoke->Activate();
					pSmoke->SetLifetime(30);
					pSmoke->FillVolume();

					return true;
				}
			}
		}
		else
	#endif // _DEBUG
	if( stricmp( cmd, "vehicleRole" ) == 0 )
	{
		// Get the vehicle role value.
		if ( args.ArgC() == 2 )
		{
			// Check to see if a player is in a vehicle.
			if ( IsInAVehicle() )
			{
				int nRole = atoi( args[1] );
				IServerVehicle *pVehicle = GetVehicle();
				if ( pVehicle )
				{
					// Only switch roles if role is empty!
					if ( !pVehicle->GetPassenger( nRole ) )
					{
						LeaveVehicle();
						GetInVehicle( pVehicle, nRole );
					}
				}			
			}

			return true;
		}
	}
	else if ( stricmp( cmd, "spectate" ) == 0 ) // join spectator team & start observer mode
	{
		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		ConVarRef mp_allowspectators( "mp_allowspectators" );
		if ( mp_allowspectators.IsValid() )
		{
			if ( ( mp_allowspectators.GetBool() == false ) && !IsHLTV() )
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
				return true;
			}
		}

		if ( !IsDead() )
		{
			CommitSuicide();	// kill player
		}

		RemoveAllItems( true );

		ChangeTeam( TEAM_SPECTATOR );

		StartObserverMode( OBS_MODE_ROAMING );
		return true;
	}
	else if ( stricmp( cmd, "spec_mode" ) == 0 ) // new observer mode
	{
		int mode;

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
			return true;
		}

		// not allowed to change spectator modes when mp_fadetoblack is being used
		if ( mp_fadetoblack.GetBool() )
		{
			if ( GetTeamNumber() > TEAM_SPECTATOR )
				return true;
		}

		// check for parameters.
		if ( args.ArgC() >= 2 )
		{
			mode = atoi( args[1] );

			if ( mode < OBS_MODE_IN_EYE || mode > LAST_PLAYER_OBSERVERMODE )
				mode = OBS_MODE_IN_EYE;
		}
		else
		{
			// switch to next spec mode if no parameter given
 			mode = GetObserverMode() + 1;
			
			if ( mode > LAST_PLAYER_OBSERVERMODE )
			{
				mode = OBS_MODE_IN_EYE;
			}
			else if ( mode < OBS_MODE_IN_EYE )
			{
				mode = OBS_MODE_ROAMING;
			}

		}
	
		// don't allow input while player or death cam animation
		if ( GetObserverMode() > OBS_MODE_DEATHCAM )
		{
			// set new spectator mode, don't allow OBS_MODE_NONE
			if ( !SetObserverMode( mode ) )
				ClientPrint( this, HUD_PRINTCONSOLE, "#Spectator_Mode_Unkown");
			else
				engine->ClientCommand( edict(), "cl_spec_mode %d", mode );
		}
		else
		{
			// remember spectator mode for later use
			m_iObserverLastMode = mode;
			engine->ClientCommand( edict(), "cl_spec_mode %d", mode );
		}

		return true;
	}
	else if ( stricmp( cmd, "spec_next" ) == 0 ) // chase next player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( false );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
		}
		
		return true;
	}
	else if ( stricmp( cmd, "spec_prev" ) == 0 ) // chase prevoius player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( true );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
		}
		
		return true;
	}
	
	else if ( stricmp( cmd, "spec_player" ) == 0 ) // chase next player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED && args.ArgC() == 2 )
		{
			int index = atoi( args[1] );

			CBasePlayer * target;

			if ( index == 0 )
			{
				target = UTIL_PlayerByName( args[1] );
			}
			else
			{
				target = UTIL_PlayerByIndex( index );
			}

			if ( IsValidObserverTarget( target ) )
			{
				SetObserverTarget( target );
			}
		}
		
		return true;
	}

	else if ( stricmp( cmd, "spec_goto" ) == 0 ) // chase next player
	{
		if ( ( GetObserverMode() == OBS_MODE_FIXED ||
			   GetObserverMode() == OBS_MODE_ROAMING ) &&
			 args.ArgC() == 6 )
		{
			Vector origin;
			origin.x = atof( args[1] );
			origin.y = atof( args[2] );
			origin.z = atof( args[3] );

			QAngle angle;
			angle.x = atof( args[4] );
			angle.y = atof( args[5] );
			angle.z = 0.0f;

			JumptoPosition( origin, angle );
		}
		
		return true;
	}
	else if ( stricmp( cmd, "playerperf" ) == 0 )
	{
		int nRecip = entindex();
		if ( args.ArgC() >= 2 )
		{
			nRecip = clamp( Q_atoi( args.Arg( 1 ) ), 1, gpGlobals->maxClients );
		}
		int nRecords = -1; // all
		if ( args.ArgC() >= 3 )
		{
			nRecords = max( Q_atoi( args.Arg( 2 ) ), 1 );
		}

		CBasePlayer *pl = UTIL_PlayerByIndex( nRecip );
		if ( pl )
		{
			pl->DumpPerfToRecipient( this, nRecords );
		}
		return true;
	}

	return false;
}

extern bool UTIL_ItemCanBeTouchedByPlayer( CBaseEntity *pItem, CBasePlayer *pPlayer );

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CBasePlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Act differently in the episodes
	if ( hl2_episodic.GetBool() )
	{
		// Don't let the player touch the item unless unobstructed
		if ( !UTIL_ItemCanBeTouchedByPlayer( pWeapon, this ) && !gEvilImpulse101 )
			return false;
	}
	else
	{
		// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
		if( pWeapon->FVisible( this, MASK_SOLID ) == false && !(GetFlags() & FL_NOTARGET) )
			return false;
	}
	
	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if (Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType())) 
	{
		if( Weapon_EquipAmmoOnly( pWeapon ) )
		{
			// Only remove me if I have no ammo left
			if ( pWeapon->HasPrimaryAmmo() )
				return false;

			UTIL_Remove( pWeapon );
			return true;
		}
		else
		{
			return false;
		}
	}
	// -------------------------
	// Otherwise take the weapon
	// -------------------------
	else 
	{
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->AddEffects( EF_NODRAW );

		Weapon_Equip( pWeapon );
		if ( IsInAVehicle() )
		{
			pWeapon->Holster();
		}
		else
		{
#ifdef HL2_DLL

			if ( IsX360() )
			{
				CFmtStr hint;
				hint.sprintf( "#valve_hint_select_%s", pWeapon->GetClassname() );
				UTIL_HudHintText( this, hint.Access() );
			}

			// Always switch to a newly-picked up weapon
			if ( !PlayerHasMegaPhysCannon() )
			{
				// If it uses clips, load it full. (this is the first time you've picked up this type of weapon)
				if ( pWeapon->UsesClipsForAmmo1() )
				{
					pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
				}

				Weapon_Switch( pWeapon );
			}
#endif
		}
		return true;
	}
}


bool CBasePlayer::RemovePlayerItem( CBaseCombatWeapon *pItem )
{
	if (GetActiveWeapon() == pItem)
	{
		ResetAutoaim( );
		pItem->Holster( );
		pItem->SetNextThink( TICK_NEVER_THINK );; // crowbar may be trying to swing again, etc
		pItem->SetThink( NULL );
	}

	if ( m_hLastWeapon.Get() == pItem )
	{
		Weapon_SetLast( NULL );
	}

	return Weapon_Detach( pItem );
}


//-----------------------------------------------------------------------------
// Purpose: Hides or shows the player's view model. The "r_drawviewmodel" cvar
//			can still hide the viewmodel even if this is set to true.
// Input  : bShow - true to show, false to hide the view model.
//-----------------------------------------------------------------------------
void CBasePlayer::ShowViewModel(bool bShow)
{
	m_Local.m_bDrawViewmodel = bShow;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bDraw - 
//-----------------------------------------------------------------------------
void CBasePlayer::ShowCrosshair( bool bShow )
{
	if ( bShow )
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CBasePlayer::BodyAngles()
{
	return EyeAngles();
}

//------------------------------------------------------------------------------
// Purpose : Add noise to BodyTarget() to give enemy a better chance of
//			 getting a clear shot when the player is peeking above a hole
//			 or behind a ladder (eventually the randomly-picked point 
//			 along the spine will be one that is exposed above the hole or 
//			 between rungs of a ladder.)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CBasePlayer::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	if ( IsInAVehicle() )
	{
		return GetVehicle()->GetVehicleEnt()->BodyTarget( posSrc, bNoisy );
	}
	if (bNoisy)
	{
		return GetAbsOrigin() + (GetViewOffset() * random->RandomFloat( 0.7, 1.0 )); 
	}
	else
	{
		return EyePosition(); 
	}
};		

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData( void )
{
	CSingleUserRecipientFilter user( this );
	user.MakeReliable();

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		UserMessageBegin( user, "ResetHUD" );
			WRITE_BYTE( 0 );
		MessageEnd();

		if ( !m_fGameHUDInitialized )
		{
			g_pGameRules->InitHUD( this );
			InitHUD();
			m_fGameHUDInitialized = true;
			if ( g_pGameRules->IsMultiplayer() )
			{
				variant_t value;
				g_EventQueue.AddEvent( "game_player_manager", "OnPlayerJoin", value, 0, this, this );
			}
		}

		variant_t value;
		g_EventQueue.AddEvent( "game_player_manager", "OnPlayerSpawn", value, 0, this, this );
	}

	// HACKHACK -- send the message to display the game title
	CWorld *world = GetWorldEntity();
	if ( world && world->GetDisplayTitle() )
	{
		UserMessageBegin( user, "GameTitle" );
		MessageEnd();
		world->SetDisplayTitle( false );
	}

	if (m_ArmorValue != m_iClientBattery)
	{
		m_iClientBattery = m_ArmorValue;

		// send "battery" update message
		if ( usermessages->LookupUserMessage( "Battery" ) != -1 )
		{
			UserMessageBegin( user, "Battery" );
				WRITE_SHORT( (int)m_ArmorValue);
			MessageEnd();
		}
	}

	CheckTrainUpdate();

	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}

	// update the client with our poison state
	m_Local.m_bPoisoned = ( m_bitsDamageType & DMG_POISON ) 
						&& ( m_nPoisonDmg > m_nPoisonRestored ) 
						&& ( m_iHealth < 100 );

	// Check if the bonus progress HUD element should be displayed
	if ( m_iBonusChallenge == 0 && m_iBonusProgress == 0 && !( m_Local.m_iHideHUD & HIDEHUD_BONUS_PROGRESS ) )
		m_Local.m_iHideHUD |= HIDEHUD_BONUS_PROGRESS;
	if ( ( m_iBonusChallenge != 0 )&& ( m_Local.m_iHideHUD & HIDEHUD_BONUS_PROGRESS ) )
		m_Local.m_iHideHUD &= ~HIDEHUD_BONUS_PROGRESS;

	// Let any global rules update the HUD, too
	g_pGameRules->UpdateClientData( this );
}

void CBasePlayer::RumbleEffect( unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags )
{
	if( !IsAlive() )
		return;

	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	UserMessageBegin( filter, "Rumble" );
	WRITE_BYTE( index );
	WRITE_BYTE( rumbleData );
	WRITE_BYTE( rumbleFlags	);
	MessageEnd();
}

void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		AddFlag( FL_FROZEN );
	else
		RemoveFlag( FL_FROZEN );

}

void CBasePlayer::CheckTrainUpdate( void )
{
	if ( ( m_iTrain & TRAIN_NEW ) )
	{
		CSingleUserRecipientFilter user( this );
		user.MakeReliable();

		// send "Train" update message
		UserMessageBegin( user, "Train" );
			WRITE_BYTE(m_iTrain & 0xF);
		MessageEnd();

		m_iTrain &= ~TRAIN_NEW;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the player should autoaim or not
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ShouldAutoaim( void )
{
	// cannot be in multiplayer
	if ( gpGlobals->maxClients > 1 )
		return false;

	// autoaiming is only for easy and medium skill
	return ( IsX360() || !g_pGameRules->IsSkillLevel(SKILL_HARD) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetAutoaimVector( float flScale )
{
	autoaim_params_t params;

	params.m_fScale = flScale;
	params.m_fMaxDist = autoaim_max_dist.GetFloat();

	GetAutoaimVector( params );
	return params.m_vecAutoAimDir;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetAutoaimVector( float flScale, float flMaxDist )
{
	autoaim_params_t params;

	params.m_fScale = flScale;
	params.m_fMaxDist = flMaxDist;

	GetAutoaimVector( params );
	return params.m_vecAutoAimDir;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBasePlayer::GetAutoaimVector( autoaim_params_t &params )
{
	// Assume autoaim will not be assisting.
	params.m_bAutoAimAssisting = false;

	if ( ( ShouldAutoaim() == false ) || ( params.m_fScale == AUTOAIM_SCALE_DIRECT_ONLY ) )
	{
		Vector	forward;
		AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );

		params.m_vecAutoAimDir = forward;
		params.m_hAutoAimEntity.Set(NULL);
		params.m_vecAutoAimPoint = vec3_invalid;
		params.m_bAutoAimAssisting = false;
		return;
	}

	Vector vecSrc	= Weapon_ShootPosition( );

	m_vecAutoAim.Init( 0.0f, 0.0f, 0.0f );

	QAngle angles = AutoaimDeflection( vecSrc, params );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = false;

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;

	Vector	forward;

	if( IsInAVehicle() && g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		m_vecAutoAim = angles;
		AngleVectors( EyeAngles() + m_vecAutoAim, &forward );
	}
	else
	{
		// always use non-sticky autoaim
		m_vecAutoAim = angles * 0.9f;
		AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle + m_vecAutoAim, &forward );
	}

	params.m_vecAutoAimDir = forward;
}

//-----------------------------------------------------------------------------
// Targets represent themselves to autoaim as a viewplane-parallel disc with
// a radius specified by the target. The player then modifies this radius
// to achieve more or less aggressive aiming assistance
//-----------------------------------------------------------------------------
float CBasePlayer::GetAutoaimScore( const Vector &eyePosition, const Vector &viewDir, const Vector &vecTarget, CBaseEntity *pTarget, float fScale, CBaseCombatWeapon *pActiveWeapon )
{
	float radiusSqr;
	float targetRadius = pTarget->GetAutoAimRadius() * fScale;

	if( pActiveWeapon != NULL )
		targetRadius *= pActiveWeapon->WeaponAutoAimScale();

	float targetRadiusSqr = Square( targetRadius );

	Vector vecNearestPoint = PointOnLineNearestPoint( eyePosition, eyePosition + viewDir * 8192, vecTarget );
	Vector vecDiff = vecTarget - vecNearestPoint;

	radiusSqr = vecDiff.LengthSqr();

	if( radiusSqr <= targetRadiusSqr )
	{
		float score;

		score = 1.0f - (radiusSqr / targetRadiusSqr);

		Assert( score >= 0.0f && score <= 1.0f );
		return score;
	}
	
	// 0 means no score- doesn't qualify for autoaim.
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSrc - 
//			flDist - 
//			flDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
QAngle CBasePlayer::AutoaimDeflection( Vector &vecSrc, autoaim_params_t &params )
{
	float		bestscore;
	float		score;
	QAngle		eyeAngles;
	Vector		bestdir;
	CBaseEntity	*bestent;
	trace_t		tr;
	Vector		v_forward, v_right, v_up;

	if ( ShouldAutoaim() == false )
	{
		m_fOnTarget = false;
		return vec3_angle;
	}

	eyeAngles = EyeAngles();
	AngleVectors( eyeAngles + m_Local.m_vecPunchAngle + m_vecAutoAim, &v_forward, &v_right, &v_up );

	// try all possible entities
	bestdir = v_forward;
	bestscore = 0.0f;
	bestent = NULL;

	//Reset this data
	m_fOnTarget					= false;
	params.m_bOnTargetNatural	= false;
	
	CBaseEntity *pIgnore = NULL;

	if( IsInAVehicle() )
	{
		pIgnore = GetVehicleEntity();
	}

	CTraceFilterSkipTwoEntities traceFilter( this, pIgnore, COLLISION_GROUP_NONE );

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * MAX_COORD_FLOAT, MASK_SHOT, &traceFilter, &tr );

	CBaseEntity *pEntHit = tr.m_pEnt;

	if ( pEntHit && pEntHit->m_takedamage != DAMAGE_NO && pEntHit->GetHealth() > 0 )
	{
		// don't look through water
		if (!((GetWaterLevel() != 3 && pEntHit->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntHit->GetWaterLevel() == 0)))
		{
			if( pEntHit->ShouldAttractAutoAim(this) )
			{
				bool bAimAtThis = true;

				if( pEntHit->IsNPC() && g_pGameRules->GetAutoAimMode() > AUTOAIM_NONE )
				{
					int iRelationType = GetDefaultRelationshipDisposition( pEntHit->Classify() );

					if( iRelationType != D_HT )
					{
						bAimAtThis = false;
					}
				}

				if( bAimAtThis )
				{
					if ( pEntHit->GetFlags() & FL_AIMTARGET )
					{
						m_fOnTarget = true;
					}

					// Player is already on target naturally, don't autoaim.
					// Fill out the autoaim_params_t struct, though.
					params.m_hAutoAimEntity.Set(pEntHit);
					params.m_vecAutoAimDir = bestdir;
					params.m_vecAutoAimPoint = tr.endpos;
					params.m_bAutoAimAssisting = false;
					params.m_bOnTargetNatural = true;
					return vec3_angle;
				}
			}

			//Fall through and look for an autoaim ent.
		}
	}

	int count = AimTarget_ListCount();
	if ( count )
	{
		CBaseEntity **pList = (CBaseEntity **)stackalloc( sizeof(CBaseEntity *) * count );
		AimTarget_ListCopy( pList, count );

		for ( int i = 0; i < count; i++ )
		{
			Vector center;
			Vector dir;
			CBaseEntity *pEntity = pList[i];

			// Don't autoaim at anything that doesn't want to be.
			if( !pEntity->ShouldAttractAutoAim(this) )
				continue;

			// Don't shoot yourself
			if ( pEntity == this )
				continue;

			if ( (pEntity->IsNPC() && !pEntity->IsAlive()) || !pEntity->edict() )
				continue;

			if ( !g_pGameRules->ShouldAutoAim( this, pEntity->edict() ) )
				continue;

			// don't look through water
			if ((GetWaterLevel() != 3 && pEntity->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntity->GetWaterLevel() == 0))
				continue;

			if( pEntity->MyNPCPointer() )
			{
				// If this entity is an NPC, only aim if it is an enemy.
				if ( IRelationType( pEntity ) != D_HT )
				{
					if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
						// Msg( "friend\n");
						continue;
				}
			}

			// Don't autoaim at the noisy bodytarget, this makes the autoaim crosshair wobble.
			//center = pEntity->BodyTarget( vecSrc, false );
			center = pEntity->WorldSpaceCenter();

			dir = (center - vecSrc);
			
			float dist = dir.Length2D();
			VectorNormalize( dir );

			// Skip if out of range.
			if( dist > params.m_fMaxDist )
				continue;

			float dot = DotProduct (dir, v_forward );

			// make sure it's in front of the player
			if( dot < 0 )
				continue;

			if( !(pEntity->GetFlags() & FL_FLY) )
			{
				// Refuse to take wild shots at targets far from reticle.
				if( GetActiveWeapon() != NULL && dot < GetActiveWeapon()->GetMaxAutoAimDeflection() )
				{
					// Be lenient if the player is looking down, though. 30 degrees through 90 degrees of pitch.
					// (90 degrees is looking down at player's own 'feet'. Looking straight ahead is 0 degrees pitch.
					// This was done for XBox to make it easier to fight headcrabs around the player's feet.
					if( eyeAngles.x < 30.0f || eyeAngles.x > 90.0f || g_pGameRules->GetAutoAimMode() != AUTOAIM_ON_CONSOLE )
					{
						continue;
					}
				}
			}

			score = GetAutoaimScore(vecSrc, v_forward, pEntity->GetAutoAimCenter(), pEntity, params.m_fScale, GetActiveWeapon() );

			if( score <= bestscore )
			{
				continue;
			}

			UTIL_TraceLine( vecSrc, center, MASK_SHOT, &traceFilter, &tr );

			if (tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			{
				// Msg( "hit %s, can't see %s\n", STRING( tr.u.ent->classname ), STRING( pEdict->classname ) );
				continue;
			}

			// This is the best candidate so far.
			bestscore = score;
			bestent = pEntity;
			bestdir = dir;
		}
		if ( bestent )
		{
			QAngle bestang;

			VectorAngles( bestdir, bestang );

			if( IsInAVehicle() )
			{
				bestang -= EyeAngles();
			}
			else
			{
				bestang -= EyeAngles() - m_Local.m_vecPunchAngle;
			}

			m_fOnTarget = true;

			// Autoaim detected a target for us. Aim automatically at its bodytarget.
			params.m_hAutoAimEntity.Set(bestent);
			params.m_vecAutoAimDir = bestdir;
			params.m_vecAutoAimPoint = bestent->BodyTarget( vecSrc, false );
			params.m_bAutoAimAssisting = true;

			return bestang;
		}
	}

	return vec3_angle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ResetAutoaim( void )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
	m_fOnTarget = false;
}

// ==========================================================================
//	> Weapon stuff
// ==========================================================================

//-----------------------------------------------------------------------------
// Purpose: Override base class, player can always use weapon
// Input  : A weapon
// Output :	true or false
//-----------------------------------------------------------------------------
bool CBasePlayer::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Override to clear dropped weapon from the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	bool bWasActiveWeapon = false;
	if ( pWeapon == GetActiveWeapon() )
	{
		bWasActiveWeapon = true;
	}

	if ( pWeapon )
	{
		if ( bWasActiveWeapon )
		{
			pWeapon->SendWeaponAnim( ACT_VM_IDLE );
		}
	}

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if ( bWasActiveWeapon )
	{
		if (!SwitchToNextBestWeapon( NULL ))
		{
			CBaseViewModel *vm = GetViewModel();
			if ( vm )
			{
				vm->AddEffects( EF_NODRAW );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : weaponSlot - 
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_DropSlot( int weaponSlot )
{
	CBaseCombatWeapon *pWeapon;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );
		
		if ( pWeapon != NULL )
		{
			// If the slots match, it's already occupied
			if ( pWeapon->GetSlot() == weaponSlot )
			{
				Weapon_Drop( pWeapon, NULL, NULL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override to add weapon to the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	bool bShouldSwitch = g_pGameRules->FShouldSwitchWeapon( this, pWeapon );

#ifdef HL2_DLL
	if ( bShouldSwitch == false && PhysCannonGetHeldEntity( GetActiveWeapon() ) == pWeapon && 
		 Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType()) )
	{
		bShouldSwitch = true;
	}
#endif//HL2_DLL

	// should we switch to this item?
	if ( bShouldSwitch )
	{
		Weapon_Switch( pWeapon );
	}
}


//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
CBaseEntity *CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		if ( !GetWeapon(i) )
			continue;

		if ( FStrEq( pszItemName, GetWeapon(i)->GetClassname() ) )
		{
			return GetWeapon(i);
		}
	}

	return NULL;
}



//================================================================================
// TEAM HANDLING
//================================================================================
//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------

void CBasePlayer::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent)
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CBasePlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// if this is our current team, just abort
	if ( iTeamNum == GetTeamNumber() )
	{
		return;
	}

	// Immediately tell all clients that he's changing team. This has to be done
	// first, so that all user messages that follow as a result of the team change
	// come after this one, allowing the client to be prepared for them.
	IGameEvent * event = gameeventmanager->CreateEvent( "player_team" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("team", iTeamNum );
		event->SetInt("oldteam", GetTeamNumber() );
		event->SetInt("disconnect", IsDisconnecting());
		event->SetInt("autoteam", bAutoTeam );
		event->SetInt("silent", bSilent );
		event->SetString("name", GetPlayerName() );

		gameeventmanager->FireEvent( event );
	}

	// Remove him from his current team
	if ( GetTeam() )
	{
		GetTeam()->RemovePlayer( this );
	}

	// Are we being added to a team?
	if ( iTeamNum )
	{
		GetGlobalTeam( iTeamNum )->AddPlayer( this );
	}

	BaseClass::ChangeTeam( iTeamNum );
}



//-----------------------------------------------------------------------------
// Purpose: Locks a player to the spot; they can't move, shoot, or be hurt
//-----------------------------------------------------------------------------
void CBasePlayer::LockPlayerInPlace( void )
{
	if ( m_iPlayerLocked )
		return;

	AddFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_NONE );
	m_iPlayerLocked = true;

	// force a client data update, so that anything that has been done to
	// this player previously this frame won't get delayed in being sent
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: Unlocks a previously locked player
//-----------------------------------------------------------------------------
void CBasePlayer::UnlockPlayer( void )
{
	if ( !m_iPlayerLocked )
		return;

	RemoveFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_WALK );
	m_iPlayerLocked = false;
}

bool CBasePlayer::ClearUseEntity()
{
	if ( m_hUseEntity != NULL )
	{
		// Stop controlling the train/object
		// TODO: Send HUD Update
		m_hUseEntity->Use( this, this, USE_OFF, 0 );
		m_hUseEntity = NULL;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::HideViewModels( void )
{
	for ( int i = 0 ; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		vm->SetWeaponModel( NULL, NULL );
	}
}

class CStripWeapons : public CPointEntity
{
	DECLARE_CLASS( CStripWeapons, CPointEntity );
public:
	void InputStripWeapons(inputdata_t &data);
	void InputStripWeaponsAndSuit(inputdata_t &data);

	void StripWeapons(inputdata_t &data, bool stripSuit);
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

BEGIN_DATADESC( CStripWeapons )
	DEFINE_INPUTFUNC( FIELD_VOID, "Strip", InputStripWeapons ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StripWeaponsAndSuit", InputStripWeaponsAndSuit ),
END_DATADESC()
	

void CStripWeapons::InputStripWeapons(inputdata_t &data)
{
	StripWeapons(data, false);
}

void CStripWeapons::InputStripWeaponsAndSuit(inputdata_t &data)
{
	StripWeapons(data, true);
}

void CStripWeapons::StripWeapons(inputdata_t &data, bool stripSuit)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		pPlayer->RemoveAllItems( stripSuit );
	}
}


class CRevertSaved : public CPointEntity
{
	DECLARE_CLASS( CRevertSaved, CPointEntity );
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	LoadThink( void );

	DECLARE_DATADESC();

	inline	float	Duration( void ) { return m_Duration; }
	inline	float	HoldTime( void ) { return m_HoldTime; }
	inline	float	LoadTime( void ) { return m_loadTime; }

	inline	void	SetDuration( float duration ) { m_Duration = duration; }
	inline	void	SetHoldTime( float hold ) { m_HoldTime = hold; }
	inline	void	SetLoadTime( float time ) { m_loadTime = time; }

	//Inputs
	void InputReload(inputdata_t &data);

#ifdef HL1_DLL
	void	MessageThink( void );
	inline	float	MessageTime( void ) { return m_messageTime; }
	inline	void	SetMessageTime( float time ) { m_messageTime = time; }
#endif

private:

	float	m_loadTime;
	float	m_Duration;
	float	m_HoldTime;

#ifdef HL1_DLL
	string_t m_iszMessage;
	float	m_messageTime;
#endif
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

BEGIN_DATADESC( CRevertSaved )

#ifdef HL1_DLL
	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_messageTime, FIELD_FLOAT, "messagetime" ),	// These are not actual times, but durations, so save as floats

	DEFINE_FUNCTION( MessageThink ),
#endif

	DEFINE_KEYFIELD( m_loadTime, FIELD_FLOAT, "loadtime" ),
	DEFINE_KEYFIELD( m_Duration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_HoldTime, FIELD_FLOAT, "holdtime" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Reload", InputReload ),


	// Function Pointers
	DEFINE_FUNCTION( LoadThink ),

END_DATADESC()

CBaseEntity *CreatePlayerLoadSave( Vector vOrigin, float flDuration, float flHoldTime, float flLoadTime )
{
	CRevertSaved *pRevertSaved = (CRevertSaved *) CreateEntityByName( "player_loadsaved" );

	if ( pRevertSaved == NULL )
		return NULL;

	UTIL_SetOrigin( pRevertSaved, vOrigin );

	pRevertSaved->Spawn();
	pRevertSaved->SetDuration( flDuration );
	pRevertSaved->SetHoldTime( flHoldTime );
	pRevertSaved->SetLoadTime( flLoadTime );

	return pRevertSaved;
}



void CRevertSaved::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if ( pPlayer )
	{
		//Adrian: Setting this flag so we can't move or save a game.
		pPlayer->pl.deadflag = true;
		pPlayer->AddFlag( (FL_NOTARGET|FL_FROZEN) );

		// clear any pending autosavedangerous
		g_ServerGameDLL.m_fAutoSaveDangerousTime = 0.0f;
		g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	}
}

void CRevertSaved::InputReload( inputdata_t &inputdata )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );

#ifdef HL1_DLL
	SetNextThink( gpGlobals->curtime + MessageTime() );
	SetThink( &CRevertSaved::MessageThink );
#else
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );
#endif

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if ( pPlayer )
	{
		//Adrian: Setting this flag so we can't move or save a game.
		pPlayer->pl.deadflag = true;
		pPlayer->AddFlag( (FL_NOTARGET|FL_FROZEN) );

		// clear any pending autosavedangerous
		g_ServerGameDLL.m_fAutoSaveDangerousTime = 0.0f;
		g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	}
}

#ifdef HL1_DLL
void CRevertSaved::MessageThink( void )
{
	UTIL_ShowMessageAll( STRING( m_iszMessage ) );
	float nextThink = LoadTime() - MessageTime();
	if ( nextThink > 0 ) 
	{
		SetNextThink( gpGlobals->curtime + nextThink );
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}
#endif


void CRevertSaved::LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{
		engine->ServerCommand("reload\n");
	}
}

#define SF_SPEED_MOD_SUPPRESS_WEAPONS	(1<<0)	// Take away weapons
#define SF_SPEED_MOD_SUPPRESS_HUD		(1<<1)	// Take away the HUD
#define SF_SPEED_MOD_SUPPRESS_JUMP		(1<<2)
#define SF_SPEED_MOD_SUPPRESS_DUCK		(1<<3)
#define SF_SPEED_MOD_SUPPRESS_USE		(1<<4)
#define SF_SPEED_MOD_SUPPRESS_SPEED		(1<<5)
#define SF_SPEED_MOD_SUPPRESS_ATTACK	(1<<6)
#define SF_SPEED_MOD_SUPPRESS_ZOOM		(1<<7)

class CMovementSpeedMod : public CPointEntity
{
	DECLARE_CLASS( CMovementSpeedMod, CPointEntity );
public:
	void InputSpeedMod(inputdata_t &data);

private:
	int GetDisabledButtonMask( void );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( player_speedmod, CMovementSpeedMod );

BEGIN_DATADESC( CMovementSpeedMod )
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ModifySpeed", InputSpeedMod ),
END_DATADESC()
	
int CMovementSpeedMod::GetDisabledButtonMask( void )
{
	int nMask = 0;

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_JUMP ) )
	{
		nMask |= IN_JUMP;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_DUCK ) )
	{
		nMask |= IN_DUCK;
	}

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_USE ) )
	{
		nMask |= IN_USE;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_SPEED ) )
	{
		nMask |= IN_SPEED;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_ATTACK ) )
	{
		nMask |= (IN_ATTACK|IN_ATTACK2);
	}

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_ZOOM ) )
	{
		nMask |= IN_ZOOM;
	}

	return nMask;
}

void CMovementSpeedMod::InputSpeedMod(inputdata_t &data)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		if ( data.value.Float() != 1.0f )
		{
			// Holster weapon immediately, to allow it to cleanup
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) )
			{
				if ( pPlayer->GetActiveWeapon() )
				{
					pPlayer->Weapon_SetLast( pPlayer->GetActiveWeapon() );
					pPlayer->GetActiveWeapon()->Holster();
					pPlayer->ClearActiveWeapon();
				}
				
				pPlayer->HideViewModels();
			}

			// Turn off the flashlight
			if ( pPlayer->FlashlightIsOn() )
			{
				pPlayer->FlashlightTurnOff();
			}
			
			// Disable the flashlight's further use
			pPlayer->SetFlashlightEnabled( false );
			pPlayer->DisableButtons( GetDisabledButtonMask() );

			// Hide the HUD
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
			{
				pPlayer->m_Local.m_iHideHUD |= HIDEHUD_ALL;
			}
		}
		else
		{
			// Bring the weapon back
			if  ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) && pPlayer->GetActiveWeapon() == NULL )
			{
				pPlayer->SetActiveWeapon( pPlayer->Weapon_GetLast() );
				if ( pPlayer->GetActiveWeapon() )
				{
					pPlayer->GetActiveWeapon()->Deploy();
				}
			}

			// Allow the flashlight again
			pPlayer->SetFlashlightEnabled( true );
			pPlayer->EnableButtons( GetDisabledButtonMask() );

			// Restore the HUD
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
			{
				pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_ALL;
			}
		}

		pPlayer->SetLaggedMovementValue( data.value.Float() );
	}
}


void SendProxy_CropFlagsToPlayerFlagBitsLength( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	int mask = (1<<PLAYER_FLAG_BITS) - 1;
	int data = *(int *)pVarData;

	pOut->m_Int = ( data & mask );
}
// -------------------------------------------------------------------------------- //
// SendTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		SendPropInt		(SENDINFO(deadflag),	1, SPROP_UNSIGNED ),
	END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE( CBasePlayer, DT_LocalPlayerExclusive )

		SendPropDataTable	( SENDINFO_DT(m_Local), &REFERENCE_SEND_TABLE(DT_Local) ),
		
// If HL2_DLL is defined, then baseflex.cpp already sends these.
#ifndef HL2_DLL
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 8, SPROP_ROUNDDOWN, -32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 8, SPROP_ROUNDDOWN, -32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 10, SPROP_CHANGES_OFTEN,	0.0f, 128.0f),
#endif

		SendPropFloat		( SENDINFO(m_flFriction),		8,	SPROP_ROUNDDOWN,	0.0f,	4.0f),

		SendPropArray3		( SENDINFO_ARRAY3(m_iAmmo), SendPropInt( SENDINFO_ARRAY(m_iAmmo), 10, SPROP_UNSIGNED ) ),
			
		SendPropInt			( SENDINFO( m_fOnTarget ), 2, SPROP_UNSIGNED ),

		SendPropInt			( SENDINFO( m_nTickBase ), -1, SPROP_CHANGES_OFTEN ),
		SendPropInt			( SENDINFO( m_nNextThinkTick ) ),

		SendPropEHandle		( SENDINFO( m_hLastWeapon ) ),
		SendPropEHandle		( SENDINFO( m_hGroundEntity ), SPROP_CHANGES_OFTEN ),

		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

#if PREDICTION_ERROR_CHECK_LEVEL > 1 
		SendPropVector		( SENDINFO( m_vecBaseVelocity ), -1, SPROP_COORD ),
#else
		SendPropVector		( SENDINFO( m_vecBaseVelocity ), 20, 0, -1000, 1000 ),
#endif

		SendPropEHandle		( SENDINFO( m_hConstraintEntity)),
		SendPropVector		( SENDINFO( m_vecConstraintCenter), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintRadius ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintWidth ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintSpeedFactor ), 0, SPROP_NOSCALE ),

		SendPropFloat		( SENDINFO( m_flDeathTime ), 0, SPROP_NOSCALE ),

		SendPropInt			( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),
		SendPropFloat		( SENDINFO( m_flLaggedMovementValue ), 0, SPROP_NOSCALE ),

	END_SEND_TABLE()


// -------------------------------------------------------------------------------- //
// DT_BasePlayer sendtable.
// -------------------------------------------------------------------------------- //

	IMPLEMENT_SERVERCLASS_ST( CBasePlayer, DT_BasePlayer )

		SendPropDataTable(SENDINFO_DT(pl), &REFERENCE_SEND_TABLE(DT_PlayerState), SendProxy_DataTableToDataTable),

		SendPropEHandle(SENDINFO(m_hVehicle)),
		SendPropEHandle(SENDINFO(m_hUseEntity)),
		SendPropInt		(SENDINFO(m_iHealth), 10 ),
		SendPropInt		(SENDINFO(m_lifeState), 3, SPROP_UNSIGNED ),
		SendPropInt		(SENDINFO(m_iBonusProgress), 15 ),
		SendPropInt		(SENDINFO(m_iBonusChallenge), 4 ),
		SendPropFloat	(SENDINFO(m_flMaxspeed), 12, SPROP_ROUNDDOWN, 0.0f, 2048.0f ),  // CL
		SendPropInt		(SENDINFO(m_fFlags), PLAYER_FLAG_BITS, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN, SendProxy_CropFlagsToPlayerFlagBitsLength ),
		SendPropInt		(SENDINFO(m_iObserverMode), 3, SPROP_UNSIGNED ),
		SendPropEHandle	(SENDINFO(m_hObserverTarget) ),
		SendPropInt		(SENDINFO(m_iFOV), 8, SPROP_UNSIGNED ),
		SendPropInt		(SENDINFO(m_iFOVStart), 8, SPROP_UNSIGNED ),
		SendPropFloat	(SENDINFO(m_flFOVTime) ),
		SendPropInt		(SENDINFO(m_iDefaultFOV), 8, SPROP_UNSIGNED ),
		SendPropEHandle	(SENDINFO(m_hZoomOwner) ),
		SendPropArray	( SendPropEHandle( SENDINFO_ARRAY( m_hViewModel ) ), m_hViewModel ),
		SendPropString	(SENDINFO(m_szLastPlaceName) ),
		SendPropInt		( SENDINFO( m_ubEFNoInterpParity ), NOINTERP_PARITY_MAX_BITS, SPROP_UNSIGNED ),

		// Data that only gets sent to the local player.
		SendPropDataTable( "localdata", 0, &REFERENCE_SEND_TABLE(DT_LocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	END_SEND_TABLE()

//=============================================================================
//
// Player Physics Shadow Code
//

void CBasePlayer::SetupVPhysicsShadow( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, CPhysCollide *pStandModel, const char *pStandHullName, CPhysCollide *pCrouchModel, const char *pCrouchHullName )
{
	solid_t solid;
	Q_strncpy( solid.surfaceprop, "player", sizeof(solid.surfaceprop) );
	solid.params = g_PhysDefaultObjectParams;
	solid.params.mass = 85.0f;
	solid.params.inertia = 1e24f;
	solid.params.enableCollisions = false;
	//disable drag
	solid.params.dragCoefficient = 0;
	// create standing hull
	m_pShadowStand = PhysModelCreateCustom( this, pStandModel, GetLocalOrigin(), GetLocalAngles(), pStandHullName, false, &solid );
	m_pShadowStand->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// create crouchig hull
	m_pShadowCrouch = PhysModelCreateCustom( this, pCrouchModel, GetLocalOrigin(), GetLocalAngles(), pCrouchHullName, false, &solid );
	m_pShadowCrouch->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// default to stand
	VPhysicsSetObject( m_pShadowStand );

	// tell physics lists I'm a shadow controller object
	PhysAddShadow( this );	
	m_pPhysicsController = physenv->CreatePlayerController( m_pShadowStand );
	m_pPhysicsController->SetPushMassLimit( 350.0f );
	m_pPhysicsController->SetPushSpeedLimit( 50.0f );
	
	// Give the controller a valid position so it doesn't do anything rash.
	UpdatePhysicsShadowToPosition( vecAbsOrigin );

	// init state
	if ( GetFlags() & FL_DUCKING )
	{
		SetVCollisionState( vecAbsOrigin, vecAbsVelocity, VPHYS_CROUCH );
	}
	else
	{
		SetVCollisionState( vecAbsOrigin, vecAbsVelocity, VPHYS_WALK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Empty, just want to keep the baseentity version from being called
//          current so we don't kick up dust, etc.
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	float savedImpact = m_impactEnergyScale;
	
	// HACKHACK: Reduce player's stress by 1/8th
	m_impactEnergyScale *= 0.125f;
	ApplyStressDamage( pPhysics, true );
	m_impactEnergyScale = savedImpact;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsShadowUpdate( IPhysicsObject *pPhysics )
{
	if ( sv_turbophysics.GetBool() )
		return;

	Vector newPosition;

	bool physicsUpdated = m_pPhysicsController->GetShadowPosition( &newPosition, NULL ) > 0 ? true : false;

	// UNDONE: If the player is penetrating, but the player's game collisions are not stuck, teleport the physics shadow to the game position
	if ( pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING )
	{
		CUtlVector<CBaseEntity *> list;
		PhysGetListOfPenetratingEntities( this, list );
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			// filter out anything that isn't simulated by vphysics
			// UNDONE: Filter out motion disabled objects?
			if ( list[i]->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				// I'm currently stuck inside a moving object, so allow vphysics to 
				// apply velocity to the player in order to separate these objects
				m_touchedPhysObject = true;
			}

			// if it's an NPC, tell them that the player is intersecting them
			CAI_BaseNPC *pNPC = list[i]->MyNPCPointer();
			if ( pNPC )
			{
				pNPC->PlayerPenetratingVPhysics();
			}
		}
	}

	bool bCheckStuck = false;
	if ( m_afPhysicsFlags & PFLAG_GAMEPHYSICS_ROTPUSH )
	{
		bCheckStuck = true;
		m_afPhysicsFlags &= ~PFLAG_GAMEPHYSICS_ROTPUSH;
	}
	if ( m_pPhysicsController->IsInContact() || (m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER) )
	{
		m_touchedPhysObject = true;
	}

	if ( IsFollowingPhysics() )
	{
		m_touchedPhysObject = true;
	}

	if ( GetMoveType() == MOVETYPE_NOCLIP || pl.deadflag )
	{
		m_oldOrigin = GetAbsOrigin();
		return;
	}

	if ( phys_timescale.GetFloat() == 0.0f )
	{
		physicsUpdated = false;
	}

	if ( !physicsUpdated )
		return;

	IPhysicsObject *pPhysGround = GetGroundVPhysics();

	Vector newVelocity;
	pPhysics->GetPosition( &newPosition, 0 );
	m_pPhysicsController->GetShadowVelocity( &newVelocity );
	// assume vphysics gave us back a position without penetration
	Vector lastValidPosition = newPosition;

	if ( physicsshadowupdate_render.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), 255, 0, 0, 24, 15.0f );
		NDebugOverlay::Box( newPosition, WorldAlignMins(), WorldAlignMaxs(), 0,0,255, 24, 15.0f);
		//	NDebugOverlay::Box( newPosition, WorldAlignMins(), WorldAlignMaxs(), 0,0,255, 24, .01f);
	}

	Vector tmp = GetAbsOrigin() - newPosition;
	if ( !m_touchedPhysObject && !(GetFlags() & FL_ONGROUND) )
	{
		tmp.z *= 0.5f;	// don't care about z delta as much
	}

	float dist = tmp.LengthSqr();
	float deltaV = (newVelocity - GetAbsVelocity()).LengthSqr();

	float maxDistErrorSqr = VPHYS_MAX_DISTSQR;
	float maxVelErrorSqr = VPHYS_MAX_VELSQR;
	if ( IsRideablePhysics(pPhysGround) )
	{
		maxDistErrorSqr *= 0.25;
		maxVelErrorSqr *= 0.25;
	}

	// player's physics was frozen, try moving to the game's simulated position if possible
	if ( m_pPhysicsController->WasFrozen() )
	{
		m_bPhysicsWasFrozen = true;
		// check my position (physics object could have simulated into my position
		// physics is not very far away, check my position
		trace_t trace;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( !trace.startsolid )
			return;

		// The physics shadow position is probably not in solid, try to move from there to the desired position
		UTIL_TraceEntity( this, newPosition, GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( !trace.startsolid )
		{
			// found a valid position between the two?  take it.
			SetAbsOrigin( trace.endpos );
			UpdateVPhysicsPosition(trace.endpos, vec3_origin, 0);
			return;
		}

	}
	if ( dist >= maxDistErrorSqr || deltaV >= maxVelErrorSqr || (pPhysGround && !m_touchedPhysObject) )
	{
		if ( m_touchedPhysObject || pPhysGround )
		{
			// BUGBUG: Rewrite this code using fixed timestep
			if ( deltaV >= maxVelErrorSqr && !m_bPhysicsWasFrozen )
			{
				Vector dir = GetAbsVelocity();
				float len = VectorNormalize(dir);
				float dot = DotProduct( newVelocity, dir );
				if ( dot > len )
				{
					dot = len;
				}
				else if ( dot < -len )
				{
					dot = -len;
				}
				
				VectorMA( newVelocity, -dot, dir, newVelocity );
				
				if ( m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER )
				{
					float val = Lerp( 0.1f, len, dot );
					VectorMA( newVelocity, val - len, dir, newVelocity );
				}

				if ( !IsRideablePhysics(pPhysGround) )
				{
					if ( !(m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER ) && IsSimulatingOnAlternateTicks() )
					{
						newVelocity *= 0.5f;
					}
					ApplyAbsVelocityImpulse( newVelocity );
				}
			}
			
			trace_t trace;
			UTIL_TraceEntity( this, newPosition, newPosition, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			if ( !trace.allsolid && !trace.startsolid )
			{
				SetAbsOrigin( newPosition );
			}
		}
		else
		{
			bCheckStuck = true;
		}
	}
	else
	{
		if ( m_touchedPhysObject )
		{
			// check my position (physics object could have simulated into my position
			// physics is not very far away, check my position
			trace_t trace;
			UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(),
				MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			
			// is current position ok?
			if ( trace.allsolid || trace.startsolid )
			{
				// no use the final stuck check to move back to old if this stuck fix didn't work
				bCheckStuck = true;
				lastValidPosition = m_oldOrigin;
				SetAbsOrigin( newPosition );
			}
		}
	}

	if ( bCheckStuck )
	{
		trace_t trace;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

		// current position is not ok, fixup
		if ( trace.allsolid || trace.startsolid )
		{
			// STUCK!?!?!
			//Warning( "Checkstuck failed.  Stuck on %s!!\n", trace.m_pEnt->GetClassname() );
			SetAbsOrigin( lastValidPosition );
		}
	}
	m_oldOrigin = GetAbsOrigin();
	m_bPhysicsWasFrozen = false;
}

// recreate physics on save/load, don't try to save the state!
bool CBasePlayer::ShouldSavePhysics()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InitVCollision( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity )
{
	// Cleanup any old vphysics stuff.
	VPhysicsDestroyObject();

	// in turbo physics players dont have a physics shadow
	if ( sv_turbophysics.GetBool() )
		return;
	
	CPhysCollide *pModel = PhysCreateBbox( VEC_HULL_MIN, VEC_HULL_MAX );
	CPhysCollide *pCrouchModel = PhysCreateBbox( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );

	SetupVPhysicsShadow( vecAbsOrigin, vecAbsVelocity, pModel, "player_stand", pCrouchModel, "player_crouch" );
}


void CBasePlayer::VPhysicsDestroyObject()
{
	// Since CBasePlayer aliases its pointer to the physics object, tell CBaseEntity to 
	// clear out its physics object pointer so we don't wind up deleting one of
	// the aliased objects twice.
	VPhysicsSetObject( NULL );

	PhysRemoveShadow( this );
	
	if ( m_pPhysicsController )
	{
		physenv->DestroyPlayerController( m_pPhysicsController );
		m_pPhysicsController = NULL;
	}

	if ( m_pShadowStand )
	{
		m_pShadowStand->EnableCollisions( false );
		PhysDestroyObject( m_pShadowStand );
		m_pShadowStand = NULL;
	}
	if ( m_pShadowCrouch )
	{
		m_pShadowCrouch->EnableCollisions( false );
		PhysDestroyObject( m_pShadowCrouch );
		m_pShadowCrouch = NULL;
	}

	BaseClass::VPhysicsDestroyObject();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::SetVCollisionState( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, int collisionState )
{
	m_vphysicsCollisionState = collisionState;
	switch( collisionState )
	{
	case VPHYS_WALK:
 		m_pShadowStand->SetPosition( vecAbsOrigin, vec3_angle, true );
		m_pShadowStand->SetVelocity( &vecAbsVelocity, NULL );
		m_pShadowCrouch->EnableCollisions( false );
		m_pPhysicsController->SetObject( m_pShadowStand );
		VPhysicsSwapObject( m_pShadowStand );
		m_pShadowStand->EnableCollisions( true );
		break;

	case VPHYS_CROUCH:
		m_pShadowCrouch->SetPosition( vecAbsOrigin, vec3_angle, true );
		m_pShadowCrouch->SetVelocity( &vecAbsVelocity, NULL );
		m_pShadowStand->EnableCollisions( false );
		m_pPhysicsController->SetObject( m_pShadowCrouch );
		VPhysicsSwapObject( m_pShadowCrouch );
		m_pShadowCrouch->EnableCollisions( true );
		break;
	
	case VPHYS_NOCLIP:
		m_pShadowCrouch->EnableCollisions( false );
		m_pShadowStand->EnableCollisions( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBasePlayer::GetFOV( void )
{
	int nDefaultFOV;

	// The vehicle's FOV wins if we're asking for a default value
	if ( GetVehicle() )
	{
		CacheVehicleView();
		nDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : (int) m_flVehicleViewFOV;
	}
	else
	{
		nDefaultFOV = GetDefaultFOV();
	}
	
	int fFOV = ( m_iFOV == 0 ) ? nDefaultFOV : m_iFOV;

	// If it's immediate, just do it
	if ( m_Local.m_flFOVRate == 0.0f )
		return fFOV;

	float deltaTime = (float)( gpGlobals->curtime - m_flFOVTime ) / m_Local.m_flFOVRate;

	if ( deltaTime >= 1.0f )
	{
		//If we're past the zoom time, just take the new value and stop lerping
		m_iFOVStart = fFOV;
	}
	else
	{
		fFOV = SimpleSplineRemapValClamped( deltaTime, 0.0f, 1.0f, m_iFOVStart, fFOV );
	}

	return fFOV;
}


//-----------------------------------------------------------------------------
// Get the current FOV used for network computations
// Choose the smallest FOV, as it will open the largest number of portals
//-----------------------------------------------------------------------------
int CBasePlayer::GetFOVForNetworking( void )
{
	int nDefaultFOV;

	// The vehicle's FOV wins if we're asking for a default value
	if ( GetVehicle() )
	{
		CacheVehicleView();
		nDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : (int) m_flVehicleViewFOV;
	}
	else
	{
		nDefaultFOV = GetDefaultFOV();
	}

	int fFOV = ( m_iFOV == 0 ) ? nDefaultFOV : m_iFOV;

	// If it's immediate, just do it
	if ( m_Local.m_flFOVRate == 0.0f )
		return fFOV;

	if ( gpGlobals->curtime - m_flFOVTime < m_Local.m_flFOVRate )
	{
		fFOV = min( fFOV, m_iFOVStart );
	}
	return fFOV;
}


float CBasePlayer::GetFOVDistanceAdjustFactorForNetworking()
{
	float defaultFOV	= (float)GetDefaultFOV();
	float localFOV		= (float)GetFOVForNetworking();

	if ( localFOV == defaultFOV || defaultFOV < 0.001f )
		return 1.0f;

	// If FOV is lower, then we're "zoomed" in and this will give a factor < 1 so apparent LOD distances can be
	//  shorted accordingly
	return localFOV / defaultFOV;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the default FOV for the player if nothing else is going on
// Input  : FOV - the new base FOV for this player
//-----------------------------------------------------------------------------
void CBasePlayer::SetDefaultFOV( int FOV )
{
	m_iDefaultFOV = ( FOV == 0 ) ? g_pGameRules->DefaultFOV() : FOV;
}

//-----------------------------------------------------------------------------
// Purpose: // static func
// Input  : set - 
//-----------------------------------------------------------------------------
void CBasePlayer::ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set )
{
	// Append our health
	set.AppendCriteria( "playerhealth", UTIL_VarArgs( "%i", GetHealth() ) );
	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "playerhealthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	CBaseCombatWeapon *weapon = GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "playerweapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "playerweapon", "none" );
	}

	// Append current activity name
	set.AppendCriteria( "playeractivity", CAI_BaseNPC::GetActivityName( GetActivity() ) );

	set.AppendCriteria( "playerspeed", UTIL_VarArgs( "%.3f", GetAbsVelocity().Length() ) );

	AppendContextToCriteria( set, "player" );
}


const QAngle& CBasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void CBasePlayer::SetPunchAngle( const QAngle &punchAngle )
{
	m_Local.m_vecPunchAngle = punchAngle;

	if ( IsAlive() )
	{
		int index = entindex();

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && i != index && pPlayer->GetObserverTarget() == this && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
			{
				pPlayer->SetPunchAngle( punchAngle );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply a movement constraint to the player
//-----------------------------------------------------------------------------
void CBasePlayer::ActivateMovementConstraint( CBaseEntity *pEntity, const Vector &vecCenter, float flRadius, float flConstraintWidth, float flSpeedFactor )
{
	m_hConstraintEntity = pEntity;
	m_vecConstraintCenter = vecCenter;
	m_flConstraintRadius = flRadius;
	m_flConstraintWidth = flConstraintWidth;
	m_flConstraintSpeedFactor = flSpeedFactor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DeactivateMovementConstraint( )
{
	m_hConstraintEntity = NULL;
	m_flConstraintRadius = 0.0f;
	m_vecConstraintCenter = vec3_origin;
}

//-----------------------------------------------------------------------------
// Perhaps a poorly-named function. This function traces against the supplied
// NPC's hitboxes (instead of hull). If the trace hits a different NPC, the 
// new NPC is selected. Otherwise, the supplied NPC is determined to be the 
// one the citizen wants. This function allows the selection of a citizen over
// another citizen's shoulder, which is impossible without tracing against
// hitboxes instead of the hull (sjb)
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::DoubleCheckUseNPC( CBaseEntity *pNPC, const Vector &vecSrc, const Vector &vecDir )
{
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if( tr.m_pEnt != NULL && tr.m_pEnt->MyNPCPointer() && tr.m_pEnt != pNPC )
	{
		// Player is selecting a different NPC through some negative space
		// in the first NPC's hitboxes (between legs, over shoulder, etc).
		return tr.m_pEnt;
	}

	return pNPC;
}


bool CBasePlayer::IsBot() const
{
	return (GetFlags() & FL_FAKECLIENT) != 0;
}

bool CBasePlayer::IsFakeClient() const
{
	return (GetFlags() & FL_FAKECLIENT) != 0;
}

void CBasePlayer::EquipSuit( bool bPlayEffects )
{ 
	m_Local.m_bWearingSuit = true;
}

void CBasePlayer::RemoveSuit( void )
{
	m_Local.m_bWearingSuit = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
void CBasePlayer::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetHealth( inputdata_t &inputdata )
{
	int iNewHealth = inputdata.value.Int();
	int iDelta = abs(GetHealth() - iNewHealth);
	if ( iNewHealth > GetHealth() )
	{
		TakeHealth( iDelta, DMG_GENERIC );
	}
	else if ( iNewHealth < GetHealth() )
	{
		// Strip off and restore armor so that it doesn't absorb any of this damage.
		int armor = m_ArmorValue;
		m_ArmorValue = 0;
		TakeDamage( CTakeDamageInfo( this, this, iDelta, DMG_GENERIC ) );
		m_ArmorValue = armor;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides or displays the HUD
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetHUDVisibility( inputdata_t &inputdata )
{
	bool bEnable = inputdata.value.Bool();

	if ( bEnable )
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_ALL;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_ALL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the fog controller data per player.
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetFogController( inputdata_t &inputdata )
{
	// Find the fog controller with the given name.
	CFogController *pFogController = dynamic_cast<CFogController*>( gEntList.FindEntityByName( NULL, inputdata.value.String() ) );
	if ( pFogController )
	{
		m_Local.m_PlayerFog.m_hCtrl.Set( pFogController );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InitFogController( void )
{
	// Setup with the default master controller.
	m_Local.m_PlayerFog.m_hCtrl = FogSystem()->GetMasterFogController();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetViewEntity( CBaseEntity *pEntity ) 
{ 
	m_hViewEntity = pEntity; 

	if ( m_hViewEntity )
	{
		engine->SetView( edict(), m_hViewEntity->edict() );
	}
	else
	{
		engine->SetView( edict(), edict() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Looks at the player's reserve ammo and also all his weapons for any ammo
//			of the specified type
// Input  : nAmmoIndex - ammo to look for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasAnyAmmoOfType( int nAmmoIndex )
{
	// Must be a valid index
	if ( nAmmoIndex < 0 )
		return false;

	// If we have some in reserve, we're already done
	if ( GetAmmoCount( nAmmoIndex ) )
		return true;

	CBaseCombatWeapon *pWeapon;

	// Check all held weapons
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );

		if ( !pWeapon )
			continue;

		// We must use clips and use this sort of ammo
		if ( pWeapon->UsesClipsForAmmo1() && pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
		{
			// If we have any ammo, we're done
			if ( pWeapon->HasPrimaryAmmo() )
				return true;
		}
		
		// We'll check both clips for the same ammo type, just in case
		if ( pWeapon->UsesClipsForAmmo2() && pWeapon->GetSecondaryAmmoType() == nAmmoIndex )
		{
			if ( pWeapon->HasSecondaryAmmo() )
				return true;
		}
	}	

	// We're completely without this type of ammo
	return false;
}

//-----------------------------------------------------------------------------
//  return a string version of the players network (i.e steam) ID.
//
//-----------------------------------------------------------------------------
const char *CBasePlayer::GetNetworkIDString()
{
	//Tony; bots don't have network id's, and this can potentially crash, especially with plugins creating them.
	if (IsBot())
		return "__BOT__";

	//Tony; if networkidstring is null for any reason, the strncpy will crash!
	if (!m_szNetworkIDString)
		return "NULLID";

	Q_strncpy( m_szNetworkIDString, engine->GetPlayerNetworkIDString( edict() ), sizeof(m_szNetworkIDString) );
	return m_szNetworkIDString; 
}

//-----------------------------------------------------------------------------
//  Assign the player a name
//-----------------------------------------------------------------------------
void CBasePlayer::SetPlayerName( const char *name )
{
	Assert( name );

	if ( name )
	{
		Assert( strlen(name) > 0 );

		Q_strncpy( m_szNetname, name, sizeof(m_szNetname) );
	}
}

//-----------------------------------------------------------------------------
// sets the "don't autokick me" flag on a player
//-----------------------------------------------------------------------------
class DisableAutokick
{
public:
	DisableAutokick( int userID )
	{
		m_userID = userID;
	}

	bool operator()( CBasePlayer *player )
	{
		if ( player->GetUserID() == m_userID )
		{
			Msg( "autokick is disabled for %s\n", player->GetPlayerName() );
			player->DisableAutoKick( true );
			return false; // don't need to check other players
		}

		return true; // keep looking at other players
	}

private:
	int m_userID;
};

//-----------------------------------------------------------------------------
// sets the "don't autokick me" flag on a player
//-----------------------------------------------------------------------------
CON_COMMAND( mp_disable_autokick, "Prevents a userid from being auto-kicked" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use mp_disable_autokick\n" );
		return;
	}

	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: mp_disable_autokick <userid>\n" );
		return;
	}

	int userID = atoi( args[1] );
	DisableAutokick disable( userID );
	ForEachPlayer( disable );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle between the duck being on and off
//-----------------------------------------------------------------------------
void CBasePlayer::ToggleDuck( void )
{
	// Toggle the state
	m_bDuckToggled = !m_bDuckToggled;
}

//-----------------------------------------------------------------------------
// Just tells us how far the stick is from the center. No directional info
//-----------------------------------------------------------------------------
float CBasePlayer::GetStickDist()
{
	Vector2D controlStick;

	controlStick.x = m_flForwardMove;
	controlStick.y = m_flSideMove;

	return controlStick.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::HandleAnimEvent( animevent_t *pEvent )
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
		if ( pEvent->event == AE_RAGDOLL )
		{
			// Convert to ragdoll immediately
			CreateRagdollEntity();
			BecomeRagdollOnClient( vec3_origin );
 
			// Force the player to start death thinking
			SetThink(&CBasePlayer::PlayerDeathThink);
			SetNextThink( gpGlobals->curtime + 0.1f );
			return;
		}
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
//  CPlayerInfo functions (simple passthroughts to get around the CBasePlayer multiple inheritence limitation)
//-----------------------------------------------------------------------------
const char *CPlayerInfo::GetName()
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerName(); 
}

int	CPlayerInfo::GetUserID() 
{ 
	Assert( m_pParent );
	return engine->GetPlayerUserId( m_pParent->edict() ); 
}

const char *CPlayerInfo::GetNetworkIDString() 
{ 
	Assert( m_pParent );
	return m_pParent->GetNetworkIDString(); 
}

int	CPlayerInfo::GetTeamIndex() 
{ 
	Assert( m_pParent );
	return m_pParent->GetTeamNumber(); 
}  

void CPlayerInfo::ChangeTeam( int iTeamNum ) 
{ 
	Assert( m_pParent );
	m_pParent->ChangeTeam(iTeamNum); 
}

int	CPlayerInfo::GetFragCount() 
{ 
	Assert( m_pParent );
	return m_pParent->FragCount(); 
}

int	CPlayerInfo::GetDeathCount() 
{ 
	Assert( m_pParent );
	return m_pParent->DeathCount(); 
}

bool CPlayerInfo::IsConnected() 
{ 
	Assert( m_pParent );
	return m_pParent->IsConnected(); 
}

int	CPlayerInfo::GetArmorValue() 
{ 
	Assert( m_pParent );
	return m_pParent->ArmorValue(); 
}

bool CPlayerInfo::IsHLTV() 
{ 
	Assert( m_pParent );
	return m_pParent->IsHLTV(); 
}

bool CPlayerInfo::IsPlayer() 
{ 
	Assert( m_pParent );
	return m_pParent->IsPlayer(); 
}

bool CPlayerInfo::IsFakeClient() 
{ 
	Assert( m_pParent );
	return m_pParent->IsFakeClient(); 
}

bool CPlayerInfo::IsDead() 
{ 
	Assert( m_pParent );
	return m_pParent->IsDead(); 
}

bool CPlayerInfo::IsInAVehicle() 
{ 
	Assert( m_pParent );
	return m_pParent->IsInAVehicle(); 
}

bool CPlayerInfo::IsObserver() 
{ 
	Assert( m_pParent );
	return m_pParent->IsObserver(); 
}

const Vector CPlayerInfo::GetAbsOrigin() 
{ 
	Assert( m_pParent );
	return m_pParent->GetAbsOrigin(); 
}

const QAngle CPlayerInfo::GetAbsAngles() 
{ 
	Assert( m_pParent );
	return m_pParent->GetAbsAngles(); 
}

const Vector CPlayerInfo::GetPlayerMins() 
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerMins(); 
}

const Vector CPlayerInfo::GetPlayerMaxs() 
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerMaxs(); 
}

const char *CPlayerInfo::GetWeaponName() 
{ 
	Assert( m_pParent );
	CBaseCombatWeapon *weap = m_pParent->GetActiveWeapon();
	if ( !weap )
	{
		return NULL;
	}
	return weap->GetName();
}

const char *CPlayerInfo::GetModelName() 
{ 
	Assert( m_pParent );
	return m_pParent->GetModelName().ToCStr(); 
}

const int CPlayerInfo::GetHealth() 
{ 
	Assert( m_pParent );
	return m_pParent->GetHealth(); 
}

const int CPlayerInfo::GetMaxHealth() 
{ 
	Assert( m_pParent );
	return m_pParent->GetMaxHealth(); 
}





void CPlayerInfo::SetAbsOrigin( Vector & vec ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetAbsOrigin(vec); 
	}
}

void CPlayerInfo::SetAbsAngles( QAngle & ang ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetAbsAngles(ang); 
	}
}

void CPlayerInfo::RemoveAllItems( bool removeSuit ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->RemoveAllItems(removeSuit); 
	}
}

void CPlayerInfo::SetActiveWeapon( const char *WeaponName ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		CBaseCombatWeapon *weap = m_pParent->Weapon_Create( WeaponName );
		if ( weap )
		{
			m_pParent->Weapon_Equip(weap); 
			m_pParent->Weapon_Switch(weap); 
		}
	}
}

void CPlayerInfo::SetLocalOrigin( const Vector& origin ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetLocalOrigin(origin); 
	}
}

const Vector CPlayerInfo::GetLocalOrigin( void ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		Vector origin = m_pParent->GetLocalOrigin();
		return origin; 
	}
	else
	{
		return Vector( 0, 0, 0 );
	}
}

void CPlayerInfo::SetLocalAngles( const QAngle& angles ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetLocalAngles( angles ); 
	}
}

const QAngle CPlayerInfo::GetLocalAngles( void ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		return m_pParent->GetLocalAngles(); 
	}
	else
	{
		return QAngle();
	}
}

void CPlayerInfo::PostClientMessagesSent( void ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->PostClientMessagesSent(); 
	}
}

bool CPlayerInfo::IsEFlagSet( int nEFlagMask ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		return m_pParent->IsEFlagSet(nEFlagMask); 
	}
	return false;
}

void CPlayerInfo::RunPlayerMove( CBotCmd *ucmd ) 
{ 
	if ( m_pParent->IsBot() )
	{
		Assert( m_pParent );
		CUserCmd cmd;
		cmd.buttons = ucmd->buttons;
		cmd.command_number = ucmd->command_number;
		cmd.forwardmove = ucmd->forwardmove;
		cmd.hasbeenpredicted = ucmd->hasbeenpredicted;
		cmd.impulse = ucmd->impulse;
		cmd.mousedx = ucmd->mousedx;
		cmd.mousedy = ucmd->mousedy;
		cmd.random_seed = ucmd->random_seed;
		cmd.sidemove = ucmd->sidemove;
		cmd.tick_count = ucmd->tick_count;
		cmd.upmove = ucmd->upmove;
		cmd.viewangles = ucmd->viewangles;
		cmd.weaponselect = ucmd->weaponselect;
		cmd.weaponsubtype = ucmd->weaponsubtype;

		// Store off the globals.. they're gonna get whacked
		float flOldFrametime = gpGlobals->frametime;
		float flOldCurtime = gpGlobals->curtime;

		m_pParent->SetTimeBase( gpGlobals->curtime );

		MoveHelperServer()->SetHost( m_pParent );
		m_pParent->PlayerRunCommand( &cmd, MoveHelperServer() );

		// save off the last good usercmd
		m_pParent->SetLastUserCommand( cmd );

		// Clear out any fixangle that has been set
		m_pParent->pl.fixangle = FIXANGLE_NONE;

		// Restore the globals..
		gpGlobals->frametime = flOldFrametime;
		gpGlobals->curtime = flOldCurtime;
		MoveHelperServer()->SetHost( NULL );
	}
}

void CPlayerInfo::SetLastUserCommand( const CBotCmd &ucmd ) 
{ 
	if ( m_pParent->IsBot() )
	{
		Assert( m_pParent );
		CUserCmd cmd;
		cmd.buttons = ucmd.buttons;
		cmd.command_number = ucmd.command_number;
		cmd.forwardmove = ucmd.forwardmove;
		cmd.hasbeenpredicted = ucmd.hasbeenpredicted;
		cmd.impulse = ucmd.impulse;
		cmd.mousedx = ucmd.mousedx;
		cmd.mousedy = ucmd.mousedy;
		cmd.random_seed = ucmd.random_seed;
		cmd.sidemove = ucmd.sidemove;
		cmd.tick_count = ucmd.tick_count;
		cmd.upmove = ucmd.upmove;
		cmd.viewangles = ucmd.viewangles;
		cmd.weaponselect = ucmd.weaponselect;
		cmd.weaponsubtype = ucmd.weaponsubtype;

		m_pParent->SetLastUserCommand(cmd); 
	}
}


CBotCmd CPlayerInfo::GetLastUserCommand()
{
	CBotCmd cmd;
	const CUserCmd *ucmd = m_pParent->GetLastUserCommand();
	if ( ucmd )
	{
		cmd.buttons = ucmd->buttons;
		cmd.command_number = ucmd->command_number;
		cmd.forwardmove = ucmd->forwardmove;
		cmd.hasbeenpredicted = ucmd->hasbeenpredicted;
		cmd.impulse = ucmd->impulse;
		cmd.mousedx = ucmd->mousedx;
		cmd.mousedy = ucmd->mousedy;
		cmd.random_seed = ucmd->random_seed;
		cmd.sidemove = ucmd->sidemove;
		cmd.tick_count = ucmd->tick_count;
		cmd.upmove = ucmd->upmove;
		cmd.viewangles = ucmd->viewangles;
		cmd.weaponselect = ucmd->weaponselect;
		cmd.weaponsubtype = ucmd->weaponsubtype;
	}
	return cmd;
}

// Notify that I've killed some other entity. (called from Victim's Event_Killed).
void CBasePlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );
	if ( pVictim != this )
	{
		gamestats->Event_PlayerKilledOther( this, pVictim, info );
	}
}

void CBasePlayer::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );
	m_nBodyPitchPoseParam = LookupPoseParameter( "body_pitch" );
}

void CBasePlayer::SetBodyPitch( float flPitch )
{
	if ( m_nBodyPitchPoseParam >= 0 )
	{
		SetPoseParameter( m_nBodyPitchPoseParam, flPitch );
	}
}

void CBasePlayer::AdjustDrownDmg( int nAmount )
{
	m_idrowndmg += nAmount;
	if ( m_idrowndmg < m_idrownrestored )
	{
		m_idrowndmg = m_idrownrestored;
	}
}
