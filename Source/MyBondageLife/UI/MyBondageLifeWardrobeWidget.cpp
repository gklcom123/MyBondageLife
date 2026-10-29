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
#include "Core/MyBondageLifeHairCatalog.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
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

	constexpr float TabWidth = 230.0f;
	constexpr float TabHeight = 84.0f;
	constexpr float TabGap = 18.0f;

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
			TabSlot->SetPosition(FVector2D(OffsetX, 116.0f));

			// Without auto size the canvas slot collapses and the tab shrinks to its
			// text, which clips the caption.
			TabSlot->SetAutoSize(true);
		}

		UButton* TabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		TabSize->AddChild(TabButton);

		UTextBlock* TabLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TabLabel->SetText(FText::FromString(LabelText));
		TabLabel->SetJustification(ETextJustify::Center);
		TabLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		{
			FSlateFontInfo TabFont = TabLabel->GetFont();
			TabFont.Size = 24;
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

	BuildRestraintSlots();

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
		ArrowSize->SetWidthOverride(76.0f);
		ArrowSize->SetHeightOverride(64.0f);
		if (UCanvasPanelSlot* ArrowSlot = BodyPage->AddChildToCanvas(ArrowSize))
		{
			ArrowSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ArrowSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ArrowSlot->SetPosition(FVector2D(OffsetX, 220.0f));
			ArrowSlot->SetAutoSize(true);
		}

		UButton* ArrowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ArrowSize->AddChild(ArrowButton);

		UTextBlock* ArrowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ArrowLabel->SetText(FText::FromString(LabelText));
		ArrowLabel->SetJustification(ETextJustify::Center);
		ArrowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		{
			FSlateFontInfo ArrowFont = ArrowLabel->GetFont();
			ArrowFont.Size = 30;
			ArrowLabel->SetFont(ArrowFont);
		}
		ArrowButton->AddChild(ArrowLabel);

		return ArrowButton;
	};

	HeightDownButton = AddArrow(TEXT("<"), 180.0f);
	HeightDownButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightDownClicked);
	HeightDownLabel = Cast<UTextBlock>(HeightDownButton->GetChildAt(0));

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
		ValueSlot->SetPosition(FVector2D(276.0f, 232.0f));
		ValueSlot->SetSize(FVector2D(170.0f, 44.0f));
	}

	HeightUpButton = AddArrow(TEXT(">"), 470.0f);
	HeightUpButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightUpClicked);
	HeightUpLabel = Cast<UTextBlock>(HeightUpButton->GetChildAt(0));

	// ------------------------------------------------------- hair row (below)
	UTextBlock* HairCaption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HairCaption->SetText(FText::FromString(TEXT("\u53D1\u578B\uFF1A")));   // 发型：
	HairCaption->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo CaptionFont = HairCaption->GetFont();
		CaptionFont.Size = 28;
		HairCaption->SetFont(CaptionFont);
	}
	if (UCanvasPanelSlot* CaptionSlot = BodyPage->AddChildToCanvas(HairCaption))
	{
		CaptionSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CaptionSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CaptionSlot->SetPosition(FVector2D(56.0f, 350.0f));
		CaptionSlot->SetAutoSize(true);
	}

	auto AddHairArrow = [this](const TCHAR* LabelText, float OffsetX)
	{
		USizeBox* ArrowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ArrowSize->SetWidthOverride(76.0f);
		ArrowSize->SetHeightOverride(64.0f);
		if (UCanvasPanelSlot* ArrowSlot = BodyPage->AddChildToCanvas(ArrowSize))
		{
			ArrowSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ArrowSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ArrowSlot->SetPosition(FVector2D(OffsetX, 330.0f));
			ArrowSlot->SetAutoSize(true);
		}

		UButton* ArrowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ArrowSize->AddChild(ArrowButton);

		UTextBlock* ArrowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ArrowLabel->SetText(FText::FromString(LabelText));
		ArrowLabel->SetJustification(ETextJustify::Center);
		ArrowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		{
			FSlateFontInfo ArrowFont = ArrowLabel->GetFont();
			ArrowFont.Size = 30;
			ArrowLabel->SetFont(ArrowFont);
		}
		ArrowButton->AddChild(ArrowLabel);

		return ArrowButton;
	};

	HairDownButton = AddHairArrow(TEXT("<"), 180.0f);
	HairDownButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHairDownClicked);
	HairDownLabel = Cast<UTextBlock>(HairDownButton->GetChildAt(0));

	HairValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HairValueLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	HairValueLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo ValueFont = HairValueLabel->GetFont();
		ValueFont.Size = 28;
		HairValueLabel->SetFont(ValueFont);
	}
	if (UCanvasPanelSlot* HairValueSlot = BodyPage->AddChildToCanvas(HairValueLabel))
	{
		HairValueSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		HairValueSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		HairValueSlot->SetPosition(FVector2D(276.0f, 342.0f));
		HairValueSlot->SetSize(FVector2D(170.0f, 44.0f));
	}

	HairUpButton = AddHairArrow(TEXT(">"), 470.0f);
	HairUpButton->OnClicked.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHairUpClicked);
	HairUpLabel = Cast<UTextBlock>(HairUpButton->GetChildAt(0));

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

	// Hair: start on the style stored on the account (falls back to the first row).
	HairId = FMyBondageLifeHairCatalog::ClampId(Account ? Account->GetHairIndex() : 1);
	RefreshHairLabel();
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

	RefreshArrowStates();
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
		RefreshRestraintSlots();
		CloseRestraintPicker();
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

