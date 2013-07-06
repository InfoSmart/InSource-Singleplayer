//=====================================================================================//
//
// InDirector
//
// Inteligencia encargada de controlar la música de fondo, los npcs, el clima y otros
// aspectos del juego.
//
// Inspiración: AI Director de Left 4 Dead: http://left4dead.wikia.com/wiki/The_Director
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "director.h"

#include "in_player.h"
#include "in_utils.h"

#include "director_spawn.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "in_gamerules.h"
#include "logicrelay.h"

#include "ai_relationship.h"

#include "tier0/memdbgon.h"

/*
	NOTAS

	Grunt: Soldado robotico con 3,000 puntos de salud, se le considera el NPC más poderoso y enemigo del jugador
	Solo debe aparecer de 1 a 3 ocaciones en mapas.

	info_director_spawn: Entidad (se coloca en el mapa) encargada de establecer una zona para crear npcs,
	es utilizado por el Director.
*/

/*
	* RELAJADO
	* El Director crea una cantidad minima de zombis con una salud baja con el fin de decorar
	* el escenario.

	* EXALTADO
	* Permite la creación de más zombis que en el modo "Relajado" con el fin de dar un escenario
	* más apocalíptico y aumentar la dificultad del juego en periodos de tiempo al azar.

	* HORDA
	* Conforme se va avanzando en el juego el Director va "recolectando" una cantidad de zombis
	* a soltar cuando se active este modo. Los zombis creados tienen una salud más alta y conocen la
	* ubicación del jugador, trataran de atacarlo.

	* BOSS
	* Los Grunt son NPC poderosos y fuertes, cuando uno aparece en el mapa el Director empieza a reproducir
	* una música de fondo indicando la presencia del mismo y se detiene la creación de zombis hasta que el mismo muera.
	* Igualmente mientras nos encontremos en este modo el Director va "recolectando" zombis para el modo "Horda" que será
	* activado cuando el Grunt muera.

	* CLIMAX
	* Este modo es activado manualmente desde el mapa (o la consola) para representar los últimos minutos de un capitulo.
	* En este modo la creación de zombis aumenta considerablemente y se crean al menos 3 Grunts (Todos persiguiendo
	* al jugador), al igual que se reproduce una música de fondo.
*/

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar director_max_alive_npcs("director_max_alive_npcs", "30", FCVAR_REPLICATED, "Cantidad de NPC's que el Director puede crear. (Aumentara con la dificultad)");
ConVar director_min_maker_distance("director_min_maker_distance",	"2000", FCVAR_REPLICATED, "Distancia en la que se debe encontrar un director_npc_spawn del jugador para que el director pueda usarlo.");

ConVar director_min_npc_boss("director_min_npc_boss", "250", FCVAR_REPLICATED, "Cantidad de NPC's minima a crear antes de dar con la posibilidad de crear un jefe");

ConVar director_max_npcs_queue("director_max_npcs_queue", "6", FCVAR_REPLICATED, "Cantidad limite de NPC's que el Director puede dejar en cola.");

ConVar director_min_npc_distance("indirector_min_npc_distance",	"1800",	FCVAR_REPLICATED, "Distancia minima en la que se debe encontrar un npc del jugador para considerlo un peligro (musica de horda)");
ConVar director_force_horde_queue("director_force_horde_queue",	"0", FCVAR_REPLICATED, "Fuerza a crear esta cantidad de NPC's durante una horda.");
ConVar director_force_spawn_outview("director_force_spawn_outview",	"1", FCVAR_REPLICATED, "Fuerza a crear los NPC's fuera del campo de visión del jugador.");

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef APOCALYPSE
	LINK_ENTITY_TO_CLASS( info_director, CDirector );
#endif

