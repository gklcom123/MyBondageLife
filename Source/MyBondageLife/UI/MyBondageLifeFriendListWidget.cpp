// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeFriendListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Canvas units are 1.5x the viewport pixels at the default UI scale.
	constexpr float FriendEntryIconSize = 64.0f;
	constexpr float FriendEntryPositionX = 780.0f;
	constexpr float FriendEntryPositionY = -30.0f;

	constexpr float FriendPanelWidth = 1500.0f;
	constexpr float FriendPanelHeight = 840.0f;
	constexpr float FriendCloseButtonSize = 52.0f;
	constexpr float FriendRowHeight = 56.0f;

	constexpr float FriendNameColumnWidth = 400.0f;
	constexpr float FriendContractColumnWidth = 320.0f;
	constexpr float FriendClassColumnWidth = 300.0f;
	constexpr float FriendStatusColumnWidth = 180.0f;

	/** Row box size and inner text placement (canvas units). */
	constexpr float FriendRowWidth =
		32.0f + FriendNameColumnWidth + FriendClassColumnWidth + FriendContractColumnWidth + FriendStatusColumnWidth;
	constexpr float FriendRowTextOffsetY = 12.0f;
	constexpr float FriendRowTextHeight = 34.0f;
	constexpr float FriendRowPaddingX = 16.0f;

	// "好友列表（V）"
	const TCHAR* FriendEntryLabelText = TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09");
	// "好友列表"
	const TCHAR* FriendPanelTitleText = TEXT("\u597D\u53CB\u5217\u8868");
	// "暂无好友"
	const TCHAR* FriendEmptyHintText = TEXT("\u6682\u65E0\u597D\u53CB");
	// "在线" / "离线"
	const TCHAR* FriendOnlineText = TEXT("\u5728\u7EBF");
	const TCHAR* FriendOfflineText = TEXT("\u79BB\u7EBF");
}

UMyBondageLifeFriendListWidget::UMyBondageLifeFriendListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Full screen overlay: the widget itself must not swallow clicks, otherwise the
	// overlays below it (and their buttons) never receive the mouse.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Hard reference so the icon is cooked into packaged builds.
	static ConstructorHelpers::FObjectFinder<UTexture2D> FriendIconFinder(
		TEXT("/Game/2DAssest/UI/UI_FriendIcon.UI_FriendIcon"));
	if (FriendIconFinder.Succeeded())
	{
		FriendIconTexture = FriendIconFinder.Object;
	}
}

