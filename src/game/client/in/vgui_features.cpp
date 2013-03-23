#include "cbase.h"
#include "vgui_features.h"

using namespace vgui;

#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/CheckButton.h>

vgui::CheckButton* HealthRegeneration;
vgui::CheckButton* TiredEffect;
vgui::CheckButton* IronViewBob;
vgui::CheckButton* IronSight;
vgui::CheckButton* CrossHair;

ConVar in_featurespanel("in_featurespanel", "0", FCVAR_CLIENTDLL, "Muestra u oculta el dialogo de funciones.");

class CFeaturesPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CFeaturesPanel, vgui::Frame);

	CFeaturesPanel(vgui::VPANEL parent);
	~CFeaturesPanel() { };

public:
	virtual void BuildPanel();

	protected:
		virtual void OnTick();
		virtual void OnCommand(const char* Command);

	private:
		CheckButton *cb_AutomaticReload;
};

class CFeaturesPanelInterface : public IFeaturesPanel
{
	private:
		CFeaturesPanel * FeaturesPanel;
	public:
		CFeaturesPanelInterface()
		{
			FeaturesPanel = NULL;
		}
		void Activate()
		{
			if ( FeaturesPanel )
			{
				FeaturesPanel->Activate();
				FeaturesPanel->BuildPanel();
			}
		}
		void Create(vgui::VPANEL parent)
		{
			FeaturesPanel = new CFeaturesPanel(parent);
		}
		void Destroy()
		{
			if ( FeaturesPanel )
			{
				FeaturesPanel->SetParent((vgui::Panel *)NULL);
				delete FeaturesPanel;
			}
		}
	};

static CFeaturesPanelInterface g_FeaturesPanel;
IFeaturesPanel* featurespanel = (IFeaturesPanel*)&g_FeaturesPanel;

CFeaturesPanel::CFeaturesPanel(vgui::VPANEL parent) : BaseClass(NULL, "FeaturesPanel")
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(true);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));
	LoadControlSettings("resource/UI/Features.res");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
}

void CFeaturesPanel::OnTick()
{
	BaseClass::OnTick();

	SetVisible(in_featurespanel.GetBool());
}

void CFeaturesPanel::OnCommand(const char* Command)
{
	// Abrir el dialogo.
	if ( !Q_stricmp(Command, "OpenFeaturesDialog") )
	{
		in_featurespanel.SetValue(true);
		featurespanel->Activate();
	}

	// Cerrar el dialogo.
	if ( !Q_stricmp(Command, "CloseFeaturesDialog") )
	{
		in_featurespanel.SetValue(false);
		featurespanel->Activate();
	}

	// Ajustar el valor de regeneración de salud.
	if ( !Q_stricmp(Command, "HealthRegeneration") )
	{
		ConVarRef in_regeneration("in_regeneration");
		in_regeneration.SetValue(HealthRegeneration->IsSelected());
	}

	// Ajustar el valor de los efectos de cansancio.
	if ( !Q_stricmp(Command, "TiredEffect") )
	{
		ConVarRef in_tired_effect("in_tired_effect");
		ConVarRef in_timescale_effect("in_timescale_effect");

		in_tired_effect.SetValue(TiredEffect->IsSelected());
		in_timescale_effect.SetValue(TiredEffect->IsSelected());
	}

	// Oscilación de camara.
	if ( !Q_stricmp(Command, "IronViewBob") )
	{
		ConVarRef in_viewbob_enabled("in_viewbob_enabled");
		in_viewbob_enabled.SetValue(IronViewBob->IsSelected());
	}

	// Apuntar.
	if ( !Q_stricmp(Command, "IronSight") )
	{
		ConVarRef in_ironsight_enabled("in_ironsight_enabled");
		in_ironsight_enabled.SetValue(IronSight->IsSelected());
	}
}

void CFeaturesPanel::BuildPanel()
{
	HealthRegeneration	= dynamic_cast<CheckButton*>(FindChildByName("HealthRegeneration", true));
	TiredEffect			= dynamic_cast<CheckButton*>(FindChildByName("TiredEffect", true));
	IronViewBob			= dynamic_cast<CheckButton*>(FindChildByName("IronViewBob", true));
	IronSight			= dynamic_cast<CheckButton*>(FindChildByName("IronSight", true));
	CrossHair			= dynamic_cast<CheckButton*>(FindChildByName("CrossHair", true));

	ConVarRef in_regeneration("in_regeneration");
	ConVarRef in_tired_effect("in_tired_effect");
	ConVarRef in_viewbob_enabled("in_viewbob_enabled");
	ConVarRef in_ironsight_enabled("in_ironsight_enabled");

	HealthRegeneration->SetSelected(in_regeneration.GetBool());
	TiredEffect->SetSelected(in_tired_effect.GetBool());
	IronViewBob->SetSelected(in_viewbob_enabled.GetBool());
	IronSight->SetSelected(in_ironsight_enabled.GetBool());
}

CON_COMMAND(OpenFeaturesDialog, "Abre el dialogo de opciones")
{
	in_featurespanel.SetValue(true);
	featurespanel->Activate();
};