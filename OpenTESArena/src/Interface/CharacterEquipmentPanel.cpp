#include "CharacterEquipmentPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
	: Panel(game) { }

CharacterEquipmentPanel::~CharacterEquipmentPanel()
{
	auto &inputManager = this->getGame().inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterEquipment, false);
}

bool CharacterEquipmentPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const TextBoxInitInfo playerNameTextBoxInitInfo = CharacterSheetUiView::getPlayerNameTextBoxInitInfo(fontLibrary);
	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	if (!this->nameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBoxInitInfo playerRaceTextBoxInitInfo = CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(fontLibrary);
	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	if (!this->raceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const TextBoxInitInfo playerClassTextBoxInitInfo = CharacterSheetUiView::getPlayerClassTextBoxInitInfo(fontLibrary);
	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	if (!this->classTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const TextBoxInitInfo playerLevelTextBoxInitInfo = CharacterEquipmentUiView::getPlayerLevelTextBoxInitInfo(fontLibrary);
	const std::string playerLevelText = CharacterSheetUiModel::getPlayerLevel(game);
	if (!this->levelTextBox.init(playerLevelTextBoxInitInfo, playerLevelText, renderer))
	{
		DebugLogError("Couldn't init player level text box.");
		return false;
	}

	Buffer<InventoryUiModel::ItemUiDefinition> itemUiDefs = InventoryUiModel::getPlayerInventoryItems(game);
	std::vector<std::pair<std::string, Color>> elements;
	for (InventoryUiModel::ItemUiDefinition &itemUiDef : itemUiDefs)
	{
		elements.emplace_back(std::move(itemUiDef.text), itemUiDef.color);
	}

	this->inventoryListBox.init(InventoryUiView::PlayerInventoryRect,
		InventoryUiView::makePlayerInventoryListBoxProperties(fontLibrary), game.renderer);
	for (int i = 0; i < static_cast<int>(elements.size()); i++)
	{
		auto &pair = elements[i];
		this->inventoryListBox.add(std::move(pair.first));
		this->inventoryListBox.setOverrideColor(i, pair.second);
		this->inventoryListBox.setCallback(i,
			[this, &game, i]()
		{
			ItemInventory &playerInventory = game.player.inventory;
			ItemInstance &itemInst = playerInventory.getSlot(i);
			itemInst.isEquipped = !itemInst.isEquipped;

			const Color &equipColor = InventoryUiView::getItemDisplayColor(itemInst);
			this->inventoryListBox.setOverrideColor(i, equipColor);
		});

		ButtonProxy::RectFunction itemRectFunc = [this, i]()
		{
			return this->inventoryListBox.getItemGlobalRect(i);
		};

		ButtonProxy::Callback itemCallback = this->inventoryListBox.getCallback(i);

		this->addButtonProxy(MouseButtonType::Left, itemRectFunc, itemCallback, this->inventoryListBox.getRect());
	}

	this->backToStatsButton = Button<Game&>(
		CharacterSheetUiView::BackToStatsButtonX,
		CharacterSheetUiView::BackToStatsButtonY,
		CharacterSheetUiView::BackToStatsButtonWidth,
		CharacterSheetUiView::BackToStatsButtonHeight,
		CharacterSheetUiController::onBackToStatsButtonSelected);
	this->spellbookButton = Button<Game&>(
		CharacterSheetUiView::SpellbookButtonX,
		CharacterSheetUiView::SpellbookButtonY,
		CharacterSheetUiView::SpellbookButtonWidth,
		CharacterSheetUiView::SpellbookButtonHeight,
		CharacterSheetUiController::onSpellbookButtonSelected);
	this->dropButton = Button<Game&, int>(
		CharacterSheetUiView::DropButtonX,
		CharacterSheetUiView::DropButtonY,
		CharacterSheetUiView::DropButtonWidth,
		CharacterSheetUiView::DropButtonHeight,
		CharacterSheetUiController::onDropButtonSelected);
	this->scrollDownButton = Button<ListBox&>(
		CharacterSheetUiView::ScrollDownButtonCenterPoint,
		CharacterSheetUiView::ScrollDownButtonWidth,
		CharacterSheetUiView::ScrollDownButtonHeight,
		CharacterSheetUiController::onInventoryScrollDownButtonSelected);
	this->scrollUpButton = Button<ListBox&>(
		CharacterSheetUiView::ScrollUpButtonCenterPoint,
		CharacterSheetUiView::ScrollUpButtonWidth,
		CharacterSheetUiView::ScrollUpButtonHeight,
		CharacterSheetUiController::onInventoryScrollUpButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->backToStatsButton.getRect(),
		[this, &game]() { this->backToStatsButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->spellbookButton.getRect(),
		[this, &game]() { this->spellbookButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->dropButton.getRect(),
		[this, &game]()
	{
		// @todo: give the index of the currently selected item instead.
		const int itemIndex = 0;
		this->dropButton.click(game, itemIndex);
	});

	this->addButtonProxy(MouseButtonType::Left, this->scrollDownButton.getRect(),
		[this, &game]() { this->scrollDownButton.click(this->inventoryListBox); });
	this->addButtonProxy(MouseButtonType::Left, this->scrollUpButton.getRect(),
		[this, &game]() { this->scrollUpButton.click(this->inventoryListBox); });

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterEquipment, true);

	auto backToStatsInputActionFunc = CharacterSheetUiController::onBackToStatsInputAction;
	this->addInputActionListener(InputActionName::Back, backToStatsInputActionFunc);
	this->addInputActionListener(InputActionName::CharacterSheet, backToStatsInputActionFunc);

	this->addMouseScrollChangedListener([this](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			this->scrollDownButton.click(this->inventoryListBox);
		}
		else if (type == MouseWheelScrollType::Up)
		{
			this->scrollUpButton.click(this->inventoryListBox);
		}
	});

	auto &textureManager = game.textureManager;
	const UiTextureID bodyTextureID = CharacterSheetUiView::allocBodyTexture(game);
	const UiTextureID pantsTextureID = CharacterSheetUiView::allocPantsTexture(game);
	const UiTextureID headTextureID = CharacterSheetUiView::allocHeadTexture(game);
	const UiTextureID shirtTextureID = CharacterSheetUiView::allocShirtTexture(game);
	const UiTextureID equipmentBgTextureID = CharacterSheetUiView::allocEquipmentBgTexture(textureManager, renderer);
	this->bodyTextureRef.init(bodyTextureID, renderer);
	this->pantsTextureRef.init(pantsTextureID, renderer);
	this->headTextureRef.init(headTextureID, renderer);
	this->shirtTextureRef.init(shirtTextureID, renderer);
	this->equipmentBgTextureRef.init(equipmentBgTextureID, renderer);

	const Int2 bodyTextureDims = *renderer.tryGetUiTextureDims(bodyTextureID);
	const Int2 pantsTextureDims = *renderer.tryGetUiTextureDims(pantsTextureID);
	const Int2 headTextureDims = *renderer.tryGetUiTextureDims(headTextureID);
	const Int2 shirtTextureDims = *renderer.tryGetUiTextureDims(shirtTextureID);
	const Int2 equipmentBgTextureDims = *renderer.tryGetUiTextureDims(equipmentBgTextureID);

	this->addDrawCall(
		bodyTextureID,
		CharacterSheetUiView::getBodyOffset(game),
		bodyTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		pantsTextureID,
		CharacterSheetUiView::getPantsOffset(game),
		pantsTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		headTextureID,
		CharacterSheetUiView::getHeadOffset(game),
		headTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		shirtTextureID,
		CharacterSheetUiView::getShirtOffset(game),
		shirtTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		equipmentBgTextureID,
		Int2::Zero,
		equipmentBgTextureDims,
		PivotType::TopLeft);

	const Rect &playerNameTextBoxRect = this->nameTextBox.getRect();
	this->addDrawCall(
		this->nameTextBox.getTextureID(),
		playerNameTextBoxRect.getTopLeft(),
		playerNameTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerRaceTextBoxRect = this->raceTextBox.getRect();
	this->addDrawCall(
		this->raceTextBox.getTextureID(),
		playerRaceTextBoxRect.getTopLeft(),
		playerRaceTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerClassTextBoxRect = this->classTextBox.getRect();
	this->addDrawCall(
		this->classTextBox.getTextureID(),
		playerClassTextBoxRect.getTopLeft(),
		playerClassTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerLevelTextBoxRect = this->levelTextBox.getRect();
	this->addDrawCall(
		this->levelTextBox.getTextureID(),
		playerLevelTextBoxRect.getTopLeft(),
		playerLevelTextBoxRect.getSize(),
		PivotType::TopLeft);

	// Need a texture func for the list box due to the non-constness of the getter.
	UiDrawCall::TextureFunc inventoryListBoxTextureFunc = [this]()
	{
		return this->inventoryListBox.getTextureID();
	};

	const Rect &inventoryListBoxRect = this->inventoryListBox.getRect();
	this->addDrawCall(
		inventoryListBoxTextureFunc,
		inventoryListBoxRect.getTopLeft(),
		inventoryListBoxRect.getSize(),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
