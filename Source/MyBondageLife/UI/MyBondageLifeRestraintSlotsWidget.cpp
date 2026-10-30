// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRestraintSlotsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"

namespace
{
	constexpr float CellWidth = 130.0f;
	constexpr float CellHeight = 152.0f;
	constexpr float CellStepX = 150.0f;
	constexpr float CellStepY = 170.0f;
	constexpr float CellOriginX = 56.0f;
	constexpr float CellOriginY = 0.0f;
	constexpr float HintY = 526.0f;
}

void UMyBondageLifeRestraintSlotClickProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->OnSlotClicked.Broadcast(Slot);
	}
}

void UMyBondageLifeRestraintSlotsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Account->OnRestraintsChanged.AddUniqueDynamic(this, &UMyBondageLifeRestraintSlotsWidget::HandleRestraintsChanged);
	}

	RefreshSlots();
}

TSharedRef<SWidget> UMyBondageLifeRestraintSlotsWidget::RebuildWidget()
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

void UMyBondageLifeRestraintSlotsWidget::SetViewAccount(const FString& InAccount)
{
	ViewAccount = InAccount;
	RefreshSlots();
}

void UMyBondageLifeRestraintSlotsWidget::BuildLayout()
{
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SlotsRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// A designed slot (W_SlotButton_N + W_SlotIcon_N + W_SlotLabel_N + W_SlotBadge_N)
	// wins over the generated one; missing slots are still built in code.
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const FString Suffix = FString::Printf(TEXT("_%d"), SlotIndex);
		UButton* DesignedButton = Cast<UButton>(WidgetTree->FindWidget(*(FString(TEXT("W_SlotButton")) + Suffix)));
		if (DesignedButton)
		{
			SlotButtons.Add(DesignedButton);
			SlotIcons.Add(Cast<UImage>(WidgetTree->FindWidget(*(FString(TEXT("W_SlotIcon")) + Suffix))));
			SlotLabels.Add(Cast<UTextBlock>(WidgetTree->FindWidget(*(FString(TEXT("W_SlotLabel")) + Suffix))));
			SlotLockBadges.Add(Cast<UImage>(WidgetTree->FindWidget(*(FString(TEXT("W_SlotBadge")) + Suffix))));

			UMyBondageLifeRestraintSlotClickProxy* Proxy =
				NewObject<UMyBondageLifeRestraintSlotClickProxy>(this);
			Proxy->Owner = this;
			Proxy->Slot = SlotIndex;
			ClickProxies.Add(Proxy);
			DesignedButton->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeRestraintSlotClickProxy::HandleClicked);
			continue;
		}
		const int32 Col = SlotIndex / 3;
		const int32 Row = SlotIndex % 3;

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CellSize->SetWidthOverride(CellWidth);
		CellSize->SetHeightOverride(CellHeight);
		if (UCanvasPanelSlot* CellSlot = RootCanvas->AddChildToCanvas(CellSize))
		{
			CellSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			CellSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			CellSlot->SetPosition(FVector2D(
				CellOriginX + Col * CellStepX,
				CellOriginY + Row * CellStepY));
			CellSlot->SetAutoSize(true);
		}

		UButton* SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UOverlay* CellOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		CellSize->AddChild(CellOverlay);
		CellOverlay->AddChildToOverlay(SlotButton);
		SlotButtons.Add(SlotButton);

		UMyBondageLifeRestraintSlotClickProxy* Proxy =
			NewObject<UMyBondageLifeRestraintSlotClickProxy>(this);
		Proxy->Owner = this;
		Proxy->Slot = SlotIndex;
		ClickProxies.Add(Proxy);
		SlotButton->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeRestraintSlotClickProxy::HandleClicked);

		UImage* LockBadge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		LockBadge->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* BadgeSlot = CellOverlay->AddChildToOverlay(LockBadge))
		{
			BadgeSlot->SetHorizontalAlignment(HAlign_Right);
			BadgeSlot->SetVerticalAlignment(VAlign_Top);
		}
		SlotLockBadges.Add(LockBadge);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		SlotButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(CellWidth);
		IconSize->SetHeightOverride(CellWidth);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		IconSize->AddChild(Icon);
		SlotIcons.Add(Icon);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex)));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.35f, 0.38f)));
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

	// Hint line under the grid.
	USizeBox* HintSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("W_Hint"));
	HintSize->SetWidthOverride(1000.0f);
	if (UCanvasPanelSlot* HintSlot = RootCanvas->AddChildToCanvas(HintSize))
	{
		HintSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		HintSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		HintSlot->SetPosition(FVector2D(CellOriginX, HintY));
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

	OnLayoutBuilt();
	RefreshSlots();
}

void UMyBondageLifeRestraintSlotsWidget::RefreshSlots()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	const bool bOther = !ViewAccount.IsEmpty();

	for (int32 SlotIndex = 0; SlotIndex < SlotButtons.Num(); ++SlotIndex)
	{
		const FString ItemId = !Account
			? FString()
			: (bOther ? Account->GetViewedRestraint(SlotIndex) : Account->GetEquippedRestraint(SlotIndex));
		const bool bWorn = !ItemId.IsEmpty();

		if (SlotLabels.IsValidIndex(SlotIndex) && SlotLabels[SlotIndex])
		{
			SlotLabels[SlotIndex]->SetText(FText::FromString(bWorn
				? FMyBondageLifeRestraintCatalog::GetDisplayName(FName(*ItemId))
				: FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex)));
			SlotLabels[SlotIndex]->SetColorAndOpacity(FSlateColor(bWorn
				? FLinearColor(0.45f, 0.2f, 0.6f)
				: FLinearColor(0.35f, 0.35f, 0.38f)));
		}

		if (SlotIcons.IsValidIndex(SlotIndex) && SlotIcons[SlotIndex])
		{
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

		if (SlotLockBadges.IsValidIndex(SlotIndex) && SlotLockBadges[SlotIndex])
		{
			const FMyBondageLifeRestraintLock Lock = !Account
				? FMyBondageLifeRestraintLock()
				: (bOther ? Account->GetViewedLock(SlotIndex) : Account->GetRestraintLock(SlotIndex));

			static const TCHAR* LockIcons[] = {
				TEXT(""), TEXT("/Game/2DAssest/UI/MasterLock.MasterLock"),
				TEXT("/Game/2DAssest/UI/MasterTimeLock.MasterTimeLock"),
				TEXT("/Game/2DAssest/UI/NormalLock.NormalLock"),
				TEXT("/Game/2DAssest/UI/PasswordLock.PasswordLock"),
				TEXT("/Game/2DAssest/UI/TimePadLock.TimePadLock") };

			UTexture2D* LockIcon = nullptr;
			if (Lock.LockType >= 1 && Lock.LockType <= 5)
			{
				LockIcon = LoadObject<UTexture2D>(nullptr, LockIcons[Lock.LockType]);
			}

			if (LockIcon)
			{
				SlotLockBadges[SlotIndex]->SetBrushFromTexture(LockIcon, true);
				SlotLockBadges[SlotIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				SlotLockBadges[SlotIndex]->SetBrush(FSlateBrush());
				SlotLockBadges[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UMyBondageLifeRestraintSlotsWidget::HandleSlotClicked(int32 SlotIndex)
{
	OnSlotClicked.Broadcast(SlotIndex);
}

void UMyBondageLifeRestraintSlotsWidget::HandleRestraintsChanged()
{
	RefreshSlots();
}
