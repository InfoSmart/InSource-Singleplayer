#include "cbase.h"
#include "vgui_inventory.h"

using namespace vgui;

#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Button.h>
#include <vgui/ILocalize.h>

#include "c_baseplayer.h"
#include "c_in_player.h"

//=========================================================
// CInventoryPanel
//=========================================================
class CInventoryPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CInventoryPanel, vgui::Frame); 
 
	CInventoryPanel(vgui::VPANEL parent);
	~CInventoryPanel() { };
 
protected:
	virtual void Reset();
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);
 
private:
	SectionedListPanel *BackpackItems;
	SectionedListPanel *PocketItems;

	ProgressBar *BackpackProgress;
	ProgressBar *InventoryProgress;

	Button *Drop;
	Button *UseItem;
	Button *Move;

	bool Updated;
};

//=========================================================
// CInventoryPanelInterface
//=========================================================
class CInventoryPanelInterface : public IInventoryPanel
{
private:
	CInventoryPanel *InventoryPanel;

public:
	CInventoryPanelInterface()
	{
		InventoryPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		InventoryPanel = new CInventoryPanel(parent);
	}
	void Destroy()
	{
		if ( InventoryPanel )
		{
			InventoryPanel->SetParent((vgui::Panel *)NULL);
			delete InventoryPanel;
		}
	}
};

static CInventoryPanelInterface g_InventoryPanel;
IInventoryPanel* InventoryPanel = (IInventoryPanel*)&g_InventoryPanel;

//=========================================================
// Comandos
//=========================================================

ConVar cl_inventory("cl_inventory",					"0",	FCVAR_CLIENTDLL, "Muestra/Oculta el inventario.");
ConVar cl_update_inventory("cl_update_inventory",	"0",	FCVAR_CLIENTDLL, "Actualiza el inventario.");

//=========================================================
// Constructor
//=========================================================
CInventoryPanel::CInventoryPanel(vgui::VPANEL parent) : BaseClass(NULL, "InventoryPanel")
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

	BackpackItems	= new SectionedListPanel(this, "BackPackItems");
	PocketItems		= new SectionedListPanel(this, "PocketItems");

	BackpackProgress	= new ProgressBar(this, "BackpackProgress");
	InventoryProgress	= new ProgressBar(this, "InventoryProgress");

	Drop			= new Button(this, "Drop", "Drop");
	UseItem			= new Button(this, "UseItem", "UseItem");
	Move			= new Button(this, "Move", "Move");

	Drop->SetCommand("dropitem");
	UseItem->SetCommand("useitem");
	Move->SetCommand("moveitem");

	Reset();	
	LoadControlSettings("resource/UI/InventoryPanel.res");
 
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
}

