// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeWardrobeWidget.h"
#include "MyBondageLifeCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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
	const float HeightPresets[] = { 0.9f, 0.95f, 1.0f, 1.05f, 1.1f };
	const TCHAR* HeightLabels[] = {
		TEXT("\u5A07\u5C0F"),   // 娇小  0.90
		TEXT("\u8F83\u5C0F"),   // 较小  0.95
		TEXT("\u666E\u901A"),   // 普通  1.00
		TEXT("\u8F83\u5927"),   // 较大  1.05
		TEXT("\u9AD8\u5927")    // 高大  1.10
	};
	constexpr int32 HeightPresetCount = UE_ARRAY_COUNT(HeightPresets);

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

	if (WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeWardrobeWidget::BuildLayout()
{
	// A designed widget blueprint wins over the code-built layout.
	if (TryAdoptBlueprintLayout())
	{
		WireAdoptedLayout();
		BuildRestraintSlots();
		RefreshRestraintSlots();
		RefreshHeightLabel();
		RefreshHairLabel();
		SelectTab(0);
		OnLayoutBuilt();
		return;
	}

	// Reuse the asset's root canvas when the widget blueprint already has one.
	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WardrobeRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

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
			TabSlot->SetPosition(FVector2D(OffsetX, 20.0f));

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


	OnLayoutBuilt();
}

void UMyBondageLifeWardrobeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Account->OnRestraintsChanged.AddDynamic(this, &UMyBondageLifeWardrobeWidget::HandleRestraintsChanged);
	}

	// Start from the height the account service already has.
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (Account)
	{
		// Pick the closest preset to whatever the account service stored.
		const float Scale = Account->GetHeightScale();
		int32 BestIndex = 0;
		float BestDelta = FLT_MAX;
		for (int32 Index = 0; Index < HeightPresetCount; ++Index)
		{
			const float Delta = FMath::Abs(HeightPresets[Index] - Scale);
			if (Delta < BestDelta)
			{
				BestDelta = Delta;
				BestIndex = Index;
			}
		}
		HeightIndex = BestIndex;
	}

	RefreshHeightLabel();

	// Hair: start on the style stored on the account (falls back to the first row).
	HairId = FMyBondageLifeHairCatalog::ClampId(Account ? Account->GetHairIndex() : 1);
	RefreshHairLabel();
}

void UMyBondageLifeWardrobeWidget::SelectTab(int32 TabIndex)
{
	// Somebody else's wardrobe has no body page.
	if (bFriendMode && TabIndex == 0)
	{
		// Somebody else's wardrobe has no body page: ignore the click entirely.
		return;
	}

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

	HeightIndex = FMath::Clamp(HeightIndex, 0, HeightPresetCount - 1);
	HeightValueLabel->SetText(FText::FromString(HeightLabels[HeightIndex]));

	RefreshArrowStates();
}

