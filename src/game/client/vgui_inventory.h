#ifndef VGUI_INVENTORY_H
#define VGUI_INVENTORY_H

class IInventoryPanel
{
public:
		virtual void Create(vgui::VPANEL parent) = 0; // The function to create our gui.
		virtual void Destroy() = 0; // The function to destory our gui.
};
 
extern IInventoryPanel* InventoryPanel;
#endif