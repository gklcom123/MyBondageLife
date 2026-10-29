// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSceneWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeEOSSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MyBondageLife.h"
#include "Network/MyBondageLifeLanDiscoverySubsystem.h"
#include "Network/MyBondageLifeRoomSubsystem.h"
#include "Network/MyBondageLifeSessionSubsystem.h"
#include "TimerManager.h"

namespace
{
	constexpr float MenuButtonWidth = 420.0f;
	constexpr float MenuButtonHeight = 64.0f;
	constexpr float RoomBrowserWidth = 900.0f;
	constexpr float RoomListHeight = 360.0f;
	constexpr float RoomBrowserRefreshSeconds = 5.0f;
	constexpr float RoomBrowserLocalRefreshSeconds = 0.5f;
}

// ---------------------------------------------------------------------------
// Row click helper
// ---------------------------------------------------------------------------

void UMyBondageLifeRoomRowAction::HandleClicked()
{
	if (UMainSceneWidget* Owner = OwnerWidget.Get())
	{
		Owner->JoinRoomByIndex(RoomIndex);
	}
}

// ---------------------------------------------------------------------------
// Main scene
// ---------------------------------------------------------------------------

UMainSceneWidget::UMainSceneWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UMainSceneWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
		BuildRoomBrowser();
		BuildConnectingOverlay();
	}

	return Super::RebuildWidget();
}

void UMainSceneWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(0);
	}

	UVerticalBox* CenterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CenterBox"));
	if (UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterBox))
	{
		CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CenterSlot->SetAutoSize(true);
		CenterSlot->SetZOrder(1);
	}

	// Title
	UTextBlock* TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleLabel"));
	TitleLabel->SetText(FText::FromString(TEXT("\u4E3B\u83DC\u5355")));
	TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo TitleFont = TitleLabel->GetFont();
		TitleFont.Size = 48;
		TitleLabel->SetFont(TitleFont);
	}
	if (UVerticalBoxSlot* BoxSlot = CenterBox->AddChildToVerticalBox(TitleLabel))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	// "当前账号：xxx"
	AccountLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AccountLabel"));
	AccountLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.74f, 0.82f)));
	AccountLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo AccountFont = AccountLabel->GetFont();
		AccountFont.Size = 18;
		AccountLabel->SetFont(AccountFont);
	}
	if (UVerticalBoxSlot* BoxSlot = CenterBox->AddChildToVerticalBox(AccountLabel))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 36.0f));
	}

	// 创建房间
	UButton* CreateRoomButton = CreateMenuButton(CenterBox, TEXT("\u521B\u5EFA\u623F\u95F4"), MenuButtonHeight, true);
	CreateRoomButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleCreateRoomClicked);

	// 加入游戏
	UButton* JoinGameButton = CreateMenuButton(CenterBox, TEXT("\u52A0\u5165\u6E38\u620F"), MenuButtonHeight, true);
	JoinGameButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleJoinGameClicked);

	// 创建本地房间（局域网，测试用，不依赖服务器）
	UButton* CreateLocalRoomButton = CreateMenuButton(CenterBox, TEXT("\u521B\u5EFA\u672C\u5730\u623F\u95F4"), MenuButtonHeight, true);
	CreateLocalRoomButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleCreateLocalRoomClicked);

	// 加入本地房间（局域网房间列表）
	UButton* JoinLocalRoomButton = CreateMenuButton(CenterBox, TEXT("\u52A0\u5165\u672C\u5730\u623F\u95F4"), MenuButtonHeight, true);
	JoinLocalRoomButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleJoinLocalRoomClicked);

	// 游戏设置 / 离开游戏 - visible but not implemented yet.
	UButton* SettingsButton = CreateMenuButton(CenterBox, TEXT("\u6E38\u620F\u8BBE\u7F6E"), MenuButtonHeight, false);
	SettingsButton->SetToolTipText(FText::FromString(TEXT("\u5C1A\u672A\u5B9E\u73B0")));
	SettingsButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleSettingsClicked);


	// 离开游戏: closes the game process.
	UButton* QuitButton = CreateMenuButton(CenterBox, TEXT("\u79BB\u5F00\u6E38\u620F"), MenuButtonHeight, true);
	QuitButton->SetToolTipText(FText::FromString(TEXT("\u5173\u95ED\u6E38\u620F\u8FDB\u7A0B")));
	QuitButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleQuitClicked);

	RefreshAccountLabel();
}

