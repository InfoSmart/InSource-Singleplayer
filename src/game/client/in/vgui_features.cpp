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

ConVar in_featurespanel("in_featurespanel", "0", FCVAR_CLIENTDLL, "Muestra u oculta el panel de funciones.");

class CFeaturesPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CFeaturesPanel, vgui::Frame);

	CFeaturesPanel(vgui::VPANEL parent);
	~CFeaturesPanel() { };

	protected:
		virtual void OnTick();
		virtual void OnCommand(const char* Command);
		virtual void BuildPanel();

	private:
		CheckButton *cb_AutomaticReload;
};

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
	BuildPanel();
}

class CFeaturesPanelInterface : public IFeaturesPanel
{
	private:
		CFeaturesPanel * FeaturesPanel;
	public:
		CFeaturesPanelInterface()
		{
			FeaturesPanel = NULL;
		}
		void Activate(void)
		{
			if(FeaturesPanel)
			{
				FeaturesPanel->Activate();
			}
		}
		void Create(vgui::VPANEL parent)
		{
			FeaturesPanel = new CFeaturesPanel(parent);
		}
		void Destroy()
		{
			if(FeaturesPanel)
			{
				FeaturesPanel->SetParent((vgui::Panel *)NULL);
				delete FeaturesPanel;
			}
		}
	};

static CFeaturesPanelInterface g_FeaturesPanel;
IFeaturesPanel* featurespanel = (IFeaturesPanel*)&g_FeaturesPanel;

void CFeaturesPanel::OnTick()
{
	BaseClass::OnTick();
	SetVisible(in_featurespanel.GetBool());
}

void CFeaturesPanel::OnCommand(const char* Command)
{
	if(!Q_stricmp(Command, "ToggleFeaturesPanel"))
	{
		in_featurespanel.SetValue(!in_featurespanel.GetBool());
		featurespanel->Activate();
	}

	if(!Q_stricmp(Command, "HealthRegeneration"))
	{
		ConVarRef in_regeneration("in_regeneration");
		in_regeneration.SetValue(HealthRegeneration->IsSelected());
	}

	if(!Q_stricmp(Command, "TiredEffect"))
	{
		ConVarRef in_tired_effect("in_tired_effect");
		ConVarRef in_timescale_effect("in_timescale_effect");

		in_tired_effect.SetValue(TiredEffect->IsSelected());
		in_timescale_effect.SetValue(TiredEffect->IsSelected());
	}

	if(!Q_stricmp(Command, "IronViewBob"))
	{
		ConVarRef in_viewbob_enabled("in_viewbob_enabled");
		in_viewbob_enabled.SetValue(IronViewBob->IsSelected());
	}

	if(!Q_stricmp(Command, "IronSight"))
	{
		ConVarRef in_ironsight_enabled("in_ironsight_enabled");
		in_ironsight_enabled.SetValue(IronSight->IsSelected());
	}

	if(!Q_stricmp(Command, "CrossHair"))
	{
		ConVarRef crosshair("crosshair");
		crosshair.SetValue(CrossHair->IsSelected());
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
	ConVarRef crosshair("crosshair");

	HealthRegeneration->SetSelected(in_regeneration.GetBool());
	TiredEffect->SetSelected(in_tired_effect.GetBool());
	IronViewBob->SetSelected(in_viewbob_enabled.GetBool());
	IronSight->SetSelected(in_ironsight_enabled.GetBool());
	CrossHair->SetSelected(crosshair.GetBool());
}

CON_COMMAND(ToggleFeaturesPanel, "Mostrar u ocultar el panel.")
{
	in_featurespanel.SetValue(!in_featurespanel.GetBool());
	featurespanel->Activate();
};