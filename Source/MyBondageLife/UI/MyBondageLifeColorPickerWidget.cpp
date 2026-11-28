// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeColorPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MyBondageLife.h"

TSharedRef<SWidget> UMyBondageLifeColorPickerWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// 只有纯 C++ 类才自建布局；蓝图子类（WBP_ColorPicker）以蓝图布局为准。
	const bool bPureCPP = GetClass() == UMyBondageLifeColorPickerWidget::StaticClass();
	if (bPureCPP && !IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeColorPickerWidget::BuildLayout()
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ColorPickerRoot"));
		WidgetTree->RootWidget = Canvas;
	}
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 设计版资产根节点是 Border：运行时包一层固定尺寸的卡片，避免铺满整个屏幕。
	if (WidgetTree->RootWidget == RootPanel && RootPanel)
	{
		UCanvasPanel* Wrapper = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ColorPickerCanvas"));
		WidgetTree->RootWidget = Wrapper;
		USizeBox* Card = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ColorPickerCard"));
		Card->SetWidthOverride(420.0f);
		Card->SetHeightOverride(360.0f);
		if (UCanvasPanelSlot* CardSlot = Wrapper->AddChildToCanvas(Card))
		{
			CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardSlot->SetAutoSize(true);
		}
		Card->AddChild(RootPanel);
		Canvas = Wrapper;
	}
	RootPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Root")));
	if (!RootPanel)
	{
		RootPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("W_ColorPicker_Root"));
		RootPanel->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.1f, 0.96f));
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(RootPanel))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetSize(FVector2D(420.0f, 360.0f));
		}

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		RootPanel->SetContent(Column);

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_ColorPicker_Title"));
		Title->SetText(FText::FromString(TEXT("\u67D3\u8272")));
		Column->AddChildToVerticalBox(Title);

		PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_ColorPicker_Preview"));
		USizeBox* PreviewSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PreviewSize->SetWidthOverride(120.0f);
		PreviewSize->SetHeightOverride(80.0f);
		PreviewSize->AddChild(PreviewImage);
		Column->AddChildToVerticalBox(PreviewSize);
	}

	UVerticalBox* Column = Cast<UVerticalBox>(RootPanel ? RootPanel->GetContent() : nullptr);

	// 采纳 WBP 里已有的控件（设计版调色板），否则下面的 if 分支会自建。
	if (!HueSlider) { HueSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Hue"))); }
	if (!SaturationSlider) { SaturationSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Saturation"))); }
	if (!ValueSlider) { ValueSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Value"))); }
	if (!HexBox) { HexBox = Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Hex"))); }
	if (!PreviewImage) { PreviewImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Preview"))); }

	auto AddRow = [this, Column](const FString& Label, TFunction<UWidget*(UWidgetTree*)> MakeWidget)
	{
		if (!Column)
		{
			return;
		}
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Caption->SetText(FText::FromString(Label));
		if (UHorizontalBoxSlot* CaptionSlot = Row->AddChildToHorizontalBox(Caption))
		{
			CaptionSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			CaptionSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UWidget* Control = MakeWidget(WidgetTree))
		{
			if (UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(Control))
			{
				ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
		Column->AddChildToVerticalBox(Row);
	};

	if (!HueSlider)
	{
		HueSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("W_ColorPicker_Hue"));
		HueSlider->SetMinValue(0.0f);
		HueSlider->SetMaxValue(1.0f);
		AddRow(TEXT("H"), [this](UWidgetTree*) -> UWidget* { return HueSlider; });
		HueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHueChanged);
	}
	if (!SaturationSlider)
	{
		SaturationSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("W_ColorPicker_Saturation"));
		SaturationSlider->SetMinValue(0.0f);
		SaturationSlider->SetMaxValue(1.0f);
		AddRow(TEXT("S"), [this](UWidgetTree*) -> UWidget* { return SaturationSlider; });
		SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleSaturationChanged);
	}
	if (!ValueSlider)
	{
		ValueSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("W_ColorPicker_Value"));
		ValueSlider->SetMinValue(0.0f);
		ValueSlider->SetMaxValue(1.0f);
		AddRow(TEXT("V"), [this](UWidgetTree*) -> UWidget* { return ValueSlider; });
		ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleValueChanged);
	}
	if (!HexBox)
	{
		HexBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("W_ColorPicker_Hex"));
		HexBox->SetHintText(FText::FromString(TEXT("#RRGGBB")));
		AddRow(TEXT("#"), [this](UWidgetTree*) -> UWidget* { return HexBox; });
		HexBox->OnTextCommitted.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexCommitted);
		HexBox->OnTextChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexChanged);

	// 无论滑块/文本框是自建还是从 WBP 接管，都统一绑定，避免接管时不响应拖动。
	if (HueSlider) { HueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHueChanged); }
	if (SaturationSlider) { SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleSaturationChanged); }
	if (ValueSlider) { ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleValueChanged); }
	if (HexBox)
	{
		HexBox->OnTextCommitted.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexCommitted);
		HexBox->OnTextChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexChanged);
	}
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Column)
	{
		Column->AddChildToVerticalBox(Buttons);
	}

	auto AddFlatButton = [this](const FString& Caption, const FName& Name) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Caption));
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		return Button;
	};

	if (!ConfirmButton)
	{
		ConfirmButton = AddFlatButton(TEXT("\u786E\u8BA4"), TEXT("W_ColorPicker_Confirm"));
		if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(ConfirmButton))
		{
			ButtonSlot->SetPadding(FMargin(4.0f));
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleConfirmClicked);
	}
	if (!CancelButton)
	{
		CancelButton = AddFlatButton(TEXT("\u53D6\u6D88"), TEXT("W_ColorPicker_Cancel"));
		if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(CancelButton))
		{
			ButtonSlot->SetPadding(FMargin(4.0f));
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleCancelClicked);
	}

	RefreshPreview();
	// Sliders / text box / buttons are adopted when designed, otherwise created above.
	auto EnsureSlider = [this, Canvas](const TCHAR* Name, float MaxValue) -> USlider*
	{
		if (USlider* Designed = Cast<USlider>(WidgetTree->FindWidget(Name)))
		{
			return Designed;
		}
		return nullptr;
	};

	HueSlider = EnsureSlider(TEXT("W_ColorPicker_Hue"), 360.0f);
	SaturationSlider = EnsureSlider(TEXT("W_ColorPicker_Saturation"), 1.0f);
	ValueSlider = EnsureSlider(TEXT("W_ColorPicker_Value"), 1.0f);
	HexBox = Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Hex")));
	ConfirmButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Confirm")));
	CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Cancel")));
	CloseButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Close")));

	if (HueSlider)
	{
		HueSlider->SetValue(Hue / 360.0f);

	// 色相条用真实彩虹渐变（/Game/UI/Textures/T_HueBar）
	if (HueSlider)
	{
		if (UTexture2D* HueTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Textures/T_HueBar.T_HueBar")))
		{
			FSliderStyle HueStyle = HueSlider->GetWidgetStyle();
			HueStyle.SetNormalBarImage(FSlateBrush());
			HueStyle.NormalBarImage.SetResourceObject(HueTexture);
			HueStyle.NormalBarImage.ImageSize = FVector2D(256.0f, 16.0f);
			HueStyle.NormalBarImage.DrawAs = ESlateBrushDrawType::Image;
			HueStyle.HoveredBarImage = HueStyle.NormalBarImage;
			HueStyle.DisabledBarImage = HueStyle.NormalBarImage;
			HueSlider->SetWidgetStyle(HueStyle);
		}
	}
		HueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHueChanged);
	}
	if (SaturationSlider)
	{
		SaturationSlider->SetValue(Saturation);
		SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleSaturationChanged);
	}
	if (ValueSlider)
	{
		ValueSlider->SetValue(Brightness);
		ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleValueChanged);
	}
	if (HexBox)
	{
		HexBox->OnTextCommitted.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexCommitted);
		HexBox->OnTextChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexChanged);

	// 无论滑块/文本框是自建还是从 WBP 接管，都统一绑定，避免接管时不响应拖动。
	if (HueSlider) { HueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHueChanged); }
	if (SaturationSlider) { SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleSaturationChanged); }
	if (ValueSlider) { ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleValueChanged); }
	if (HexBox)
	{
		HexBox->OnTextCommitted.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexCommitted);
		HexBox->OnTextChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexChanged);
	}
	}
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleCancelClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleCloseClicked);
	}
}

