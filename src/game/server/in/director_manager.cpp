//=====================================================================================//
//
// Inteligencia encargada de la creación de NPC's y hordas.
//
// Inspiración: I.A. Director de Left 4 Dead 2
//
//=====================================================================================//

#include "cbase.h"
#include "director_manager.h"

#include "director.h"

#include "in_player.h"
#include "in_gamerules.h"
#include "in_utils.h"

#include "BasePropDoor.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definición de comandos de consola.
//=========================================================

ConVar director_update_nodes("director_update_nodes", "1.0f", 0, "Cantidad de segundos para la actualización de nodos");

//=========================================================
// Configuración
//=========================================================

#define MAX_TRIES 3

#define SPAWN_OUT_DISTANCE 600

//=========================================================
// Lista de posibles NPC's a crear.
//=========================================================
Director_Child pChilds[] =
{
	Director_Child("npc_zombie",		90),	// Zombi normal: 80%
	//Director_Child("npc_fastzombie",	50),	// Zombi rápido: 50%
	Director_Child("npc_zombine",		10),	// Zombine:	30%
	Director_Child("npc_poisonzombie",	10),		// Zombi venenoso: 10%
	Director_Child("npc_headcrab",		5),
	//Director_Child("npc_burned",		5),
};

Director_Child pBoss[] =
{
	Director_Child("npc_grunt", 100)
};

CDirector_Manager g_Manager;
CDirector_Manager *DirectorManager() { return &g_Manager; }

//=========================================================
// Constructor
//=========================================================
CDirector_Manager::CDirector_Manager()
{
}

//=========================================================
// Destructor
//=========================================================
CDirector_Manager::~CDirector_Manager()
{
}

//=========================================================
// Inicialización
//=========================================================
void CDirector_Manager::Init()
{
	for ( int i = 0; i < NELEMS(pChilds); ++i )
		pChilds[i].psChildName = AllocPooledString(pChilds[i].pChildName);

	SpawnNodes.Purge();
	CandidateUpdateTimer.Invalidate();
}

//=========================================================
// Revela la posición de los jugadores.
//=========================================================
void CDirector_Manager::Disclose()
{
	CAI_BaseNPC *pNPC = NULL;

	do
	{
		// Buscamos a todos los hijos en el mapa.
		pNPC = (CAI_BaseNPC *)gEntList.FindEntityByName(pNPC, CHILD_NAME);

		// No existe o esta muerto.
		if ( !pNPC || !pNPC->IsAlive() )
			continue;

		// Ya tiene a un jugador como enemigo.
		if ( pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() )
			continue;

		// Seleccionamos al jugador más cercano.
		float flDistance = 0.0f;
		CIN_Player *pPlayer = UTIL_GetNearestInPlayer(pPlayer->GetAbsOrigin(), flDistance);

		if ( !pPlayer )
			continue;

		// Le decimos que su nuevo enemigo es el jugador y le damos la ubicación de este.
		pNPC->SetEnemy(pPlayer);
		pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());

	}
	while ( pNPC );
}

//=========================================================
// Devuelve la red de nodos.
//=========================================================
CAI_Network *CDirector_Manager::GetNetwork()
{
	return g_pBigAINet;
}