void UMyBondageLifeWardrobeWidget::RefreshArrowStates()
{
	// The selector must not leave the allowed range: 0 = 娇小, 2 = 高大.
	const bool bCanGoDown = HeightIndex > 0;
	const bool bCanGoUp = HeightIndex < 2;

	if (HeightDownButton)
	{
		HeightDownButton->SetIsEnabled(bCanGoDown);
	}
	if (HeightUpButton)
	{
		HeightUpButton->SetIsEnabled(bCanGoUp);
	}

	const FLinearColor EnabledColor(0.05f, 0.05f, 0.05f);
	const FLinearColor DisabledColor(0.72f, 0.72f, 0.74f);

	if (HeightDownLabel)
	{
		HeightDownLabel->SetColorAndOpacity(FSlateColor(bCanGoDown ? EnabledColor : DisabledColor));
	}
	if (HeightUpLabel)
	{
		HeightUpLabel->SetColorAndOpacity(FSlateColor(bCanGoUp ? EnabledColor : DisabledColor));
	}
}

void UMyBondageLifeWardrobeWidget::RefreshHairLabel()
{
	if (!HairValueLabel)
	{
		return;
	}

	HairId = FMyBondageLifeHairCatalog::ClampId(HairId);
	HairValueLabel->SetText(FText::FromString(FString::Printf(TEXT("%d \u53F7"), HairId)));   // "N 号"

	RefreshHairArrowStates();
}

void UMyBondageLifeWardrobeWidget::RefreshHairArrowStates()
{
	const int32 NextId = FMyBondageLifeHairCatalog::StepId(HairId, 1);
	const int32 PrevId = FMyBondageLifeHairCatalog::StepId(HairId, -1);

	const bool bCanGoUp = NextId != HairId;
	const bool bCanGoDown = PrevId != HairId;

	if (HairDownButton)
	{
		HairDownButton->SetIsEnabled(bCanGoDown);
	}
	if (HairUpButton)
	{
		HairUpButton->SetIsEnabled(bCanGoUp);
	}

	const FLinearColor EnabledColor(0.05f, 0.05f, 0.05f);
	const FLinearColor DisabledColor(0.72f, 0.72f, 0.74f);

	if (HairDownLabel)
	{
		HairDownLabel->SetColorAndOpacity(FSlateColor(bCanGoDown ? EnabledColor : DisabledColor));
	}
	if (HairUpLabel)
	{
		HairUpLabel->SetColorAndOpacity(FSlateColor(bCanGoUp ? EnabledColor : DisabledColor));
	}
}

void UMyBondageLifeWardrobeWidget::StepHair(int32 Delta)
{
	const int32 NewId = FMyBondageLifeHairCatalog::StepId(HairId, Delta);
	if (NewId == HairId)
	{
		return;
	}

	HairId = NewId;
	RefreshHairLabel();

	// Persist first; the controller republishes the identity when the service answers.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->RequestUpdateHair(HairId);
		}
	}
}

void UMyBondageLifeWardrobeWidget::HandleHairDownClicked()
{
	StepHair(-1);
}

void UMyBondageLifeWardrobeWidget::HandleHairUpClicked()
{
	StepHair(1);
}

// ---------------------------------------------------------------------------
// Restraint slots
// ---------------------------------------------------------------------------

void UMyBondageLifeWardrobeClickProxy::HandleClicked()
{
	if (!Owner)
	{
		return;
	}

	if (bIsItemButton)
	{
		Owner->PickRestraintItem(ItemId);
	}
	else
	{
		Owner->OpenRestraintPicker(Slot);
	}
}

