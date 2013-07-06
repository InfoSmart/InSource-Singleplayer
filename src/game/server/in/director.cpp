//=====================================================================================//
//
// Inteligencia artificial encargada de la creación de enemigos alrededor de los jugadores.
//
// Inspiración: I.A. Director de Left 4 Dead
//
//=====================================================================================//

#include "cbase.h"
#include "director.h"
#include "director_manager.h"

#include "in_player.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "soundent.h"
#include "engine/ienginesound.h"

#include "ai_basenpc.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de comandos de consola.
//=========================================================

ConVar director_debug("director_debug", "0", 0, "Activa la depuración y herramientas de testeo del Director");
ConVar director_force_panic_queue("director_force_panic_queue", "0", 0, "Cantidad de la próxima horda.");

ConVar director_max_alive("director_max_alive", "30", 0, "Cantidad máxima de hijos vivos.");

ConVar director_min_distance("director_min_distance", "800", 0, "Distancia minima en la que se debe encontrar un hijo de los jugadores.");
ConVar director_max_distance("director_max_distance", "3500", 0, "Distancia máxima en la que se debe encontrar un hijo de los jugadores.");

ConVar director_kills_boss("director_kills_boss", "250", 0, "Cantidad de hijos muertos para tener la posibilidad de crear un Jefe.");
ConVar director_danger_distance("director_danger_distance", "1800", 0, "Distancia en la que se debe encontrar un hijo para considerarlo un peligro.");

ConVar director_spawn_outview("director_spawn_outview", "1", 0, "¿Crear hijos fuera del campo de visión de los jugadores?");

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef APOCALYPSE
	LINK_ENTITY_TO_CLASS(info_director, CDirector);
#endif

BEGIN_DATADESC( CDirector )

	/* KEYFIELDS */
	//DEFINE_KEYFIELD( Disabled,	FIELD_BOOLEAN,		"StartDisabled" ),

	DEFINE_KEYFIELD( Sounds[0],	FIELD_SOUNDNAME,	"SoundClimax" ),
	DEFINE_KEYFIELD( Sounds[1],	FIELD_SOUNDNAME,	"SoundMiniClimax" ),
	DEFINE_KEYFIELD( Sounds[2],	FIELD_SOUNDNAME,	"SoundHorde" ),
	DEFINE_KEYFIELD( Sounds[3],	FIELD_SOUNDNAME,	"SoundDanger" ),
	DEFINE_KEYFIELD( Sounds[4],	FIELD_SOUNDNAME,	"SoundDangerA" ),
	DEFINE_KEYFIELD( Sounds[5],	FIELD_SOUNDNAME,	"SoundDangerB" ),
	DEFINE_KEYFIELD( Sounds[6],	FIELD_SOUNDNAME,	"SoundBoss" ),
	DEFINE_KEYFIELD( Sounds[7],	FIELD_SOUNDNAME,	"SoundBossBrothers" ),
	DEFINE_KEYFIELD( Sounds[9],	FIELD_SOUNDNAME,	"SoundEscape" ),
	// ¡No agregar más sonidos! O cambia el valor máximo de la variable "Sounds"

