//=====================================================================================//
//
// InDirector
//
// Inteligencia encargada de controlar la música de fondo, los zombis, el clima y otros
// aspectos del juego.
//
// Inspiración: AI Director de Left 4 Dead: http://left4dead.wikia.com/wiki/The_Director
//
//=====================================================================================//

#include "cbase.h"
#include "director.h"

#include "in_player.h"
#include "scripted.h"

#include "npc_grunt.h"
#include "director_zombie_maker.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "tier0/memdbgon.h"

/*
	NOTAS

	Grunt: Soldado robotico con 3,000 de salud, se le considera el NPC más poderoso y enemigo del jugador
	Solo debe aparecer de 1 a 3 ocaciones en mapas.

	npc_zombie_maker: Entidad (se coloca en el mapa) encargada de definir una zona para crear zombis,
	es utilizado por el Director.
*/

//=========================================================
// Definición de variables de configuración.
//=========================================================

ConVar indirector_enabled				("indirector_enabled", "1", FCVAR_REPLICATED, "Activa o desactiva el Director.");
ConVar indirector_max_zombies			("indirector_max_zombies", "40", FCVAR_REPLICATED, "Cantidad de zombis que el Director puede crear.");
ConVar indirector_maker_promity			("indirector_maker_promity", "1900", FCVAR_REPLICATED, "Distancia en la que se debe encontrar un director_zombie_maker del jugador para que el director pueda usarlo.");
ConVar indirector_min_zombies_grunt		("indirector_min_zombies_grunt", "150", FCVAR_REPLICATED, "Cantidad de zombis minima a crear antes de dar con la posibilidad de crear un Grunt");
ConVar indirector_max_zombies_queue		("indirector_max_zombies_queue", "6", FCVAR_REPLICATED, "Cantidad limite de zombis que el Director puede dejar en cola.");
//ConVar indirector_respawn_interval		("indirector_respawn_interval", "2", FCVAR_REPLICATED, "Cantidad de minutos que se debe esperar después de haber creado a todos los zombis.");
ConVar indirector_min_zombie_distance	("indirector_min_zombie_distance", "350", FCVAR_REPLICATED, "Distancia minima en la que se debe encontrar un zombi del jugador para considerlo un peligro (musica de horda)");
ConVar indirector_horde_queue			("indirector_horde_queue", "0", FCVAR_REPLICATED, "Fuerza a crear esta cantidad de zombis durante una horda.");
ConVar indirector_force_spawn_outview	("indirector_force_spawn_outview", "1", FCVAR_REPLICATED, "Fuerza a crear esta los zombis fuera del campo de visión del jugador.");

//=========================================================
// Estados de InDirector
//=========================================================

enum
{
	RELAXED = 0,	// Relajado: Vamos con calma.
	EXALTED,		// Exaltado: Algunos zombis por aquí y una música aterradora.
	HORDE,			// Horda: ¡Ataquen mis valientes!
	GRUNT,			// Grunt: Estado especial indicando la aparición de un Grunt.
	CLIMAX			// Climax: ¡¡Se nos va!! ¡Ataquen sin compasión carajo!
};

//=========================================================
// Guardado y definición de datos
//=========================================================

LINK_ENTITY_TO_CLASS(info_director, CInDirector);

BEGIN_DATADESC(CInDirector)

	DEFINE_INPUTFUNC(FIELD_VOID, "ForceRelax",		InputForceRelax),
	DEFINE_INPUTFUNC(FIELD_VOID, "ForceExalted",	InputForceExalted),
	DEFINE_INPUTFUNC(FIELD_VOID, "ForceHorde",		InputForceHorde),
	DEFINE_INPUTFUNC(FIELD_VOID, "ForceGrunt",		InputForceGrunt),
	DEFINE_INPUTFUNC(FIELD_VOID, "ForceClimax",		InputForceClimax),

	DEFINE_INPUTFUNC(FIELD_VOID, "SetDisabledFor",		InputSetDisabledFor),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetMakerProximity",	InputSetMakerProximity),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetHordeQueue",		InputSetHordeQueue),

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

	m_left4Horde		= random->RandomInt(100, 300);
	t_blockedSpawn		= 0;
	t_disabledDirector	= 0;

	m_gruntsAlive			= 0;
	m_gruntsMusic			= false;
	s_gruntMusic			= NULL;
	m_gruntSpawnPending		= false;

	m_playingHordeMusic			= false;
	s_HordeMusic				= NULL;
	m_playingHordeAMusic		= false;
	s_HordeMusic				= NULL;
	m_playingHordeBMusic		= false;
	s_HordeBMusic				= NULL;

	t_lastSpawnZombis		= 0;
	m_zombiesHordeQueue		= 0;

	m_zombiesAlive			= 0;
	m_zombieSpawnQueue		= indirector_max_zombies_queue.GetInt();
	m_zombiesSpawned		= 0;
	m_zombiesExaltedSpawned = 0;

	// Pensamos ¡ahora!
	SetNextThink(gpGlobals->curtime);
}

