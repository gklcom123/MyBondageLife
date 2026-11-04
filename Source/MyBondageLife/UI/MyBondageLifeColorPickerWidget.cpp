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

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
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
	PickerSlot = InSlot;
	PickerChannel = InChannel;

	if (HexColor.Len() == 7 && HexColor.StartsWith(TEXT("#")))
	{
		const FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(HexColor));
		const FLinearColor Hsv = Color.LinearRGBToHSV();
		Hue = Hsv.R;
		Saturation = Hsv.G;
		Brightness = Hsv.B;
		Hue *= 360.0f;
	}

	if (HueSlider) { HueSlider->SetValue(Hue / 360.0f); }
	if (SaturationSlider) { SaturationSlider->SetValue(Saturation); }
	if (ValueSlider) { ValueSlider->SetValue(Brightness); }

	RefreshPreview();
	SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeColorPickerWidget::ClosePicker()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeColorPickerWidget::RefreshPreview()
{
	const FLinearColor Color = FLinearColor::MakeFromHSV8(
		static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 360.0f) / 360.0f * 255.0f),
		static_cast<uint8>(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8>(FMath::Clamp(Brightness, 0.0f, 1.0f) * 255.0f));

	if (PreviewImage)
	{
		PreviewImage->SetColorAndOpacity(Color);
	}
	if (HexBox)
	{
		HexBox->SetText(FText::FromString(Color.ToFColor(true).ToHex().LeftChop(2)));
	}
}

void UMyBondageLifeColorPickerWidget::HandleHueChanged(float Value)
{
	Hue = FMath::Clamp(Value, 0.0f, 1.0f) * 360.0f;
	RefreshPreview();
}

void UMyBondageLifeColorPickerWidget::HandleSaturationChanged(float Value)
{
	Saturation = FMath::Clamp(Value, 0.0f, 1.0f);
	RefreshPreview();
}

void UMyBondageLifeColorPickerWidget::HandleValueChanged(float Value)
{
	Brightness = FMath::Clamp(Value, 0.0f, 1.0f);
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
		Hue *= 360.0f;
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
	if (PreviewImage) { PreviewImage->SetColorAndOpacity(Color); }
	bUpdatingHex = false;
}