END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CDirector::CDirector()
{
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
// Inicialización
//=========================================================
void CDirector::Init()
{
	// Iniciamos "Relajados"
	Relaxed();

	// Iniciamos a nuestro ayudante.
	if ( DirectorManager() )
		DirectorManager()->Init();

	Disabled4Seconds	= 0;
	LastDisclose		= gpGlobals->curtime;

	InfinitePanic		= false;
	PanicQueue			= 0;
	PanicChilds			= 0;
	PanicCount			= 0;

	RestartPanic();

	SpawnQueue		= 0;
	ChildsAlive		= 0;
	ChildsTooClose	= 0;

	InitMusic();
}

//=========================================================
// Inicializa la música.
//=========================================================
void CDirector::InitMusic()
{
	DangerMusic		= new EnvMusic(STRING(Sounds[3]));
	DangerMusic_A	= new EnvMusic(STRING(Sounds[4]));
	DangerMusic_B	= new EnvMusic(STRING(Sounds[5]));

	HordeMusic		= new EnvMusic(STRING(Sounds[2]));

	ClimaxMusic		= new EnvMusic(STRING(Sounds[0]));
	BossMusic		= new EnvMusic(STRING(Sounds[6]));
}

//=========================================================
// Creación
//=========================================================
void CDirector::Spawn()
{
	// Somos invisibles.
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_NONE);

	// Guardar en caché objetos necesarios.
	Precache();

	// Inicializamos al Director
	Init();

	BaseClass::Spawn();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CDirector::Precache()
{
	// Sonidos.
	// Estos son los sonidos que se pueden cambiar en la construcción de un mapa. (mappers)
	for ( int i = 0; i <= ARRAYSIZE(Sounds); ++i )
	{
		string_t pSoundName = Sounds[i];
		Assert(pSoundName == NULL_STRING);

		if ( pSoundName == NULL_STRING )
			continue;

		PrecacheScriptSound(STRING(pSoundName));
	}

	// Sonidos que no se deben cambiar.
	PrecacheScriptSound("Director.Horde.Alert");
	PrecacheScriptSound("Director.Horde.Coming");
	PrecacheScriptSound("Director.Choir");

	// Modelos de los enemigos.
	// @TODO: Ser más flexible.
	UTIL_PrecacheOther("npc_zombie");
	UTIL_PrecacheOther("npc_fastzombie");
	UTIL_PrecacheOther("npc_zombine");
	UTIL_PrecacheOther("npc_poisonzombie");
	UTIL_PrecacheOther("npc_grunt");
	UTIL_PrecacheOther("npc_burned");

	BaseClass::Precache();
}

//=========================================================
// Pensar: Bucle de ejecución de tareas.
//=========================================================
void CDirector::Think()
{
	BaseClass::Think();

	// Director desactivado
	if ( Disabled4Seconds > 0 || Disabled )
	{
		// Un segundo menos para la activación.
		if ( Disabled4Seconds > 0 )
			--Disabled4Seconds;

		// Volvemos a pensar en 1 segundo.
		SetNextThink(gpGlobals->curtime + 1);
		return;
	}

	// Modo "Relajado"
	if ( Status == RELAXED )
	{
		// Hora de pasar a "Exaltado"
		if ( Left4Exalted.IsElapsed() )
			SetStatus(EXALTED);
	}

	// Modo "Exaltado"
	if ( Status == EXALTED )
	{
		// Obtenemos el promedio de la salud de todos los jugadores.
		int pPlayersHealth = UTIL_GetPlayersHealth();

		// Se encuentran con una buena salud y se han creado más de 35 hijos.
		// cambiar a modo Relajado.
		if ( pPlayersHealth > 60 && ChildsAlive > 35 )
			Relaxed();

		// Se encuentran con baja salud y se han creado más de 15 hijos.
		// cambiar a modo Relajado.
		else if ( pPlayersHealth > 30 && ChildsAlive > 15 )
			Relaxed();

		// Literalmente estan muriendo...
		else
			Relaxed();
	}

	// Modo "Panico"
	if ( Status == PANIC )
	{
		// Notificamos a nuestros hijos la ubicación de los jugadores.
		// Esto con el fin de evitar que algunos de ellos se queden estancados sin hacer nada.
		if ( LastDisclose <= gpGlobals->curtime && DirectorManager() )
		{
			DirectorManager()->Disclose();
			LastDisclose = gpGlobals->curtime + 5;
		}

		// No es un evento infinito.
		if ( !InfinitePanic )
		{
			// Se han creado todos los hijos en modo Horda, pasar a Relajado.
			if ( PanicQueue <= 0 )
				Relaxed();
		}
	}

	// No estamos en Panico ni en Climax.
	if ( Status != PANIC && Status != CLIMAX )
	{
		// Verificamos si es adecuado agregar un hijo a la cola de la próxima horda.
		QueuePanic();

		// No estamos en un Jefe.
		if ( Status != BOSS )
		{
			// Hora de crear una Horda.
			if ( Left4Panic.IsElapsed() )
				Panic();
		}
	}

	HandleChilds();		// Hijos
	HandleBoss();		// Jefes
	HandleMusic();		// Música

	// Volvemos a pensar en 1 segundo.
	SetNextThink(gpGlobals->curtime + 1);
}

//=========================================================
// ¡Han matado a un hijo!
//=========================================================
void CDirector::DeathNotice(CBaseEntity *pVictim)
{
	// Era un hijo de la horda.
	if ( PanicChilds > 0 )
		--PanicChilds;

	// Un hijo más asesinado.
	++ChildsKilled;
}

//=========================================================
// Reinicia y establece el número de segundos antes de entrar
// en modo "Exaltado"
//=========================================================
void CDirector::RestartExalted()
{
	switch ( InGameRules()->GetSkillLevel() )
	{
		case SKILL_EASY:
			Left4Exalted.Start(RandomInt(20, 80));
		break;

		case SKILL_MEDIUM:
			Left4Exalted.Start(RandomInt(20, 50));
		break;

		case SKILL_HARD:
			Left4Exalted.Start(RandomInt(10, 40));
		break;

		case SKILL_REALISTIC:
			Left4Exalted.Start(RandomInt(10, 30));
		break;
	}
}

//=========================================================
// Reinicia y establece el número de segundos antes de entrar
// en modo "Panico"
//=========================================================
void CDirector::RestartPanic()
{
	switch ( InGameRules()->GetSkillLevel() )
	{
		case SKILL_EASY:
			Left4Panic.Start(RandomInt(100, 300));
		break;

		case SKILL_MEDIUM:
			Left4Panic.Start(RandomInt(100, 200));
		break;

		case SKILL_HARD:
			Left4Panic.Start(RandomInt(60, 150));
		break;

		case SKILL_REALISTIC:
			Left4Panic.Start(RandomInt(80, 300));
		break;
	}
}

//=========================================================
// Verifica si un hijo esta muy lejos de los jugadores.
// Usado para eliminar a los NPC's que se encuentren lejos.
//=========================================================
bool CDirector::IsTooFar(CAI_BaseNPC *pNPC)
{
	// Distancia minima.
	float minDistance	= director_max_distance.GetFloat();

	// Examinamos a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= UTIL_InPlayerByIndex(i);

		if ( !pPlayer )
			continue;

		// Obtenemos la distancia del NPC a este jugador.
		float dist	= pNPC->GetAbsOrigin().DistTo(pPlayer->GetAbsOrigin());

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

		// Obtenemos la distancia del NPC a este jugador.
		float distance = pNPC->GetAbsOrigin().DistTo(pPlayer->GetAbsOrigin());

		// Esta cerca del jugador y con el objetivo en el.
		if ( distance < director_min_distance.GetFloat() && pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() )
			return true;
	}

	return false;
}