//=========================================================
// Actualiza los mejores nodos para la creación de hijos.
//=========================================================
void CDirector_Manager::UpdateNodes()
{
	// Aún no toca actualizar.
	if ( CandidateUpdateTimer.HasStarted() && !CandidateUpdateTimer.IsElapsed() )
		return;

	// Empezar el cronometro.
	CandidateUpdateTimer.Start(director_update_nodes.GetFloat());

	// ¡Este mapa no tiene nodos!
	if ( !GetNetwork() || !GetNetwork()->NumNodes() )
	{
		ClientPrint(UTIL_InPlayerByIndex(1), HUD_PRINTCENTER, "#NoNODES");
		Warning("[NODOS] Este mapa no tiene nodos de movimiento.\r\n");
		return;
	}

	int iNumNodes = GetNetwork()->NumNodes();
	SpawnNodes.Purge(); // Limpiamos la lista de nodos.

	// Revisamos cada nodo.
	for ( int i = 0; i < iNumNodes; ++i )
	{
		CAI_Node *pNode = GetNetwork()->GetNode(i);

		// El nodo ya no existe o no es de suelo.
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		// Buscar al jugador más cercano.
		float flDistance	= 0;
		Vector vecPos		= pNode->GetPosition(HULL_HUMAN);
		CIN_Player *pPlayer = UTIL_GetNearestInPlayer(vecPos, flDistance);

		// ¡Ninguno!
		if ( !pPlayer )
			return;

		ConVarRef director_debug("director_debug");
		ConVarRef director_min_distance("director_min_distance");
		ConVarRef director_max_distance("director_max_distance");
		ConVarRef director_spawn_outview("director_spawn_outview");

		CBaseEntity *pChild = gEntList.FindEntityByNameNearest(CHILD_NAME, vecPos, 20);
		CBaseEntity *pSpawn = gEntList.FindEntityByClassname(NULL, "info_player_start");

		if ( pSpawn )
		{
			float spawnDistance = vecPos.DistTo(pSpawn->GetAbsOrigin());

			// Este nodo esta muy cerca del Spawn.
			if ( spawnDistance < SPAWN_OUT_DISTANCE )
				continue;
		}

		// Hay un hijo aquí.
		if ( pChild )
			continue;

		// Este nodo esta muy lejos o muy cerca.
		if ( flDistance > director_max_distance.GetFloat() || flDistance < director_min_distance.GetFloat() )
			continue;

		// No usar nodos que esten a la vista de los jugadores.
		if ( director_spawn_outview.GetBool() )
		{
			if ( UTIL_IsPlayersVisible(vecPos) )
				continue;
		}

		// Marcamos al nodo afortunado.
		if ( director_debug.GetBool() )
			NDebugOverlay::Box(vecPos, -Vector(5, 5, 5), Vector(5, 5, 5), 32, 32, 128, 10, 6.0f);

		// Lo agregamos a la lista.
		SpawnNodes.AddToTail(i);
	}
}

//=========================================================
// Selecciona un nodo candidato al azar.
//=========================================================
CAI_Node *CDirector_Manager::GetRandomNode()
{
	// No hay nodos candidatos.
	if ( SpawnNodes.Count() == 0 )
		return NULL;

	int iNode			= RandomInt(0, SpawnNodes.Count() - 1);
	CAI_Node *pNode		= GetNetwork()->GetNode(SpawnNodes[iNode]);

	return pNode;
}

//=========================================================
// Devuelve si es posible crear un hijo en esta ubicación.
//=========================================================
bool CDirector_Manager::CanMake(const Vector &vecPosition)
{
	ConVarRef director_spawn_outview("director_spawn_outview");

	// No usar la ubicación si esta a la vista de los jugadores.
	if ( director_spawn_outview.GetBool() )
	{
		if ( UTIL_IsPlayersVisibleCone(vecPosition) )
		{
			DevMsg("[MANAGER] El lugar de creacion estaba en el campo de vision. \r\n");
			return false;
		}
	}

	return true;
}

//=========================================================
// Verificaciones después de crear al NPC.
//=========================================================
bool CDirector_Manager::PostSpawn(CAI_BaseNPC *pNPC)
{
	bool bStuck = true;

	while ( bStuck )
	{
		trace_t tr;
		UTIL_TraceHull(pNPC->GetAbsOrigin(), pNPC->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_NPCSOLID, pNPC, COLLISION_GROUP_NONE, &tr);

		if ( tr.fraction != 1.0 && tr.m_pEnt )
		{
			// Nos hemos atorado en un objeto con fisicas.
			if ( FClassnameIs(tr.m_pEnt, "prop_physics") )
			{
				// Lo ajustamos como "No solido" para que el bucle lo ignore.
				tr.m_pEnt->AddSolidFlags(FSOLID_NOT_SOLID);
				// Removemos el objeto.
				UTIL_RemoveImmediate(tr.m_pEnt);
				continue;
			}

			// Nos hemos atorado con una pared o algo del mundo.
			if ( tr.m_pEnt->IsWorld() )
			{
				// No... no podemos eliminar una pared...
				// Removemos el NPC para evitar eventos sobrenaturales.
				UTIL_RemoveImmediate(pNPC);
				return false;
			}

			// No es un objeto con físicas ni del mundo.
			// Intentamos encontrar una nueva ubicación para el hijo.

			//DevMsg("[MANAGER] <%s> STUCK!!!! \r\n", tr.m_pEnt->GetClassname());
			CAI_Node *pNode	= GetRandomNode();

			// Nada
			if ( !pNode )
				UTIL_RemoveImmediate(pNPC);

			pNPC->SetAbsOrigin(pNode->GetPosition(HULL_HUMAN));
		}

		bStuck = false;
	}

	return true;
}

