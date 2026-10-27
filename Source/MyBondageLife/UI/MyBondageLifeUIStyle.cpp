// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeUIStyle.h"

#include "Components/EditableTextBox.h"

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
}