//=========================================================
// Activado el estado "Relajado"
//=========================================================
void CDirector::Relaxed()
{
	// Ya estamos en este modo.
	if ( Status == RELAXED )
		return;

	// Establecemos el modo.
	SetStatus(RELAXED);
	// Reiniciamos el contador para el Modo Exaltado.
	RestartExalted();

	// Desactivar el panico infinito.
	InfinitePanic = false;
}

//=========================================================
// Activa el estado "Panico"
//=========================================================
void CDirector::Panic(bool super, bool infinite)
{
	// Ya estamos en este modo.
	if ( Status == PANIC )
		return;

	// Desactivamos al Director por 7 segundos.
	// para reproducir el sonido de panico.
	Disabled4Seconds = 7;

	if ( super )
		EmitSound("Director.Horde.Alert");		// ¡Vamos a por ti!
	else
		EmitSound("Director.Horde.Coming");		// Algo grande se acerca...

	// No es un evento infinito.
	if ( !infinite )
	{
		// 20 hijos como minimo para una Horda.
		if ( PanicQueue < 20 )
			PanicQueue = 20;

		if ( director_force_panic_queue.GetInt() != 0 )
			PanicQueue = director_force_panic_queue.GetInt();

		if ( super )
			PanicQueue = PanicQueue + 20;
	}

	// ¿Más que el limite?
	//if ( PanicQueue > GetMaxChilds() )
		//PanicQueue = GetMaxChilds();

	// Reiniciamos el contador para el Modo Panico.
	RestartPanic();

	InfinitePanic	= infinite;
	PanicChilds		= PanicQueue;

	// Revelamos la posición de los jugadores.
	if ( DirectorManager() )
		DirectorManager()->Disclose();

	// Establecemos el estado panico.
	SetStatus(PANIC);
	++PanicCount;
}