//=========================================================
// Destructor
//=========================================================
CInDirector::~CInDirector()
{
}

//=========================================================
// Aparecer
//=========================================================
void CInDirector::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	Precache();

	Vector vecForward;
	AngleVectors(EyeAngles(), &vecForward);

	Vector vecOrigin = GetAbsOrigin() + vecForward;
	QAngle vecAngles(0, GetAbsAngles().y - 90, 0);

	CAI_ScriptedSchedule *m_Scripted = (CAI_ScriptedSchedule *)CBaseEntity::Create("aiscripted_schedule", vecOrigin, vecAngles, NULL);
	m_Scripted->SetName(MAKE_STRING("director_horde_follow"));
	m_Scripted->SetTargetName(MAKE_STRING("director_zombie"));
	m_Scripted->SetNPCState(NPC_STATE_COMBAT);
	m_Scripted->SetSchedule(SCHED_SCRIPT_ENEMY_IS_GOAL_AND_RUN_TO_GOAL);
	m_Scripted->SetInterruptability(DAMAGEORDEATH_INTERRUPTABILITY);
	m_Scripted->SetGoalEnt(MAKE_STRING("!player"));
}

//=========================================================
// Guardar los objetos necesarios en caché.
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

	PrecacheScriptSound("Director.Horde.Drums1");
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
	PrecacheScriptSound("Director.Horde.Drums6_A");

	BaseClass::Precache();
}

//=========================================================
// Establece el estado actual de InDirector
//=========================================================
void CInDirector::SetStatus(int status)
{
	DevMsg("%s %s -> %s \r\n", MS(), GetStatusName(m_status), GetStatusName(status));
	m_status = status;
}

//=========================================================
// Devuelve el estado actual de InDirector
//=========================================================
int CInDirector::Status()
{
	return m_status;
}

//=========================================================
// Ajusta el modo como "Relajado"
//=========================================================
void CInDirector::Relaxed()
{
	SetStatus(RELAXED);

	m_left4Relax			= random->RandomInt(50, 150);
	m_zombiesExaltedSpawned = 0;
}

//=========================================================
// Ajusta el modo como "Horda"
//=========================================================
void CInDirector::Horde(bool super)
{
	if (Status() == HORDE)
		return;

	// Sonido: ¡¡Vamos por ti!!
	if (super)
		EmitSound("Director.Horde.Alert");
	else
		EmitSound("Director.Horde.Coming");

	SetStatus(HORDE);

	if (m_zombiesHordeQueue < 20)
		m_zombiesHordeQueue = m_zombiesHordeQueue + 5;

	if (indirector_horde_queue.GetInt() != 0)
		m_zombiesHordeQueue = indirector_horde_queue.GetInt();

	if (super)
		m_zombiesHordeQueue = m_zombiesHordeQueue + 20;

	m_left4Horde	= random->RandomInt(100, 300);
	t_blockedSpawn	= gpGlobals->curtime + 5;
}

//=========================================================
// Ajusta el modo como "Climax"
// Algo grande esta a punto de suceder...
//=========================================================
void CInDirector::Climax()
{
	if (Status() == CLIMAX)
		return;

	CIN_Player *pPlayer	= ToInPlayer(UTIL_GetLocalPlayer());

	pPlayer->EmitMusic("Director.Climax");
	SetStatus(CLIMAX);
}