UButton* UMainSceneWidget::CreateMenuButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive)
{
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonSize->SetWidthOverride(MenuButtonWidth);
	ButtonSize->SetHeightOverride(Height);

	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(ButtonSize))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetIsEnabled(bInteractive);
	ButtonSize->AddChild(Button);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(LabelText));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(bInteractive
		? FLinearColor(0.02f, 0.02f, 0.02f)
		: FLinearColor(0.45f, 0.45f, 0.48f)));
	{
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 26;
		Label->SetFont(LabelFont);
	}
	Button->AddChild(Label);

	return Button;
}

void UMainSceneWidget::BuildRoomBrowser()
{
	RoomBrowser = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RoomBrowser"));
	RoomBrowser->SetBrushColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.97f));
	RoomBrowser->SetHorizontalAlignment(HAlign_Center);
	RoomBrowser->SetVerticalAlignment(VAlign_Center);

	if (UCanvasPanelSlot* BrowserSlot = RootCanvas->AddChildToCanvas(RoomBrowser))
	{
		BrowserSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BrowserSlot->SetOffsets(FMargin(0.0f));
		BrowserSlot->SetZOrder(10);
	}

	RoomBrowser->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RoomBrowserColumn"));
	RoomBrowser->AddChild(Column);

	// Header: title + refresh + close
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RoomBrowserHeader"));
	if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	RoomBrowserTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoomBrowserTitle"));
	RoomBrowserTitle->SetText(FText::FromString(TEXT("\u623F\u95F4\u5217\u8868")));
	RoomBrowserTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	{
		FSlateFontInfo HeaderFont = RoomBrowserTitle->GetFont();
		HeaderFont.Size = 30;
		RoomBrowserTitle->SetFont(HeaderFont);
	}
	if (UHorizontalBoxSlot* LabelSlot = HeaderRow->AddChildToHorizontalBox(RoomBrowserTitle))
	{
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 40.0f, 0.0f));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	auto AddHeaderButton = [this, HeaderRow](const FString& Label, UButton** OutButton)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(110.0f);
		Size->SetHeightOverride(44.0f);
		if (UHorizontalBoxSlot* SizeSlot = HeaderRow->AddChildToHorizontalBox(Size))
		{
			SizeSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			SizeSlot->SetVerticalAlignment(VAlign_Center);
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Size->AddChild(Button);

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		{
			FSlateFontInfo TextFont = Text->GetFont();
			TextFont.Size = 18;
			Text->SetFont(TextFont);
		}
		Button->AddChild(Text);

		*OutButton = Button;
	};

	UButton* RefreshButton = nullptr;
	UButton* CloseButton = nullptr;
	AddHeaderButton(TEXT("\u5237\u65B0"), &RefreshButton);
	AddHeaderButton(TEXT("\u5173\u95ED"), &CloseButton);
	RefreshButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleRefreshClicked);
	CloseButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleCloseBrowserClicked);

	// Room list
	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RoomListSize"));
	ListSize->SetWidthOverride(RoomBrowserWidth);
	ListSize->SetHeightOverride(RoomListHeight);
	if (UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(ListSize))
	{
		ListSlot->SetHorizontalAlignment(HAlign_Center);
	}

	RoomListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RoomListBox"));
	ListSize->AddChild(RoomListBox);

	// Empty state / hint
	RoomListHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoomListHint"));
	RoomListHint->SetText(FText::FromString(TEXT(
		"\u6682\u65E0\u623F\u95F4\uFF0C\u70B9\u51FB\u300C\u5237\u65B0\u300D\u91CD\u8BD5\u2026")));
	RoomListHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.68f, 0.75f)));
	RoomListHint->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo HintFont = RoomListHint->GetFont();
		HintFont.Size = 20;
		RoomListHint->SetFont(HintFont);
	}
	if (UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(RoomListHint))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Center);
		HintSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	UTextBlock* FooterHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoomBrowserFooter"));
	FooterHint->SetText(FText::FromString(TEXT(
		"\u70B9\u51FB\u623F\u95F4\u5373\u53EF\u52A0\u5165")));
	FooterHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.53f, 0.6f)));
	FooterHint->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo FooterFont = FooterHint->GetFont();
		FooterFont.Size = 16;
		FooterHint->SetFont(FooterFont);
	}
	if (UVerticalBoxSlot* FooterSlot = Column->AddChildToVerticalBox(FooterHint))
	{
		FooterSlot->SetHorizontalAlignment(HAlign_Center);
		FooterSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}
}

