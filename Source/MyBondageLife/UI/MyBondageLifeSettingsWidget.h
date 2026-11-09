// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeSettingsWidget.generated.h"

class UButton;
class UComboBoxString;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

/**
 *  游戏设置面板（代码生成 UI）：画面 / 声音 / 语言 三个页签。
 *
 *  画面：画质等级、分辨率、窗口模式、垂直同步、抗锯齿质量（直接作用于 GameUserSettings）。
 *  声音：总音量 / 背景 / 音效 / UI 音量滑条（当前仅保存数值，等音效总线接好后再应用）。
 *  语言：中文简体 / 中文繁体 / 英语 / 日语 / 韩语（写入 Internationalization 并保存）。
 */

/** 动态委托代理：让代码生成的组合框/滑条能回调 lambda。 */
UCLASS()
class UMyBondageLifeSettingsActionProxy : public UObject
{
	GENERATED_BODY()

public:

	TFunction<void(int32)> OnIndexChanged;
	TArray<FString> Options;

	TFunction<void(float)> OnValueChanged;

	UFUNCTION()
	void HandleComboChanged(FString SelectedItem, ESelectInfo::Type SelectInfo);

	UFUNCTION()
	void HandleSliderChanged(float NewValue);
};
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	void OpenPanel();
	void ClosePanel();
	void TogglePanel();
	bool IsPanelOpen() const;

protected:

	void BuildLayout();

	/** 画面页 */
	void AddQualityRow(UVerticalBox* Parent, const TCHAR* Label, TFunction<int32()> Getter, TFunction<void(int32)> Setter);
	void AddComboRow(UVerticalBox* Parent, const TCHAR* Label, const TArray<FString>& Options, int32 CurrentIndex, TFunction<void(int32)> OnChanged);
	void ApplyQualityLevel(int32 Level);
	void ApplyResolution(const FString& ResolutionText);
	void ApplyWindowMode(int32 Mode);
	void ApplyVSync(bool bEnabled);
	void ApplyAntiAliasing(int32 Level);

	/** 页签切换 */
	void SwitchTab(int32 TabIndex);

	/** 配置读写（DefaultGame.ini [/Script/MyBondageLife.Settings]）。 */
	static float LoadFloat(const TCHAR* Key, float DefaultValue);
	static void SaveFloat(const TCHAR* Key, float Value);
	static FString LoadString(const TCHAR* Key, const FString& DefaultValue);
	static void SaveString(const TCHAR* Key, const FString& Value);

	UFUNCTION() void HandleCloseClicked();
	UFUNCTION() void HandleTab0Clicked();
	UFUNCTION() void HandleTab1Clicked();
	UFUNCTION() void HandleTab2Clicked();

	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> TabSwitcher;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> PicturePage;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SoundPage;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> LanguagePage;

	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TabButtons;

	bool bLayoutBuilt = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeSettingsActionProxy>> ActionProxies;

	/** 语言代码（用于 Internationalization）。 */
	static const TCHAR* LanguageCode(int32 Index);
};
