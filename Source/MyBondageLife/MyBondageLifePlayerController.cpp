
// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifePlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"


#include "Network/MyBondageLifeRoomSubsystem.h"
#include "Core/MyBondageLifeFurnitureBase.h"

#include "Components/AudioComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeOutfitCatalog.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Core/Garbler/MyBondageLifeGarbler.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "InputMappingContext.h"
#include "MyBondageLife.h"
#include "UI/MyBondageLifeRuleWidgets.h"
#include "UI/MyBondageLifeRuleDetailPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/AnimMontage.h"
#include "UI/MyBondageLifeEmoteWidgets.h"
#include "Core/MyBondageLifeEmoteCatalog.h"
#include "UI/MyBondageLifeSitHintWidget.h"
#include "MyBondageLifeCharacter.h"
#include "Network/MyBondageLifeSessionSubsystem.h"
#include "UI/MainSceneWidget.h"
#include "UI/MyBondageLifeChatWidget.h"
#include "UI/MyBondageLifeMessageWidgets.h"
#include "UI/MyBondageLifeMainHudWidget.h"
#include "UI/MyBondageLifeHudEntryWidget.h"
#include "UI/MyBondageLifeHudButtonWidget.h"
#include "UI/MyBondageLifeRestraintStatusWidget.h"
#include "UI/MyBondageLifeFriendListWidget.h"
#include "UI/MyBondageLifeGameMenuWidget.h"
#include "UI/MyBondageLifeHostInfoWidget.h"
#include "UI/MyBondageLifeLoginWidget.h"
#include "UI/MyBondageLifeNameplateLayer.h"
#include "UI/MyBondageLifePlayerInfoWidget.h"
#include "UI/MyBondageLifeWardrobeWidget.h"
#include "UI/MyBondageLifeUIStyle.h"
#include "UI/MyBondageLifeSettingsWidget.h"
#include "UI/MyBondageLifeWardrobeButtonWidget.h"
#include "UI/MyBondageLifePlayerInfoEntryWidget.h"

namespace
{
	/** Z orders, low to high, so the layers stack predictably. */
	constexpr int32 NameplateZOrder = 5;
	constexpr int32 LoginZOrder = 10;
	constexpr int32 MenuZOrder = 10;
	constexpr int32 HostInfoZOrder = 40;
	constexpr int32 ChatZOrder = 45;
	constexpr int32 PlayerInfoZOrder = 46;
	constexpr int32 FriendListZOrder = 47;
	constexpr int32 GameMenuZOrder = 150;
}

AMyBondageLifePlayerController::AMyBondageLifePlayerController()
{
	// The front end is fully C++: no blueprint class is required.
}

// ---------------------------------------------------------------------------
// Login screen
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowLogin()
{
	if (!IsLocalPlayerController() || LoginWidget)
	{
		return;
	}

	if (!LoginWidgetClass)
	{
		LoginWidgetClass = UMyBondageLifeLoginWidget::StaticClass();
	}

	LoginWidget = CreateWidget<UMyBondageLifeLoginWidget>(this, LoginWidgetClass);
	if (!LoginWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("Login: could not create the login widget."));
		return;
	}

	LoginWidget->OnLoginCompleted.AddDynamic(this, &AMyBondageLifePlayerController::HandleLoginCompleted);
	LoginWidget->AddToViewport(LoginZOrder);

	// The login screen owns the keyboard.
	SetMenuInputMode(true);
}

void AMyBondageLifePlayerController::HideLogin()
{
	if (LoginWidget)
	{
		LoginWidget->OnLoginCompleted.RemoveDynamic(this, &AMyBondageLifePlayerController::HandleLoginCompleted);
		LoginWidget->RemoveFromParent();
		LoginWidget = nullptr;
	}
}

bool AMyBondageLifePlayerController::IsLoginVisible() const
{
	return LoginWidget != nullptr && LoginWidget->IsInViewport();
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowMainScene()
{
	if (!IsLocalPlayerController() || MainSceneWidget)
	{
		return;
	}

	if (!MainSceneWidgetClass)
	{
		MainSceneWidgetClass = UMainSceneWidget::StaticClass();
	}

	MainSceneWidget = CreateWidget<UMainSceneWidget>(this, MainSceneWidgetClass);
	if (!MainSceneWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("MainScene: could not create the menu widget."));
		return;
	}

	MainSceneWidget->AddToViewport(MenuZOrder);

	SetMenuInputMode(true);
}

void AMyBondageLifePlayerController::HideMainScene()
{
	if (MainSceneWidget)
	{
		MainSceneWidget->RemoveFromParent();
		MainSceneWidget = nullptr;
	}
}

bool AMyBondageLifePlayerController::IsMainSceneVisible() const
{
	return MainSceneWidget != nullptr && MainSceneWidget->IsInViewport();
}

// ---------------------------------------------------------------------------
// Session overlay
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowHostInfo()
{
	if (!IsLocalPlayerController() || HostInfoWidget)
	{
		return;
	}

	if (!HostInfoWidgetClass)
	{
		HostInfoWidgetClass = UMyBondageLifeHostInfoWidget::StaticClass();
	}

	HostInfoWidget = CreateWidget<UMyBondageLifeHostInfoWidget>(this, HostInfoWidgetClass);
	if (HostInfoWidget)
	{
		if (!HostInMainHud(TEXT("W_RoomInfoHost"), HostInfoWidget))
		{
			HostInfoWidget->AddToViewport(HostInfoZOrder);
		}
	}
}

void AMyBondageLifePlayerController::HideHostInfo()
{
	if (HostInfoWidget)
	{
		HostInfoWidget->RemoveFromParent();
		HostInfoWidget = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Nameplates
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowNameplateLayer()
{
	if (!IsLocalPlayerController() || NameplateLayer)
	{
		return;
	}

	if (!NameplateLayerClass)
	{
		NameplateLayerClass = UMyBondageLifeNameplateLayer::StaticClass();
	}

	NameplateLayer = CreateWidget<UMyBondageLifeNameplateLayer>(this, NameplateLayerClass);
	if (NameplateLayer)
	{
		NameplateLayer->AddToViewport(NameplateZOrder);
	}
}

void AMyBondageLifePlayerController::HideNameplateLayer()
{
	if (NameplateLayer)
	{
		NameplateLayer->RemoveFromParent();
		NameplateLayer = nullptr;
	}
}

// ---------------------------------------------------------------------------
// In-game menu (Escape)
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ToggleGameMenu()
{
	// Escape while typing cancels the chat input instead of opening the menu.
	if (bChatTextInput)
	{
		LeaveChatInputMode(true);
		return;
	}

	// 面板/弹层优先（规则、衣柜弹层、表情、消息通知）。
	if (HandleEscapePanels())
	{
		return;
	}

	if (bWardrobeOpen)
	{
		ToggleWardrobe();
		return;
	}

	if (bFriendPanelOpen)
	{
		ToggleFriendList();
		return;
	}

	if (bPlayerInfoOpen)
	{
		TogglePlayerInfo();
		return;
	}

	// The Escape that just closed the chat must not also open the menu.
	if (FPlatformTime::Seconds() - LastChatInputEndSeconds < 0.2)
	{
		return;
	}

	if (IsGameMenuOpen())
	{
		HideGameMenu();
	}
	else
	{
		ShowGameMenu();
	}
}

void AMyBondageLifePlayerController::ShowGameMenu()
{
	if (!IsLocalPlayerController() || GameMenuWidget)
	{
		return;
	}

	// Only meaningful while actually in a room.
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;
	if (!Session || !Session->IsInSession())
	{
		return;
	}

	if (!GameMenuWidgetClass)
	{
		GameMenuWidgetClass = UMyBondageLifeGameMenuWidget::StaticClass();
	}

	GameMenuWidget = CreateWidget<UMyBondageLifeGameMenuWidget>(this, GameMenuWidgetClass);
	if (!GameMenuWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("GameMenu: could not create the menu widget."));
		return;
	}

	GameMenuWidget->AddToViewport(GameMenuZOrder);

	// The pawn stops responding entirely. GameAndUI mode is used instead of UIOnly
	// so Escape still reaches this controller and can close the menu again.
	SetGameplayInputBlocked(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AMyBondageLifePlayerController::HideGameMenu()
{
	if (GameMenuWidget)
	{
		GameMenuWidget->RemoveFromParent();
		GameMenuWidget = nullptr;
	}

	SetGameplayInputBlocked(false);
	RefreshChatInputState();
}

bool AMyBondageLifePlayerController::IsGameMenuOpen() const
{
	return GameMenuWidget != nullptr && GameMenuWidget->IsInViewport();
}

void AMyBondageLifePlayerController::SetGameplayInputBlocked(bool bBlocked)
{
	if (bGameplayInputBlocked == bBlocked)
	{
		return;
	}

	bGameplayInputBlocked = bBlocked;

	if (bBlocked)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
	else
	{
		// SetIgnore*Input are counters, so repeated blocking would leave them above
		// zero and the pawn would never respond again. Resetting clears them.
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();   // look was blocked above as well; must be cleared too
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		if (bBlocked)
		{
			ControlledPawn->DisableInput(this);
		}
		else
		{
			ControlledPawn->EnableInput(this);
		}
	}
}

void AMyBondageLifePlayerController::SetMenuInputMode(bool bMenuMode)
{
	if (bMenuMode)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

// ---------------------------------------------------------------------------
// Chat
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowChat()
{
	if (!IsLocalPlayerController() || ChatWidget)
	{
		return;
	}

	if (!ChatWidgetClass)
	{
		ChatWidgetClass = UMyBondageLifeChatWidget::StaticClass();
	}

	ChatWidget = CreateWidget<UMyBondageLifeChatWidget>(this, ChatWidgetClass);
	if (!ChatWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("Chat: could not create the chat widget."));
		return;
	}

	ChatWidget->OnMessageSubmitted.AddDynamic(this, &AMyBondageLifePlayerController::HandleChatMessageSubmitted);

	if (UMyBondageLifeAccountSubsystem* SystemChatAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		SystemChatAccount->OnSystemChatMessage.AddUniqueDynamic(this,
			&AMyBondageLifePlayerController::HandleSystemChatMessage);
	}
	ChatWidget->OnInputEnded.AddDynamic(this, &AMyBondageLifePlayerController::HandleChatInputEnded);
	if (!HostInMainHud(TEXT("W_ChatHost"), ChatWidget))
		{
			ChatWidget->AddToViewport(ChatZOrder);
		}

	// Join/leave notices that arrived while the level was still loading.
	for (const FString& PendingMessage : PendingSystemMessages)
	{
		ChatWidget->AddSystemMessage(PendingMessage);
	}
	PendingSystemMessages.Reset();
}

void AMyBondageLifePlayerController::HideChat()
{
	if (bChatTextInput)
	{
		LeaveChatInputMode(true);
	}

	if (ChatWidget)
	{
		ChatWidget->RemoveFromParent();
		ChatWidget = nullptr;
	}
}

void AMyBondageLifePlayerController::EnterChatInputMode()
{
	if (!ChatWidget)
	{
		return;
	}

	bChatTextInput = true;
	RefreshChatInputState();
	ChatWidget->BeginTextInput();
}

void AMyBondageLifePlayerController::LeaveChatInputMode(bool bClearText)
{
	if (ChatWidget)
	{
		// EndTextInput broadcasts OnInputEnded, which runs HandleChatInputEnded.
		ChatWidget->EndTextInput(bClearText);
	}

	if (bChatTextInput)
	{
		bChatTextInput = false;
		RefreshChatInputState();
	}

	LastChatInputEndSeconds = FPlatformTime::Seconds();
}

void AMyBondageLifePlayerController::HandleChatEnterPressed()
{
	if (bChatTextInput || bAltMouseMode || IsGameMenuOpen())
	{
		return;
	}

	EnterChatInputMode();
}

void AMyBondageLifePlayerController::HandleAltPressed()
{
	if (IsGameMenuOpen() || bChatTextInput || bAltMouseMode || bPlayerInfoOpen || bFriendPanelOpen)
	{
		return;
	}

	bAltMouseMode = true;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleAltReleased()
{
	if (!bAltMouseMode)
	{
		return;
	}

	bAltMouseMode = false;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleChatMessageSubmitted(const FString& Message)
{
	// 同一条内容在极短时间内重复提交：直接忽略（Enter 与 OnTextCommitted 会各来一次）。
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Message == LastSubmittedChatText && (Now - LastSubmittedChatTime) < 0.8)
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("Chat: duplicate submit ignored"));
		return;
	}
	LastSubmittedChatText = Message;
	LastSubmittedChatTime = Now;

	// "/" 开头的输入是指令（如 /safeword），不当作聊天内容发送。
	const FString Trimmed = Message.TrimStartAndEnd();
	if (Trimmed.StartsWith(TEXT("/")))
	{
		ExecuteChatCommand(Trimmed);
		return;
	}

	// CantOOC：含 * / : 的发言直接拦下，并广播系统提示。
	if (UMyBondageLifeAccountSubsystem* RuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		if (RuleAccount->IsRuleEnabled(TEXT("CantOOC"))
			&& (Trimmed.Contains(TEXT("*")) || Trimmed.Contains(TEXT(":"))
				|| Trimmed.Contains(TEXT("\uFF0A")) || Trimmed.Contains(TEXT("\uFF1A"))))
		{
			if (AMyBondageLifeCharacter* OOCCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
			{
				// 复用表情广播接口（text 覆盖），所有人可见且不受口球影响。
				const FString OOCNotice = FString::Printf(TEXT("%s\u8BD5\u56FE\u8FDB\u884Cooc\u804A\u5929\u88AB\u7981\u6B62"), *OOCCharacter->GetCharacterName().ToString());
				RuleAccount->BroadcastSystemText(OOCNotice, TEXT("CantOOC"));
				UE_LOG(LogMyBondageLife, Log, TEXT("OOC: blocked chat, broadcast via text override"));
			}
			return;
		}
	}

	// Whitelist_Hear：嘴被堵住时把原句和许可 UID 一起带给服务器，由收件人决定是否显示原文。
	FString Payload = Message;
	if (const AMyBondageLifeCharacter* ListenCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		if (ListenCharacter->GetGagActive())
		{
			if (const UMyBondageLifeAccountSubsystem* RuleAccount = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
			{
				FMyBondageLifeRuleDetail HearDetail;
				if (RuleAccount->IsRuleEnabled(TEXT("Whitelist_Hear"))
					&& RuleAccount->GetRuleDetailStruct(TEXT("Whitelist_Hear"), HearDetail)
					&& HearDetail.Ints.Num() > 0)
				{
					FString UidList = FString::JoinBy(HearDetail.Ints, TEXT(","), [](int32 Value) { return FString::FromInt(Value); });
					Payload = FString::Printf(TEXT("<WH|%s|%s>"), *UidList, *Message);

				UE_LOG(LogMyBondageLife, Log, TEXT("Whitelist_Hear: sending uids [%s] enabled=%d"), *UidList, RuleAccount->IsRuleEnabled(TEXT("Whitelist_Hear")) ? 1 : 0);
				}
			}
	}
	}

	ServerSendChatMessage(Payload);
}

void AMyBondageLifePlayerController::ExecuteChatCommand(const FString& CommandLine)
{
	FString Command = CommandLine;
	Command.RemoveFromStart(TEXT("/"));

	FString Name = Command;
	FString Args;
	if (!Command.Split(TEXT(" "), &Name, &Args))
	{
		Name = Command;
		Args.Reset();
	}
	Name = Name.ToLower();

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	if (Name == TEXT("safeword"))
	{
		if (Account->GetDifficulty().Equals(TEXT("extreme"), ESearchCase::IgnoreCase))
		{
			ShowSystemChatMessage(TEXT("极限难度下无法使用安全词"));
			return;
		}

		Account->RequestSafeword();
		return;
	}

	ShowSystemChatMessage(FString::Printf(TEXT("未知指令：/%s"), *Name));
}

void AMyBondageLifePlayerController::HandleChatInputEnded()
{
	// The widget already left text mode (submitted or cancelled).
	bChatTextInput = false;
	LastChatInputEndSeconds = FPlatformTime::Seconds();
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	// Character name only, matching the nameplate.
	FString RoleName;
	const AMyBondageLifeCharacter* SenderCharacter = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (SenderCharacter)
	{
		RoleName = SenderCharacter->GetCharacterName().ToString();
	}
	if (RoleName.IsEmpty())
	{
		RoleName = TEXT("\u89D2\u8272\u540D");
	}

	const FString SenderName = RoleName;

	// 解析 Whitelist_Hear 标记：<WH|uid,uid|原文>
	FString PlainMessage = TrimmedMessage;
	FString WhitelistUids;
	FString WhitelistOriginal;
	if (TrimmedMessage.StartsWith(TEXT("<WH|")))
	{
		FString Rest = TrimmedMessage.RightChop(4);
		int32 SeparatorIndex = INDEX_NONE;
		if (Rest.FindChar(TCHAR('|'), SeparatorIndex))
		{
			WhitelistUids = Rest.Left(SeparatorIndex);
			WhitelistOriginal = Rest.RightChop(SeparatorIndex + 1);
			WhitelistOriginal.RemoveFromEnd(TEXT(">"));
			PlainMessage = WhitelistOriginal;
		}
	}

	// 嘴里塞着东西时，正文换成堵嘴后的发音再广播。
	// 放在服务端做：所有客户端（包括自己）看到的是同一份文本，客户端也绕不过去。
	FString OutgoingMessage = PlainMessage;
	if (SenderCharacter && SenderCharacter->GetGagActive())
	{
		const FName GagProfile = SenderCharacter->GetGagProfileId();
		if (!GagProfile.IsNone())
		{
			FMyBondageLifeGarblerOptions GarblerOptions;
			const FString Garbled = FMyBondageLifeGarbler::Garble(PlainMessage, { GagProfile }, GarblerOptions);
			// 密封头套XL 这类"完全发不出声"的档位会得到空串：补一个省略号占位，
			// 绝不能退回原文（否则等于没堵嘴）。
			OutgoingMessage = Garbled.IsEmpty() ? FString(TEXT("\u2026\u2026")) : Garbled;
			UE_LOG(LogMyBondageLife, Log, TEXT("Chat: gagged [%s] %s -> %s"),
				*GagProfile.ToString(), *TrimmedMessage, *OutgoingMessage);
		}
	}

	{
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	{
	}

	// 白名单收件人需要原句：附在正文后面（不参与混淆）。
	if (!WhitelistUids.IsEmpty())
	{
		OutgoingMessage = FString::Printf(TEXT("%s<WH|%s|%s>"), *OutgoingMessage, *WhitelistUids, *WhitelistOriginal);
	}

	// Deliver to everyone in the room, including the sender.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMyBondageLifePlayerController* OtherController = Cast<AMyBondageLifePlayerController>(It->Get()))
		{
			const APlayerState* SenderState = GetPlayerState<APlayerState>();
			const FString SenderAccount = SenderState ? SenderState->GetPlayerName() : FString();
			OtherController->ClientReceiveChatMessage(SenderAccount, SenderName, OutgoingMessage);
		}
	}
}

void AMyBondageLifePlayerController::ClientReceiveChatMessage_Implementation(const FString& SenderAccount, const FString& SenderName, const FString& Message)
{
	// Whitelist_Hear：<WH|uid,uid|原文> 标记；命中白名单的收件人显示「混淆（原文）」。
	FString DisplayMessage = Message;
	{
		int32 MarkerIndex = INDEX_NONE;
		if (DisplayMessage.FindChar(TCHAR('<'), MarkerIndex))
		{
			const FString Tail = DisplayMessage.Mid(MarkerIndex);
			if (Tail.StartsWith(TEXT("<WH|")))
			{
				const FString Body = Tail.RightChop(4);
				int32 SeparatorIndex = INDEX_NONE;
				if (Body.FindChar(TCHAR('|'), SeparatorIndex))
				{
					const FString UidList = Body.Left(SeparatorIndex);
					FString Original = Body.RightChop(SeparatorIndex + 1);
					Original.RemoveFromEnd(TEXT(">"));

					const FString Garbled = DisplayMessage.Left(MarkerIndex);
					DisplayMessage = Garbled;

					if (const UMyBondageLifeAccountSubsystem* ChatAccount = GetGameInstance()
						? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
					{
				const FString OwnUid = ChatAccount->GetOwnUid();
				const bool bSenderIsSelf = !SenderAccount.IsEmpty()
					&& SenderAccount.Equals(ChatAccount->GetAccountName(), ESearchCase::IgnoreCase);

				// 只有白名单命中才显示原文（发送者也不例外，除非把自己填进白名单）。
				TArray<FString> AllowedUids;
				UidList.ParseIntoArray(AllowedUids, TEXT(","), true);
				const bool bWhitelisted = !OwnUid.IsEmpty() && AllowedUids.ContainsByPredicate(
					[&OwnUid](const FString& Entry)
					{
						const FString Trimmed = Entry.TrimStartAndEnd();
						return !Trimmed.IsEmpty() && Trimmed != TEXT("0") && Trimmed == OwnUid;
					});

				UE_LOG(LogMyBondageLife, Log, TEXT("Whitelist_Hear: receive uids=[%s] own=%s sender=%s isSelf=%d hit=%d"),
					*UidList, *OwnUid, *SenderAccount, bSenderIsSelf ? 1 : 0, bWhitelisted ? 1 : 0);

				if (bWhitelisted)
						{
							DisplayMessage = FString::Printf(TEXT("%s\uFF08%s\uFF09"), *Garbled, *Original);
						}
					}
				}
			}
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Chat: [%s] %s"), *SenderName, *Message);

	{
		// Contract partners are labelled: [奴隶] (purple) for my slaves, [主人] (red)
		// for my master; third parties never see the prefix.
		FString Prefix;
		FLinearColor PrefixColor(1.0f, 1.0f, 1.0f);

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
			{
				const EMyBondageLifeContractRole ContractRole = Account->GetContractRoleWith(SenderAccount);
				if (ContractRole == EMyBondageLifeContractRole::TemporaryMaster || ContractRole == EMyBondageLifeContractRole::Master)
				{
					Prefix = TEXT("[\u5974\u96B6]");
					PrefixColor = FLinearColor(0.72f, 0.42f, 1.0f);
				}
				else if (ContractRole == EMyBondageLifeContractRole::TemporarySlave || ContractRole == EMyBondageLifeContractRole::Slave)
				{
					Prefix = TEXT("[\u4E3B\u4EBA]");
					PrefixColor = FLinearColor(1.0f, 0.28f, 0.28f);
				}
			}
		}

		if (Prefix.IsEmpty())
		{
				ChatWidget->AddChatMessage(SenderName, DisplayMessage);
		}
		else
		{
				ChatWidget->AddChatMessageWithPrefix(Prefix, PrefixColor, SenderName, DisplayMessage);
		}
	}
}

void AMyBondageLifePlayerController::ClientReceiveSystemMessage_Implementation(const FString& Message)
{
	if (ChatWidget)
	{
		ChatWidget->AddSystemMessage(Message);
		return;
	}

	// The chat widget is created when the player enters the game; keep early
	// notices so a freshly joined client still sees them.
	PendingSystemMessages.Add(Message);
}

void AMyBondageLifePlayerController::BroadcastSystemMessage(const FString& Message)
{
	{
	}

	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Chat: %s"), *Message);

	// Everyone in the room sees it, the host included.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMyBondageLifePlayerController* OtherController = Cast<AMyBondageLifePlayerController>(It->Get()))
		{
			OtherController->ClientReceiveSystemMessage(Message);
		}
	}
}

FString AMyBondageLifePlayerController::GetDisplayCharacterName() const
{
	if (const AMyBondageLifeCharacter* PlayerCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		const FString RoleName = PlayerCharacter->GetCharacterName().ToString();
		if (!RoleName.IsEmpty())
		{
			return RoleName;
		}
	}

	if (const APlayerState* OwningPlayerState = PlayerState)
	{
		const FString AccountName = OwningPlayerState->GetPlayerName();
		if (!AccountName.IsEmpty())
		{
			return AccountName;
		}
	}

	// "玩家"
	return TEXT("\u73A9\u5BB6");
}

void AMyBondageLifePlayerController::RefreshChatInputState()
{
	// While the Escape menu is up it owns the input mode.
	if (IsGameMenuOpen())
	{
		return;
	}

	const bool bBlocked = bChatTextInput || bAltMouseMode || bPlayerInfoOpen || bFriendPanelOpen
		|| bFriendRequestPending || bWardrobeOpen || bSocialModalOpen || bMessageCenterOpen
		|| bEmotePanelOpen || bRulePanelOpen || bRuleDetailOpen;

	SetGameplayInputBlocked(bBlocked);

	if (bBlocked)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		// Alt and the panels free the mouse; typing keeps the cursor hidden.
		bShowMouseCursor = bAltMouseMode || bPlayerInfoOpen || bFriendPanelOpen
			|| bFriendRequestPending || bWardrobeOpen || bSocialModalOpen || bMessageCenterOpen
			|| bEmotePanelOpen || bRulePanelOpen || bRuleDetailOpen;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
}

// ---------------------------------------------------------------------------
// Player info
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowPlayerInfo()
{
	if (!IsLocalPlayerController() || PlayerInfoWidget)
	{
		return;
	}

	if (!PlayerInfoWidgetClass)
	{
		UClass* DesignedInfo = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_PlayerInfo.WBP_PlayerInfo_C"));
		PlayerInfoWidgetClass = DesignedInfo ? DesignedInfo : UMyBondageLifePlayerInfoWidget::StaticClass();
	}

	PlayerInfoWidget = CreateWidget<UMyBondageLifePlayerInfoWidget>(this, PlayerInfoWidgetClass);
	if (!PlayerInfoWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("PlayerInfo: could not create the widget."));
		return;
	}

	PlayerInfoWidget->OnPanelOpenChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandlePlayerInfoOpenChanged);
	PlayerInfoWidget->OnAddFriendRequested.AddDynamic(this, &AMyBondageLifePlayerController::HandleAddFriendRequested);
	PlayerInfoWidget->OnContractRequested.AddDynamic(this, &AMyBondageLifePlayerController::HandleContractRequested);

	// A new body height has to reach this pawn (and replicate to everyone else).
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnHeightChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleHeightChanged);
			Account->OnHairChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleHairChanged);
			Account->OnRestraintsChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleRestraintsChanged);
		}
	}
	PlayerInfoWidget->AddToViewport(PlayerInfoZOrder);
}

