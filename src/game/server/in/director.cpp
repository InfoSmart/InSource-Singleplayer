//=====================================================================================//
//
// InDirector
//
// Inteligencia encargada de controlar la m�sica de fondo, los zombis, el clima y otros
// aspectos del juego.
//
// Inspiraci�n: AI Director de Left 4 Dead: http://left4dead.wikia.com/wiki/The_Director
//
//=====================================================================================//

#include "cbase.h"
#include "director.h"

#include "in_player.h"
#include "scripted.h"

#include "npc_grunt.h"
#include "director_zombie_spawn.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "tier0/memdbgon.h";

/*
	NOTAS

	Grunt: Soldado robotico con 3,000 puntos de salud, se le considera el NPC m�s poderoso y enemigo del jugador
	Solo debe aparecer de 1 a 3 ocaciones en mapas.

	info_zombie_maker: Entidad (se coloca en el mapa) encargada de establecer una zona para crear zombis,
	es utilizado por el Director.
*/

/*
	* RELAJADO
	* El Director crea una cantidad minima de zombis con una salud baja con el fin de decorar
	* el escenario.

	* EXALTADO
	* Permite la creaci�n de m�s zombis que en el modo "Relajado" con el fin de dar un escenario
	* m�s apocal�ptico y aumentar la dificultad del juego en periodos de tiempo al azar.

	* HORDA
	* Conforme se va avanzando en el juego el Director va "recolectando" una cantidad de zombis
	* a soltar cuando se active este modo. Los zombis creados tienen una salud m�s alta y conocen la
	* ubicaci�n del jugador, trataran de atacarlo.

	* GRUNT
	* Los Grunt son NPC poderosos y fuertes, cuando uno aparece en el mapa el Director empieza a reproducir
	* una m�sica de fondo indicando la presencia del mismo y se detiene la creaci�n de zombis hasta que el mismo muera.
	* Igualmente mientras nos encontremos en este modo el Director va "recolectando" zombis para el modo "Horda" que ser�
	* activado cuando el Grunt muera.

	* CLIMAX
	* Este modo es activado manualmente desde el mapa (o la consola) para representar los �ltimos minutos de un capitulo.
	* En este modo la creaci�n de zombis aumenta considerablemente y se crean al menos 3 Grunts (Todos persiguiendo
	* al jugador), al igual que se reproduce una m�sica de fondo.
*/

//=========================================================
// Definici�n de variables de configuraci�n.
//=========================================================

ConVar indirector_enabled					("indirector_enabled",				"1",	FCVAR_REPLICATED, "Activa o desactiva el Director.");
ConVar indirector_max_alive_zombies			("indirector_max_alive_zombies",	"30",	FCVAR_REPLICATED, "Cantidad de zombis que el Director puede crear. (Aumentara con la dificultad)");
ConVar indirector_min_maker_distance		("indirector_min_maker_distance",	"2000", FCVAR_REPLICATED, "Distancia en la que se debe encontrar un director_zombie_maker del jugador para que el director pueda usarlo.");
ConVar indirector_min_zombies_grunt			("indirector_min_zombies_grunt",	"550",	FCVAR_REPLICATED, "Cantidad de zombis minima a crear antes de dar con la posibilidad de crear un Grunt");
ConVar indirector_max_zombies_queue			("indirector_max_zombies_queue",	"6",	FCVAR_REPLICATED, "Cantidad limite de zombis que el Director puede dejar en cola.");
ConVar indirector_min_zombie_distance		("indirector_min_zombie_distance",	"1800",	FCVAR_REPLICATED, "Distancia minima en la que se debe encontrar un zombi del jugador para considerlo un peligro (musica de horda)");
ConVar indirector_force_horde_queue			("indirector_force_horde_queue",	"0",	FCVAR_REPLICATED, "Fuerza a crear esta cantidad de zombis durante una horda.");
ConVar indirector_force_spawn_outview		("indirector_force_spawn_outview",	"1",	FCVAR_REPLICATED, "Fuerza a crear los zombis fuera del campo de visi�n del jugador.");

//=========================================================
// Guardado y definici�n de datos
//=========================================================

LINK_ENTITY_TO_CLASS( info_director, CInDirector );

