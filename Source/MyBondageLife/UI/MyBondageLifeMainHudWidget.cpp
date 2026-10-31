// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeMainHudWidget.h"

#include "Blueprint/WidgetTree.h"
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

	for (const FName& HostName : MyBondageLifeHudHosts::Names)
	{
		if (UPanelWidget* Host = Cast<UPanelWidget>(WidgetTree->FindWidget(HostName)))
		{
			Hosts.Add(HostName, Host);
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
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return true;
}
