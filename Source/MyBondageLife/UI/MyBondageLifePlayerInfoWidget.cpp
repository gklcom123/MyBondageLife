// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifePlayerInfoWidget.h"
#include "MyBondageLifePlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerState.h"
#include "MyBondageLife.h"
#include "MyBondageLifeCharacter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Canvas units are 1.5x the viewport pixels at the default UI scale.
	constexpr float EntryIconSize = 64.0f;
	constexpr float EntryPositionX = 636.0f;
	constexpr float EntryPositionY = -30.0f;

	constexpr float PanelWidth = 1500.0f;
	constexpr float PanelHeight = 840.0f;
	constexpr float CloseButtonSize = 52.0f;

	// "个人信息（C）"
	const TCHAR* EntryLabelText = TEXT("\u4E2A\u4EBA\u4FE1\u606F\uFF08C\uFF09");
	// "按F打开信息以交互"
	const TCHAR* InteractionHintText = TEXT("\u6309F\u6253\u5F00\u4FE1\u606F\u4EE5\u4EA4\u4E92");
}

UMyBondageLifePlayerInfoWidget::UMyBondageLifePlayerInfoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Full screen overlay: the widget itself must not swallow clicks, otherwise the
	// overlays below it (and their buttons) never receive the mouse.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Hard reference so the icon is cooked into packaged builds.
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeadIconFinder(
		TEXT("/Game/2DAssest/UI/UI_HeadIcon.UI_HeadIcon"));
	if (HeadIconFinder.Succeeded())
	{
		HeadIconTexture = HeadIconFinder.Object;
	}
}