TSharedRef<SWidget> UMyBondageLifeFriendListWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		if (!TryAdoptBlueprintLayout())
		{
			BuildLayout();
		}
		BuildDialogs();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeFriendListWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;
	// A full screen overlay must not swallow clicks meant for the layers below it;
	// only its children (buttons, panels) take part in hit testing.
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// ------------------------------------------------------------ bottom entry
	OpenButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FriendListButton"));
	{
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		OpenButton->SetStyle(ButtonStyle);
	}
	OpenButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleOpenButtonClicked);

	if (UCanvasPanelSlot* EntrySlot = RootCanvas->AddChildToCanvas(OpenButton))
	{
		EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		EntrySlot->SetPosition(FVector2D(FriendEntryPositionX, FriendEntryPositionY));
		EntrySlot->SetAutoSize(true);
		EntrySlot->SetZOrder(20);
	}

	UVerticalBox* EntryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendEntryColumn"));
	OpenButton->AddChild(EntryColumn);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendIconSize"));
	IconSize->SetWidthOverride(FriendEntryIconSize);
	IconSize->SetHeightOverride(FriendEntryIconSize);
	if (UVerticalBoxSlot* IconSlot = EntryColumn->AddChildToVerticalBox(IconSize))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UImage* FriendIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FriendEntryIcon"));
	if (FriendIconTexture)
	{
		FriendIcon->SetBrushFromTexture(FriendIconTexture, true);
	}
	IconSize->AddChild(FriendIcon);

	UTextBlock* EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendEntryLabel"));
	EntryLabel->SetText(FText::FromString(FriendEntryLabelText));
	EntryLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	EntryLabel->SetJustification(ETextJustify::Center);
	EntryLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
	EntryLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	{
		FSlateFontInfo EntryFont = EntryLabel->GetFont();
		EntryFont.Size = 16;
		EntryLabel->SetFont(EntryFont);
	}
	if (UVerticalBoxSlot* LabelSlot = EntryColumn->AddChildToVerticalBox(EntryLabel))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	}

	// ------------------------------------------------------------------ panel
	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendListPanel"));
	PanelBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetSize(FVector2D(FriendPanelWidth, FriendPanelHeight));
		PanelSlot->SetZOrder(30);
	}
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FriendPanelCanvas"));
	PanelBorder->AddChild(PanelCanvas);

	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendCloseSize"));
	CloseSize->SetWidthOverride(FriendCloseButtonSize);
	CloseSize->SetHeightOverride(FriendCloseButtonSize);
	if (UCanvasPanelSlot* CloseSlot = PanelCanvas->AddChildToCanvas(CloseSize))
	{
		CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CloseSlot->SetPosition(FVector2D(-18.0f, 18.0f));
	}

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FriendCloseButton"));
	CloseButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleCloseButtonClicked);
	CloseSize->AddChild(CloseButton);

	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendCloseLabel"));
	CloseLabel->SetText(FText::FromString(TEXT("X")));
	CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	CloseLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo CloseFont = CloseLabel->GetFont();
		CloseFont.Size = 20;
		CloseLabel->SetFont(CloseFont);
	}
	CloseButton->AddChild(CloseLabel);

	// Title
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendPanelTitle"));
	Title->SetText(FText::FromString(FriendPanelTitleText));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo TitleFont = Title->GetFont();
		TitleFont.Size = 30;
		Title->SetFont(TitleFont);
	}
	if (UCanvasPanelSlot* TitleSlot = PanelCanvas->AddChildToCanvas(Title))
	{
		TitleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		TitleSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		TitleSlot->SetPosition(FVector2D(56.0f, 44.0f));
		TitleSlot->SetAutoSize(true);
	}

	// Column header
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FriendHeaderRow"));
	if (UCanvasPanelSlot* HeaderSlot = PanelCanvas->AddChildToCanvas(HeaderRow))
	{
		HeaderSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		HeaderSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		HeaderSlot->SetPosition(FVector2D(56.0f, 104.0f));
		HeaderSlot->SetAutoSize(true);
	}

	auto AddHeaderCell = [this, HeaderRow](const TCHAR* Text, float Width)
	{
		USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Cell->SetWidthOverride(Width);
		if (UHorizontalBoxSlot* CellSlot = HeaderRow->AddChildToHorizontalBox(Cell))
		{
			CellSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
		}

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f)));
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 22;
			Label->SetFont(LabelFont);
		}
		Cell->AddChild(Label);
	};

	// 角色名 / 角色类别 / 状态
	AddHeaderCell(TEXT("\u89D2\u8272\u540D"), FriendNameColumnWidth);
	AddHeaderCell(TEXT("\u89D2\u8272\u7C7B\u522B"), FriendClassColumnWidth);
	AddHeaderCell(TEXT("\u4E3B\u5974\u5173\u7CFB"), FriendContractColumnWidth);
	AddHeaderCell(TEXT("\u72B6\u6001"), 200.0f);

	// Scrollable list
	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendListSize"));
	ListSize->SetWidthOverride(FriendPanelWidth - 112.0f);
	ListSize->SetHeightOverride(FriendPanelHeight - 200.0f);
	if (UCanvasPanelSlot* ListSlot = PanelCanvas->AddChildToCanvas(ListSize))
	{
		ListSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		ListSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		ListSlot->SetPosition(FVector2D(56.0f, 148.0f));

		// Without auto size the canvas slot keeps a zero sized rect and the scroll
		// box is clipped to nothing (only a sliver of the first row is visible).
		ListSlot->SetAutoSize(true);
	}

	FriendListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("FriendListBox"));
	FriendListBox->SetScrollbarThickness(FVector2D(12.0f, 12.0f));
	ListSize->AddChild(FriendListBox);

	// Empty state
	EmptyHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendEmptyHint"));
	EmptyHint->SetText(FText::FromString(FriendEmptyHintText));
	EmptyHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
	{
		FSlateFontInfo HintFont = EmptyHint->GetFont();
		HintFont.Size = 24;
		EmptyHint->SetFont(HintFont);
	}
	if (UCanvasPanelSlot* EmptySlot = PanelCanvas->AddChildToCanvas(EmptyHint))
	{
		EmptySlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		EmptySlot->SetAlignment(FVector2D(0.5f, 0.5f));
		EmptySlot->SetAutoSize(true);
	}
	EmptyHint->SetVisibility(ESlateVisibility::Collapsed);

	// Footer note about the presence delay.
	UTextBlock* FooterNote = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendListFooter"));
	FooterNote->SetText(FText::FromString(
		TEXT("\u597D\u53CB\u79BB\u7EBF\u663E\u793A\u53EF\u80FD\u4F1A\u67091\u5206\u949F\u7684\u5EF6\u8FDF\u663E\u793A")));
	FooterNote->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.55f, 0.58f)));
	FooterNote->SetJustification(ETextJustify::Left);
	{
		FSlateFontInfo FooterFont = FooterNote->GetFont();
		FooterFont.Size = 18;
		FooterNote->SetFont(FooterFont);
	}
	if (UCanvasPanelSlot* FooterSlot = PanelCanvas->AddChildToCanvas(FooterNote))
	{
		FooterSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		FooterSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		FooterSlot->SetPosition(FVector2D(56.0f, -22.0f));
		FooterSlot->SetAutoSize(true);
	}
	// ------------------------------------------------------- request dialog
	RequestBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendRequestBackdrop"));
	RequestBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(RequestBackdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(40);
	}
	RequestBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	RequestBackdrop->SetHorizontalAlignment(HAlign_Center);
	RequestBackdrop->SetVerticalAlignment(VAlign_Center);

	UBorder* DialogBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendRequestBox"));
	DialogBox->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	DialogBox->SetPadding(FMargin(40.0f, 32.0f));
	RequestBackdrop->AddChild(DialogBox);

	UVerticalBox* DialogColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendRequestColumn"));
	DialogBox->AddChild(DialogColumn);

	RequestText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendRequestText"));
	RequestText->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	RequestText->SetJustification(ETextJustify::Center);
	RequestText->SetAutoWrapText(true);
	{
		FSlateFontInfo RequestFont = RequestText->GetFont();
		RequestFont.Size = 26;
		RequestText->SetFont(RequestFont);
	}
	if (UVerticalBoxSlot* TextSlot = DialogColumn->AddChildToVerticalBox(RequestText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	UHorizontalBox* ButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FriendRequestButtons"));
	if (UVerticalBoxSlot* RowSlot = DialogColumn->AddChildToVerticalBox(ButtonsRow))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	auto AddDialogButton = [this, ButtonsRow](const TCHAR* LabelText, bool bPrimary)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(160.0f);
		Size->SetHeightOverride(56.0f);
		if (UHorizontalBoxSlot* SizeSlot = ButtonsRow->AddChildToHorizontalBox(Size))
		{
			SizeSlot->SetPadding(FMargin(12.0f, 0.0f));
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Size->AddChild(Button);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(LabelText));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 24;
			Label->SetFont(LabelFont);
		}
		Button->AddChild(Label);
		(void)bPrimary;
		return Button;
	};

	// 是 / 否
	UButton* AcceptButton = AddDialogButton(TEXT("\u662F"), true);
	AcceptButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleAcceptClicked);

	UButton* DeclineButton = AddDialogButton(TEXT("\u5426"), false);
	DeclineButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleDeclineClicked);
}

void UMyBondageLifeFriendListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnFriendsChanged.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleFriendsChanged);
		}
	}

	RefreshFriends();
}

void UMyBondageLifeFriendListWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnFriendsChanged.RemoveDynamic(this, &UMyBondageLifeFriendListWidget::HandleFriendsChanged);
		}
	}

	Super::NativeDestruct();
}

void UMyBondageLifeFriendListWidget::RefreshFriends()
{
	if (!FriendListBox || !EmptyHint)
	{
		return;
	}

	FriendListBox->ClearChildren();
	RowProxies.Reset();

	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	const TArray<FMyBondageLifeFriendEntry> Friends = Account
		? Account->GetFriends()
		: TArray<FMyBondageLifeFriendEntry>();

	EmptyHint->SetVisibility(Friends.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	for (const FMyBondageLifeFriendEntry& Friend : Friends)
	{
		// Absolute layout: fixed row box with a canvas, so the three columns always
		// land at fixed offsets regardless of text width.
		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		RowSize->SetWidthOverride(FriendRowWidth);
		RowSize->SetHeightOverride(FriendRowHeight);

		// Clickable on purpose; per-friend actions come with the backend.
		UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UMyBondageLifeFriendRowProxy* RowProxy = NewObject<UMyBondageLifeFriendRowProxy>(this);
		RowProxy->Owner = this;
		RowProxy->Account = Friend.AccountName;
		RowProxy->ContractId = Friend.ContractId;
		RowProxy->Role = Friend.ContractRole;
		RowProxies.Add(RowProxy);
		RowButton->OnClicked.AddDynamic(RowProxy, &UMyBondageLifeFriendRowProxy::HandleClicked);
		RowSize->AddChild(RowButton);

		UBorder* RowBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBackground->SetBrushColor(FLinearColor(0.93f, 0.93f, 0.95f, 1.0f));
		RowBackground->SetPadding(FMargin(0.0f));
		RowButton->AddChild(RowBackground);

		UCanvasPanel* RowCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		RowBackground->AddChild(RowCanvas);

		auto AddCell = [this, RowCanvas](float OffsetX, const FString& Text, const FLinearColor& Color)
		{
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Label->SetText(FText::FromString(Text));
			Label->SetColorAndOpacity(FSlateColor(Color));
			Label->SetJustification(ETextJustify::Left);
			Label->SetAutoWrapText(false);
			{
				FSlateFontInfo LabelFont = Label->GetFont();
				LabelFont.Size = 22;
				Label->SetFont(LabelFont);
			}

			if (UCanvasPanelSlot* LabelSlot = RowCanvas->AddChildToCanvas(Label))
			{
				LabelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
				LabelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				LabelSlot->SetPosition(FVector2D(OffsetX, FriendRowTextOffsetY));
				LabelSlot->SetSize(FVector2D(FriendRowWidth - OffsetX, FriendRowTextHeight));
			}
		};

		// 角色名 / 角色类别 / 状态
		const FLinearColor TextColor(0.05f, 0.05f, 0.05f);
		AddCell(FriendRowPaddingX, Friend.CharacterName, TextColor);
		AddCell(FriendRowPaddingX + FriendNameColumnWidth,
			UMyBondageLifeAccountSubsystem::MakeCharacterClassLabel(Friend.CharacterClass), TextColor);
		AddCell(FriendRowPaddingX + FriendNameColumnWidth + FriendClassColumnWidth,
			GetContractRoleText(Friend.ContractRole),
			Friend.ContractRole == EMyBondageLifeContractRole::None
				? FLinearColor(0.35f, 0.35f, 0.38f)
				: FLinearColor(0.45f, 0.2f, 0.6f));

		AddCell(FriendRowPaddingX + FriendNameColumnWidth + FriendClassColumnWidth + FriendContractColumnWidth,
			Friend.bOnline ? FriendOnlineText : FriendOfflineText,
			Friend.bOnline ? FLinearColor(0.10f, 0.55f, 0.15f) : FLinearColor(0.5f, 0.5f, 0.5f));

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(FriendListBox->AddChild(RowSize)))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
			RowSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}
}

