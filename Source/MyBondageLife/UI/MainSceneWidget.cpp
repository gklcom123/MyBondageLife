// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainSceneWidget.h"

#include "Components/MultiLineEditableTextBox.h"




#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"



#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "MyBondageLifeUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "MyBondageLifePlayerController.h"
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

#include "UI/MyBondageLifePasswordDialog.h"



static FString ReadInputText(UWidget* W)
{
	if (UEditableTextBox* Single = Cast<UEditableTextBox>(W)) { return Single->GetText().ToString(); }
	if (UMultiLineEditableTextBox* Multi = Cast<UMultiLineEditableTextBox>(W)) { return Multi->GetText().ToString(); }
	return FString();
}

static void SetInputText(UWidget* W, const FString& Value)
{
	if (UEditableTextBox* Single = Cast<UEditableTextBox>(W)) { Single->SetText(FText::FromString(Value)); return; }
	if (UMultiLineEditableTextBox* Multi = Cast<UMultiLineEditableTextBox>(W)) { Multi->SetText(FText::FromString(Value)); }
}
static UWidget* FindWidgetDeep(UWidgetTree* Tree, const TCHAR* Name);



static UWidget* FindWidgetDeep(UWidgetTree* Tree, const TCHAR* Name)
{
	if (!Tree) { return nullptr; }
	TArray<UWidget*> All;
	Tree->GetAllWidgets(All);
	for (UWidget* W : All)
	{
		if (W && W->GetName() == Name) { return W; }
	}
	return nullptr;
}


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
		// 不再创建第二套"正在加入房间"面板：连接进度统一由 RoomSubsystem 的 Slate 覆层显示（带倒计时）。
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
		TitleFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
		AccountFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
	UButton* SettingsButton = CreateMenuButton(CenterBox, TEXT("\u6E38\u620F\u8BBE\u7F6E"), MenuButtonHeight, true);
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
		LabelFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
		HeaderFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
			TextFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
		HintFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
		FooterFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
		Font.FontObject = MyBondageLifeUI::LoadUIFont();
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
		Font.FontObject = MyBondageLifeUI::LoadUIFont();
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
		Font.FontObject = MyBondageLifeUI::LoadUIFont();
		CancelLabel->SetFont(Font);
	}
	CancelButton->AddChild(CancelLabel);
}

void UMainSceneWidget::ShowConnecting()
{
	// 统一由 UMyBondageLifeRoomSubsystem 的 Slate 覆层显示连接进度（带 45 秒倒计时），
	// 这里不再弹自己的 UMG 面板，避免两套等待界面互相覆盖。
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

	FString RoomName = Account ? Account->GetDefaultRoomName() : FString(TEXT("Room"));
	if (Session->GetRoomSettings().Name.IsEmpty() == false) { RoomName = Session->GetRoomSettings().Name; }

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
		if (Room.bIsFull) { RowButton->SetIsEnabled(false); }

		UTextBlock* RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RowLabel->SetText(FText::FromString(Room.ToDisplayString()));
		RowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		RowLabel->SetJustification(ETextJustify::Left);
		{
			FSlateFontInfo RowFont = RowLabel->GetFont();
			RowFont.Size = 20;
			RowFont.FontObject = MyBondageLifeUI::LoadUIFont();
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

	if (Room.bIsFull)
	{
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("[房间] 房间已满")); }
		return;
	}
	if (Room.bHasPassword)
	{
		PendingOnlineRoomIndex = RoomIndex;
		if (!OnlineRoomPasswordDialog)
		{
			OnlineRoomPasswordDialog = CreateWidget<UMyBondageLifePasswordDialog>(this, UMyBondageLifePasswordDialog::StaticClass());
			if (OnlineRoomPasswordDialog)
			{
				OnlineRoomPasswordDialog->OnConfirmed.AddUniqueDynamic(this, &UMainSceneWidget::HandleOnlineRoomPasswordConfirmed);
				OnlineRoomPasswordDialog->OnCancelled.AddUniqueDynamic(this, &UMainSceneWidget::HandleOnlineRoomPasswordCancelled);
				OnlineRoomPasswordDialog->AddToViewport(120);
			}
		}
		return;
	}
	Rooms->JoinRoom(Room.RoomId);
}

