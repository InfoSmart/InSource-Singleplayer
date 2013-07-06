#ifndef ENV_INSTRUCTOR_HINT_H

#define ENV_INSTRUCTOR_HINT_H

#ifdef _WIN32
#pragma once
#endif

class CEnvInstructorHint : public CPointEntity
{
public:
	DECLARE_CLASS(CEnvInstructorHint, CPointEntity);
	DECLARE_DATADESC();

private:
	void InputShowHint(inputdata_t &inputdata);
	void InputEndHint(inputdata_t &inputdata);

	string_t	m_iszReplace_Key;
	string_t	m_iszHintTargetEntity;
	int			m_iTimeout;
	string_t	m_iszIcon_Onscreen;
	string_t	m_iszIcon_Offscreen;
	string_t	m_iszCaption;
	string_t	m_iszActivatorCaption;
	color32		m_Color;
	float		m_fIconOffset;
	float		m_fRange;
	uint8		m_iPulseOption;
	uint8		m_iAlphaOption;
	uint8		m_iShakeOption;
	bool		m_bStatic;
	bool		m_bNoOffscreen;
	bool		m_bForceCaption;
	string_t	m_iszBinding;
	bool		m_bAllowNoDrawTarget;
	bool		m_bLocalPlayerOnly;
};

#endif // ENV_INSTRUCTOR_HINT_H