TSharedRef<SWidget> UMyBondageLifePlayerInfoWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		if (TryAdoptBlueprintLayout())
		{
			WireAdoptedLayout();
		}
		else
		{
			BuildLayout();
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifePlayerInfoWidget::BuildLayout()
{
	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}
	// A full screen overlay must not swallow clicks meant for the layers below it;
	// only its children (buttons, panels) take part in hit testing.
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// ------------------------------------------------------------ bottom entry
	OpenButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PlayerInfoButton"));
	{
		// No chrome: only the icon and the caption are visible.
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		OpenButton->SetStyle(ButtonStyle);
	}
	OpenButton->OnClicked.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleOpenButtonClicked);

	// Bottom-left anchor, hugging the chat box and growing upward.
	if (UCanvasPanelSlot* EntrySlot = RootCanvas->AddChildToCanvas(OpenButton))
	{
		EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		EntrySlot->SetPosition(FVector2D(EntryPositionX, EntryPositionY));
		EntrySlot->SetAutoSize(true);
		EntrySlot->SetZOrder(20);
	}

	UVerticalBox* EntryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryColumn"));
	OpenButton->AddChild(EntryColumn);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EntryIconSize"));
	IconSize->SetWidthOverride(EntryIconSize);
	IconSize->SetHeightOverride(EntryIconSize);
	if (UVerticalBoxSlot* IconSlot = EntryColumn->AddChildToVerticalBox(IconSize))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UImage* HeadIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("EntryIcon"));
	if (HeadIconTexture)
	{
		HeadIcon->SetBrushFromTexture(HeadIconTexture, true);
	}
	IconSize->AddChild(HeadIcon);

	UTextBlock* EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EntryLabel"));
	EntryLabel->SetText(FText::FromString(EntryLabelText));
	EntryLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	EntryLabel->SetJustification(ETextJustify::Center);
	EntryLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
	EntryLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	{
		FSlateFontInfo EntryFont = EntryLabel->GetFont();
		EntryFont.Size = 16;
		EntryLabel->SetFont(EntryFont);
	}
	if (UVerticalBoxSlot* LabelSlot = EntryColumn->AddChildToVerticalBox(EntryLabel))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	}

	// -------------------------------------------------------- proximity hint
	// "按[F]与[" + <character name, yellow> + "]进行交互"
	InteractionHintRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InteractionHintRow"));
	if (UCanvasPanelSlot* HintSlot = RootCanvas->AddChildToCanvas(InteractionHintRow))
	{
		// Centre horizontally, a little below the middle of the screen.
		HintSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		HintSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		HintSlot->SetPosition(FVector2D(0.0f, 200.0f));
		HintSlot->SetAutoSize(true);
		HintSlot->SetZOrder(25);
	}

	auto AddHintPart = [this](const TCHAR* PartText, const FLinearColor& PartColor, bool bIsName)
	{
		UTextBlock* Part = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Part->SetText(FText::FromString(PartText));
		Part->SetColorAndOpacity(FSlateColor(PartColor));
		Part->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Part->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		{
			FSlateFontInfo PartFont = Part->GetFont();
			PartFont.Size = 24;
			Part->SetFont(PartFont);
		}
		if (UHorizontalBoxSlot* PartSlot = InteractionHintRow->AddChildToHorizontalBox(Part))
		{
			PartSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (bIsName)
		{
			InteractionHintName = Part;
		}
		return Part;
	};

	// "按[F]与["
	AddHintPart(TEXT("\u6309[F]\u4E0E["), FLinearColor(1.0f, 1.0f, 1.0f, 0.95f), false);
	// The character name, highlighted in yellow.
	AddHintPart(TEXT("---"), FLinearColor(1.0f, 0.86f, 0.18f, 1.0f), true);
	// "]进行交互"
	AddHintPart(TEXT("]\u8FDB\u884C\u4EA4\u4E92"), FLinearColor(1.0f, 1.0f, 1.0f, 0.95f), false);

	InteractionHintRow->SetVisibility(ESlateVisibility::Collapsed);

	// ------------------------------------------------------------------ panel
	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerInfoPanel"));
	PanelBorder->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetSize(FVector2D(PanelWidth, PanelHeight));
		PanelSlot->SetZOrder(30);
	}
	PanelBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanel* PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PanelCanvas"));
	PanelBorder->AddChild(PanelCanvas);

	// Square close button in the panel's top-right corner.
	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CloseSize"));
	CloseSize->SetWidthOverride(CloseButtonSize);
	CloseSize->SetHeightOverride(CloseButtonSize);
	if (UCanvasPanelSlot* CloseSlot = PanelCanvas->AddChildToCanvas(CloseSize))
	{
		CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CloseSlot->SetPosition(FVector2D(-18.0f, 18.0f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->OnClicked.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleCloseButtonClicked);
	CloseSize->AddChild(CloseButton);

	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
	CloseLabel->SetText(FText::FromString(TEXT("X")));
	CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
	CloseLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo CloseFont = CloseLabel->GetFont();
		CloseFont.Size = 20;
		CloseLabel->SetFont(CloseFont);
	}
	CloseButton->AddChild(CloseLabel);

	// Text block, top-left inside the panel.
	UVerticalBox* InfoColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoColumn"));
	if (UCanvasPanelSlot* InfoSlot = PanelCanvas->AddChildToCanvas(InfoColumn))
	{
		InfoSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		InfoSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		InfoSlot->SetPosition(FVector2D(56.0f, 52.0f));
		InfoSlot->SetAutoSize(true);
	}

	auto AddInfoLine = [this, InfoColumn](const TCHAR* InitialText)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Line->SetText(FText::FromString(InitialText));
		Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f)));
		{
			FSlateFontInfo LineFont = Line->GetFont();
			LineFont.Size = 24;
			Line->SetFont(LineFont);
		}
		if (UVerticalBoxSlot* LineSlot = InfoColumn->AddChildToVerticalBox(Line))
		{
			LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		return Line;
	};

	NameValue = AddInfoLine(TEXT("\u59D3\u540D\uFF1A---"));
	ClassValue = AddInfoLine(TEXT("\u89D2\u8272\uFF1A---"));
	JoinedValue = AddInfoLine(TEXT("\u52A0\u5165\u65F6\u95F4\uFF1A---"));
	ContractStatusLine = AddInfoLine(TEXT(""));
	ContractStatusLine->SetVisibility(ESlateVisibility::Collapsed);

	// "添加好友" sits at the panel's bottom-left and only shows for other players.
	USizeBox* AddFriendSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AddFriendSize"));
	AddFriendSize->SetWidthOverride(360.0f);
	AddFriendSize->SetHeightOverride(88.0f);
	if (UCanvasPanelSlot* AddFriendSlot = PanelCanvas->AddChildToCanvas(AddFriendSize))
	{
		AddFriendSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		AddFriendSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		AddFriendSlot->SetPosition(FVector2D(56.0f, -56.0f));

		// Without auto size the canvas slot keeps a zero sized rect, which makes the
		// button invisible and impossible to click.
		AddFriendSlot->SetAutoSize(true);
	}

	AddFriendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddFriendButton"));
	AddFriendButton->OnClicked.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleAddFriendClicked);
	AddFriendSize->AddChild(AddFriendButton);

	AddFriendLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddFriendLabel"));
	AddFriendLabel->SetText(FText::FromString(TEXT("\u6DFB\u52A0\u597D\u53CB")));
	AddFriendLabel->SetJustification(ETextJustify::Center);
	AddFriendLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
	{
		FSlateFontInfo AddFriendFont = AddFriendLabel->GetFont();
		AddFriendFont.Size = 20;
		AddFriendLabel->SetFont(AddFriendFont);
	}
	AddFriendButton->AddChild(AddFriendLabel);

	// The size box stays visible; only the button itself is toggled (it is the
	// only thing that should appear or disappear).
	AddFriendButton->SetVisibility(ESlateVisibility::Collapsed);

	if (WardrobeButton)
	{
		WardrobeButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	// "签订契约" sits to the right of the add-friend button.
	USizeBox* ContractSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ContractSize"));
	ContractSize->SetWidthOverride(360.0f);
	ContractSize->SetHeightOverride(88.0f);
	if (UCanvasPanelSlot* ContractSlot = PanelCanvas->AddChildToCanvas(ContractSize))
	{
		ContractSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		ContractSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		ContractSlot->SetPosition(FVector2D(436.0f, -56.0f));
		ContractSlot->SetAutoSize(true);
	}

	ContractButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContractButton"));
	ContractButton->OnClicked.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleContractClicked);
	ContractSize->AddChild(ContractButton);

	ContractLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContractLabel"));
	ContractLabel->SetText(FText::FromString(TEXT("\u7B7E\u8BA2\u5951\u7EA6")));
	ContractLabel->SetJustification(ETextJustify::Center);
	ContractLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
	{
		FSlateFontInfo ContractFont = ContractLabel->GetFont();
		ContractFont.Size = 20;
		ContractLabel->SetFont(ContractFont);
	}
	ContractButton->AddChild(ContractLabel);
	ContractButton->SetVisibility(ESlateVisibility::Collapsed);

	// "衣物/拘束" sits to the right of the contract button; only for friends.
	USizeBox* WardrobeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WardrobeSize"));
	WardrobeSize->SetWidthOverride(300.0f);
	WardrobeSize->SetHeightOverride(88.0f);
	if (UCanvasPanelSlot* WardrobeSlot = PanelCanvas->AddChildToCanvas(WardrobeSize))
	{
		WardrobeSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		WardrobeSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		WardrobeSlot->SetPosition(FVector2D(808.0f, -56.0f));
		WardrobeSlot->SetAutoSize(true);
	}

	WardrobeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WardrobeButton"));
	WardrobeButton->OnClicked.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleWardrobeClicked);
	WardrobeSize->AddChild(WardrobeButton);

	UTextBlock* WardrobeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WardrobeLabel"));
	WardrobeLabel->SetText(FText::FromString(TEXT("\u8863\u7269/\u62D8\u675F")));
	WardrobeLabel->SetJustification(ETextJustify::Center);
	WardrobeLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f)));
	{
		FSlateFontInfo WardrobeFont = WardrobeLabel->GetFont();
		WardrobeFont.Size = 20;
		WardrobeLabel->SetFont(WardrobeFont);
	}
	WardrobeButton->AddChild(WardrobeLabel);
	WardrobeButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifePlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// A late profile answer has to repaint the panel (contract line + button state).
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnUserProfileUpdated.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleUserProfileUpdated);
			Account->OnContractsChanged.AddDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleContractsChanged);
		}
	}

	// Default to the local player so the panel is never empty.
	if (!DisplayedCharacter.IsValid())
	{
		ShowCharacterInfo(Cast<AMyBondageLifeCharacter>(GetOwningPlayerPawn()));
	}
	else
	{
		RefreshContents();
	}
}