void UMainSceneWidget::HandleCreateRoomClicked()
{
	OpenRoomInfoPanel(false);
}

void UMainSceneWidget::HandleJoinGameClicked()
{
	OpenRoomBrowser();
}

void UMainSceneWidget::HandleSettingsClicked()
{
	if (AMyBondageLifePlayerController* PC = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		PC->ToggleSettings();
	}
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
	// Stays true until travel succeeds or OnRoomsFailed fires. Resetting it here
	// (before ClientTravel) lets a second click tear down the pending EOS
	// connection while ICE is still checking.
	if (!bJoinInProgress)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("MainScene: duplicate room-joined event ignored."));
		return;
	}

	// 等待框保持到 travel 成功或失败（成功时地图切换会销毁本控件）。
	CloseRoomBrowser();

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	if (!Session)
	{
		bJoinInProgress = false;
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: session subsystem unavailable."));
		return;
	}

	if (!Session->JoinServerRoom(Room))
	{
		bJoinInProgress = false;
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: JoinServerRoom rejected the travel."));
	}
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
	FString RoomName = Account ? Account->GetDefaultRoomName() : FString(TEXT("Room"));
	if (Session->GetRoomSettings().Name.IsEmpty() == false) { RoomName = Session->GetRoomSettings().Name; }

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
	// 按自己的 UID 过滤：黑名单永不显示；管理员/白名单模式仅对名单内可见。
	{
		FString MyUid;
		FString MyAccount;
		if (const UMyBondageLifeAccountSubsystem* SelfAccount = GameInstance ? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			MyUid = SelfAccount->GetOwnUid();
			MyAccount = SelfAccount->GetAccountName();
		}

		auto HasUid = [](const FString& Csv, const FString& Uid)
		{
			if (Uid.IsEmpty() || Csv.IsEmpty()) { return false; }
			TArray<FString> Parts;
			Csv.ParseIntoArray(Parts, TEXT(","), true);
			for (FString& P : Parts) { P.TrimStartAndEndInline(); if (P == Uid) { return true; } }
			return false;
		};

		for (int32 Index = NewRooms.Num() - 1; Index >= 0; --Index)
		{
			const FMyBondageLifeRoomInfo& R = NewRooms[Index];
			const bool bHost = !MyAccount.IsEmpty() && R.HostAccount.Equals(MyAccount, ESearchCase::IgnoreCase);

			if (!MyUid.IsEmpty() && HasUid(R.Blacklist, MyUid)) { NewRooms.RemoveAt(Index); continue; }
			if (bHost) { continue; }

			if (R.VisitMode == 1 && !HasUid(R.Admins, MyUid)) { NewRooms.RemoveAt(Index); continue; }
			if (R.VisitMode == 2 && !HasUid(R.Admins, MyUid) && !HasUid(R.Whitelist, MyUid)) { NewRooms.RemoveAt(Index); continue; }
		}
	}
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
		if (Room.bIsFull) { RowButton->SetIsEnabled(false); }

		UTextBlock* RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RowLabel->SetText(FText::FromString(Room.ToDisplayString()));
		RowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
		RowLabel->SetJustification(ETextJustify::Left);
		{
			FSlateFontInfo RowFont = RowLabel->GetFont();
			RowFont.Size = 20;
			RowFont.FontObject = MyBondageLifeUI::LoadUIFont();
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

	const int32 MaxAllowed = Room.MaxPlayersSetting > 0 ? Room.MaxPlayersSetting : Room.MaxPlayers;
	if (MaxAllowed > 0 && Room.CurrentPlayers >= MaxAllowed)
	{
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("[房间] 房间已满")); }
		return;
	}

	if (Room.bHasPassword)
	{
		PendingLocalRoomIndex = RoomIndex;
		if (!LocalRoomPasswordDialog)
		{
			LocalRoomPasswordDialog = CreateWidget<UMyBondageLifePasswordDialog>(this, UMyBondageLifePasswordDialog::StaticClass());
			if (LocalRoomPasswordDialog)
			{
				LocalRoomPasswordDialog->OnConfirmed.AddUniqueDynamic(this, &UMainSceneWidget::HandleLocalRoomPasswordConfirmed);
				LocalRoomPasswordDialog->OnCancelled.AddUniqueDynamic(this, &UMainSceneWidget::HandleLocalRoomPasswordCancelled);
				LocalRoomPasswordDialog->AddToViewport(120);
			}
		}
		return;
	}

	Session->JoinRoom(Room);
}

