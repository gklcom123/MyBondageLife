// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRestraintSlotsWidget.h"
#include "MyBondageLifeUIStyle.h"

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
#include "MyBondageLifePlayerController.h"

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
	RefreshSlaveControlPanel();
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
	RefreshSlaveControlPanel();
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

			// 染色按钮：W_ChangeColor_{槽}_{1..3}
			for (int32 Channel = 1; Channel <= 3; ++Channel)
			{
				const FString ColorName = FString::Printf(TEXT("W_ChangeColor_%d_%d"), SlotIndex, Channel);
				if (UButton* ColorButton = Cast<UButton>(WidgetTree->FindWidget(*ColorName)))
				{
					ColorButtons.Add(ColorName, ColorButton);
					ColorButton->SetVisibility(ESlateVisibility::Collapsed);

					UMyBondageLifeRestraintColorProxy* ColorProxy = NewObject<UMyBondageLifeRestraintColorProxy>(this);
					ColorProxy->Owner = this;
					ColorProxy->Slot = SlotIndex;
					ColorProxy->Channel = Channel;
					ColorProxies.Add(ColorProxy);
					ColorButton->OnClicked.AddUniqueDynamic(ColorProxy, &UMyBondageLifeRestraintColorProxy::HandleClicked);
				}
			}

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
			LabelFont.FontObject = MyBondageLifeUI::LoadUIFont();
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
	// 设计里若已存在名为 W_Hint 的控件（SizeBox 或 TextBlock 皆可），完全交给蓝图；
	// 只有不存在时才在代码里创建，并放在最后一行槽位下方。
	UWidget* ExistingHint = WidgetTree ? WidgetTree->FindWidget(TEXT("W_Hint")) : nullptr;
	USizeBox* HintSize = Cast<USizeBox>(ExistingHint);
	UTextBlock* HintText = Cast<UTextBlock>(ExistingHint);

	if (!HintSize && !HintText)
	{
		HintSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("W_Hint"));
		HintSize->SetWidthOverride(1000.0f);
		if (UCanvasPanelSlot* HintSlot = RootCanvas->AddChildToCanvas(HintSize))
		{
			HintSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			HintSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			// 3 行 × CellStepY + 32px 间距，落在最后一行槽位下方。
			HintSlot->SetPosition(FVector2D(CellOriginX, CellOriginY + 3.0f * CellStepY + 32.0f));
			HintSlot->SetAutoSize(true);
		}

		HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HintText->SetText(FText::FromString(TEXT("\u624B\u81C2\u5931\u53BB\u81EA\u7531\u72B6\u6001\u4E0B\u65E0\u6CD5\u7ED9\u522B\u4EBA\u6DFB\u52A0\u62D8\u675F\uFF0C\u4E14\u53EA\u80FD\u7531\u652F\u914D\u8005\u7ED9\u5176\u4F59\u89D2\u8272\u914D\u7F6E\u62D8\u675F\uFF0C\u6216\u8005\u7531\u8F6C\u6362\u8005\u7ED9\u8F6C\u6362\u8005\u3001\u81E3\u670D\u8005\u3001\u5974\u96B6\u53D1\u8D77\u62D8\u675F")));
		HintText->SetAutoWrapText(true);
		HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.42f)));
		FSlateFontInfo HintFont = HintText->GetFont();
		HintFont.Size = 16;
		HintFont.FontObject = MyBondageLifeUI::LoadUIFont();
		HintText->SetFont(HintFont);
		HintSize->AddChild(HintText);
	}
	else if (HintSize && HintSize->GetChildrenCount() == 0)
	{
		UTextBlock* Child = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Child->SetText(FText::FromString(TEXT("\u624B\u81C2\u5931\u53BB\u81EA\u7531\u72B6\u6001\u4E0B\u65E0\u6CD5\u7ED9\u522B\u4EBA\u6DFB\u52A0\u62D8\u675F\uFF0C\u4E14\u53EA\u80FD\u7531\u652F\u914D\u8005\u7ED9\u5176\u4F59\u89D2\u8272\u914D\u7F6E\u62D8\u675F\uFF0C\u6216\u8005\u7531\u8F6C\u6362\u8005\u7ED9\u8F6C\u6362\u8005\u3001\u81E3\u670D\u8005\u3001\u5974\u96B6\u53D1\u8D77\u62D8\u675F")));
		Child->SetAutoWrapText(true);
		Child->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.42f)));
		HintSize->AddChild(Child);
	}


	OnLayoutBuilt();
	RefreshSlots();
	RefreshSlaveControlPanel();
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
				SlotIcons[SlotIndex]->SetBrushFromTexture(FMyBondageLifeRestraintCatalog::LoadEmptySlotIcon(), true);
			}
		}

		// 染色按钮：有穿戴才显示，材质有 ColorN 才可点
		const int32 ColorMask = bWorn
			? FMyBondageLifeRestraintCatalog::GetColorChannelMask(FName(*ItemId))
			: 0;
		for (int32 Channel = 1; Channel <= 3; ++Channel)
		{
			const FString ColorName = FString::Printf(TEXT("W_ChangeColor_%d_%d"), SlotIndex, Channel);
			if (TObjectPtr<UButton>* FoundButton = ColorButtons.Find(ColorName))
			{
				if (UButton* ColorButton = FoundButton->Get())
				{
					ColorButton->SetVisibility(bWorn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
					ColorButton->SetIsEnabled(bWorn && (ColorMask & (1 << (Channel - 1))) != 0);
				}
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
				TEXT("/Game/2DAssest/UI/TimePadLock.TimePadLock"),
				TEXT("/Game/2DAssest/UI/LoversLock.LoversLock") };
			UTexture2D* LockIcon = nullptr;
			if (Lock.LockType >= 1 && Lock.LockType <= 6)
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
				SlotLockBadges[SlotIndex]->SetBrushFromTexture(FMyBondageLifeRestraintCatalog::LoadEmptySlotIcon(), true);
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
	RefreshSlaveControlPanel();
}

void UMyBondageLifeRestraintColorProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->HandleColorClicked(Slot, Channel);
	}
}