void UMyBondageLifeColorPickerWidget::OpenForColor(int32 InSlot, int32 InChannel, const FString& HexColor)
{
	EnsureWidgetsBound();

	PickerSlot = InSlot;
	PickerChannel = InChannel;
	if (HexColor.Len() != 7) { PendingColor = FLinearColor::White; }

	if (HexColor.Len() == 7 && HexColor.StartsWith(TEXT("#")))
	{
		const FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(HexColor));
		const FLinearColor Hsv = Color.LinearRGBToHSV();
		Hue = Hsv.R;
		Saturation = Hsv.G;
		Brightness = Hsv.B;
		PendingColor = Color;
	}

	LastHueValue = Hue / 360.0f;
	LastSatValue = Saturation;
	LastValValue = Brightness;
	PendingColor = PendingColor;
	bUpdatingSliders = true;
	if (HueSlider) { HueSlider->SetValue(Hue / 360.0f); }
	if (HexColor.Len() != 7 || !HexColor.StartsWith(TEXT("#")))
	{
		// 没有配置颜色时回到默认白色，避免沿用上一次打开时的取值。
		Hue = 0.0f;
		Saturation = 0.0f;
		Brightness = 1.0f;
		if (HueSlider) { HueSlider->SetValue(0.0f); }
		if (SaturationSlider) { SaturationSlider->SetValue(0.0f); }
		if (ValueSlider) { ValueSlider->SetValue(1.0f); }
	}
	if (SaturationSlider) { SaturationSlider->SetValue(Saturation); }
	if (ValueSlider) { ValueSlider->SetValue(Brightness); }
	bUpdatingSliders = false;

	RefreshPreview();
	SetVisibility(ESlateVisibility::Visible);
	{
		const FString UiHex = HexBox ? HexBox->GetText().ToString() : TEXT("<null>");
		const FString Msg = FString::Printf(TEXT("读取到的服务器配色为：%s，计算的配色为：%s，计算的HSV为：(%.1f, %.3f, %.3f)，UI面板中实际显示的RGB值为：%s"),
			*HexColor, *PendingColor.ToFColor(true).ToHex().LeftChop(2), Hue, Saturation, Brightness, *UiHex);
		// UE_LOG(LogMyBondageLife, Log, TEXT("%s"), *Msg);
		// if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, Msg); }
	}
	{
		const FLinearColor DebugHsvColor = FLinearColor::MakeFromHSV8(
			static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f),
			static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f),
			static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));
	}
}