void UMyBondageLifeFriendListWidget::SetPanelOpen(bool bOpen)
{
	if (IsPanelOpen() == bOpen)
	{
		return;
	}

	if (bOpen)
	{
		RefreshFriends();
	}

	PanelBorder->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	OnPanelOpenChanged.Broadcast(bOpen);
}

void UMyBondageLifeFriendListWidget::TogglePanel()
{
	SetPanelOpen(!IsPanelOpen());
}

bool UMyBondageLifeFriendListWidget::IsPanelOpen() const
{
	return PanelBorder && PanelBorder->GetVisibility() == ESlateVisibility::Visible;
}

void UMyBondageLifeFriendListWidget::ShowFriendRequest(const FString& RequestId, const FString& RequesterCharacterName)
{
	if (!RequestBackdrop || !RequestText)
	{
		return;
	}

	PendingRequestId = RequestId;
	PendingRequesterAccount.Empty();

	// "[name] 请求添加为好友，是否同意?"
	RequestText->SetText(FText::FromString(FString::Printf(
		TEXT("[%s]\u8BF7\u6C42\u6DFB\u52A0\u4E3A\u597D\u53CB\uFF0C\u662F\u5426\u540C\u610F\uFF1F"),
		*RequesterCharacterName)));

	bRequestDialogIsContract = false;
	RequestText->SetWrapTextAt(0.0f);
	RequestBackdrop->SetVisibility(ESlateVisibility::Visible);
	UpdateModalState();
}

