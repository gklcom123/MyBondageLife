// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeUIStyle.h"

#include "Components/EditableTextBox.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "MyBondageLife.h"

namespace MyBondageLifeUI
{
	void ApplyReadableTextInputStyle(UEditableTextBox* TextBox)
	{
		if (!TextBox)
		{
			return;
		}

		FEditableTextBoxStyle Style = TextBox->GetWidgetStyle();

		// The field is dark so light text reads well on the dark menus. The stock
		// rounded border image is reused; only its tint changes.
		Style.BackgroundColor = FLinearColor(0.04f, 0.05f, 0.07f, 0.90f);
		Style.BackgroundImageNormal.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		Style.BackgroundImageHovered.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		Style.BackgroundImageFocused.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		Style.BackgroundImageReadOnly.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

		// Placeholder text (drawn from ForegroundColor) is a readable light grey,
		// typed text turns near-white and fully white while the field has focus.
		Style.ForegroundColor = FSlateColor(FLinearColor(0.82f, 0.85f, 0.92f, 1.0f));
		Style.FocusedForegroundColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		Style.ReadOnlyForegroundColor = FSlateColor(FLinearColor(0.74f, 0.77f, 0.84f, 1.0f));

		TextBox->SetWidgetStyle(Style);
	}

	UFont* LoadUIFont()
	{
		static TWeakObjectPtr<UFont> CachedFont;
		if (CachedFont.IsValid())
		{
			return CachedFont.Get();
		}

		UFont* Font = LoadObject<UFont>(nullptr,
			TEXT("/Game/Font/AlimamaFangYuanTiVF-Thin-2_Font.AlimamaFangYuanTiVF-Thin-2_Font"));

		CachedFont = Font;
		return Font;
	}

	FSlateFontInfo Font(int32 Size)
	{
		if (UFont* UIFont = LoadUIFont())
		{
			return FSlateFontInfo(UIFont, Size);
		}

		FSlateFontInfo Info;
		Info.Size = Size;
		return Info;
	}

	void ApplyFonts(UWidget* Root)
	{
		if (!Root)
		{
			return;
		}

		UFont* UIFont = LoadUIFont();
		if (!UIFont)
		{
			return;
		}

		TArray<UWidget*> Stack;

		// UUserWidget 本身不是 Panel：必须从它的 WidgetTree 根控件开始遍历。
		if (const UUserWidget* UserWidget = Cast<UUserWidget>(Root))
		{
			if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
			{
				Stack.Add(UserWidget->WidgetTree->RootWidget);
			}
		}
		else
		{
			Stack.Add(Root);
		}

		while (Stack.Num() > 0)
		{
			UWidget* Current = Stack.Pop();
			if (!Current)
			{
				continue;
			}

			if (UTextBlock* Label = Cast<UTextBlock>(Current))
			{
				FSlateFontInfo Info = Label->GetFont();
				Info.FontObject = UIFont;
				Label->SetFont(Info);
			}

			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					Stack.Add(Panel->GetChildAt(Index));
				}
			}
		}
	}
}
