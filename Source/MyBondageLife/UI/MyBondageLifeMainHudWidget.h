// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeMainHudWidget.generated.h"

class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UWidgetAnimation;

/**
 *  Root HUD of the in-room screen: chat box, message entry, the C/V/B entries and the
 *  room info in the top-right corner.
 *
 *  Designed in /Game/UI/WBP_MainHUD. Put empty containers with these names where the
 *  matching element should live; the game adds the real widget inside them:
 *
 *    W_ChatHost         chat log + input
 *    W_MessageHost      message-center entry (icon + red dot)
 *    W_InfoEntryHost    "个人信息（C）" entry
 *    W_FriendsEntryHost "好友列表（V）" entry
 *    W_WardrobeEntryHost"衣柜（B）" entry
 *    W_RoomInfoHost     room name / player count
 *
 *  Anything missing falls back to the old fixed screen position, so the HUD keeps
 *  working while the layout is still being designed.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeMainHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Puts Widget inside the named host panel. Returns false when that host is missing. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	bool HostWidget(FName HostName, UUserWidget* Widget);

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool HasHost(FName HostName) const;

	/** 性欲值进度：Value01 为 0..1，Percent 为显示百分比，bPlayAnim 时播放 SexProgressBarAnmi。 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetSexProgress(float Value01, float Percent, bool bPlayAnim);

	/** 拘束穿戴进度：0..1，LabelText 由调用方决定。 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowBondageProgress(float Value01, const FString& LabelText);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideBondageProgress();

	UPROPERTY(Transient)
	TObjectPtr<UWidget> BondageOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BondageProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BondageProgressLabel;

protected:

	void BuildLayout();
	UPanelWidget* FindHost(FName HostName) const;

	/** Set to true once the blueprint layout is final: the code stops overriding host positions. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	bool bUseDesignedHostLayout = true;

	TMap<FName, TObjectPtr<UPanelWidget>> Hosts;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> SexProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SexProgressLabel;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> SexProgressAnimation;
	bool bLayoutBuilt = false;
};