//=========================================================
// Devuelve la etiqueta de InDirector junto a su estado.
// Útil para debugear en la consola. DevMsg y Warning
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
	switch(status)
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
	return GetStatusName(Status());
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CInDirector::Think()
{
	BaseClass::Think();
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	// El Director esta desactivado.
	if (!indirector_enabled.GetBool() || t_disabledDirector >= gpGlobals->curtime)
	{
		t_disabledDirector = 0;

		SetNextThink(gpGlobals->curtime + 1);
		return;
	}

	// Estamos "Exaltados"
	if (Status() == EXALTED)
	{
		// Exaltado solo sirve para crear más zombis que en relajado.
		// Dependiendo del nivel de salud del jugador.

		if (pPlayer->GetHealth() > 60 && m_zombiesExaltedSpawned > 35)
			Relaxed();
		else if (pPlayer->GetHealth() > 10 && m_zombiesExaltedSpawned > 20)
			Relaxed();
	}

	// Estamos "Relajados"
	if (Status() == RELAXED)
	{
		m_left4Relax--;

		if (m_left4Relax <= 0)
			SetStatus(EXALTED);
	}

	// Estamos en una "Horda"
	if (Status() == HORDE)
	{
		// La horda ha terminado.
		if (m_zombiesHordeQueue <= 0)
			Relaxed();
	}

	// Mientras no estemos en una "Horda" ni en "Climax"
	if (Status() != HORDE && Status() != CLIMAX)
	{
		// Dependiendo de la situación, agregamos un zombi a la cola de la próxima horda.
		MayQueueHordeZombies();

		// Mientras el usuario este con un "Grunt" el Director va "recolectando" zombis para la horda que se activará cuando este muera.
		// Por lo tanto mientras estemos en Grunt no debemos activar una horda.
		if (Status() != GRUNT)
		{
			m_left4Horde--;

			if (m_left4Horde <= 0)
				Horde();
		}
	}

	// Checamos el estado de los zombis.
	CheckZombieMaker();

	// Checamos el estado de los Grunts.
	CheckGrunts();
	
	// Volvemos a pensar dentro de 1 segundo
	SetNextThink(gpGlobals->curtime + 1);
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
	Horde();
}

//=========================================================
// INPUT: Forzar estado "Grunt"
//=========================================================

void CInDirector::InputForceGrunt(inputdata_t &inputdata)
{
	m_gruntSpawnPending = true;
}

//=========================================================
// INPUT: Forzar climax
//=========================================================

void CInDirector::InputForceClimax(inputdata_t &inputdata)
{
	Climax();
}

void CInDirector::InputSetDisabledFor(inputdata_t &inputdata)
{
	t_disabledDirector = gpGlobals->curtime + inputdata.value.Int();
}

void CInDirector::InputSetMakerProximity(inputdata_t &inputdata)
{
	indirector_maker_promity.SetValue(inputdata.value.Int());
}

void CInDirector::InputSetHordeQueue(inputdata_t &inputdata)
{
	indirector_horde_queue.SetValue(inputdata.value.Int());
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL IA DEL DIRECTOR
//=========================================================
//=========================================================

//=========================================================
// Verifica si es conveniente crear zombis
//=========================================================
bool CInDirector::MayQueueZombies()
{
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	// ¿El jugador no ha sido creado?
	if (!pPlayer)
		return false;

	// Mientres el spawn este bloqueado :NO:
	if (gpGlobals->curtime < t_blockedSpawn)
		return false;

	// Estamos en Climax, SI O SI
	if (Status() == CLIMAX)
	{
		if (random->RandomInt(0, 50) > 10)
			return true;
	}

	// Cuando estemos en modo "Exaltado" debe haber almenos un tercio del limite de zombis.
	if (Status() == EXALTED && m_zombiesAlive < ceil(indirector_max_zombies.GetFloat() / 3))
		return true;

	// Estamos en modo Grunt ¡no más zombis!
	if (Status() == GRUNT)
		return false;

	// Estamos en modo relajado, no debe haber más de 10 zombis vivos.
	if (Status() == RELAXED && m_zombiesAlive >= 10)
		return false;

	// 
	if (t_lastSpawnZombis > (gpGlobals->curtime - 20))
		return false;

	// Más probabilidades si se tiene mucha salud.
	if (pPlayer->GetHealth() > 30)
	{
		if (random->RandomInt(pPlayer->GetHealth(), 100) < 60)
			return false;
	}
	else
	{
		if (random->RandomInt(1, 30) > 15)
			return false;
	}

	return true;
}

//=========================================================
// Verifica si es conveniente poner en cola la creación de zombis
// para una próxima horda/evento de panico.
//=========================================================
bool CInDirector::MayQueueHordeZombies()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	// ¿El jugador no ha sido creado?
	if (!pPlayer)
		return false;

	if (random->RandomInt(1, 30) > 5)
		return false;

	if (pPlayer->GetHealth() < 50)
	{
		if (random->RandomInt(1, 10) > 3)
			return false;
	}

	m_zombiesHordeQueue++;
	return true;
}