void UMyBondageLifeWardrobeWidget::BuildRestraintSlots()
{
	if (!RestraintPage)
	{
		return;
	}

	SlotButtons.Reset();
	SlotIcons.Reset();
	SlotLabels.Reset();
	ClickProxies.Reset();

	// 4 columns x 3 rows, filled column by column to match the enum order.
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const int32 ColIndex = SlotIndex / 3;
		const int32 Row = SlotIndex % 3;

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CellSize->SetWidthOverride(150.0f);
		CellSize->SetHeightOverride(178.0f);

		if (UCanvasPanelSlot* CellSlot = RestraintPage->AddChildToCanvas(CellSize))
		{
			CellSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			CellSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			CellSlot->SetPosition(FVector2D(56.0f + ColIndex * 185.0f, 30.0f + Row * 200.0f));
			CellSlot->SetAutoSize(true);
		}

		UButton* SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		CellSize->AddChild(SlotButton);
		SlotButtons.Add(SlotButton);

		UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		Proxy->Owner = this;
		Proxy->Slot = SlotIndex;
		Proxy->bIsItemButton = false;
		ClickProxies.Add(Proxy);
		SlotButton->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		SlotButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(150.0f);
		IconSize->SetHeightOverride(150.0f);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		IconSize->AddChild(Icon);
		SlotIcons.Add(Icon);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex)));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		Label->SetJustification(ETextJustify::Center);
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 18;
			Label->SetFont(LabelFont);
		}
		if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
		SlotLabels.Add(Label);
	}

	// --------------------------------------------------------- picker menu
	// A dim overlay with a centred box: the old inline strip sat under the grid and
	// was hard to use.
	PickerBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RestraintPicker"));
	PickerBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	PickerBackdrop->SetHorizontalAlignment(HAlign_Center);
	PickerBackdrop->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* PickerBoxSlot = RestraintPage->AddChildToCanvas(PickerBackdrop))
	{
		PickerBoxSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PickerBoxSlot->SetOffsets(FMargin(0.0f));
		PickerBoxSlot->SetZOrder(20);
	}
	PickerBackdrop->SetVisibility(ESlateVisibility::Collapsed);

	UBorder* PickerBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PickerBox->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	PickerBox->SetPadding(FMargin(28.0f, 22.0f));
	PickerBackdrop->AddChild(PickerBox);

	UVerticalBox* PickerColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PickerBox->AddChild(PickerColumn);

	PickerTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PickerTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo TitleFont = PickerTitle->GetFont();
		TitleFont.Size = 22;
		PickerTitle->SetFont(TitleFont);
	}
	if (UVerticalBoxSlot* TitleSlot = PickerColumn->AddChildToVerticalBox(PickerTitle))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	PickerGrid = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	if (UVerticalBoxSlot* GridSlot = PickerColumn->AddChildToVerticalBox(PickerGrid))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Center);
	}
	PickerGrid->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UMyBondageLifeWardrobeWidget::RefreshRestraintSlots()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	for (int32 SlotIndex = 0; SlotIndex < SlotButtons.Num(); ++SlotIndex)
	{
		const FString ItemId = Account ? Account->GetEquippedRestraint(SlotIndex) : FString();
		const bool bWorn = !ItemId.IsEmpty();

		if (SlotLabels.IsValidIndex(SlotIndex) && SlotLabels[SlotIndex])
		{
			SlotLabels[SlotIndex]->SetText(FText::FromString(
				bWorn ? FMyBondageLifeRestraintCatalog::GetDisplayName(FName(*ItemId))
				      : FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex)));
			SlotLabels[SlotIndex]->SetColorAndOpacity(FSlateColor(
				bWorn ? FLinearColor(0.45f, 0.2f, 0.6f) : FLinearColor(0.35f, 0.35f, 0.38f)));
		}

		if (SlotIcons.IsValidIndex(SlotIndex) && SlotIcons[SlotIndex])
		{
			if (bWorn)
			{
				if (UTexture2D* Icon = FMyBondageLifeRestraintCatalog::LoadIcon(FName(*ItemId)))
				{
					SlotIcons[SlotIndex]->SetBrushFromTexture(Icon, true);
				}
			}
		}
	}
}