void UMainSceneWidget::BuildConnectingOverlay()
{
	ConnectingOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ConnectingOverlay"));
	ConnectingOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	ConnectingOverlay->SetHorizontalAlignment(HAlign_Center);
	ConnectingOverlay->SetVerticalAlignment(VAlign_Center);
	ConnectingOverlay->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(ConnectingOverlay))
	{
		OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		OverlaySlot->SetOffsets(FMargin(0.0f));
		OverlaySlot->SetZOrder(30);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConnectingBox"));
	ConnectingOverlay->AddChild(Box);

	ConnectingLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConnectingLabel"));
	ConnectingLabel->SetText(FText::FromString(TEXT("\u6B63\u5728\u8FDE\u63A5\u623F\u95F4\u2026")));
	ConnectingLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConnectingLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = ConnectingLabel->GetFont();
		Font.Size = 34;
		ConnectingLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* LabelSlot = Box->AddChildToVerticalBox(ConnectingLabel))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Hint->SetText(FText::FromString(TEXT("\u8BF7\u7A0D\u5019\uFF0C\u6700\u591A\u7B49\u5F85 15 \u79D2")));
	Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.74f, 0.82f)));
	Hint->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = Hint->GetFont();
		Font.Size = 18;
		Hint->SetFont(Font);
	}
	if (UVerticalBoxSlot* HintSlot = Box->AddChildToVerticalBox(Hint))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Center);
	}

	USizeBox* CancelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CancelSize->SetWidthOverride(180.0f);
	CancelSize->SetHeightOverride(48.0f);
	if (UVerticalBoxSlot* CancelSlot = Box->AddChildToVerticalBox(CancelSize))
	{
		CancelSlot->SetHorizontalAlignment(HAlign_Center);
		CancelSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
	}

	UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelSize->AddChild(CancelButton);
	CancelButton->OnClicked.AddDynamic(this, &UMainSceneWidget::HandleCancelJoinClicked);

	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelLabel->SetText(FText::FromString(TEXT("\u53D6\u6D88")));
	CancelLabel->SetJustification(ETextJustify::Center);
	CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
	{
		FSlateFontInfo Font = CancelLabel->GetFont();
		Font.Size = 20;
		CancelLabel->SetFont(Font);
	}
	CancelButton->AddChild(CancelLabel);
}

void UMainSceneWidget::ShowConnecting()
{
	if (ConnectingOverlay)
	{
		ConnectingOverlay->SetVisibility(ESlateVisibility::Visible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JoinTimeoutTimer);
		World->GetTimerManager().SetTimer(
			JoinTimeoutTimer,
			FTimerDelegate::CreateUObject(this, &UMainSceneWidget::HandleJoinTimeout),
			15.0f,
			false);
	}
}

void UMainSceneWidget::HideConnecting()
{
	if (ConnectingOverlay)
	{
		ConnectingOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JoinTimeoutTimer);
	}
}

void UMainSceneWidget::HandleJoinTimeout()
{
	if (!bJoinInProgress)
	{
		return;
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("MainScene: join timed out."));

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->CancelJoin();
		}
	}

	HideConnecting();
	bJoinInProgress = false;
	OpenRoomBrowser();

	if (RoomListHint)
	{
		RoomListHint->SetText(FText::FromString(TEXT("\u8FDE\u63A5\u8D85\u65F6\uFF0C\u8BF7\u91CD\u8BD5")));
		RoomListHint->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainSceneWidget::HandleCancelJoinClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->CancelJoin();
		}
	}

	HideConnecting();
	bJoinInProgress = false;
	OpenRoomBrowser();
}

void UMainSceneWidget::RefreshAccountLabel()
{
	if (!AccountLabel)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	const FString AccountName = (Account && Account->IsLoggedIn()) ? Account->GetAccountName() : FString(TEXT("---"));

	// "当前账号：<name>"
	AccountLabel->SetText(FText::FromString(FString::Printf(TEXT("\u5F53\u524D\u8D26\u53F7\uFF1A%s"), *AccountName)));
}

void UMainSceneWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshAccountLabel();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->OnRoomsUpdated.AddDynamic(this, &UMainSceneWidget::HandleRoomsUpdated);
			Rooms->OnRoomsFailed.AddDynamic(this, &UMainSceneWidget::HandleRoomsFailed);
			Rooms->OnRoomJoined.AddDynamic(this, &UMainSceneWidget::HandleRoomJoined);
			Rooms->RefreshRooms();
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("MainScene: menu shown."));
}

void UMainSceneWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoomRefreshTimerHandle);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->OnRoomsUpdated.RemoveDynamic(this, &UMainSceneWidget::HandleRoomsUpdated);
			Rooms->OnRoomsFailed.RemoveDynamic(this, &UMainSceneWidget::HandleRoomsFailed);
			Rooms->OnRoomJoined.RemoveDynamic(this, &UMainSceneWidget::HandleRoomJoined);
		}
	}

	Super::NativeDestruct();
}

void UMainSceneWidget::CreateRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	if (!Session)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: session subsystem unavailable."));
		return;
	}

	UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>();
	UMyBondageLifeEOSSubsystem* EOS = GameInstance->GetSubsystem<UMyBondageLifeEOSSubsystem>();

	if (!EOS || !EOS->IsLoggedIn() || EOS->GetProductUserId().IsEmpty())
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("MainScene: EOS 尚未登录，无法创建房间。"));
		return;
	}

	const FString RoomName = Account ? Account->GetDefaultRoomName() : FString(TEXT("Room"));

	Session->HostRoomWithEOS(RoomName, EOS->GetProductUserId());
}

void UMainSceneWidget::OpenRoomBrowser()
{
	if (!RoomBrowser)
	{
		return;
	}

	BrowserMode = ERoomBrowserMode::Server;
	bJoinInProgress = false;

	if (RoomBrowserTitle)
	{
		// "房间列表"
		RoomBrowserTitle->SetText(FText::FromString(TEXT("\u623F\u95F4\u5217\u8868")));
	}
	if (RoomListHint)
	{
		RoomListHint->SetText(FText::FromString(TEXT("\u6682\u65E0\u623F\u95F4\uFF0C\u70B9\u51FB\u300C\u5237\u65B0\u300D\u91CD\u8BD5\u2026")));
	}

	RoomBrowser->SetVisibility(ESlateVisibility::Visible);
	RebuildRoomRows();
	RefreshRoomList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RoomRefreshTimerHandle,
			FTimerDelegate::CreateUObject(this, &UMainSceneWidget::RefreshRoomList),
			RoomBrowserRefreshSeconds,
			true);
	}
}

void UMainSceneWidget::OpenLocalRoomBrowser()
{
	if (!RoomBrowser)
	{
		return;
	}

	BrowserMode = ERoomBrowserMode::Local;
	bJoinInProgress = false;

	if (RoomBrowserTitle)
	{
		// "本地房间列表"
		RoomBrowserTitle->SetText(FText::FromString(TEXT("\u672C\u5730\u623F\u95F4\u5217\u8868")));
	}
	if (RoomListHint)
	{
		// "暂未发现局域网房间…"
		RoomListHint->SetText(FText::FromString(TEXT("\u6682\u672A\u53D1\u73B0\u5C40\u57DF\u7F51\u623F\u95F4\u2026")));
	}

	RoomBrowser->SetVisibility(ESlateVisibility::Visible);
	RebuildRoomRows();

	if (UWorld* World = GetWorld())
	{
		// LAN discovery updates continuously; just re-read the list often.
		World->GetTimerManager().SetTimer(
			RoomRefreshTimerHandle,
			FTimerDelegate::CreateUObject(this, &UMainSceneWidget::RefreshRoomList),
			RoomBrowserLocalRefreshSeconds,
			true);
	}
}

void UMainSceneWidget::CloseRoomBrowser()
{
	if (RoomBrowser)
	{
		RoomBrowser->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoomRefreshTimerHandle);
	}

	RowActions.Reset();
	DisplayedRooms.Reset();
	DisplayedLocalRooms.Reset();

	if (RoomListBox)
	{
		RoomListBox->ClearChildren();
	}
}

bool UMainSceneWidget::IsRoomBrowserOpen() const
{
	return RoomBrowser && RoomBrowser->GetVisibility() == ESlateVisibility::Visible;
}

