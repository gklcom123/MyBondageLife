// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRestraintStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
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

		auto MakeIcon = [this, Column](const TCHAR* Name, UTexture2D* Texture) -> UImage*
		{
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Size->SetWidthOverride(56.0f);
			Size->SetHeightOverride(56.0f);
			if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Size))
			{
				Slot->SetHorizontalAlignment(HAlign_Left);
				Slot->SetPadding(FMargin(0.0f, 4.0f));
			}

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
			if (Texture)
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			Size->AddChild(Icon);
			return Icon;
		};

		if (!GagIcon)
		{
			GagIcon = MakeIcon(TEXT("W_StatusGag"), GagTexture);
		}
		if (!ArmIcon)
		{
			ArmIcon = MakeIcon(TEXT("W_StatusArm"), ArmTexture);
		}
		if (!LegIcon)
		{
			LegIcon = MakeIcon(TEXT("W_StatusLeg"), LegTexture);
		}
		if (!CantMoveIcon)
		{
			CantMoveIcon = MakeIcon(TEXT("W_StatusCantMove"), CantMoveTexture);
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