void UMyBondageLifeWardrobeWidget::StepHeight(int32 Delta)
{
	const int32 NewIndex = FMath::Clamp(HeightIndex + Delta, 0, HeightPresetCount - 1);
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
	return HeightPresets[FMath::Clamp(HeightIndex, 0, HeightPresetCount - 1)];
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

		if (!bOpen)
		{
			ResetToOwnMode();
		}
	}

	if (bOpen)
	{
		// Opening the panel normally means "my wardrobe"; OpenFriendWardrobe sets the
		// pending flag right before opening (EnterFriendMode already ran).
		if (bFriendMode && !bPendingFriendOpen)
		{
			ResetToOwnMode();
		}
		bPendingFriendOpen = false;

		SelectTab(bFriendMode ? 2 : 0);
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
	const bool bCanGoUp = HeightIndex < HeightPresetCount - 1;

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

	if (bClosePicker)
	{
		Owner->CloseRestraintPicker();
		Owner->CloseLockMenu();
		Owner->CloseLockInput();
		return;
	}

	if (bLockConfirm)
	{
		Owner->ConfirmLockInput();
		return;
	}

	if (bLockCancel)
	{
		// Same button is used by the lock menu and the parameter popup.
		Owner->CloseLockInput();
		Owner->CloseLockMenu();
		return;
	}

	if (bLockAction)
	{
		Owner->HandleLockAction();
		return;
	}

	if (bLockChoice)
	{
		// 2 and 5 need a duration, 4 needs a password: those open a small popup.
		if (LockType == 2 || LockType == 4 || LockType == 5)
		{
			Owner->OpenLockInput(LockType);
		}
		else if (UMyBondageLifeAccountSubsystem* Account = Owner->GetGameInstance()
			? Owner->GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
			: nullptr)
		{
			Owner->bClosePanelsOnNextChange = true;
			Owner->CloseLockMenu();
			Owner->PlayLockCue();
			if (Owner->IsFriendMode()) { Account->LockRestraintFor(Owner->GetFriendAccount(), Owner->PickerSlot, LockType, 0, FString()); } else { Account->LockRestraint(Owner->PickerSlot, LockType, 0, FString()); }
		}
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

	// Optional designed container for the 12 cells.
	UCanvasPanel* SlotContainer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("W_SlotContainer")));
	if (!SlotContainer)
	{
		SlotContainer = RestraintPage;
	}
	// Hint line under the slot grid.
	USizeBox* HintSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RestraintHint"));
	HintSize->SetWidthOverride(1000.0f);
	if (UCanvasPanelSlot* HintSlot = RestraintPage->AddChildToCanvas(HintSize))
	{
		HintSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		HintSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		HintSlot->SetPosition(FVector2D(56.0f, 650.0f));
		HintSlot->SetAutoSize(true);
	}

	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HintText->SetText(FText::FromString(TEXT("\u624B\u81C2\u5931\u53BB\u81EA\u7531\u72B6\u6001\u4E0B\u65E0\u6CD5\u7ED9\u522B\u4EBA\u6DFB\u52A0\u62D8\u675F\uFF0C\u4E14\u53EA\u80FD\u7531\u652F\u914D\u8005\u7ED9\u5176\u4F59\u89D2\u8272\u914D\u7F6E\u62D8\u675F\uFF0C\u6216\u8005\u7531\u8F6C\u6362\u8005\u7ED9\u8F6C\u6362\u8005\u3001\u81E3\u670D\u8005\u3001\u5974\u96B6\u53D1\u8D77\u62D8\u675F")));
	HintText->SetAutoWrapText(true);
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.42f)));
	{
		FSlateFontInfo HintFont = HintText->GetFont();
		HintFont.Size = 16;
		HintText->SetFont(HintFont);
	}
	HintSize->AddChild(HintText);

	SlotIcons.Reset();
	SlotLabels.Reset();
	SlotLockBadges.Reset();
	ClickProxies.Reset();

	// 4 columns x 3 rows, filled column by column to match the enum order.
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const int32 ColIndex = SlotIndex / 3;
		const int32 Row = SlotIndex % 3;

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CellSize->SetWidthOverride(130.0f);
		CellSize->SetHeightOverride(152.0f);

		if (UCanvasPanelSlot* CellSlot = SlotContainer->AddChildToCanvas(CellSize))
		{
			CellSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			CellSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			// Start below the tab row (tabs sit at y=116 in the panel canvas).
			CellSlot->SetPosition(FVector2D(56.0f + ColIndex * 150.0f, 124.0f + Row * 170.0f));
			CellSlot->SetAutoSize(true);
		}

		UButton* SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UOverlay* CellOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		CellSize->AddChild(CellOverlay);
		CellOverlay->AddChildToOverlay(SlotButton);
		SlotButtons.Add(SlotButton);

		// Corner lock badge (decorative, never blocks the click).
		UImage* LockBadge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		LockBadge->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* BadgeSlot = CellOverlay->AddChildToOverlay(LockBadge))
		{
			BadgeSlot->SetHorizontalAlignment(HAlign_Right);
			BadgeSlot->SetVerticalAlignment(VAlign_Top);
		}
		SlotLockBadges.Add(LockBadge);

		UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		Proxy->Owner = this;
		Proxy->Slot = SlotIndex;
		Proxy->bIsItemButton = false;
		ClickProxies.Add(Proxy);
		SlotButton->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		SlotButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(130.0f);
		IconSize->SetHeightOverride(130.0f);
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
			LabelFont.Size = 16;
			Label->SetFont(LabelFont);
		}
		if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
		SlotLabels.Add(Label);
	}

	// A designed picker (W_Picker) wins; otherwise build the default one.
	if (!TryAdoptPicker())
	{
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

	// "上锁 / 开锁" action, always present; its text and enabled state follow the slot.
	USizeBox* LockActionSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LockActionSize->SetWidthOverride(180.0f);
	LockActionSize->SetHeightOverride(56.0f);
	if (UVerticalBoxSlot* ActionSlot = PickerColumn->AddChildToVerticalBox(LockActionSize))
	{
		ActionSlot->SetHorizontalAlignment(HAlign_Center);
		ActionSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
	}

	LockActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	LockActionSize->AddChild(LockActionButton);

	UMyBondageLifeWardrobeClickProxy* ActionProxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
	ActionProxy->Owner = this;
	ActionProxy->bLockAction = true;
	ClickProxies.Add(ActionProxy);
	LockActionButton->OnClicked.AddDynamic(ActionProxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

	LockActionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LockActionLabel->SetJustification(ETextJustify::Center);
	LockActionLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo ActionFont = LockActionLabel->GetFont();
		ActionFont.Size = 20;
		LockActionLabel->SetFont(ActionFont);
	}
	LockActionButton->AddChild(LockActionLabel);
	// Always available, even when the slot has no candidates at all.
	USizeBox* CancelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CancelSize->SetWidthOverride(140.0f);
	CancelSize->SetHeightOverride(56.0f);
	if (UVerticalBoxSlot* CancelSlot = PickerColumn->AddChildToVerticalBox(CancelSize))
	{
		CancelSlot->SetHorizontalAlignment(HAlign_Center);
		CancelSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
	}

	UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelSize->AddChild(CancelButton);

	UMyBondageLifeWardrobeClickProxy* CancelProxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
	CancelProxy->Owner = this;
	CancelProxy->bClosePicker = true;
	ClickProxies.Add(CancelProxy);
	CancelButton->OnClicked.AddDynamic(CancelProxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelLabel->SetText(FText::FromString(TEXT("\u53D6\u6D88")));
	CancelLabel->SetJustification(ETextJustify::Center);
	CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	{
		FSlateFontInfo CancelFont = CancelLabel->GetFont();
		CancelFont.Size = 20;
		CancelLabel->SetFont(CancelFont);
	}
	CancelButton->AddChild(CancelLabel);
	}
}