bool UMyBondageLifeFriendListWidget::IsRequestDialogOpen() const
{
	return RequestBackdrop && RequestBackdrop->GetVisibility() == ESlateVisibility::Visible;
}

void UMyBondageLifeFriendListWidget::HandleOpenButtonClicked()
{
	SetPanelOpen(true);
}

void UMyBondageLifeFriendListWidget::HandleCloseButtonClicked()
{
	SetPanelOpen(false);
}

void UMyBondageLifeFriendListWidget::HandleFriendRowClicked()
{
	// Clickable on purpose; the per-friend actions come with the backend.
	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: row clicked (no action yet)."));
}

void UMyBondageLifeFriendListWidget::HandleAcceptClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: accept clicked (dialogOpen=%d, requester='%s')"),
		IsRequestDialogOpen() ? 1 : 0, *PendingRequesterAccount);

	if (!IsRequestDialogOpen())
	{
		return;
	}

	RequestBackdrop->SetVisibility(ESlateVisibility::Collapsed);

	if (bRequestDialogIsContract)
	{
		const FString RequestId = PendingContractRequestId;
		bRequestDialogIsContract = false;
		PendingContractRequestId.Empty();
		OnContractRequestAnswered.Broadcast(RequestId, true);
	}
	else
	{
		OnRequestAnswered.Broadcast(true);
	}

	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleDeclineClicked()
{
	if (!IsRequestDialogOpen())
	{
		return;
	}

	RequestBackdrop->SetVisibility(ESlateVisibility::Collapsed);

	if (bRequestDialogIsContract)
	{
		const FString RequestId = PendingContractRequestId;
		bRequestDialogIsContract = false;
		PendingContractRequestId.Empty();
		OnContractRequestAnswered.Broadcast(RequestId, false);
	}
	else
	{
		OnRequestAnswered.Broadcast(false);
	}

	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleFriendsChanged()
{
	if (IsPanelOpen())
	{
		RefreshFriends();
	}
}

// ---------------------------------------------------------------------------
// Friend action menu, confirmation dialog, contract request dialog
// ---------------------------------------------------------------------------

FString UMyBondageLifeFriendListWidget::GetContractRoleText(EMyBondageLifeContractRole Role)
{
	switch (Role)
	{
	case EMyBondageLifeContractRole::TemporaryMaster: return TEXT("\u4E34\u65F6\u5974\u96B6");   // 临时奴隶
	case EMyBondageLifeContractRole::TemporarySlave:  return TEXT("\u4E34\u65F6\u4E3B\u4EBA");   // 临时主人
	case EMyBondageLifeContractRole::Master:          return TEXT("\u5974\u96B6");               // 奴隶
	case EMyBondageLifeContractRole::Slave:           return TEXT("\u4E3B\u4EBA");               // 主人
	default:                                          return TEXT("\u597D\u53CB");               // 好友
	}
}

void UMyBondageLifeFriendListWidget::UpdateModalState()
{
	const bool bMenuOpen = MenuBackdrop && MenuBackdrop->GetVisibility() == ESlateVisibility::Visible;
	const bool bConfirmOpen = ConfirmBackdrop && ConfirmBackdrop->GetVisibility() == ESlateVisibility::Visible;
	const bool bRequestOpen = IsRequestDialogOpen();

	OnModalChanged.Broadcast(bMenuOpen || bConfirmOpen || bRequestOpen);
}

void UMyBondageLifeFriendListWidget::BuildDialogs()
{
	// ------------------------------------------------------- friend action menu
	MenuBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendMenuBackdrop"));
	MenuBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	MenuBackdrop->SetHorizontalAlignment(HAlign_Center);
	MenuBackdrop->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuBackdrop))
	{
		MenuSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		MenuSlot->SetOffsets(FMargin(0.0f));
		MenuSlot->SetZOrder(50);
	}
	MenuBackdrop->SetVisibility(ESlateVisibility::Collapsed);

	UBorder* MenuBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendMenuBox"));
	MenuBox->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	MenuBox->SetPadding(FMargin(32.0f, 26.0f));
	MenuBackdrop->AddChild(MenuBox);

	UVerticalBox* MenuColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendMenuColumn"));
	MenuBox->AddChild(MenuColumn);

	MenuTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendMenuTitle"));
	MenuTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	MenuTitle->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo TitleFont = MenuTitle->GetFont();
		TitleFont.Size = 26;
		MenuTitle->SetFont(TitleFont);
	}
	if (UVerticalBoxSlot* TitleSlot = MenuColumn->AddChildToVerticalBox(MenuTitle))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	auto AddMenuButton = [this, MenuColumn](const TCHAR* LabelText, bool bPrimary)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(340.0f);
		Size->SetHeightOverride(56.0f);
		if (UVerticalBoxSlot* SizeSlot = MenuColumn->AddChildToVerticalBox(Size))
		{
			SizeSlot->SetHorizontalAlignment(HAlign_Center);
			SizeSlot->SetPadding(FMargin(0.0f, 5.0f));
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Size->AddChild(Button);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(LabelText));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(bPrimary
			? FLinearColor(0.02f, 0.02f, 0.02f)
			: FLinearColor(0.55f, 0.55f, 0.58f)));
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 22;
			Label->SetFont(LabelFont);
		}
		Button->AddChild(Label);

		return Button;
	};

	// "发送私聊" is not implemented yet: shown but disabled.
	UButton* ChatButton = AddMenuButton(TEXT("\u53D1\u9001\u79C1\u804A"), false);
	ChatButton->SetIsEnabled(false);

	MenuDissolveButton = AddMenuButton(TEXT("\u89E3\u9664\u5173\u7CFB"), true);
	MenuDissolveButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleMenuDissolveClicked);
	MenuDissolveLabel = Cast<UTextBlock>(MenuDissolveButton->GetChildAt(0));

	MenuDeleteButton = AddMenuButton(TEXT("\u5220\u9664\u597D\u53CB"), true);
	MenuDeleteButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleMenuDeleteClicked);
	MenuDeleteLabel = Cast<UTextBlock>(MenuDeleteButton->GetChildAt(0));

	UButton* CancelButton = AddMenuButton(TEXT("\u53D6\u6D88"), true);
	CancelButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleMenuCancelClicked);

	// --------------------------------------------------------- confirm dialog
	ConfirmBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendConfirmBackdrop"));
	ConfirmBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	ConfirmBackdrop->SetHorizontalAlignment(HAlign_Center);
	ConfirmBackdrop->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* ConfirmSlot = RootCanvas->AddChildToCanvas(ConfirmBackdrop))
	{
		ConfirmSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ConfirmSlot->SetOffsets(FMargin(0.0f));
		ConfirmSlot->SetZOrder(60);
	}
	ConfirmBackdrop->SetVisibility(ESlateVisibility::Collapsed);

	UBorder* ConfirmBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FriendConfirmBox"));
	ConfirmBox->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	ConfirmBox->SetPadding(FMargin(40.0f, 32.0f));
	ConfirmBackdrop->AddChild(ConfirmBox);

	UVerticalBox* ConfirmColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendConfirmColumn"));
	ConfirmBox->AddChild(ConfirmColumn);

	ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FriendConfirmText"));
	ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	ConfirmText->SetJustification(ETextJustify::Center);
	ConfirmText->SetAutoWrapText(true);
	ConfirmText->SetWrapTextAt(760.0f);
	{
		FSlateFontInfo ConfirmFont = ConfirmText->GetFont();
		ConfirmFont.Size = 26;
		ConfirmText->SetFont(ConfirmFont);
	}
	if (UVerticalBoxSlot* ConfirmTextSlot = ConfirmColumn->AddChildToVerticalBox(ConfirmText))
	{
		ConfirmTextSlot->SetHorizontalAlignment(HAlign_Center);
		ConfirmTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	UHorizontalBox* ConfirmButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* ConfirmRowSlot = ConfirmColumn->AddChildToVerticalBox(ConfirmButtons))
	{
		ConfirmRowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	auto AddConfirmButton = [this, ConfirmButtons](const TCHAR* LabelText)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(160.0f);
		Size->SetHeightOverride(56.0f);
		if (UHorizontalBoxSlot* SizeSlot = ConfirmButtons->AddChildToHorizontalBox(Size))
		{
			SizeSlot->SetPadding(FMargin(12.0f, 0.0f));
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Size->AddChild(Button);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(LabelText));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 22;
			Label->SetFont(LabelFont);
		}
		Button->AddChild(Label);

		return Button;
	};

	AddConfirmButton(TEXT("\u662F"))->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleConfirmYesClicked);
	AddConfirmButton(TEXT("\u5426"))->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleConfirmNoClicked);
}

