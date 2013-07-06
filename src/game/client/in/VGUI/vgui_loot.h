#ifndef VGUI_LOOT_H
#define VGUI_LOOT_H

class ILootPanel
{
public:
		virtual void Create(vgui::VPANEL parent) = 0; // The function to create our gui.
		virtual void Destroy() = 0; // The function to destory our gui.
};

extern ILootPanel* LootPanel;

#endif