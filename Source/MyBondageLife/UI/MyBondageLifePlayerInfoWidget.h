// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifePlayerInfoWidget.generated.h"

class AMyBondageLifeCharacter;
class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UHorizontalBox;
class UVerticalBox;

/** Raised when the panel is opened or closed, so the controller can gate input. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifePlayerInfoOpenChanged, bool, bIsOpen);

/** Raised when the player asks to add the displayed character as a friend. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeAddFriendRequested, const AMyBondageLifeCharacter*, Target);

/**
 *  Personal info: the bottom "个人信息(C)" entry, the panel it opens, and the
 *  "按F打开信息以交互" hint shown when another player is close by.
 *
 *  The panel shows any character (own or another player) from the replicated
 *  identity on AMyBondageLifeCharacter.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifePlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifePlayerInfoWidget(const FObjectInitializer& ObjectInitializer);

	/** Shows or hides the panel and refreshes its contents. */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void SetPanelOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void TogglePanel();

	UFUNCTION(BlueprintPure, Category = "PlayerInfo")
	bool IsPanelOpen() const;

	/** Fills the panel from the given character (null clears the fields). */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void ShowCharacterInfo(const AMyBondageLifeCharacter* Character);

	/** Shows the "按[F]与[xxx]进行交互" hint for a character (null hides it). */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void SetInteractionHintTarget(const AMyBondageLifeCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "PlayerInfo")
	bool IsInteractionHintVisible() const;

	UPROPERTY(BlueprintAssignable, Category = "PlayerInfo")
	FMyBondageLifePlayerInfoOpenChanged OnPanelOpenChanged;

	UPROPERTY(BlueprintAssignable, Category = "PlayerInfo")
	FMyBondageLifeAddFriendRequested OnAddFriendRequested;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	void BuildLayout();

	/** Rewrites the three lines from the cached character. */
	void RefreshContents();

	UFUNCTION()
	void HandleOpenButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleAddFriendClicked();

	/** True when the panel shows somebody else, i.e. the add-friend button applies. */
	bool IsDisplayingOtherPlayer() const;

	/** True when the displayed character is already in the local friend list. */
	bool IsDisplayedCharacterFriend() const;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AddFriendButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AddFriendLabel;

	/** Head icon used by the bottom entry (Content/2DAssest/UI/UI_HeadIcon). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> HeadIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	//~ Bottom entry
	UPROPERTY(Transient)
	TObjectPtr<UButton> OpenButton;

	//~ Panel
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameValue;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ClassValue;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> JoinedValue;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	//~ Proximity hint: "按[F]与[" + <name> + "]进行交互"
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> InteractionHintRow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InteractionHintName;

	/** Character the hint currently names, so the text is only touched on change. */
	UPROPERTY(Transient)
	TWeakObjectPtr<const AMyBondageLifeCharacter> HintTarget;

	/** Cached name shown in the hint, compared to detect late replication. */
	FString HintName;

	/** Character currently shown in the panel. */
	UPROPERTY(Transient)
	TWeakObjectPtr<const AMyBondageLifeCharacter> DisplayedCharacter;
};