void AMyBondageLifePlayerController::HidePlayerInfo()
{
	// Closing the panel keeps the bottom entry (and its C binding) available.
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetPanelOpen(false);
	}

	bPlayerInfoOpen = false;
}

bool AMyBondageLifePlayerController::IsPlayerInfoOpen() const
{
	return bPlayerInfoOpen;
}

void AMyBondageLifePlayerController::TogglePlayerInfo()
{
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	if (!PlayerInfoWidget)
	{
		return;
	}

	PlayerInfoWidget->TogglePanel();
}

void AMyBondageLifePlayerController::HandlePlayerInfoOpenChanged(bool bIsOpen)
{
	bPlayerInfoOpen = bIsOpen;

	// A single place decides the input mode: opening the panel freezes the
	// character and frees the cursor, closing it hands control back to the game.
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandlePlayerInfoKeyPressed()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	// "C" belongs to the chat text while typing.
	if (bChatTextInput || IsGameMenuOpen())
	{
		return;
	}

	if (!PlayerInfoWidget)
	{
		return;
	}

	AMyBondageLifeCharacter* OwnCharacter = Cast<AMyBondageLifeCharacter>(GetPawn());

	// "C" always means *my* info: when somebody else's panel is on screen it
	// switches to the local character instead of just closing, and it only closes
	// when the local character is already the one displayed.
	if (PlayerInfoWidget->IsPanelOpen() && PlayerInfoWidget->IsDisplayingCharacter(OwnCharacter))
	{
		PlayerInfoWidget->SetPanelOpen(false);
		return;
	}

	PlayerInfoWidget->ShowCharacterInfo(OwnCharacter);
	PlayerInfoWidget->SetPanelOpen(true);
}

void AMyBondageLifePlayerController::HandleInspectPressed()
{
	if (bInFurniture) { return; }

	if (bTractionFollower) { return; }
	if (!PlayerInfoWidget)
	{
		return;
	}

	// Any open panel owns the input: F must not stack another window on top.
	if (PlayerInfoWidget->IsPanelOpen()
		|| IsWardrobeOpen()
		|| (FriendListWidget && FriendListWidget->IsPanelOpen()))
	{
		return;
	}
	const AMyBondageLifeCharacter* Target = NearbyCharacter.Get();

	// Same rule as "C": switch the subject first, close only when it is already
	// the character on screen.
	if (Target && !(PlayerInfoWidget->IsPanelOpen() && PlayerInfoWidget->IsDisplayingCharacter(Target)))
	{
		PlayerInfoWidget->ShowCharacterInfo(Target);
		PlayerInfoWidget->SetPanelOpen(true);
		return;
	}

	if (bPlayerInfoOpen)
	{
		PlayerInfoWidget->SetPanelOpen(false);
	}
}

void AMyBondageLifePlayerController::UpdateNearbyCharacter()
{
	NearbyCharacter = nullptr;

	if (!PlayerInfoWidget || bPlayerInfoOpen || bChatTextInput || IsGameMenuOpen())
	{
		if (PlayerInfoWidget)
		{
			PlayerInfoWidget->SetInteractionHintTarget(nullptr);
		}
		return;
	}

	const APawn* OwnPawn = GetPawn();
	{
	}

	UWorld* World = GetWorld();
	if (!OwnPawn || !World)
	{
		PlayerInfoWidget->SetInteractionHintTarget(nullptr);
		return;
	}

	// Nearest other player inside the interaction range.
	AMyBondageLifeCharacter* Nearest = nullptr;
	float NearestDistanceSq = FMath::Square(InteractionRange);

	for (TActorIterator<AMyBondageLifeCharacter> It(World); It; ++It)
	{
		AMyBondageLifeCharacter* Other = *It;
		if (!Other || Other == OwnPawn)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(OwnPawn->GetActorLocation(), Other->GetActorLocation());
		if (DistanceSq <= NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			Nearest = Other;
		}
	}

	NearbyCharacter = Nearest;

	// Nearest wins when several players are in range.
	PlayerInfoWidget->SetInteractionHintTarget(Nearest);
}

// ---------------------------------------------------------------------------
// Friends
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowFriendList()
{
	if (!IsLocalPlayerController() || FriendListWidget)
	{
		return;
	}

	if (!FriendListWidgetClass)
	{
		UClass* DesignedList = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_FriendList.WBP_FriendList_C"));
		FriendListWidgetClass = DesignedList ? DesignedList : UMyBondageLifeFriendListWidget::StaticClass();
	}

	FriendListWidget = CreateWidget<UMyBondageLifeFriendListWidget>(this, FriendListWidgetClass);
	if (!FriendListWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("Friends: could not create the widget."));
		return;
	}

	FriendListWidget->OnPanelOpenChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleFriendPanelOpenChanged);
	FriendListWidget->OnRequestAnswered.AddDynamic(this, &AMyBondageLifePlayerController::HandleFriendRequestAnswered);
	FriendListWidget->OnContractRequestAnswered.AddDynamic(this, &AMyBondageLifePlayerController::HandleContractRequestAnswered);
	FriendListWidget->OnModalChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleSocialModalChanged);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnFriendRequestReceived.AddDynamic(this, &AMyBondageLifePlayerController::HandleFriendRequestReceived);
			Account->OnFriendRequestAnswered.AddDynamic(this, &AMyBondageLifePlayerController::HandleFriendAnswerFinished);
			Account->OnFriendRequestSent.AddDynamic(this, &AMyBondageLifePlayerController::HandleFriendRequestSent);
			Account->OnContractRequestReceived.AddDynamic(this, &AMyBondageLifePlayerController::HandleContractRequestReceived);
			Account->OnContractActionFinished.AddDynamic(this, &AMyBondageLifePlayerController::HandleContractActionFinished);
			Account->OnContractsChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleContractsChanged);
		}
	}
	FriendListWidget->AddToViewport(FriendListZOrder);
}

void AMyBondageLifePlayerController::HideFriendList()
{
	if (FriendListWidget)
	{
		FriendListWidget->RemoveFromParent();
		FriendListWidget = nullptr;
	}

	bFriendPanelOpen = false;
}

void AMyBondageLifePlayerController::ToggleFriendList()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	if (!FriendListWidget)
	{
		return;
	}

	FriendListWidget->TogglePanel();
}

void AMyBondageLifePlayerController::HandleFriendListKeyPressed()
{
	// "V" belongs to the chat text while typing.
	if (bChatTextInput || IsGameMenuOpen())
	{
		return;
	}

	ToggleFriendList();
}

void AMyBondageLifePlayerController::HandleFriendPanelOpenChanged(bool bIsOpen)
{
	bFriendPanelOpen = bIsOpen;

	if (bIsOpen)
	{
		// Opening the list pulls the latest data so a recent friend shows up.
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
			{
				Account->RefreshFriendsFromServer();
			}
		}
	}

	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleAddFriendRequested(const AMyBondageLifeCharacter* Target)
{
	if (!Target)
	{
		return;
	}

	const APlayerState* TargetState = Target->GetPlayerState();
	if (!TargetState || TargetState->GetPlayerName().IsEmpty())
	{
		return;
	}

	// Server-backed friend request (the P2P prototype RPC is no longer used).
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			PendingAddFriendName = Target->GetCharacterName().IsEmpty()
				? TargetState->GetPlayerName()
				: Target->GetCharacterName().ToString();
			Account->RequestAddFriend(TargetState->GetPlayerName());
		}
	}

	// Sender feedback: local system line plus closing the inspected panel.
	const FString TargetName = Target->GetCharacterName().IsEmpty()
		? TargetState->GetPlayerName()
		: Target->GetCharacterName().ToString();

	if (ChatWidget)
	{
		// "已向[xxx]发送好友请求"
		ChatWidget->AddSystemMessage(FString::Printf(
			TEXT("\u5DF2\u5411[%s]\u53D1\u9001\u597D\u53CB\u8BF7\u6C42"), *TargetName));
	}

	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetPanelOpen(false);
	}
}