//=========================================================
// Activa el estado "Climax"
//=========================================================
void CDirector::Climax(bool mini)
{
	// Ya estamos en este modo.
	if ( Status == CLIMAX )
		return;

	SetStatus(CLIMAX);
	OnClimax.FireOutput(NULL, this);
}

//=========================================================
// Devuelve el comienzo para un LOG del Director.
//=========================================================
const char *CDirector::Ms()
{
	char message[300];

	Q_snprintf(message, sizeof(message), "[Director] [%s]", GetStatus(Status));
	return message;
}

//=========================================================
// Establece el estado del Director
//=========================================================
void CDirector::SetStatus(DirectorStatus status)
{
	DevMsg("%s %s -> %s \r\n", Ms(), GetStatus(Status), GetStatus(status));
	Status = status;
}

//=========================================================
// Devuelve el nombre de un estado del Director
//=========================================================
const char *CDirector::GetStatus(DirectorStatus status)
{
	switch ( status )
	{
		case RELAXED:
			return "RELAJADO";
		break;

		case EXALTED:
			return "EXALTADO";
		break;

		case PANIC:
			return "PANICO";
		break;

		case BOSS:
			return "JEFE";
		break;

		case CLIMAX:
			return "CLIMAX";
		break;
	}

	DevWarning("[Director] Se ha recibido un estado no valido. \r\n");
	return "RELAJADO";
}

//=========================================================
// Devuelve el nombre del estado actual.
//=========================================================
const char *CDirector::GetStatus()
{
	return GetStatus(Status);
}

//=========================================================
// Devuelve la cantidad máxima de hijos vivos según el nivel
// de dificultad.
//=========================================================
float CDirector::GetMaxChilds()
{
	float MaxChilds = director_max_alive.GetFloat();

	switch ( InGameRules()->GetSkillLevel() )
	{
		case SKILL_EASY:
			if ( Status == RELAXED )
				MaxChilds = ceil(MaxChilds / 3);
		break;

		case SKILL_MEDIUM:
			// Aumentamos 5
			MaxChilds = MaxChilds + 5;

			if ( Status == RELAXED )
				MaxChilds = ceil(MaxChilds / 2);
		break;

		case SKILL_HARD:
		case SKILL_REALISTIC:
			// Aumentamos 10
			MaxChilds = MaxChilds + 10;

			if ( Status == RELAXED )
				MaxChilds = MaxChilds - 5;
		break;
	}

	// CLIMAX: Aumentamos 5
	if ( Status == CLIMAX )
		MaxChilds = MaxChilds + 5;

	// Más de 40 NPC's causa lag y problemas de audio...
	if ( MaxChilds > 40 )
		MaxChilds = 40;

	return MaxChilds;
}