void UMyBondageLifeFriendListWidget::OpenFriendMenu(const FString& InAccountName, const FString& InContractId, EMyBondageLifeContractRole InRole)
{
	if (!MenuBackdrop)
	{
		return;
	}

	SelectedFriendAccount = InAccountName;
	SelectedFriendContractId = InContractId;
	SelectedFriendRole = InRole;

	FString DisplayName = InAccountName;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			FMyBondageLifeFriendEntry Entry;
			if (Account->GetFriendEntry(InAccountName, Entry) && !Entry.CharacterName.IsEmpty())
			{
				DisplayName = Entry.CharacterName;
			}
		}
	}

	if (MenuTitle)
	{
		MenuTitle->SetText(FText::FromString(DisplayName));
	}

	// A formal contract can only be dissolved by the master.
	const bool bHasContract = InRole != EMyBondageLifeContractRole::None;
	const bool bCanDissolve = bHasContract
		&& InRole != EMyBondageLifeContractRole::Slave;

	if (MenuDissolveButton)
	{
		MenuDissolveButton->SetIsEnabled(bCanDissolve);
	}
	if (MenuDissolveLabel)
	{
		MenuDissolveLabel->SetColorAndOpacity(FSlateColor(bCanDissolve
			? FLinearColor(0.02f, 0.02f, 0.02f)
			: FLinearColor(0.62f, 0.62f, 0.65f)));
	}

	// A formal contract's slave may not break the relation at all, deleting the
	// friend included.
	const bool bCanDelete = InRole != EMyBondageLifeContractRole::Slave;
	if (MenuDeleteButton)
	{
		MenuDeleteButton->SetIsEnabled(bCanDelete);
	}
	if (MenuDeleteLabel)
	{
		MenuDeleteLabel->SetColorAndOpacity(FSlateColor(bCanDelete
			? FLinearColor(0.02f, 0.02f, 0.02f)
			: FLinearColor(0.62f, 0.62f, 0.65f)));
	}

	MenuBackdrop->SetVisibility(ESlateVisibility::Visible);
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleMenuChatClicked()
{
	// Private chat is not implemented yet.
}