BEGIN_DATADESC( CDirector )

	DEFINE_FIELD( Status,		FIELD_INTEGER ),
	DEFINE_FIELD( AngryLevel,	FIELD_INTEGER ),

	DEFINE_FIELD( Left4Exalted,		FIELD_INTEGER ),
	DEFINE_FIELD( Left4Horde,		FIELD_INTEGER ),
	DEFINE_FIELD( TriggerHorde,		FIELD_BOOLEAN ),
	DEFINE_FIELD( SpawnBlocked,		FIELD_INTEGER ),
	DEFINE_FIELD( DirectorDisabled, FIELD_TIME ),

	DEFINE_SOUNDPATCH( pSound ),
	DEFINE_SOUNDPATCH( Boss_Music ),
	DEFINE_SOUNDPATCH( Horde_Music ),
	DEFINE_SOUNDPATCH( Danger_Music ),
	DEFINE_SOUNDPATCH( Danger_A_Music ),
	DEFINE_SOUNDPATCH( Danger_B_Music ),
	DEFINE_SOUNDPATCH( Climax_Music ),

	DEFINE_FIELD( AllowMusic, FIELD_BOOLEAN ),

	DEFINE_FIELD( MaxChildAlive,	FIELD_INTEGER ),
	DEFINE_FIELD( LastSpawnChilds,	FIELD_INTEGER ),
	DEFINE_FIELD( HordeQueue,		FIELD_INTEGER ),
	DEFINE_FIELD( HordesPassed,		FIELD_INTEGER ),
	DEFINE_FIELD( LastHordeChilds,	FIELD_INTEGER ),

	DEFINE_FIELD( ChildsAlive,			FIELD_INTEGER ),
	DEFINE_FIELD( ChildsKilled,			FIELD_INTEGER ),
	DEFINE_FIELD( ChildsTargetPlayer,	FIELD_INTEGER ),
	DEFINE_FIELD( SpawnQueue,			FIELD_INTEGER ),
	DEFINE_FIELD( ChildsSpawned,		FIELD_INTEGER ),
	DEFINE_FIELD( ChildsExaltedSpawned,	FIELD_INTEGER ),

	DEFINE_FIELD( ChildChoir,	FIELD_BOOLEAN ),

	DEFINE_FIELD( InFinal,			FIELD_BOOLEAN ),
	DEFINE_FIELD( FinalWaves,		FIELD_INTEGER ),
	DEFINE_FIELD( FinalWavesLeft,	FIELD_INTEGER ),
	DEFINE_FIELD( FinalBoss,		FIELD_INTEGER ),
	DEFINE_FIELD( ExitWait,			FIELD_TIME ),
		
	/* KEYFIELDS */
	DEFINE_KEYFIELD( Disabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_KEYFIELD( SoundClimax,		FIELD_SOUNDNAME, "SoundClimax" ),
	DEFINE_KEYFIELD( SoundMiniClimax,	FIELD_SOUNDNAME, "SoundMiniClimax" ),
	DEFINE_KEYFIELD( SoundHorde,		FIELD_SOUNDNAME, "SoundHorde" ),
	DEFINE_KEYFIELD( SoundDangerA,		FIELD_SOUNDNAME, "SoundDangerA" ),
	DEFINE_KEYFIELD( SoundDangerB,		FIELD_SOUNDNAME, "SoundDangerB" ),
	DEFINE_KEYFIELD( SoundBoss,			FIELD_SOUNDNAME, "SoundBoss" ),
	DEFINE_KEYFIELD( SoundBossBrothers, FIELD_SOUNDNAME, "SoundBossBrothers" ),
	DEFINE_KEYFIELD( SoundStartFinal,	FIELD_SOUNDNAME, "SoundStartFinal" ),
	DEFINE_KEYFIELD( SoundEscape,		FIELD_SOUNDNAME, "SoundEscape" ),

	/* INPUTS */
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRelax",				InputForceRelax ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceExalted",			InputForceExalted ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceHorde",				InputForceHorde ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceTriggeredHorde",	InputForceTriggeredHorde ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceBoss",				InputForceBoss ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceClimax",			InputForceClimax ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "StartFinal",			InputStartFinal ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EndFinal",				InputEndFinal ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDisabledFor",			InputSetDisabledFor ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMakerProximity",		InputSetMakerDistance ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHordeQueue",			InputSetHordeQueue ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisclosePlayer",			InputDisclosePlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KillChilds",				InputKillChilds ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KillNoVisibleChilds",	InputKillNoVisibleChilds ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",		InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",		InputToggle ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMusic",	InputEnableMusic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMusic",	InputDisableMusic ),

	/* OUTPUTS */
	DEFINE_OUTPUT( OnSpawnChild,	"OnSpawnChild" ),
	DEFINE_OUTPUT( OnChildDead,		"OnSpawnChild" ),
	DEFINE_OUTPUT( OnSpawnBoss,		"OnSpawnBoss" ),
	DEFINE_OUTPUT( OnBossDead,		"OnBossDead" ),
	DEFINE_OUTPUT( OnFinalStart,	"OnFinalStart" ),
	DEFINE_OUTPUT( OnClimax,		"OnClimax" ),

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CDirector::CDirector()
{
	// Iniciamos relajados.
	Relaxed();
	Msg("%s Iniciado \r\n", MS());

	// Reiniciamos variables.

	AngryLevel			= HAPPY;						// Iniciamos felices.

	Left4Horde			= random->RandomInt(100, 300);	// La próxima horda comenzará en un periodo de 100 a 300 segundos.
	SpawnBlocked		= 0;							// Sin bloqueo de creación.	
	DirectorDisabled	= 0;							// Director desactivado por este número de segundos.
	TriggerHorde		= false;						// ¿Una horda especial?

	BossAlive			= 0;							// Jefes vivos.
	Boss_Music			= NULL;							// Recurso de la canción del jefe.
	BossSpawnPending	= false;						// ¿Hay pendiente la creación de un Jefe?
	BossKilled			= 0;							// Jefes muertos.

	Horde_Music			= NULL;					// Recurso de la canción de horda.
	Danger_Music		= NULL;					// Recurso de la canción de horda básica.
	Danger_A_Music		= NULL;					// Recurso de la canción de horda básica A.
	Danger_B_Music		= NULL;					// Recurso de la canción de horda básica B.
	Climax_Music		= NULL;					// Recurso de la canción del CLIMAX.

	AllowMusic			= true;					// Esta permitido la música del Director.

	LastSpawnChilds		= 0;					// Última cantidad de NPC's creados.
	HordeQueue			= 0;					// NPC's en la cola de horda.
	HordesPassed		= 0;					// Cantidad de hordas que han pasado.
	LastHordeChilds		= 0;					// Última cantidad de NPC's creados en modo Horda.

	ChildsAlive				= 0;						// NPC's vivos.
	ChildsKilled			= 0;						// NPC's muertos.
	ChildsTargetPlayer		= 0;						// NPC's con objetivo al jugador.
	SpawnQueue				= director_max_npcs_queue.GetInt(); // NPC's en la cola de creación.
	ChildsSpawned			= 0;						// NPC's creados.
	ChildsExaltedSpawned	= 0;						// NPC's en modo Exaltado creados.

	InFinal			= false;	// ¿Estamos en la parte final del capitulo?
	FinalWaves		= 0;		// Hordas para el final.
	FinalWavesLeft	= 0;		// Número de hordas para terminar.
	FinalBoss		= 0;		// Jefes para el final.
	ExitWait		= 0;		// 
	 
	// Pensamos ¡ahora!
	SetNextThink(gpGlobals->curtime);
}

//=========================================================
// Destructor
//=========================================================
CDirector::~CDirector()
{
}

//=========================================================
// Creación
//=========================================================
void CDirector::Spawn()
{
	BaseClass::Spawn();

	// Somos invisibles.
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);

	// Guardar en caché objetos necesarios.
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirector::Precache()
{
	// Sonidos
	PrecacheScriptSound(STRING(SoundClimax));
	PrecacheScriptSound(STRING(SoundMiniClimax));

	PrecacheScriptSound("Director.Horde.Alert");
	PrecacheScriptSound("Director.Horde.Coming");
	
	PrecacheScriptSound(STRING(SoundHorde));

	PrecacheScriptSound(STRING(SoundDanger));
	PrecacheScriptSound(STRING(SoundDangerA));
	PrecacheScriptSound(STRING(SoundDangerB));

	PrecacheScriptSound("Director.Choir");
	PrecacheScriptSound(STRING(SoundBoss));
	PrecacheScriptSound(STRING(SoundBossBrothers));
	PrecacheScriptSound(STRING(SoundEscape));

	BaseClass::Precache();
}

//=========================================================
// Establece el estado actual de InDirector
//=========================================================
void CDirector::SetStatus(DirectorStatus status)
{
	DevMsg("%s %s -> %s \r\n", MS(), GetStatusName(Status), GetStatusName(status));
	Status = status;
}

//=========================================================
// Calcula el nivel de enojo del Director
//=========================================================
int CDirector::UpdateAngryLevel()
{
	DirectorAngry NewAngryLevel	= HAPPY;
	int AngryPoints				= 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Director es amigable con el jugador, dependiendo del nivel
		// de dificultad del juego podremos calcular su "nivel de enojo"
		switch ( GameRules()->GetSkillLevel() )
		{
			// Fácil
			case SKILL_EASY:
			{
				// Tiene más del 90% de salud.
				if ( pPlayer->GetHealth() > 90 )
					++AngryPoints;

				// ¡Ha matado a la mitad de mis NPC's!
				if ( ChildsKilled > (ChildsSpawned / 2)  )
					++AngryPoints;

				// Ha matado a 2 de mis Jefes y además tiene más del 80% de vida
				if ( BossKilled > 2 && pPlayer->GetHealth() > 80 )
					++AngryPoints;

			

				// @TODO: Verificación del progreso del mapa.
				// @TODO: Verificación de clima.
			}
			break;

			// Medio
			case SKILL_MEDIUM:
			{
				// Tiene más del 70% de salud.
				if ( pPlayer->GetHealth() > 70 )
					++AngryPoints;

				// ¡Ha matado a más de un tercio de mis NPC's!
				if ( ChildsKilled > (ChildsSpawned / 3)  )
					++AngryPoints;

				// Ha matado a 1 de mis Jefes y además tiene más del 60% de vida
				if ( BossKilled > 1 && pPlayer->GetHealth() > 60 )
					++AngryPoints;
			}
			break;

			// Dificil
			case SKILL_HARD:
			{
				// Tiene más del 60% de salud.
				if ( pPlayer->GetHealth() > 60 )
					++AngryPoints;

				// ¡Ha matado a más de un cuarto de mis NPC's!
				if ( ChildsKilled > (ChildsSpawned / 4)  )
					++AngryPoints;

				// Ha matado a 1 de mis Jefes y además tiene más del 40% de vida
				if ( BossKilled > 1 && pPlayer->GetHealth() > 40 )
					++AngryPoints;
			}
			break;
		}
	}

	switch ( AngryPoints )
	{
		case 0:
		default:
			NewAngryLevel = HAPPY;
		break;

		case 1:
			NewAngryLevel = UNCOMFORTABLE;
		break;

		case 2:
			NewAngryLevel = ANGRY;
		break;

		case 3:
			NewAngryLevel = FURIOUS;
		break;
	}

	AngryLevel = NewAngryLevel;
	return AngryPoints;
}