void UMyBondageLifeWardrobeWidget::RefreshRestraintSlots()
{
	RefreshLockBadges();
	RefreshLockActionButton();
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	for (int32 SlotIndex = 0; SlotIndex < SlotButtons.Num(); ++SlotIndex)
	{
		const FString ItemId = Account ? (bFriendMode ? Account->GetViewedRestraint(SlotIndex) : Account->GetEquippedRestraint(SlotIndex)) : FString();
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
			// Clearing matters: an unequipped slot must drop the previous icon.
			UTexture2D* Icon = bWorn ? FMyBondageLifeRestraintCatalog::LoadIcon(FName(*ItemId)) : nullptr;
			if (Icon)
			{
				SlotIcons[SlotIndex]->SetBrushFromTexture(Icon, true);
			}
			else
			{
				SlotIcons[SlotIndex]->SetBrush(FSlateBrush());
			}
		}
	}
}

void UMyBondageLifeWardrobeWidget::OpenRestraintPicker(int32 SlotIndex)
{
	if (bFriendMode)
	{
		const UMyBondageLifeAccountSubsystem* Guard = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
		const AMyBondageLifeCharacter* OwnPawn = Cast<AMyBondageLifeCharacter>(GetOwningPlayerPawn());
		if (!Guard || !Guard->CanEditRestraintsOf(FriendAccount) || (OwnPawn && OwnPawn->IsArmsRestrained()))
		{
			return;
		}
	}

	if (!PickerBackdrop || !PickerGrid || !PickerTitle)
	{
		return;
	}

	PickerSlot = SlotIndex;

	if (bFriendMode)
	{
		// The owner may have changed things externally: re-read before showing state.
		if (UMyBondageLifeAccountSubsystem* RefreshAccount = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			RefreshAccount->RefreshRestraintsFor(FriendAccount);
		}
	}

	const UMyBondageLifeAccountSubsystem* LockAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	const FMyBondageLifeRestraintLock SlotLock = LockAccount
		? (bFriendMode ? LockAccount->GetViewedLock(SlotIndex) : LockAccount->GetRestraintLock(SlotIndex))
		: FMyBondageLifeRestraintLock();
	const bool bSlotLocked = SlotLock.LockType > 0;
	PickerLockType = SlotLock.LockType;
	PickerLockExpiry = SlotLock.ExpiresAt;
	bPickerLockHasExpiry = SlotLock.bHasExpiry;

	PickerGrid->ClearChildren();
	ClickProxies.RemoveAll([](const TObjectPtr<UMyBondageLifeWardrobeClickProxy>& Proxy)
	{
		return Proxy && Proxy->bIsItemButton;
	});

	{
		FString Title = FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex);
		if (bSlotLocked)
		{
			static const TCHAR* LockNames[] = { TEXT(""), TEXT("\u4E3B\u4EBA\u9501"), TEXT("\u4E3B\u4EBA\u8BA1\u65F6\u9501"),
				TEXT("\u666E\u901A\u6302\u9501"), TEXT("\u5BC6\u7801\u9501"), TEXT("\u8BA1\u65F6\u9501") };
			const int32 LockIndex = FMath::Clamp(SlotLock.LockType, 0, 5);
			Title += FString::Printf(TEXT(" \uFF1A\u5DF2\u4E0A\u9501\uFF08%s\uFF09"), LockNames[LockIndex]);
		}
		else
		{
			Title += TEXT(" \uFF1A\u9009\u62E9\u62D8\u675F");
		}
		PickerTitle->SetText(FText::FromString(Title));
	}

	const TArray<const FMyBondageLifeRestraintItem*> Items = FMyBondageLifeRestraintCatalog::GetItemsForSlot(SlotIndex);

	int32 Index = 0;
	for (const FMyBondageLifeRestraintItem* Item : Items)
	{
		// A locked slot cannot be changed, so no candidates are listed.
		if (bSlotLocked)
		{
			break;
		}
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
		IconSize->SetWidthOverride(130.0f);
		IconSize->SetHeightOverride(130.0f);
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
			if (!bSlotLocked && !(bFriendMode ? Account->GetViewedRestraint(SlotIndex) : Account->GetEquippedRestraint(SlotIndex)).IsEmpty())
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

	CloseLockMenu();
	CloseLockInput();
	RefreshLockActionButton();

	if (bPickerLockHasExpiry)
	{
		UpdateLockCountdown();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LockCountdownTimer, this,
				&UMyBondageLifeWardrobeWidget::UpdateLockCountdown, 1.0f, true);
		}
	}
	else
	{
		StopLockCountdown();
	}

	PickerBackdrop->SetVisibility(ESlateVisibility::Visible);
	PickerGrid->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PickerTitle->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMyBondageLifeWardrobeWidget::ResetToOwnMode()
{
	if (UWorld* StopWorld = GetWorld())
	{
		StopWorld->GetTimerManager().ClearTimer(ViewedSyncTimer);
	}

	bFriendMode = false;
	FriendAccount.Reset();
	RefreshRestraintSlots();
	RefreshLockBadges();
	RefreshLockActionButton();
}