void UMainSceneWidget::HandleCreateLocalRoomClicked()
{
	OpenRoomInfoPanel(true);
}

void UMainSceneWidget::HandleJoinLocalRoomClicked()
{
	OpenLocalRoomBrowser();
}



// ---------------------------------------------------------------------------
// 房间信息面板
// ---------------------------------------------------------------------------
namespace
{
	FString TrimField(const FString& In) { return In.TrimStartAndEnd(); }

	bool IsDigitsOnly(const FString& In)
	{
		if (In.IsEmpty()) { return false; }
		for (const TCHAR Ch : In)
		{
			if (Ch < '0' || Ch > '9') { return false; }
		}
		return true;
	}

	// 返回规范化后的列表（去空格）；格式非法返回 false。
	bool NormalizeUidList(const FString& In, FString& Out)
	{
	// 只允许数字与英文逗号；忽略空白/换行（多行输入）；连续逗号或其它字符判为非法。
	Out.Reset();
	bool bExpectDigit = true;
	bool bAny = false;
	for (const TCHAR Ch : In)
	{
		if (Ch == TEXT(' ') || Ch == TEXT('\t') || Ch == TEXT('\r') || Ch == TEXT('\n')) { continue; }
		if (Ch == TEXT(','))
		{
			if (bExpectDigit) { return false; }
			Out += TEXT(",");
			bExpectDigit = true;
			continue;
		}
		if (Ch < '0' || Ch > '9') { return false; }
		Out += Ch;
		bExpectDigit = false;
		bAny = true;
	}
	if (!bAny) { return true; }
	if (bExpectDigit) { return false; }
	return true;
	}
}