//=========================================================
// Verifica si un NPC esta muy lejos de los jugadores.
// Usado para eliminar a los NPC's que se encuentren lejos.
//=========================================================
bool CDirector::IsTooFar(CAI_BaseNPC *pNPC)
{
	// Distancia minima para considerarse peligroso.
	float minDistance	= director_min_maker_distance.GetFloat();

	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Calculamos la distancia del NPC a este jugador.
		Vector distToEnemy	= pNPC->GetAbsOrigin() - pPlayer->GetAbsOrigin();	
		float dist			= VectorNormalize(distToEnemy);

		// Esta cerca.
		if ( dist <= (minDistance * 1.5) )
			return false;
	}

	return true;
}

//=========================================================
// Verifica si un NPC esta muy cerca de los jugadores.
// Usado para reproducir la música de peligro.
//=========================================================
bool CDirector::IsTooClose(CAI_BaseNPC *pNPC)
{
	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Calculamos la distancia del NPC a este jugador.
		Vector distToEnemy	= pNPC->GetAbsOrigin() - pPlayer->GetAbsOrigin();
		float dist			= VectorNormalize(distToEnemy);

		// Esta cerca del jugador y con el objetivo en el.
		if ( dist < director_min_npc_distance.GetFloat() && pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() )
			return true;
	}

	return false;
}

//=========================================================
// Para la música del Director.
//=========================================================
void CDirector::StopMusic()
{
	if ( PlayingHordeMusic() )
		FadeoutMusic(Horde_Music, 1.5f, true);

	if ( PlayingDangerMusic() )
		FadeoutMusic(Danger_Music, 1.5f, true);

	if ( PlayingDanger_A_Music() )
		FadeoutMusic(Danger_A_Music, 1.5f, true);

	if ( PlayingDanger_B_Music() )
		FadeoutMusic(Danger_B_Music, 1.5f, true);

	if ( PlayingBossMusic() )
		FadeoutMusic(Boss_Music, 1.5f, true);

	if ( PlayingClimaxMusic() )
		FadeoutMusic(Climax_Music, 1.5f, true);
}

//=========================================================
// El final ha terminado. Han escapado...
//=========================================================
void CDirector::FinalEnd()
{
	// Buscamos por la entidad que hará todo el trabajo.
	CLogicRelay *pEnd = (CLogicRelay *)gEntList.FindEntityByName(NULL, "director_end_relay");

	// Lo activamos.
	if ( pEnd )
	{
		inputdata_t inputdata;
		inputdata.pActivator = this;

		pEnd->InputTrigger(inputdata);
	}

	// Activamos la relación para que todos los NPC's dejen en paz al jugador.
	CAI_Relationship *pRelation = (CAI_Relationship *)CreateEntityByName("ai_relationship");
	pRelation->m_iszSubject = MAKE_STRING("npc_*");
	pRelation->m_target		= MAKE_STRING("!player");
	pRelation->Spawn();
	pRelation->ApplyRelationship(this);

	// Paramos la música.
	StopMusic();

	// Has escapado.
	EmitMusic(STRING(SoundEscape));

	AllowMusic		= false;
	MuteOtherSounds = true;	
}

//=========================================================
// Inicia el modo "Relajado"
//=========================================================
void CDirector::Relaxed()
{
	// Estamos en el final, no podemos cambiar el estado.
	if ( InFinal )
		return;

	SetStatus(RELAXED);

	Left4Exalted			= random->RandomInt(20, 80);		// Comenzar el modo Exaltado en 20 a 80 segundos.
	ChildsExaltedSpawned	= 0;							// Reiniciamos contador.
	TriggerHorde			= false;						// Ya no estamos en una horda especial.
}

//=========================================================
// Inicia el modo "Horda"
//=========================================================
void CDirector::Horde(bool super, bool triggered)
{
	// Ya estamos en una Horda o en el final.
	if ( Status == HORDE )
		return;

	DirectorDisabled = gpGlobals->curtime + 5; // Desactivamos al director por 5 segs (Para que se reproduzca el sonido)

	// Sonido: ¡¡Vamos por ti!!
	if ( super )
		EmitSound("Director.Horde.Alert");
	else
		EmitSound("Director.Horde.Coming");
		
	// No es una horda especial.
	if ( !triggered )
	{
		// Si la cantidad de la horda es menor a 20, agregarle 5
		if ( HordeQueue < 20 )
			HordeQueue = HordeQueue + 5;

		// Espera, nos esta pidiendo crear una cantidad determinada.
		if ( director_force_horde_queue.GetInt() != 0 )
			HordeQueue = director_force_horde_queue.GetInt();

		// Es una horda especial.
		if ( super )
			HordeQueue = HordeQueue + 20;
	}

	Left4Horde			= random->RandomInt(100, 300);	// Próxima horda en 100 a 300 segundos.	
	TriggerHorde		= triggered;					// ¿Es una horda especial? (Continua)
	LastHordeChilds		= HordeQueue;					// Cantidad de zombis de esta horda.

	// Los que siguen vivos ¡Vayan por el!
	DisclosePlayer();

	SetStatus(HORDE);
	++HordesPassed;
}

//=========================================================
// Inicia el modo "Climax"
// Escapa si puedes...
//=========================================================
void CDirector::Climax(bool mini)
{
	// Ya estamos en Climax
	if ( Status == CLIMAX )
		return;

	// Reproducimos la música del CLIMAX
	if ( !PlayingClimaxMusic() )
	{
		if ( mini )
			Climax_Music = EmitMusic(STRING(SoundMiniClimax));
		else
			Climax_Music = EmitMusic(STRING(SoundClimax));
	}

	SetStatus(CLIMAX);
	OnClimax.FireOutput(NULL, this);
}

//=========================================================
// Inicia el modo "En Espera"
//=========================================================
void CDirector::Wait()
{
	if ( Status == WAIT )
		return;

	// La espera tardará de 10 a 15 segs.
	int pWait = random->RandomInt(10, 15);

	// Es la primera vez. 
	// 25 segs dura la canción.
	if ( FinalWavesLeft == FinalWaves )
		pWait = 26;

	// ¡Llega el CLIMAX! No lo hagamos esperar: 5 a 10 segs.
	if ( FinalWavesLeft <= 0 )
		pWait = random->RandomInt(5, 10);

	ExitWait = gpGlobals->curtime + pWait;
	SetStatus(WAIT);
}