//=========================================================
// Devuelve el número de hijos vivos en el mapa.
//=========================================================
int CDirector::CountChilds()
{
	ChildsAlive			= 0;	// Hijos vivos
	ChildsTooClose		= 0;	// Hijos cercanos al jugador (peligrosos)
	CAI_BaseNPC *pNPC	= NULL;

	do
	{
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, CHILD_NAME);

		// El hijo ya no existe (acaba de ser asesinado)
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// Esta muy lejos de los jugadores.
		// No nos sirve de nada, matenlo.
		if ( IsTooFar(pNPC) )
		{
			CTakeDamageInfo damage;
			damage.SetDamage(pNPC->GetHealth() + 5);
			damage.SetDamageType(DMG_GENERIC);
			damage.SetAttacker(this);
			damage.SetInflictor(this);

			pNPC->TakeDamage(damage);
			continue;
		}

		// Esta muy cerca de algún jugador.
		if ( IsTooClose(pNPC) )
			++ChildsTooClose;

		// Hijo vivo.
		++ChildsAlive;

	} while ( pNPC );

	return ChildsAlive;
}

//=========================================================
// Devuelve si es conveniente crear hijos.
//=========================================================
bool CDirector::QueueChilds()
{
	int MaxChilds = GetMaxChilds();

	// Limite de hijos, no crear más.
	if ( ChildsAlive >= MaxChilds )
		return false;

	// Estamos creando hijos, no crear más.
	if ( Spawning )
		return false;

	// CLIMAX: ¡Si!
	if ( Status == CLIMAX )
		return true;

	// EXALTADO: Solo si hay menos de 1/3 de hijos vivos.
	if ( Status == EXALTED && ChildsAlive < ceil(MaxChilds+0.0f / 3) )
		return true;

	// JEFE: Solo si estamos en dificultad Dificil.
	if ( Status == BOSS && InGameRules()->IsSkillLevel(SKILL_HARD) )
		return true;

	int pPlayersHealth = UTIL_GetPlayersHealth();

	// Más probabilidades si los jugadores tienen más del 50% de salud.
	if ( pPlayersHealth > 50 )
	{
		if ( RandomInt(pPlayersHealth, 100) < 60 )
			return false;
	}
	else
	{
		if ( RandomInt(1, 30) > 15 )
			return false;
	}

	return true;
}

//=========================================================
// Devuelve si es conveniente sumar un hijo a la cola
// de la próxima horda.
//=========================================================
bool CDirector::QueuePanic()
{
	// Azar
	if ( RandomInt(1, 30) > 5 )
		return false;

	// La cola ha superado el limite de hijos vivos.
	if ( PanicQueue >= GetMaxChilds() )
		return false;

	// Si los jugadores tienen poca salud, menos probabilidades.
	if ( UTIL_GetPlayersHealth() < 50 )
	{
		if ( RandomInt(1, 10) > 3 )
			return false;
	}

	// Agregamos un hijo a la cola.
	++PanicQueue;
	return true;
}

//=========================================================
// Procesa los hijos.
//=========================================================
void CDirector::HandleChilds()
{
	CountChilds();

	// El director esta obligado a no pasar esta cantidad de hijos vivos.
	float MaxChilds = GetMaxChilds();

	// PANICO y no estamos en un evento infinito.
	if ( Status == PANIC && !InfinitePanic )
	{
		// Dejar en cola exactamente la cantidad de hijos en la cola de la horda.
		if ( PanicQueue > 0 && ChildsAlive < MaxChilds )
			SpawnQueue = PanicQueue;
	}
	else
	{
		// Es recomendable crear hijos.
		if ( QueueChilds() )
		{
			// Creamos solo los que faltan.
			int Left4Spawn	= MaxChilds - ChildsAlive;
			SpawnQueue		= Left4Spawn;
		}
	}

	// No hay hijos por crear.
	if ( SpawnQueue <= 0 || Spawning )
		return;

	// Crear hijos.
	if ( DirectorManager() )
		DirectorManager()->SpawnChilds();
}

