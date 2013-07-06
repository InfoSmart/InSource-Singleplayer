#include "cbase.h"
#include "vgui_loot.h"
#include "hud_macros.h"

using namespace vgui;

#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Button.h>
#include <vgui/ILocalize.h>

#include "c_baseplayer.h"
#include "c_in_player.h"
#include "usermessages.h"

//=========================================================
// CLootPanel
//=========================================================
class CLootPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CLootPanel, vgui::Frame); 
 
	CLootPanel(vgui::VPANEL parent);
	~CLootPanel() { };
	
	int pLootItems[100];
	int pIndexLoot;
protected:
	virtual void Reset();
	virtual void OnTick();
	virtual void OnCommand(const char* cmd);
 
private:
	SectionedListPanel *BackpackItems;
	SectionedListPanel *LootItems;

	ProgressBar *BackpackProgress;
	Button *Move;

	bool Updated;
};

//=========================================================
// CInventoryPanelInterface
//=========================================================
class CLootPanelInterface : public ILootPanel
{
public:
	CLootPanel *LootPanel;

public:
	CLootPanelInterface()
	{
		LootPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		LootPanel = new CLootPanel(parent);
	}
	void Destroy()
	{
		if ( LootPanel )
		{
			LootPanel->SetParent((vgui::Panel *)NULL);
			delete LootPanel;
		}
	}
};

static CLootPanelInterface g_LootPanel;
ILootPanel* LootPanel = (ILootPanel*)&g_LootPanel;

//=========================================================
// Comandos
//=========================================================

ConVar cl_lootpanel("cl_lootpanel", "0",	FCVAR_CLIENTDLL, "Muestra/Oculta el panel de Loot.");
ConVar cl_update_loot("cl_update_loot", "0",	FCVAR_CLIENTDLL, "Actualiza el panel de Loot");

//=========================================================
// Mensaje: Recibe los objetos del Loot.
//=========================================================
void __MsgFunc_Loot(bf_read &msg)
{
	g_LootPanel.LootPanel->pIndexLoot = msg.ReadShort(); // ID del Loot

	int pID		= msg.ReadShort(); // ID del array.
	int pEntity	= msg.ReadShort(); // ID del objeto.

	g_LootPanel.LootPanel->pLootItems[pID] = pEntity;
}

//=========================================================
// Constructor
//=========================================================
CLootPanel::CLootPanel(vgui::VPANEL parent) : BaseClass(NULL, "LootPanel")
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
	LootItems		= new SectionedListPanel(this, "LootItems");

	BackpackProgress	= new ProgressBar(this, "BackpackProgress");
	Move				= new Button(this, "Move", "Move");

	Move->SetCommand("moveitem");

	Reset();	
	LoadControlSettings("resource/UI/LootPanel.res");
 
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	HOOK_MESSAGE(Loot);
}