void UMyBondageLifeColorPickerWidget::ClosePicker()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeColorPickerWidget::RefreshPreview()
{
	const FLinearColor Color = PendingColor;

	if (PreviewImage)
	{
		FSlateBrush Brush = FSlateColorBrush(Color);
		PreviewImage->SetBrush(Brush);
	}
	if (HexBox)
	{
		HexBox->SetText(FText::FromString(Color.ToFColor(true).ToHex().LeftChop(2)));
	}
}

void UMyBondageLifeColorPickerWidget::HandleHueChanged(float Value)
{
	if (bUpdatingSliders) { return; }
	float Min = 0.0f, Max = 1.0f;
	if (HueSlider) { Min = HueSlider->GetMinValue(); Max = HueSlider->GetMaxValue(); }
	const float Norm = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
	Hue = Norm * 360.0f;
	PendingColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f), static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f), static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));
	RefreshPreview();
}

void UMyBondageLifeColorPickerWidget::HandleSaturationChanged(float Value)
{
	if (bUpdatingSliders) { return; }
	float Min = 0.0f, Max = 1.0f;
	if (SaturationSlider) { Min = SaturationSlider->GetMinValue(); Max = SaturationSlider->GetMaxValue(); }
	Saturation = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
	PendingColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f), static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f), static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));
	RefreshPreview();
}

void UMyBondageLifeColorPickerWidget::HandleValueChanged(float Value)
{
	if (bUpdatingSliders) { return; }
	float Min = 0.0f, Max = 1.0f;
	if (ValueSlider) { Min = ValueSlider->GetMinValue(); Max = ValueSlider->GetMaxValue(); }
	Brightness = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
	PendingColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f), static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f), static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));
	RefreshPreview();
}

void UMyBondageLifeColorPickerWidget::HandleHexCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	const FString Hex = Text.ToString().TrimStartAndEnd();
	if (Hex.Len() == 7 && Hex.StartsWith(TEXT("#")))
	{
		const FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		const FLinearColor Hsv = Color.LinearRGBToHSV();
		Hue = Hsv.R;
		Saturation = Hsv.G;
		Brightness = Hsv.B;
		PendingColor = Color;
		RefreshPreview();
	}
}

void UMyBondageLifeColorPickerWidget::HandleConfirmClicked()
{
	const FLinearColor Color = FLinearColor::MakeFromHSV8(
		static_cast<uint8>(Hue / 360.0f * 255.0f),
		static_cast<uint8>(Saturation * 255.0f),
		static_cast<uint8>(Brightness * 255.0f));

	const FString Hex = FString::Printf(TEXT("#%s"), *Color.ToFColor(true).ToHex().LeftChop(2));
	const int32 ChosenSlotId = PickerSlot;
	const int32 ChosenChannelId = PickerChannel;

	ClosePicker();
	OnColorCommitted.Broadcast(ChosenSlotId, ChosenChannelId, Hex);
}

