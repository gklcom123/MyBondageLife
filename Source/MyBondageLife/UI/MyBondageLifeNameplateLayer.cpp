// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeNameplateLayer.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyBondageLifeCharacter.h"
#include "MyBondageLife.h"

UMyBondageLifeNameplateLayer::UMyBondageLifeNameplateLayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Overlay only: never intercept clicks or focus.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UMyBondageLifeNameplateLayer::RebuildWidget()
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

void UMyBondageLifeNameplateLayer::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;
}

UTextBlock* UMyBondageLifeNameplateLayer::FindOrCreateLabel(AMyBondageLifeCharacter* Character)
{
	if (const TObjectPtr<UTextBlock>* Existing = Labels.Find(Character))
	{
		return *Existing;
	}

	if (!RootCanvas)
	{
		return nullptr;
	}

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.0f)));
	Label->SetJustification(ETextJustify::Center);
	// Shadow instead of a background plate, keeps the text readable anywhere.
	Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	{
		FSlateFontInfo NameFont = Label->GetFont();
		NameFont.Size = 18;
		Label->SetFont(NameFont);
	}
	Label->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* LabelSlot = RootCanvas->AddChildToCanvas(Label))
	{
		// Anchored at the canvas origin with centre alignment: SetPosition(screen)
		// therefore puts the middle of the text exactly on the projected pixel.
		LabelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		LabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		LabelSlot->SetAutoSize(true);
		LabelSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	Labels.Add(Character, Label);
	return Label;
}

void UMyBondageLifeNameplateLayer::UpdateNameplates()
{
	APlayerController* PlayerController = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!PlayerController || !World || !RootCanvas)
	{
		return;
	}

	TSet<AMyBondageLifeCharacter*> AliveCharacters;

	// The canvas lays out in its own local space (viewport size divided by the UI
	// DPI scale), while projections come back in viewport pixels.
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	const FVector2D CanvasLocalSize = RootCanvas->GetCachedGeometry().GetLocalSize();
	FVector2D ViewportToCanvasScale(1.0f, 1.0f);
	if (ViewportSizeX > 0 && ViewportSizeY > 0 && CanvasLocalSize.X > 0.0f && CanvasLocalSize.Y > 0.0f)
	{
		ViewportToCanvasScale = FVector2D(
			CanvasLocalSize.X / static_cast<float>(ViewportSizeX),
			CanvasLocalSize.Y / static_cast<float>(ViewportSizeY));
	}

	for (TActorIterator<AMyBondageLifeCharacter> It(World); It; ++It)
	{
		AMyBondageLifeCharacter* Character = *It;
		if (!IsValid(Character))
		{
			continue;
		}

		UTextBlock* Label = FindOrCreateLabel(Character);
		if (!Label)
		{
			continue;
		}

		AliveCharacters.Add(Character);

		// Text: only rewritten when it actually changed.
		const FString Text = Character->GetNameplateText();
		FString& CachedText = LabelTexts.FindOrAdd(Character);
		if (CachedText != Text)
		{
			CachedText = Text;
			Label->SetText(FText::FromString(Text));
		}

		// Position: straight from the world-to-screen projection of the head, so it
		// stays glued to the character no matter how the camera moves.
		//
		// ProjectWorldToScreen returns viewport pixels, but the canvas lays out in
		// its own local space, which is scaled by the UI DPI setting. Convert.
		const FVector NameplateWorldLocation = Character->GetNameplateLocation();
		FVector2D ScreenPosition;
		if (UGameplayStatics::ProjectWorldToScreen(PlayerController, NameplateWorldLocation, ScreenPosition, false))
		{
			if (UCanvasPanelSlot* LabelSlot = Cast<UCanvasPanelSlot>(Label->Slot))
			{
				LabelSlot->SetPosition(ScreenPosition * ViewportToCanvasScale);
			}

			if (Label->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				Label->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else if (Label->GetVisibility() != ESlateVisibility::Collapsed)
		{
			// Behind the camera.
			Label->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Remove plates belonging to characters that are gone.
	TArray<TObjectPtr<AMyBondageLifeCharacter>> StaleCharacters;
	for (const TPair<TObjectPtr<AMyBondageLifeCharacter>, TObjectPtr<UTextBlock>>& Pair : Labels)
	{
		if (!AliveCharacters.Contains(Pair.Key))
		{
			StaleCharacters.Add(Pair.Key);
		}
	}

	for (const TObjectPtr<AMyBondageLifeCharacter>& StaleCharacter : StaleCharacters)
	{
		if (const TObjectPtr<UTextBlock>* Label = Labels.Find(StaleCharacter))
		{
			if (*Label)
			{
				(*Label)->RemoveFromParent();
			}
		}

		Labels.Remove(StaleCharacter);
		LabelTexts.Remove(StaleCharacter);
	}
}