//=========================================================
// Reinicio
//=========================================================
void CInventoryPanel::Reset()
{
	BackpackItems->RemoveAll();
	BackpackItems->RemoveAllSections();
	BackpackItems->AddSection(0, "");
	BackpackItems->AddColumnToSection(0, "name", "#Inventory_Backpack", 0, 300);
	BackpackItems->AddColumnToSection(0, "count", "", 0, 100);

	PocketItems->RemoveAll();
	PocketItems->RemoveAllSections();
	PocketItems->AddSection(0, "");
	PocketItems->AddColumnToSection(0, "name", "#Inventory_Pocket", 0, 300);
	PocketItems->AddColumnToSection(0, "count", "", 0, 100);

	BackpackProgress->SetProgress(0.0);
	InventoryProgress->SetProgress(0.0);
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CInventoryPanel::OnTick()
{
	BaseClass::OnTick();
	C_IN_Player *pPlayer = (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
	{
		SetVisible(false);
		return;
	}

	if ( cl_update_inventory.GetBool() )
	{
		// Obtenemos los limites del inventario.
		ConVarRef sv_max_inventory_backpack("sv_max_inventory_backpack");
		ConVarRef sv_max_inventory_pocket("sv_max_inventory_pocket");

		// Lo devolvemos a 0
		cl_update_inventory.SetValue(0);

		// Removemos todos los objetos.
		BackpackItems->RemoveAll();
		PocketItems->RemoveAll();

		int pEntity;
		const char *pItemName;
		const char *pItemClassName;

		float pBackpackCountItems	= 0;
		float pPocketCountItems		= 0;

		int pBackpackItems[100];
		int pPocketItems[100];

		// Iniciamos los array
		for ( int i = 0; i < ARRAYSIZE(pBackpackItems); i++ )
			pBackpackItems[i]	= 0;
		for ( int i = 0; i < ARRAYSIZE(pPocketItems); i++ )
			pPocketItems[i]		= 0;
		
		// Mochila
		// Primero obtenemos todos los objetos y los agrupamos en array.
		for ( int i = 1; i < ARRAYSIZE(pBackpackItems); i++ )
		{
			pEntity	= pPlayer->Inventory_GetItem(i, INVENTORY_BACKPACK);

			// No hay nada en este Slot
			if ( pEntity == 0 )
				continue;

			// Aquí vamos contando los objetos de este mismo tipo.
			// !!!NOTE: Todo esto fue desarrollado por alguien que no sabe mucho C++... Odio los array de C++...
			pBackpackItems[pEntity] = (pBackpackItems[pEntity] + 1);
			pBackpackCountItems++;			
		}
		// Ahora obtenemos la lista de objetos con su respectiva cantidad y la mostramos.
		for ( int i = 1; i < ARRAYSIZE(pBackpackItems); i++ )
		{
			// !!!NOTE: i ahora es la ID del objeto
			if ( pBackpackItems[i] == 0 || !pBackpackItems[i] )
				continue;

			pItemName		= pPlayer->Inventory_GetItemNameByID(i);
			pItemClassName	= pPlayer->Inventory_GetItemClassNameByID(i);

			// No hay nada en este Slot
			if ( pItemName == "" || pItemClassName == "" )
				continue;

			// Agregamos el objeto.
			KeyValues *itemData = new KeyValues("data");
			itemData->SetString("name",			pItemName);
			itemData->SetString("classname",	pItemClassName);
			itemData->SetInt("count",			pBackpackItems[i]);
			BackpackItems->AddItem(0, itemData);
		}

		// Bolsillo
		// Primero obtenemos todos los objetos y los agrupamos en array.
		for ( int i = 1; i < ARRAYSIZE(pPocketItems); i++ )
		{
			pEntity	= pPlayer->Inventory_GetItem(i, INVENTORY_POCKET);

			// No hay nada en este Slot
			if ( pEntity == 0 )
				continue;

			// Aquí vamos contando los objetos de este mismo tipo.
			// !!!NOTE: Todo esto fue desarrollado por alguien que no sabe mucho C++... Odio los array de C++...
			pPocketItems[pEntity] = (pPocketItems[pEntity] + 1);
			pPocketCountItems++;			
		}
		// Ahora obtenemos la lista de objetos con su respectiva cantidad y la mostramos.
		for ( int i = 1; i < ARRAYSIZE(pPocketItems); i++ )
		{
			// !!!NOTE: i ahora es la ID del objeto
			if ( pPocketItems[i] == 0 || !pPocketItems[i] )
				continue;

			pItemName		= pPlayer->Inventory_GetItemNameByID(i);
			pItemClassName	= pPlayer->Inventory_GetItemClassNameByID(i);

			if ( pItemName == "" || pItemClassName == "" )
				continue;

			KeyValues *itemData = new KeyValues("data");
			itemData->SetString("name",			pItemName);
			itemData->SetString("classname",	pItemClassName);
			itemData->SetInt("count",			pPocketItems[i]);
			PocketItems->AddItem(0, itemData);
		}

		// Actualizamos las barras de llenado.
		BackpackProgress->SetProgress( pBackpackCountItems / sv_max_inventory_backpack.GetFloat() );
		InventoryProgress->SetProgress( pPocketCountItems / sv_max_inventory_pocket.GetFloat() );
	}

	SetVisible(cl_inventory.GetBool());
}

void CInventoryPanel::OnCommand(const char* cmd)
{
	int Backpack_pItem			= BackpackItems->GetSelectedItem();
	int Pocket_pItem			= PocketItems->GetSelectedItem();

	const char *pItemClassName	= "";
	const char *pFrom			= "";
	KeyValues *ItemData;
	char com[100];

	// Se ha seleccionado un objeto de la mochila.
	if ( Backpack_pItem != -1 )
	{
		ItemData		= BackpackItems->GetItemData(Backpack_pItem);
		pItemClassName	= ItemData->GetString("classname");
		pFrom			= "backpack";
	}
	// Se ha seleccionado un objeto del bolsillo.
	else if ( Pocket_pItem != -1 )
	{
		ItemData		= PocketItems->GetItemData(Pocket_pItem);
		pItemClassName	= ItemData->GetString("classname");
		pFrom			= "pocket";
	}

	// Ocultar el inventario.
	if ( !Q_stricmp(cmd, "turnoff") )
		cl_inventory.SetValue(0);

	// Tirar un objeto.
	if ( !Q_stricmp(cmd, "dropitem") )
	{
		Q_snprintf(com, sizeof(com), "dropitem %s %s", pFrom, pItemClassName);

		engine->ServerCmd(com);
		cl_update_inventory.SetValue(1);
	}

	// Usar un objeto.
	if ( !Q_stricmp(cmd, "useitem") )
	{
		Q_snprintf(com, sizeof(com), "useitem %s %s", pFrom, pItemClassName);

		engine->ServerCmd(com);
		cl_update_inventory.SetValue(1);
	}

	// Mover un objeto.
	if ( !Q_stricmp(cmd, "moveitem") )
	{
		const char *pTo = "backpack";

		if ( pFrom == "backpack" )
			pTo = "pocket";

		Q_snprintf(com, sizeof(com), "moveitem %s %s %s", pFrom, pTo, pItemClassName);

		engine->ServerCmd(com);
		cl_update_inventory.SetValue(1);
	}
}

CON_COMMAND(openinventory, "Toggles the Inventory Screen")
{
	cl_inventory.SetValue(!cl_inventory.GetBool());
};