void AMyBondageLifePlayerController::HandleFriendRequestAnswered(bool bAccepted)
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: answering request from '%s' (accepted=%d)."),
		*PendingRequesterAccount, bAccepted ? 1 : 0);

	bFriendRequestPending = false;
	RefreshChatInputState();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			const FString RequestId = FriendListWidget ? FriendListWidget->GetPendingRequestId() : FString();
			Account->RespondToFriendRequest(RequestId, bAccepted, PendingRequesterName);
		}
	}

	if (bAccepted)
	{
		// The requester is added locally straight away; the other side learns about
		// it through ClientFriendRequestAnswered.
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
			{
				FMyBondageLifeFriendEntry Entry;
				Entry.AccountName = PendingRequesterAccount;
				Entry.CharacterName = PendingRequesterName;
				Entry.CharacterClass = PendingRequesterClass;
				Entry.bOnline = true;
				Account->AddFriend(Entry);
			}
		}
	}

	// PendingRequesterName / Account are cleared by HandleFriendAnswerFinished once
	// the backend replies.
}

void AMyBondageLifePlayerController::ServerSendFriendRequest_Implementation(const FString& TargetAccountName)
{
	{
	}

	UWorld* World = GetWorld();
	if (!World || TargetAccountName.IsEmpty())
	{
		return;
	}

	// Identity of the requester, taken from the replicated character.
	FString RequesterName = TEXT("---");
	int32 RequesterClass = 1;
	if (const AMyBondageLifeCharacter* RequesterCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		if (!RequesterCharacter->GetCharacterName().IsEmpty())
		{
			RequesterName = RequesterCharacter->GetCharacterName().ToString();
		}
		RequesterClass = RequesterCharacter->GetCharacterClass();
	}

	const FString RequesterAccount = PlayerState ? PlayerState->GetPlayerName() : FString();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AMyBondageLifePlayerController* OtherController = Cast<AMyBondageLifePlayerController>(It->Get());
		if (!OtherController || OtherController == this)
		{
			continue;
		}

		if (OtherController->PlayerState && OtherController->PlayerState->GetPlayerName() == TargetAccountName)
		{
			OtherController->ClientFriendRequestReceived(RequesterAccount, RequesterName, RequesterClass);
			return;
		}
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Friends: target '%s' is not in this session."), *TargetAccountName);
}

void AMyBondageLifePlayerController::ServerAnswerFriendRequest_Implementation(const FString& RequesterAccountName, bool bAccepted)
{
	{
	}

	UWorld* World = GetWorld();
	if (!World || RequesterAccountName.IsEmpty())
	{
		return;
	}

	// Identity of the player answering.
	FString AnswererName = TEXT("---");
	int32 AnswererClass = 1;
	if (const AMyBondageLifeCharacter* AnswererCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		if (!AnswererCharacter->GetCharacterName().IsEmpty())
		{
			AnswererName = AnswererCharacter->GetCharacterName().ToString();
		}
		AnswererClass = AnswererCharacter->GetCharacterClass();
	}

	const FString AnswererAccount = PlayerState ? PlayerState->GetPlayerName() : FString();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AMyBondageLifePlayerController* OtherController = Cast<AMyBondageLifePlayerController>(It->Get());
		if (!OtherController || OtherController == this)
		{
			continue;
		}

		if (OtherController->PlayerState && OtherController->PlayerState->GetPlayerName() == RequesterAccountName)
		{
			OtherController->ClientFriendRequestAnswered(AnswererAccount, AnswererName, AnswererClass, bAccepted);
			return;
		}
	}
}

void AMyBondageLifePlayerController::ClientFriendRequestReceived_Implementation(const FString& RequesterAccountName, const FString& RequesterCharacterName, int32 RequesterClass)
{
	PendingRequesterAccount = RequesterAccountName;
	PendingRequesterName = RequesterCharacterName;
	PendingRequesterClass = RequesterClass;

	if (FriendListWidget)
	{
		// Message center owns request prompts now (no auto popup).
	}

	// "收到来自[xxx]的好友请求" - only this client sees it.
	if (ChatWidget)
	{
		ChatWidget->AddSystemMessage(FString::Printf(
			TEXT("\u6536\u5230\u6765\u81EA[%s]\u7684\u597D\u53CB\u8BF7\u6C42"), *RequesterCharacterName));
	}

	// The dialog takes the mouse and freezes the character until it is answered.
	bFriendRequestPending = true;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::ClientFriendRequestAnswered_Implementation(const FString& AnswererAccountName, const FString& AnswererCharacterName, int32 AnswererClass, bool bAccepted)
{
	if (!bAccepted)
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("Friends: '%s' declined the request."), *AnswererCharacterName);
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			FMyBondageLifeFriendEntry Entry;
			Entry.AccountName = AnswererAccountName;
			Entry.CharacterName = AnswererCharacterName;
			Entry.CharacterClass = AnswererClass;
			Entry.bOnline = true;
			Account->AddFriend(Entry);
		}
	}
}

void AMyBondageLifePlayerController::RefreshFriendPresence()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	{
	}

	UWorld* World = GetWorld();
	if (!Account || !World)
	{
		return;
	}

	// Prototype presence: a friend counts as online when a character with that
	// account name exists in the current session.
	for (const FMyBondageLifeFriendEntry& Friend : Account->GetFriends())
	{
		bool bOnline = false;

		for (TActorIterator<AMyBondageLifeCharacter> It(World); It; ++It)
		{
			const AMyBondageLifeCharacter* FriendCharacter = *It;
			const APlayerState* OwnerState = FriendCharacter ? FriendCharacter->GetPlayerState() : nullptr;
			if (OwnerState && OwnerState->GetPlayerName() == Friend.AccountName)
			{
				bOnline = true;
				break;
			}
		}

		Account->SetFriendOnline(Friend.AccountName, bOnline);
	}
}

// ---------------------------------------------------------------------------
// Login -> session identity
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::HandleLoginCompleted()
{
	HideLogin();

	ApplyAccountNameToPlayerState();
	ShowMainScene();
}

void AMyBondageLifePlayerController::ApplyAccountNameToPlayerState()
{
	{
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (!Account || !Account->IsLoggedIn())
	{
		return;
	}

	// A listen server hosts several players but has a single account subsystem:
	// only the owning client may push its own name, otherwise the host's identity
	// would be applied to every other player.
	if (!IsLocalPlayerController())
	{
		return;
	}

	// On a client this becomes ServerChangeName, so the name replicates to every
	// other player's game state and shows up on their nameplates.
	SetName(Account->GetAccountName());

	// 把自己的账号写到角色（服务端执行并复制给所有人），供好友面板/规则判定使用。
	if (HasAuthority())
	{
		if (AMyBondageLifeCharacter* NamedCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
		{
			NamedCharacter->SetAccountNameFree(Account->GetAccountName());
		}
	}
	else
	{
		ServerAssignAccount(Account->GetAccountName());
	}

	ApplyCharacterNameToPawn();

	// 把账号名同步到角色（好友衣柜按账号查找目标角色用）。
	if (AMyBondageLifeCharacter* BondageCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		// 优先 PlayerState 名字（服务器已把每个玩家的 SetName 同步过来）；
		// 只有本机玩家且 PlayerState 还没名字时才用本机账号兜底，
		// 否则服务器上的 AccountSubsystem 会把主机账号写到所有角色上。
		FString LoginAccount = PlayerState ? PlayerState->GetPlayerName() : FString();
		if (LoginAccount.IsEmpty() && IsLocalPlayerController())
		{
			if (const UMyBondageLifeAccountSubsystem* AccountSub = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
			{
				LoginAccount = AccountSub->GetAccountName();
			}
		}

		if (!LoginAccount.IsEmpty())
		{
			BondageCharacter->SetAccountNameFree(LoginAccount);
		}
	}

	// Remote players change our wardrobe through the service: poll it.
	if (UWorld* SyncWorld = GetWorld())
	{
		SyncWorld->GetTimerManager().SetTimer(RestraintSyncTimer, this,
			&AMyBondageLifePlayerController::SyncRestraintsFromServer, 5.0f, true);
	}

	// Restore the worn restraints on the freshly possessed pawn.
	PushRestraintsToPawn();
}

void AMyBondageLifePlayerController::ApplyCharacterNameToPawn()
{
	// Same reason as above: only the local player may publish its identity.
	if (!IsLocalPlayerController())
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (!Account || !Account->IsLoggedIn())
	{
		return;
	}

	const FString RoleName = Account->GetCharacterName();
	if (RoleName.IsEmpty())
	{
		return;
	}

	const int32 RoleClass = FMath::Clamp(Account->GetCharacterClass(), 1, 4);

	// Account creation date, only used once the backend returned a real value.
	const FDateTime CreatedAt = Account->GetCreatedAt();
	const int64 CreatedAtUnix = (CreatedAt.GetYear() > 2000) ? CreatedAt.ToUnixTimestamp() : 0;

	// Applied locally right away, then again on the server so it replicates to
	// every other client's nameplate and inspect panel.
	if (AMyBondageLifeCharacter* PlayerCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		PlayerCharacter->SetCharacterIdentity(FText::FromString(RoleName), RoleClass, CreatedAtUnix, Account->GetHeightScale(), Account->GetHairIndex());
	}

	ServerSetCharacterInfo(RoleName, RoleClass, CreatedAtUnix, Account->GetHeightScale(), Account->GetHairIndex());
}

void AMyBondageLifePlayerController::ServerSetCharacterInfo_Implementation(const FString& NewCharacterName, int32 NewCharacterClass, int64 NewCreatedAtUnix, float NewHeightScale, int32 NewHairIndex)
{
	if (AMyBondageLifeCharacter* PlayerCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		PlayerCharacter->SetCharacterIdentity(FText::FromString(NewCharacterName), NewCharacterClass, NewCreatedAtUnix, NewHeightScale, NewHairIndex);
	}

	// The name has just arrived, so this is the first moment the join notice can
	// show the right name. Only the first call announces, and only while actually
	// inside a room (the menu level also possesses a pawn).
	const UMyBondageLifeSessionSubsystem* Session = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;
	const bool bInRoom = Session && Session->IsInSession();

	// The host creates the room itself, so it does not announce its own arrival.
	if (bInRoom && !bJoinNoticeSent && !IsLocalPlayerController())
	{
		bJoinNoticeSent = true;

		// Cached for the leave notice: by then the pawn is already destroyed.
		JoinNoticeName = GetDisplayCharacterName();

		BroadcastSystemMessage(FString::Printf(TEXT("\uFF08%s\u52A0\u5165\u4E86\u623F\u95F4\uFF09"), *JoinNoticeName));
	}
}

void AMyBondageLifePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AMyBondageLifeCharacter* BondageCharacter = Cast<AMyBondageLifeCharacter>(InPawn))
	{
		FString LoginAccount = PlayerState ? PlayerState->GetPlayerName() : FString();
		if (LoginAccount.IsEmpty() && IsLocalPlayerController())
		{
			if (const UMyBondageLifeAccountSubsystem* AccountSub = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
			{
				LoginAccount = AccountSub->GetAccountName();
			}
		}

		if (!LoginAccount.IsEmpty())
		{
			BondageCharacter->SetAccountNameFree(LoginAccount);
		}
	}


	ApplyCharacterNameToPawn();
	// Remote players change our wardrobe through the service: poll it.
	if (UWorld* SyncWorld = GetWorld())
	{
		SyncWorld->GetTimerManager().SetTimer(RestraintSyncTimer, this,
			&AMyBondageLifePlayerController::SyncRestraintsFromServer, 5.0f, true);
	}

	// Restore the worn restraints on the freshly possessed pawn.
	PushRestraintsToPawn();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Possession also happens after a travel, where the input mode set on the
	// previous world no longer applies: restore the mapping contexts and hand the
	// mouse back to the game so the look axis keeps working.
	EnsureInputSetup();

	// 统一字体：登录/菜单/游戏内控件都会在此后创建，用低频轮询持续覆盖。
	// The engine's ignore counters live on the controller and survive a travel, so
	// clear them before the input mode is applied for the new pawn.
	bGameplayInputBlocked = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();

	// A pose requested from the main menu applies as soon as we have a character.
	if (bRestraintPosePending)
	{
		bRestraintPosePending = false;

		if (AMyBondageLifeCharacter* PossessedCharacter = Cast<AMyBondageLifeCharacter>(InPawn))
		{
			if (HasAuthority())
			{
				PossessedCharacter->SetRestraintPose(true);
			}
			else
			{
				ServerSetRestraintPose(true);
			}
		}
	}

	RefreshChatInputState();
}

void AMyBondageLifePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Best effort: tell the service this account is leaving so friends see it offline
	// without waiting for the presence window.
	if (IsLocalPlayerController())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
			{
				Account->SendPresenceOffline();
			}
		}
	}

	// The host announces the departure before this controller goes away, so the
	// remaining players see who left.
	if (HasAuthority() && bJoinNoticeSent && !IsLocalPlayerController())
	{
		// The pawn and player state are already gone here, so use the cached name.
		const FString LeaveName = JoinNoticeName.IsEmpty() ? GetDisplayCharacterName() : JoinNoticeName;
		BroadcastSystemMessage(FString::Printf(TEXT("\uFF08%s\u79BB\u5F00\u4E86\u623F\u95F4\uFF09"), *LeaveName));
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// AActor / APlayerController
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Movement / look / jump mapping contexts must exist in every world (menu and
	// game), otherwise the pawn ignores the mouse after a travel.
	EnsureInputSetup();

	// 统一字体：登录/菜单/游戏内控件都会在此后创建，用低频轮询持续覆盖。
	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	const UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	const bool bLoggedIn = Account && Account->IsLoggedIn();
	const bool bInSession = Session && Session->IsInSession();

	// Fresh world: make sure no stale move/look ignore survives the travel.
	bGameplayInputBlocked = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();

	// Publish the identity before deciding what to show, so the pawn already has
	// the right name when the world appears.
	ApplyAccountNameToPlayerState();

	if (bInSession)
	{
		// Nameplates and the chat box only exist while in a room.
		// The HUD hosts must exist before anything is placed inside them.
		ShowMainHud();
		ShowHudEntries();

		ShowNameplateLayer();
		if (UMyBondageLifeAccountSubsystem* OutfitAccount = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			OutfitAccount->OnOutfitsChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleOutfitsChanged);
			OutfitAccount->RefreshOutfits(FString());
			OutfitAccount->OnAccountOutfitsChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleAccountOutfitsChanged);
			OutfitAccount->OnSkinChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleSkinChanged);
			PushSkinToPawn();

			if (UWorld* OutfitWorld = GetWorld())
			{
				OutfitWorld->GetTimerManager().SetTimer(OutfitPollTimer, this,
					&AMyBondageLifePlayerController::PollOtherOutfits, 5.0f, true);
			}
		}

		ShowChat();
		ShowPlayerInfo();
		ShowFriendList();
		ShowWardrobeButton();
	ShowMessageHud();
		ShowWardrobe();
		ShowHostInfo();

		RefreshChatInputState();
	}
	else if (bLoggedIn)
	{
		ShowMainScene();
	}
	else
	{
		ShowLogin();
	}
}

void AMyBondageLifePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();


	if (!InputComponent)
	{
		return;
	}

	// R：靠近椅子时坐下 / 起身（家具相关交互统一用 R）
	InputComponent->BindKey(EKeys::R, IE_Pressed, this,
		&AMyBondageLifePlayerController::HandleSitKeyPressed);

	// G：靠近好友时打开对方拘束页（人际交互，保持不变）
	InputComponent->BindKey(EKeys::G, IE_Pressed, this,
		&AMyBondageLifePlayerController::HandleFriendInteractKeyPressed);

	// Space：取消拘束穿戴
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this,
		&AMyBondageLifePlayerController::CancelBondage);

	// Z：表情动作面板
	InputComponent->BindKey(EKeys::Z, IE_Pressed, this,
		&AMyBondageLifePlayerController::HandleEmoteKeyPressed);

	// ESC：先关细节面板，再关规则面板
	// ESC 统一由 ToggleGameMenu 处理（面板 → 衣柜/好友/信息 → 退出菜单）。

	// 坐着时按任意移动键起身（只起身，不会触发坐下）
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AMyBondageLifePlayerController::HandleStandUpOnMove);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AMyBondageLifePlayerController::HandleStandUpOnMove);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AMyBondageLifePlayerController::HandleStandUpOnMove);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AMyBondageLifePlayerController::HandleStandUpOnMove);

	if (UWorld* SeatWorld = GetWorld())
	{
		SeatWorld->GetTimerManager().SetTimer(SeatProximityTimer, this,
			&AMyBondageLifePlayerController::UpdateSeatProximity, 0.25f, true);

		// 拘束染色：低频重申，保证远端一定拿到最新颜色
		SeatWorld->GetTimerManager().SetTimer(RestraintColorSyncTimer, this,
			&AMyBondageLifePlayerController::PushRestraintColors, 2.0f, true);
	}

	// Escape: the in-game menu (or closes whatever is on top).
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMyBondageLifePlayerController::ToggleGameMenu);

	// Enter: open the chat input box. The chat widget itself submits on Enter.
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AMyBondageLifePlayerController::HandleChatEnterPressed);

	// 个人信息面板：C 键与底部入口同效。
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AMyBondageLifePlayerController::HandlePlayerInfoKeyPressed);

	// 好友列表面板：V 键与底部入口同效（A 是移动键，不占用）。
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AMyBondageLifePlayerController::HandleFriendListKeyPressed);

	// 衣柜面板：B 键与底部入口同效。
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AMyBondageLifePlayerController::HandleWardrobeKeyPressed);

	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AMyBondageLifePlayerController::HandleMessageKeyPressed);

	// 束缚姿势改由装备的拘束（UpAnmi）驱动，F9 临时开关已移除。

	// F: inspect the nearby player.
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AMyBondageLifePlayerController::HandleInspectPressed);

	// Alt: temporarily free the mouse so the chat scrollbar can be dragged.
	InputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &AMyBondageLifePlayerController::HandleAltPressed);
	InputComponent->BindKey(EKeys::LeftAlt, IE_Released, this, &AMyBondageLifePlayerController::HandleAltReleased);
	InputComponent->BindKey(EKeys::RightAlt, IE_Pressed, this, &AMyBondageLifePlayerController::HandleAltPressed);
	InputComponent->BindKey(EKeys::RightAlt, IE_Released, this, &AMyBondageLifePlayerController::HandleAltReleased);
}