void UMyBondageLifeWardrobeWidget::CloseRestraintPicker()
{
	StopLockCountdown();
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
		bClosePanelsOnNextChange = true;
	if (bFriendMode) { Account->UnequipRestraintFor(FriendAccount, PickerSlot); } else { Account->UnequipRestraint(PickerSlot); }
	}
	else
	{
		bClosePanelsOnNextChange = true;
	if (bFriendMode) { Account->EquipRestraintFor(FriendAccount, PickerSlot, ItemId.ToString()); } else { Account->EquipRestraint(PickerSlot, ItemId.ToString()); }
	}

	CloseRestraintPicker();
	RefreshRestraintSlots();
}

// ---------------------------------------------------------------------------
// Restraint locks
// ---------------------------------------------------------------------------

namespace
{
	const TCHAR* LockIconPath(int32 LockType)
	{
		switch (LockType)
		{
		case 1: return TEXT("/Game/2DAssest/UI/MasterLock.MasterLock");
		case 2: return TEXT("/Game/2DAssest/UI/MasterTimeLock.MasterTimeLock");
		case 3: return TEXT("/Game/2DAssest/UI/NormalLock.NormalLock");
		case 4: return TEXT("/Game/2DAssest/UI/PasswordLock.PasswordLock");
		case 5: return TEXT("/Game/2DAssest/UI/TimePadLock.TimePadLock");
		default: return nullptr;
		}
	}

	FText LockTitle(int32 LockType)
	{
		switch (LockType)
		{
		case 1: return FText::FromString(TEXT("\u4E3B\u4EBA\u9501"));
		case 2: return FText::FromString(TEXT("\u4E3B\u4EBA\u8BA1\u65F6\u9501"));
		case 3: return FText::FromString(TEXT("\u666E\u901A\u6302\u9501"));
		case 4: return FText::FromString(TEXT("\u5BC6\u7801\u9501"));
		case 5: return FText::FromString(TEXT("\u8BA1\u65F6\u9501"));
		default: return FText::GetEmpty();
		}
	}

	FText LockTooltip(int32 LockType)
	{
		switch (LockType)
		{
		case 1:
			return FText::FromString(TEXT("\u4E3B\u4EBA\u9501\uFF1A\u53EA\u80FD\u7531\u4E3B\u4EBA\u7ED9\u5176\u5974\u96B6/\u4E34\u65F6\u5974\u96B6\u4F7F\u7528\uFF0C\u81EA\u5DF1\u65E0\u6CD5\u5BF9\u81EA\u5DF1\u4F7F\u7528\u3002\u4E0A\u9501\u540E\u6CA1\u6709\u65F6\u9650\uFF0C\u76F4\u5230\u4E3B\u4EBA\u53D6\u4E0B\u3002"));
		case 2:
			return FText::FromString(TEXT("\u4E3B\u4EBA\u8BA1\u65F6\u9501\uFF1A\u53EA\u80FD\u7531\u4E3B\u4EBA\u7ED9\u5176\u5974\u96B6/\u4E34\u65F6\u5974\u96B6\u4F7F\u7528\u3002\u4E0A\u9501\u65F6\u8BBE\u5B9A\u65F6\u95F4\uFF08\u6700\u591A 30 \u5929\uFF09\uFF0C\u4E3B\u4EBA\u4E5F\u53EF\u4EE5\u63D0\u524D\u53D6\u4E0B\u3002"));
		case 3:
			return FText::FromString(TEXT("\u666E\u901A\u6302\u9501\uFF1A\u652F\u914D\u8005\u53EF\u7ED9\u4EFB\u610F\u4F4D\u7F6E\u66F4\u4F4E\u7684\u597D\u53CB\u4F7F\u7528\uFF0C\u8F6C\u6362\u8005\u4E4B\u95F4\u4E5F\u53EF\u3002\u65E0\u65F6\u9650\uFF0C\u9664\u88AB\u9501\u8005\u5916\u4EFB\u4F55\u4EBA\u90FD\u53EF\u53D6\u4E0B\u3002"));
		case 4:
			return FText::FromString(TEXT("\u5BC6\u7801\u9501\uFF1A\u652F\u914D\u8005\u53EF\u7ED9\u4F4E\u4F4D\u597D\u53CB\u4F7F\u7528\uFF0C\u8F6C\u6362\u8005\u4E4B\u95F4\u4E5F\u53EF\u3002\u65E0\u65F6\u9650\uFF0C\u77E5\u9053 4~8 \u4F4D\u5BC6\u7801\u7684\u4EFB\u4F55\u4EBA\u90FD\u53EF\u53D6\u4E0B\u3002"));
		case 5:
			return FText::FromString(TEXT("\u8BA1\u65F6\u9501\uFF1A\u652F\u914D\u8005\u53EF\u7ED9\u4F4E\u4F4D\u597D\u53CB\u4F7F\u7528\uFF0C\u4E5F\u53EF\u5BF9\u81EA\u5DF1\u4F7F\u7528\u3002\u8BA1\u65F6\u5F00\u59CB\u540E\u65E0\u4EBA\u53EF\u53D6\u4E0B\uFF1B\u81EA\u9501\u6700\u591A 24 \u5C0F\u65F6\uFF0C\u4ED6\u4EBA\u4E0A\u9501\u6700\u591A 7 \u5929\u3002"));
		default:
			return FText::GetEmpty();
		}
	}