//=========================================================
// Devuelve una clase de hijo para la creación.
//=========================================================
const char *CDirector_Manager::GetChildClass()
{
	const char *pClass	= "npc_zombie";
	int pRandom			= RandomInt(1, 100);

	for ( int i = 0; i < NELEMS(pChilds); ++i )
	{
		if ( pChilds[i].pChildPorcent > pRandom )
			pClass = pChilds[i].pChildName;
	}

	return pClass;
}

//=========================================================
// Devuelve una clase de jefe para la creación.
//=========================================================
const char *CDirector_Manager::GetBossClass()
{
	const char *pClass	= "npc_grunt";
	int pRandom			= RandomInt(1, 100);

	for ( int i = 0; i < NELEMS(pBoss); ++i )
	{
		if ( pBoss[i].pChildPorcent > pRandom )
			pClass = pBoss[i].pChildName;
	}

	return pClass;
}

//=========================================================
// Crea los hijos.
//=========================================================
void CDirector_Manager::SpawnChilds()
{
	// No se ha podido acceder al Director.
	if ( !Director() )
		return;

	// Estamos creando hijos.
	Director()->Spawning		= true;

	DirectorStatus Status	= Director()->Status;
	int Queue				= Director()->SpawnQueue;

	// Estamos en modo Climax o Panico.
	// Crear bonches de hijos.
	if ( Status == CLIMAX || Status == PANIC )
	{
		int HalfQueue = (Queue / 3);

		// Creamos 3 "bonches" de hijos.
		AddHorde(HalfQueue);
		AddHorde(HalfQueue);
		AddHorde(HalfQueue);

		// Vaciar la cola de creación.
		Director()->SpawnQueue	= 0;
		Director()->PanicQueue	= 0;

		Director()->Spawning		= false;
		return;
	}

	// Crear cada hijo en una ubicación diferente.
	for ( int i = 0; i < Queue; ++i )
		AddChild();

	Director()->Spawning	 = false;
}

//=========================================================
// Crea un jefe.
//=========================================================
void CDirector_Manager::SpawnBoss()
{
	// Actualizamos los nodos candidatos para la creación.
	UpdateNodes();

	// ¡No hay nodos!
	if ( SpawnNodes.Count() <= 0 )
		return;

	for ( int i = 0; i < MAX_TRIES; ++i )
	{
		// Seleccionamos un nodo candidato al azar.
		CAI_Node *pNode	= GetRandomNode();

		// El nodo no existe.
		if ( !pNode )
			continue;

		// Obtenemos la ubicación del nodo.
		Vector vecOrigin	= pNode->GetPosition(HULL_MEDIUM_TALL);
		bool bSpawn			= AddBoss(vecOrigin);

		// El hijo no se ha podido crear.
		if ( !bSpawn )
			continue;
		
		return;
	}

	// No se pudo crear al Jefe, intentarlo en otro momento.
	Director()->BossPendient = true;
}