void UMainSceneWidget::RebuildRoomRows()
{
	if (!RoomListBox)
	{
		return;
	}

	if (BrowserMode == ERoomBrowserMode::Local)
	{
		RebuildLocalRoomRows();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeRoomSubsystem* Rooms = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>()
		: nullptr;

	TArray<FMyBondageLifeServerRoom> NewRooms;
	if (Rooms)
	{
		NewRooms = Rooms->GetRooms();
	}

	// The list is polled twice per second. Rebuilding rows on every poll would
	// destroy the button under the cursor between mouse down and mouse up, so the
	// click could never complete. Only rebuild when the data actually changed.
	bool bSameAsDisplayed = NewRooms.Num() == DisplayedRooms.Num();
	if (bSameAsDisplayed)
	{
		for (int32 Index = 0; Index < NewRooms.Num(); ++Index)
		{
			if (NewRooms[Index].RoomId != DisplayedRooms[Index].RoomId ||
				NewRooms[Index].RoomName != DisplayedRooms[Index].RoomName ||
				NewRooms[Index].CurrentPlayers != DisplayedRooms[Index].CurrentPlayers)
			{
				bSameAsDisplayed = false;
				break;
			}
		}
	}

	if (bSameAsDisplayed)
	{
		return;
	}

	RoomListBox->ClearChildren();
	RowActions.Reset();
	DisplayedRooms = MoveTemp(NewRooms);

	if (RoomListHint)
	{
		RoomListHint->SetVisibility(DisplayedRooms.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	for (int32 Index = 0; Index < DisplayedRooms.Num(); ++Index)
	{
		const FMyBondageLifeServerRoom& Room = DisplayedRooms[Index];

		UMyBondageLifeRoomRowAction* RowAction = NewObject<UMyBondageLifeRoomRowAction>(this);
		RowAction->OwnerWidget = this;
		RowAction->RoomIndex = Index;
		RowActions.Add(RowAction);

		UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		RowButton->OnClicked.AddDynamic(RowAction, &UMyBondageLifeRoomRowAction::HandleClicked);

		UTextBlock* RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RowLabel->SetText(FText::FromString(Room.ToDisplayString()));
		RowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		RowLabel->SetJustification(ETextJustify::Left);
		{
			FSlateFontInfo RowFont = RowLabel->GetFont();
			RowFont.Size = 20;
			RowLabel->SetFont(RowFont);
		}
		RowButton->AddChild(RowLabel);

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(RoomListBox->AddChild(RowButton)))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
		}
	}
}

void UMainSceneWidget::JoinRoomByIndex(int32 RoomIndex)
{
	if (bJoinInProgress)
	{
		return;
	}

	if (BrowserMode == ERoomBrowserMode::Local)
	{
		JoinLocalRoomByIndex(RoomIndex);
		return;
	}

	if (!DisplayedRooms.IsValidIndex(RoomIndex))
	{
		return;
	}

	const FMyBondageLifeServerRoom Room = DisplayedRooms[RoomIndex];

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeRoomSubsystem* Rooms = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>()
		: nullptr;

	if (!Rooms)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: room subsystem unavailable."));
		return;
	}

	// Only one travel at a time. Every extra ClientTravel leaves an EOS socket
	// behind; its deferred Close() kills the connection that finally succeeded.
	bJoinInProgress = true;
	CloseRoomBrowser();

	Rooms->JoinRoom(Room.RoomId);
}

void UMainSceneWidget::HandleCreateRoomClicked()
{
	CreateRoom();
}

void UMainSceneWidget::HandleJoinGameClicked()
{
	OpenRoomBrowser();
}

void UMainSceneWidget::HandleSettingsClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("MainScene: 游戏设置 is not implemented yet."));
}

void UMainSceneWidget::HandleQuitClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("MainScene: 离开游戏 pressed, closing the game process."));

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainSceneWidget::HandleRefreshClicked()
{
	RefreshRoomList();
}

void UMainSceneWidget::HandleCloseBrowserClicked()
{
	CloseRoomBrowser();
}

void UMainSceneWidget::RefreshRoomList()
{
	if (BrowserMode == ERoomBrowserMode::Local)
	{
		// The discovery subsystem refreshes itself; re-read its list.
		RebuildRoomRows();
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->RefreshRooms();
		}
	}
}

void UMainSceneWidget::HandleRoomsUpdated(const TArray<FMyBondageLifeServerRoom>& Rooms)
{
	RebuildRoomRows();
}