void AMyBondageLifePlayerController::PlayerTick(float DeltaTime)
{
	// 蒙眼时看不到其他玩家头顶的名字。
	if (NameplateLayer)
	{
		const AMyBondageLifeCharacter* LocalCharacter = Cast<AMyBondageLifeCharacter>(GetPawn());
		const bool bBlind = LocalCharacter && LocalCharacter->GetBlindLevel() > 0;
		NameplateLayer->SetVisibility(bBlind ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	Super::PlayerTick(DeltaTime);


	// Project every character's head each frame: the plate position is derived from
	// the projection only, so it cannot drift when the camera rotates.
	if (NameplateLayer)
	{
		NameplateLayer->UpdateNameplates();
	}

	if (!PendingFurnitureKey.IsEmpty())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
		for (TActorIterator<AMyBondageLifeFurnitureBase> It(GetWorld()); It; ++It)
		{
			if (IsValid(*It) && (*It)->GetFurnitureKey().Equals(PendingFurnitureKey, ESearchCase::IgnoreCase))
			{
				AttachToFurniture(*It);
				PendingFurnitureKey.Reset();
				break;
			}
		}
		}
	}

	UpdateFurnitureProximity();
	UpdateTractionFollow(DeltaTime);
	UpdateTractionChain();

	UpdateNearbyCharacter();
	TickRoomHeartbeat();
}

void AMyBondageLifePlayerController::TickRoomHeartbeat()
{
	// A killed host never sends a "goodbye" and UDP does not report the dead socket
	// quickly, so the client keeps its own keepalive and leaves when the answers stop.
	if (!IsLocalPlayerController() || GetNetMode() != NM_Client)
	{
		return;
	}

	UMyBondageLifeSessionSubsystem* Session = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;
	if (!Session || !Session->IsInSession())
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	// Start the clock the first time this controller ticks inside a room.
	if (LastHostAckSeconds <= 0.0)
	{
		LastHostAckSeconds = Now;
	}

	if (Now - LastHeartbeatSentSeconds >= 1.0)
	{
		LastHeartbeatSentSeconds = Now;
		ServerRoomHeartbeat();
	}

	if (Now - LastHostAckSeconds > 4.0)
	{
		LastHostAckSeconds = Now;
		UE_LOG(LogMyBondageLife, Warning, TEXT("Session: no answer from the host for 4s, leaving the room."));
		Session->LeaveRoom();
	}
}

void AMyBondageLifePlayerController::ServerRoomHeartbeat_Implementation()
{
	// Answering proves the host is alive, which is all the client needs to know.
	ClientRoomHeartbeatAck();
}

void AMyBondageLifePlayerController::ClientRoomHeartbeatAck_Implementation()
{
	LastHostAckSeconds = FPlatformTime::Seconds();
}

bool AMyBondageLifePlayerController::ShouldUseTouchControls() const
{
	if (bForceTouchControls)
	{
		return true;
	}

#if PLATFORM_ANDROID || PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

void AMyBondageLifePlayerController::EnsureInputSetup()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	int32 AddedContexts = 0;
	for (UInputMappingContext* Context : DefaultMappingContexts)
	{
		if (Context)
		{
			Subsystem->AddMappingContext(Context, 0);
			++AddedContexts;
		}
	}

	// Desktop additionally needs the contexts that mobile excludes: the mouse look
	// (IMC_MouseLook) lives there, so without it the camera cannot be rotated.
	if (ShouldUseTouchControls())
	{
		for (UInputMappingContext* Context : MobileExcludedMappingContexts)
		{
			if (Context)
			{
				Subsystem->RemoveMappingContext(Context);
			}
		}
	}
	else
	{
		for (UInputMappingContext* Context : MobileExcludedMappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
				++AddedContexts;
			}
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Input: %d mapping context(s) registered."), AddedContexts);
}

// ---------------------------------------------------------------------------
// Friends: server callbacks
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::HandleFriendRequestReceived(const FString& RequestId, const FString& RequesterCharacterName)
{
	PendingRequesterName = RequesterCharacterName;
	PendingRequesterAccount.Empty();

	if (FriendListWidget)
	{
		// Message center owns request prompts now (no auto popup).
	}

	// "收到来自[xxx]的好友请求" - only this client sees it.
	if (ChatWidget)
	{
		ChatWidget->AddSystemMessage(FString::Printf(
			TEXT("\u6536\u5230\u6765\u81EA[%s]\u7684\u597D\u53CB\u8BF7\u6C42"), *RequesterCharacterName));
	}

	// The dialog takes the mouse and freezes the character until it is answered.
	// Message center handles it: do not freeze the character on a pending request.
	bFriendRequestPending = false;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleFriendAnswerFinished(bool bAccepted)
{
	bFriendRequestPending = false;
	RefreshChatInputState();

	if (ChatWidget && !PendingRequesterName.IsEmpty())
	{
		ChatWidget->AddSystemMessage(bAccepted
			? FString::Printf(TEXT("\u5DF2\u6DFB\u52A0[%s]\u4E3A\u597D\u53CB"), *PendingRequesterName)
			: FString::Printf(TEXT("\u5DF2\u62D2\u7EDD[%s]\u7684\u597D\u53CB\u8BF7\u6C42"), *PendingRequesterName));
	}

	PendingRequesterName.Reset();
	PendingRequesterAccount.Reset();
	PendingRequesterClass = 1;
}

void AMyBondageLifePlayerController::HandleFriendRequestSent(bool bSucceeded, const FString& TargetAccount)
{
	if (ChatWidget)
	{
		const FString DisplayName = PendingAddFriendName.IsEmpty() ? TargetAccount : PendingAddFriendName;
		ChatWidget->AddSystemMessage(bSucceeded
			? FString::Printf(TEXT("\u5DF2\u5411[%s]\u53D1\u9001\u597D\u53CB\u8BF7\u6C42"), *DisplayName)
			: FString::Printf(TEXT("\u6DFB\u52A0[%s]\u597D\u53CB\u5931\u8D25"), *DisplayName));
	}

	PendingAddFriendName.Reset();
}

// ---------------------------------------------------------------------------
// Wardrobe
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowWardrobe()
{
	if (!IsLocalPlayerController() || WardrobeWidget)
	{
		return;
	}

	if (!WardrobeWidgetClass)
	{
		// Prefer the designed widget blueprint; fall back to the C++ layout.
		UClass* DesignedClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_Wardrobe.WBP_Wardrobe_C"));
		WardrobeWidgetClass = DesignedClass ? DesignedClass : UMyBondageLifeWardrobeWidget::StaticClass();
	}

	WardrobeWidget = CreateWidget<UMyBondageLifeWardrobeWidget>(this, WardrobeWidgetClass);

	// Own wardrobe by default; OpenFriendWardrobe switches it afterwards.
	if (WardrobeWidget)
	{
		WardrobeWidget->EnterFriendMode(FString());
	}
	if (!WardrobeWidget)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("Wardrobe: could not create the widget."));
		return;
	}

	WardrobeWidget->OnPanelOpenChanged.AddDynamic(this, &AMyBondageLifePlayerController::HandleWardrobeOpenChanged);
	WardrobeWidget->AddToViewport(48);

	// The panel is created with the HUD but must start closed.
	WardrobeWidget->SetPanelOpen(false);
}

void AMyBondageLifePlayerController::HideWardrobe()
{
	// Leaving somebody else's wardrobe must not leak into the next open.
	if (WardrobeWidget)
	{
		WardrobeWidget->ResetToOwnMode();
	}

	if (WardrobeWidget)
	{
		WardrobeWidget->RemoveFromParent();
		WardrobeWidget = nullptr;
	}

	bWardrobeOpen = false;
}

void AMyBondageLifePlayerController::ToggleWardrobe()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	if (!WardrobeWidget)
	{
		return;
	}

	WardrobeWidget->TogglePanel();
}

void AMyBondageLifePlayerController::HandleWardrobeKeyPressed()
{
	// "B" belongs to the chat text while typing.
	if (bChatTextInput || IsGameMenuOpen())
	{
		return;
	}

	ToggleWardrobe();
}

void AMyBondageLifePlayerController::HandleWardrobeOpenChanged(bool bIsOpen)
{
	bWardrobeOpen = bIsOpen;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleHeightChanged(float NewHeightScale)
{
	// Re-publish the identity so the pawn (and every other client) picks the new size.
	ApplyCharacterNameToPawn();
}

// ---------------------------------------------------------------------------
// Contracts
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::HandleContractRequested(const AMyBondageLifeCharacter* Target, bool bFormal)
{
	if (!Target)
	{
		return;
	}

	const APlayerState* TargetState = Target->GetPlayerState();
	const FString TargetAccount = TargetState ? TargetState->GetPlayerName() : FString();
	if (TargetAccount.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	PendingAddFriendName = Target->GetCharacterName().IsEmpty()
		? TargetAccount
		: Target->GetCharacterName().ToString();

	Account->RequestContract(TargetAccount, bFormal);

	// The request was sent: close the panel only. Removing the widget would also
	// remove the bottom "个人信息（C）" entry for the rest of the session.
	if (PlayerInfoWidget)
	{
		PlayerInfoWidget->SetPanelOpen(false);
	}
}

void AMyBondageLifePlayerController::HandleContractActionFinished(bool bSucceeded, const FString& ErrorCode, int32 Action)
{
	if (ChatWidget)
	{
		// Action: 1 request sent, 2 accepted, 3 dissolved, 4 declined.
		if (Action == 1)
		{
			const FString DisplayName = PendingAddFriendName.IsEmpty() ? TEXT("---") : PendingAddFriendName;

			if (bSucceeded)
			{
				ChatWidget->AddSystemMessage(FString::Printf(
					TEXT("\u5DF2\u5411[%s]\u53D1\u8D77\u5951\u7EA6\u7533\u8BF7"), *DisplayName));
			}
			else if (ErrorCode == TEXT("TARGET_ALREADY_OWNED"))
			{
				ChatWidget->AddSystemMessage(FString::Printf(
					TEXT("[%s]\u5DF2\u7ECF\u62E5\u6709\u4E3B\u4EBA"), *DisplayName));
			}
			else
			{
				ChatWidget->AddSystemMessage(TEXT("\u5951\u7EA6\u7533\u8BF7\u5931\u8D25"));
			}
		}
		else if (Action == 2)
		{
			ChatWidget->AddSystemMessage(FString::Printf(
				TEXT("已同意[%s]的契约请求"), *(PendingRequesterName.IsEmpty() ? FString(TEXT("对方")) : PendingRequesterName)));   // 已同意[x]的契约请求
		}
		else if (Action == 4)
		{
			ChatWidget->AddSystemMessage(FString::Printf(
				TEXT("\u5DF2\u62D2\u7EDD[%s]\u7684\u5951\u7EA6\u8BF7\u6C42"), *PendingRequesterName));   // 已拒绝[x]的契约请求
		}
		else if (Action == 3 && bSucceeded)
		{
			ChatWidget->AddSystemMessage(TEXT("\u5DF2\u89E3\u9664\u4E3B\u5974\u5951\u7EA6"));              // 已解除主奴契约
		}
	}

	if (!bSucceeded && ErrorCode == TEXT("TARGET_ALREADY_OWNED"))
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
			{
				Account->MarkAccountOwned(PendingAddFriendName);
			}
		}
	}

	PendingAddFriendName.Reset();

	if (PlayerInfoWidget && IsPlayerInfoOpen())
	{
		PlayerInfoWidget->RefreshContents();
	}
}

void AMyBondageLifePlayerController::HandleContractRequestReceived(const FString& RequestId, const FString& RequesterCharacterName, bool bFormal)
{
	// Remember who asked: the answer message names them.
	PendingRequesterName = RequesterCharacterName;

	if (FriendListWidget)
	{
		// Message center owns request prompts now (no auto popup).
	}

	if (ChatWidget)
	{
		if (bFormal)
		{
			ChatWidget->AddSystemMessage(FString::Printf(
				TEXT("\u6536\u5230\u6765\u81EA[%s]\u7684\u6B63\u5F0F\u5951\u7EA6\u9080\u8BF7"), *RequesterCharacterName));
		}
		else
		{
			ChatWidget->AddSystemMessage(FString::Printf(
				TEXT("\u6536\u5230\u6765\u81EA[%s]\u7684\u5951\u7EA6\u7533\u8BF7"), *RequesterCharacterName));
		}
	}
}

void AMyBondageLifePlayerController::HandleContractRequestAnswered(const FString& RequestId, bool bAccepted)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (UMyBondageLifeAccountSubsystem* Account = GameInstance ? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		Account->RespondToContractRequest(RequestId, bAccepted);
	}

	bFriendRequestPending = false;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleSocialModalChanged(bool bIsOpen)
{
	bSocialModalOpen = bIsOpen;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleContractsChanged()
{
	// The friend list carries the contract column, the info panel the button state.
	if (FriendListWidget && IsFriendListOpen())
	{
		FriendListWidget->RefreshFriends();
	}
	if (PlayerInfoWidget && IsPlayerInfoOpen())
	{
		PlayerInfoWidget->RefreshContents();
	}
}

void AMyBondageLifePlayerController::ClientHostLeftRoom_Implementation()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Session: host left the room, returning to the main menu."));

	if (ChatWidget)
	{
		ChatWidget->AddSystemMessage(TEXT("\u623F\u4E3B\u5DF2\u9000\u51FA\u623F\u95F4"));   // 房主已退出房间
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeSessionSubsystem* Session = GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>())
		{
			Session->LeaveRoom();
		}
	}
}

void AMyBondageLifePlayerController::HandleHairChanged(int32 NewHairIndex)
{
	// Re-publish the identity so the pawn (and everyone else) wears the new hair.
	ApplyCharacterNameToPawn();
}

// ---------------------------------------------------------------------------
// Animation test
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::HandleRestraintTestKeyPressed()
{
	ToggleRestraintPose();
}

void AMyBondageLifePlayerController::ToggleRestraintPose()
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		const bool bNewState = !TargetCharacter->IsRestraintPoseActive();

		if (HasAuthority())
		{
			TargetCharacter->SetRestraintPose(bNewState);
		}
		else
		{
			ServerSetRestraintPose(bNewState);
		}

		return;
	}

	// No pawn yet (main menu): remember the request and apply it on possession.
	bRestraintPosePending = !bRestraintPosePending;
	UE_LOG(LogMyBondageLife, Log, TEXT("Animation: no pawn yet, restraint pose requested=%d."),
		bRestraintPosePending ? 1 : 0);
}

void AMyBondageLifePlayerController::ServerSetRestraintPose_Implementation(bool bActive)
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		TargetCharacter->SetRestraintPose(bActive);
	}
}

void AMyBondageLifePlayerController::PushRestraintsToPawn()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	const TMap<int32, FString> Equipped = Account->GetEquippedRestraints();

	TArray<int32> Slots;
	TArray<FName> Items;
	for (const TPair<int32, FString>& Pair : Equipped)
	{
		if (Pair.Value.IsEmpty())
		{
			continue;
		}
		Slots.Add(Pair.Key);
		Items.Add(FName(*Pair.Value));
	}

	if (HasAuthority())
	{
		if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
		{
			TargetCharacter->SetRestraints(Slots, Items);
		}
	}
	else
	{
		ServerSetRestraints(Slots, Items);

	// 拘束染色：每次推送拘束时一起下发，远端才能看到颜色
	TArray<FString> RestraintColors;
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const TMap<int32, FString> SlotColors = Account->GetRestraintColors(SlotIndex);
		for (int32 Channel = 1; Channel <= 3; ++Channel)
		{
			const FString* Hex = SlotColors.Find(Channel);
			RestraintColors.Add(Hex ? *Hex : FString());
		}
	}

	if (HasAuthority())
	{
		if (AMyBondageLifeCharacter* ColorCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
		{
			ColorCharacter->SetRestraintColors(RestraintColors);
		}
	}
	else
	{
		ServerSetRestraintColors(RestraintColors);
	}
	}
}

void AMyBondageLifePlayerController::ServerSetRestraints_Implementation(const TArray<int32>& Slots, const TArray<FName>& Items)
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		TargetCharacter->SetRestraints(Slots, Items);
	}
}

