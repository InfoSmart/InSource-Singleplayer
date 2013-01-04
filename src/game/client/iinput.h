//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined(IINPUT_H)
	#define IINPUT_H
#ifdef _WIN32
	#pragma once
#endif

class bf_write;
class bf_read;
class CUserCmd;
class C_BaseCombatWeapon;
struct kbutton_t;

struct CameraThirdData_t
{
	float	m_flPitch;
	float	m_flYaw;
	float	m_flDist;
	float	m_flLag;
	Vector	m_vecHullMin;
	Vector	m_vecHullMax;
};

abstract_class IInput
{
public:
	// Inicialización/Apagado del sistema.
	virtual	void		Init_All( void ) = 0;
	virtual void		Shutdown_All( void ) = 0;

	virtual int			GetButtonBits( int ) = 0;

	// Crear comandos de movimiento.
	virtual void		CreateMove ( int sequence_number, float input_sample_frametime, bool active ) = 0;
	virtual void		ExtraMouseSample( float frametime, bool active ) = 0;
	virtual bool		WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand ) = 0;
	virtual void		EncodeUserCmdToBuffer( bf_write& buf, int slot ) = 0;
	virtual void		DecodeUserCmdFromBuffer( bf_read& buf, int slot ) = 0;

	virtual CUserCmd	*GetUserCmd( int sequence_number ) = 0;
	virtual void		MakeWeaponSelection( C_BaseCombatWeapon *weapon ) = 0;

	// Estado de la tecla.
	virtual float		KeyState ( kbutton_t *key ) = 0;
	// Liberar evento de la tecla.
	virtual int			KeyEvent( int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding ) = 0;
	// Mirar por una tecla.
	virtual kbutton_t	*FindKey( const char *name ) = 0;

	// Comandos de los controladores.
	virtual void		ControllerCommands( void ) = 0;

	// Extra inicialización de los Joystick.
	virtual void		Joystick_Advanced( void ) = 0;
	virtual void		Joystick_SetSampleTime( float frametime ) = 0;
	virtual void		IN_SetSampleTime( float frametime ) = 0;

	virtual void		AccumulateMouse( void ) = 0;

	// Activar / Desactivar Mouse
	virtual void		ActivateMouse( void ) = 0;
	virtual void		DeactivateMouse( void ) = 0;

	virtual void		ClearStates( void ) = 0;
	virtual float		GetLookSpring( void ) = 0;

	// Posición del mouse.
	virtual void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = 0, int *unclampedy = 0 ) = 0;
	virtual void		SetFullscreenMousePos( int mx, int my ) = 0;
	virtual void		ResetMouse( void ) = 0;
	virtual	float		GetLastForwardMove( void ) = 0;

	// Tercera persona ¡FIX ME!
	virtual void		CAM_Think( void ) = 0;
	virtual int			CAM_IsThirdPerson( void ) = 0;
	virtual void		CAM_GetCameraOffset( Vector& ofs ) = 0;
	virtual void		CAM_ToThirdPerson(void) = 0;
	virtual void		CAM_ToFirstPerson(void) = 0;
	virtual void		CAM_StartMouseMove(void) = 0;
	virtual void		CAM_EndMouseMove(void) = 0;
	virtual void		CAM_StartDistance(void) = 0;
	virtual void		CAM_EndDistance(void) = 0;
	virtual int			CAM_InterceptingMouse( void ) = 0;

	// Información ortografica de la camara.
	virtual void		CAM_ToOrthographic() = 0;
	virtual	bool		CAM_IsOrthographic() const = 0;
	virtual	void		CAM_OrthographicSize( float& w, float& h ) const = 0;

#if defined( HL2_CLIENT_DLL )
	virtual void		AddIKGroundContactInfo( int entindex, float minheight, float maxheight ) = 0;
#endif

	virtual void		LevelInit( void ) = 0;
	virtual void		ClearInputButton( int bits ) = 0;

	virtual	void		CAM_SetCameraThirdData( CameraThirdData_t *pCameraData, const QAngle &vecCameraOffset ) = 0;
	virtual void		CAM_CameraThirdThink( void ) = 0;
};

extern ::IInput *input;

#endif