BEGIN_DATADESC( CInDirector )

	DEFINE_SOUNDPATCH( pSound ),
	DEFINE_SOUNDPATCH( Sound_GruntMusic ),
	DEFINE_SOUNDPATCH( Sound_HordeMusic ),
	DEFINE_SOUNDPATCH( Sound_Horde_A_Music ),
	DEFINE_SOUNDPATCH( Sound_Horde_B_Music ),

	DEFINE_KEYFIELD( Disabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRelax",				InputForceRelax ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceExalted",			InputForceExalted ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceHorde",				InputForceHorde ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceTriggeredHorde",	InputForceTriggeredHorde ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceGrunt",				InputForceGrunt ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceClimax",			InputForceClimax ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetDisabledFor",		InputSetDisabledFor ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetMakerProximity",	InputSetMakerDistance ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetHordeQueue",		InputSetHordeQueue ),

	DEFINE_INPUTFUNC( FIELD_VOID,		"DisclosePlayer",		InputDisclosePlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"KillZombies",			InputKillZombies ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"KillNoVisibleZombies",	InputKillNoVisibleZombies ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",		InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",		InputToggle ),

END_DATADESC();

//=========================================================
// Constructor
//=========================================================
CInDirector::CInDirector()
{
	// Iniciamos relajados.
	Relaxed();
	DevMsg("%s Iniciado \r\n", MS());

	// Reiniciamos variables.

	AngryLevel			= HAPPY;

	Left4Horde			= random->RandomInt(100, 300);
	SpawnBlocked		= 0;
	DirectorDisabled	= 0;
	TriggerHorde		= false;

	GruntsAlive				= 0;
	GruntsMusic				= false;
	Sound_GruntMusic		= NULL;
	GruntSpawnPending		= false;
	GruntsKilled			= 0;

	PlayingHordeMusic			= false;
	Sound_HordeMusic			= NULL;
	PlayingHorde_A_Music		= false;
	Sound_Horde_A_Music			= NULL;
	PlayingHorde_B_Music		= false;
	Sound_Horde_B_Music			= NULL;

	LastSpawnZombies		= 0;
	HordeQueue				= 0;

	ZombiesAlive			= 0;
	ZombiesKilled			= 0;
	SpawnQueue				= indirector_max_zombies_queue.GetInt();
	ZombiesSpawned			= 0;
	ZombiesExaltedSpawned	= 0;

	// Pensamos �ahora!
	SetNextThink(gpGlobals->curtime);
}

//=========================================================
// Destructor
//=========================================================
CInDirector::~CInDirector()
{
}

//=========================================================
// Creaci�n
//=========================================================
void CInDirector::Spawn()
{
	BaseClass::Spawn();

	// Somos invisibles.
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);

	// Guardar en cach� objetos necesarios.
	Precache();
}

//=========================================================
// Guardar los objetos necesarios en cach�.
//=========================================================
void CInDirector::Precache()
{
	// Sonidos
	PrecacheScriptSound("Director.Climax");
	PrecacheScriptSound("Director.Miniclimax");

	PrecacheScriptSound("Director.Horde.Alert");
	PrecacheScriptSound("Director.Horde.Coming");

	PrecacheScriptSound("Director.Horde.Main");
	PrecacheScriptSound("Director.Horde.Main_A");
	PrecacheScriptSound("Director.Horde.Main_B");
	PrecacheScriptSound("Director.Horde.Main_C");

	PrecacheScriptSound("Director.Choir");

	/*PrecacheScriptSound("Director.Horde.Drums1");
	PrecacheScriptSound("Director.Horde.Drums1_A");

	PrecacheScriptSound("Director.Horde.Drums2");
	PrecacheScriptSound("Director.Horde.Drums2_A");

	PrecacheScriptSound("Director.Horde.Drums3");
	PrecacheScriptSound("Director.Horde.Drums3_A");

	PrecacheScriptSound("Director.Horde.Drums4");
	PrecacheScriptSound("Director.Horde.Drums4_A");

	PrecacheScriptSound("Director.Horde.Drums5");
	PrecacheScriptSound("Director.Horde.Drums5_A");

	PrecacheScriptSound("Director.Horde.Drums6");
	PrecacheScriptSound("Director.Horde.Drums6_A");*/

	BaseClass::Precache();
}

//=========================================================
// Establece el estado actual de InDirector
//=========================================================
void CInDirector::SetStatus(int status)
{
	DevMsg("%s %s -> %s \r\n", MS(), GetStatusName(Status), GetStatusName(status));
	Status = status;
}

