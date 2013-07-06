//=====================================================================================//
//
// Sistema encargado de constrolar al personaje.
//
// InfoSmart 2013. Todos los derechos reservados.
//=====================================================================================//

#include "cbase.h"
#include "fr_player.h"

#include "in_gamerules.h"

#include "tier0/memdbgon.h"

//=========================================================
// Configuración
//=========================================================



//=========================================================
// Definición de comandos de consola.
//=========================================================

ConVar fr_max_nectar("fr_max_nectar", "1", 0, "Nivel máximo del nectar.");
ConVar fr_emptynectar_seconds("fr_emptynectar_seconds", "20", 0, "Segundos antes de sufrir las consecuencias de no tener nectar.");

ConVar fr_nectar_healthregen("fr_nectar_healthregen",	"1", 0, "Puntos de regeneración de salud por usar el nectar.");
ConVar fr_nectar_healthpenalty("fr_nectar_healthpenalty", "5", 0, "Puntos de de perdida de nectar por usar la regeneración de salud.");

//=========================================================
// Guardado y definición de datos
//=========================================================

#ifdef FURNANCE
	LINK_ENTITY_TO_CLASS( player, CFR_Player );
	PRECACHE_REGISTER(player);
#endif

// Información que se recuperará al restaurar/abrir una partida guardada.
BEGIN_DATADESC ( CFR_Player )
	DEFINE_FIELD( m_iMaxNectar,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iNectar,				FIELD_INTEGER ),
	DEFINE_FIELD( m_iEmptyNectarSeconds,	FIELD_INTEGER ),

	DEFINE_THINKFUNC( NectarThink ),
END_DATADESC()

//=========================================================
// Constructor
//=========================================================
CFR_Player::CFR_Player()
{
	NextPainSound			= gpGlobals->curtime;		// Próximo sonido de dolor.
	BodyHurt				= 0;						// Próxima vez que haremos efecto de dolor.

	m_iMaxNectar			= fr_max_nectar.GetInt();			// Nivel máximo de nectar.
	m_HL2Local.m_iNectar	= m_iNectar = m_iMaxNectar;			// Nivel de nectar.
	m_iEmptyNectarSeconds	= fr_emptynectar_seconds.GetInt();	// Segundos antes de sufrir las consecuencias de no tener nectar.

	m_angEyeAngles.Init();
}

//=========================================================
// Destructor
//=========================================================
CFR_Player::~CFR_Player()
{
}

//=========================================================
//=========================================================
// FUNCIONES PRINCIPALES
//=========================================================
//=========================================================

void CFR_Player::Spawn()
{
	RegisterThinkContext("NectarContext");
	SetContextThink(&CFR_Player::NectarThink, gpGlobals->curtime, "NectarContext");

	BaseClass::Spawn();
}

//=========================================================
// Guardar los objetos necesarios en caché.
//=========================================================
void CFR_Player::Precache()
{
	BaseClass::Precache();
}

//=========================================================
// Pensamiento: Post
//=========================================================
void CFR_Player::PostThink()
{
	BaseClass::PostThink();
}

//=========================================================
// Pensamiento: Nectar
//=========================================================
void CFR_Player::NectarThink()
{
	// Menos nectar para tu cuerpo.
	if ( random->RandomInt(0, 5) <= 2 && m_iNectar > 0 )
		--m_iNectar;

	DevMsg("m_iEmptyNectarSeconds: %i \r\n", m_iEmptyNectarSeconds);
	
	// ¡Te has quedado sin nectar!
	if ( m_iNectar <= 0 )
	{
		// Todavia te quedan unos segundos para recuperar el nectar perdido.
		if ( m_iEmptyNectarSeconds > 0 )
			--m_iEmptyNectarSeconds;

		// Sin nectar y sin segundos. Le quitamos salud cada 5 segs.
		else if ( GetLastDamageTime() < (gpGlobals->curtime - 5) )
		{
			CTakeDamageInfo damage;

			damage.SetAttacker(this);
			damage.SetInflictor(this);
			damage.SetDamageType(DMG_GENERIC);
			damage.SetDamage(random->RandomInt(10, 30)); // Entre 10 a 30 de daño.

			TakeDamage(damage);
		}
	}

	// Tienes nectar, restaurar los segundos para recuperarlo.
	else if ( m_iEmptyNectarSeconds != fr_emptynectar_seconds.GetInt() )
		m_iEmptyNectarSeconds = fr_emptynectar_seconds.GetInt();

	m_HL2Local.m_iNectar = GetNectar();
	SetNextThink(gpGlobals->curtime + 1, "NectarContext");
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS A HL2
//=========================================================
//=========================================================

//=========================================================
// Reproduce una sentencia desde el traje de protección.
//=========================================================
void CFR_Player::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	// No hay traje de protección parlante.
	return;
}

//=========================================================
// Ejecuta un comando de tipo impulse <iImpulse>
//=========================================================
void CFR_Player::CheatImpulseCommands(int iImpulse)
{
	if ( !sv_cheats->GetBool() )
		return;

	if ( iImpulse == 102 )
	{
		if ( GetHealth() < GetMaxHealth() )
			TakeHealth(GetMaxHealth(), DMG_GENERIC);

		if ( GetNectar() < GetMaxNectar() )
			TakeNectar(GetMaxNectar());

		return;
	}

	switch ( iImpulse )
	{
		default:
			BaseClass::CheatImpulseCommands(iImpulse);
	}
}

//=========================================================
//=========================================================
// FUNCIONES RELACIONADAS AL DAÑO/SALUD
//=========================================================
//=========================================================

//=========================================================
// Regeneración de salud.
//=========================================================
void CFR_Player::HealthRegeneration()
{
	if ( m_iNectar <= 0 || m_iHealth >= GetMaxHealth() )
		return;

	SetNectar(m_iNectar - fr_nectar_healthpenalty.GetInt());
	TakeHealth(fr_nectar_healthregen.GetInt(), DMG_GENERIC);
}

//=========================================================
// Añade una cantidad de nectar al jugador.
//=========================================================
int CFR_Player::TakeNectar(float flNectar)
{
	int iMax = fr_max_nectar.GetInt();

	if ( m_iNectar >= iMax )
		return 0;

	const int oldNectar = m_iNectar;
	m_iNectar += flNectar;

	if ( m_iNectar > iMax )
		m_iNectar = iMax;

	SetNectar(m_iNectar);
	return m_iNectar - oldNectar;
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
PlayerGender CFR_Player::Gender()
{
	return PLAYER_MALE;
}

void IN_HealthRegen( const CCommand &args ) 
{ 
	CFR_Player *pPlayer = GetFRPlayer(UTIL_GetLocalPlayer());

	if ( !pPlayer )
		return;

	pPlayer->HealthRegeneration();
}

static ConCommand starthealthregen("+healthregen", IN_HealthRegen);