//=========================================================
// Devuelve el número de jefes vivos en el mapa.
//=========================================================
int CDirector::CountBoss()
{
	BossAlive			= 0;
	CAI_BaseNPC *pNPC	= NULL;

	do
	{
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, BOSS_NAME);

		// El jefe ya no existe (acaba de ser asesinado)
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// ¡Esta muy lejos de los jugadores!
		if ( IsTooFar(pNPC) && DirectorManager() )
		{
			// Actualizamos los nodos candidatos.
			DirectorManager()->UpdateNodes();

			// Intentamos teletransportarlo a un lugar cercano a los jugadores.
			// ¡Aquí no se salva nadie!
			CAI_Node *pNode = DirectorManager()->GetRandomNode();

			// ¡No se encontro un lugar cercano! Suertudos...
			if ( !pNode )
			{
				// Lo eliminamos.
				UTIL_RemoveImmediate(pNPC);
				continue;
			}

			// Por el poder que me ha conferido Kolesias123... yo te teletransporto!
			pNPC->SetAbsOrigin(pNode->GetPosition(HULL_MEDIUM_TALL));
		}

		// Jefe vivo.
		++BossAlive;

	} while ( pNPC );
	
	return BossAlive;
}

//=========================================================
// Devuelve si es conveniente crear un jefe.
//=========================================================
bool CDirector::QueueBoss()
{
	// Hay más de 4 jefes...
	if ( BossAlive > 4 )
		return false;

	// Si no estamos en Climax no debe haber más de 1 Jefe.
	if ( Status != CLIMAX && BossAlive >= 1 )
		return false;

	// Hay pendiente a creación de un Jefe.
	if ( BossPendient )
		return true;

	// ¡Climax!
	if ( Status == CLIMAX )
		return true;

	int MinHealth			= 80;										// Salud minima del jugador.
	int MinChildsSpawned	= director_kills_boss.GetInt() + 100;		// Cantidad minima de NPC's eliminados para la creación de un jefe.

	switch ( InGameRules()->GetSkillLevel() )
	{
		// Normal
		case SKILL_MEDIUM:
			MinHealth			= 50;
			MinChildsSpawned	= director_kills_boss.GetInt();
		break;

		// Dificil
		case SKILL_HARD:
			MinHealth			= 20;
			MinChildsSpawned	= director_kills_boss.GetInt() / 2;
		break;
	}

	// La salud del jugador debe ser más de [depende dificultad] y
	// antes debemos crear una cantidad determinada de NPC's.
	if ( UTIL_GetPlayersHealth() < MinHealth || ChildsKilled < MinChildsSpawned )
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
// Procesa los jefes.
//=========================================================
void CDirector::HandleBoss()
{
	CountBoss();

	// No estamos en Jefe y hay jefes vivos.
	if ( Status != BOSS && BossAlive > 0 )
		SetStatus(BOSS);

	// Estabamos en Jefe pero los han matado.
	if ( Status == BOSS && BossAlive <= 0 )
	{
		// Fácil: Pasamos a Relajado.
		// o de otra forma pasamos a ¡Panico!
		if ( InGameRules()->IsSkillLevel(SKILL_EASY) )
			Relaxed();
		else
			Panic();
	}

	if ( QueueBoss() && DirectorManager() )
		DirectorManager()->SpawnBoss();
}

//=========================================================
// Procesa la música.
//=========================================================
void CDirector::HandleMusic()
{
	// ¡CLIMAX!
	if ( Status == CLIMAX )
	{
		// Reproducir
		if ( !ClimaxMusic->IsPlaying() )
			ClimaxMusic->Play();

		// Parar música del Jefe.
		if ( BossMusic->IsPlaying() )
			BossMusic->Fadeout();

		// Parar música de la horda.
		if ( HordeMusic->IsPlaying() )
			HordeMusic->Fadeout();

		// Parar música del peligro.
		StopDangerMusic();

		return;
	}
	else if ( ClimaxMusic->IsPlaying() )
		ClimaxMusic->Fadeout();

	if ( Status == BOSS )
	{
		// Reproducir
		if ( !BossMusic->IsPlaying() )
			BossMusic->Play();

		// Parar música de la horda.
		if ( HordeMusic->IsPlaying() )
			HordeMusic->Fadeout();

		// Parar música del peligro.
		StopDangerMusic();

		return;
	}
	else if ( BossMusic->IsPlaying() )
		BossMusic->Fadeout();

	if ( PanicChilds > 0 )
	{
		if ( !HordeMusic->IsPlaying() )
			HordeMusic->Play();

		// Parar música del peligro.
		StopDangerMusic();

		return;
	}
	else if ( HordeMusic->IsPlaying() )
		HordeMusic->Fadeout();

	// RELAJADO o EXALTADO
	if ( Status == RELAXED || Status == EXALTED )
	{
		// Tenemos más de 8 hijos cerca.
		if ( ChildsTooClose >= 8 )
		{
			bool a = false;
			bool b = false;

			// Más de 11
			if ( ChildsTooClose >= 10 )
				a = true;

			// Más de 13
			if ( ChildsTooClose >= 13 )
				b = true;

			// Reproducir música de peligro.
			EmitDangerMusic(a, b);
		}

		// Menos de 8 hijos. Parar la música.
		else if ( ChildsTooClose < 8 && DangerMusic->IsPlaying() )
			StopDangerMusic();
	}
}

//=========================================================
// Reproduce la música de fondo de peligro.
//=========================================================
void CDirector::EmitDangerMusic(bool A, bool B)
{
	// Peligro: Reproducir
	if ( !DangerMusic->IsPlaying() )
		DangerMusic->Play();

	// Peligro A: Reproducir
	if ( A && !DangerMusic_A->IsPlaying() )
		DangerMusic_A->Play();

	// Peligro A: Parar
	if ( !A && DangerMusic_A->IsPlaying() )
		DangerMusic_A->Fadeout();

	// Peligro B: Reproducir
	if ( B && !DangerMusic_B->IsPlaying() )
		DangerMusic_B->Play();

	// Peligro B: Parar
	if ( !B && DangerMusic_B->IsPlaying() )
		DangerMusic_B->Fadeout();
}

//=========================================================
// Para la música de fondo de peligro.
//=========================================================
void CDirector::StopDangerMusic()
{
	DangerMusic_B->Fadeout(1.5f);
	DangerMusic_A->Fadeout(1.5f);
	DangerMusic->Fadeout(1.5f);
}

//=========================================================
//=========================================================
int CDirector::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char message[512];
		int MaxChilds = GetMaxChilds();

		Q_snprintf(message, sizeof(message),
			"Estado: %s",
		GetStatus());
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Hijos creados: %i",
		ChildsSpawned);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Hijos vivos: %i (de un maximo de %i)",
		ChildsAlive, MaxChilds);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Hijos muertos: %i",
		ChildsKilled);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Hijos por crear: %i",
		SpawnQueue);
		EntityText(text_offset++, message, 0);

		Q_snprintf(message, sizeof(message),
			"Hijos cerca del jugador: %i",
		ChildsTooClose);
		EntityText(text_offset++, message, 0);

		if ( Status != CLIMAX )
		{
			Q_snprintf(message, sizeof(message),
				"Cantidad de la Horda: %i (en %f segundos)",
			PanicQueue, Left4Panic.GetRemainingTime());
			EntityText(text_offset++, message, 0);

			Q_snprintf(message, sizeof(message),
				"Hijos de la ultima horda: %i",
			PanicChilds);
			EntityText(text_offset++, message, 0);

			Q_snprintf(message, sizeof(message),
				"Hordas: %i",
			PanicCount);
			EntityText(text_offset++, message, 0);
		}

		if ( Status == RELAXED )
		{
			Q_snprintf(message, sizeof(message),
				"Segundos para salir de [RELAX]: %f",
				Left4Exalted.GetRemainingTime());
			EntityText(text_offset++, message, 0);
		}
	}

	return text_offset;
}

//=========================================================
//=========================================================
//=========================================================

void CC_ForcePanic()
{
	if ( !Director() )
		return;

	Director()->Panic(true);
}

void CC_ForceBoss()
{
	if ( !Director() )
		return;

	Director()->BossPendient = true;
}

static ConCommand director_force_panic("director_force_panic", CC_ForcePanic, "Activa una horda.");
static ConCommand director_force_boss("director_force_boss", CC_ForceBoss, "Crea un jefe.");