//=========================================================
// Calcula el nivel de enojo del Director
//=========================================================
int CInDirector::CalculateAngryLevel()
{
	int NewAngryLevel	= HAPPY;
	int AngryPoints		= 0;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	// Director es amigable con el jugador, dependiendo del nivel
	// de dificultad del juego podremos calcular su "nivel de enojo"
	switch ( GameRules()->GetSkillLevel() )
	{
		// F�cil
		case SKILL_EASY:
		{
			// Tiene m�s del 90% de salud.
			if ( pPlayer->GetHealth() > 90 )
				AngryPoints++;

			// �Ha matado a la mitad de mis zombis!
			if ( ZombiesKilled > (ZombiesSpawned / 2)  )
				AngryPoints++;

			// Ha matado a 2 de mis Grunts y adem�s tiene m�s del 80% de vida
			// �que carajo?
			if ( GruntsKilled > 2 && pPlayer->GetHealth() > 80 )
				AngryPoints++;

			

			// @TODO: Verificaci�n del progreso del mapa.
			// @TODO: Verificaci�n de clima.
		}
		break;

		// Medio
		case SKILL_MEDIUM:
		{
			// Tiene m�s del 70% de salud.
			if ( pPlayer->GetHealth() > 70 )
				AngryPoints++;

			// �Ha matado a m�s de un tercio de mis zombis!
			if ( ZombiesKilled > (ZombiesSpawned / 3)  )
				AngryPoints++;

			// Ha matado a 1 de mis Grunts y adem�s tiene m�s del 60% de vida
			if ( GruntsKilled > 1 && pPlayer->GetHealth() > 60 )
				AngryPoints++;
		}
		break;

		// Dificil
		case SKILL_HARD:
		{
			// Tiene m�s del 60% de salud.
			if ( pPlayer->GetHealth() > 60 )
				AngryPoints++;

			// �Ha matado a m�s de un cuarto de mis zombis!
			if ( ZombiesKilled > (ZombiesSpawned / 4)  )
				AngryPoints++;

			// Ha matado a 1 de mis Grunts y adem�s tiene m�s del 40% de vida
			if ( GruntsKilled > 1 && pPlayer->GetHealth() > 40 )
				AngryPoints++;
		}
		break;
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
// Inicia el modo "Relajado"
//=========================================================
void CInDirector::Relaxed()
{
	SetStatus(RELAXED);

	Left4Exalted			= random->RandomInt(50, 150);
	ZombiesExaltedSpawned	= 0;
	TriggerHorde			= false;
}

//=========================================================
// Inicia el modo "Horda"
//=========================================================
void CInDirector::Horde(bool super, bool triggered)
{
	if ( Status == HORDE )
		return;

	// Sonido: ��Vamos por ti!!
	if ( super )
		EmitSound("Director.Horde.Alert");
	else
		EmitSound("Director.Horde.Coming");

	// Los que siguen vivos �Vayan por el!
	DisclosePlayer();

	if ( !triggered )
	{
		if ( HordeQueue < 20 )
			HordeQueue = HordeQueue + 5;

		if ( indirector_force_horde_queue.GetInt() != 0 )
			HordeQueue = indirector_force_horde_queue.GetInt();

		if ( super )
			HordeQueue = HordeQueue + 20;
	}

	Left4Horde		= random->RandomInt(100, 300);
	SpawnBlocked	= gpGlobals->curtime + 5;
	TriggerHorde	= triggered;

	SetStatus(HORDE);
	HordesPassed++;
}

//=========================================================
// Inicia el modo "Climax"
// Algo grande esta a punto de suceder...
//=========================================================
void CInDirector::Climax(bool mini)
{
	if ( Status == CLIMAX )
		return;

	CIN_Player *pPlayer	= GetInPlayer(UTIL_GetLocalPlayer());

	if ( mini )
		pPlayer->EmitMusic("Director.MiniClimax");
	else
		pPlayer->EmitMusic("Director.Climax");

	SetStatus(CLIMAX);
}

//=========================================================
// Devuelve la etiqueta de InDirector junto a su estado.
// �til para debugear en la consola. DevMsg y Warning
// TODO: Algo mejor que esto...
//=========================================================
const char *CInDirector::MS()
{
	char message[100];

	Q_snprintf(message, sizeof(message), "[InDirector] [%s]", GetStatusName());
	return message;
}

//=========================================================
// Tranforma el estado int a su nombre en string.
//=========================================================
const char *CInDirector::GetStatusName(int status)
{
	switch( status )
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

		case GRUNT:
			return "GRUNT";
		break;

		case CLIMAX:
			return "CLIMAX";
		break;
	}

	return "DESCONOCIDO";
}

//=========================================================
// Tranforma el estado actual a su nombre en string.
//=========================================================
const char *CInDirector::GetStatusName()
{
	return GetStatusName(Status);
}

//=========================================================
// Pensar: Bucle de ejecuci�n de tareas.
//=========================================================
void CInDirector::Think()
{
	BaseClass::Think();
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	// El director por ahora no funciona en Multiplayer.
	if ( GameRules()->IsMultiplayer() )
		return;

	// El Director esta desactivado.
	if ( !indirector_enabled.GetBool() || DirectorDisabled >= gpGlobals->curtime || Disabled )
	{
		if ( DirectorDisabled <= gpGlobals->curtime )
			DirectorDisabled = 0;

		SetNextThink(gpGlobals->curtime + 1);
		return;
	}

	// Estamos "Exaltados"
	if ( Status == EXALTED )
	{
		// Si la salud del jugador es mayor a 60%
		// y se han creado 35 zombis, pasar al modo Relajado.
		if ( pPlayer->GetHealth() > 60 && ZombiesExaltedSpawned > 35 )
			Relaxed();

		// Si la salud del jugador es mayor a 10%
		// y se han creado 15 zombis, pasar al modo Relajado.
		else if ( pPlayer->GetHealth() > 10 && ZombiesExaltedSpawned > 15 )
			Relaxed();

		// Abiga�l esta en muy mal estado (salud menor a 10%)
		// recordemos los buenos tiempos y dej�mosle vivir m�s tiempo...
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
	if ( Status == HORDE && !TriggerHorde )
	{
		// TODO: Terminar hasta que los zombis de la misma hayan sido eliminados.
		// La horda ha terminado.
		if ( HordeQueue <= 0 )
			Relaxed();
	}

	// Mientras no estemos en una "Horda" ni en el "Climax"
	// O en si, mientras estemos en "Relajado", "Exaltado" o "Grunt"
	if ( Status != HORDE && Status != CLIMAX )
	{
		// Dependiendo de la situaci�n, agregamos un zombi a la cola de la pr�xima horda.
		MayQueueHordeZombies();

		// Mientras el usuario este con un "Grunt" el Director va "recolectando" zombis para la horda que se activar� cuando este muera.
		// Por lo tanto mientras estemos en Grunt no debemos activar una horda.
		if ( Status != GRUNT )
		{
			// Un segundo menos para el modo Horda.
			Left4Horde--;

			// �Hora de la horda!
			if ( Left4Horde <= 0 )
				Horde();
		}
	}

	// Checamos el estado de los zombis.
	CheckZombies();

	// Checamos el estado de los Grunts.
	CheckGrunts();

	// Calculamos el nivel de enojo del Director.
	CalculateAngryLevel();

	// Volvemos a pensar dentro de 1 segundo
	SetNextThink(gpGlobals->curtime + 0.8);
}

//=========================================================
// Activa el Director
//=========================================================
void CInDirector::Enable()
{
	indirector_enabled.SetValue(1);
	Disabled = false;
}

//=========================================================
// Desactiva el Director
//=========================================================
void CInDirector::Disable()
{
	indirector_enabled.SetValue(0);
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
void CInDirector::InputForceRelax(inputdata_t &inputdata)
{
	Relaxed();
}

//=========================================================
// INPUT: Forzar estado "Exaltado"
//=========================================================
void CInDirector::InputForceExalted(inputdata_t &inputdata)
{
	SetStatus(EXALTED);
}

//=========================================================
// INPUT: Forzar estado "Horda"
//=========================================================
void CInDirector::InputForceHorde(inputdata_t &inputdata)
{
	Horde(true);
}

//=========================================================
// INPUT: Forzar estado "Horda" hasta que se fuerze el
// estado "Relajado"
//=========================================================
void CInDirector::InputForceTriggeredHorde(inputdata_t &inputdata)
{
	Horde(true, true);
}

//=========================================================
// INPUT: Forzar estado "Grunt"
//=========================================================
void CInDirector::InputForceGrunt(inputdata_t &inputdata)
{
	GruntSpawnPending = true;
}

//=========================================================
// INPUT: Forzar climax
//=========================================================
void CInDirector::InputForceClimax(inputdata_t &inputdata)
{
	Climax();
}

//=========================================================
// INPUT: Establece una cantidad en segundos en el que
// el Director estara desactivado.
//=========================================================
void CInDirector::InputSetDisabledFor(inputdata_t &inputdata)
{
	DirectorDisabled = gpGlobals->curtime + inputdata.value.Int();
}

//=========================================================
// INPUT: Establece la distancia minima de un creador de zombis.
//=========================================================
void CInDirector::InputSetMakerDistance(inputdata_t &inputdata)
{
	DevMsg("MIN MAKER %i\r\n", inputdata.value.Int());
	indirector_min_maker_distance.SetValue(inputdata.value.Int());
}

//=========================================================
// INPUT: Establece la cantidad de zombis en la cola de la Horda.
//=========================================================
void CInDirector::InputSetHordeQueue(inputdata_t &inputdata)
{
	indirector_force_horde_queue.SetValue(inputdata.value.Int());
}

void CInDirector::InputDisclosePlayer(inputdata_t &inputdata)
{
	DisclosePlayer();
}

void CInDirector::InputKillZombies(inputdata_t &inputdata)
{
	KillZombies();
}

void CInDirector::InputKillNoVisibleZombies(inputdata_t &inputdata)
{
	KillZombies(true);
}

//=========================================================
// INPUT: Activa el Director.
//=========================================================
void CInDirector::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

//=========================================================
// INPUT: Desactiva el Director.
//=========================================================
void CInDirector::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

//=========================================================
// INPUT: Activa o Desactiva el Director.
//=========================================================
void CInDirector::InputToggle(inputdata_t &inputdata)
{
	if( indirector_enabled.GetBool() )
		Disable();
	else
		Enable();
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A LOS ZOMBIS
//=========================================================
//=========================================================

//=========================================================
// Obtiene la m�xima cantidad de zombis vivos dependiendo
// del nivel de dificultad.
//=========================================================
int CInDirector::GetMaxZombiesScale()
{
	float MaxZombies = indirector_max_alive_zombies.GetFloat();

	switch ( GameRules()->GetSkillLevel() )
	{
		case SKILL_EASY:
			if ( Status == RELAXED )
				MaxZombies = ceil(MaxZombies / 3);
		break;

		case SKILL_MEDIUM:
			MaxZombies = MaxZombies + 5;

			if ( Status == RELAXED )
				MaxZombies = ceil(MaxZombies / 2);
		break;

		case SKILL_HARD:
			MaxZombies = MaxZombies + 10;

			if ( Status == RELAXED )
				MaxZombies = MaxZombies - 5;
		break;
	}

	if ( Status == CLIMAX )
		MaxZombies = MaxZombies + 5;

	// M�s de 40 zombis causa lagg y problemas de audio.
	if ( MaxZombies > 40 )
		MaxZombies = 40;

	return MaxZombies;
}

//=========================================================
// Cuenta la cantidad de Zombis vivos.
//=========================================================
int CInDirector::CountZombies()
{
	ZombiesAlive		= 0;
	ZombiesTargetPlayer	= 0;

	CAI_BaseNPC *pNPC		= NULL;
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();
	float minDistance		= indirector_min_maker_distance.GetFloat();

	ZombiesClassicAlive	= 0;
	ZombinesAlive		= 0;
	ZombiesFastAlive	= 0;
	ZombiesPoisonAlive	= 0;

	do
	{
		// Buscamos todos los zombis en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_zombie");

		if ( pNPC && pNPC->IsAlive() )
		{
			// Calculamos la distancia del zombi al jugador.
			Vector distToEnemy	= pNPC->GetAbsOrigin() - pPlayer->GetAbsOrigin();
			float dist			= VectorNormalize(distToEnemy);

			// Esta muy lejos del jugador
			// no nos sirve de nada... matenlo.
			if ( dist > (minDistance * 1.5) )
			{
				CTakeDamageInfo damage;

				damage.SetDamage(pNPC->GetHealth());
				damage.SetDamageType(DMG_GENERIC);
				damage.SetAttacker(this);
				damage.SetInflictor(this);

				pNPC->TakeDamage(damage);
				continue;
			}

			// Por alguna raz�n hay m�s zombis que la permitida...
			// FIX: Si se pasa de horda a relajado de forma instantanea, eliminara los zombis creados por la horda...
			if ( ZombiesAlive > (GetMaxZombiesScale() + 15) )
			{
				UTIL_RemoveImmediate(pNPC);
				continue;
			}

			// Esta lo suficientemente cerca como para consideralo un peligro.
			// y tiene como enemigo al jugador.
			if ( dist < indirector_min_zombie_distance.GetFloat() && pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() )
				ZombiesTargetPlayer++;

			// + Zombi vivo.
			ZombiesAlive++;

			// Obtenemos el nombre clase de este zombi y lo contamos en su variable.
			const char *className = pNPC->GetClassname();

			if ( className == "npc_zombie" )
				ZombiesClassicAlive++;

			if ( className == "npc_zombine" )
				ZombinesAlive++;

			if ( className == "npc_fastzombie" )
				ZombiesFastAlive++;

			if ( className == "npc_poisonzombie" )
				ZombiesPoisonAlive++;
		}

	} while(pNPC);

	return ZombiesAlive;
}

//=========================================================
// Verifica si es conveniente crear zombis
//=========================================================
bool CInDirector::MayQueueZombies()
{
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return false;

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
	// @TODO: �Alguna manera mejor?
	float MaxZombies = GetMaxZombiesScale() + 0;

	if ( ZombiesAlive >= MaxZombies )
		return false;

	// Cuando estemos en modo "Exaltado" debe haber almenos un tercio del limite de zombis.
	if ( Status == EXALTED && ZombiesAlive < ceil(MaxZombies / 3) )
		return true;

	// Estamos en modo Grunt �no m�s zombis!
	if ( Status == GRUNT )
		return false;

	// M�s probabilidades si se tiene mucha salud.
	if ( pPlayer->GetHealth() > 50 )
	{
		if ( random->RandomInt(pPlayer->GetHealth(), 100) < 60 )
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
// Verifica si es conveniente poner en cola la creaci�n de zombis
// para una pr�xima horda/evento de panico.
//=========================================================
bool CInDirector::MayQueueHordeZombies()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return false;

	// �Azar!
	if ( random->RandomInt(1, 30) > 5 )
		return false;

	// �No m�s!
	if ( HordeQueue >= GetMaxZombiesScale() )
		return false;

	if ( AngryLevel < ANGRY )
	{
		if ( pPlayer->GetHealth() < 50 )
		{
			if ( random->RandomInt(1, 10) > 3 )
				return false;
		}
	}

	HordeQueue++;
	return true;
}

//=========================================================
// Checa los Zombis vivos
//=========================================================
void CInDirector::CheckZombies()
{
	// Contamos los zombis vivos.
	CountZombies();

	// El director esta obligado a no pasar esta cantidad de zombis vivos.
	float MaxZombies = GetMaxZombiesScale();

	// El limite de zombis estan vivos.
	if ( ZombiesAlive >= MaxZombies && !ZombieChoir )
	{
		EmitSound("Director.Choir");
		ZombieChoir = true;
	}
	else if ( ZombiesAlive < MaxZombies )
		ZombieChoir = false;

	// No estamos en "Grunt" o en el "Climax"
	if ( Status != GRUNT && Status != CLIMAX )
	{
		// Dependiendo de la cantidad de zombis atacando al jugador reproducimos una m�sica de fondo.
		if ( ZombiesTargetPlayer >= 8 )
		{
			bool a = false;
			bool b = false;

			if ( ZombiesTargetPlayer >= 9 )
				a = true;

			if ( ZombiesTargetPlayer >= 13 )
				b = true;

			EmitHordeMusic(a, b);
		}

		// Ya no tenemos zombis cerca
		// y estabamos reproduciendo la m�sica.
		else if ( ZombiesTargetPlayer < 8 && PlayingHordeMusic )
			FadeoutHordeMusic();
	}

	// Estabamos reproduciendo la m�sica.
	else if ( PlayingHordeMusic )
		FadeoutHordeMusic();

	// Estamos en una horda.
	if ( Status == HORDE && !TriggerHorde )
	{
		if ( HordeQueue > 0 && ZombiesAlive < MaxZombies )
			SpawnQueue = HordeQueue;
	}
	else
	{
		// La cantidad de zombis vivos es menor al limite y
		// adem�s ya no hay ning�n zombi por crear en la cola y
		// es posible y recomendado crear zombis.
		if ( ZombiesAlive <= MaxZombies && MayQueueZombies() )
		{
			// Zombis que faltan por crear.
			int Left4Creating	= MaxZombies - ZombiesAlive;
			int MaxZombiesQueue	= indirector_max_zombies_queue.GetInt();

			// En CLIMAX agregamos 5 al limite de zombis por crear.
			if ( Status == CLIMAX )
				MaxZombiesQueue = MaxZombiesQueue + 5;

			// Evitamos sobrecargas al director.
			if ( Left4Creating > MaxZombiesQueue )
				SpawnQueue = MaxZombiesQueue;
			else
				SpawnQueue = Left4Creating;
		}
	}

	// No hay m�s zombis por crear.
	if ( SpawnQueue <= 0 )
		return;

	SpawnZombies();
}

//=========================================================
// Crea nuevos zombis
//=========================================================
void CInDirector::SpawnZombies()
{
	CDirectorZombieSpawn *pMaker			= NULL;
	CBasePlayer *pPlayer			= UTIL_GetLocalPlayer();

	bool SpawnerFound				= false;
	bool Horde						= ( Status == HORDE || Status == CLIMAX ) ? true : false;

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return;

	do
	{
		// Si la cola de creaci�n de zombis es menor o igual a 0
		// hemos terminado.
		if ( SpawnQueue <= 0 )
			break;

		// Obtenemos la lista de las entidades "director_zombie_maker" que podremos utilizar para
		// crear los zombis, estas deben estar cerca del jugador.
		// TODO: En IA Director de L4D no es necesario una entidad, el director los ubica automaticamente en el suelo
		// �podremos crear algo as�?

		pMaker = (CDirectorZombieSpawn *)gEntList.FindEntityByClassnameWithin(pMaker, "director_zombie_maker", pPlayer->GetAbsOrigin(), indirector_min_maker_distance.GetFloat());

		if ( pMaker )
		{
			// Si el estado actual no es Climax
			if ( Status != CLIMAX )
			{
				// Este creador de zombis ha creado uno hace menos de 2 segundos...
				// ir al siguiente.
				if ( pMaker->LastSpawn >= (gpGlobals->curtime - 2) )
					continue;
			}

			// �hemos encontrado un lugar de creaci�n!
			SpawnerFound = true;

			DevMsg("%s Creando zombi - Quedan: %i \r\n", MS(), SpawnQueue);
			CAI_BaseNPC *pNPC = pMaker->MakeNPC(Horde, Horde);

			// �No se ha podido crear el zombi!
			if ( !pNPC )
				continue;

			// Un zombi menos que crear.
			SpawnQueue--;

			if ( Status == HORDE && !TriggerHorde )
				HordeQueue--;
			else if ( Status != CLIMAX )
			{
				ZombiesSpawned++;

				if ( SpawnQueue <= 0 )
					LastSpawnZombies = gpGlobals->curtime;
			}

			if ( Status == EXALTED )
				ZombiesExaltedSpawned++;
		}

	} while(pMaker);

	// No hemos encontrado un spawn
	if ( !SpawnerFound )
		DevMsg("%s Hay zombis por crear pero no se encontro un director_zombie_maker cercano. \r\n", MS());
}

void CInDirector::KillZombies(bool onlyNoVisible)
{
	CAI_BaseNPC *pNPC		= NULL;
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	do
	{
		// Buscamos todos los zombis en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_zombie");

		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		if ( onlyNoVisible )
		{
			if ( pPlayer->FVisible(pNPC->GetAbsOrigin()) )
				continue;
		}

		CTakeDamageInfo damage;

		damage.SetDamage(pNPC->GetHealth());
		damage.SetDamageType(DMG_GENERIC);
		damage.SetAttacker(this);
		damage.SetInflictor(this);

		pNPC->TakeDamage(damage);

	} while(pNPC);

}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL IA DEL DIRECTOR
//=========================================================
//=========================================================

//=========================================================
// Revela la posici�n del jugador a los zombis.
//=========================================================
void CInDirector::DisclosePlayer()
{
	CAI_BaseNPC *pNPC		= NULL;
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return;

	do
	{
		// Buscamos todos los zombis en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_zombie");

		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		pNPC->SetEnemy(pPlayer);
		pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());

	} while(pNPC);
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL GRUNT
//=========================================================
//=========================================================

//=========================================================
// Verifica si es conveniente crear un Grunt
//=========================================================
bool CInDirector::MayQueueGrunt()
{
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return false;

	// Hay pendiente la creaci�n de un Grunt.
	if ( GruntSpawnPending )
		return true;

	// Climax, ��MUERE!!
	if ( Status == CLIMAX )
		return true;

	int MinHealth			= 80;
	int MinZombiesSpawned	= indirector_min_zombies_grunt.GetInt() + 100;

	switch ( GameRules()->GetSkillLevel() )
	{
		case SKILL_MEDIUM:
			MinHealth			= 50;
			MinZombiesSpawned	= indirector_min_zombies_grunt.GetInt();
		break;

		case SKILL_HARD:
			MinHealth			= 20;
			MinZombiesSpawned	= indirector_min_zombies_grunt.GetInt() / 2;
		break;
	}

	// La salud del jugador debe ser m�s de [depende dificultad] y
	// antes debemos crear una cantidad determinada de zombis.
	if ( pPlayer->GetHealth() < MinHealth || ZombiesSpawned < MinZombiesSpawned )
		return false;

	// Estamos en dificultad f�cil.
	if ( GameRules()->IsSkillLevel(SKILL_EASY) )
	{
		// Deben haber menos de 10 zombis vivos.
		if ( ZombiesAlive > 10 )
			return false;
	}

	// Menos oportunidades de aparecer ;)
	if ( random->RandomInt(1, 10) > 8 )
		return false;

	return true;
}

//=========================================================
// Cuenta cuantos Grunts estan vivos.
//=========================================================
int CInDirector::CountGrunts()
{
	GruntsAlive			= 0;
	CNPC_Grunt *pGrunt	= NULL;

	do
	{
		pGrunt = (CNPC_Grunt *)gEntList.FindEntityByClassname(pGrunt, "npc_grunt");

		if ( pGrunt )
			GruntsAlive++;

	} while(pGrunt);

	return GruntsAlive;
}

//=========================================================
// Checa los Grunts vivos y realiza las acciones correspondientes.
//=========================================================
void CInDirector::CheckGrunts()
{
	// Contar los Grunts vivos.
	CountGrunts();

	// Debajo del if se hace lo necesario para reproducir la m�sica de fondo del Grunt.
	// Debido a que el modo "Climax" tiene su propia m�sica no es necesario ni bonito reproducir la m�sica del Grunt al mismo tiempo.
	if ( Status != CLIMAX )
	{
		// Si hay m�s de un Grunt iniciar la m�sica de los Grunt �cuidado Abiga�l!
		if ( GruntsAlive > 0 )
		{
			if ( Status != GRUNT )
				SetStatus(GRUNT);

			if ( !GruntsMusic )
			{
				DevMsg("%s Reproduciendo musica de fondo Grunt.\r\n", MS());
				CIN_Player *pPlayer	= GetInPlayer(UTIL_GetLocalPlayer());

				Sound_GruntMusic	= pPlayer->EmitMusic("NPC_Grunt.BackgroundMusic");
				GruntsMusic			= true;
			}
		}

		// Todos los Grunts fueron eliminados. Ganaste esta vez abiga�l.
		else if ( GruntsAlive <= 0 && GruntsMusic )
		{
			DevMsg("[InDirector] El/Los Grunt han sido eliminados, deteniendo musica...\r\n");

			FadeoutGruntMusic();

			// Despu�s de un Grunt hay muchos zombis �no?
			Horde();
		}
	}

	if ( MayQueueGrunt() && GruntsAlive < 3 )
		SpawnGrunt();
}

//=========================================================
// Crea un nuevo Grunt
//=========================================================
void CInDirector::SpawnGrunt()
{
	CDirectorZombieSpawn *pMaker		= NULL;
	CBasePlayer *pPlayer				= UTIL_GetLocalPlayer();
	bool SpawnerFound					= false;

	// �El jugador no ha sido creado?
	if ( !pPlayer )
		return;

	do
	{
		// Obtenemos la lista de las entidades "director_zombie_maker" que podremos utilizar para
		// crear el Grunt, estas deben estar cerca del jugador.
		pMaker = (CDirectorZombieSpawn *)gEntList.FindEntityByClassnameWithin(pMaker, "director_zombie_maker", pPlayer->GetAbsOrigin(), indirector_min_maker_distance.GetFloat());

		if ( pMaker )
		{
			// �hemos encontrado un lugar de creaci�n!
			SpawnerFound = true;

			// Si este spawn esta permitido para crear un Grunt
			if ( pMaker->CanMakeGrunt() )
			{
				DevMsg("%s Creando grunt!! \r\n", MS());
				CAI_BaseNPC *pNPC = pMaker->MakeGrunt();

				if ( pNPC )
				{
					// Estamos en modo CLIMAX, no es necesario pasar a modo GRUNT.
					if ( Status != CLIMAX )
						SetStatus(GRUNT);

					// Reiniciamos el contador.
					ZombiesSpawned		= 0;
					GruntSpawnPending	= false;
				}

				break;
			}
		}

	} while(pMaker);

	// No hemos encontrado un spawn
	if ( !SpawnerFound )
	{
		DevMsg("%s Hay un Grunt por crear pero no se encontro un director_zombie_maker cercano o que admita la creacion de Grunt \r\n", MS());
		GruntSpawnPending = true;
	}
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL SONIDO/MUSICA
//=========================================================
//=========================================================

//=========================================================
// Inicia la m�sica de las hordas de zombis.
//=========================================================
void CInDirector::EmitHordeMusic(bool A, bool B)
{
	CIN_Player *pPlayer	= GetInPlayer(UTIL_GetLocalPlayer());

	if ( !PlayingHordeMusic )
	{
		Sound_HordeMusic	= pPlayer->EmitMusic("Director.Horde.Main");
		PlayingHordeMusic	= true;
	}

	// A
	if ( A && !PlayingHorde_A_Music )
	{
		Sound_Horde_A_Music		= pPlayer->EmitMusic("Director.Horde.Main_A");
		PlayingHorde_A_Music	= true;
	}

	if ( !A && PlayingHorde_A_Music )
	{
		pPlayer->FadeoutMusic(Sound_Horde_A_Music);
		PlayingHorde_A_Music	= false;
		Sound_Horde_A_Music		= NULL;
	}

	// B
	if ( B && !PlayingHorde_B_Music )
	{
		if ( random->RandomInt(0, 5) > 3 )
			Sound_Horde_B_Music		= pPlayer->EmitMusic("Director.Horde.Main_B");
		else
			Sound_Horde_B_Music		= pPlayer->EmitMusic("Director.Horde.Main_C");

		PlayingHorde_B_Music	= true;
	}

	if ( !B && PlayingHorde_B_Music )
	{
		pPlayer->FadeoutMusic(Sound_Horde_B_Music);
		PlayingHorde_B_Music	= false;
		Sound_Horde_B_Music		= NULL;
	}
}

//=========================================================
// Desaparecer y parar m�sica de fondo de la Horda.
//=========================================================
void CInDirector::FadeoutHordeMusic()
{
	DevMsg("%s Bajando musica de la horda... \r\n", MS());
	CIN_Player *pPlayer	= GetInPlayer(UTIL_GetLocalPlayer());

	pPlayer->FadeoutMusic(Sound_HordeMusic);
	pPlayer->FadeoutMusic(Sound_Horde_A_Music);
	pPlayer->FadeoutMusic(Sound_Horde_B_Music);

	Sound_HordeMusic			= NULL;

	PlayingHordeMusic			= false;
	PlayingHorde_A_Music		= false;
	PlayingHorde_B_Music		= false;
}

//=========================================================
// Desaparecer y parar m�sica de fondo del Grunt.
//=========================================================
void CInDirector::FadeoutGruntMusic()
{
	DevMsg("%s Bajando musica del Grunt... \r\n", MS());
	CIN_Player *pPlayer	= GetInPlayer(UTIL_GetLocalPlayer());

	pPlayer->FadeoutMusic(Sound_GruntMusic, 2.0f);
	GruntsMusic = false;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A OTROS
//=========================================================
//=========================================================

int CInDirector::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char message[512];

		int MaxZombies = GetMaxZombiesScale();

		Q_snprintf(message, sizeof(message),
			"Estado: %s",
		GetStatusName());
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Zombis creados: %i",
		ZombiesSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Zombis vivos: %i (de un maximo de %i)",
		ZombiesAlive, MaxZombies);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Zombis por crear: %i",
		SpawnQueue);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Zombis cerca del jugador: %i",
		ZombiesTargetPlayer);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Zombis creados en modo Exaltado: %i",
		ZombiesExaltedSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Cantidad de la Horda: %i (en %i segundos)",
		HordeQueue, Left4Horde);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Segundos para salir de [RELAX]: %i",
		Left4Exalted);
		EntityText(text_offset++, message, 0);
	}

	return text_offset;
}

void CC_ForceSpawnGrunt()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->SpawnGrunt();
}

void CC_ForceHorde()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->Horde(true);
}

void CC_ForceClimax()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "info_director");

	if ( !pDirector )
		return;

	pDirector->Climax();
}

static ConCommand indirector_force_spawn_grunt("indirector_force_spawn_grunt",	CC_ForceSpawnGrunt,		"Fuerza al director a crear un Grunt.\n",					FCVAR_CHEAT);
static ConCommand indirector_force_horde("indirector_force_horde",				CC_ForceHorde,			"Fuerza al director a tomar el estado de Horda/Panico.\n",	FCVAR_CHEAT);
static ConCommand indirector_force_climax("indirector_force_climax",			CC_ForceClimax,			"Fuerza al director a tomar el estado de Climax.\n",		FCVAR_CHEAT);