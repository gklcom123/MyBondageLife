// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeWardrobeWidget.h"

#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeHairCatalog.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"

/**
 *  Lock countdown text.
 *
 *  Kept in its own translation unit: the picker title is rewritten once per second
 *  while a timed lock is shown, and this file is replaced wholesale by the tooling.
 */
void UMyBondageLifeWardrobeWidget::UpdateLockCountdown()
{
	if (!PickerTitle)
	{
		return;
	}

	static const TCHAR* LockNames[] = {
		TEXT(""),
		TEXT("\u4E3B\u4EBA\u9501"),
		TEXT("\u4E3B\u4EBA\u8BA1\u65F6\u9501"),
		TEXT("\u666E\u901A\u6302\u9501"),
		TEXT("\u5BC6\u7801\u9501"),
		TEXT("\u8BA1\u65F6\u9501")
	};

	const int32 LockIndex = FMath::Clamp(PickerLockType, 0, 5);
	const FString SlotName = FMyBondageLifeRestraintCatalog::GetSlotName(PickerSlot);
	const FTimespan Remaining = PickerLockExpiry - FDateTime::UtcNow();

	if (Remaining.GetTotalSeconds() <= 0.0)
	{
		PickerTitle->SetText(FText::FromString(FString::Printf(
			TEXT("%s \uFF1A\u5DF2\u4E0A\u9501\uFF08%s\uFF09 \u00B7 \u5DF2\u5230\u671F"),
			*SlotName, LockNames[LockIndex])));
		StopLockCountdown();
		return;
	}

	const int32 TotalSeconds = static_cast<int32>(Remaining.GetTotalSeconds());
	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds % 3600) / 60;
	const int32 Seconds = TotalSeconds % 60;

	const FString Countdown = (Hours >= 24)
		? FString::Printf(TEXT("%d \u5929 %02d:%02d:%02d"), Hours / 24, Hours % 24, Minutes, Seconds)
		: FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);

	PickerTitle->SetText(FText::FromString(FString::Printf(
		TEXT("%s \uFF1A\u5DF2\u4E0A\u9501\uFF08%s\uFF09 \u00B7 \u5269\u4F59 %s"),
		*SlotName, LockNames[LockIndex], *Countdown)));
}

void UMyBondageLifeWardrobeWidget::StopLockCountdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LockCountdownTimer);
	}
}
