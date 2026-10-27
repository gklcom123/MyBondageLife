// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeWardrobeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
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
	constexpr float WardrobeEntryIconSize = 64.0f;
	constexpr float WardrobeEntryPositionX = 930.0f;
	constexpr float WardrobeEntryPositionY = -30.0f;

	constexpr float WardrobePanelWidth = 1500.0f;
	constexpr float WardrobePanelHeight = 840.0f;
	constexpr float WardrobeCloseSize = 52.0f;

	constexpr float TabWidth = 200.0f;
	constexpr float TabHeight = 72.0f;
	constexpr float TabGap = 16.0f;

	/** Height presets: 娇小 / 普通 / 高大. */
	const float HeightPresets[] = { 0.9f, 1.0f, 1.1f };
	const TCHAR* HeightLabels[] = {
		TEXT("\u5A07\u5C0F"),   // 娇小
		TEXT("\u666E\u901A"),   // 普通
		TEXT("\u9AD8\u5927")    // 高大
	};

	const TCHAR* WardrobeEntryText = TEXT("\u8863\u67DC\uFF08B\uFF09");   // 衣柜（B）
	const TCHAR* WardrobeTitleText = TEXT("\u8863\u67DC");                // 衣柜
	const TCHAR* BodyTabText = TEXT("\u8EAB\u4F53");                      // 身体
	const TCHAR* AppearanceTabText = TEXT("\u5916\u89C2");                // 外观
	const TCHAR* RestraintTabText = TEXT("\u62D8\u675F");                 // 拘束
	const TCHAR* HeightCaptionText = TEXT("\u8EAB\u9AD8\uFF1A");          // 身高：
}

UMyBondageLifeWardrobeWidget::UMyBondageLifeWardrobeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Full screen overlay must not swallow clicks meant for the layers below it.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Hard reference so the icon is cooked into packaged builds.
	static ConstructorHelpers::FObjectFinder<UTexture2D> WardrobeIconFinder(
		TEXT("/Game/2DAssest/UI/UI_Wardrobe.UI_Wardrobe"));
	if (WardrobeIconFinder.Succeeded())
	{
		WardrobeIconTexture = WardrobeIconFinder.Object;
	}
}

TSharedRef<SWidget> UMyBondageLifeWardrobeWidget::RebuildWidget()
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

void UMyBondageLifeWardrobeWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WardrobeRoot"));
	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// ------------------------------------------------------------ bottom entry
	OpenButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WardrobeButton"));
	{
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		OpenButton->SetStyle(ButtonStyle);
	}
	OpenButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleOpenButtonClicked);

	if (UCanvasPanelSlot* EntrySlot = RootCanvas->AddChildToCanvas(OpenButton))
	{
		EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		EntrySlot->SetPosition(FVector2D(WardrobeEntryPositionX, WardrobeEntryPositionY));
		EntrySlot->SetAutoSize(true);
		EntrySlot->SetZOrder(20);
	}

	UVerticalBox* EntryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OpenButton->AddChild(EntryColumn);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	IconSize->SetWidthOverride(WardrobeEntryIconSize);
	IconSize->SetHeightOverride(WardrobeEntryIconSize);
	if (UVerticalBoxSlot* IconSlot = EntryColumn->AddChildToVerticalBox(IconSize))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UImage* WardrobeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (WardrobeIconTexture)
	{
		WardrobeIcon->SetBrushFromTexture(WardrobeIconTexture, true);
	}
	IconSize->AddChild(WardrobeIcon);

	UTextBlock* EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	EntryLabel->SetText(FText::FromString(WardrobeEntryText));
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
	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WardrobePanel"));
	PanelBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetSize(FVector2D(WardrobePanelWidth, WardrobePanelHeight));
		PanelSlot->SetZOrder(30);
	}
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WardrobeCanvas"));
	PanelBorder->AddChild(PanelCanvas);

	// Close button
	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CloseSize->SetWidthOverride(WardrobeCloseSize);
	CloseSize->SetHeightOverride(WardrobeCloseSize);
	if (UCanvasPanelSlot* CloseSlot = PanelCanvas->AddChildToCanvas(CloseSize))
	{
		CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CloseSlot->SetPosition(FVector2D(-18.0f, 18.0f));
	}

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloseButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleCloseButtonClicked);
	CloseSize->AddChild(CloseButton);

	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
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
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(WardrobeTitleText));
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
		TitleSlot->SetPosition(FVector2D(56.0f, 40.0f));
		TitleSlot->SetAutoSize(true);
	}

	// --------------------------------------------------------------- tab buttons
	auto AddTab = [this, PanelCanvas](const TCHAR* LabelText, float OffsetX)
	{
		USizeBox* TabSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		TabSize->SetWidthOverride(TabWidth);
		TabSize->SetHeightOverride(TabHeight);
		if (UCanvasPanelSlot* TabSlot = PanelCanvas->AddChildToCanvas(TabSize))
		{
			TabSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			TabSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			TabSlot->SetPosition(FVector2D(OffsetX, 110.0f));
		}

		UButton* TabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		TabSize->AddChild(TabButton);

		UTextBlock* TabLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TabLabel->SetText(FText::FromString(LabelText));
		TabLabel->SetJustification(ETextJustify::Center);
		TabLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		{
			FSlateFontInfo TabFont = TabLabel->GetFont();
			TabFont.Size = 26;
			TabLabel->SetFont(TabFont);
		}
		TabButton->AddChild(TabLabel);

		return TabButton;
	};

	UButton* BodyTab = AddTab(BodyTabText, 56.0f);
	BodyTabLabel = Cast<UTextBlock>(BodyTab->GetChildAt(0));
	BodyTab->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleBodyTabClicked);

	UButton* AppearanceTab = AddTab(AppearanceTabText, 56.0f + TabWidth + TabGap);
	AppearanceTabLabel = Cast<UTextBlock>(AppearanceTab->GetChildAt(0));
	AppearanceTab->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleAppearanceTabClicked);

	UButton* RestraintTab = AddTab(RestraintTabText, 56.0f + (TabWidth + TabGap) * 2.0f);
	RestraintTabLabel = Cast<UTextBlock>(RestraintTab->GetChildAt(0));
	RestraintTab->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleRestraintTabClicked);

	// ------------------------------------------------------------------ pages
	auto AddPage = [this, PanelCanvas](const FName& WidgetName)
	{
		UCanvasPanel* Page = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), WidgetName);
		if (UCanvasPanelSlot* PageSlot = PanelCanvas->AddChildToCanvas(Page))
		{
			PageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			PageSlot->SetOffsets(FMargin(0.0f));
			PageSlot->SetZOrder(5);
		}
		return Page;
	};

	BodyPage = AddPage(TEXT("WardrobeBodyPage"));
	AppearancePage = AddPage(TEXT("WardrobeAppearancePage"));
	RestraintPage = AddPage(TEXT("WardrobeRestraintPage"));

	// Body page: "身高： < 普通 >"
	UTextBlock* HeightCaption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HeightCaption->SetText(FText::FromString(HeightCaptionText));
	HeightCaption->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo CaptionFont = HeightCaption->GetFont();
		CaptionFont.Size = 28;
		HeightCaption->SetFont(CaptionFont);
	}
	if (UCanvasPanelSlot* CaptionSlot = BodyPage->AddChildToCanvas(HeightCaption))
	{
		CaptionSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CaptionSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CaptionSlot->SetPosition(FVector2D(56.0f, 240.0f));
		CaptionSlot->SetAutoSize(true);
	}

	auto AddArrow = [this](const TCHAR* LabelText, float OffsetX)
	{
		USizeBox* ArrowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ArrowSize->SetWidthOverride(56.0f);
		ArrowSize->SetHeightOverride(52.0f);
		if (UCanvasPanelSlot* ArrowSlot = BodyPage->AddChildToCanvas(ArrowSize))
		{
			ArrowSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ArrowSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ArrowSlot->SetPosition(FVector2D(OffsetX, 226.0f));
		}

		UButton* ArrowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ArrowSize->AddChild(ArrowButton);

		UTextBlock* ArrowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ArrowLabel->SetText(FText::FromString(LabelText));
		ArrowLabel->SetJustification(ETextJustify::Center);
		ArrowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		{
			FSlateFontInfo ArrowFont = ArrowLabel->GetFont();
			ArrowFont.Size = 26;
			ArrowLabel->SetFont(ArrowFont);
		}
		ArrowButton->AddChild(ArrowLabel);

		return ArrowButton;
	};

	UButton* DownButton = AddArrow(TEXT("<"), 190.0f);
	DownButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightDownClicked);

	HeightValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HeightValueLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	HeightValueLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo ValueFont = HeightValueLabel->GetFont();
		ValueFont.Size = 28;
		HeightValueLabel->SetFont(ValueFont);
	}
	if (UCanvasPanelSlot* ValueSlot = BodyPage->AddChildToCanvas(HeightValueLabel))
	{
		ValueSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		ValueSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		ValueSlot->SetPosition(FVector2D(258.0f, 236.0f));
		ValueSlot->SetSize(FVector2D(160.0f, 40.0f));
	}

	UButton* UpButton = AddArrow(TEXT(">"), 430.0f);
	UpButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightUpClicked);

	SelectTab(0);
}

void UMyBondageLifeWardrobeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start from the height the account service already has.
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (Account)
	{
		const float Scale = Account->GetHeightScale();
		if (Scale < 0.95f)
		{
			HeightIndex = 0;
		}
		else if (Scale > 1.05f)
		{
			HeightIndex = 2;
		}
		else
		{
			HeightIndex = 1;
		}
	}

	RefreshHeightLabel();
}

void UMyBondageLifeWardrobeWidget::SelectTab(int32 TabIndex)
{
	// 0 body, 1 appearance, 2 restraint.
	if (BodyPage)
	{
		BodyPage->SetVisibility(TabIndex == 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (AppearancePage)
	{
		AppearancePage->SetVisibility(TabIndex == 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RestraintPage)
	{
		RestraintPage->SetVisibility(TabIndex == 2 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	const FLinearColor ActiveColor(0.05f, 0.05f, 0.05f);
	const FLinearColor InactiveColor(0.55f, 0.55f, 0.58f);

	if (BodyTabLabel) { BodyTabLabel->SetColorAndOpacity(FSlateColor(TabIndex == 0 ? ActiveColor : InactiveColor)); }
	if (AppearanceTabLabel) { AppearanceTabLabel->SetColorAndOpacity(FSlateColor(TabIndex == 1 ? ActiveColor : InactiveColor)); }
	if (RestraintTabLabel) { RestraintTabLabel->SetColorAndOpacity(FSlateColor(TabIndex == 2 ? ActiveColor : InactiveColor)); }
}

void UMyBondageLifeWardrobeWidget::RefreshHeightLabel()
{
	if (!HeightValueLabel)
	{
		return;
	}

	HeightIndex = FMath::Clamp(HeightIndex, 0, 2);
	HeightValueLabel->SetText(FText::FromString(HeightLabels[HeightIndex]));
}

void UMyBondageLifeWardrobeWidget::StepHeight(int32 Delta)
{
	const int32 NewIndex = FMath::Clamp(HeightIndex + Delta, 0, 2);
	if (NewIndex == HeightIndex)
	{
		return;
	}

	HeightIndex = NewIndex;
	RefreshHeightLabel();

	// Persist first; the controller applies the new scale when the service confirms.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->RequestUpdateHeight(HeightPresets[HeightIndex]);
		}
	}
}

float UMyBondageLifeWardrobeWidget::GetDisplayedHeightScale() const
{
	return HeightPresets[FMath::Clamp(HeightIndex, 0, 2)];
}

void UMyBondageLifeWardrobeWidget::SetPanelOpen(bool bOpen)
{
	if (IsPanelOpen() == bOpen)
	{
		return;
	}

	if (PanelBorder)
	{
		PanelBorder->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bOpen)
	{
		SelectTab(0);
		RefreshHeightLabel();
	}

	OnPanelOpenChanged.Broadcast(bOpen);
}

void UMyBondageLifeWardrobeWidget::TogglePanel()
{
	SetPanelOpen(!IsPanelOpen());
}

bool UMyBondageLifeWardrobeWidget::IsPanelOpen() const
{
	return PanelBorder && PanelBorder->GetVisibility() == ESlateVisibility::Visible;
}

void UMyBondageLifeWardrobeWidget::HandleOpenButtonClicked()
{
	SetPanelOpen(true);
}

void UMyBondageLifeWardrobeWidget::HandleCloseButtonClicked()
{
	SetPanelOpen(false);
}

void UMyBondageLifeWardrobeWidget::HandleBodyTabClicked()
{
	SelectTab(0);
}

void UMyBondageLifeWardrobeWidget::HandleAppearanceTabClicked()
{
	SelectTab(1);
}

void UMyBondageLifeWardrobeWidget::HandleRestraintTabClicked()
{
	SelectTab(2);
}

void UMyBondageLifeWardrobeWidget::HandleHeightDownClicked()
{
	StepHeight(-1);
}

void UMyBondageLifeWardrobeWidget::HandleHeightUpClicked()
{
	StepHeight(1);
}