//=========================================================
// Verifica si es conveniente crear un Grunt
//=========================================================
bool CInDirector::MayQueueGrunt()
{
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();

	// ¿El jugador no ha sido creado?
	if (!pPlayer)
		return false;

	// Hay pendiente la creación de un Grunt.
	if (m_gruntSpawnPending)
		return true;

	// ¡¡MUERE!!
	if (Status() == CLIMAX)
		return true;

	// La salud del jugador debe ser más de 50% y
	// antes debemos crear una cantidad determinada de zombis.
	if (pPlayer->GetHealth() < 50 || m_zombiesSpawned < indirector_min_zombies_grunt.GetInt())
		return false;

	// Deben haber menos de 10 zombis vivos.
	if (m_zombiesAlive > 10)
		return false;

	// Menos oportunidades de aparecer ;)
	if (random->RandomInt(1, 10) > 8)
		return false;

	return true;
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL DIRECTOR ZOMBIE MAKER
//=========================================================
//=========================================================

//=========================================================
// Cuenta la cantidad de Zombis vivos.
//=========================================================
int CInDirector::CountZombies()
{
	m_zombiesAlive			= 0;
	m_zombiesTargetPlayer	= 0;

	CAI_BaseNPC *pNPC		= NULL;
	CBasePlayer *pPlayer	= UTIL_GetLocalPlayer();
	float minDistance		= indirector_min_zombie_distance.GetFloat();

	m_zombiesClassicAlive	= 0;
	m_zombinesAlive			= 0;
	m_zombiesFastAlive		= 0;
	m_zombiesPoisonAlive	= 0;
	
	do
	{
		// Buscamos todos los zombis en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, "director_zombie");

		if (pNPC)
		{
			// Al parecer este zombi tiene en la mira al jugador.
			if (pNPC->GetEnemy() == pPlayer)
			{
				// Calculamos la distancia del zombi al jugador.
				Vector distToEnemy	= pNPC->GetAbsOrigin() - pPlayer->GetAbsOrigin();
				float dist			= VectorNormalize(distToEnemy);

				// Si esta muy lejos o la última vez que vio a un enemigo (jugador) fue hace más de 3 minutos
				// no nos sirve de nada... matenlo.
				if (dist >= (minDistance * 2) || pNPC->GetLastEnemyTime() < (gpGlobals->curtime + (3 * 60)))
					pNPC->AddFlag(EFL_KILLME);

				// Esta lo suficientemente cerca como para consideralo un peligro.
				if (dist < minDistance)
					m_zombiesTargetPlayer++;
			}

			// + Zombi vivo.
			m_zombiesAlive++;

			// Obtenemos el nombre clase de este zombi y lo contamos en su variable.
			const char *className = pNPC->GetClassname();

			if (className == "npc_zombie")
				m_zombiesClassicAlive++;

			if (className == "npc_zombine")
				m_zombinesAlive++;

			if (className == "npc_fastzombie")
				m_zombiesFastAlive++;

			if (className == "npc_poisonzombie")
				m_zombiesPoisonAlive++;
		}

	} while(pNPC);

	return m_zombiesAlive;
}

//=========================================================
// Checa los Zombis vivos y 
//=========================================================
void CInDirector::CheckZombieMaker()
{
	// Contar los zombis vivos.
	CountZombies();

	// El director esta obligado a no pasar esta cantidad de zombis vivos.
	float maxZombies = indirector_max_zombies.GetFloat();

	// En estado Relajado hay que disminuir el limite de zombis vivos.
	if (Status() == RELAXED)
		maxZombies = ceil(maxZombies / 3);

	// En estado CLIMAX no hay compasión.
	if (Status() == CLIMAX)
		maxZombies = maxZombies + 30;	
	
	// No estamos en "Grunt" o en el "Climax"
	if (Status() != GRUNT && Status() != CLIMAX)
	{
		// Dependiendo de la cantidad de zombis atacando al jugador reproducimos una música de fondo.
		// [FIXME] Tiene problemas al intentar quitarlo.

		if (m_zombiesTargetPlayer >= 6 || Status() == HORDE)
		{
			bool a = false;
			bool b = false;

			if (m_zombiesTargetPlayer >= 8)
				a = true;

			if (m_zombiesTargetPlayer >= 10)
				b = true;

			EmitHordeMusic(a, b);
		}
		else if (m_zombiesTargetPlayer < 6 && m_playingHordeMusic)
			FadeoutHordeMusic();
	}

	if (Status() == HORDE)
	{
		if (m_zombiesHordeQueue > 0)
			m_zombieSpawnQueue = m_zombiesHordeQueue;
	}
	else
	{
		// Los zombis vivos son menos que el limite y 
		// además ya no hay ningún zombi por crear en la cola y
		// es posible y recomendado crear zombis.
		if (m_zombiesAlive < maxZombies && m_zombieSpawnQueue <= 0 && MayQueueZombies())
		{
			// Zombis que faltan por crear.
			int zombiesDifference	= maxZombies - m_zombiesAlive;
			int maxZombiesQueue		= indirector_max_zombies_queue.GetInt();

			// En CLIMAX agregamos 10 al limite de zombis por crear.
			if (Status() == CLIMAX)
				maxZombiesQueue = maxZombiesQueue + 10;

			// Evitamos sobrecargas al director.
			if (zombiesDifference > maxZombiesQueue)
				m_zombieSpawnQueue = maxZombiesQueue;
			else
				m_zombieSpawnQueue = zombiesDifference;
		}
	}

	// No hay más zombis por crear.
	if (m_zombieSpawnQueue <= 0)
		return;

	SpawnZombies();
}

//=========================================================
// Crea nuevos zombis
//=========================================================
void CInDirector::SpawnZombies()
{
	CNPCZombieMaker *pMaker			= NULL;
	CAI_ScriptedSchedule *pScripted = NULL;
	CBasePlayer *pPlayer			= UTIL_GetLocalPlayer();

	bool spawnerFound				= false;
	bool horde						= (Status() == HORDE || Status() == CLIMAX) ? true : false;

	if (!pPlayer)
		return;

	// Si estamos en modo "Horda" o "Climax" usaremos esta entidad para hacer que los zombis
	// vayan/sigan directamente al jugador.
	if (Status() == HORDE || Status() == CLIMAX)
		pScripted = (CAI_ScriptedSchedule *)gEntList.FindEntityByName(pScripted, "director_horde_follow");

	do
	{
		// Obtenemos la lista de las entidades "npc_zombie_maker" que podremos utilizar para
		// crear los zombis, estas deben estar cerca del jugador.
		// TODO: En IA Director de L4D no es necesario una entidad, el director los ubica automaticamente en el suelo
		// ¿podremos crear algo así?

		pMaker = (CNPCZombieMaker *)gEntList.FindEntityByClassnameWithin(pMaker, "director_zombie_maker", pPlayer->GetAbsOrigin(), indirector_maker_promity.GetFloat());

		if (pMaker)
		{
			// ¡hemos encontrado un lugar de creación!
			spawnerFound = true;

			// Si la cola de creación de zombis es mayor a 0 y
			// el numero generado al azar da 1 (Una forma de dar más realismo y utilizar más de 1 spawn)
			if (m_zombieSpawnQueue > 0 && random->RandomInt(1, 2) == 1)
			{
				DevMsg("%s Creando zombi - Quedan: %i \r\n", MS(), m_zombieSpawnQueue);
				CAI_BaseNPC *pNPC = pMaker->MakeNPC(horde);

				if (pNPC)
				{
					// Un zombi menos que crear.
					m_zombieSpawnQueue--;

					if (Status() == HORDE)
					{
						m_zombiesHordeQueue--;

						// Perseguir al jugador.
						if (pScripted)
							pScripted->StartSchedule(pNPC);
					}
					else if (Status() == CLIMAX)
					{
						// Perseguir al jugador.
						if (pScripted)
							pScripted->StartSchedule(pNPC);
					}
					else
					{
						m_zombiesSpawned++;

						if (m_zombieSpawnQueue <= 0)
							t_lastSpawnZombis = gpGlobals->curtime;
					}

					if (Status() == EXALTED)
						m_zombiesExaltedSpawned++;
				}
				else
					Warning("%s Ha ocurrido un problema al crear un Zombi. \r\n", MS());
			}
		}

	} while(pMaker);

	// No hemos encontrado un spawn
	if (!spawnerFound)
		DevMsg("%s Hay zombis por crear pero no se encontro un director_zombie_maker cercano. \r\n", MS());
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL GRUNT
//=========================================================
//=========================================================

//=========================================================
// Cuenta cuantos Grunts estan vivos.
//=========================================================
int CInDirector::CountGrunts()
{
	m_gruntsAlive		= 0;
	CNPC_Grunt *pGrunt	= NULL;

	do
	{
		pGrunt = (CNPC_Grunt *)gEntList.FindEntityByClassname(pGrunt, "npc_grunt");

		if (pGrunt)
			m_gruntsAlive++;

	} while(pGrunt);

	return m_gruntsAlive;
}

//=========================================================
// Checa los Grunts vivos y realiza las acciones correspondientes.
//=========================================================
void CInDirector::CheckGrunts()
{
	// Contar los Grunts vivos.
	CountGrunts();

	// Debajo del if se hace lo necesario para reproducir la música de fondo del Grunt.
	// Debido a que el modo "Climax" tiene su propia música no es necesario ni bonito reproducir la música del Grunt.
	if (Status() != CLIMAX)
	{
		// Si hay más de un Grunt iniciar la música de los Grunt ¡cuidado abigaíl!
		if (m_gruntsAlive > 0)
		{
			DevMsg("%s Se han detectado Grunts en el juego.\r\n", MS());

			if (Status() != GRUNT)
				SetStatus(GRUNT);

			if (!m_gruntsMusic)
			{
				DevMsg("%s Reproduciendo musica de fondo Grunt.\r\n", MS());
				CIN_Player *pPlayer	= ToInPlayer(UTIL_GetLocalPlayer());

				s_gruntMusic	= pPlayer->EmitMusic("NPC_Grunt.BackgroundMusic");
				m_gruntsMusic	= true;
			}
		}
		// Todos los Grunts fueron eliminados. Ganas esta vez abigaíl.
		else if (m_gruntsAlive <= 0 && m_gruntsMusic)
		{
			DevMsg("[InDirector] El/Los Grunt han sido eliminados, deteniendo musica...\r\n");

			FadeoutGruntMusic();

			// Después de un Grunt hay muchos zombis ¿no?
			Horde();
		}
	}

	if (MayQueueGrunt() && m_gruntsAlive < 3)
		SpawnGrunt();
}

//=========================================================
// Crea un nuevo Grunt
//=========================================================
void CInDirector::SpawnGrunt()
{
	CNPCZombieMaker *pMaker			= NULL;
	CAI_ScriptedSchedule *pScripted = NULL;
	CBasePlayer *pPlayer			= UTIL_GetLocalPlayer();
	bool spawnerFound				= false;

	if (!pPlayer)
		return;

	pScripted = (CAI_ScriptedSchedule *)gEntList.FindEntityByName(pScripted, "director_horde_follow");

	do
	{
		// Obtenemos la lista de las entidades "npc_zombie_maker" que podremos utilizar para
		// crear los zombis, estas deben estar cerca del jugador.
		pMaker = (CNPCZombieMaker *)gEntList.FindEntityByClassnameWithin(pMaker, "director_zombie_maker", pPlayer->GetAbsOrigin(), indirector_maker_promity.GetFloat());

		if (pMaker)
		{
			// ¡hemos encontrado un lugar de creación!
			spawnerFound = true;

			// Si este spawn esta permitido para crear un Grunt
			if (pMaker->CanMakeGrunt())
			{
				DevMsg("%s Creando grunt!! \r\n", MS());
				CAI_BaseNPC *pNPC = pMaker->MakeGrunt();
					
				// Perseguir al jugador
				if (pScripted)
					pScripted->StartSchedule(pNPC);

				// Estamos en modo CLIMAX, no es necesario pasar a modo GRUNT.
				if (Status() != CLIMAX)
					SetStatus(GRUNT);

				// Reiniciamos el contador.
				m_zombiesSpawned	= 0;
				m_gruntSpawnPending = false;

				break;
				return;
			}
		}

	} while(pMaker);

	// No hemos encontrado un spawn
	if (!spawnerFound)
	{
		DevMsg("%s Hay un Grunt por crear pero no se encontro un director_zombie_maker cercano o que admita la creacion de Grunt \r\n", MS());
		m_gruntSpawnPending = true;
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
void CInDirector::EmitHordeMusic(bool A, bool B)
{
	CIN_Player *pPlayer	= ToInPlayer(UTIL_GetLocalPlayer());

	if (!m_playingHordeMusic)
	{
		s_HordeMusic		= pPlayer->EmitMusic("Director.Horde.Main");
		m_playingHordeMusic	= true;
	}

	// A
	if (A && !m_playingHordeAMusic)
	{
		s_HordeAMusic		 = pPlayer->EmitMusic("Director.Horde.Main_A");
		m_playingHordeAMusic = true;
	}

	if (!A && m_playingHordeAMusic)
	{
		pPlayer->FadeoutMusic(s_HordeAMusic, 1.0f);
		m_playingHordeAMusic = false;
	}

	// B
	if (B && !m_playingHordeBMusic)
	{
		s_HordeBMusic			= pPlayer->EmitMusic("Director.Horde.Main_B");
		m_playingHordeBMusic	= true;
	}

	if (!B && m_playingHordeBMusic)
	{
		pPlayer->FadeoutMusic(s_HordeBMusic, 1.0f);
		m_playingHordeBMusic = false;
	}
}

//=========================================================
// Desaparecer y parar música de fondo de la Horda.
//=========================================================
void CInDirector::FadeoutHordeMusic()
{
	DevMsg("%s Bajando musica de la horda... \r\n", MS());
	CIN_Player *pPlayer	= ToInPlayer(UTIL_GetLocalPlayer());

	pPlayer->FadeoutMusic(s_HordeMusic);
	pPlayer->FadeoutMusic(s_HordeAMusic);
	pPlayer->FadeoutMusic(s_HordeBMusic);

	m_playingHordeMusic			= false;
	m_playingHordeAMusic		= false;
	m_playingHordeBMusic		= false;
}

//=========================================================
// Desaparecer y parar música de fondo del Grunt.
//=========================================================
void CInDirector::FadeoutGruntMusic()
{
	DevMsg("%s Bajando musica del Grunt... \r\n", MS());
	CIN_Player *pPlayer	= ToInPlayer(UTIL_GetLocalPlayer());

	pPlayer->FadeoutMusic(s_gruntMusic, 2.0f);
	m_gruntsMusic = false;
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

		float maxZombies = indirector_max_zombies.GetFloat();

		if (Status() == RELAXED)
			maxZombies = ceil(maxZombies / 3);

		Q_snprintf(message, sizeof(message), 
			"Estado: %s", 
		GetStatusName());
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Zombis creados: %i", 
		m_zombiesSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Zombis vivos: %i (de un maximo de %f)", 
		m_zombiesAlive, maxZombies);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Zombis cerca del jugador: %i", 
		m_zombiesTargetPlayer);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Zombis creados en modo Exaltado: %i", 
		m_zombiesExaltedSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Zombis en la cola de creacion: %i", 
		m_zombieSpawnQueue);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Cantidad de la Horda: %i (en %i segundos)", 
		m_zombiesHordeQueue, m_left4Horde);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message), 
			"Segundos para salir de [RELAX]: %i", 
		m_left4Relax);
		EntityText(text_offset++, message, 0);
	}

	return text_offset;
}

void CC_ForceSpawnGrunt()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "in_director");

	if (!pDirector)
		return;

	pDirector->SpawnGrunt();
}

void CC_ForceHorde()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "in_director");

	if (!pDirector)
		return;

	pDirector->Horde(true);
}

void CC_ForceClimax()
{
	CInDirector *pDirector = NULL;
	pDirector = (CInDirector *)gEntList.FindEntityByClassname(pDirector, "in_director");

	if (!pDirector)
		return;

	pDirector->Climax();
}

static ConCommand indirector_force_spawn_grunt("indirector_force_grunt", CC_ForceSpawnGrunt, "Fuerza al director a crear un Grunt.\n", FCVAR_CHEAT);
static ConCommand indirector_force_horde("indirector_force_horde", CC_ForceHorde, "Fuerza al director a tomar el estado de Horda/Panico.\n", FCVAR_CHEAT);
static ConCommand indirector_force_climax("indirector_force_climax", CC_ForceClimax, "Fuerza al director a tomar el estado de Climax.\n", FCVAR_CHEAT);