	/** "HH:MM:SS" (or plain seconds) -> seconds. */
	int32 ParseDuration(const FString& Text)
	{
		TArray<FString> Parts;
		Text.TrimStartAndEnd().ParseIntoArray(Parts, TEXT(":"), true);
		if (Parts.Num() == 0)
		{
			return 0;
		}

		if (Parts.Num() == 1)
		{
			return FCString::Atoi(*Parts[0]);
		}

		int32 Seconds = 0;
		int32 Multipliers[3] = { 3600, 60, 1 };
		const int32 Offset = 3 - FMath::Min(Parts.Num(), 3);
		for (int32 Index = 0; Index < Parts.Num() && Index < 3; ++Index)
		{
			Seconds += FCString::Atoi(*Parts[Index]) * Multipliers[Offset + Index];
		}
		return Seconds;
	}
}

void UMyBondageLifeWardrobeWidget::RefreshLockBadges()
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	for (int32 SlotIndex = 0; SlotIndex < SlotLockBadges.Num(); ++SlotIndex)
	{
		UImage* Badge = SlotLockBadges[SlotIndex];
		if (!Badge)
		{
			continue;
		}

		const FMyBondageLifeRestraintLock Lock = Account
			? (bFriendMode ? Account->GetViewedLock(SlotIndex) : Account->GetRestraintLock(SlotIndex))
			: FMyBondageLifeRestraintLock();

		UTexture2D* Icon = Lock.LockType > 0
			? LoadObject<UTexture2D>(nullptr, LockIconPath(Lock.LockType))
			: nullptr;

		if (Icon)
		{
			Badge->SetBrushFromTexture(Icon, true);
			Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Badge->SetBrush(FSlateBrush());
			Badge->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UMyBondageLifeWardrobeWidget::RefreshLockActionButton()
{
	if (!LockActionButton || !LockActionLabel)
	{
		return;
	}

	// An empty slot has nothing to lock, so the button is hidden entirely.
	const bool bHasItem = [this]() -> bool
	{
		UGameInstance* GameInstance = GetGameInstance();
		UMyBondageLifeAccountSubsystem* AccountSub = GameInstance ? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
		if (!AccountSub) { return false; }
		return !(bFriendMode ? AccountSub->GetViewedRestraint(PickerSlot) : AccountSub->GetEquippedRestraint(PickerSlot)).IsEmpty();
	}();

	LockActionButton->SetVisibility(bHasItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bHasItem)
	{
		return;
	}

	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	const FMyBondageLifeRestraintLock Lock = Account
		? (bFriendMode ? Account->GetViewedLock(PickerSlot) : Account->GetRestraintLock(PickerSlot))
		: FMyBondageLifeRestraintLock();

	if (Lock.LockType > 0)
	{
		LockActionLabel->SetText(FText::FromString(TEXT("\u5F00\u9501")));
		LockActionButton->SetIsEnabled(Lock.bCanUnlock);
		LockActionButton->SetToolTipText(FText::FromString(
			Lock.bCanUnlock ? FString() : Lock.UnlockReason));
	}
	else
	{
		LockActionLabel->SetText(FText::FromString(TEXT("\u4E0A\u9501")));
		LockActionButton->SetIsEnabled(true);
		LockActionButton->SetToolTipText(FText::GetEmpty());
	}
}

void UMyBondageLifeWardrobeWidget::HandleLockAction()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Lock: no account subsystem, action ignored."));
		return;
	}

	const FMyBondageLifeRestraintLock Lock = (bFriendMode ? Account->GetViewedLock(PickerSlot) : Account->GetRestraintLock(PickerSlot));
	UE_LOG(LogMyBondageLife, Warning, TEXT("Lock: action slot=%d lockType=%d canUnlock=%d"),
		PickerSlot, Lock.LockType, Lock.bCanUnlock ? 1 : 0);
	if (Lock.LockType == 0)
	{
		OpenLockMenu();
		return;
	}

	if (!Lock.bCanUnlock)
	{
		return;
	}

	if (Lock.LockType == 4)
	{
		// Password locks need the password to open.
		OpenLockInput(104);
		return;
	}

	bClosePanelsOnNextChange = true;
	if (bFriendMode) { Account->UnlockRestraintFor(FriendAccount, PickerSlot, FString()); } else { Account->UnlockRestraint(PickerSlot, FString()); }
}

