// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Network/MyBondageLifeLanTypes.h"
#include "Network/MyBondageLifeServerRoom.h"
#include "MainSceneWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UMyBondageLifeRoomRowAction;

/** Which room list the browser is showing. */
enum class ERoomBrowserMode : uint8
{
	/** Cloud / EOS rooms provided by the room service. */
	Server,
	/** Rooms announced on the local network (test mode, no backend needed). */
	Local
};

/**
 *  Main menu, shown right after login.
 *
 *  Buttons: 创建房间 (host a LAN listen server), 加入游戏 (browse rooms found on
 *  the LAN and travel to one), 游戏设置 and 离开游戏 (placeholders for now).
 */
UCLASS()
class MYBONDAGELIFE_API UMainSceneWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMainSceneWidget(const FObjectInitializer& ObjectInitializer);

	/** Hosts a room through the room service (EOS). */
	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void CreateRoom();

	/** Hosts a plain LAN room: listen server plus UDP announcement. */
	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void CreateLocalRoom();

	/** Shows the room browser listing cloud rooms. */
	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void OpenRoomBrowser();

	/** Shows the room browser listing rooms found on the local network. */
	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void OpenLocalRoomBrowser();

	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void CloseRoomBrowser();

	UFUNCTION(BlueprintPure, Category = "MainScene")
	bool IsRoomBrowserOpen() const;

	/** Travels to the room at the given index of the currently displayed list. */
	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void JoinRoomByIndex(int32 RoomIndex);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();
	void BuildRoomBrowser();
	void BuildConnectingOverlay();
	void ShowConnecting();
	void HideConnecting();
	void RefreshAccountLabel();

	/** Recreates one row per displayed room, for the current browser mode. */
	void RebuildRoomRows();

	/** Recreates the rows from the LAN discovery list. */
	void RebuildLocalRoomRows();

	/** Travels to a room discovered on the LAN. */
	void JoinLocalRoomByIndex(int32 RoomIndex);

	/** Asks the room service for the current list. */
	void RefreshRoomList();

	/** Shared helper: fixed size button with a centered label. */
	UButton* CreateMenuButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive);

	UFUNCTION()
	void HandleCreateRoomClicked();

	UFUNCTION()
	void HandleJoinGameClicked();

	UFUNCTION()
	void HandleCreateLocalRoomClicked();

	UFUNCTION()
	void HandleJoinLocalRoomClicked();

	UFUNCTION()
	void HandleSettingsClicked();



	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleCloseBrowserClicked();

	UFUNCTION()
	void HandleRoomsUpdated(const TArray<FMyBondageLifeServerRoom>& Rooms);

	UFUNCTION()
	void HandleRoomsFailed(const FText& ErrorMessage);

	UFUNCTION()
	void HandleRoomJoined(const FMyBondageLifeServerRoom& Room);

	UFUNCTION()
	void HandleJoinTimeout();

	UFUNCTION()
	void HandleCancelJoinClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AccountLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RoomBrowser;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> RoomListBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoomListHint;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoomBrowserTitle;

	/** Keeps the per-row click handlers alive. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeRoomRowAction>> RowActions;

	/** Rooms currently shown, in display order (server mode). */
	TArray<FMyBondageLifeServerRoom> DisplayedRooms;

	/** Rooms currently shown, in display order (local mode). */
	TArray<FMyBondageLifeRoomInfo> DisplayedLocalRooms;

	/** Which list the browser is showing. */
	ERoomBrowserMode BrowserMode = ERoomBrowserMode::Server;

	/** Blocks a second join request while a travel is already in progress. */
	bool bJoinInProgress = false;

	/** Full-screen "connecting" overlay, shown while waiting for the join response. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ConnectingOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConnectingLabel;

	FTimerHandle JoinTimeoutTimer;

	FTimerHandle RoomRefreshTimerHandle;
};

/**
 *  Tiny helper that lets each dynamically created room row have its own click
 *  handler (dynamic delegates cannot bind a lambda or an index).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRoomRowAction : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TWeakObjectPtr<UMainSceneWidget> OwnerWidget;

	UPROPERTY()
	int32 RoomIndex = INDEX_NONE;
};
