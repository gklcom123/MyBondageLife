// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeMainHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
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
		TEXT("W_RoomInfoHost"),
		TEXT("W_StatusHost")
	};
}

TSharedRef<SWidget> UMyBondageLifeMainHudWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	TSharedRef<SWidget> BuiltWidget = Super::RebuildWidget();

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return BuiltWidget;
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
			const bool bUntouched = !bUseDesignedHostLayout;
			if (Layout && bUntouched)
			{
				CanvasSlot->SetAnchors(FAnchors(Layout->Anchor.X, Layout->Anchor.Y));
				CanvasSlot->SetAlignment(Layout->Alignment);
				CanvasSlot->SetPosition(Layout->Position);
				CanvasSlot->SetSize(Layout->Size);
			}
		}

		// The host borders are layout guides only: keep them transparent in game.
		if (UBorder* HostBorder = Cast<UBorder>(Host))
		{
			HostBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
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

	// Give the host an inner canvas so the content can be pinned to its bottom-left
	// corner: the designed box is the anchor, the widget keeps its natural size.
	UCanvasPanel* InnerCanvas = nullptr;
	if (UBorder* BorderHost = Cast<UBorder>(Host))
	{
		BorderHost->SetPadding(FMargin(0.0f));
		BorderHost->SetHorizontalAlignment(HAlign_Fill);
		BorderHost->SetVerticalAlignment(VAlign_Fill);

		InnerCanvas = Cast<UCanvasPanel>(BorderHost->GetContent());
		if (!InnerCanvas)
		{
			InnerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
			BorderHost->AddChild(InnerCanvas);
		}
	}

	if (InnerCanvas)
	{
		const bool bFill = HostName == TEXT("W_ChatHost");
		if (UCanvasPanelSlot* ChildSlot = InnerCanvas->AddChildToCanvas(Widget))
		{
			if (bFill)
			{
				ChildSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				ChildSlot->SetOffsets(FMargin(0.0f));
			}
			else
			{
				// Bottom-left of the host = the widget's own origin.
				ChildSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
				ChildSlot->SetAlignment(FVector2D(0.0f, 1.0f));
				ChildSlot->SetPosition(FVector2D::ZeroVector);
				ChildSlot->SetAutoSize(true);
			}
		}
	}
	else
	{
		Host->AddChild(Widget);
	}

	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UE_LOG(LogMyBondageLife, Log, TEXT("MainHud: hosted %s into %s"), *Widget->GetName(), *HostName.ToString());
	return true;
}