void AMyBondageLifePlayerController::HandleRestraintsChanged()
{
	PushRestraintsToPawn();
}
void AMyBondageLifePlayerController::OpenFriendWardrobe(const FString& InAccount)
{
	if (InAccount.IsEmpty())
	{
		return;
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Wardrobe: open friend %s"), *InAccount);

	HidePlayerInfo();

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Account->RefreshRestraintsFor(InAccount);
	}

	// ShowWardrobe() early-returns when the widget already exists, so handle both cases.
	if (WardrobeWidget)
	{
		WardrobeWidget->EnterFriendMode(InAccount);
		WardrobeWidget->SetPanelOpen(true);
	}
	else
	{
		ShowWardrobe();
		if (WardrobeWidget)
		{
			WardrobeWidget->EnterFriendMode(InAccount);
			WardrobeWidget->SetPanelOpen(true);
		}
	}
}

void AMyBondageLifePlayerController::SyncRestraintsFromServer()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		// Nobody else can push to this client, so poll: another player may have
		// equipped / removed / locked something on us.
		// Polling is only the fallback: the realtime socket pushes changes.
		if (!Account->GetAccountName().IsEmpty() && !Account->IsRealtimeConnected())
		{
			Account->RefreshRestraints();
		}
	}
}

void AMyBondageLifePlayerController::ShowWardrobeButton()
{
	if (true)
	{
		// The main HUD owns the wardrobe entry now (shared HUD entry template).
		return;
	}

	if (!IsLocalPlayerController() || WardrobeButtonWidget)
	{
		return;
	}

	if (!WardrobeButtonWidgetClass)
	{
		UClass* DesignedClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_WardrobeEntry.WBP_WardrobeEntry_C"));
		WardrobeButtonWidgetClass = DesignedClass ? DesignedClass : UMyBondageLifeWardrobeButtonWidget::StaticClass();
	}

	WardrobeButtonWidget = CreateWidget<UMyBondageLifeWardrobeButtonWidget>(this, WardrobeButtonWidgetClass);
	if (WardrobeButtonWidget)
	{
		if (!HostInMainHud(TEXT("W_WardrobeEntryHost"), WardrobeButtonWidget))
		{
			WardrobeButtonWidget->AddToViewport(WardrobeButtonZOrder);
			WardrobeButtonWidget->SetPositionInViewport(FVector2D(930.0f, -90.0f), false);
			WardrobeButtonWidget->SetDesiredSizeInViewport(FVector2D(150.0f, 70.0f));
		}
	}
}

void AMyBondageLifePlayerController::ShowPlayerInfoEntry()
{
	if (!IsLocalPlayerController() || PlayerInfoEntryWidget)
	{
		return;
	}

	if (!PlayerInfoEntryWidgetClass)
	{
		UClass* DesignedEntry = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_PlayerInfoEntry.WBP_PlayerInfoEntry_C"));
		PlayerInfoEntryWidgetClass = DesignedEntry ? DesignedEntry : UMyBondageLifePlayerInfoEntryWidget::StaticClass();
	}

	PlayerInfoEntryWidget = CreateWidget<UMyBondageLifePlayerInfoEntryWidget>(this, PlayerInfoEntryWidgetClass);
	if (PlayerInfoEntryWidget)
	{
		PlayerInfoEntryWidget->AddToViewport(PlayerInfoEntryZOrder);
	}
}

// ---------------------------------------------------------------------------
// Message center
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowMessageHud()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!MessageButtonWidgetClass)
	{
		UClass* DesignedButton = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MessageButton.WBP_MessageButton_C"));
		MessageButtonWidgetClass = DesignedButton ? DesignedButton : UMyBondageLifeMessageButtonWidget::StaticClass();
	}
	if (!MessageCenterWidgetClass)
	{
		UClass* DesignedCenter = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MessageCenter.WBP_MessageCenter_C"));
		MessageCenterWidgetClass = DesignedCenter ? DesignedCenter : UMyBondageLifeMessageCenterWidget::StaticClass();
	}


	if (!MessageCenterWidget)
	{
		MessageCenterWidget = CreateWidget<UMyBondageLifeMessageCenterWidget>(this, MessageCenterWidgetClass);
		if (MessageCenterWidget)
		{
			MessageCenterWidget->AddToViewport(60);
			MessageCenterWidget->SetPanelOpen(false);
		}
	}


	// The message entry is created by ShowHudEntries() from the shared HUD entry template.
	if (false)
	{
		MessageButtonWidget = CreateWidget<UMyBondageLifeMessageButtonWidget>(this, MessageButtonWidgetClass);
		if (MessageButtonWidget)
		{
			if (!HostInMainHud(TEXT("W_MessageHost"), MessageButtonWidget))
			{
				MessageButtonWidget->AddToViewport(13);
			}
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnNotificationAdded.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleNotificationAdded);
			Account->RefreshNotifications();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(NotificationPollTimer, this,
			&AMyBondageLifePlayerController::PollNotifications, 12.0f, true);

		World->GetTimerManager().SetTimer(SexLibidoTimer, this,
		&AMyBondageLifePlayerController::TickSexLibido, 3.0f, true);
	}
}

void AMyBondageLifePlayerController::PollNotifications()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->RefreshNotifications();
		}
	}
}

void AMyBondageLifePlayerController::HandleNotificationAdded(const FMyBondageLifeNotification& Notification)
{
	// 主界面（WBP_MainHUD）里自定义的 CreateBoardCastText(Text)：屏幕上给自己一条提示。
	if (MainHudWidget)
	{
		if (UFunction* BoardCastFunction = MainHudWidget->FindFunction(TEXT("CreateBoardCastText")))
		{
			struct FBoardCastParams
			{
				FText Text;
			};

			FBoardCastParams Params;
			Params.Text = FText::FromString(TEXT("\u4F60\u6536\u5230\u4E86\u4E00\u6761\u65B0\u7684\u901A\u77E5"));
			MainHudWidget->ProcessEvent(BoardCastFunction, &Params);
		}
		else
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("HUD: CreateBoardCastText not found on WBP_MainHUD"));
	}
	}

	if (!ChatWidget)
	{
		return;
	}

	FString Kind;
	switch (Notification.Type)
	{
	case EMyBondageLifeNotificationType::FriendRequest:
		Kind = TEXT("\u597D\u53CB\u8BF7\u6C42");
		break;
	case EMyBondageLifeNotificationType::ContractTrial:
		Kind = TEXT("\u7B7E\u8BA2\u4E34\u65F6\u5951\u7EA6");
		break;
	case EMyBondageLifeNotificationType::RomanceRequest:
		Kind = TEXT("\u604B\u4EBA\u7533\u8BF7");
		break;
	case EMyBondageLifeNotificationType::MarriageRequest:
		Kind = TEXT("\u7ED3\u5A5A\u7533\u8BF7");
		break;
	default:
		Kind = TEXT("\u7B7E\u8BA2\u5951\u7EA6");
		break;
	}

	ChatWidget->AddSystemMessage(FString::Printf(
		TEXT("\u6536\u5230\u4E86\u6765\u81EA[%s]\u7684[%s]"), *Notification.CharacterName, *Kind));
}

void AMyBondageLifePlayerController::ToggleMessageCenter()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}


	if (!MessageCenterWidget)
	{
		ShowMessageHud();
	}
	if (MessageCenterWidget)
	{
		MessageCenterWidget->TogglePanel();
	}
}

bool AMyBondageLifePlayerController::IsMessageCenterOpen() const
{
	return MessageCenterWidget && MessageCenterWidget->IsPanelOpen();
}

void AMyBondageLifePlayerController::HandleMessageKeyPressed()
{
	// "Q" belongs to the chat text while typing.
	if (bChatTextInput || IsGameMenuOpen())
	{
		return;
	}

	ToggleMessageCenter();
}

void AMyBondageLifePlayerController::SetMessageCenterOpen(bool bOpen)
{
	if (bMessageCenterOpen == bOpen)
	{
		return;
	}

	bMessageCenterOpen = bOpen;
	RefreshChatInputState();
}
// ---------------------------------------------------------------------------
// Main HUD
// ---------------------------------------------------------------------------

void AMyBondageLifePlayerController::ShowMainHud()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!MainHudWidgetClass)
	{
		UClass* DesignedHud = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MainHUD.WBP_MainHUD_C"));
		MainHudWidgetClass = DesignedHud ? DesignedHud : UMyBondageLifeMainHudWidget::StaticClass();
	}

	if (MainHudWidget)
	{
		return;
	}

	MainHudWidget = CreateWidget<UMyBondageLifeMainHudWidget>(this, MainHudWidgetClass);
	if (MainHudWidget)
	{
		MainHudWidget->AddToViewport(11);
	}
}

bool AMyBondageLifePlayerController::HostInMainHud(FName HostName, UUserWidget* Widget)
{
	if (!MainHudWidget || !Widget)
	{
		return false;
	}

	return MainHudWidget->HostWidget(HostName, Widget);
}

void AMyBondageLifePlayerController::ShowHudEntries()
{
	if (!IsLocalPlayerController() || HudEntries.Num() > 0)
	{
		return;
	}

	// 底栏按钮已由设计器放进 MainHUD：直接找出来绑定，不再动态创建。
	if (MainHudWidget && MainHudWidget->WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		MainHudWidget->WidgetTree->GetAllWidgets(AllWidgets);

		for (UWidget* Candidate : AllWidgets)
		{
			UMyBondageLifeHudButtonWidget* Button = Cast<UMyBondageLifeHudButtonWidget>(Candidate);
			if (!Button)
			{
				continue;
			}

			Button->OnClicked.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleHudButtonClicked);
			HudEntries.Add(Button);
		}

		UE_LOG(LogMyBondageLife, Log, TEXT("Hud: bound %d designed button(s)."), HudEntries.Num());
	}
	// HUD 建立后同步一次规则（决定 W_RuleInfoButton 是否置灰）
	if (UMyBondageLifeAccountSubsystem* HudRuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		HudRuleAccount->RefreshRules();
		HudRuleAccount->OnRomanceActionFailed.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleRomanceFailed);
		HudRuleAccount->OnTractionChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleTractionChanged);
	HudRuleAccount->OnFurnitureChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::ApplyFurnitureState);
	GetWorldTimerManager().SetTimer(FurnitureRefreshTimer, this, &AMyBondageLifePlayerController::RefreshFurnitureStatePeriodic, 5.0f, true);
	RefreshFurnitureStatePeriodic();
		HudRuleAccount->RefreshTractionState();
	}

	// Restraint status icons (gag / arms / legs / cannot move) live in the HUD too.
	if (!RestraintStatusWidget)
	{
		UClass* StatusClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RestraintStatus.WBP_RestraintStatus_C"));
		if (!StatusClass)
		{
			StatusClass = UMyBondageLifeRestraintStatusWidget::StaticClass();
		}

		RestraintStatusWidget = CreateWidget<UMyBondageLifeRestraintStatusWidget>(this, StatusClass);
		if (RestraintStatusWidget)
		{
			if (!HostInMainHud(TEXT("W_StatusHost"), RestraintStatusWidget))
			{
				RestraintStatusWidget->AddToViewport(14);
				RestraintStatusWidget->SetPositionInViewport(FVector2D(20.0f, 20.0f), false);
			}
		}
	}
}

void AMyBondageLifePlayerController::SetRestraintMovementBlocked(bool bBlocked)
{
	if (bRestraintMovementBlocked == bBlocked)
	{
		return;
	}

	bRestraintMovementBlocked = bBlocked;

	// Only movement is disabled: the look axes stay untouched so the camera can turn.
	if (bBlocked)
	{
		SetIgnoreMoveInput(true);
	}
	else
	{
		ResetIgnoreMoveInput();
	}
}

void AMyBondageLifePlayerController::PushOutfitsToPawn()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	TArray<int32> Slots;
	TArray<FName> Items;
	for (const TPair<int32, FString>& Pair : Account->GetEquippedOutfits())
	{
		if (Pair.Value.IsEmpty())
		{
			continue;
		}
		Slots.Add(Pair.Key);
		Items.Add(FName(*Pair.Value));
	}

	TArray<FString> Colors;
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeOutfitCatalog::SlotCount; ++SlotIndex)
	{
		const TMap<int32, FString> SlotColors = Account->GetOutfitColors(SlotIndex);
		for (int32 Channel = 1; Channel <= 3; ++Channel)
		{
			const FString* Hex = SlotColors.Find(Channel);
			Colors.Add(Hex ? *Hex : FString());
		}
	}

	if (HasAuthority())
	{
		if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
		{
			TargetCharacter->SetOutfits(Slots, Items);
			TargetCharacter->SetOutfitColors(Colors);

			TArray<FString> RestraintColors;
			for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
			{
				const TMap<int32, FString> SlotColors = Account->GetRestraintColors(SlotIndex);
				for (int32 Channel = 1; Channel <= 3; ++Channel)
				{
					const FString* Hex = SlotColors.Find(Channel);
					RestraintColors.Add(Hex ? *Hex : FString());
				}
			}
			TargetCharacter->SetRestraintColors(RestraintColors);
		}
	}
	else
	{
		ServerSetOutfits(Slots, Items);
		ServerSetOutfitColors(Colors);
	}
}

void AMyBondageLifePlayerController::ServerSetOutfits_Implementation(const TArray<int32>& Slots, const TArray<FName>& Items)
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		TargetCharacter->SetOutfits(Slots, Items);
	}
}

void AMyBondageLifePlayerController::HandleOutfitsChanged()
{
	PushOutfitsToPawn();
}
void AMyBondageLifePlayerController::PollOtherOutfits()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	const FString OwnAccount = Account->GetAccountName();
	for (TActorIterator<AMyBondageLifeCharacter> It(GetWorld()); It; ++It)
	{
		AMyBondageLifeCharacter* Other = *It;
		if (!Other || Other == GetPawn())
		{
			continue;
		}

		const FString OtherAccount = Other->GetAccountName();
		if (OtherAccount.IsEmpty() || OtherAccount.Equals(OwnAccount, ESearchCase::IgnoreCase))
		{
			continue;
		}

		Account->RefreshOutfitsForAccount(OtherAccount);
	}
}

void AMyBondageLifePlayerController::HandleAccountOutfitsChanged(const FString& Account)
{
	UMyBondageLifeAccountSubsystem* Subsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Subsystem)
	{
		return;
	}

	const TMap<int32, FString> Equipped = Subsystem->GetOutfitsForAccount(Account);

	TArray<int32> Slots;
	TArray<FName> Items;
	for (const TPair<int32, FString>& Pair : Equipped)
	{
		if (Pair.Value.IsEmpty())
		{
			continue;
		}
		Slots.Add(Pair.Key);
		Items.Add(FName(*Pair.Value));
	}

	for (TActorIterator<AMyBondageLifeCharacter> It(GetWorld()); It; ++It)
	{
		AMyBondageLifeCharacter* Other = *It;
		if (Other && Other->GetAccountName().Equals(Account, ESearchCase::IgnoreCase))
		{
			Other->SetOutfits(Slots, Items);
		}
	}
}
void AMyBondageLifePlayerController::ShowSystemChatMessage(const FString& Message)
{
	if (ChatWidget)
	{
		ChatWidget->AddSystemMessage(Message);
	}
	else
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("System chat: %s"), *Message);
	}
}

void AMyBondageLifePlayerController::PushSkinToPawn()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	const int32 SkinId = Account->GetSkinId();
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		if (HasAuthority())
		{
			TargetCharacter->ServerSetSkin(SkinId);
		}
		else
		{
			TargetCharacter->ServerSetSkin(SkinId);
		}
	}
}

void AMyBondageLifePlayerController::HandleSkinChanged(int32 InSkinId)
{
	PushSkinToPawn();
}
void AMyBondageLifePlayerController::HandleSystemChatMessage(const FString& Text)
{
	ShowSystemChatMessage(Text);
}

void AMyBondageLifePlayerController::ServerSetOutfitColors_Implementation(const TArray<FString>& Colors)
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		TargetCharacter->SetOutfitColors(Colors);
	}
}
void AMyBondageLifePlayerController::HandleInteractPressed()
{
	TryToggleSitting();
}

void AMyBondageLifePlayerController::HandleSitKeyPressed()
{
	if (HandleFurnitureInteraction())
	{
		return;
	}

	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}


	// CantSit：规则开启时按 R 直接无效。
	if (const UMyBondageLifeAccountSubsystem* SitAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		if (SitAccount->IsRuleEnabled(TEXT("CantSit")))
		{
			return;
		}
	}

	TryToggleSitting();
}

