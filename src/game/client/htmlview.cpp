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
#include "IHTMLView.h"
 
using namespace vgui;
 
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/TextEntry.h>
 
// JT: OB
#include <vgui_controls/EntHTML.h>
 
#include <cdll_client_int.h>
 
#define ALLOW_JAVASCRIPT	false
#define VERSION				"1.00.2 Orange Box"
 
// JT: OB
static void onTargetUpdate (IConVar *var, const char *pOldValue, float flOldValue);
 
ConVar cl_showhtmlview("cl_showhtmlview", "0", FCVAR_CLIENTDLL, "Sets the state of HTMLView <state>");
ConVar cl_htmltarget("cl_htmltarget", "", FCVAR_CLIENTDLL, 
					 "Sets the HTML address to <state>.", onTargetUpdate);
 
// Constuctor: Initializes the Panel
CHTMLView_Panel::CHTMLView_Panel(vgui::VPANEL parent)
: BaseClass(NULL, "HTMLView")
{
	SetParent( parent );
 
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
 
	SetProportional( true );
	SetTitleBarVisible( true );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( true );
	SetSizeable( true );	// was false
	SetMoveable( true );
	SetVisible( true );
 
	m_HTML = new EntHTML(this, "MyHTMLPage", ALLOW_JAVASCRIPT);
 
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));
 
	LoadControlSettings("resource/UI/HTMLView.res");
 
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
 
	DevMsg("HTMLView has been constructed\n");
 
	CenterThisPanelOnScreen();
 
}
 
//Class: CHTMLView_PanelInterface Class. Used for construction.
class CHTMLView_PanelInterface : public IHTMLView
{
private:
	CHTMLView_Panel *MyPanel;
public:
	CHTMLView_PanelInterface()
	{
		MyPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		MyPanel = new CHTMLView_Panel(parent);
	}
	void Destroy()
	{
		if (MyPanel)
		{
			MyPanel->SetParent( (vgui::Panel *)NULL);
			delete MyPanel;
		}
	}
	void Activate( void )
	{
		if ( MyPanel )
		{
			MyPanel->Activate();
		}
	}
 
	// Update HTML address
	void UpdateHTML ( void )
	{
		char *target;
		char temp[1024];
		if ( MyPanel )
		{
			// Get value of 'cl_htmltarget'
			target = (char *) cl_htmltarget.GetString();
			if(strlen(target) > 0)
			{
				if(target[0] != ':') 
				{
					// Start of address is not :, assume local file
					strcpy(temp, engine->GetGameDirectory());
					strcat(temp, "/");
					strcat(temp, target);
					MyPanel->m_HTML->OpenURL(temp, true);
				} else {
					// : denotes a web address
					// Replace : with 'http://'
					char *n_target = target;
					n_target++; if(!*n_target) { return ; } // Buffer check
					//n_target++;   // - Removed 16th Feb 2009
					strcpy(temp, "http://");
					strcat(temp, n_target);
					MyPanel->m_HTML->OpenURL(temp, true);
				}
			}
		}
	}
};
 
void CHTMLView_Panel::OnTick()
{
	BaseClass::OnTick();
	SetVisible(cl_showhtmlview.GetBool()); //CL_SHOWMYPANEL / 1 BY DEFAULT
}
 
//
// Handle commands from VGUI events.
// By default, only the 'exit' button has a command, and that
// command is 'turnoff'
void CHTMLView_Panel::OnCommand(const char* pcCommand)
{
	if(!Q_stricmp(pcCommand, "turnoff"))
		cl_showhtmlview.SetValue(0);
}
 
static void onTargetUpdate (IConVar *var, const char *pOldValue, float flOldValue)
{
	htmlview->UpdateHTML();
}
 
static CHTMLView_PanelInterface g_MyPanel;
 
// The following are exported to the game system
IHTMLView* htmlview = (IHTMLView*)&g_MyPanel;
 
CON_COMMAND(ToggleHTMLView, "Toggles HTMLView on or off")
{
	cl_showhtmlview.SetValue(!cl_showhtmlview.GetBool());
	htmlview->Activate();	
};
 
CON_COMMAND(UpdateHTMLView, "Updates HTMLView address")
{
	DevMsg("Updating myPanel location");
	htmlview->UpdateHTML();
}
 
// This was used for debugging
// Displays current mod directory
CON_COMMAND(WhereAmI, "Displays current path")
{
	const char *pGameDir = engine->GetGameDirectory();
	DevMsg(pGameDir);
}
 
CON_COMMAND(HTMLViewVersion, "Displays VGUI HTML Screen version")
{
	DevMsg("VGUI HTML Screen v " VERSION " by Daedalus\n");
}