void UMyBondageLifeWardrobeWidget::OpenLockMenu()
{
	if (!RestraintPage)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Lock: menu has no page."));
		return;
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Lock: opening menu, created=%d"),
		LockMenuBackdrop ? 1 : 0);

	if (!LockMenuBackdrop)
	{
		LockMenuBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LockMenu"));
		LockMenuBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
		LockMenuBackdrop->SetHorizontalAlignment(HAlign_Center);
		LockMenuBackdrop->SetVerticalAlignment(VAlign_Center);
		if (UCanvasPanelSlot* BackdropSlot = RestraintPage->AddChildToCanvas(LockMenuBackdrop))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
			BackdropSlot->SetZOrder(40);
		}

		UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Box->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		Box->SetPadding(FMargin(26.0f, 20.0f));
		LockMenuBackdrop->AddChild(Box);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->AddChild(Column);

		LockMenuTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LockMenuTitle->SetText(FText::FromString(TEXT("\u9009\u62E9\u9501")));
		LockMenuTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		{
			FSlateFontInfo TitleFont = LockMenuTitle->GetFont();
			TitleFont.Size = 22;
			LockMenuTitle->SetFont(TitleFont);
		}
		if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(LockMenuTitle))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Row);

		for (int32 LockType = 1; LockType <= 5; ++LockType)
		{
			// One column per lock: icon on top, caption underneath.
			UVerticalBox* LockBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (UHorizontalBoxSlot* LockSlot = Row->AddChildToHorizontalBox(LockBox))
			{
				LockSlot->SetPadding(FMargin(6.0f));
			}

			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			IconSize->SetWidthOverride(120.0f);
			IconSize->SetHeightOverride(120.0f);
			if (UVerticalBoxSlot* IconSlot = LockBox->AddChildToVerticalBox(IconSize))
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
			}

			UButton* IconButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			IconSize->AddChild(IconButton);
			IconButton->SetToolTipText(LockTooltip(LockType));

			UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
			Proxy->Owner = this;
			Proxy->bLockChoice = true;
			Proxy->LockType = LockType;
			ClickProxies.Add(Proxy);
			IconButton->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, LockIconPath(LockType)))
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			IconButton->AddChild(Icon);

			// Only the locks the local player may actually use stay clickable.
			const bool bAllowed = IsLockTypeAllowed(LockType);
			IconButton->SetIsEnabled(bAllowed);
			Icon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, bAllowed ? 1.0f : 0.3f));
			if (!bAllowed)
			{
				IconButton->SetToolTipText(FText::FromString(TEXT("\uFF08\u5F53\u524D\u8EAB\u4EFD\u4E0D\u53EF\u7528\uFF09")));
			}

			UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Caption->SetText(LockTitle(LockType));
			Caption->SetJustification(ETextJustify::Center);
			Caption->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f)));
			{
				FSlateFontInfo CaptionFont = Caption->GetFont();
				CaptionFont.Size = 15;
				Caption->SetFont(CaptionFont);
			}
			if (UVerticalBoxSlot* CaptionSlot = LockBox->AddChildToVerticalBox(Caption))
			{
				CaptionSlot->SetHorizontalAlignment(HAlign_Center);
				CaptionSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
			}
		}

		USizeBox* CancelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CancelSize->SetWidthOverride(160.0f);
		CancelSize->SetHeightOverride(52.0f);
		if (UVerticalBoxSlot* CancelSlot = Column->AddChildToVerticalBox(CancelSize))
		{
			CancelSlot->SetHorizontalAlignment(HAlign_Center);
			CancelSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
		}

		UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		CancelSize->AddChild(CancelButton);

		UMyBondageLifeWardrobeClickProxy* CancelProxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		CancelProxy->Owner = this;
		CancelProxy->bLockCancel = true;
		ClickProxies.Add(CancelProxy);
		CancelButton->OnClicked.AddDynamic(CancelProxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

		UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CancelLabel->SetText(FText::FromString(TEXT("\u53D6\u6D88")));
		CancelLabel->SetJustification(ETextJustify::Center);
		CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		CancelButton->AddChild(CancelLabel);
	}

	LockMenuBackdrop->SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeWardrobeWidget::CloseLockMenu()
{
	if (LockMenuBackdrop)
	{
		LockMenuBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMyBondageLifeWardrobeWidget::OpenLockInput(int32 InLockType)
{
	if (!RestraintPage)
	{
		return;
	}

	PendingLockType = InLockType;

	if (!LockInputBackdrop)
	{
		LockInputBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LockInput"));
		LockInputBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		LockInputBackdrop->SetHorizontalAlignment(HAlign_Center);
		LockInputBackdrop->SetVerticalAlignment(VAlign_Center);
		if (UCanvasPanelSlot* BackdropSlot = RestraintPage->AddChildToCanvas(LockInputBackdrop))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
			BackdropSlot->SetZOrder(50);
		}

		UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Box->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		Box->SetPadding(FMargin(26.0f, 20.0f));
		LockInputBackdrop->AddChild(Box);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->AddChild(Column);

		LockInputTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LockInputTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(LockInputTitle))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		LockInputHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LockInputHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f)));
		if (UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(LockInputHint))
		{
			HintSlot->SetHorizontalAlignment(HAlign_Center);
			HintSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		LockInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		LockInputBox->SetHintText(FText::FromString(TEXT("HH:MM:SS")));
		if (UVerticalBoxSlot* BoxSlot = Column->AddChildToVerticalBox(LockInputBox))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
			BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(ButtonRow))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}

		auto AddButton = [this, ButtonRow](const TCHAR* Caption, bool bConfirm) -> void
		{
			USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			ButtonSize->SetWidthOverride(130.0f);
			ButtonSize->SetHeightOverride(52.0f);
			if (UHorizontalBoxSlot* ButtonSlot = ButtonRow->AddChildToHorizontalBox(ButtonSize))
			{
				ButtonSlot->SetPadding(FMargin(6.0f));
			}

			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			ButtonSize->AddChild(Button);

			UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
			Proxy->Owner = this;
			Proxy->bLockConfirm = bConfirm;
			Proxy->bLockCancel = !bConfirm;
			ClickProxies.Add(Proxy);
			Button->OnClicked.AddDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Label->SetText(FText::FromString(Caption));
			Label->SetJustification(ETextJustify::Center);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
			Button->AddChild(Label);
		};

		AddButton(TEXT("\u786E\u5B9A"), true);
		AddButton(TEXT("\u53D6\u6D88"), false);
	}

	const bool bUnlockPassword = (InLockType == 104);
	const bool bPassword = (InLockType == 4) || bUnlockPassword;

	LockInputTitle->SetText(bPassword
		? FText::FromString(TEXT("\u8F93\u5165\u5BC6\u7801"))
		: FText::FromString(bUnlockPassword ? TEXT("") : LockTitle(InLockType).ToString() + TEXT(" \u65F6\u957F")));
	LockInputHint->SetText(bPassword
		? FText::FromString(TEXT("4~8 \u4F4D\u7EAF\u6570\u5B57"))
		: FText::FromString(TEXT("\u683C\u5F0F \u65F6:\u5206:\u79D2\uFF0C\u4F8B\u5982 01:30:00")));
	LockInputBox->SetHintText(FText::FromString(bPassword ? TEXT("0000") : TEXT("HH:MM:SS")));
	LockInputBox->SetText(FText::GetEmpty());

	LockInputBackdrop->SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeWardrobeWidget::CloseLockInput()
{
	if (LockInputBackdrop)
	{
		LockInputBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMyBondageLifeWardrobeWidget::ConfirmLockInput()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account || !LockInputBox)
	{
		return;
	}

	const FString Text = LockInputBox->GetText().ToString().TrimStartAndEnd();

	if (PendingLockType == 104)
	{
		bClosePanelsOnNextChange = true;
	if (bFriendMode) { Account->UnlockRestraintFor(FriendAccount, PickerSlot, Text); } else { Account->UnlockRestraint(PickerSlot, Text); }
	}
	else if (PendingLockType == 4)
	{
		bClosePanelsOnNextChange = true;
		PlayLockCue();
		if (bFriendMode) { Account->LockRestraintFor(FriendAccount, PickerSlot, 4, 0, Text); } else { Account->LockRestraint(PickerSlot, 4, 0, Text); }
	}
	else
	{
		bClosePanelsOnNextChange = true;
		PlayLockCue();
		if (bFriendMode) { Account->LockRestraintFor(FriendAccount, PickerSlot, PendingLockType, ParseDuration(Text), FString()); } else { Account->LockRestraint(PickerSlot, PendingLockType, ParseDuration(Text), FString()); }
	}

	CloseLockInput();
	CloseLockMenu();
}

