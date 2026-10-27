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
	constexpr float EntryIconSize = 64.0f;
	constexpr float EntryPositionX = 780.0f;
	constexpr float EntryPositionY = -30.0f;

	constexpr float PanelWidth = 1500.0f;
	constexpr float PanelHeight = 840.0f;
	constexpr float CloseButtonSize = 52.0f;
	constexpr float RowHeight = 56.0f;

	constexpr float NameColumnWidth = 520.0f;
	constexpr float ClassColumnWidth = 360.0f;
	constexpr float StatusColumnWidth = 200.0f;

	/** Row box size and inner text placement (canvas units). */
	constexpr float RowWidth = 16.0f + NameColumnWidth + ClassColumnWidth + StatusColumnWidth;
	constexpr float RowTextOffsetY = 12.0f;
	constexpr float RowTextHeight = 34.0f;
	constexpr float RowPaddingX = 16.0f;

	// "好友列表（V）"
	const TCHAR* EntryLabelText = TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09");
	// "好友列表"
	const TCHAR* PanelTitleText = TEXT("\u597D\u53CB\u5217\u8868");
	// "暂无好友"
	const TCHAR* EmptyHintText = TEXT("\u6682\u65E0\u597D\u53CB");
	// "在线" / "离线"
	const TCHAR* OnlineText = TEXT("\u5728\u7EBF");
	const TCHAR* OfflineText = TEXT("\u79BB\u7EBF");
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

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
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
		EntrySlot->SetPosition(FVector2D(EntryPositionX, EntryPositionY));
		EntrySlot->SetAutoSize(true);
		EntrySlot->SetZOrder(20);
	}

	UVerticalBox* EntryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FriendEntryColumn"));
	OpenButton->AddChild(EntryColumn);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendIconSize"));
	IconSize->SetWidthOverride(EntryIconSize);
	IconSize->SetHeightOverride(EntryIconSize);
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
	EntryLabel->SetText(FText::FromString(EntryLabelText));
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
		PanelSlot->SetSize(FVector2D(PanelWidth, PanelHeight));
		PanelSlot->SetZOrder(30);
	}
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FriendPanelCanvas"));
	PanelBorder->AddChild(PanelCanvas);

	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendCloseSize"));
	CloseSize->SetWidthOverride(CloseButtonSize);
	CloseSize->SetHeightOverride(CloseButtonSize);
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
	Title->SetText(FText::FromString(PanelTitleText));
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
	AddHeaderCell(TEXT("\u89D2\u8272\u540D"), NameColumnWidth);
	AddHeaderCell(TEXT("\u89D2\u8272\u7C7B\u522B"), ClassColumnWidth);
	AddHeaderCell(TEXT("\u72B6\u6001"), 200.0f);

	// Scrollable list
	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FriendListSize"));
	ListSize->SetWidthOverride(PanelWidth - 112.0f);
	ListSize->SetHeightOverride(PanelHeight - 200.0f);
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
	EmptyHint->SetText(FText::FromString(EmptyHintText));
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
		RowSize->SetWidthOverride(RowWidth);
		RowSize->SetHeightOverride(RowHeight);

		// Clickable on purpose; per-friend actions come with the backend.
		UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		RowButton->OnClicked.AddDynamic(this, &UMyBondageLifeFriendListWidget::HandleFriendRowClicked);
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
				LabelSlot->SetPosition(FVector2D(OffsetX, RowTextOffsetY));
				LabelSlot->SetSize(FVector2D(RowWidth - OffsetX, RowTextHeight));
			}
		};

		// 角色名 / 角色类别 / 状态
		const FLinearColor TextColor(0.05f, 0.05f, 0.05f);
		AddCell(RowPaddingX, Friend.CharacterName, TextColor);
		AddCell(RowPaddingX + NameColumnWidth,
			UMyBondageLifeAccountSubsystem::MakeCharacterClassLabel(Friend.CharacterClass), TextColor);
		AddCell(RowPaddingX + NameColumnWidth + ClassColumnWidth,
			Friend.bOnline ? OnlineText : OfflineText,
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

	RequestBackdrop->SetVisibility(ESlateVisibility::Visible);
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
	OnRequestAnswered.Broadcast(true);
}

void UMyBondageLifeFriendListWidget::HandleDeclineClicked()
{
	if (!IsRequestDialogOpen())
	{
		return;
	}

	RequestBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	OnRequestAnswered.Broadcast(false);
}

void UMyBondageLifeFriendListWidget::HandleFriendsChanged()
{
	if (IsPanelOpen())
	{
		RefreshFriends();
	}
}
