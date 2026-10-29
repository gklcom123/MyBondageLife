// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeWardrobeWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "MyBondageLife.h"

/**
 *  Sound played when "上锁" is pressed.
 *
 *  The cue is looked up lazily so the widget still works (silently) before the
 *  asset is imported, and so moving the asset only needs a path update here.
 */
void UMyBondageLifeWardrobeWidget::PlayLockCue()
{
	static const TCHAR* CandidatePaths[] = {
		TEXT("/Game/2DAssest/UI/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/2DAssest/Sound/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/Characters/Sound/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/UI/Sound/UI_Lock_Cue.UI_Lock_Cue"),
	};

	for (const TCHAR* Path : CandidatePaths)
	{
		if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, Path))
		{
			UGameplayStatics::PlaySound2D(this, Cue);
			return;
		}
	}

	UE_LOG(LogMyBondageLife, Verbose, TEXT("Lock: UI_Lock_Cue not found, no sound played."));
}