void UMyBondageLifeWardrobeWidget::BuildLockWidgets()
{
	RefreshLockBadges();
}


bool UMyBondageLifeWardrobeWidget::IsLockTypeAllowed(int32 LockType) const
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return false;
	}

	// Today only the local wardrobe is editable, so the actor is always the owner.
	// Keep the per-type rules here so opening a friend's wardrobe later needs no rewrite.
	const bool bSelf = true;

	if (bFriendMode)
	{
		// Someone else's wardrobe: 1/2 need a master contract, 3/4/5 the hierarchy rule.
		const int32 ActorClass = Account->GetCharacterClass();
		const int32 OwnerClass = Account->GetFriendClass(FriendAccount);

		if (LockType == 1 || LockType == 2)
		{
			return Account->GetContractRoleWith(FriendAccount) == EMyBondageLifeContractRole::Master;
		}

		return (ActorClass == 1 && OwnerClass >= 2) || (ActorClass == 2 && OwnerClass >= 2);
	}

	if (bSelf)
	{
		// 1/2 need a master, 3/4 must be applied by someone else, 5 allows self locking.
		return LockType == 5;
	}

	switch (LockType)
	{
	case 1:
	case 2:
		return Account->GetContractRoleWith(Account->GetAccountName()) == EMyBondageLifeContractRole::Master;
	case 3:
	case 4:
	case 5:
		return true;
	default:
		return false;
	}
}
void UMyBondageLifeWardrobeWidget::HandleRestraintsChanged()
{
	// Only a write closes the popups; a plain refresh (e.g. re-reading the viewed
	// wardrobe) must leave whatever the player has open alone.
	if (bClosePanelsOnNextChange)
	{
		bClosePanelsOnNextChange = false;
		CloseLockInput();
		CloseLockMenu();
		CloseRestraintPicker();
	}

	RefreshRestraintSlots();
	RefreshLockBadges();
	RefreshLockActionButton();
}
void UMyBondageLifeWardrobeWidget::EnterFriendMode(const FString& InAccount)
{
	FriendAccount = InAccount;
	bFriendMode = !InAccount.IsEmpty();
	bPendingFriendOpen = bFriendMode;

	if (bFriendMode && BodyTabLabel)
	{
		// No body tab for somebody else's wardrobe.
		BodyTabLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f)));
	}

	SelectTab(bFriendMode ? 2 : 0);

	if (UWorld* SyncWorld = GetWorld())
	{
		SyncWorld->GetTimerManager().ClearTimer(ViewedSyncTimer);
		if (bFriendMode)
		{
			SyncWorld->GetTimerManager().SetTimer(ViewedSyncTimer, this,
				&UMyBondageLifeWardrobeWidget::SyncViewedAccount, 2.0f, true);
		}
	}

	RefreshRestraintSlots();
	RefreshLockBadges();
	RefreshLockActionButton();
}
void UMyBondageLifeWardrobeWidget::SyncViewedAccount()
{
	if (!bFriendMode || FriendAccount.IsEmpty())
	{
		return;
	}

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		// The owner is not the one being pushed events, so poll their wardrobe while
		// it is on screen.
		Account->RefreshRestraintsFor(FriendAccount);
	}
}

