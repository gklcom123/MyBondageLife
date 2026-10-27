// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEditableTextBox;

/**
 *  Shared look for text input fields.
 *
 *  The stock UMG style paints a light grey field with dim grey text, which reads
 *  poorly on the dark menus. This darkens the field and brightens the text:
 *  the placeholder (drawn from ForegroundColor) is clearly legible, and the text
 *  gets brighter as soon as the player focuses the field and types.
 */
namespace MyBondageLifeUI
{
	void ApplyReadableTextInputStyle(UEditableTextBox* TextBox);
}
