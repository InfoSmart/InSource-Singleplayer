//========= Public Domain 2009, Julian 'Daedalus' Thatcher. =====================//
//
// Purpose: Header file for HTMLView window
//
//
// $NoKeywords: $
//=============================================================================//
 
// IHTMLView.h
#include <vgui_controls/Frame.h>
#include <vgui_controls/HTML.h>
// JT: OB
#include <vgui_controls/EntHTML.h>
 
using namespace vgui;
 
class IHTMLView
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
	virtual void		Activate( void ) = 0;
	virtual void		UpdateHTML ( void ) = 0;
};
 
//CHTMLView_Panel class: Our HTMLView Frame example class
class CHTMLView_Panel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHTMLView_Panel, vgui::Frame); 
	//CHTMLView_Panel : This Class / vgui::Frame : BaseClass
 
	CHTMLView_Panel(vgui::VPANEL parent); 	// Constructor
	~CHTMLView_Panel(){};					// Destructor
 
protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);
 
private:
	//Other used VGUI control Elements:
	vgui::Button *m_exit;
	vgui::Image  *m_img;
 
public:
	EntHTML   *m_HTML;
};
 
extern IHTMLView* htmlview;
 
#define CenterThisPanelOnScreen()\
    int x,w,h;\
    GetBounds(x,x,w,h);\
    SetPos((ScreenWidth()-w)/2,(ScreenHeight()-h)/2)
 
#define CenterPanelOnScreen(panel)\
    int x,w,h;\
    panel->GetBounds(x,x,w,h);\
    panel->SetPos((panel->ScreenWidth()-w)/2,(panel->ScreenHeight()-h)/2)