void AMyBondageLifePlayerController::TryToggleSitting()
{
	AMyBondageLifeCharacter* SittingPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!SittingPawn)
	{
		return;
	}

	if (SittingPawn->IsSitting())
	{
		const FVector StandLocation = SittingPawn->GetActorLocation()
			+ SittingPawn->GetActorForwardVector() * 150.0f;
		SittingPawn->ServerSetSitting(false, StandLocation, SittingPawn->GetActorRotation());
		return;
	}

	if (!NearSeatActor.IsValid())
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Seat: no chair nearby on G press."));
		return;
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Seat: sitting on '%s'."), *NearSeatActor->GetName());

	USceneComponent* SitPoint = nullptr;
	TArray<USceneComponent*> Components;
	NearSeatActor->GetComponents<USceneComponent>(Components);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component->GetFName() == TEXT("SitPoint"))
		{
			SitPoint = Component;
			break;
		}
	}

	const FVector SeatLocation = SitPoint ? SitPoint->GetComponentLocation() : NearSeatActor->GetActorLocation();
	const FRotator SeatRotation = SitPoint ? SitPoint->GetComponentRotation() : NearSeatActor->GetActorRotation();

	// 家具蓝图里的浮点变量 SitOffset（BP float 在 UE5 里是 double）
	float SitOffset = 0.0f;
	AActor* SeatActorPtr = NearSeatActor.Get();
	if (SeatActorPtr)
	{
		if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(SeatActorPtr->GetClass()->FindPropertyByName(TEXT("SitOffset"))))
		{
			SitOffset = static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(SeatActorPtr));
		}
		else if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(SeatActorPtr->GetClass()->FindPropertyByName(TEXT("SitOffset"))))
		{
			SitOffset = FloatProperty->GetPropertyValue_InContainer(SeatActorPtr);
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Seat: SitOffset=%.1f (class=%s)"), SitOffset, *SeatActorPtr->GetClass()->GetName());

	SittingPawn->ServerSetSitting(true, SeatLocation, SeatRotation, SitOffset, NearSeatActor.Get());
}

void AMyBondageLifePlayerController::UpdateSeatProximity()
{
	// CantSit：规则开启时不显示坐下提示，也不允许坐下。
	if (const UMyBondageLifeAccountSubsystem* SitRuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		if (SitRuleAccount->IsRuleEnabled(TEXT("CantSit")))
		{
			if (SeatHintWidget)
			{
				SeatHintWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			return;
		}
	}

	AMyBondageLifeCharacter* SittingPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!SittingPawn)
	{
		return;
	}

	AActor* Best = nullptr;
	float BestDistance = 100.0f;

	if (!SittingPawn->IsSitting())
	{
		// 只扫描一次世界，缓存所有带 SitPoint 的椅子（避免每 0.25 秒 TActorIterator 全场景）。
		if (!bSeatCandidatesBuilt)
		{
			bSeatCandidatesBuilt = true;
			SeatCandidates.Reset();
			SeatCandidateLocations.Reset();
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				AActor* Candidate = *It;
				if (!Candidate)
				{
					continue;
				}

				FVector SitLocation = Candidate->GetActorLocation();
				bool bHasSitPoint = false;
				TArray<USceneComponent*> Components;
				Candidate->GetComponents<USceneComponent>(Components);
				for (USceneComponent* Component : Components)
				{
					if (Component && Component->GetFName() == TEXT("SitPoint"))
					{
						bHasSitPoint = true;
						SitLocation = Component->GetComponentLocation();
						break;
					}
				}

				if (bHasSitPoint)
				{
					SeatCandidates.Add(Candidate);
					SeatCandidateLocations.Add(SitLocation);
				}
			}
		}

		// 进入/退出范围只做距离比较；占用判断只对范围内最近的椅子做。
		for (int32 Index = 0; Index < SeatCandidates.Num(); ++Index)
		{
			AActor* Candidate = SeatCandidates[Index].Get();
			if (!Candidate)
			{
				continue;
			}

			const float Distance = FVector::Dist(SittingPawn->GetActorLocation(), SeatCandidateLocations[Index]);
			if (Distance >= BestDistance)
			{
				continue;
			}

			bool bOccupiedByOther = false;
			for (TActorIterator<AMyBondageLifeCharacter> CharIt(GetWorld()); CharIt; ++CharIt)
			{
				AMyBondageLifeCharacter* OtherCharacter = *CharIt;
				if (OtherCharacter && OtherCharacter != SittingPawn
					&& OtherCharacter->GetOccupiedSeat() == Candidate)
				{
					bOccupiedByOther = true;
					break;
				}
			}

			if (bOccupiedByOther)
			{
				continue;
			}

			BestDistance = Distance;
			Best = Candidate;
		}
	}

	if (Best)
	{
		UE_LOG(LogMyBondageLife, Verbose, TEXT("Seat: near %s (%.0fcm)."), *Best->GetName(), BestDistance);
	}

	NearSeatActor = Best;

	// UI 提示："按[R]坐下"
	const bool bShowHint = Best != nullptr;
	if (bShowHint && !SeatHintWidget)
	{
		UClass* HintClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_SitHint.WBP_SitHint_C"));
		if (!HintClass)
		{
			HintClass = UMyBondageLifeSitHintWidget::StaticClass();
		}
		SeatHintWidget = CreateWidget<UMyBondageLifeSitHintWidget>(this, HintClass);
		if (SeatHintWidget)
		{
			SeatHintWidget->AddToViewport(40);
		}
	}

	if (SeatHintWidget)
	{
		SeatHintWidget->SetVisibility(bShowHint
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		SeatHintWidget->SetPositionInViewport(FVector2D(560.0f, 470.0f), false);
	}
}

void AMyBondageLifePlayerController::HandleStandUpOnMove()
{
	AMyBondageLifeCharacter* MovingPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!MovingPawn || !MovingPawn->IsSitting())
	{
		return;
	}

	const FVector StandLocation = MovingPawn->GetActorLocation()
		+ MovingPawn->GetActorForwardVector() * 150.0f;
	MovingPawn->ServerSetSitting(false, StandLocation, MovingPawn->GetActorRotation());
}
void AMyBondageLifePlayerController::ServerSetRestraintColors_Implementation(const TArray<FString>& Colors)
{
	if (AMyBondageLifeCharacter* TargetCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		TargetCharacter->SetRestraintColors(Colors);
	}
}

void AMyBondageLifePlayerController::PushRestraintColors()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	TArray<FString> Colors;
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const TMap<int32, FString> SlotColors = Account->GetRestraintColors(SlotIndex);
		for (int32 Channel = 1; Channel <= 3; ++Channel)
		{
			const FString* Hex = SlotColors.Find(Channel);
			Colors.Add(Hex ? *Hex : FString());
		}
	}

	if (AMyBondageLifeCharacter* ColorPawn = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		if (HasAuthority())
		{
			ColorPawn->SetRestraintColors(Colors);
		}
		else
		{
			ServerSetRestraintColors(Colors);
		}
	}
}

void AMyBondageLifePlayerController::HandleEmoteKeyPressed()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	ToggleEmoteList();
}

void AMyBondageLifePlayerController::ToggleEmoteList()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	if (!EmoteListWidget)
	{
		UClass* ListClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_EmoteList.WBP_EmoteList_C"));
		if (!ListClass)
		{
			ListClass = UMyBondageLifeEmoteListWidget::StaticClass();
		}

		EmoteListWidget = CreateWidget<UMyBondageLifeEmoteListWidget>(this, ListClass);
		if (!EmoteListWidget)
		{
			return;
		}

		EmoteListWidget->AddToViewport(150);
		EmoteListWidget->OnEmotePicked.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleEmotePicked);
		EmoteListWidget->OnCloseRequested.AddUniqueDynamic(this, &AMyBondageLifePlayerController::CloseEmotePanel);
	}

	if (bEmotePanelOpen)
	{
		CloseEmotePanel();
		return;
	}

	EmoteListWidget->PopulateEmotes();
	EmoteListWidget->SetVisibility(ESlateVisibility::Visible);
		bEmotePanelOpen = true;
		RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleEmotePicked(FName EmoteId)
{
	const FMyBondageLifeEmoteItem* Item = FMyBondageLifeEmoteCatalog::Find(EmoteId);
	AMyBondageLifeCharacter* EmoteCharacter = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!Item || !EmoteCharacter)
	{
		return;
	}

	auto Deny = [this](const FString& Reason)
	{
		ShowSystemChatMessage(Reason);
		if (EmoteListWidget)
		{
		CloseEmotePanel();
		}
		SetShowMouseCursor(false);
	};

	// 状态：1=站立，2=椅子，4=地面，8=宠物（后两者预留）
	const int32 StateBit = EmoteCharacter->IsSitting() ? 2 : 1;
	if (!FMyBondageLifeEmoteCatalog::MatchesState(*Item, StateBit))
	{
		Deny(TEXT("当前状态无法使用该表情"));
		return;
	}

	// 拘束限制：两个都要满足
	if (EmoteCharacter->GetArmActive() && !Item->bArmBound)
	{
		Deny(TEXT("由于拘束无法发送该表情"));
		return;
	}
	if (EmoteCharacter->GetLegActive() && !Item->bLegBound)
	{
		Deny(TEXT("由于拘束无法发送该表情"));
		return;
	}

	UAnimMontage* Montage = Item->Montage.LoadSynchronous();
	if (!Montage)
	{
		Deny(TEXT("该表情没有配置动画"));
		return;
	}

	EmoteCharacter->PlayEmote(Montage);

	// 让其他玩家也看到这个表情
	if (!HasAuthority())
	{
		EmoteCharacter->ServerPlayEmote(Item->Montage.ToSoftObjectPath().ToString());
	}else
	{
		EmoteCharacter->MulticastPlayEmote(Item->Montage.ToSoftObjectPath().ToString());
	}

	// 服务端系统广播，不受口球影响
	if (UMyBondageLifeAccountSubsystem* EmoteAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		EmoteAccount->BroadcastEmote(EmoteId.ToString(), Item->Name);
	}

	if (EmoteListWidget)
	{
		CloseEmotePanel();
	}
	SetShowMouseCursor(false);
	bEmotePanelOpen = false;
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleHudButtonClicked(EMyBondageLifeHudEntryAction Action)
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Hud: clicked action=%d"), static_cast<int32>(Action));

	switch (Action)
	{
	case EMyBondageLifeHudEntryAction::Message:
		ToggleMessageCenter();
		break;
	case EMyBondageLifeHudEntryAction::FriendList:
		ToggleFriendList();
		break;
	case EMyBondageLifeHudEntryAction::Wardrobe:
		ToggleWardrobe();
		break;
	case EMyBondageLifeHudEntryAction::Emote:
		ToggleEmoteList();
		break;
	default:
		TogglePlayerInfo();
		break;
	}
}

void AMyBondageLifePlayerController::CloseEmotePanel()
{
	if (EmoteListWidget)
	{
		EmoteListWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	bEmotePanelOpen = false;
	SetShowMouseCursor(false);

	// 面板可能加过移动/视角忽略，这里全部清干净，避免关闭后无法操作。
	SetIgnoreMoveInput(false);
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetInputMode(FInputModeGameOnly());
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::ToggleRulePanelFor(const FString& TargetAccount)
{
	// 已经打开时只刷新内容，不再关闭（防止按钮触发两次导致一开一合）。
	if (bRulePanelOpen && RulePanelWidget)
	{
		bRuleDetailOpen = false;
	RuleTargetAccount = TargetAccount;
		RulePanelWidget->SetVisibility(ESlateVisibility::Visible);
		RulePanelWidget->PopulateRules(TargetAccount);
		return;
	}

	UMyBondageLifeAccountSubsystem* RuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!RuleAccount)
	{
		return;
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: open requested for target='%s'"), *TargetAccount);

	// 先拉「被查看者」的规则，确认对方有主人且有权限，再开面板。
	TWeakObjectPtr<AMyBondageLifePlayerController> WeakThis(this);
	RuleAccount->RefreshRulesFor(TargetAccount, [WeakThis, TargetAccount](bool bOk)
	{
		AMyBondageLifePlayerController* Self = WeakThis.Get();
		if (!Self)
		{
			return;
		}

		UMyBondageLifeAccountSubsystem* Account = Self->GetGameInstance()
			? Self->GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
		if (!bOk || !Account || !Account->HasMasterAccount())
		{
			Self->ShowSystemChatMessage(TEXT("\u5BF9\u65B9\u6CA1\u6709\u4E3B\u4EBA\uFF0C\u65E0\u6CD5\u67E5\u770B\u5974\u96B6\u89C4\u5219"));
			return;
		}

		Self->OpenRulePanelNow(TargetAccount);
	});
}

void AMyBondageLifePlayerController::OpenRulePanelNow(const FString& TargetAccount)
{
	if (!RulePanelWidget)
	{
		UClass* PanelClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RulePanel.WBP_RulePanel_C"));
		if (!PanelClass)
		{
			PanelClass = UMyBondageLifeRulePanelWidget::StaticClass();
		}

		RulePanelWidget = CreateWidget<UMyBondageLifeRulePanelWidget>(this, PanelClass);
		if (!RulePanelWidget)
		{
			return;
		}

		RulePanelWidget->AddToViewport(600);

		if (RulePanelWidget->WidgetTree && RulePanelWidget->WidgetTree->RootWidget)
		{
			RulePanelWidget->WidgetTree->RootWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogMyBondageLife, Log, TEXT("Rule: root widget %s forced visible"), *RulePanelWidget->WidgetTree->RootWidget->GetName());
		}
		RulePanelWidget->SetIsFocusable(true);

		RulePanelWidget->OnSwitchRequested.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleRuleSwitchRequested);
		RulePanelWidget->OnDetailRequested.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleRuleDetailRequested);
		RulePanelWidget->OnClosed.AddUniqueDynamic(this, &AMyBondageLifePlayerController::CloseRulePanel);

		// 规则数据回来（或保存回包）后，刷新面板里的行。
		if (UMyBondageLifeAccountSubsystem* Sub = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			Sub->OnRulesChanged.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleRulesCacheChanged);
		}
	}

	bRuleDetailOpen = false;
	RuleTargetAccount = TargetAccount;
	RulePanelWidget->SetVisibility(ESlateVisibility::Visible);
	RulePanelWidget->PopulateRules(TargetAccount);

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: panel vis=%d inViewport=%d"), static_cast<int32>(RulePanelWidget->GetVisibility()), RulePanelWidget->IsInViewport() ? 1 : 0);

	bRulePanelOpen = true;
	SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(RulePanelWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMyBondageLifePlayerController::CloseRulePanel()
{
	if (RulePanelWidget)
	{
		RulePanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	bRulePanelOpen = false;
	SetShowMouseCursor(false);
	SetInputMode(FInputModeGameOnly());
	RefreshChatInputState();
}

void AMyBondageLifePlayerController::HandleRuleSwitchRequested(const FString& RuleId, bool bWantEnabled)
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: switch request rule='%s' -> %d target='%s'"), *RuleId, bWantEnabled ? 1 : 0, *RuleTargetAccount);
	if (UMyBondageLifeAccountSubsystem* RuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		// 改的是「被查看者」的规则（RuleTargetAccount），不是自己。
		RuleAccount->SetRule(RuleTargetAccount, RuleId, bWantEnabled, RuleAccount->GetRuleDetail(RuleId));
		RuleAccount->RefreshRulesFor(RuleTargetAccount, TFunction<void(bool)>());
	}else
	{
		ShowSystemChatMessage(TEXT("\u65E0\u6743\u4FEE\u6539\u8BE5\u89C4\u5219"));
	}

	if (RulePanelWidget)
	{
		RulePanelWidget->PopulateRules(RuleTargetAccount);
	}
}

// HandleRuleDetailRequested is defined below.

void AMyBondageLifePlayerController::HandleRulesCacheChanged()
{
	if (bRulePanelOpen && RulePanelWidget)
	{
		RulePanelWidget->PopulateRules(RuleTargetAccount);
	}
}

void AMyBondageLifePlayerController::HandleRuleDetailRequested(const FString& RuleId)
{
	UE_LOG(LogMyBondageLife, Log, TEXT("RuleDetail: request '%s' target='%s'"), *RuleId, *RuleTargetAccount);
	if (!RuleDetailWidget)
	{
		UClass* PanelClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RuleDetail.WBP_RuleDetail_C"));
		if (!PanelClass)
		{
			PanelClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RuleDetailPanel.WBP_RuleDetailPanel_C"));
		}
		if (!PanelClass)
		{
			PanelClass = UMyBondageLifeRuleDetailPanelWidget::StaticClass();
		}

		RuleDetailWidget = CreateWidget<UMyBondageLifeRuleDetailPanelWidget>(this, PanelClass);
		if (!RuleDetailWidget)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("RuleDetail: CreateWidget failed (class=%s)"), PanelClass ? *PanelClass->GetName() : TEXT("none"));
			return;
		}

		RuleDetailWidget->AddToViewport(700);

	UE_LOG(LogMyBondageLife, Log, TEXT("RuleDetail: created %s"), *PanelClass->GetName());
		RuleDetailWidget->OnSaved.AddUniqueDynamic(this, &AMyBondageLifePlayerController::HandleRuleDetailSaved);
		RuleDetailWidget->OnClosed.AddUniqueDynamic(this, &AMyBondageLifePlayerController::CloseRuleDetailPanel);
	}

	RuleDetailWidget->SetVisibility(ESlateVisibility::Visible);
	RuleDetailWidget->BuildForRule(RuleId);
	bRuleDetailOpen = true;
	RefreshChatInputState();

	SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(RuleDetailWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMyBondageLifePlayerController::HandleRuleDetailSaved(const FString& RuleId, const FMyBondageLifeRuleDetail& Detail)
{
	if (UMyBondageLifeAccountSubsystem* RuleAccount = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		RuleAccount->SetRuleDetail(RuleTargetAccount, RuleId, Detail);
		RuleAccount->RefreshRulesFor(RuleTargetAccount, TFunction<void(bool)>());
	}

	CloseRuleDetailPanel();
}

void AMyBondageLifePlayerController::CloseRuleDetailPanel()
{
	if (RuleDetailWidget)
	{
		RuleDetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	bRuleDetailOpen = false;
	SetShowMouseCursor(bRulePanelOpen);
	RefreshChatInputState();
}

bool AMyBondageLifePlayerController::HandleEscapePanels()
{
	// 高潮期间忽略所有快捷键（含 ESC）。
	if (bClimaxLocked)
	{
		return true;
	}

	// 游戏设置最优先关闭。
	if (SettingsWidget && SettingsWidget->IsPanelOpen())
	{
		SettingsWidget->ClosePanel();
		return true;
	}

	// 细节面板优先关闭，其次规则面板。
	if (bRuleDetailOpen)
	{
		CloseRuleDetailPanel();
		return true;
	}

	if (bRulePanelOpen)
	{
		CloseRulePanel();
		return true;
	}

	// 衣柜里的弹层（锁输入 / 锁菜单 / 颜色 / 拘束选择 / 衣服选择）优先于衣柜本体。
	if (WardrobeWidget && WardrobeWidget->CloseTopPopup())
	{
		return true;
	}

	// 表情动作面板。
	if (bEmotePanelOpen)
	{
		CloseEmotePanel();
		return true;
	}

	// 消息通知。
	if (MessageCenterWidget && MessageCenterWidget->IsPanelOpen())
	{
		MessageCenterWidget->CloseCenter();
		return true;
	}
	return false;
}


void AMyBondageLifePlayerController::HandleRomanceFailed(const FString& Code, const FString& Message)
{
	FString Toast;
	if (Code == TEXT("SELF_LIMIT_REACHED"))
	{
		Toast = TEXT("自己已经有了5位恋人");
	}
	else if (Code == TEXT("TARGET_LIMIT_REACHED"))
	{
		Toast = TEXT("对方已经有了5位恋人");
	}
	else if (Code == TEXT("TOO_EARLY"))
	{
		Toast = TEXT("成为恋人满7天后才能结婚");
	}
	else
	{
		return;
	}

	if (MainHudWidget)
	{
		if (UFunction* BoardCastFunction = MainHudWidget->FindFunction(TEXT("CreateBoardCastText")))
		{
			struct FBoardCastParams
			{
				FText Text;
			};

			FBoardCastParams Params;
			Params.Text = FText::FromString(Toast);
			MainHudWidget->ProcessEvent(BoardCastFunction, &Params);
		}
	}
}
// ---------------------------------------------------------------------------
// 性欲值 / 高潮（纯客户端计算）
// ---------------------------------------------------------------------------

float AMyBondageLifePlayerController::ComputeLibidoSum() const
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return 0.0f;
	}

	float Sum = 0.0f;
	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeRestraintCatalog::SlotCount; ++SlotIndex)
	{
		const FString ItemId = Account->GetEquippedRestraint(SlotIndex);
		if (ItemId.IsEmpty())
		{
			continue;
		}

		if (const FMyBondageLifeRestraintItem* Item = FMyBondageLifeRestraintCatalog::Find(FName(*ItemId)))
		{
			Sum += Item->Libido;
		}
	}
	return Sum;
}