void UMainSceneWidget::ShowRoomInfoError(const FString& Message)
{
	UE_LOG(LogMyBondageLife, Warning, TEXT("RoomInfo: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, FString::Printf(TEXT("[房间信息] %s"), *Message));
	}
}


void UMainSceneWidget::HandleVisitModeAll(bool bChecked)
{
	if (!bChecked || !RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	if (UCheckBox* Admin = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin")))) { Admin->SetIsChecked(false); }
	if (UCheckBox* White = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist")))) { White->SetIsChecked(false); }
}

void UMainSceneWidget::HandleVisitModeAdmin(bool bChecked)
{
	if (!bChecked || !RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	if (UCheckBox* All = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All")))) { All->SetIsChecked(false); }
	if (UCheckBox* White = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist")))) { White->SetIsChecked(false); }
}

void UMainSceneWidget::HandleVisitModeWhitelist(bool bChecked)
{
	if (!bChecked || !RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	if (UCheckBox* All = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All")))) { All->SetIsChecked(false); }
	if (UCheckBox* Admin = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin")))) { Admin->SetIsChecked(false); }
}

void UMainSceneWidget::BindVisitModeCheckboxes()
{
	if (!RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	if (UCheckBox* All = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All"))))
	{
		All->OnCheckStateChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleVisitModeAll);
	}
	if (UCheckBox* Admin = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin"))))
	{
		Admin->OnCheckStateChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleVisitModeAdmin);
	}
	if (UCheckBox* White = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist"))))
	{
		White->OnCheckStateChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleVisitModeWhitelist);
	}
}

void UMainSceneWidget::HandleRoomsUpdatedForInfo(const TArray<FMyBondageLifeServerRoom>& Rooms)
{
	FString MyAccount;
	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		MyAccount = Account->GetAccountName();
	}

	for (const FMyBondageLifeServerRoom& Room : Rooms)
	{
		if (!MyAccount.IsEmpty() && Room.HostAccount.Equals(MyAccount, ESearchCase::IgnoreCase))
		{
			if (UMyBondageLifeRoomSubsystem* RoomsSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeRoomSubsystem>() : nullptr)
			{
				RoomsSub->FetchRoomDetails(Room.RoomId);
			}
			return;
		}
	}
}

void UMainSceneWidget::HandleRoomDetailsForInfo(const FMyBondageLifeServerRoom& Room)
{
	FillRoomInfoFromRoom(Room);
}

void UMainSceneWidget::FillRoomInfoFromRoom(const FMyBondageLifeServerRoom& Room)
{
	if (!RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	auto SetText = [this](const TCHAR* Name, const FString& Value)
	{
		if (UWidget* Box = FindWidgetDeep(RoomInfoWidget->WidgetTree, Name))
		{
			SetInputText(Box, Value);
		}
	};

	SetText(TEXT("W_Room_Input_RoomName"), Room.RoomName);
	SetText(TEXT("W_Room_Input_RoomDesc"), Room.Description);
	SetText(TEXT("W_Room_Input_RoomMaxPlayerNum"), FString::FromInt(Room.MaxPlayers));
	SetText(TEXT("W_Room_RoomAdminInput"), FString::Join(Room.Admins, TEXT(",")));
	SetText(TEXT("W_Room_RoomWhitelistInput"), FString::Join(Room.Whitelist, TEXT(",")));
	SetText(TEXT("W_Room_RoomBlacklistInput"), FString::Join(Room.Blacklist, TEXT(",")));

	if (UComboBoxString* LanguageBox = Cast<UComboBoxString>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_RoomLanguageComboBox"))))
	{
		FString Label = TEXT("中文");
		if (Room.Language == TEXT("en")) { Label = TEXT("English"); }
		else if (Room.Language == TEXT("ko")) { Label = TEXT("한국어"); }
		else if (Room.Language == TEXT("ja")) { Label = TEXT("日本語"); }
		if (LanguageBox->GetOptionCount() == 0)
		{
			LanguageBox->AddOption(TEXT("中文"));
			LanguageBox->AddOption(TEXT("English"));
			LanguageBox->AddOption(TEXT("한국어"));
			LanguageBox->AddOption(TEXT("日本語"));
		}
		LanguageBox->SetSelectedOption(Label);
	}

	UCheckBox* AllBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All")));
	UCheckBox* AdminBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin")));
	UCheckBox* WhiteBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist")));
	if (AllBox) { AllBox->SetIsChecked(Room.VisitMode == TEXT("all")); }
	if (AdminBox) { AdminBox->SetIsChecked(Room.VisitMode == TEXT("admin")); }
	if (WhiteBox) { WhiteBox->SetIsChecked(Room.VisitMode == TEXT("admin_whitelist")); }
}

namespace
{
	FString RoomInfoCachePath()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RoomInfo.json"));
	}
}

void UMainSceneWidget::SaveRoomInfoCache(const FMyBondageLifeRoomSettings& Settings)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("name"), Settings.Name);
	Root->SetStringField(TEXT("description"), Settings.Description);
	Root->SetStringField(TEXT("language"), Settings.Language);
	Root->SetNumberField(TEXT("maxPlayers"), Settings.MaxPlayers);
	Root->SetNumberField(TEXT("visitMode"), Settings.VisitMode);
	Root->SetStringField(TEXT("admins"), Settings.Admins);
	Root->SetStringField(TEXT("whitelist"), Settings.Whitelist);
	Root->SetStringField(TEXT("blacklist"), Settings.Blacklist);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(Body, *RoomInfoCachePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool UMainSceneWidget::LoadRoomInfoCache(FMyBondageLifeRoomSettings& OutSettings)
{
	FString Body;
	if (!FFileHelper::LoadFileToString(Body, *RoomInfoCachePath())) { return false; }

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) { return false; }

	Root->TryGetStringField(TEXT("name"), OutSettings.Name);
	Root->TryGetStringField(TEXT("description"), OutSettings.Description);
	Root->TryGetStringField(TEXT("language"), OutSettings.Language);
	double Number = 0.0;
	if (Root->TryGetNumberField(TEXT("maxPlayers"), Number)) { OutSettings.MaxPlayers = static_cast<int32>(Number); }
	if (Root->TryGetNumberField(TEXT("visitMode"), Number)) { OutSettings.VisitMode = static_cast<int32>(Number); }
	Root->TryGetStringField(TEXT("admins"), OutSettings.Admins);
	Root->TryGetStringField(TEXT("whitelist"), OutSettings.Whitelist);
	Root->TryGetStringField(TEXT("blacklist"), OutSettings.Blacklist);
	return !OutSettings.Name.IsEmpty();
}

namespace
{
	FString SanitizeUidText(const FString& In)
	{
		FString Out;
		bool bLastSep = false;
		for (const TCHAR Ch : In)
		{
			if (Ch >= 48 && Ch <= 57)
			{
				Out += Ch;
				bLastSep = false;
			}
			else if (Ch == 44 || Ch == 10 || Ch == 13)
			{
				if (!bLastSep && !Out.IsEmpty())
				{
					Out += (Ch == 44) ? TEXT(",") : TEXT("\n");
					bLastSep = true;
				}
			}
		}
		return Out;
	}
}

void UMainSceneWidget::BindUidInputFilters()
{
	if (!RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	if (UEditableTextBox* Box = Cast<UEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomAdminInput")))) { Box->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleAdminInputChanged); }
	else if (UMultiLineEditableTextBox* MBox = Cast<UMultiLineEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomAdminInput")))) { MBox->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleAdminInputChanged); }
	if (UEditableTextBox* Box = Cast<UEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomWhitelistInput")))) { Box->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleWhitelistInputChanged); }
	else if (UMultiLineEditableTextBox* MBox = Cast<UMultiLineEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomWhitelistInput")))) { MBox->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleWhitelistInputChanged); }
	if (UEditableTextBox* Box = Cast<UEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomBlacklistInput")))) { Box->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleBlacklistInputChanged); }
	else if (UMultiLineEditableTextBox* MBox = Cast<UMultiLineEditableTextBox>(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomBlacklistInput")))) { MBox->OnTextChanged.AddUniqueDynamic(this, &UMainSceneWidget::HandleBlacklistInputChanged); }
}

void UMainSceneWidget::HandleAdminInputChanged(const FText& Text)
{
	{
		const FString Clean = SanitizeUidText(Text.ToString());
		UE_LOG(LogMyBondageLife, Log, TEXT("RoomInfo: uid input %s -> %s"), *Text.ToString(), *Clean);
		if (Clean != Text.ToString() && RoomInfoWidget && RoomInfoWidget->WidgetTree) { SetInputText(FindWidgetDeep(RoomInfoWidget->WidgetTree, TEXT("W_Room_RoomAdminInput")), Clean); }
	}
}

void UMainSceneWidget::HandleWhitelistInputChanged(const FText& Text)
{
	if (UEditableTextBox* Box = RoomInfoWidget && RoomInfoWidget->WidgetTree ? Cast<UEditableTextBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_RoomWhitelistInput"))) : nullptr)
	{
		const FString Clean = SanitizeUidText(Text.ToString());
		UE_LOG(LogMyBondageLife, Log, TEXT("RoomInfo: uid input %s -> %s"), *Text.ToString(), *Clean);
		if (Clean != Text.ToString()) { Box->SetText(FText::FromString(Clean)); }
	}
}

void UMainSceneWidget::HandleBlacklistInputChanged(const FText& Text)
{
	if (UEditableTextBox* Box = RoomInfoWidget && RoomInfoWidget->WidgetTree ? Cast<UEditableTextBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_RoomBlacklistInput"))) : nullptr)
	{
		const FString Clean = SanitizeUidText(Text.ToString());
		UE_LOG(LogMyBondageLife, Log, TEXT("RoomInfo: uid input %s -> %s"), *Text.ToString(), *Clean);
		if (Clean != Text.ToString()) { Box->SetText(FText::FromString(Clean)); }
	}
}
void UMainSceneWidget::FillRoomInfoDefaults()
{
	if (!RoomInfoWidget || !RoomInfoWidget->WidgetTree) { return; }
	auto SetText = [this](const TCHAR* Name, const FString& Value)
	{
		if (UWidget* Box = FindWidgetDeep(RoomInfoWidget->WidgetTree, Name))
		{
			SetInputText(Box, Value);
		}
	};

	FMyBondageLifeRoomSettings Settings;
	if (UMyBondageLifeSessionSubsystem* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>() : nullptr)
	{
		Settings = Session->GetRoomSettings();
	}
	if (Settings.Name.IsEmpty())
	{
		LoadRoomInfoCache(Settings);
	}
	if (Settings.Name.IsEmpty())
	{
		FString PlayerName = TEXT("玩家");
		if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			const FString CharacterName = Account->GetCharacterName();
			PlayerName = CharacterName.IsEmpty() ? Account->GetAccountName() : CharacterName;
		}
		Settings.Name = PlayerName + TEXT("的房间");
		Settings.MaxPlayers = 8;
		Settings.Language = TEXT("zh");
		Settings.VisitMode = 0;
	}

	SetText(TEXT("W_Room_Input_RoomName"), Settings.Name);
	SetText(TEXT("W_Room_Input_RoomDesc"), Settings.Description);
	SetText(TEXT("W_Room_Input_RoomSecretWord"), Settings.Password);
	SetText(TEXT("W_Room_Input_RoomMaxPlayerNum"), FString::FromInt(Settings.MaxPlayers));
	SetText(TEXT("W_Room_RoomAdminInput"), Settings.Admins);
	SetText(TEXT("W_Room_RoomWhitelistInput"), Settings.Whitelist);
	SetText(TEXT("W_Room_RoomBlacklistInput"), Settings.Blacklist);

	if (UComboBoxString* LanguageBox = Cast<UComboBoxString>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_RoomLanguageComboBox"))))
	{
		if (LanguageBox->GetOptionCount() == 0)
		{
			LanguageBox->AddOption(TEXT("中文"));
			LanguageBox->AddOption(TEXT("English"));
			LanguageBox->AddOption(TEXT("한국어"));
			LanguageBox->AddOption(TEXT("日本語"));
		}
		FString Label = TEXT("中文");
		if (Settings.Language == TEXT("en")) { Label = TEXT("English"); }
		else if (Settings.Language == TEXT("ko")) { Label = TEXT("한국어"); }
		else if (Settings.Language == TEXT("ja")) { Label = TEXT("日本語"); }
		LanguageBox->SetSelectedOption(Label);
	}

	UCheckBox* AllBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All")));
	UCheckBox* AdminBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin")));
	UCheckBox* WhiteBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist")));
	if (AllBox) { AllBox->SetIsChecked(Settings.VisitMode == 0); }
	if (AdminBox) { AdminBox->SetIsChecked(Settings.VisitMode == 1); }
	if (WhiteBox) { WhiteBox->SetIsChecked(Settings.VisitMode == 2); }
}

bool UMainSceneWidget::ReadRoomInfoSettings(FMyBondageLifeRoomSettings& OutSettings, FString& OutError)
{
	if (!RoomInfoWidget || !RoomInfoWidget->WidgetTree) { OutError = TEXT("面板不可用"); return false; }

	auto GetText = [this](const TCHAR* Name) -> FString
	{
		if (UWidget* Box = FindWidgetDeep(RoomInfoWidget->WidgetTree, Name))
		{
			return TrimField(ReadInputText(Box));
		}
		return FString();
	};

	OutSettings.Name = GetText(TEXT("W_Room_Input_RoomName"));
	if (OutSettings.Name.IsEmpty()) { OutError = TEXT("房间名不能为空"); return false; }
	if (OutSettings.Name.Len() > 10) { OutError = TEXT("房间名不能超过 10 个字符"); return false; }

	OutSettings.Description = GetText(TEXT("W_Room_Input_RoomDesc"));
	if (OutSettings.Description.Len() > 40) { OutError = TEXT("房间描述不能超过 40 个字符"); return false; }

	OutSettings.Password = GetText(TEXT("W_Room_Input_RoomSecretWord"));
	if (!OutSettings.Password.IsEmpty())
	{
		if (!IsDigitsOnly(OutSettings.Password) || OutSettings.Password.Len() > 8)
		{
			OutError = TEXT("密码只能是数字且不超过 8 位");
			return false;
		}
	}

	const FString MaxText = GetText(TEXT("W_Room_Input_RoomMaxPlayerNum"));
	if (!IsDigitsOnly(MaxText)) { OutError = TEXT("人数上限必须是数字"); return false; }
	OutSettings.MaxPlayers = FCString::Atoi(*MaxText);
	if (OutSettings.MaxPlayers < 1 || OutSettings.MaxPlayers > 10)
	{
		OutError = TEXT("人数上限需在 1~10 之间");
		return false;
	}

	if (!NormalizeUidList(GetText(TEXT("W_Room_RoomAdminInput")), OutSettings.Admins))
	{
		OutError = TEXT("管理员列表格式错误（数字+英文逗号）");
		return false;
	}
	if (!NormalizeUidList(GetText(TEXT("W_Room_RoomWhitelistInput")), OutSettings.Whitelist))
	{
		OutError = TEXT("白名单格式错误（数字+英文逗号）");
		return false;
	}
	if (!NormalizeUidList(GetText(TEXT("W_Room_RoomBlacklistInput")), OutSettings.Blacklist))
	{
		OutError = TEXT("黑名单格式错误（数字+英文逗号）");
		return false;
	}

	if (UComboBoxString* LanguageBox = Cast<UComboBoxString>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_RoomLanguageComboBox"))))
	{
		const FString Selected = LanguageBox->GetSelectedOption();
		if (Selected == TEXT("English")) { OutSettings.Language = TEXT("en"); }
		else if (Selected == TEXT("한국어")) { OutSettings.Language = TEXT("ko"); }
		else if (Selected == TEXT("日本語")) { OutSettings.Language = TEXT("ja"); }
		else { OutSettings.Language = TEXT("zh"); }
	}

	UCheckBox* AllBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_All")));
	UCheckBox* AdminBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdmin")));
	UCheckBox* WhiteBox = Cast<UCheckBox>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Room_VisitMode_ModeCheck_OnlyAdminWhitelist")));

	// 互斥：以最后勾选的一项为准（优先级 白名单 > 管理员 > 所有人）。
	const bool bAll = AllBox && AllBox->IsChecked();
	const bool bAdmin = AdminBox && AdminBox->IsChecked();
	const bool bWhite = WhiteBox && WhiteBox->IsChecked();

	if (bWhite) { OutSettings.VisitMode = 2; }
	else if (bAdmin) { OutSettings.VisitMode = 1; }
	else { OutSettings.VisitMode = 0; }

	// 回写互斥状态，保证界面上始终只有一个勾选。
	if (AllBox) { AllBox->SetIsChecked(OutSettings.VisitMode == 0); }
	if (AdminBox) { AdminBox->SetIsChecked(OutSettings.VisitMode == 1); }
	if (WhiteBox) { WhiteBox->SetIsChecked(OutSettings.VisitMode == 2); }

	return true;
}
void UMainSceneWidget::OpenRoomInfoPanel(bool bLocal)
{
	bPendingLocalRoom = bLocal;

	if (!RoomInfoWidget)
	{
		UClass* PanelClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RoomInfo.WBP_RoomInfo_C"));
		if (!PanelClass)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("RoomInfo: WBP_RoomInfo not found"));
			return;
		}
		RoomInfoWidget = CreateWidget<UUserWidget>(this, PanelClass);
		if (!RoomInfoWidget) { return; }
		RoomInfoWidget->AddToViewport(60);

		if (UButton* CloseBtn = Cast<UButton>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Close"))))
		{
			CloseBtn->OnClicked.AddUniqueDynamic(this, &UMainSceneWidget::HandleRoomInfoCloseClicked);
		}
		if (UButton* ConfirmBtn = Cast<UButton>(RoomInfoWidget->WidgetTree->FindWidget(TEXT("W_Confirm"))))
		{
			ConfirmBtn->OnClicked.AddUniqueDynamic(this, &UMainSceneWidget::HandleRoomInfoConfirmClicked);
		}
	}

	RoomInfoWidget->SetVisibility(ESlateVisibility::Visible);
	BindVisitModeCheckboxes();
	BindUidInputFilters();
	FillRoomInfoDefaults();

	if (UMyBondageLifeRoomSubsystem* RoomsSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeRoomSubsystem>() : nullptr)
	{
		RoomsSub->OnRoomsUpdated.AddUniqueDynamic(this, &UMainSceneWidget::HandleRoomsUpdatedForInfo);
		RoomsSub->OnRoomDetails.AddUniqueDynamic(this, &UMainSceneWidget::HandleRoomDetailsForInfo);
		RoomsSub->OnMyRoomSettings.AddUniqueDynamic(this, &UMainSceneWidget::HandleRoomDetailsForInfo);
		RoomsSub->FetchMyRoomSettings();
		RoomsSub->RefreshRooms();
	}
	UE_LOG(LogMyBondageLife, Log, TEXT("RoomInfo: opened for %s room"), bLocal ? TEXT("local") : TEXT("online"));
}

void UMainSceneWidget::HandleRoomInfoCloseClicked()
{
	if (RoomInfoWidget) { RoomInfoWidget->SetVisibility(ESlateVisibility::Collapsed); }
}

void UMainSceneWidget::HandleRoomInfoConfirmClicked()
{
	FMyBondageLifeRoomSettings Settings;
	FString Error;
	if (!ReadRoomInfoSettings(Settings, Error))
	{
		ShowRoomInfoError(Error);
		return;
	}

	const bool bLocal = bPendingLocalRoom;
	if (RoomInfoWidget) { RoomInfoWidget->SetVisibility(ESlateVisibility::Collapsed); }

	if (UMyBondageLifeSessionSubsystem* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>() : nullptr)
	{
		Session->SetRoomSettings(Settings);
		SaveRoomInfoCache(Settings);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("RoomInfo: confirm %s room name=%s max=%d mode=%d"), bLocal ? TEXT("local") : TEXT("online"), *Settings.Name, Settings.MaxPlayers, Settings.VisitMode);
	if (bLocal) { CreateLocalRoom(); } else { CreateRoom(); }
}

void UMainSceneWidget::HandleLocalRoomPasswordConfirmed(const FString& Password)
{
	if (!DisplayedLocalRooms.IsValidIndex(PendingLocalRoomIndex))
	{
		PendingLocalRoomIndex = -1;
		return;
	}

	const FMyBondageLifeRoomInfo Room = DisplayedLocalRooms[PendingLocalRoomIndex];
	PendingLocalRoomIndex = -1;

	if (Room.Password != Password)
	{
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("[房间] 密码错误")); }
		return;
	}

	if (UMyBondageLifeSessionSubsystem* Session = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>() : nullptr)
	{
		Session->JoinRoom(Room);
	}
}

void UMainSceneWidget::HandleLocalRoomPasswordCancelled()
{
	PendingLocalRoomIndex = -1;
}

void UMainSceneWidget::HandleOnlineRoomPasswordConfirmed(const FString& Password)
{
	if (!DisplayedRooms.IsValidIndex(PendingOnlineRoomIndex))
	{
		PendingOnlineRoomIndex = -1;
		return;
	}

	const FMyBondageLifeServerRoom Room = DisplayedRooms[PendingOnlineRoomIndex];
	PendingOnlineRoomIndex = -1;

	if (UMyBondageLifeRoomSubsystem* Rooms = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeRoomSubsystem>() : nullptr)
	{
		Rooms->JoinRoom(Room.RoomId, Password);
	}
}

void UMainSceneWidget::HandleOnlineRoomPasswordCancelled()
{
	PendingOnlineRoomIndex = -1;
}