void UMainSceneWidget::HandleRoomsFailed(const FText& ErrorMessage)
{
	UE_LOG(LogMyBondageLife, Warning, TEXT("MainScene: room request failed: %s"), *ErrorMessage.ToString());

	HideConnecting();
	bJoinInProgress = false;

	// The browser is closed while joining; bring it back so the error is visible.
	if (RoomBrowser && RoomBrowser->GetVisibility() != ESlateVisibility::Visible)
	{
		OpenRoomBrowser();
	}

	if (RoomListHint)
	{
		RoomListHint->SetText(ErrorMessage);
		RoomListHint->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainSceneWidget::HandleRoomJoined(const FMyBondageLifeServerRoom& Room)
{
	HideConnecting();
	bJoinInProgress = false;
	CloseRoomBrowser();

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	if (!Session)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: session subsystem unavailable."));
		return;
	}

	Session->JoinServerRoom(Room);
}

// ---------------------------------------------------------------------------
// Local (LAN) rooms - test path, no backend involved
// ---------------------------------------------------------------------------

void UMainSceneWidget::CreateLocalRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	if (!Session)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: session subsystem unavailable."));
		return;
	}

	UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>();
	const FString RoomName = Account ? Account->GetDefaultRoomName() : FString(TEXT("Room"));

	UE_LOG(LogMyBondageLife, Log, TEXT("MainScene: creating a local (LAN) room '%s'."), *RoomName);

	// Listen server + UDP announcement, exactly like the original LAN flow.
	Session->HostRoom(RoomName);
}

void UMainSceneWidget::RebuildLocalRoomRows()
{
	if (!RoomListBox)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>()
		: nullptr;

	TArray<FMyBondageLifeRoomInfo> NewRooms;
	if (Discovery)
	{
		NewRooms = Discovery->GetDiscoveredRooms();
	}

	// Same change detection as the server list: rebuilding every poll would break
	// clicks by destroying the row between mouse down and mouse up.
	bool bSameAsDisplayed = NewRooms.Num() == DisplayedLocalRooms.Num();
	if (bSameAsDisplayed)
	{
		for (int32 Index = 0; Index < NewRooms.Num(); ++Index)
		{
			if (NewRooms[Index].RoomId != DisplayedLocalRooms[Index].RoomId ||
				NewRooms[Index].RoomName != DisplayedLocalRooms[Index].RoomName ||
				NewRooms[Index].CurrentPlayers != DisplayedLocalRooms[Index].CurrentPlayers)
			{
				bSameAsDisplayed = false;
				break;
			}
		}
	}

	if (bSameAsDisplayed)
	{
		return;
	}

	RoomListBox->ClearChildren();
	RowActions.Reset();
	DisplayedLocalRooms = MoveTemp(NewRooms);

	if (RoomListHint)
	{
		RoomListHint->SetVisibility(DisplayedLocalRooms.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	for (int32 Index = 0; Index < DisplayedLocalRooms.Num(); ++Index)
	{
		const FMyBondageLifeRoomInfo& Room = DisplayedLocalRooms[Index];

		UMyBondageLifeRoomRowAction* RowAction = NewObject<UMyBondageLifeRoomRowAction>(this);
		RowAction->OwnerWidget = this;
		RowAction->RoomIndex = Index;
		RowActions.Add(RowAction);

		UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		RowButton->OnClicked.AddDynamic(RowAction, &UMyBondageLifeRoomRowAction::HandleClicked);

		UTextBlock* RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RowLabel->SetText(FText::FromString(Room.ToDisplayString()));
		RowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		RowLabel->SetJustification(ETextJustify::Left);
		{
			FSlateFontInfo RowFont = RowLabel->GetFont();
			RowFont.Size = 20;
			RowLabel->SetFont(RowFont);
		}
		RowButton->AddChild(RowLabel);

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(RoomListBox->AddChild(RowButton)))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
		}
	}
}

void UMainSceneWidget::JoinLocalRoomByIndex(int32 RoomIndex)
{
	if (!DisplayedLocalRooms.IsValidIndex(RoomIndex))
	{
		return;
	}

	const FMyBondageLifeRoomInfo Room = DisplayedLocalRooms[RoomIndex];
	CloseRoomBrowser();

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	if (!Session)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: session subsystem unavailable."));
		return;
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("MainScene: joining local room '%s' at %s."), *Room.RoomName, *Room.GetEndpoint());

	Session->JoinRoom(Room);
}

void UMainSceneWidget::HandleCreateLocalRoomClicked()
{
	CreateLocalRoom();
}

void UMainSceneWidget::HandleJoinLocalRoomClicked()
{
	OpenLocalRoomBrowser();
}