void UMyBondageLifePlayerInfoWidget::ShowCharacterInfo(const AMyBondageLifeCharacter* Character)
{
	DisplayedCharacter = Character;
	RefreshContents();
}

void UMyBondageLifePlayerInfoWidget::RefreshContents()
{
	const AMyBondageLifeCharacter* Character = DisplayedCharacter.Get();

	const FString CharacterName = (Character && !Character->GetCharacterName().IsEmpty())
		? Character->GetCharacterName().ToString()
		: FString(TEXT("---"));

	// "姓名：<name>"
	if (NameValue)
	{
		NameValue->SetText(FText::FromString(FString::Printf(TEXT("\u59D3\u540D\uFF1A%s"), *CharacterName)));
	}

	// "角色：支配者"
	if (ClassValue)
	{
		const FString ClassLabel = UMyBondageLifeAccountSubsystem::MakeCharacterClassLabel(
			Character ? Character->GetCharacterClass() : 0);
		ClassValue->SetText(FText::FromString(FString::Printf(TEXT("\u89D2\u8272\uFF1A%s"), *ClassLabel)));
	}

	// "加入时间：2026年10月26日" from the replicated account creation stamp.
	if (JoinedValue)
	{
		FString JoinedText = TEXT("---");
		if (Character && Character->GetCreatedAtUnix() > 0)
		{
			const FDateTime CreatedAtUtc = FDateTime::FromUnixTimestamp(Character->GetCreatedAtUnix());

			// FDateTime has no UTC->local helper, so derive the offset from the
			// current local/UTC pair (rounded to whole minutes, keeping DST).
			const FTimespan RawOffset = FDateTime::Now() - FDateTime::UtcNow();
			const FTimespan LocalOffset = FTimespan::FromMinutes(FMath::RoundToDouble(RawOffset.GetTotalMinutes()));

			const FDateTime LocalTime = CreatedAtUtc + LocalOffset;
			JoinedText = FString::Printf(TEXT("%d\u5E74%d\u6708%d\u65E5"),
				LocalTime.GetYear(), LocalTime.GetMonth(), LocalTime.GetDay());
		}

		JoinedValue->SetText(FText::FromString(FString::Printf(TEXT("\u52A0\u5165\u65F6\u95F4\uFF1A%s"), *JoinedText)));
	}

	// Bottom-left "添加好友" button: only for other players, label flips for friends.
	RefreshContractSection();

	if (AddFriendButton && AddFriendLabel)
	{
		if (!IsDisplayingOtherPlayer())
		{
			AddFriendButton->SetVisibility(ESlateVisibility::Collapsed);
			if (WardrobeButton)
			{
				WardrobeButton->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			const bool bIsFriend = IsDisplayedCharacterFriend();
			AddFriendButton->SetVisibility(ESlateVisibility::Visible);
			AddFriendButton->SetIsEnabled(!bIsFriend);

			if (WardrobeButton)
			{
				WardrobeButton->SetVisibility((bIsFriend && IsDisplayingOtherPlayer()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			}
			AddFriendLabel->SetText(FText::FromString(bIsFriend
				? FString(TEXT("\u5DF2\u662F\u597D\u53CB"))     // 已是好友
				: FString(TEXT("\u6DFB\u52A0\u597D\u53CB"))));  // 添加好友
			AddFriendLabel->SetColorAndOpacity(FSlateColor(bIsFriend
				? FLinearColor(0.45f, 0.45f, 0.45f)
				: FLinearColor(0.02f, 0.02f, 0.02f)));
		}
	}
}

bool UMyBondageLifePlayerInfoWidget::IsDisplayingOtherPlayer() const
{
	const AMyBondageLifeCharacter* Character = DisplayedCharacter.Get();
	return Character && Character != GetOwningPlayerPawn();
}

bool UMyBondageLifePlayerInfoWidget::IsDisplayingCharacter(const AMyBondageLifeCharacter* Character) const
{
	return Character != nullptr && DisplayedCharacter.Get() == Character;
}

bool UMyBondageLifePlayerInfoWidget::IsDisplayedCharacterFriend() const
{
	const AMyBondageLifeCharacter* Character = DisplayedCharacter.Get();
	if (!Character)
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	const APlayerState* OwnerState = Character->GetPlayerState();
	if (!Account || !OwnerState)
	{
		return false;
	}

	return Account->IsFriend(OwnerState->GetPlayerName());
}

void UMyBondageLifePlayerInfoWidget::HandleAddFriendClicked()
{
	const AMyBondageLifeCharacter* Character = DisplayedCharacter.Get();

	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: add-friend clicked (character=%s, other=%d, friend=%d)"),
		*GetNameSafe(Character), IsDisplayingOtherPlayer() ? 1 : 0, IsDisplayedCharacterFriend() ? 1 : 0);

	// Only meaningful for other players: hide it whenever this panel shows ourselves.
	if (WardrobeButton)
	{
		WardrobeButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!Character || !IsDisplayingOtherPlayer() || IsDisplayedCharacterFriend())
	{
		return;
	}

	OnAddFriendRequested.Broadcast(Character);
}

void UMyBondageLifePlayerInfoWidget::SetInteractionHintTarget(const AMyBondageLifeCharacter* Character)
{
	if (!InteractionHintRow)
	{
		return;
	}

	if (!Character)
	{
		HintTarget = nullptr;
		InteractionHintRow->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// The replicated name can arrive after the target is first seen, so refresh the
	// text whenever either the target or its name changes.
	const FString TargetName = Character->GetCharacterName().IsEmpty()
		? FString(TEXT("---"))
		: Character->GetCharacterName().ToString();

	if (HintTarget.Get() != Character || HintName != TargetName)
	{
		HintTarget = Character;
		HintName = TargetName;

		if (InteractionHintName)
		{
			InteractionHintName->SetText(FText::FromString(TargetName));
		}
	}

	InteractionHintRow->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UMyBondageLifePlayerInfoWidget::IsInteractionHintVisible() const
{
	return InteractionHintRow && InteractionHintRow->GetVisibility() == ESlateVisibility::HitTestInvisible;
}

void UMyBondageLifePlayerInfoWidget::SetPanelOpen(bool bOpen)
{
	if (IsPanelOpen() == bOpen)
	{
		return;
	}

	if (PanelBorder)
	{
		PanelBorder->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bOpen)
	{
		RefreshContents();
	}

	OnPanelOpenChanged.Broadcast(bOpen);
}

void UMyBondageLifePlayerInfoWidget::TogglePanel()
{
	SetPanelOpen(!IsPanelOpen());
}

bool UMyBondageLifePlayerInfoWidget::IsPanelOpen() const
{
	return PanelBorder && PanelBorder->GetVisibility() == ESlateVisibility::Visible;
}

void UMyBondageLifePlayerInfoWidget::HandleOpenButtonClicked()
{
	// The bottom entry always inspects the local player.
	ShowCharacterInfo(Cast<AMyBondageLifeCharacter>(GetOwningPlayerPawn()));
	SetPanelOpen(true);
}

void UMyBondageLifePlayerInfoWidget::HandleCloseButtonClicked()
{
	SetPanelOpen(false);
}

void UMyBondageLifePlayerInfoWidget::HandleContractClicked()
{
	const AMyBondageLifeCharacter* Target = DisplayedCharacter.Get();
	if (!Target)
	{
		return;
	}

	OnContractRequested.Broadcast(Target, bContractActionIsFormal);
}

void UMyBondageLifePlayerInfoWidget::RefreshContractSection()
{
	const AMyBondageLifeCharacter* Target = DisplayedCharacter.Get();
	const bool bIsSelf = Target != nullptr && Target == GetOwningPlayerPawn();

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	// ------------------------------------------------------- "已成为…" line
	// The public profile says whether the shown character is somebody's slave, which
	// makes the line visible to every viewer (not just the two contract partners).
	FMyBondageLifeUserProfile Profile;
	bool bHasProfile = false;

	if (Account && Account->IsLoggedIn() && Target)
	{
		const APlayerState* TargetState = Target->GetPlayerState();
		const FString TargetAccount = TargetState ? TargetState->GetPlayerName() : FString();

		if (!bIsSelf && !TargetAccount.IsEmpty())
		{
			bHasProfile = Account->GetUserProfile(TargetAccount, Profile);

			// Always ask for a refresh (the subsystem throttles it to one call per
			// 10s): a contract dissolved elsewhere must not stay on screen.
			Account->RequestUserProfile(TargetAccount);
		}
	}

	FString StatusText;
	if (Account && Account->IsLoggedIn() && Target)
	{
		// Contract stamps are UTC (ISO-8601): compare against UtcNow, not local time.
		const FDateTime Now = FDateTime::UtcNow();

		if (bIsSelf && Account->HasMaster())
		{
			const FMyBondageLifeContractInfo Info = Account->GetContractAsSlave();
			const TCHAR* RoleWord = Info.Role == EMyBondageLifeContractRole::Slave ? TEXT("\u6B63\u5F0F") : TEXT("\u4E34\u65F6");
			const int32 Days = Info.Since.GetTicks() ? FMath::Max(0, static_cast<int32>(FMath::FloorToInt((Now - Info.Since).GetTotalHours() / 24.0))) : 0;
			StatusText = FString::Printf(
				TEXT("\u5DF2\u6210\u4E3A[%s]\u7684%s\u5974\u96B6\u957F\u8FBE%d\u5929"), *Info.PeerCharacterName, RoleWord, Days);
		}
		else if (!bIsSelf && bHasProfile && Profile.bHasMaster)
		{
			const TCHAR* RoleWord = Profile.bMasterIsFormal ? TEXT("\u6B63\u5F0F") : TEXT("\u4E34\u65F6");
			const int32 Days = Profile.ContractSince.GetTicks() ? FMath::Max(0, static_cast<int32>(FMath::FloorToInt((Now - Profile.ContractSince).GetTotalHours() / 24.0))) : 0;
			StatusText = FString::Printf(
				TEXT("\u5DF2\u6210\u4E3A[%s]\u7684%s\u5974\u96B6\u957F\u8FBE%d\u5929"), *Profile.MasterCharacterName, RoleWord, Days);
		}
	}

	if (ContractStatusLine)
	{
		if (StatusText.IsEmpty())
		{
			// The row is always shown: without a master it says so instead of hiding.
			StatusText = TEXT("\u672A\u62E5\u6709\u4E3B\u4EBA\u6216\u81EA\u5DF1\u662F\u652F\u914D\u8005");   // 未拥有主人或自己是支配者
		}

		ContractStatusLine->SetText(FText::FromString(StatusText));
		ContractStatusLine->SetVisibility(ESlateVisibility::Visible);
	}

	// ------------------------------------------------------------ the button
	if (!ContractButton || !ContractLabel)
	{
		return;
	}

	const bool bShowButton = !bIsSelf && Target && Account && Account->IsLoggedIn();
	if (!bShowButton)
	{
		ContractButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FString TargetAccount = Target->GetPlayerState() ? Target->GetPlayerState()->GetPlayerName() : FString();
	const EMyBondageLifeContractRole Role = Account->GetContractRoleWith(TargetAccount);
	const int32 MyClass = Account->GetCharacterClass();
	const int32 TargetClass = Target->GetCharacterClass();
	// Contract stamps are UTC (ISO-8601): compare against UtcNow, not local time.
	const FDateTime Now = FDateTime::UtcNow();

	// "签订契约"
	FString LabelText = TEXT("\u7B7E\u8BA2\u5951\u7EA6");
	bool bEnabled = false;
	bContractActionIsFormal = false;

	if (Role == EMyBondageLifeContractRole::TemporaryMaster)
	{
		FMyBondageLifeFriendEntry Entry;
		Account->GetFriendEntry(TargetAccount, Entry);

		if (Entry.ContractTrialEndsAt.GetTicks() != 0 && Now >= Entry.ContractTrialEndsAt)
		{
			// 72h trained: the master may invite the slave to the formal contract.
			LabelText = TEXT("\u6B63\u5F0F\u7B7E\u8BA2\u5951\u7EA6");
			bEnabled = true;
			bContractActionIsFormal = true;
		}
		else
		{
			const int32 Hours = Entry.ContractSince.GetTicks()
				? FMath::Clamp(static_cast<int32>(FMath::FloorToInt((Now - Entry.ContractSince).GetTotalHours())), 0, 72)
				: 0;
			LabelText = FString::Printf(TEXT("\u5DF2\u8BAD\u7EC3%d\u5C0F\u65F6"), Hours);
		}
	}
	else if (Role == EMyBondageLifeContractRole::Master)
	{
		LabelText = TEXT("\u5DF2\u7B7E\u8BA2\u5951\u7EA6");
	}
	else if (Role == EMyBondageLifeContractRole::TemporarySlave || Role == EMyBondageLifeContractRole::Slave)
	{
		// The shown character is my master: a mutual contract is not allowed.
		LabelText = TEXT("\u65E0\u6CD5\u53D1\u8D77\u5951\u7EA6");
	}
	else if ((bHasProfile && Profile.bHasMaster) || Account->IsAccountKnownOwned(TargetAccount))
	{
		// Somebody else owns the shown character.
		LabelText = TEXT("\u5DF2\u62E5\u6709\u4E3B\u4EBA");
	}
	else if (!Account->IsFriend(TargetAccount))
	{
		LabelText = TEXT("\u7B7E\u8BA2\u5951\u7EA6");
	}
	else if (MyClass > 2 || TargetClass < 2)
	{
		LabelText = TEXT("\u65E0\u6CD5\u53D1\u8D77\u5951\u7EA6");
	}
	else
	{
		LabelText = TEXT("\u7B7E\u8BA2\u5951\u7EA6");
		bEnabled = true;
	}

	ContractButton->SetVisibility(ESlateVisibility::Visible);
	ContractButton->SetIsEnabled(bEnabled);
	ContractLabel->SetText(FText::FromString(LabelText));
	ContractLabel->SetColorAndOpacity(FSlateColor(bEnabled
		? FLinearColor(0.02f, 0.02f, 0.02f)
		: FLinearColor(0.55f, 0.55f, 0.58f)));
}

void UMyBondageLifePlayerInfoWidget::HandleUserProfileUpdated(const FString& Account)
{
	const AMyBondageLifeCharacter* Target = DisplayedCharacter.Get();
	const APlayerState* TargetState = Target ? Target->GetPlayerState() : nullptr;
	const FString ShownAccount = TargetState ? TargetState->GetPlayerName() : FString();

	if (!ShownAccount.IsEmpty() && ShownAccount == Account)
	{
		RefreshContents();
	}
}

void UMyBondageLifePlayerInfoWidget::HandleContractsChanged()
{
	if (IsPanelOpen())
	{
		RefreshContents();
	}
}

void UMyBondageLifePlayerInfoWidget::HandleWardrobeClicked()
{
	UE_LOG(LogMyBondageLife, Warning, TEXT("Wardrobe button clicked (char=%s)"), *GetNameSafe(DisplayedCharacter.Get()));

	const AMyBondageLifeCharacter* Character = DisplayedCharacter.Get();
	if (!Character)
	{
		return;
	}

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	// Same rule as IsDisplayedCharacterFriend(): the player name is the account name.
	FString Target;
	if (const APlayerState* OwnerState = Character->GetPlayerState())
	{
		Target = OwnerState->GetPlayerName();
	}
	if (Target.IsEmpty())
	{
		Target = Account->FindAccountByCharacterName(Character->GetCharacterName());
	}

	if (Target.IsEmpty() || !Account->IsFriend(Target))
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Wardrobe button: cannot resolve account (friend=%d)."),
			Target.IsEmpty() ? 0 : 1);
		return;
	}

	if (AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		Controller->OpenFriendWardrobe(Target);
	}
}
namespace
{
	template <typename TWidget>
	TWidget* AdoptFind(UWidgetTree* Tree, const TCHAR* Name)
	{
		return Tree ? Cast<TWidget>(Tree->FindWidget(Name)) : nullptr;
	}

	UButton* AdoptFindButton(UWidget* Root)
	{
		if (!Root)
		{
			return nullptr;
		}
		if (UButton* AsButton = Cast<UButton>(Root))
		{
			return AsButton;
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (UButton* Found = AdoptFindButton(Panel->GetChildAt(Index)))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}

	UButton* AdoptFindNamedButton(UWidgetTree* Tree, const TCHAR* Name)
	{
		return AdoptFindButton(Tree ? Tree->FindWidget(Name) : nullptr);
	}
}

bool UMyBondageLifePlayerInfoWidget::TryAdoptBlueprintLayout()
{
	if (!WidgetTree || !AdoptFind<UBorder>(WidgetTree, TEXT("W_Panel")))
	{
		return false;
	}

	PanelBorder = AdoptFind<UBorder>(WidgetTree, TEXT("W_Panel"));
	NameValue = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_Name"));
	ClassValue = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_Class"));
	JoinedValue = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_Joined"));
	ContractStatusLine = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_ContractLine"));

	AddFriendButton = AdoptFindNamedButton(WidgetTree, TEXT("W_AddFriend"));
	AddFriendLabel = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_AddFriend_Label"));
	ContractButton = AdoptFindNamedButton(WidgetTree, TEXT("W_Contract"));
	ContractLabel = AdoptFind<UTextBlock>(WidgetTree, TEXT("W_Contract_Label"));
	WardrobeButton = AdoptFindNamedButton(WidgetTree, TEXT("W_WardrobeButton"));
	CloseButton = AdoptFindNamedButton(WidgetTree, TEXT("W_Close"));

	UE_LOG(LogMyBondageLife, Log, TEXT("PlayerInfo: using the designed layout."));
	return true;
}

void UMyBondageLifePlayerInfoWidget::WireAdoptedLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleCloseButtonClicked);
	}
	if (IsValid(AddFriendButton))
	{
		AddFriendButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleAddFriendClicked);
	}
	if (IsValid(ContractButton))
	{
		ContractButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleContractClicked);
	}
	if (IsValid(WardrobeButton))
	{
		WardrobeButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePlayerInfoWidget::HandleWardrobeClicked);
	}
}