void AMyBondageLifePlayerController::TickSexLibido()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	// 高潮 5 秒内冻结性欲值，等表现结束再清零。
	if (bClimaxLocked)
	{
		return;
	}

	const float Sum = ComputeLibidoSum();
	if (Sum <= 0.0f)
	{
		// 没有带 Libido 的拘束：每 3 秒衰减 1.5%，直到 0。
		if (SexValue > 0.0f)
		{
			SexValue = FMath::Max(0.0f, SexValue - 1.5f);
			if (MainHudWidget)
			{
				MainHudWidget->SetSexProgress(SexValue / 100.0f, SexValue, false);
			}
			UpdateLibidoEffects(SexValue);
		}
		return;
	}

	SexValue += Sum;
	if (SexValue >= 100.0f)
	{
		// 先停在满值（界面显示 100%），5 秒高潮结束后再清零。
		SexValue = 100.0f;
		OnOrgasmReached();
		BeginClimaxSequence();

		// 全房间可见的公开文本。
		const FString Name = Account->GetCharacterName().IsEmpty() ? TEXT("有人") : Account->GetCharacterName();
		Account->BroadcastSystemText(FString::Printf(TEXT("%s达到了高潮"), *Name), FString());
	}

	if (MainHudWidget)
	{
		MainHudWidget->SetSexProgress(SexValue / 100.0f, SexValue, true);
	}

	UpdateLibidoEffects(SexValue);

	{
	}
}
void AMyBondageLifePlayerController::UpdateLibidoEffects(float Value)
{
	const int32 Level = (Value >= 70.0f) ? 2 : ((Value >= 40.0f) ? 1 : 0);
	if (Level == LibidoEffectLevel)
	{
		return;
	}
	LibidoEffectLevel = Level;

	if (UWorld* AudioWorld = GetWorld())
	{
		if (Level >= 1)
		{
			AudioWorld->GetTimerManager().SetTimer(LibidoAudioTimer, this,
				&AMyBondageLifePlayerController::TickLibidoAudio, 0.35f, true);
		}
		else
		{
			AudioWorld->GetTimerManager().ClearTimer(LibidoAudioTimer);
		}
	}

	// 画面：交给角色的后处理合并（与蒙眼共存）。
	if (AMyBondageLifeCharacter* LibidoCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		LibidoCharacter->SetLibidoScreenLevel(Level);
	}

	// 音效：40% 心跳，70% 喘息；低于阈值即停。
	auto EnsureAudio = [this](TObjectPtr<UAudioComponent>& Comp, const TCHAR* Path, bool bPlay)
	{
		if (bPlay)
		{
			if (!Comp)
			{
				Comp = NewObject<UAudioComponent>(this);
				Comp->bAutoDestroy = false;
				Comp->bIsUISound = true;
				Comp->bAllowSpatialization = false;
				Comp->RegisterComponent();
			}
			if (!Comp->Sound)
			{
				Comp->SetSound(LoadObject<USoundBase>(nullptr, Path));
			}
			if (Comp->Sound && !Comp->IsPlaying())
			{
				Comp->Play();
			}
		}
		else if (Comp && Comp->IsPlaying())
		{
			Comp->Stop();
		}
	};

	EnsureAudio(HeartbeatAudio, TEXT("/Game/Characters/Sound/Sex/Sound_Heartbeat_Cue.Sound_Heartbeat_Cue"), Level >= 1);
	EnsureAudio(GaspingAudio, TEXT("/Game/Characters/Sound/Sex/Sound_Gasping_Cue.Sound_Gasping_Cue"), Level >= 2);
}

void AMyBondageLifePlayerController::TickLibidoAudio()
{
	auto KeepPlaying = [](TObjectPtr<UAudioComponent>& Comp)
	{
		if (Comp && Comp->Sound && !Comp->IsPlaying())
		{
			Comp->Play();
		}
	};

	if (LibidoEffectLevel >= 1)
	{
		KeepPlaying(HeartbeatAudio);
	}
	if (LibidoEffectLevel >= 2)
	{
		KeepPlaying(GaspingAudio);
	}
}
void AMyBondageLifePlayerController::BeginClimaxSequence()
{
	if (bClimaxLocked)
	{
		return;
	}
	bClimaxLocked = true;

	if (AMyBondageLifeCharacter* ClimaxCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		ClimaxCharacter->BeginClimax();
	}

	DisableInput(this);
	// 只锁移动，保留鼠标旋转视角；同时让角色 Move() 直接返回，防止 EnhancedInput 绕过忽略标记。
	SetIgnoreMoveInput(true);
	if (AMyBondageLifeCharacter* ClimaxPawn = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		ClimaxPawn->bBlockMoveInput = true;
	}

	// 临时隐藏 HUD / 聊天。
	ClimaxHiddenHud = MainHudWidget;
	if (ClimaxHiddenHud)
	{
		ClimaxSavedHudVisibility = ClimaxHiddenHud->GetVisibility();
		ClimaxHiddenHud->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ChatWidget)
	{
		ChatWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UWorld* ClimaxWorld = GetWorld())
	{
		ClimaxWorld->GetTimerManager().SetTimer(ClimaxTimer, this,
			&AMyBondageLifePlayerController::EndClimaxSequence, 10.0f, false);
	}
}

void AMyBondageLifePlayerController::EndClimaxSequence()
{
	bClimaxLocked = false;

	// 5 秒高潮结束：性欲值此时才清零，并同步界面与阈值效果。
	SexValue = 0.0f;
	if (MainHudWidget)
	{
		MainHudWidget->SetSexProgress(0.0f, 0.0f, false);
	}
	UpdateLibidoEffects(0.0f);

	if (AMyBondageLifeCharacter* ClimaxCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		ClimaxCharacter->EndClimax();
	}

	EnableInput(this);
	SetIgnoreMoveInput(false);
	if (AMyBondageLifeCharacter* ClimaxPawn = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		ClimaxPawn->bBlockMoveInput = false;
	}

	if (ClimaxHiddenHud)
	{
		ClimaxHiddenHud->SetVisibility(ClimaxSavedHudVisibility);
		ClimaxHiddenHud = nullptr;
	}
	if (ChatWidget)
	{
		ChatWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void AMyBondageLifePlayerController::ToggleSettings()
{
	if (bInFurniture) { return; }
	if (bTractionFollower) { return; }
	if (bClimaxLocked)
	{
		return;
	}

	if (!SettingsWidget)
	{
		SettingsWidget = CreateWidget<UMyBondageLifeSettingsWidget>(this, UMyBondageLifeSettingsWidget::StaticClass());
		if (SettingsWidget)
		{
			SettingsWidget->AddToViewport(600);
		}
	}

	if (!SettingsWidget)
	{
		return;
	}

	SettingsWidget->TogglePanel();
}

void AMyBondageLifePlayerController::StartBondageWear(int32 SlotIndex, FName ItemId, AMyBondageLifeCharacter* Target, float Duration, bool bSelfTarget, const FString& TargetAccount)
{
	AMyBondageLifeCharacter* Binder = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!Binder || !Target)
	{
		return;
	}

	Binder->ServerStartBondage(Target, Duration, bSelfTarget, SlotIndex, ItemId, TargetAccount);
}

void AMyBondageLifePlayerController::ClientBondageStarted(float Duration, int32 InRole, int32 SlotIndex, FName ItemId, const FString& BinderName, const FString& TargetName, const FString& TargetAccount)
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Bondage: ClientBondageStarted dur=%.2f role=%d"), Duration, InRole);

	bBondageUiActive = true;
	BondageDuration = FMath::Max(0.1f, Duration);
	BondageElapsed = 0.0f;
	bBondageSelfTarget = (InRole == 2);
	bBondageBinder = (InRole != 1);
	BondagePendingSlot = SlotIndex;
	BondagePendingItem = ItemId;
	BondageBinderName = BinderName;
	BondageTargetName = TargetName;
	BondageTargetAccount = TargetAccount;

	SetIgnoreMoveInput(true);

	// 彻底禁止移动（EnhancedInput 的移动不受 SetIgnoreMoveInput 影响）。
	if (ACharacter* BondagePawn = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* Movement = BondagePawn->GetCharacterMovement())
		{
			Movement->DisableMovement();
		}
	}

	const FString Label = (InRole == 1) ? TEXT("受缚中（按Space抵抗）") : TEXT("捆绑中（按Space取消）");
	if (MainHudWidget)
	{
		MainHudWidget->ShowBondageProgress(0.0f, Label);
	}

	GetWorldTimerManager().SetTimer(BondageProgressTimer, [this, Label]()
	{
		BondageElapsed += 0.05f;
		const float Alpha = FMath::Clamp(BondageElapsed / BondageDuration, 0.0f, 1.0f);
		if (MainHudWidget)
		{
			MainHudWidget->ShowBondageProgress(Alpha, Label);
		}

		if (Alpha >= 1.0f)
		{
			GetWorldTimerManager().ClearTimer(BondageProgressTimer);
		}
	}, 0.05f, true);
}

void AMyBondageLifePlayerController::ClientBondageEnded(bool bSuccess, bool bResisted, const FString& BinderName, const FString& TargetName, const FString& TargetAccount)
{
	if (!bBondageUiActive)
	{
		return;
	}

	bBondageUiActive = false;
	GetWorldTimerManager().ClearTimer(BondageProgressTimer);
	SetIgnoreMoveInput(false);

	if (ACharacter* BondagePawn = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* Movement = BondagePawn->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
	}
	}

	if (MainHudWidget)
	{
		MainHudWidget->HideBondageProgress();
	}

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	if (bSuccess)
	{
		// 只有拘束者客户端负责落地穿戴，避免双方各穿一次。
		if (!bBondageBinder)
		{
			return;
		}

		UE_LOG(LogMyBondageLife, Log, TEXT("Bondage: equip targetAccount=[%s] own=[%s] slot=%d item=%s"),
			*BondageTargetAccount, *Account->GetAccountName(), BondagePendingSlot, *BondagePendingItem.ToString());

		if (BondagePendingSlot >= 0 && !BondagePendingItem.IsNone())
		{
			if (BondageTargetAccount.IsEmpty() || BondageTargetAccount.Equals(Account->GetAccountName(), ESearchCase::IgnoreCase))
			{
				Account->EquipRestraint(BondagePendingSlot, BondagePendingItem.ToString());
			}
			else
			{
				Account->EquipRestraintFor(BondageTargetAccount, BondagePendingSlot, BondagePendingItem.ToString());
			}
		}
	}
	else
	{
		// 只有发起方负责发公开文本，避免重复。
		if (bBondageBinder)
		{
			Account->BroadcastSystemText(
				bResisted
					? FString::Printf(TEXT("%s抵抗了%s的拘束"), *TargetName, *BinderName)
					: FString::Printf(TEXT("%s取消了对%s的拘束"), *BinderName, *TargetName),
				FString());
		}
	}
}

void AMyBondageLifePlayerController::CancelBondage()
{
	if (!bBondageUiActive)
	{
		return;
	}

	if (AMyBondageLifeCharacter* BondageChar = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		BondageChar->ServerCancelBondage();
	}
}

FString AMyBondageLifePlayerController::GetOwnAccountName() const
{
	if (const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		return Account->GetAccountName();
	}
	return FString();
}

void AMyBondageLifePlayerController::CloseWardrobeForBondage()
{
	if (WardrobeWidget)
	{
		WardrobeWidget->SetPanelOpen(false);
	}
}

void AMyBondageLifePlayerController::ServerAssignAccount_Implementation(const FString& InAccount)
{
	if (InAccount.IsEmpty())
	{
		return;
	}

	if (PlayerState)
	{
		PlayerState->SetPlayerName(InAccount);
	}
	if (AMyBondageLifeCharacter* AssignedCharacter = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		AssignedCharacter->SetAccountNameFree(InAccount);
	}
}


void AMyBondageLifePlayerController::ApplyUIFontsPass()
{
	auto ApplyTo = [](UUserWidget* Widget)
	{
		if (Widget)
		{
			MyBondageLifeUI::ApplyFonts(Widget);
		}
	};

	ApplyTo(MainHudWidget);
	ApplyTo(ChatWidget);
	ApplyTo(WardrobeWidget);
	ApplyTo(PlayerInfoWidget);
	ApplyTo(FriendListWidget);
	ApplyTo(MessageCenterWidget);
	ApplyTo(SettingsWidget);
	ApplyTo(LoginWidget);
	ApplyTo(GameMenuWidget);
	ApplyTo(HostInfoWidget);
	ApplyTo(RulePanelWidget);
	ApplyTo(EmoteListWidget);
	ApplyTo(SeatHintWidget);

	++UIFontPassCount;
}

void AMyBondageLifePlayerController::ToggleTractionFor(const FString& TargetAccount)
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account || TargetAccount.IsEmpty())
	{
		return;
	}

	const FString MyName = Account->GetCharacterName();
	const bool bAlready = Account->GetTractionFollowers().Contains(TargetAccount);

	// 聊天只由发起方广播一次，用角色名。
	auto BroadcastOnce = [Account](const FString& Text)
	{
		Account->BroadcastSystemText(Text, FString());
	};

	if (bAlready)
	{
		Account->StopTraction(TargetAccount);
		BroadcastOnce(FString::Printf(TEXT("%s结束了对%s的牵引。"), *MyName, *TargetAccount));
	}
	else
	{
		Account->StartTraction(TargetAccount);
		BroadcastOnce(FString::Printf(TEXT("%s开始了对%s的牵引。"), *MyName, *TargetAccount));
	}
}