void UMyBondageLifeColorPickerWidget::HandleCancelClicked()
{
	ClosePicker();
	OnColorCancelled.Broadcast();
}

void UMyBondageLifeColorPickerWidget::HandleCloseClicked()
{
	HandleCancelClicked();
}

void UMyBondageLifeColorPickerWidget::HandleHexChanged(const FText& Text)
{
	if (bUpdatingHex)
	{
		return;
	}

	const FString Hex = Text.ToString().TrimStartAndEnd();
	if (Hex.Len() != 7 || !Hex.StartsWith(TEXT("#")))
	{
		return;
	}

	for (int32 Index = 1; Index < Hex.Len(); ++Index)
	{
		if (!FChar::IsHexDigit(Hex[Index]))
		{
			return;
		}
	}

	const FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	const FLinearColor Hsv = Color.LinearRGBToHSV();
	Hue = Hsv.R;
	Saturation = Hsv.G;
	Brightness = Hsv.B;

	if (SaturationSlider) { SaturationSlider->SetValue(Saturation); }
	if (ValueSlider) { ValueSlider->SetValue(Brightness); }
	bUpdatingSliders = false;
	if (PreviewImage) { PreviewImage->SetColorAndOpacity(Color); }
	bUpdatingHex = false;
}

void UMyBondageLifeColorPickerWidget::EnsureWidgetsBound()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!HueSlider) { HueSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Hue"))); }
	if (!SaturationSlider) { SaturationSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Saturation"))); }
	if (!ValueSlider) { ValueSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Value"))); }
	if (!HexBox) { HexBox = Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Hex"))); }
	if (!PreviewImage) { PreviewImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Preview"))); }
	if (!ConfirmButton) { ConfirmButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Confirm"))); }
	if (!CancelButton) { CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ColorPicker_Cancel"))); }

	if (HueSlider)
	{
		HueSlider->SetMinValue(0.0f);
		HueSlider->SetMaxValue(1.0f);
		HueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHueChanged);
	}
	if (SaturationSlider)
	{
		SaturationSlider->SetMinValue(0.0f);
		SaturationSlider->SetMaxValue(1.0f);
		SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleSaturationChanged);
	}
	if (ValueSlider)
	{
		ValueSlider->SetMinValue(0.0f);
		ValueSlider->SetMaxValue(1.0f);
		ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleValueChanged);
	}
	if (HexBox)
	{
		HexBox->OnTextCommitted.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexCommitted);
		HexBox->OnTextChanged.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleHexChanged);
	}
	if (ConfirmButton) { ConfirmButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleConfirmClicked); }
	if (CancelButton) { CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeColorPickerWidget::HandleCancelClicked); }
}

void UMyBondageLifeColorPickerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetVisibility() != ESlateVisibility::Visible || bUpdatingSliders)
	{
		return;
	}

	if (!HueSlider && !SaturationSlider && !ValueSlider)
	{
		return;
	}

	bool bChanged = false;

	if (HueSlider)
	{
		const float Min = HueSlider->GetMinValue();
		const float Max = HueSlider->GetMaxValue();
		const float Value = HueSlider->GetValue();
		if (!FMath::IsNearlyEqual(Value, LastHueValue, 0.001f))
		{
			LastHueValue = Value;
			const float Norm = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
			Hue = Norm * 360.0f;
			bChanged = true;
		}
	}
	if (SaturationSlider)
	{
		const float Min = SaturationSlider->GetMinValue();
		const float Max = SaturationSlider->GetMaxValue();
		const float Value = SaturationSlider->GetValue();
		if (!FMath::IsNearlyEqual(Value, LastSatValue, 0.001f))
		{
			LastSatValue = Value;
			Saturation = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
			bChanged = true;
		}
	}
	if (ValueSlider)
	{
		const float Min = ValueSlider->GetMinValue();
		const float Max = ValueSlider->GetMaxValue();
		const float Value = ValueSlider->GetValue();
		if (!FMath::IsNearlyEqual(Value, LastValValue, 0.001f))
		{
			LastValValue = Value;
			Brightness = (Max - Min) > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f) : FMath::Clamp(Value, 0.0f, 1.0f);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		PendingColor = FLinearColor::MakeFromHSV8(
			static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f),
			static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f),
			static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));
		RefreshPreview();
	}
}

void UMyBondageLifeColorPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 统一尺寸：700 x 500，屏幕居中。
	SetAnchorsInViewport(FAnchors(0.5f, 0.5f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetDesiredSizeInViewport(FVector2D(700.0f, 500.0f));
	// 蓝图子类的控件树此时才可用：接管并按名字绑定。
	EnsureWidgetsBound();
}