//=========================================================
// Reinicio
//=========================================================
void CLootPanel::Reset()
{
	BackpackItems->RemoveAll();
	BackpackItems->RemoveAllSections();
	BackpackItems->AddSection(0, "");
	BackpackItems->AddColumnToSection(0, "name", "#Inventory_Backpack", 0, 300);
	BackpackItems->AddColumnToSection(0, "count", "", 0, 100);

	LootItems->RemoveAll();
	LootItems->RemoveAllSections();
	LootItems->AddSection(0, "");
	LootItems->AddColumnToSection(0, "name", "", 0, 300);
	LootItems->AddColumnToSection(0, "count", "", 0, 100);

	BackpackProgress->SetProgress(0.0);
	Updated = false;

	pIndexLoot = 0;

	for ( int i = 0; i < ARRAYSIZE(pLootItems); i++ )
		pLootItems[i] = 0;
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CLootPanel::OnTick()
{
	BaseClass::OnTick();
	C_IN_Player *pPlayer = (C_IN_Player *)C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
	{
		SetVisible(false);
		return;
	}

	if ( cl_update_loot.GetBool() )
	{
		// Obtenemos los limites del inventario.
		ConVarRef sv_max_inventory_backpack("sv_max_inventory_backpack");

		// Ya no necesitamos actualizar.
		cl_update_loot.SetValue(0);

		// Removemos todos los objetos de la lista.
		BackpackItems->RemoveAll();
		LootItems->RemoveAll();

		int pEntity;
		const char *pItemName;
		const char *pItemClassName;

		float pBackpackCountItems = 0;

		int pBackpackItems[100];	// Mochila: Objetos y cantidad.
		int iLootItems[100];		// Loot: Objetos y cantidad.

		// Iniciamos los array
		for ( int i = 0; i < ARRAYSIZE(pBackpackItems); i++ )
			pBackpackItems[i]	= 0;
		for ( int i = 0; i < ARRAYSIZE(iLootItems); i++ )
			iLootItems[i]	= 0;
		
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
			// No tenemos este objeto.
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

		// Loot
		// Primero obtenemos todos los objetos y los agrupamos en array.
		for ( int i = 1; i < ARRAYSIZE(pLootItems); i++ )
		{
			pEntity = pLootItems[i];

			// No hay nada en este Slot
			if ( pEntity == 0 )
				continue;

			// Aquí vamos contando los objetos de este mismo tipo.
			// !!!NOTE: Todo esto fue desarrollado por alguien que no sabe mucho C++... Odio los array de C++...
			iLootItems[pEntity] = (iLootItems[pEntity] + 1);		
		}
		// Ahora obtenemos la lista de objetos con su respectiva cantidad y la mostramos.
		for ( int i = 1; i < ARRAYSIZE(pLootItems); i++ )
		{
			// !!!NOTE: i ahora es la ID del objeto
			// No tenemos este objeto.
			if ( iLootItems[i] == 0 || !iLootItems[i] )
				continue;

			pItemName		= pPlayer->Inventory_GetItemNameByID(i);
			pItemClassName	= pPlayer->Inventory_GetItemClassNameByID(i);

			if ( pItemName == "" || pItemClassName == "" )
				continue;

			KeyValues *itemData = new KeyValues("data");
			itemData->SetString("name",			pItemName);
			itemData->SetString("classname",	pItemClassName);
			itemData->SetInt("count",			iLootItems[i]);
			LootItems->AddItem(0, itemData);
		}

		// Actualizamos las barras de llenado.
		BackpackProgress->SetProgress( pBackpackCountItems / sv_max_inventory_backpack.GetFloat() );
		Updated = true;
	}

	// Ya se ha cerrado el panel pero los datos del Loot anterior siguen.
	// Reiniciamos (limpiamos) el panel para evitar "Loot remoto"
	if ( !cl_lootpanel.GetBool() && Updated )
		Reset();

	SetVisible(cl_lootpanel.GetBool());
}

void CLootPanel::OnCommand(const char* cmd)
{
	// Ocultar el panel de Loot.
	if ( !Q_stricmp(cmd, "turnoff") )
		cl_lootpanel.SetValue(0);

	// Mover un objeto.
	if ( !Q_stricmp(cmd, "moveitem") )
	{
		int LootpItem = LootItems->GetSelectedItem();

		// No hay seleccionado ningún objeto del Loot.
		if ( LootpItem == -1 )
			return;

		KeyValues *pItemData		= LootItems->GetItemData(LootpItem);
		const char *pItemClassName	= pItemData->GetString("classname");

		// ¿Clase invalida?
		if ( pItemClassName == "" )
			return;

		Msg("[LOOT] Intentando enviar el objeto %s del Loot %i \r\n", pItemClassName, pIndexLoot);

		char com[100];
		Q_snprintf(com, sizeof(com), "lootitem %i %s", pIndexLoot, pItemClassName);
		// Ejemplo: lootmoveitem 10 item_bandage

		engine->ServerCmd(com);
	}
}

CON_COMMAND(openloot, "")
{
	cl_lootpanel.SetValue(!cl_lootpanel.GetBool());
};