void AMyBondageLifePlayerController::HandleTractionChanged()
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	TractionLeaderAccount = Account->GetTractionLeader();
	bTractionFollower = !TractionLeaderAccount.IsEmpty();

	// 找房间内的牵引者角色。
	TractionLeaderActor = nullptr;
	if (bTractionFollower && GetWorld())
	{
		for (TActorIterator<AMyBondageLifeCharacter> It(GetWorld()); It; ++It)
		{
			AMyBondageLifeCharacter* Candidate = *It;
			if (Candidate && Candidate->GetAccountName().Equals(TractionLeaderAccount, ESearchCase::IgnoreCase))
			{
				TractionLeaderActor = Candidate;
				break;
			}
		}
	}

	SetIgnoreMoveInput(bTractionFollower);

	if (!bTractionFollower && FollowerOriginalWalkSpeed >= 0.0f)
	{
		if (ACharacter* RestoreChar = Cast<ACharacter>(GetPawn()))
		{
			if (UCharacterMovementComponent* RestoreMove = RestoreChar->GetCharacterMovement())
			{
				RestoreMove->MaxWalkSpeed = FollowerOriginalWalkSpeed;
			}
		}
		FollowerOriginalWalkSpeed = -1.0f;
	}

	if (AMyBondageLifeCharacter* Controlled = Cast<AMyBondageLifeCharacter>(GetPawn()))
	{
		Controlled->bBlockMoveInput = bTractionFollower;
	}


	if (MainHudWidget)
	{
		MainHudWidget->SetTractionOverlayVisible(bTractionFollower);
	}
}

void AMyBondageLifePlayerController::UpdateTractionFollow(float DeltaTime)
{
	if (!bTractionFollower || !TractionLeaderActor.IsValid())
	{
		return;
	}

	AMyBondageLifeCharacter* Follower = Cast<AMyBondageLifeCharacter>(GetPawn());
	AMyBondageLifeCharacter* Leader = TractionLeaderActor.Get();
	if (!Follower || !Leader || Follower == Leader)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Follower->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 跟随速度为牵引者的 0.9 倍。
	if (const UCharacterMovementComponent* LeaderMove = Leader->GetCharacterMovement())
	{
		if (FollowerOriginalWalkSpeed < 0.0f)
		{
			FollowerOriginalWalkSpeed = MoveComp->MaxWalkSpeed;
		}
		MoveComp->MaxWalkSpeed = LeaderMove->MaxWalkSpeed * 0.9f;
	}

	const float Distance = FVector::Dist(Follower->GetActorLocation(), Leader->GetActorLocation());
	if (Distance > 120.0f)
	{
		const FVector Direction = (Leader->GetActorLocation() - Follower->GetActorLocation()).GetSafeNormal2D();
		Follower->AddMovementInput(Direction, 1.0f);
		Follower->SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
	}
	else
	{
		// 距离足够近：不施加移动输入。
	}
}

void AMyBondageLifePlayerController::UpdateTractionChain()
{
	UWorld* World = GetWorld();
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!World || !Account)
	{
		return;
	}

	AMyBondageLifeCharacter* Local = Cast<AMyBondageLifeCharacter>(GetPawn());
	AMyBondageLifeCharacter* LeaderChar = nullptr;
	AMyBondageLifeCharacter* FollowerChar = nullptr;

	if (bTractionFollower)
	{
		FollowerChar = Local;
		LeaderChar = TractionLeaderActor.Get();
	}
	else if (Account->GetTractionFollowers().Num() > 0)
	{
		LeaderChar = Local;
		for (TActorIterator<AMyBondageLifeCharacter> It(World); It; ++It)
		{
			if (IsValid(*It) && (*It)->GetAccountName() == Account->GetTractionFollowers()[0])
			{
				FollowerChar = *It;
				break;
			}
		}
	}

	if (!IsValid(LeaderChar) || !IsValid(FollowerChar))
	{
		if (IsValid(ChainCableActor))
		{
			ChainCableActor->Destroy();
			ChainCableActor = nullptr;
		}
		return;
	}

	// 蓝图 ChainCable：创建一次并调用 CreateLink(Master, Slave)。
	if (!IsValid(ChainCableActor))
	{
		UClass* CableClass = LoadClass<AActor>(nullptr, TEXT("/Game/BP/Chain/ChainCable.ChainCable_C"));
		if (!CableClass)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Traction: ChainCable class not found"));
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ChainCableActor = World->SpawnActor<AActor>(CableClass, LeaderChar->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

		if (UFunction* CreateLink = ChainCableActor ? ChainCableActor->FindFunction(TEXT("CreateLink")) : nullptr)
		{
			struct FCreateLinkParams
			{
				AActor* Master = nullptr;
				AActor* Slave = nullptr;
			};

			FCreateLinkParams Params;
			Params.Master = LeaderChar;
			Params.Slave = FollowerChar;
			ChainCableActor->ProcessEvent(CreateLink, &Params);
		}
		else
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Traction: ChainCable has no CreateLink"));
		}
	}
}

void AMyBondageLifePlayerController::HandleFriendInteractKeyPressed()
{
	if (bInFurniture) { return; }

	if (bTractionFollower) { return; }
	if (bClimaxLocked) { return; }

	// 靠近其他玩家时，G 打开对方的拘束页签（衣柜好友模式 → 拘束）。
	if (NearbyCharacter.IsValid())
	{
		const FString NearbyAccount = NearbyCharacter->GetAccountName();
		if (NearbyAccount.IsEmpty())
		{
			return;
		}

		// 只有好友才能用 G 打开对方拘束页。
		bool bIsFriend = false;
		if (const UMyBondageLifeAccountSubsystem* FriendAccount = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
		for (const FMyBondageLifeFriendEntry& Entry : FriendAccount->GetFriends())
		{
			if (Entry.AccountName.Equals(NearbyAccount, ESearchCase::IgnoreCase))
			{
				bIsFriend = true;
				break;
			}
	}
	}

	if (!bIsFriend)
	{
		return;
	}

	OpenFriendWardrobe(NearbyAccount);
	if (WardrobeWidget)
	{
		WardrobeWidget->SelectTab(2);
	}
	return;
	}
}

// ---------------------------------------------------------------------------
// 固定式家具
// ---------------------------------------------------------------------------
FString AMyBondageLifePlayerController::GetFurnitureHostAccount() const
{
	if (const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		if (const UMyBondageLifeSessionSubsystem* Session = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeSessionSubsystem>() : nullptr)
		{
			if (Session->IsHosting())
			{
				return Account->GetAccountName();
			}
		}

		if (const UMyBondageLifeRoomSubsystem* Rooms = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeRoomSubsystem>() : nullptr)
		{
			const FString Host = Rooms->GetJoinedHostAccount();
			if (!Host.IsEmpty())
			{
				return Host;
			}
		}

		return Account->GetAccountName();
	}
	return FString();
}

void AMyBondageLifePlayerController::RefreshFurnitureStatePeriodic()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	const FString Host = GetFurnitureHostAccount();
	if (Account && !Host.IsEmpty())
	{
		Account->RefreshFurnitureState(Host);
	}
}

bool AMyBondageLifePlayerController::CanLockOccupant(const FMyBondageLifeFurnitureState& State) const
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account || State.Occupant.IsEmpty())
	{
		return false;
	}

	const FString Me = Account->GetAccountName();
	// 占用者自己不能上锁。
	if (State.Occupant.Equals(Me, ESearchCase::IgnoreCase))
	{
		return false;
	}

	// 房主或被占用者的契约主人。
	if (State.HostAccount.Equals(Me, ESearchCase::IgnoreCase))
	{
		return true;
	}
	return Account->GetContractRoleWith(State.Occupant) == EMyBondageLifeContractRole::Master;
}

bool AMyBondageLifePlayerController::CanUnlockOccupant(const FMyBondageLifeFurnitureState& State) const
{
	const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return false;
	}

	const FString Me = Account->GetAccountName();
	const bool bIOccupy = State.Occupant.Equals(Me, ESearchCase::IgnoreCase);
	const bool bIHost = State.HostAccount.Equals(Me, ESearchCase::IgnoreCase);

	// 房主被锁在自己家具里：不能自解（只能安全词/其主人）。
	if (bIHost && bIOccupy)
	{
		return false;
	}
	if (bIHost)
	{
		return true;
	}
	if (!State.Occupant.IsEmpty()
		&& Account->GetContractRoleWith(State.Occupant) == EMyBondageLifeContractRole::Master)
	{
		return true;
	}
	return false;
}

void AMyBondageLifePlayerController::ShowFurnitureHint(const FString& Text)
{
	if (!FurnitureHintWidget)
	{
		UClass* HintClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_SitHint.WBP_SitHint_C"));
		if (!HintClass)
		{
			HintClass = UMyBondageLifeSitHintWidget::StaticClass();
		}
		FurnitureHintWidget = CreateWidget<UMyBondageLifeSitHintWidget>(this, HintClass);
		if (FurnitureHintWidget)
		{
			FurnitureHintWidget->AddToViewport(36);
		}
	}
	if (FurnitureHintWidget)
	{
		if (LastFurnitureHint != Text)
		{
			LastFurnitureHint = Text;
			FurnitureHintWidget->SetHintText(Text);
		}
		FurnitureHintWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void AMyBondageLifePlayerController::UpdateFurnitureProximity()
{
	UWorld* World = GetWorld();
	AMyBondageLifeCharacter* OwnPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;

	NearbyFurniture = nullptr;
	if (World && OwnPawn && Account)
	{
		float Best = 260.0f;
		for (TActorIterator<AMyBondageLifeFurnitureBase> It(World); It; ++It)
		{
			AMyBondageLifeFurnitureBase* Furniture = *It;
			if (!IsValid(Furniture))
			{
				continue;
			}
			const float Distance = FVector::Dist(OwnPawn->GetActorLocation(), Furniture->GetInteractionLocation());
			if (Distance <= FMath::Min(Best, Furniture->InteractRange))
			{
				Best = Distance;
				NearbyFurniture = Furniture;
			}
		}
	}

	AMyBondageLifeFurnitureBase* Furniture = NearbyFurniture.Get();
	const FMyBondageLifeFurnitureState* State = (Furniture && Account)
		? Account->FindFurniture(Furniture->GetFurnitureKey()) : nullptr;

	const bool bShow = Furniture != nullptr;
	const FString Occupant = State ? State->Occupant : FString();
	const FString OccupantName = State && !State->OccupantName.IsEmpty()
		? State->OccupantName : Occupant;
	const FString OccupantText = Occupant.IsEmpty()
		? FString(TEXT("空闲中"))
		: FString::Printf(TEXT("%s正在使用中..."), *OccupantName);

	if (MainHudWidget && Furniture)
	{
		MainHudWidget->SetInterFurnitureInfo(bShow, Furniture->GetFurnitureDisplayName(), OccupantText);

	}
	else if (MainHudWidget)
	{
		MainHudWidget->SetInterFurnitureInfo(false, FString(), FString());
	}

	if (!bShow)
	{
		return;
	}

	const bool bMine = State && !State->Occupant.IsEmpty()
		&& State->Occupant.Equals(Account->GetAccountName(), ESearchCase::IgnoreCase);
	// 服务端已记录我在使用该家具但本地还没吸附：立即补上（避免事件竞态导致只变 UI 不动人）。
	if (bMine && OwnPawn)
	{
		bInFurniture = true;
		OwnPawn->bBlockMoveInput = true;   // 每帧强制，家具交互期间禁止移动
	}

	if (bMine && (!bInFurniture || ActiveFurniture.Get() != Furniture))
	{
		AttachToFurniture(Furniture);
	}
	else if (!bMine && bInFurniture && ActiveFurniture.Get() == Furniture && Account->GetFurnitureStates().Num() > 0)
	{
		DetachFromFurniture(false);
	}
	

	if (bInFurniture && bMine)
	{
		ShowFurnitureHint(State->bLocked ? TEXT("已被锁，无法逃脱") : TEXT("按[R]退出"));
	}
	else if (Occupant.IsEmpty())
	{
		ShowFurnitureHint(TEXT("按[R]进入"));
	}
	else if (State && !State->bLocked && CanLockOccupant(*State))
	{
		ShowFurnitureHint(TEXT("按[R]上锁"));
	}
	else if (State && State->bLocked && CanUnlockOccupant(*State))
	{
		ShowFurnitureHint(TEXT("按[R]解锁"));
	}
}

bool AMyBondageLifePlayerController::HandleFurnitureInteraction()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return false;
	}

	const FString Host = GetFurnitureHostAccount();
	if (Host.IsEmpty())
	{
		return false;
	}

	// 已经在家具里：只能退出（被锁时不行）。
	if (bInFurniture)
	{
		AMyBondageLifeFurnitureBase* Active = ActiveFurniture.Get();
		const FMyBondageLifeFurnitureState* State = Active ? Account->FindFurniture(Active->GetFurnitureKey()) : nullptr;
		if (!Active)
		{
			DetachFromFurniture(false);
			return true;
		}
		if (State && State->bLocked)
		{
			ShowFurnitureHint(TEXT("已被锁，无法逃脱"));
			return true;
		}
		// 本地先解除吸附（服务端确认后再刷新状态），避免等待 5 秒轮询显得卡住。
		const FString KeyToExit = Active->GetFurnitureKey();
		DetachFromFurniture(true);
		Account->ExitFurniture(KeyToExit, Host);
		return true;
	}

	AMyBondageLifeFurnitureBase* Furniture = NearbyFurniture.Get();
	if (!Furniture)
	{
		return false;
	}

	const FMyBondageLifeFurnitureState* State = Account->FindFurniture(Furniture->GetFurnitureKey());
	const FString Key = Furniture->GetFurnitureKey();

	if (!State || State->Occupant.IsEmpty())
	{
		Account->EnterFurniture(Key, Host);
		return true;
	}

	if (!State->bLocked && CanLockOccupant(*State))
	{
		Account->LockFurniture(Key, Host);
		return true;
	}
	if (State->bLocked && CanUnlockOccupant(*State))
	{
		Account->UnlockFurniture(Key, Host);
		return true;
	}

	return true;
}

void AMyBondageLifePlayerController::AttachToFurniture(AMyBondageLifeFurnitureBase* Furniture)
{
	AMyBondageLifeCharacter* OwnPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (!Furniture || !OwnPawn)
	{
		return;
	}

	ActiveFurniture = Furniture;
	bInFurniture = true;

	// 视角接管：若家具配置了相机点，用它生成临时 CameraActor。
	if (USceneComponent* CamPoint = Furniture->GetInteractionCameraComponent())
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("Furniture: camera=%s"), *CamPoint->GetName());
	}
	else
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Furniture: InteractionCamera not found (name it exactly InteractionCamera)"));
	}

	if (USceneComponent* CamPoint = Furniture->GetInteractionCameraComponent())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FurnitureCameraActor = GetWorld()->SpawnActor<AActor>(ACameraActor::StaticClass(), CamPoint->GetComponentTransform(), Params);
		if (FurnitureCameraActor)
		{
			SetViewTargetWithBlend(FurnitureCameraActor, 0.25f);
		}
	}

	OwnPawn->bBlockMoveInput = true;
	OwnPawn->ApplyFullAnmi(true);
	OwnPawn->ServerSetFullAnmi(true);
	FTransform SocketTransform = Furniture->GetInteractionTransform();
	if (const UCapsuleComponent* Capsule = OwnPawn->GetCapsuleComponent())
	{
		SocketTransform.AddToTranslation(FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight()));
	}
	OwnPawn->SetActorTransform(SocketTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (UAnimMontage* Montage = Furniture->GetInteractionMontage())
	{
		OwnPawn->PlayAnimMontage(Montage);            // 本地立即播放
		OwnPawn->ServerPlayFurnitureMontage(Montage); // 同步给其他人
	}
}

void AMyBondageLifePlayerController::DetachFromFurniture(bool bMoveForward)
{
	AMyBondageLifeCharacter* OwnPawn = Cast<AMyBondageLifeCharacter>(GetPawn());
	if (OwnPawn)
	{
		OwnPawn->bBlockMoveInput = false;
		OwnPawn->ApplyFullAnmi(false);
		OwnPawn->ServerSetFullAnmi(false);
		OwnPawn->ServerStopFurnitureMontage();
		if (bMoveForward)
		{
			OwnPawn->SetActorLocation(OwnPawn->GetActorLocation() + OwnPawn->GetActorForwardVector() * 150.0f, false);
		}
	}
	if (FurnitureCameraActor)
	{
		SetViewTargetWithBlend(OwnPawn, 0.25f);
		FurnitureCameraActor->Destroy();
		FurnitureCameraActor = nullptr;
	}

	ActiveFurniture = nullptr;
	bInFurniture = false;
}

void AMyBondageLifePlayerController::ApplyFurnitureState()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	const FString Me = Account->GetAccountName();
	const FMyBondageLifeFurnitureState* MyState = nullptr;
	for (const FMyBondageLifeFurnitureState& Item : Account->GetFurnitureStates())
	{
		if (!Item.Occupant.IsEmpty() && Item.Occupant.Equals(Me, ESearchCase::IgnoreCase))
		{
			MyState = &Item;
			break;
		}
	}

	if (MyState)
	{
		if (!bInFurniture || !ActiveFurniture.IsValid())
		{
			UWorld* World = GetWorld();
			PendingFurnitureKey = MyState->FurnitureId;
			if (World)
			{
				for (TActorIterator<AMyBondageLifeFurnitureBase> It(World); It; ++It)
				{
					if (IsValid(*It) && (*It)->GetFurnitureKey().Equals(MyState->FurnitureId, ESearchCase::IgnoreCase))
					{
						AttachToFurniture(*It);
						break;
					}
				}
			}
		}
	}
	else if (bInFurniture)
	{
		DetachFromFurniture(true);
	}
}