//=========================================================
// Devuelve la etiqueta de InDirector junto a su estado.
// Útil para debugear en la consola. DevMsg y Warning
// @TODO: Algo mejor que esto...
//=========================================================
const char *CDirector::MS()
{
	char message[100];

	Q_snprintf(message, sizeof(message), "[Director] [%s]", GetStatusName());
	return message;
}

//=========================================================
// Tranforma el estado int a su nombre en string.
//=========================================================
const char *CDirector::GetStatusName(DirectorStatus status)
{
	switch ( status )
	{
		case RELAXED:
			return "RELAJADO";
		break;

		case EXALTED:
			return "EXALTADO";
		break;

		case HORDE:
			return "HORDA";
		break;

		case BOSS:
			return "JEFE";
		break;

		case CLIMAX:
			return "CLIMAX";
		break;

		case WAIT:
			return "EN ESPERA";
		break;
	}

	Warning("[Director] ¡Se ha establecido un estado desconocido! Restaurando a Relajado... \r\n");
	SetStatus(RELAXED);

	return "DESCONOCIDO";
}

//=========================================================
// Tranforma el estado actual a su nombre en string.
//=========================================================
const char *CDirector::GetStatusName()
{
	return GetStatusName(Status);
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CDirector::Think()
{
	BaseClass::Think();
	
	/*
	if ( engine->GetEntityCount() >= director_maxentities.GetInt() && GameRules()->IsMultiplayer() )
	{
		DevWarning("[DIRECTOR] ¡Exceso de entidades! Un poco más y el servidor se cerrará (%i) \r\n", engine->GetEntityCount());
		engine->ServerCommand("ent_fire director_child kill");

		SetNextThink(gpGlobals->curtime + 1);
		return;
	}
	*/

	// El Director esta desactivado.
	if ( DirectorDisabled >= gpGlobals->curtime || Disabled )
	{
		if ( DirectorDisabled < gpGlobals->curtime )
			DirectorDisabled = 0;

		// Volvemos a pensar dentro de 1 segundo
		SetNextThink(gpGlobals->curtime + 1);
		return;
	}

	// En el estado final el Director tiene una funcionalidad para este.
	if ( InFinal )
	{
		FinalThink();

		// Volvemos a pensar dentro de 1 segundo
		SetNextThink(gpGlobals->curtime + 1);
		return;
	}

	// Estamos "Exaltados"
	if ( Status == EXALTED )
	{
		int pPlayersHealth = UTIL_GetPlayersHealth();

		// Si la salud del jugador es mayor a 60%
		// y se han creado 35 NPC's, pasar al modo Relajado.
		if ( pPlayersHealth > 60 && ChildsExaltedSpawned > 35 )
			Relaxed();

		// Si la salud del jugador es mayor a 10%
		// y se han creado 15 NPC's, pasar al modo Relajado.
		else if ( pPlayersHealth > 10 && ChildsExaltedSpawned > 15 )
			Relaxed();

		// Abigaíl esta en muy mal estado (salud menor a 10%)
		// recordemos los buenos tiempos y dejémosle vivir más tiempo...
		else
			Relaxed();
	}

	// Estamos "Relajados"
	if ( Status == RELAXED )
	{
		// Un segundo menos para el modo Exaltado.
		Left4Exalted--;

		if ( Left4Exalted <= 0 )
			SetStatus(EXALTED);
	}

	// Estamos en una "Horda" y
	// la horda actual no termina hasta ser solicitado.
	if ( Status == HORDE )
	{
		// Revelamos la posición del jugador cada 5 segundos.
		// Con esto evitamos que los NPC's se queden sin hacer nada en un lugar escondido mientras pareciera que la música de horda
		// se reproduce sin ningún sentido.
		if ( LastHordeDisclose <= gpGlobals->curtime )
		{
			DisclosePlayer();
			LastHordeDisclose = gpGlobals->curtime + 5;
		}

		// No es una horda especial.
		if ( !TriggerHorde )
		{
			// Los NPC's de la horda han sido creados, pasar al modo relajado.
			if ( HordeQueue <= 0 )
				Relaxed();
		}
	}

	// Mientras no estemos en una "Horda" ni en el "Climax"
	// O en si, mientras estemos en "Relajado", "Exaltado" o "Jefe"
	if ( Status != HORDE && Status != CLIMAX )
	{
		// Dependiendo de la situación, agregamos un zombi a la cola de la próxima horda.
		MayQueueHordeChilds();

		// Mientras el usuario este con un "Grunt" el Director va "recolectando" zombis para la horda que se activará cuando este muera.
		// Por lo tanto mientras estemos en Grunt no debemos activar una horda.
		if ( Status != BOSS )
		{
			// Un segundo menos para el modo Horda.
			Left4Horde--;

			// ¡Hora de la horda!
			if ( Left4Horde <= 0 )
				Horde();
		}
	}

	// Checamos el estado de los NPC's.
	CheckChilds();

	// Checamos el estado de los Jefes.
	CheckBoss();

	// Actualizamos el nivel de enojo del Director.
	UpdateAngryLevel();

	// Volvemos a pensar dentro de 1 segundo.
	SetNextThink(gpGlobals->curtime + 1);
}

//=========================================================
// Pensar: Hordas para el final.
//=========================================================
void CDirector::FinalThink()
{
	// Estamos en espera.
	if ( Status == WAIT )
	{
		// Ha terminado la espera y
		// no hay pendiente la creación de un jefe.
		if ( ExitWait <= gpGlobals->curtime && !BossSpawnPending )
		{
			// Aún hay hordas por crear.
			if ( FinalWavesLeft > 0 )
			{
				// ¡Jefe!
				// No crear un jefe si esta es la penultima horda.
				// La última horda siempre será un jefe.
				if ( random->RandomInt(1, 3) == 2 && FinalBoss > 0 && FinalWavesLeft != 2 && FinalWavesLeft != FinalWaves || FinalWavesLeft == 1 )
				{
					SetStatus(BOSS);
					
					// ¡Es la última horda! 2 Jefes.
					if ( FinalWavesLeft == 1 )
						SpawnBoss(2);
					else
						SpawnBoss();
				}

				// Creamos una horda.
				else
				{
					// @TODO: Depende del nivel de dificultad.
					HordeQueue = random->RandomInt(15, 30);

					Horde();
					--FinalWavesLeft;
				}
			}

			// Hordas terminadas, ¡climax!
			else
			{
				InFinal = false;
				Climax();
			}
		}
	}

	// Sin hordas especiales.
	if ( TriggerHorde )
		TriggerHorde = false;

	// Estamos en una "Horda"
	if ( Status == HORDE )
	{
		// Los NPC's de la horda han sido eliminados.
		if ( LastHordeChilds <= 0 )
			Wait();
	}

	// Checamos el estado de los NPC's.
	CheckChilds();

	// Checamos el estado de los Jefes.
	CheckBoss();
}

//=========================================================
// Activa el Director
//=========================================================
void CDirector::Enable()
{
	Disabled = false;
}

//=========================================================
// Desactiva el Director
//=========================================================
void CDirector::Disable()
{
	Disabled = true;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LA ENTRADA (INPUTS)
//=========================================================
//=========================================================

//=========================================================
// INPUT: Forzar estado "Relajado"
//=========================================================
void CDirector::InputForceRelax(inputdata_t &inputdata)
{
	Relaxed();
}

//=========================================================
// INPUT: Forzar estado "Exaltado"
//=========================================================
void CDirector::InputForceExalted(inputdata_t &inputdata)
{
	SetStatus(EXALTED);
}

//=========================================================
// INPUT: Forzar estado "Horda"
//=========================================================
void CDirector::InputForceHorde(inputdata_t &inputdata)
{
	Horde(true);
}

//=========================================================
// INPUT: Forzar estado "Horda" hasta que se fuerze el
// estado "Relajado"
//=========================================================
void CDirector::InputForceTriggeredHorde(inputdata_t &inputdata)
{
	Horde(true, true);
}

//=========================================================
// INPUT: Forzar estado "Grunt"
//=========================================================
void CDirector::InputForceBoss(inputdata_t &inputdata)
{
	BossSpawnPending = true;
}

//=========================================================
// INPUT: Forzar climax
//=========================================================
void CDirector::InputForceClimax(inputdata_t &inputdata)
{
	Climax();
}

//=========================================================
// INPUT: Empezar el estado final.
//=========================================================
void CDirector::InputStartFinal(inputdata_t &inputdata)
{
	InFinal		= true;
	FinalWaves	= inputdata.value.Int();
	FinalBoss	= 1;

	// Dependiendo del nivel de dificultad serán las hordas y el número de jefes.
	// Fácil
	if ( InGameRules()->GetSkillLevel() == SKILL_EASY )
		--FinalWaves; // Una horda menos.

	// Normal
	if ( InGameRules()->GetSkillLevel() == SKILL_MEDIUM )
		FinalBoss	= 2;				// 2 Jefes

	// Dificil
	if ( InGameRules()->GetSkillLevel() == SKILL_HARD )
	{
		FinalBoss	= 2;				// 2 Jefes
		FinalWaves	= FinalWaves + 2;	// 2 hordas más.
	}

	// Algo malo paso aquí.
	if ( FinalWaves <= 0 )
		FinalWaves = 1;

	// Más alcance.
	director_min_maker_distance.SetValue(director_min_maker_distance.GetInt() + 500);

	OnFinalStart.FireOutput(NULL, this);

	FinalWavesLeft = FinalWaves;
	Wait();
}

//=========================================================
// INPUT: Acaba el final.
//=========================================================
void CDirector::InputEndFinal(inputdata_t &inputdata)
{
	FinalEnd();
}

//=========================================================
// INPUT: Establece una cantidad en segundos en el que
// el Director estara desactivado.
//=========================================================
void CDirector::InputSetDisabledFor(inputdata_t &inputdata)
{
	DirectorDisabled = gpGlobals->curtime + inputdata.value.Int();
}

//=========================================================
// INPUT: Establece la distancia minima de un creador de zombis.
//=========================================================
void CDirector::InputSetMakerDistance(inputdata_t &inputdata)
{
	director_min_maker_distance.SetValue(inputdata.value.Int());
}

//=========================================================
// INPUT: Establece la cantidad de zombis en la cola de la Horda.
//=========================================================
void CDirector::InputSetHordeQueue(inputdata_t &inputdata)
{
	director_force_horde_queue.SetValue(inputdata.value.Int());
}

//=========================================================
// INPUT: Revela la posición de los jugadores.
//=========================================================
void CDirector::InputDisclosePlayer(inputdata_t &inputdata)
{
	DisclosePlayer();
}

//=========================================================
// INPUT: Mata a todos los NPC's
//=========================================================
void CDirector::InputKillChilds(inputdata_t &inputdata)
{
	KillChilds();
}

//=========================================================
// INPUT: Mata a todos los NPC's no visibles por los jugadores.
//=========================================================
void CDirector::InputKillNoVisibleChilds(inputdata_t &inputdata)
{
	KillChilds(true);
}

//=========================================================
// INPUT: Activa el Director.
//=========================================================
void CDirector::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

//=========================================================
// INPUT: Desactiva el Director.
//=========================================================
void CDirector::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

//=========================================================
// INPUT: Activa o Desactiva el Director.
//=========================================================
void CDirector::InputToggle(inputdata_t &inputdata)
{
	if ( !Disabled )
		Disable();
	else
		Enable();
}

//=========================================================
// INPUT: Activa la música del Director.
//=========================================================
void CDirector::InputEnableMusic(inputdata_t &inputdata)
{
	AllowMusic = true;
}

//=========================================================
// INPUT: Desactiva la música del director.
//=========================================================
void CDirector::InputDisableMusic(inputdata_t &inputdata)
{
	StopMusic();
	AllowMusic = false;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LOS NPC's
//=========================================================
//=========================================================

//=========================================================
// Obtiene la máxima cantidad de npcs vivos dependiendo
// del nivel de dificultad.
//=========================================================
int CDirector::GetMaxChildsScale()
{
	// Número predeterminado de máximos NPC's vivos.
	float MaxChilds = director_max_alive_npcs.GetFloat();

	// Iván: Yo y mis matematicas.
	switch ( GameRules()->GetSkillLevel() )
	{
		// Fácil
		case SKILL_EASY:
			// Relajado: Dividir entre 3
			if ( Status == RELAXED && LastHordeChilds == 0 )
				MaxChilds = ceil(MaxChilds / 3);
		break;

		// Normal.
		case SKILL_MEDIUM:
			// Aumentamos 5
			MaxChilds = MaxChilds + 5;

			// Relajado: Dividimos entre 2
			if ( Status == RELAXED && LastHordeChilds == 0 )
				MaxChilds = ceil(MaxChilds / 2);
		break;

		// Dificil
		case SKILL_HARD:
			// Aumentamos 10
			MaxChilds = MaxChilds + 10;

			// Relajado: Restamos 5
			if ( Status == RELAXED && LastHordeChilds == 0 )
				MaxChilds = MaxChilds - 5;
		break;
	}

	// CLIMAX: Aumentamos 5
	if ( Status == CLIMAX )
		MaxChilds = MaxChilds + 5;

#ifdef ALPHA
	// Más de 30 NPC's causa lagg y problemas de audio...
	if ( MaxChilds > 30 )
		MaxChilds = 30;
#else
	// Más de 40 NPC's causa lagg y problemas de audio...
	if ( MaxChilds > 40 )
		MaxChilds = 40;
#endif

	return MaxChilds;
}

//=========================================================
// Devuelve la cantidad de NPC's vivos.
//=========================================================
int CDirector::CountChilds()
{
	ChildsAlive			= 0; // NPC's vivos.
	ChildsTargetPlayer	= 0; // NPC's con objetivo al jugador.
	CAI_BaseNPC *pNPC	= NULL;

	do
	{
		// Buscamos todos los NPC's en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_child");

		// Existe y sigue vivo.
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// Esta muy lejos de los jugadores.
		// no nos sirve de nada... matenlo.
		if ( IsTooFar(pNPC) )
		{
			CTakeDamageInfo damage;

			damage.SetDamage(pNPC->GetHealth());
			damage.SetDamageType(DMG_GENERIC);
			damage.SetAttacker(this);
			damage.SetInflictor(this);

			pNPC->TakeDamage(damage);
			continue;
		}

		// Por alguna razón hay más NPC's que la permitida...
		// FIX: Si se pasa de horda a relajado de forma instantanea, eliminara los zombis creados por la horda...
		/*if ( ChildsAlive > GetMaxChildsScale() )
		{
			UTIL_RemoveImmediate(pNPC);
			continue;
		}*/

		// Esta lo suficientemente cerca como para consideralo un peligro.
		if ( IsTooClose(pNPC) )
			ChildsTargetPlayer++;

		// + NPC vivo.
		ChildsAlive++;

	} while(pNPC);

	return ChildsAlive;
}

//=========================================================
// Verifica si es conveniente crear NPC's
//=========================================================
bool CDirector::MayQueueChilds()
{
	// Mientres el spawn este bloqueado :NO:
	if ( gpGlobals->curtime <= SpawnBlocked )
		return false;

	// Estamos en Climax, SI O SI
	if ( Status == CLIMAX )
	{
		if ( random->RandomInt(0, 50) > 10 )
			return true;
	}

	// Convertimos int a float
	// @TODO: ¿Alguna manera mejor?
	float MaxChilds = GetMaxChildsScale() + 0.0;

	if ( ChildsAlive >= MaxChilds )
		return false;

	// Cuando estemos en modo "Exaltado" debe haber almenos un tercio del limite de zombis.
	if ( Status == EXALTED && ChildsAlive < ceil(MaxChilds / 3) )
		return true;

	// Estamos en modo Grunt ¡no más zombis!
	// a menos que estemos en dificultad díficil.
	if ( Status == BOSS && !InGameRules()->IsSkillLevel(SKILL_HARD) )
		return false;

	int pPlayersHealth = UTIL_GetPlayersHealth();

	// Más probabilidades si se tiene mucha salud.
	if ( pPlayersHealth > 50 )
	{
		if ( random->RandomInt(pPlayersHealth, 100) < 60 )
			return false;
	}
	else
	{
		if ( random->RandomInt(1, 30) > 15 )
			return false;
	}

	return true;
}

//=========================================================
// Verifica si es conveniente poner en cola la creación de zombis
// para una próxima horda/evento de panico.
//=========================================================
bool CDirector::MayQueueHordeChilds()
{
	// ¡Azar!
	if ( random->RandomInt(1, 30) > 5 )
		return false;

	// ¡No más!
	if ( HordeQueue >= GetMaxChildsScale() )
		return false;

	if ( AngryLevel < ANGRY )
	{
		if ( UTIL_GetPlayersHealth() < 50 )
		{
			if ( random->RandomInt(1, 10) > 3 )
				return false;
		}
	}

	++HordeQueue;
	return true;
}

//=========================================================
// Checa los NPC's vivos
//=========================================================
void CDirector::CheckChilds()
{
	// Contamos los NPC's vivos.
	CountChilds();

	// El director esta obligado a no pasar esta cantidad de NPc's vivos.
	float MaxChilds = GetMaxChildsScale();

	// ¡Música de una Horda!
	if ( LastHordeChilds > 0 )
	{
		if ( !PlayingHordeMusic() && AllowMusic ) 
			Horde_Music = EmitMusic(STRING(SoundHorde));
	}
	else if ( PlayingHordeMusic() ) 
	{
		FadeoutMusic(Horde_Music);
		Horde_Music = NULL;
	}

	// No estamos en "Jefe", ni "Climax" ni en "Espera", ni en una Horda.
	if ( Status != BOSS && Status != CLIMAX && Status != WAIT && LastHordeChilds == 0 && AllowMusic )
	{
		// Dependiendo de la cantidad de NPC's atacando al jugador reproducimos una música de fondo.
		if ( ChildsTargetPlayer >= 8 )
		{
			bool a = false;
			bool b = false;

			if ( ChildsTargetPlayer >= 9 )
				a = true;

			if ( ChildsTargetPlayer >= 13 )
				b = true;

			EmitDangerMusic(a, b);
		}

		// Ya no tenemos NPC's cerca
		// y estabamos reproduciendo la música.
		else if ( ChildsTargetPlayer < 8 && PlayingDangerMusic() )
			FadeDangerMusic();
	}

	// Estabamos reproduciendo la música.
	else if ( PlayingDangerMusic() )
		FadeDangerMusic();

	// Estamos en una horda.
	if ( Status == HORDE && !TriggerHorde )
	{
		if ( HordeQueue > 0 && ChildsAlive < MaxChilds )
			SpawnQueue = HordeQueue;
	}

	// No estamos en espera.
	else if ( Status != WAIT )
	{
		// La cantidad de NPC's vivos es menor al limite y
		// es posible y recomendado crear npc's.
		if ( ChildsAlive <= MaxChilds && MayQueueChilds() )
		{
			// Npc's que faltan por crear.
			int Left4Creating	= MaxChilds - ChildsAlive;
			int MaxChildsQueue	= director_max_npcs_queue.GetInt();

			// En CLIMAX agregamos 5 al limite por crear.
			if ( Status == CLIMAX )
				MaxChildsQueue = MaxChildsQueue + 5;

			// Evitamos sobrecargas al director.
			if ( Left4Creating > MaxChildsQueue )
				SpawnQueue = MaxChildsQueue;
			else
				SpawnQueue = Left4Creating;
		}
	}

	// No hay más NPC's por crear.
	if ( SpawnQueue <= 0 )
		return;

	SpawnChilds();
}

//=========================================================
// Crea nuevos NPC's
//=========================================================
void CDirector::SpawnChilds()
{
	CDirectorSpawn *pMaker			= NULL;
	bool SpawnerFound				= false;
	bool Horde						= ( Status == HORDE || Status == CLIMAX ) ? true : false;

	do
	{
		// Si la cola de creación de NPC's es menor o igual a 0
		// hemos terminado.
		if ( SpawnQueue <= 0 )
			break;

		// Obtenemos la lista de las entidades "director_spawn" que podremos utilizar para
		// crear los NPC'S, estas deben estar cerca del jugador.
		// @TODO: En IA Director de L4D no es necesario una entidad, el director los ubica automaticamente en el suelo
		// ¿podremos crear algo así?

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

			if ( !pPlayer )
				continue;

			pMaker = (CDirectorSpawn *)gEntList.FindEntityByClassnameWithin(pMaker, "director_spawn", pPlayer->GetAbsOrigin(), director_min_maker_distance.GetFloat());

			// Ya no existe o esta desactivado.
			if ( !pMaker || !pMaker->Enabled() )
				continue;
		
			// No estamos en Climax
			if ( Status != CLIMAX )
			{
				// Este creador ha creado un NPC hace menos de 1 segundo...
				// ir al siguiente.
				if ( pMaker->LastSpawn >= (gpGlobals->curtime - 1) )
					continue;
			}

			// ¡hemos encontrado un lugar de creación!
			SpawnerFound	= true;
			// Cantidad inicial de npc's que creaaremos.
			int SpawnMount	= 1;
			
			// Estamos en una horda y toco suerte
			// crear todos los faltantes de una. ¿preparad@?
			if ( Horde && random->RandomInt(0, 2) == 1 )
				SpawnMount = SpawnQueue;

			CAI_BaseNPC *pNPC;

			// Creamos la cantidad de npcs en este Spawn.
			for ( int i = 0; i <= SpawnMount; ++i )
			{
				// Si creamos más de 1 npc, crearlo sin colisiones hacia los demás npc's.
				if ( SpawnMount > 1 )
					pNPC = pMaker->MakeNoCollisionNPC(Horde, Horde);
				else
					pNPC = pMaker->MakeNPC(Horde, Horde);

				// ¡No se ha podido crear el NPC!
				if ( !pNPC )
					continue;

				// Un NPC menos que crear.
				SpawnQueue--;

				if ( SpawnMount > 1 )
					Msg("%s <%s> Creando horda de NPC's <%i> (Quedan %i) \r\n", MS(), pPlayer->GetPlayerName(), SpawnMount, SpawnQueue);
				else
					Msg("%s <%s> Creando NPC <%s> (Quedan %i) \r\n", MS(), pPlayer->GetPlayerName(), pNPC->GetClassname(), SpawnQueue);

				// Estamos en una horda pero no en una horda especial.
				// Remover uno a la cola de la horda.
				if ( Status == HORDE && !TriggerHorde )
					--HordeQueue;

				// No estamos en Climax
				else if ( Status != CLIMAX )
				{
					// Un NPC más.
					++ChildsSpawned;

					if ( SpawnQueue <= 0 )
						LastSpawnChilds = gpGlobals->curtime;
				}

				// Estamos en exaltado, agregamos un NPC más a los creados en Exaltado.
				if ( Status == EXALTED )
					ChildsExaltedSpawned++;

				OnSpawnChild.FireOutput(pNPC, this);
			}
		}

	} while(pMaker);

	// No hemos encontrado un spawn
	if ( !SpawnerFound )
		DevMsg("%s Hay NPC's por crear pero no se encontro un director_spawn cercano. \r\n", MS());
}

//=========================================================
// Han matado a un NPC creado por el Director.
//=========================================================
void CDirector::ChildKilled(CBaseEntity *pVictim)
{
	if ( LastHordeChilds > 0 )
		--LastHordeChilds;

	// Liberamos el entindex de este NPC.
	pVictim->NetworkProp()->DetachEdict();

	ChildsKilled++;
	OnChildDead.FireOutput(NULL, this);
}

//=========================================================
// Matar a todos los NPC's creados por el Director.
//=========================================================
void CDirector::KillChilds(bool onlyNoVisible)
{
	CAI_BaseNPC *pNPC = NULL;

	do
	{
		// Buscamos todos los NPC's en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_child");
			
		// No existe o esta muerto.
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// Solo matar a los que no estan visibles.
		if ( onlyNoVisible )
		{
			if ( UTIL_IsPlayersVisible(pNPC->GetAbsOrigin()) )
				continue;
		}

		UTIL_Remove(pNPC);

	} while(pNPC);

}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL IA DEL DIRECTOR
//=========================================================
//=========================================================

//=========================================================
// Revela la posición del jugador a todos los NPC's.
//=========================================================
void CDirector::DisclosePlayer()
{
	CAI_BaseNPC *pNPC = NULL;

	do
	{
		// Buscamos todos los NPC's en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_child");

		// No existe o esta muerto.
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// Ya tiene a un jugador como enemigo.
		if ( pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() )
			continue;

		// Seleccionamos a un jugador al azar.
		// @TODO: ¿Al más cercano?
		CIN_Player *pPlayer = UTIL_GetRandomInPlayer();

		if ( !pPlayer )
			continue;

		// Le decimos que su nuevo enemigo es el jugador y le damos la ubicación de este.
		pNPC->SetEnemy(pPlayer);
		pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());

		// @TODO: ¿Alguna forma de obtener a los NPCs que se han quedado "atorados" o sin posibilidad de seguir al jugador?
		// es necesario eliminarlos o el estado de horda jamas acabará.

	} while(pNPC);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL JEFE
//=========================================================
//=========================================================

//=========================================================
// Verifica si es conveniente crear un Jefe
//=========================================================
bool CDirector::MayQueueBoss()
{
	// Hay pendiente la creación de un Jefe.
	if ( BossSpawnPending )
		return true;

	// Climax, ¡SI!
	if ( Status == CLIMAX )
		return true;

	int MinHealth			= 80;										// Salud minima del jugador.
	int MinChildsSpawned	= director_min_npc_boss.GetInt() + 100;		// Cantidad minima de NPC's eliminados para la creación de un jefe.

	switch ( InGameRules()->GetSkillLevel() )
	{
		// Normal
		case SKILL_MEDIUM:
			MinHealth			= 50;
			MinChildsSpawned	= director_min_npc_boss.GetInt();
		break;

		// Dificil
		case SKILL_HARD:
			MinHealth			= 20;
			MinChildsSpawned	= director_min_npc_boss.GetInt() / 2;
		break;
	}

	// La salud del jugador debe ser más de [depende dificultad] y
	// antes debemos crear una cantidad determinada de NPC's.
	if ( UTIL_GetPlayersHealth() < MinHealth || ChildsSpawned < MinChildsSpawned )
		return false;

	// Estamos en dificultad fácil.
	if ( GameRules()->IsSkillLevel(SKILL_EASY) )
	{
		// Deben haber menos de 10 zombis vivos.
		if ( ChildsAlive > 10 )
			return false;
	}

	// Menos oportunidades de aparecer ;)
	if ( random->RandomInt(1, 10) > 8 )
		return false;

	return true;
}

//=========================================================
// Devuelve la cantidad de jefes vivos.
//=========================================================
int CDirector::CountBoss()
{
	BossAlive			= 0;
	CAI_BaseNPC *pNPC	= NULL;

	do
	{
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_boss");

		if ( pNPC )
			BossAlive++;

	} while(pNPC);

	return BossAlive;
}

//=========================================================
// Checa los Jefes vivos y realiza las acciones correspondientes.
//=========================================================
void CDirector::CheckBoss()
{
	// Contar los Grunts vivos.
	CountBoss();

	// Debajo del if se hace lo necesario para reproducir la música de fondo del Jefe.
	// Debido a que el modo "Climax" tiene su propia música no es necesario ni bonito reproducir la música del Jefe al mismo tiempo.
	if ( Status != CLIMAX )
	{
		// Si hay más de un Jefe iniciar la música de los Jefes ¡cuidado Abigaíl!
		if ( BossAlive > 0 )
		{
			if ( Status != BOSS )
				SetStatus(BOSS);

			// Todos los jefes han sido creados.
			if ( !BossSpawnPending )
			{
				// Música predeterminada.
				const char *BossMusicName = STRING(SoundBoss);

				// ¡Hay más de 1 jefe! Reproducimos música especial.
				if ( BossAlive > 1 )
					BossMusicName = STRING(SoundBossBrothers);

				// No se esta reproduciendo la música y esta permitido.
				if ( !PlayingBossMusic() && AllowMusic )
					Boss_Music = EmitMusic(BossMusicName);
			}
		}

		// Todos los Jefes fueron eliminados. Ganaste esta vez abigaíl.
		else if ( BossAlive <= 0 && PlayingBossMusic() )
		{
			//DevMsg("%s Los Jefes han sido eliminados, deteniendo musica...\r\n", MS());
			FadeBossMusic();

			// Final, entramos a modo Espera.
			if ( InFinal )
			{
				--FinalBoss;

				// Si era la última horda, quitarla.
				if ( FinalWavesLeft == 1 )
					--FinalWavesLeft;

				Wait();
			}

			// Después de un Grunt hay muchos zombis ¿no?
			else
				Horde();

			OnBossDead.FireOutput(NULL, this);
		}
	}

	// Hay un jefe por crear y hay menos de 3 vivos.
	if ( MayQueueBoss() && BossAlive < 3 )
		SpawnBoss();
}

//=========================================================
// Crea un nuevo Jefe
//=========================================================
void CDirector::SpawnBoss(int pCount)
{
	CDirectorSpawn *pMaker		= NULL;
	bool SpawnerFound			= false;
	bool Spawned				= false;

	if ( !BossSpawnPending )
		BossSpawnPendingCount = pCount;

	do
	{
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CIN_Player *pPlayer = UTIL_InPlayerByIndex(i);

			if ( !pPlayer )
				continue;

			// Obtenemos la lista de las entidades "director_spawn" que podremos utilizar para
			// crear el Jefe, estas deben estar cerca del jugador.
			pMaker = (CDirectorSpawn *)gEntList.FindEntityByClassnameWithin(pMaker, "director_spawn", pPlayer->GetAbsOrigin(), (director_min_maker_distance.GetFloat() + 500));

			if ( !pMaker || !pMaker->Enabled() || Spawned )
				continue;
		
			// ¡hemos encontrado un lugar de creación!
			SpawnerFound = true;

			// Al parecer este creador no puede crear Jefes.
			if ( !pMaker->CanMakeBoss() )
				continue;
		
			CAI_BaseNPC *pNPC = pMaker->MakeBoss();

			if ( pNPC )
			{
				// Estamos en modo CLIMAX, no es necesario pasar a modo Jefe.
				if ( Status != CLIMAX )
					SetStatus(BOSS);

				// Reiniciamos el contador.
				ChildsSpawned		= 0;
				Spawned				= true;

				--BossSpawnPendingCount;

				if ( BossSpawnPendingCount > 0 )
					BossSpawnPending = true;
				else
					BossSpawnPending = false;

				OnSpawnBoss.FireOutput(pNPC, this);
				break;
			}
			else
				BossSpawnPending = true;
		}


	} while(pMaker);

	// No hemos encontrado un spawn
	if ( !SpawnerFound )
	{
		DevWarning("%s Hay un Jefe por crear pero no se encontro un director_spawn cercano o que admita la creacion de un Jefe \r\n", MS());
		BossSpawnPending = true;
	}
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL SONIDO/MUSICA
//=========================================================
//=========================================================

//=========================================================
// Inicia la música de las hordas de zombis.
//=========================================================
void CDirector::EmitDangerMusic(bool A, bool B)
{
	if ( !PlayingHordeMusic() )
		Danger_Music = EmitMusic(STRING(SoundDanger));

	// A
	if ( A && !PlayingDanger_A_Music() )
		Danger_A_Music = EmitMusic(STRING(SoundDangerA));

	if ( !A && PlayingDanger_A_Music() )
	{
		FadeoutMusic(Danger_A_Music);
		Danger_A_Music	= NULL;
	}

	// B
	if ( B && !PlayingDanger_B_Music() )
		Danger_B_Music = EmitMusic(STRING(SoundDangerB));

	if ( !B && PlayingDanger_B_Music() )
	{
		FadeoutMusic(Danger_B_Music);
		Danger_B_Music	= NULL;
	}
}

//=========================================================
// Desaparecer y parar música de fondo de la Horda.
//=========================================================
void CDirector::FadeDangerMusic()
{
	DevMsg("%s Bajando musica de peligro... \r\n", MS());

	FadeoutMusic(Danger_Music, 1.5f, true);
	FadeoutMusic(Danger_A_Music, 1.5f, true);
	FadeoutMusic(Danger_B_Music, 1.5f, true);

	Danger_Music	= NULL;
	Danger_A_Music	= NULL;
	Danger_B_Music	= NULL;
}

//=========================================================
// Desaparecer y parar música de fondo del Grunt.
//=========================================================
void CDirector::FadeBossMusic()
{
	DevMsg("%s Bajando musica del Jefe... \r\n", MS());

	FadeoutMusic(Boss_Music, 2.0f, true);
	Boss_Music = NULL;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A OTROS
//=========================================================
//=========================================================

int CDirector::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char message[512];

		int MaxChilds = GetMaxChildsScale();

		Q_snprintf(message, sizeof(message),
			"Estado: %s",
		GetStatusName());
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"NPC's creados: %i",
		ChildsSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"NPC's vivos: %i (de un maximo de %i)",
		ChildsAlive, MaxChilds);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"NPC's por crear: %i",
		SpawnQueue);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"NPC's cerca del jugador: %i",
		ChildsTargetPlayer);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"NPC's creados en modo Exaltado: %i",
		ChildsExaltedSpawned);
		EntityText(text_offset++, message, 0);

		// Información no necesaria en modo "En espera" y "CLIMAX"
		if ( Status != WAIT && Status != CLIMAX )
		{
			Q_snprintf(message, sizeof(message),
				"Cantidad de la Horda: %i (en %i segundos)",
			HordeQueue, Left4Horde);
			EntityText(text_offset++, message, 0);

			Q_snprintf(message, sizeof(message),
				"Hordas: %i",
			HordesPassed);
			EntityText(text_offset++, message, 0);
		}

		Q_snprintf(message, sizeof(message),
			"NPC's de la ultima horda: %i",
		LastHordeChilds);
		EntityText(text_offset++, message, 0);

		if ( Status == RELAXED )
		{
			Q_snprintf(message, sizeof(message),
				"Segundos para salir de [RELAX]: %i",
			Left4Exalted);
			EntityText(text_offset++, message, 0);
		}

		if ( InFinal )
		{
			Q_snprintf(message, sizeof(message),
				"Hordas del Final: %i (%i)",
			FinalWavesLeft, FinalWaves);
			EntityText(text_offset++, message, 0);

			Q_snprintf(message, sizeof(message),
				"Jefes del Final: %i",
			FinalBoss);
			EntityText(text_offset++, message, 0);
		}
	}

	return text_offset;
}

void CC_ForceSpawnBoss()
{
	CDirector *pDirector = NULL;
	pDirector = (CDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->SpawnBoss();
}

void CC_ForceHorde()
{
	CDirector *pDirector = NULL;
	pDirector = (CDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->Horde(true);
}

void CC_ForceClimax()
{
	CDirector *pDirector = NULL;
	pDirector = (CDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->Climax();
}

static ConCommand director_force_spawn_boss("director_force_spawn_boss",	CC_ForceSpawnBoss, "Fuerza al director a crear un Jefe.\n",					FCVAR_CHEAT);
static ConCommand director_force_horde("director_force_horde", CC_ForceHorde, "Fuerza al director a tomar el estado de Horda/Panico.\n",	FCVAR_CHEAT);
static ConCommand director_force_climax("director_force_climax", CC_ForceClimax, "Fuerza al director a tomar el estado de Climax.\n",		FCVAR_CHEAT);