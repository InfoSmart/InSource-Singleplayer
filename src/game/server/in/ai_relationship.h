#ifndef AI_RELATIONSHIP_H

#define AI_RELATIONSHIP_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	NOT_REVERTING,
	REVERTING_TO_PREV,
	REVERTING_TO_DEFAULT,
};

#define SF_RELATIONSHIP_NOTIFY_SUBJECT	(1<<0)	// Alert the subject of the change and give them a memory of the target entity
#define SF_RELATIONSHIP_NOTIFY_TARGET	(1<<1)	// Alert the target of the change and give them a memory of the subject entity

//=========================================================
//=========================================================
class CAI_Relationship : public CBaseEntity, public IEntityListener
{
	DECLARE_CLASS( CAI_Relationship, CBaseEntity );

public:
	CAI_Relationship() : m_iPreviousDisposition( -1 )  { }

	void Spawn();
	void Activate();

	void SetActive( bool bActive );
	void ChangeRelationships( int disposition, int iReverting, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	void ApplyRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void RevertRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void RevertToDefaultRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	void UpdateOnRemove();
	void OnRestore();

	bool IsASubject( CBaseEntity *pEntity );
	bool IsATarget( CBaseEntity *pEntity );

	void OnEntitySpawned( CBaseEntity *pEntity );
	void OnEntityDeleted( CBaseEntity *pEntity );

	string_t	m_iszSubject;
	int			m_iDisposition;

private:

	void	ApplyRelationshipThink( void );
	CBaseEntity *FindEntityForProceduralName( string_t iszName, CBaseEntity *pActivator, CBaseEntity *pCaller );
	void	DiscloseNPCLocation( CBaseCombatCharacter *pSubject, CBaseCombatCharacter *pTarget );

	
	int			m_iRank;
	bool		m_fStartActive;
	bool		m_bIsActive;
	int			m_iPreviousDisposition;
	float		m_flRadius;
	int			m_iPreviousRank;
	bool		m_bReciprocal;

public:
	// Input functions
	void InputApplyRelationship( inputdata_t &inputdata );
	void InputRevertRelationship( inputdata_t &inputdata );
	void InputRevertToDefaultRelationship( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

#endif