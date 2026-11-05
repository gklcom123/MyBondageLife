// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeEmoteWidgets.generated.h"

class UButton;
class UImage;
class UScrollBox;
class UTextBlock;
class UWidgetTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeEmotePicked, FName, EmoteId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeEmoteCloseRequested);

/** One row of the emote list (WBP_Emote). */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeEmoteRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Emote")
	void SetEmote(FName InEmoteId, const FString& InName, class UTexture2D* Icon);

	FName GetEmoteId() const { return EmoteId; }

	UPROPERTY(BlueprintAssignable, Category = "Emote")
	FMyBondageLifeEmotePicked OnPicked;

protected:

	UFUNCTION()
	void HandleClicked();

	FName EmoteId;
	UPROPERTY(Transient) TObjectPtr<UButton> Button;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UImage> IconImage;
	bool bLayoutBuilt = false;
};

/** Emote panel (WBP_EmoteList): rows created from DT_Emote into W_EmoteScrollBox. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeEmoteListWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Emote")
	void PopulateEmotes();

	UPROPERTY(BlueprintAssignable, Category = "Emote")
	FMyBondageLifeEmotePicked OnEmotePicked;

	UPROPERTY(BlueprintAssignable, Category = "Emote")
	FMyBondageLifeEmoteCloseRequested OnCloseRequested;

protected:

	UFUNCTION()
	void HandleRowPicked(FName EmoteId);

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<class UPanelWidget> EmoteContainer;
	UPROPERTY(Transient) TArray<TObjectPtr<UMyBondageLifeEmoteRowWidget>> Rows;
	bool bLayoutBuilt = false;
};