void UMyBondageLifeRestraintSlotsWidget::HandleColorClicked(int32 SlotIndex, int32 Channel)
{
	OnColorRequested.Broadcast(SlotIndex, Channel);
}

void UMyBondageLifeRestraintSlotsWidget::RefreshSlaveControlPanel()
{
	if (!WidgetTree)
	{
		return;
	}

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;

	// 状态变化时自动刷新按钮（绑定一次）。
	if (Account && !bTractionBound)
	{
		Account->OnTractionChanged.AddUniqueDynamic(this, &UMyBondageLifeRestraintSlotsWidget::HandleTractionStateChanged);
		bTractionBound = true;
	}

	UWidget* ControlPanel = WidgetTree->FindWidget(TEXT("W_Overlay_SlaveControlPanel"));
	UButton* TractionButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_ControlBtn_Traction")));

	const bool bShow = !ViewAccount.IsEmpty() && Account
		&& Account->GetContractRoleWith(ViewAccount) == EMyBondageLifeContractRole::Master;

	if (ControlPanel)
	{
		ControlPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (TractionButton)
	{
		const FString CollarId = Account ? Account->GetViewedRestraint(1) : FString();
		const bool bHasCollar = !CollarId.IsEmpty();

		// 目标被关在固定式家具里时不能牵引。
	bool bInFurnitureNow = false;
	if (Account && !ViewAccount.IsEmpty())
	{
		for (const FMyBondageLifeFurnitureState& F : Account->GetFurnitureStates())
		{
			if (!F.Occupant.IsEmpty() && F.Occupant.Equals(ViewAccount, ESearchCase::IgnoreCase))
			{
				bInFurnitureNow = true;
				break;
			}
		}
	}

	const bool bCanTraction = bHasCollar && !bInFurnitureNow;
		const bool bTractioned = Account && !ViewAccount.IsEmpty() && Account->GetTractionFollowers().Contains(ViewAccount);

		TractionButton->SetIsEnabled(bShow && bCanTraction);
		TractionButton->SetToolTipText(FText::FromString(bHasCollar ? FString() : TEXT("需要目标穿戴项圈")));

		if (UTextBlock* Label = Cast<UTextBlock>(TractionButton->GetChildAt(0)))
		{
			Label->SetText(FText::FromString(bTractioned ? TEXT("正在被牵引中") : TEXT("牵引")));
		}

		if (!TractionButton->OnClicked.IsAlreadyBound(this, &UMyBondageLifeRestraintSlotsWidget::HandleTractionClicked))
		{
			TractionButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRestraintSlotsWidget::HandleTractionClicked);
		}
	}
}

void UMyBondageLifeRestraintSlotsWidget::HandleTractionStateChanged()
{
	RefreshSlaveControlPanel();
}

void UMyBondageLifeRestraintSlotsWidget::HandleTractionClicked()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account || ViewAccount.IsEmpty())
	{
		return;
	}

	// 点击时再校验一次：必须是主人且目标戴项圈。
	if (Account->GetContractRoleWith(ViewAccount) != EMyBondageLifeContractRole::Master)
	{
		return;
	}

	const bool bTractioned = Account->GetTractionFollowers().Contains(ViewAccount);
	if (!bTractioned && Account->GetViewedRestraint(1).IsEmpty())
	{
		return;
	}

	if (AMyBondageLifePlayerController* PC = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		PC->ToggleTractionFor(ViewAccount);
	}

	RefreshSlots();
}
