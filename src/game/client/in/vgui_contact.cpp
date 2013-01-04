//========= Public Domain 2009, Julian 'Daedalus' Thatcher. =====================//
//
// Purpose: HTMLView implementation
//
// Ingame Usage Commands:
//  cl_htmltarget path_to_file		Sets the URL to view
//  ToggleHTMLView					Shows or hides the HTML view
// 
// If the given url begins with ':', it is considered a http request.
//  (: is replaced with http://, since the console does not properly allow http://)
// If the given url does not begin with ':', it is considered relative
// to the mod directory.
// If the given url starts with 'entity://', it will trigger an entity action.
// Example: 'entity://relay_open_door->trigger'
//  You may also add a url to go to after the entity action has been fired by
//  placing a ; at the end, and then your url. Eg:
//   'entity://relay_open_door->trigger;html/sample2.html'
//   'entity://relay_close_door->trigger;:www.google.com'
//
//
// $Created: Tuesday, 26 December 2006
// $LastUpdated: Thursday, 5th February 2007
// $Author:  Julian 'Daedalus' Thatcher (daedalus.raistlin@gmail.com)
// $NoKeywords: $
//=============================================================================//
 
//The following include files are necessary to allow your HTMLView.cpp to compile.
#include "cbase.h"
#include "vgui_contact.h"
 
using namespace vgui;
 
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/TextEntry.h>
 
#include <vgui_controls/EntHTML.h> 
#include <cdll_client_int.h>
 
#define ALLOW_JAVASCRIPT	true
 
static void onTargetUpdate (IConVar *var, const char *pOldValue, float flOldValue);
 
ConVar in_contactpanel("in_contactpanel", "0", FCVAR_CLIENTDLL, "Muestra u oculta el panel de contacto");
 
CContactPanel::CContactPanel(vgui::VPANEL parent) : BaseClass(NULL, "CContactPanel")
{
	SetParent(parent);
 
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
 
	SetProportional(true);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
 
	m_HTML = new EntHTML(this, "ContactView", ALLOW_JAVASCRIPT);
 
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme")); 
	LoadControlSettings("resource/UI/Contact.res");
 
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100); 
	CenterThisPanelOnScreen(); 
}

class CContactPanelInterface : public IContactPanel
{
private:
	CContactPanel *ContactPanel;
public:
	CContactPanelInterface()
	{
		ContactPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		ContactPanel = new CContactPanel(parent);
	}
	void Destroy()
	{
		if (ContactPanel)
		{
			ContactPanel->SetParent( (vgui::Panel *)NULL);
			delete ContactPanel;
		}
	}
	void Activate( void )
	{
		if (ContactPanel)
		{
			ContactPanel->Activate();
		}
	}
 
	void UpdateHTML ( void )
	{
		char *target;
		char temp[1024];

		if (ContactPanel)
		{
			target = "resource/html/contact.html";

			if(strlen(target) > 0)
			{
				if(target[0] != ':') 
				{
					strcpy(temp, engine->GetGameDirectory());
					strcat(temp, "/");
					strcat(temp, target);
					ContactPanel->m_HTML->OpenURL(temp, true);
				} 
				else 
				{
					char *n_target = target;
					n_target++; if(!*n_target) { return ; }

					strcpy(temp, "http://");
					strcat(temp, n_target);
					ContactPanel->m_HTML->OpenURL(temp, true);
				}
			}
		}
	}
};

static CContactPanelInterface g_ContactPanel;
IContactPanel* contactpanel = (IContactPanel*)&g_ContactPanel;
 
void CContactPanel::OnTick()
{
	BaseClass::OnTick();
	SetVisible(in_contactpanel.GetBool());
}
 
void CContactPanel::OnCommand(const char* pcCommand)
{
	if(!Q_stricmp(pcCommand, "ToggleContactPanel"))
	{
		in_contactpanel.SetValue(!in_contactpanel.GetBool());
		contactpanel->Activate();
	}
}
 
CON_COMMAND(ToggleContactPanel, "Muestra u oculta el panel de contacto")
{
	in_contactpanel.SetValue(!in_contactpanel.GetBool());
	contactpanel->UpdateHTML();
	contactpanel->Activate();	
};
 
CON_COMMAND(UpdateContactView, "Updates HTMLView address")
{
	DevMsg("Updating contact location");
	contactpanel->UpdateHTML();
}