namespace
{
	/** Designer assets use the W_ prefix; the old names still work as a fallback. */
	template <typename TWidget>
	TWidget* FindEither(UWidgetTree* Tree, const TCHAR* Preferred, const TCHAR* Legacy)
	{
		if (!Tree)
		{
			return nullptr;
		}

		if (TWidget* Found = Cast<TWidget>(Tree->FindWidget(Preferred)))
		{
			return Found;
		}

		return Cast<TWidget>(Tree->FindWidget(Legacy));
	}
}

bool UMyBondageLifeWardrobeWidget::TryAdoptBlueprintLayout()
{
	if (!WidgetTree)
	{
		return false;
	}

	// The designer asset must provide the panel; everything else is optional.
	UBorder* ExistingPanel = FindEither<UBorder>(WidgetTree, TEXT("W_Panel"), TEXT("WardrobePanel"));
	if (!ExistingPanel)
	{
		return false;
	}

	PanelBorder = ExistingPanel;

	BodyPage = FindEither<UCanvasPanel>(WidgetTree, TEXT("W_Page_Body"), TEXT("WardrobeBodyPage"));
	AppearancePage = FindEither<UCanvasPanel>(WidgetTree, TEXT("W_Page_Appearance"), TEXT("WardrobeAppearancePage"));
	RestraintPage = FindEither<UCanvasPanel>(WidgetTree, TEXT("W_Page_Restraint"), TEXT("WardrobeRestraintPage"));

	BodyTabLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Tab_Body_Label"), TEXT("BodyTabLabel"));
	AppearanceTabLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Tab_Appearance_Label"), TEXT("AppearanceTabLabel"));
	RestraintTabLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Tab_Restraint_Label"), TEXT("RestraintTabLabel"));

	HeightValueLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Height_Value"), TEXT("HeightValueLabel"));
	HeightDownButton = FindEither<UButton>(WidgetTree, TEXT("W_Height_Down"), TEXT("HeightDownButton"));
	HeightUpButton = FindEither<UButton>(WidgetTree, TEXT("W_Height_Up"), TEXT("HeightUpButton"));
	HeightDownLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Height_Down_Label"), TEXT("HeightDownLabel"));
	HeightUpLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Height_Up_Label"), TEXT("HeightUpLabel"));

	HairValueLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Hair_Value"), TEXT("HairValueLabel"));
	HairDownButton = FindEither<UButton>(WidgetTree, TEXT("W_Hair_Down"), TEXT("HairDownButton"));
	HairUpButton = FindEither<UButton>(WidgetTree, TEXT("W_Hair_Up"), TEXT("HairUpButton"));
	HairDownLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Hair_Down_Label"), TEXT("HairDownLabel"));
	HairUpLabel = FindEither<UTextBlock>(WidgetTree, TEXT("W_Hair_Up_Label"), TEXT("HairUpLabel"));

	bBlueprintLayout = true;
	UE_LOG(LogMyBondageLife, Log, TEXT("Wardrobe: using the designed widget layout."));
	return true;
}

void UMyBondageLifeWardrobeWidget::WireAdoptedLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Close"), TEXT("CloseButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleCloseButtonClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Tab_Body"), TEXT("BodyTabButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleBodyTabClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Tab_Appearance"), TEXT("AppearanceTabButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleAppearanceTabClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Tab_Restraint"), TEXT("RestraintTabButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleRestraintTabClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Height_Down"), TEXT("HeightDownButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightDownClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Height_Up"), TEXT("HeightUpButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHeightUpClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Hair_Down"), TEXT("HairDownButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHairDownClicked);
	}
	if (UButton* Button = FindEither<UButton>(WidgetTree, TEXT("W_Hair_Up"), TEXT("HairUpButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeWidget::HandleHairUpClicked);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Wardrobe: adopted layout wired."));
}
bool UMyBondageLifeWardrobeWidget::TryAdoptPicker()
{
	if (!WidgetTree)
	{
		return false;
	}

	UBorder* AdoptedBackdrop = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_Picker")));
	if (!AdoptedBackdrop)
	{
		return false;
	}

	PickerBackdrop = AdoptedBackdrop;
	PickerTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Picker_Title")));
	PickerGrid = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("W_Picker_Grid")));

	LockActionButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Picker_LockAction")));
	LockActionLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Picker_LockAction_Label")));

	if (UButton* CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Picker_Cancel"))))
	{
		UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		Proxy->Owner = this;
		Proxy->bClosePicker = true;
		ClickProxies.Add(Proxy);
		CancelButton->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);
	}

	if (LockActionButton)
	{
		UMyBondageLifeWardrobeClickProxy* Proxy = NewObject<UMyBondageLifeWardrobeClickProxy>(this);
		Proxy->Owner = this;
		Proxy->bLockAction = true;
		ClickProxies.Add(Proxy);
		LockActionButton->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeWardrobeClickProxy::HandleClicked);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Wardrobe: using the designed slot picker."));
	return true;
}
