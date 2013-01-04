#include <vgui_controls/CheckButton.h>

class IFeaturesPanel
{
public:
	virtual void	Activate(void) = 0;
	virtual void	Create(vgui::VPANEL parent) = 0;
	virtual void	Destroy(void) = 0;

};

extern IFeaturesPanel* featurespanel;