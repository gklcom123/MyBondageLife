// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifePasswordDialog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/MyBondageLifeUIStyle.h"

TSharedRef<SWidget> UMyBondageLifePasswordDialog::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PasswordRootSize"));
		WidgetTree->RootWidget = RootSize;
		RootSize->SetWidthOverride(420.0f);

		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PasswordBackground"));
		Background->SetBrushColor(FLinearColor(0.06f, 0.06f, 0.08f, 0.96f));
		Background->SetPadding(FMargin(18.0f));
		RootSize->AddChild(Background);

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PasswordBox"));
		Background->SetContent(Box);

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PasswordTitle"));
		Title->SetText(FText::FromString(TEXT("请输入房间密码")));
		Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Title->SetFont(MyBondageLifeUI::Font(22));
		if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		PasswordInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("W_PasswordInput"));
		PasswordInput->SetHintText(FText::FromString(TEXT("密码（数字）")));
		if (UVerticalBoxSlot* InputSlot = Box->AddChildToVerticalBox(PasswordInput))
		{
			InputSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PasswordButtons"));
		if (UVerticalBoxSlot* ButtonsSlot = Box->AddChildToVerticalBox(Buttons))
		{
			ButtonsSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UButton* OkButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_PasswordOk"));
		OkButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePasswordDialog::HandleOkClicked);
		UTextBlock* OkLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OkLabel->SetText(FText::FromString(TEXT("确定")));
		OkLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f)));
		OkLabel->SetFont(MyBondageLifeUI::Font(20));
		OkButton->AddChild(OkLabel);
		if (UHorizontalBoxSlot* OkSlot = Buttons->AddChildToHorizontalBox(OkButton))
		{
			OkSlot->SetPadding(FMargin(8.0f));
			OkSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_PasswordCancel"));
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePasswordDialog::HandleCancelClicked);
		UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CancelLabel->SetText(FText::FromString(TEXT("取消")));
		CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f)));
		CancelLabel->SetFont(MyBondageLifeUI::Font(20));
		CancelButton->AddChild(CancelLabel);
		if (UHorizontalBoxSlot* CancelSlot = Buttons->AddChildToHorizontalBox(CancelButton))
		{
			CancelSlot->SetPadding(FMargin(8.0f));
			CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifePasswordDialog::HandleOkClicked()
{
	const FString Password = PasswordInput ? PasswordInput->GetText().ToString().TrimStartAndEnd() : FString();
	OnConfirmed.Broadcast(Password);
	RemoveFromParent();
}

void UMyBondageLifePasswordDialog::HandleCancelClicked()
{
	OnCancelled.Broadcast();
	RemoveFromParent();
}
