// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRestraintStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MyBondageLifeCharacter.h"

TSharedRef<SWidget> UMyBondageLifeRestraintStatusWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeRestraintStatusWidget::BuildLayout()
{
	GagIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_StatusGag")));
	ArmIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_StatusArm")));
	LegIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_StatusLeg")));
	CantMoveIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_StatusCantMove")));

	if (!GagIcon && !ArmIcon && !LegIcon && !CantMoveIcon)
	{
		// Default look: a shrink-wrapped column of four 56px icons, stacked top-left.
		UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!Column)
		{
			Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusRoot"));
			WidgetTree->RootWidget = Column;
		}

		auto MakeIcon = [this, Column](const TCHAR* Name, const TCHAR* Caption, const FLinearColor& CaptionColor, UTexture2D* Texture) -> UImage*
		{
			// Icon + red caption are one unit: a column that pulses as a whole.
			UVerticalBox* Unit = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (UVerticalBoxSlot* UnitSlot = Column->AddChildToVerticalBox(Unit))
			{
				UnitSlot->SetHorizontalAlignment(HAlign_Left);
				UnitSlot->SetPadding(FMargin(0.0f, 6.0f));
			}

			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Size->SetWidthOverride(112.0f);
			Size->SetHeightOverride(112.0f);
			if (UVerticalBoxSlot* SizeSlot = Unit->AddChildToVerticalBox(Size))
			{
				SizeSlot->SetHorizontalAlignment(HAlign_Center);
			}

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
			if (Texture)
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			Size->AddChild(Icon);

			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Text->SetText(FText::FromString(Caption));
			Text->SetColorAndOpacity(FSlateColor(CaptionColor));
			Text->SetJustification(ETextJustify::Center);
			Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
			{
				FSlateFontInfo Font = Text->GetFont();
				Font.Size = 16;
				Text->SetFont(Font);
			}
			if (UVerticalBoxSlot* TextSlot = Unit->AddChildToVerticalBox(Text))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Center);
			}

			Unit->SetVisibility(ESlateVisibility::Collapsed);
			Unit->SetRenderScale(FVector2D(1.0f, 1.0f));
			Units.Add(Unit);
			return Icon;
		};

		const FLinearColor CaptionRed(0.95f, 0.15f, 0.15f);

		if (!GagIcon)
		{
			GagIcon = MakeIcon(TEXT("W_StatusGag"), TEXT("\u8BF4\u8BDD\u53D7\u9650"), CaptionRed, GagTexture);
		}
		if (!ArmIcon)
		{
			ArmIcon = MakeIcon(TEXT("W_StatusArm"), TEXT("\u53CC\u624B\u53D7\u9650"), CaptionRed, ArmTexture);
		}
		if (!LegIcon)
		{
			LegIcon = MakeIcon(TEXT("W_StatusLeg"), TEXT("\u53CC\u811A\u53D7\u9650"), CaptionRed, LegTexture);
		}
		if (!CantMoveIcon)
		{
			CantMoveIcon = MakeIcon(TEXT("W_StatusCantMove"), TEXT("\u7981\u6B62\u79FB\u52A8"), CaptionRed, CantMoveTexture);
		}
	}

	bLayoutBuilt = true;
}

void UMyBondageLifeRestraintStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bLayoutBuilt)
	{
		return;
	}

	UpdateStates(InDeltaTime);
}

void UMyBondageLifeRestraintStatusWidget::UpdateStates(float InDeltaTime)
{
	const AMyBondageLifeCharacter* Character = Cast<AMyBondageLifeCharacter>(GetOwningPlayerPawn());
	if (!Character)
	{
		return;
	}

	const bool bGag = Character->GetGagActive();
	const bool bArm = Character->GetArmActive();
	const bool bLeg = Character->GetLegActive();
	const bool bCantMove = Character->GetCantMoveActive();

	const bool bAny = bGag || bArm || bLeg || bCantMove;
	PulseTime = bAny ? PulseTime + InDeltaTime : 0.0f;

	// Breathing: 1.0 +- 0.1 (0.9 .. 1.1).
	const float Pulse = 1.0f + 0.1f * FMath::Sin(PulseTime * 4.0f);

	auto Apply = [Pulse](UImage* Icon, bool bActive)
	{
		if (!Icon)
		{
			return;
		}

		Icon->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bActive)
		{
			Icon->SetRenderScale(FVector2D(Pulse, Pulse));
		}
	};

	Apply(GagIcon, bGag);
	Apply(ArmIcon, bArm);
	Apply(LegIcon, bLeg);
	Apply(CantMoveIcon, bCantMove);
}