//=========================================================
// Crea un hijo en un nodo candidato.
//=========================================================
void CDirector_Manager::AddChild()
{
	// Actualizamos los nodos candidatos para la creación.
	UpdateNodes();

	// ¡No hay nodos!
	if ( SpawnNodes.Count() <= 0 )
		return;

	// Intentamos crear al hijo.
	for ( int i = 0; i < MAX_TRIES; ++i )
	{
		// Seleccionamos un nodo candidato al azar.
		CAI_Node *pNode	= GetRandomNode();

		// El nodo no existe.
		if ( !pNode )
			continue;

		// Obtenemos la ubicación del nodo e intentamos crear al hijo.
		Vector vecOrigin	= pNode->GetPosition(HULL_HUMAN);
		bool bSpawn			= AddChild(vecOrigin);

		// El hijo no se ha podido crear.
		if ( !bSpawn )
			continue;

		// Un hijo menos en la cola.
		Director()->SpawnQueue = Director()->SpawnQueue - 1;
		break;
	}
}

//=========================================================
// Crea un hijo en la ubicación especificada.
//=========================================================
bool CDirector_Manager::AddChild(const Vector &vecPosition, int flag)
{
	// No se ha podido acceder al Director.
	if ( !Director() )
		return false;

	// No es posible crear un hijo aquí.
	if ( !CanMake(vecPosition) )
		return false;

	// Creamos un hijo de la lista.
	const char *pChildName	= GetChildClass();
	CAI_BaseNPC *pChild		= (CAI_BaseNPC *)CreateEntityByName(pChildName);

	QAngle angles = RandomAngle(0, 360);
	angles.x = 0.0;
	angles.z = 0.0;	
	pChild->SetAbsAngles(angles);

	// Establecemos la ubicación de creación.
	pChild->SetAbsOrigin(vecPosition);

	// Debe caer al suelo y desaparecer.
	pChild->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	pChild->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	UTIL_DropToFloor(pChild, MASK_SOLID);

	ConVarRef director_debug("director_debug");

	DispatchSpawn(pChild);
	pChild->SetOwnerEntity(Director());
	DispatchActivate(pChild);

	// ¡¡NO CAMBIAR!!
	pChild->SetName(MAKE_STRING(CHILD_NAME));

	// Sin colisiones.
	if ( flag == SPAWN_NO_COLLISION || flag == SPAWN_NO_COLLISION_AND_POWERFUL )
	{
		pChild->SetCollisionGroup(COLLISION_GROUP_SPECIAL_NPC);
		Vector vecOriginRadius;

		if ( Director()->Status == PANIC && RandomInt(0, 4) == 2 )
			pChild->SetCollisionGroup(COLLISION_GROUP_NPC);

		// Intentamos crearlo en un radio de 100
		if ( CAI_BaseNPC::FindSpotForNPCInRadius(&vecOriginRadius, vecPosition, pChild, 150, false) )
		{
			// Evitamos que se mueva por debajo del suelo.
			vecOriginRadius.z = vecPosition.z;

			// Movemos hacia esta ubicación.
			pChild->SetAbsOrigin(vecOriginRadius);

			// Marcamos al nodo afortunado. (Naranja)
			if ( director_debug.GetBool() )
				NDebugOverlay::Box(vecOriginRadius, -Vector(10, 10, 10), Vector(10, 10, 10), 255, 128, 0, 10, 20.0f);
		}
	}

	// Poderoso.
	if ( flag == SPAWN_POWERFUL || flag == SPAWN_NO_COLLISION_AND_POWERFUL )
	{
		int moreHealth = 3;

		// Normal: 5 más de salud.
		if ( InGameRules()->IsSkillLevel(SKILL_MEDIUM) ) 
			moreHealth = 5;

		// Dificil: 8 más de salud.
		if ( InGameRules()->IsSkillLevel(SKILL_HARD) )
			moreHealth = 8;

		// Más rápido.
		pChild->SetAddAccel(40);

		// Establecemos la nueva salud.
		pChild->SetMaxHealth(pChild->GetMaxHealth() + moreHealth);
		pChild->SetHealth(pChild->GetMaxHealth());

		// Seleccionamos al jugador más cercano.
		float flDistance = 0.0f;
		CIN_Player *pPlayer = UTIL_GetNearestInPlayer(pChild->GetAbsOrigin(), flDistance);

		if ( pPlayer )
		{
			// Le decimos que su nuevo enemigo es el jugador y le damos la ubicación de este.
			pChild->SetEnemy(pPlayer);
			pChild->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());
		}
	}

	// Al parecer se atoro en una pared.
	if ( !PostSpawn(pChild) )
	{
		// Marcamos al nodo desafortunado. (Negro)
		if ( director_debug.GetBool() )
			NDebugOverlay::Box(vecPosition, -Vector(10, 10, 10), Vector(10, 10, 10), 0, 0, 0, 10, 3.0f);

		return false;
	}

	// Marcamos al nodo afortunado. (Rojo)
	if ( director_debug.GetBool() )
		NDebugOverlay::Box(vecPosition, -Vector(10, 10, 10), Vector(10, 10, 10), 223, 1, 1, 10, 3.0f);

	DevMsg("[MANAGER] Se ha creado <%s> (faltan %i) \r\n", pChildName, Director()->SpawnQueue);
	++Director()->ChildsSpawned;

	return true;
}

