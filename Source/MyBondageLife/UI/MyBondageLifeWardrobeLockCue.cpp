// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeWardrobeWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "MyBondageLife.h"
#include "Sound/SoundBase.h"

/**
 *  Sound played when "上锁" is pressed.
 *
 *  Played through Slate so it behaves as a real UI sound (own volume, unaffected by
 *  3D attenuation / listener position). Falls back to PlaySound2D when Slate is not
 *  available.
 */
void UMyBondageLifeWardrobeWidget::PlayLockCue()
{
	static const TCHAR* CandidatePaths[] = {
		TEXT("/Game/Characters/Sound/UI/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/2DAssest/UI/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/Characters/Sound/UI_Lock_Cue.UI_Lock_Cue"),
		TEXT("/Game/2DAssest/Sound/UI_Lock_Cue.UI_Lock_Cue"),
	};

	for (const TCHAR* Path : CandidatePaths)
	{
		USoundBase* Cue = LoadObject<USoundBase>(nullptr, Path);
		if (!Cue)
		{
			continue;
		}

		if (FSlateApplication::IsInitialized())
		{
			FSlateSound SlateSound;
			SlateSound.SetResourceObject(Cue);
			FSlateApplication::Get().PlaySound(SlateSound);
		}
		else
		{
			UGameplayStatics::PlaySound2D(this, Cue);
		}
		return;
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Lock: UI_Lock_Cue not found under /Game/Characters/Sound/UI."));
}