void UMyBondageLifeWardrobeWidget::OpenRestraintPicker(int32 SlotIndex)
{
	if (!PickerBackdrop || !PickerGrid || !PickerTitle)
	{
		return;
	}

	PickerSlot = SlotIndex;
	PickerGrid->ClearChildren();
	ClickProxies.RemoveAll([](const TObjectPtr<UMyBondageLifeWardrobeClickProxy>& Proxy)
	{
		return Proxy && Proxy->bIsItemButton;
	});

	PickerTitle->SetText(FText::FromString(FString::Printf(
		TEXT("%s \uFF1A\u9009\u62E9\u62D8\u675F"), *FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex))));

	const TArray<const FMyBondageLifeRestraintItem*> Items = FMyBondageLifeRestraintCatalog::GetItemsForSlot(SlotIndex);

	int32 Index = 0;
	for (const FMyBondageLifeRestraintItem* Item : Items)
	{
		const int32 ColIndex = Index % 4;
		const int32 RowIndex = Index / 4;
		++Index;

		USizeBox* ItemSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ItemSize->SetWidthOverride(150.0f);
		ItemSize->SetHeightOverride(190.0f);
		if (UCanvasPanelSlot* ItemSlot = PickerGrid->AddChildToCanvas(ItemSize))
		{
			ItemSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ItemSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ItemSlot->SetPosition(FVector2D(ColIndex * 170.0f, RowIndex * 200.0f));
			ItemSlot->SetAutoSize(true);
		}

		UButton* ItemButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ItemSize->AddChild(ItemButton);

		UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		Proxy->Owner = this;
		Proxy->Slot = SlotIndex;
		Proxy->ItemId = Item->ItemId;
		Proxy->bIsItemButton = true;
		ClickProxies.Add(Proxy);
		ItemButton->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

		UVerticalBox* ItemColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		ItemButton->AddChild(ItemColumn);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(150.0f);
		IconSize->SetHeightOverride(150.0f);
		if (UVerticalBoxSlot* IconSlot = ItemColumn->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (UTexture2D* Texture = Item->Icon.LoadSynchronous())
		{
			Icon->SetBrushFromTexture(Texture, true);
		}
		IconSize->AddChild(Icon);

		UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameLabel->SetText(FText::FromString(FMyBondageLifeRestraintCatalog::GetDisplayName(Item->ItemId)));
		NameLabel->SetJustification(ETextJustify::Center);
		NameLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		{
			FSlateFontInfo NameFont = NameLabel->GetFont();
			NameFont.Size = 16;
			NameLabel->SetFont(NameFont);
		}
		if (UVerticalBoxSlot* NameSlot = ItemColumn->AddChildToVerticalBox(NameLabel))
		{
			NameSlot->SetHorizontalAlignment(HAlign_Center);
			NameSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
	}

	// "脱下" button when the slot already holds something.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			if (!Account->GetEquippedRestraint(SlotIndex).IsEmpty())
			{
				USizeBox* RemoveSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				RemoveSize->SetWidthOverride(140.0f);
				RemoveSize->SetHeightOverride(60.0f);
				if (UCanvasPanelSlot* RemoveSlot = PickerGrid->AddChildToCanvas(RemoveSize))
				{
					RemoveSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
					RemoveSlot->SetAlignment(FVector2D(0.0f, 0.0f));
					RemoveSlot->SetPosition(FVector2D(0.0f, 410.0f));
					RemoveSlot->SetAutoSize(true);
				}

				UButton* RemoveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
				RemoveSize->AddChild(RemoveButton);

				UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
				Proxy->Owner = this;
				Proxy->Slot = SlotIndex;
				Proxy->ItemId = NAME_None;
				Proxy->bIsItemButton = true;
				ClickProxies.Add(Proxy);
				RemoveButton->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

				UTextBlock* RemoveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				RemoveLabel->SetText(FText::FromString(TEXT("\u8131\u4E0B")));
				RemoveLabel->SetJustification(ETextJustify::Center);
				RemoveLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
				RemoveButton->AddChild(RemoveLabel);
			}
		}
	}

	PickerBackdrop->SetVisibility(ESlateVisibility::Visible);
	PickerGrid->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PickerTitle->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMyBondageLifeWardrobeWidget::CloseRestraintPicker()
{
	if (PickerBackdrop)
	{
		PickerBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PickerGrid)
	{
		PickerGrid->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PickerTitle)
	{
		PickerTitle->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMyBondageLifeWardrobeWidget::PickRestraintItem(FName ItemId)
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	if (ItemId.IsNone())
	{
		Account->UnequipRestraint(PickerSlot);
	}
	else
	{
		Account->EquipRestraint(PickerSlot, ItemId.ToString());
	}

	CloseRestraintPicker();
	RefreshRestraintSlots();
}