//=========================================================
// Crea un bonche de hijos en un nodo candidato.
//=========================================================
void CDirector_Manager::AddHorde(int pMount)
{
	// Actualizamos los nodos candidatos para la creación.
	UpdateNodes();

	// ¡No hay nodos!
	if ( SpawnNodes.Count() <= 0 )
		return;

	for ( int i = 0; i < MAX_TRIES; ++i )
	{
		// Seleccionamos un nodo candidato al azar.
		CAI_Node *pNode	= GetRandomNode();

		// El nodo no existe.
		if ( !pNode )
			continue;

		// Obtenemos la ubicación del nodo.
		Vector vecOrigin = pNode->GetPosition(HULL_HUMAN);

		ConVarRef director_debug("director_debug");

		if ( director_debug.GetBool() )
			NDebugOverlay::Cross3D(vecOrigin, 50.0f, 255, 128, 0, true, 20.0f);

		// Creamos varios hijos en este nodo.
		// Con las colisiones especiales (No colisionan con los otros hijos)
		for ( int h = 0; h < pMount; ++h )
		{
			for ( int s = 0; s < MAX_TRIES; ++s )
			{
				bool pSpawn = AddChild(vecOrigin, SPAWN_NO_COLLISION_AND_POWERFUL);

				if ( pSpawn )
					break;
			}
		}

		break;
	}
}

bool CDirector_Manager::AddBoss(const Vector &vecPosition)
{
	// No se ha podido acceder al Director.
	if ( !Director() )
		return false;

	// No es posible crear un jefe aquí.
	if ( !CanMake(vecPosition) )
		return false;

	// Creamos un jefe de la lista.
	const char *pBossName	= GetBossClass();
	CAI_BaseNPC *pBoss		= (CAI_BaseNPC *)CreateEntityByName(pBossName);

	QAngle angles = RandomAngle(0, 360);
	angles.x = 0.0;
	angles.z = 0.0;	
	pBoss->SetAbsAngles(angles);

	// Establecemos la ubicación de creación.
	pBoss->SetAbsOrigin(vecPosition);

	// Debe caer al suelo y desaparecer.
	pBoss->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	UTIL_DropToFloor(pBoss, MASK_SOLID);

	ConVarRef director_debug("director_debug");

	// Marcamos al nodo afortunado.
	if ( director_debug.GetBool() )
		NDebugOverlay::Box(vecPosition, -Vector(15, 15, 15), Vector(15, 15, 15), 223, 1, 1, 10, 3.0f);

	DispatchSpawn(pBoss);
	pBoss->SetOwnerEntity(Director());
	DispatchActivate(pBoss);

	// ¡¡NO CAMBIAR!!
	pBoss->SetName(MAKE_STRING(BOSS_NAME));

	// Al parecer se atoro en una pared.
	if ( !PostSpawn(pBoss) )
		return false;

	DevMsg("[MANAGER] Se ha creado un JEFE. \r\n");

	++Director()->BossSpawned;
	Director()->BossPendient = false;
	Director()->ChildsKilled = 0;

	return true;
}