void UMyBondageLifeFriendListWidget::HandleMenuDissolveClicked()
{
	if (!ConfirmBackdrop || !ConfirmText)
	{
		return;
	}

	PendingConfirmAction = 1;
	const bool bTrial = SelectedFriendRole == EMyBondageLifeContractRole::TemporaryMaster
		|| SelectedFriendRole == EMyBondageLifeContractRole::TemporarySlave;

	ConfirmText->SetText(FText::FromString(bTrial
		? TEXT("\u662F\u5426\u786E\u8BA4\u89E3\u9664\u53CC\u65B9\u7684\u4E34\u65F6\u4E3B\u5974\u5951\u7EA6")   // 是否确认解除双方的临时主奴契约
		: TEXT("\u662F\u5426\u786E\u8BA4\u89E3\u9664\u53CC\u65B9\u7684\u4E3B\u5974\u5951\u7EA6")));        // 是否确认解除双方的主奴契约

	ConfirmBackdrop->SetVisibility(ESlateVisibility::Visible);
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleMenuDeleteClicked()
{
	if (!ConfirmBackdrop || !ConfirmText)
	{
		return;
	}

	PendingConfirmAction = 2;
	ConfirmText->SetText(FText::FromString(FString::Printf(
		TEXT("\u662F\u5426\u786E\u8BA4\u5220\u9664\u597D\u53CB[%s]\uFF1F\u53CC\u65B9\u7684\u4E3B\u5974\u5951\u7EA6\u5C06\u4E00\u5E76\u89E3\u9664"),
		*SelectedFriendAccount)));

	ConfirmBackdrop->SetVisibility(ESlateVisibility::Visible);
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleMenuCancelClicked()
{
	if (MenuBackdrop)
	{
		MenuBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleConfirmYesClicked()
{
	const int32 Action = PendingConfirmAction;
	PendingConfirmAction = 0;

	if (ConfirmBackdrop)
	{
		ConfirmBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (Account)
	{
		if (Action == 1)
		{
			Account->DissolveContract(SelectedFriendContractId);
		}
		else if (Action == 2)
		{
			Account->RemoveFriendOnServer(SelectedFriendAccount);
		}
	}

	if (MenuBackdrop)
	{
		MenuBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshFriends();
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::HandleConfirmNoClicked()
{
	PendingConfirmAction = 0;
	if (ConfirmBackdrop)
	{
		ConfirmBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}
	UpdateModalState();
}

void UMyBondageLifeFriendListWidget::ShowContractRequest(const FString& RequestId, const FString& RequesterCharacterName, bool bFormal)
{
	if (!RequestBackdrop || !RequestText)
	{
		return;
	}

	PendingContractRequestId = RequestId;
	bPendingContractFormal = bFormal;
	bRequestDialogIsContract = true;

	RequestText->SetWrapTextAt(860.0f);

	if (bFormal)
	{
		// "[x]邀请你正式成为其永久奴隶，若答应该契约，则只能由对方主动解除该主奴契约，是否同意？"
		RequestText->SetText(FText::FromString(FString::Printf(
			TEXT("[%s]\u9080\u8BF7\u4F60\u6B63\u5F0F\u6210\u4E3A\u5176\u6C38\u4E45\u5974\u96B6\uFF0C\u82E5\u7B54\u5E94\u8BE5\u5951\u7EA6\uFF0C\u5219\u53EA\u80FD\u7531\u5BF9\u65B9\u4E3B\u52A8\u89E3\u9664\u8BE5\u4E3B\u5974\u5951\u7EA6\uFF0C\u662F\u5426\u540C\u610F\uFF1F"),
			*RequesterCharacterName)));
	}
	else
	{
		// "[x]申请你尝试成为它的奴隶。试用期为期三日……是否同意？"
		RequestText->SetText(FText::FromString(FString::Printf(
			TEXT("[%s]\u7533\u8BF7\u4F60\u5C1D\u8BD5\u6210\u4E3A\u5B83\u7684\u5974\u96B6\u3002\u8BD5\u7528\u671F\u4E3A\u671F\u4E09\u65E5\uFF0C\u8BD5\u7528\u671F\u5185\u53EF\u4EE5\u968F\u65F6\u89E3\u9664\u5173\u7CFB\uFF0C\u4E09\u65E5\u540E\u53CC\u65B9\u53EF\u518D\u6B21\u534F\u5546\u7B7E\u8BA2\u6C38\u4E45\u7684\u5974\u96B6\u5951\u7EA6\uFF0C\u662F\u5426\u540C\u610F\uFF1F"),
			*RequesterCharacterName)));
	}

	RequestBackdrop->SetVisibility(ESlateVisibility::Visible);
	UpdateModalState();
}

void UMyBondageLifeFriendRowProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->OpenFriendMenu(Account, ContractId, Role);
	}
}

bool UMyBondageLifeFriendListWidget::TryAdoptBlueprintLayout()
{
	if (!WidgetTree)
	{
		return false;
	}

	PanelBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_Panel")));
	FriendListBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("W_List")));
	EmptyHint = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_EmptyHint")));

	if (!PanelBorder)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("FriendList: no W_Panel, using the code layout."));
		return false;
	}

	if (PanelBorder)
	{
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	// The bottom HUD entry stays under code control (same spot as before).
	if (!OpenButton)
	{
		BuildEntryButtonOnly();
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("FriendList: using the designed layout."));
	return true;
}
void UMyBondageLifeFriendListWidget::BuildEntryButtonOnly()
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	if (!Canvas || OpenButton)
	{
		return;
	}

	OpenButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FriendListButton"));
	FButtonStyle Transparent;
	Transparent.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
	Transparent.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
	Transparent.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
	Transparent.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
	OpenButton->SetStyle(Transparent);
	OpenButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeFriendListWidget::HandleOpenButtonClicked);

	if (UCanvasPanelSlot* EntrySlot = Canvas->AddChildToCanvas(OpenButton))
	{
		EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		EntrySlot->SetPosition(FVector2D(FriendEntryPositionX, FriendEntryPositionY));
		EntrySlot->SetAutoSize(true);
		EntrySlot->SetZOrder(30);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OpenButton->AddChild(Column);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	IconSize->SetWidthOverride(FriendEntryIconSize);
	IconSize->SetHeightOverride(FriendEntryIconSize);
	if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_FriendIcon.UI_FriendIcon")))
	{
		Icon->SetBrushFromTexture(Texture, true);
	}
	IconSize->AddChild(Icon);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09")));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Label->SetJustification(ETextJustify::Center);
	Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	{
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 16;
		Label->SetFont(LabelFont);
	}
	if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
	}
}
