// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifePlayerInfoWidget.h"

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

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifePlayerInfoWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;
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
}

void UMyBondageLifePlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	if (AddFriendButton && AddFriendLabel)
	{
		if (!IsDisplayingOtherPlayer())
		{
			AddFriendButton->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			const bool bIsFriend = IsDisplayedCharacterFriend();
			AddFriendButton->SetVisibility(ESlateVisibility::Visible);
			AddFriendButton->SetIsEnabled(!bIsFriend);
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
