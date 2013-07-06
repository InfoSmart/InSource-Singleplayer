#include "cbase.h"
#include "baseentity.h"
#include "world.h"

#include "env_instructor_hint.h"

#include "tier0/memdbgon.h"

//=========================================================
// Definici�n de variables de configuraci�n.
//=========================================================

#define LOCATOR_ICON_FX_PULSE_SLOW		0x00000001
#define LOCATOR_ICON_FX_ALPHA_SLOW		0x00000008
#define LOCATOR_ICON_FX_SHAKE_NARROW	0x00000040
#define LOCATOR_ICON_FX_STATIC			0x00000100	// This icon draws at a fixed location on the HUD.

//=========================================================
// Guardado y definici�n de datos
//=========================================================

LINK_ENTITY_TO_CLASS( env_instructor_hint, CEnvInstructorHint );

BEGIN_DATADESC( CEnvInstructorHint )

	DEFINE_KEYFIELD( m_iszReplace_Key,		FIELD_STRING, "hint_replace_key" ),
	DEFINE_KEYFIELD( m_iszHintTargetEntity, FIELD_STRING, "hint_target" ),
	DEFINE_KEYFIELD( m_iTimeout,			FIELD_INTEGER, "hint_timeout" ),
	DEFINE_KEYFIELD( m_iszIcon_Onscreen,	FIELD_STRING, "hint_icon_onscreen" ),
	DEFINE_KEYFIELD( m_iszIcon_Offscreen,	FIELD_STRING, "hint_icon_offscreen" ),
	DEFINE_KEYFIELD( m_iszCaption,			FIELD_STRING, "hint_caption" ),
	DEFINE_KEYFIELD( m_iszActivatorCaption, FIELD_STRING, "hint_activator_caption" ),
	DEFINE_KEYFIELD( m_Color,				FIELD_COLOR32, "hint_color" ),
	DEFINE_KEYFIELD( m_fIconOffset,			FIELD_FLOAT, "hint_icon_offset" ),
	DEFINE_KEYFIELD( m_fRange,				FIELD_FLOAT, "hint_range" ),
	DEFINE_KEYFIELD( m_iPulseOption,		FIELD_CHARACTER, "hint_pulseoption" ),
	DEFINE_KEYFIELD( m_iAlphaOption,		FIELD_CHARACTER, "hint_alphaoption" ),
	DEFINE_KEYFIELD( m_iShakeOption,		FIELD_CHARACTER, "hint_shakeoption" ),
	DEFINE_KEYFIELD( m_bStatic,				FIELD_BOOLEAN, "hint_static" ),
	DEFINE_KEYFIELD( m_bNoOffscreen,		FIELD_BOOLEAN, "hint_nooffscreen" ),
	DEFINE_KEYFIELD( m_bForceCaption,		FIELD_BOOLEAN, "hint_forcecaption" ),
	DEFINE_KEYFIELD( m_iszBinding,			FIELD_STRING, "hint_binding" ),
	DEFINE_KEYFIELD( m_bAllowNoDrawTarget,	FIELD_BOOLEAN, "hint_allow_nodraw_target" ),	
	DEFINE_KEYFIELD( m_bLocalPlayerOnly,	FIELD_BOOLEAN, "hint_local_player_only" ),
	
	DEFINE_INPUTFUNC( FIELD_STRING,	"ShowHint",	InputShowHint ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EndHint",	InputEndHint ),

END_DATADESC()

//=========================================================
//=========================================================
void CEnvInstructorHint::InputShowHint(inputdata_t &inputdata)
{
	//static int s_InstructorServerHintEventCreate = 0;
	IGameEvent *event = gameeventmanager->CreateEvent("instructor_server_hint_create", true);

	if ( event )
	{
		DevMsg("Mostrando HINT \r\n");
		CBaseEntity *pTargetEntity = gEntList.FindEntityByName(NULL, m_iszHintTargetEntity);

		if ( pTargetEntity == NULL && !m_bStatic )
			pTargetEntity = inputdata.pActivator;

		if ( pTargetEntity == NULL )
			pTargetEntity = GetWorldEntity();

		char szColorString[128];
		Q_snprintf(szColorString, sizeof( szColorString ), "%.3d,%.3d,%.3d", m_Color.r, m_Color.g, m_Color.b);

		int iFlags = 0;

		iFlags |= (m_iPulseOption == 0) ? 0 : (LOCATOR_ICON_FX_PULSE_SLOW << (m_iPulseOption - 1));
		iFlags |= (m_iAlphaOption == 0) ? 0 : (LOCATOR_ICON_FX_ALPHA_SLOW << (m_iAlphaOption - 1));
		iFlags |= (m_iShakeOption == 0) ? 0 : (LOCATOR_ICON_FX_SHAKE_NARROW << (m_iShakeOption - 1));
		iFlags |= m_bStatic ? LOCATOR_ICON_FX_STATIC : 0;

		CBasePlayer *pActivator = NULL;
		bool bFilterByActivator = m_bLocalPlayerOnly;

		if ( inputdata.value.StringID() != NULL_STRING )
		{
			CBaseEntity *pTarget	= gEntList.FindEntityByName(NULL, inputdata.value.String());
			pActivator				= dynamic_cast<CBasePlayer*>(pTarget);

			if ( pActivator )
				bFilterByActivator = true;
		}
		else
		{
			if ( GameRules()->IsMultiplayer() == false )
				pActivator = UTIL_GetLocalPlayer();
			else
			{
				Warning("Failed to play server side instructor hint: no player specified for hint\n");
				Assert(0);
			}
		}

		const char *pActivatorCaption = m_iszActivatorCaption.ToCStr();

		if ( !pActivatorCaption || pActivatorCaption[ 0 ] == '\0' )
			pActivatorCaption = m_iszCaption.ToCStr();

		event->SetString("hint_name",			GetEntityName().ToCStr() );
		event->SetString("hint_replace_key",	m_iszReplace_Key.ToCStr() );
		event->SetInt("hint_target",			pTargetEntity->entindex() );
		event->SetInt("hint_activator_userid", ( pActivator ? pActivator->GetUserID() : 0 ) );
		event->SetInt("hint_timeout",			m_iTimeout );
		event->SetString("hint_icon_onscreen",	m_iszIcon_Onscreen.ToCStr() );
		event->SetString("hint_icon_offscreen", m_iszIcon_Offscreen.ToCStr() );
		event->SetString("hint_caption",		m_iszCaption.ToCStr() );
		event->SetString("hint_activator_caption", pActivatorCaption );
		event->SetString("hint_color",			szColorString );
		event->SetFloat("hint_icon_offset",		m_fIconOffset );
		event->SetFloat("hint_range",			m_fRange );
		event->SetInt("hint_flags",				iFlags );
		event->SetString("hint_binding",		m_iszBinding.ToCStr() );
		event->SetBool("hint_allow_nodraw_target", m_bAllowNoDrawTarget );
		event->SetBool("hint_nooffscreen",		m_bNoOffscreen );
		event->SetBool("hint_forcecaption",		m_bForceCaption );
		event->SetBool("hint_local_player_only", bFilterByActivator );

		DevMsg("Fire HINT \r\n");
		gameeventmanager->FireEvent(event);
	}
	else
	{
		DevWarning("No se pudo crear HINT \r\n");
	}

}

//=========================================================
//=========================================================
void CEnvInstructorHint::InputEndHint(inputdata_t &inputdata)
{
	//static int s_InstructorServerHintEventStop = 0;
	IGameEvent *event = gameeventmanager->CreateEvent("instructor_server_hint_stop", true);

	if ( event )
	{
		event->SetString("hint_name", GetEntityName().ToCStr());
		gameeventmanager->FireEvent(event);
	}
	else
	{
		DevWarning("No se pudo esconder HINT \r\n");
	}
}

//=========================================================
//=========================================================
//=========================================================
//=========================================================

class CInfoInstructorHintTarget : public CPointEntity
{
public:
	DECLARE_CLASS(CInfoInstructorHintTarget, CPointEntity);

	virtual int UpdateTransmitState()
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(info_target_instructor_hint, CInfoInstructorHintTarget);
BEGIN_DATADESC(CInfoInstructorHintTarget)

END_DATADESC();