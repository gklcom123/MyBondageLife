// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Font.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"

class UEditableTextBox;
class UWidget;

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

	/** 全局 UI 字体：AlimamaFangYuanTiVF-Thin-2_Font */
	UFont* LoadUIFont();

	/** 生成带统一字体的 FSlateFontInfo（找不到字体时返回空字体）。 */
	FSlateFontInfo Font(int32 Size);

	/** 递归把控件树里所有 TextBlock 换成统一字体。 */
	void ApplyFonts(UWidget* Root);
}
