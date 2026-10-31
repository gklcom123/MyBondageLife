// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeMainHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "MyBondageLife.h"

namespace MyBondageLifeHudHosts
{
	static const FName Names[] =
	{
		TEXT("W_ChatHost"),
		TEXT("W_MessageHost"),
		TEXT("W_InfoEntryHost"),
		TEXT("W_FriendsEntryHost"),
		TEXT("W_WardrobeEntryHost"),
		TEXT("W_RoomInfoHost")
	};
}

TSharedRef<SWidget> UMyBondageLifeMainHudWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeMainHudWidget::BuildLayout()
{
	Hosts.Reset();

	// Sensible default layout, so the HUD is usable even before the design is arranged.
	// (The designer can still move things afterwards; code only applies it when the host
	// is untouched, i.e. has no size yet.)
	struct FHostLayout
	{
		FVector2D Anchor;
		FVector2D Alignment;
		FVector2D Position;
		FVector2D Size;
	};

	static const TMap<FName, FHostLayout> DefaultLayout =
	{
		{ TEXT("W_ChatHost"),          { FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(20.0f, -270.0f),  FVector2D(560.0f, 240.0f) } },
		{ TEXT("W_MessageHost"),       { FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(20.0f, -360.0f),  FVector2D(220.0f, 90.0f)  } },
		{ TEXT("W_InfoEntryHost"),     { FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(-260.0f, -70.0f), FVector2D(150.0f, 70.0f)  } },
		{ TEXT("W_FriendsEntryHost"),  { FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(-90.0f, -70.0f),  FVector2D(150.0f, 70.0f)  } },
		{ TEXT("W_WardrobeEntryHost"), { FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(80.0f, -70.0f),   FVector2D(150.0f, 70.0f)  } },
		{ TEXT("W_RoomInfoHost"),      { FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(-240.0f, 18.0f),  FVector2D(220.0f, 50.0f)  } },
	};

	for (const FName& HostName : MyBondageLifeHudHosts::Names)
	{
		UPanelWidget* Host = Cast<UPanelWidget>(WidgetTree->FindWidget(HostName));
		if (!Host)
		{
			continue;
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Host->Slot))
		{
			const FHostLayout* Layout = DefaultLayout.Find(HostName);
			const bool bUntouched = CanvasSlot->GetAutoSize()
				|| CanvasSlot->GetSize().X < 2.0f || CanvasSlot->GetSize().Y < 2.0f;
			if (Layout && bUntouched)
			{
				CanvasSlot->SetAnchors(FAnchors(Layout->Anchor.X, Layout->Anchor.Y));
				CanvasSlot->SetAlignment(Layout->Alignment);
				CanvasSlot->SetPosition(Layout->Position);
				CanvasSlot->SetSize(Layout->Size);
			}
		}

		Hosts.Add(HostName, Host);
	}

	// Placeholder art used while designing must not show in game.
	{
		TArray<UWidget*> Stack;
		if (WidgetTree->RootWidget)
		{
			Stack.Add(WidgetTree->RootWidget);
		}
		while (Stack.Num() > 0)
		{
			UWidget* Current = Stack.Pop();
			if (!Current)
			{
				continue;
			}
			if (Current->GetName().StartsWith(TEXT("W_Preview")))
			{
				Current->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					if (UWidget* Child = Panel->GetChildAt(Index))
					{
						Stack.Add(Child);
					}
				}
			}
		}
	}
	UE_LOG(LogMyBondageLife, Log, TEXT("MainHud: adopted %d/%d hosts."),
		Hosts.Num(), UE_ARRAY_COUNT(MyBondageLifeHudHosts::Names));
}

UPanelWidget* UMyBondageLifeMainHudWidget::FindHost(FName HostName) const
{
	const TObjectPtr<UPanelWidget>* Found = Hosts.Find(HostName);
	return Found ? Found->Get() : nullptr;
}

bool UMyBondageLifeMainHudWidget::HasHost(FName HostName) const
{
	return FindHost(HostName) != nullptr;
}

bool UMyBondageLifeMainHudWidget::HostWidget(FName HostName, UUserWidget* Widget)
{
	if (!Widget || Widget->GetParent())
	{
		return false;
	}

	UPanelWidget* Host = FindHost(HostName);
	if (!Host)
	{
		return false;
	}

	Host->AddChild(Widget);
	UE_LOG(LogMyBondageLife, Log, TEXT("MainHud: hosted %s into %s"), *Widget->GetName(), *HostName.ToString());
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return true;
}
