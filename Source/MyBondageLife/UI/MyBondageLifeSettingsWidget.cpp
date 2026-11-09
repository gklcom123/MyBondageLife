// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameUserSettings.h"
#include "Internationalization/Internationalization.h"
#include "Misc/ConfigCacheIni.h"
#include "MyBondageLife.h"
#include "Styling/CoreStyle.h"

namespace
{
	const TCHAR* SettingsSection = TEXT("/Script/MyBondageLife.Settings");
	const TCHAR* SettingsFile = TEXT("Game");

	FSlateFontInfo MakeFont(int32 Size, bool bBold = false)
	{
		// 必须带字体对象：只设置 Size 会得到空字体，中文全部显示为方块。
		return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
	}

	UTextBlock* MakeLabel(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetFont(MakeFont(Size));
		return Label;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FString& Caption, const FLinearColor& Tint)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(Tint);
		Button->SetClickMethod(EButtonClickMethod::MouseDown);

		UTextBlock* Label = MakeLabel(Tree, Caption, 30, FLinearColor(0.05f, 0.05f, 0.05f));
		Label->SetJustification(ETextJustify::Center);
		Button->AddChild(Label);
		return Button;
	}

	const TArray<FString>& QualityNames()
	{
		static TArray<FString> Names = { TEXT("非常低"), TEXT("低"), TEXT("中"), TEXT("高"), TEXT("非常高") };
		return Names;
	}

	const TArray<FString>& ResolutionNames()
	{
		static TArray<FString> Names = {
			TEXT("1280x720"), TEXT("1366x768"), TEXT("1600x900"),
			TEXT("1920x1080"), TEXT("2560x1440"), TEXT("3840x2160") };
		return Names;
	}

	const TArray<FString>& WindowModeNames()
	{
		static TArray<FString> Names = { TEXT("全屏"), TEXT("全屏窗口化"), TEXT("窗口") };
		return Names;
	}

	const TArray<FString>& LanguageNames()
	{
		static TArray<FString> Names = { TEXT("中文简体"), TEXT("中文繁体"), TEXT("英语"), TEXT("日语"), TEXT("韩语") };
		return Names;
	}
}

TSharedRef<SWidget> UMyBondageLifeSettingsWidget::RebuildWidget()
{
	// 纯代码 UI：必须在 Super 之前构建 WidgetTree，否则 Slate 控件已经生成，后加的树不会显示。
	if (!bLayoutBuilt)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
		}

		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeSettingsWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrushColor(FLinearColor(0.93f, 0.90f, 0.90f, 1.0f));
	Panel->SetPadding(FMargin(48.0f, 32.0f));
	Backdrop->AddChild(Panel);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelSize->SetWidthOverride(1200.0f);
	Panel->AddChild(PanelSize);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PanelSize->AddChild(Column);

	// 标题 + 关闭
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(Header);

	UTextBlock* Title = MakeLabel(WidgetTree, TEXT("游戏设置"), 44, FLinearColor(0.08f, 0.08f, 0.09f));
	Title->SetFont(MakeFont(44, true));
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}

	UButton* CloseButton = MakeButton(WidgetTree, TEXT("关闭"), FLinearColor(0.78f, 0.78f, 0.80f, 1.0f));
	if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(8.0f, 0.0f));
	}
	CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeSettingsWidget::HandleCloseClicked);

	// 页签
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* TabsSlot = Column->AddChildToVerticalBox(Tabs))
	{
		TabsSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 10.0f));
	}

	const TCHAR* TabCaptions[] = { TEXT("画面"), TEXT("声音"), TEXT("语言") };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* Tab = MakeButton(WidgetTree, TabCaptions[Index], FLinearColor(0.85f, 0.82f, 0.86f, 1.0f));
		if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Tab))
		{
			TabSlot->SetPadding(FMargin(6.0f, 0.0f));
		}
		TabButtons.Add(Tab);
	}

	TabButtons[0]->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeSettingsWidget::HandleTab0Clicked);
	TabButtons[1]->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeSettingsWidget::HandleTab1Clicked);
	TabButtons[2]->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeSettingsWidget::HandleTab2Clicked);

	// 内容
	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	if (UVerticalBoxSlot* SwitcherSlot = Column->AddChildToVerticalBox(TabSwitcher))
	{
		SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	auto MakePage = [this](const TCHAR* Name) -> UVerticalBox*
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), Name);
		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Scroll->AddChild(Page);
		TabSwitcher->AddChild(Scroll);
		return Page;
	};

	PicturePage = MakePage(TEXT("PicturePage"));
	SoundPage = MakePage(TEXT("SoundPage"));
	LanguagePage = MakePage(TEXT("LanguagePage"));

	// ---------------------------------------------------------------- 画面
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

	int32 OverallLevel = Settings ? Settings->GetOverallScalabilityLevel() : 2;
	if (OverallLevel < 0 && Settings)
	{
		// 自定义画质时 VS 返回 -1，用抗锯齿档位兜底，避免误显示为「非常低」。
		OverallLevel = Settings->GetAntiAliasingQuality();
	}
	OverallLevel = FMath::Clamp(OverallLevel, 0, 4);
	AddComboRow(PicturePage, TEXT("画质等级"), QualityNames(), FMath::Clamp(OverallLevel, 0, 4),
		[this](int32 Index) { ApplyQualityLevel(Index); });

	{
		FIntPoint CurrentResolution(1920, 1080);
		if (Settings)
		{
			CurrentResolution = Settings->GetScreenResolution();
		}

		const FString CurrentText = FString::Printf(TEXT("%dx%d"), CurrentResolution.X, CurrentResolution.Y);

		// 用"当前实际分辨率 + 常见列表"生成选项：当前值不在列表时补进去并选中它。
		TArray<FString> ResolutionOptions = ResolutionNames();
		int32 Selected = ResolutionOptions.IndexOfByKey(CurrentText);
		if (Selected == INDEX_NONE)
		{
			Selected = ResolutionOptions.Add(CurrentText);
			ResolutionOptions.Sort([](const FString& A, const FString& B)
			{
				return FCString::Atoi(*A) < FCString::Atoi(*B);
			});
			Selected = ResolutionOptions.IndexOfByKey(CurrentText);
		}

		AddComboRow(PicturePage, TEXT("分辨率"), ResolutionOptions, Selected,
			[this, ResolutionOptions](int32 Index)
			{
				if (ResolutionOptions.IsValidIndex(Index))
				{
					ApplyResolution(ResolutionOptions[Index]);
				}
			});
	}

	{
		int32 ModeIndex = 2;
		if (Settings)
		{
			switch (Settings->GetFullscreenMode())
			{
			case EWindowMode::Fullscreen: ModeIndex = 0; break;
			case EWindowMode::WindowedFullscreen: ModeIndex = 1; break;
			default: ModeIndex = 2; break;
			}
		}

		AddComboRow(PicturePage, TEXT("窗口模式"), WindowModeNames(), ModeIndex,
			[this](int32 Index) { ApplyWindowMode(Index); });
	}

	{
		const bool bVSync = Settings ? Settings->IsVSyncEnabled() : true;
		AddComboRow(PicturePage, TEXT("垂直同步"), { TEXT("开"), TEXT("关") }, bVSync ? 0 : 1,
			[this](int32 Index) { ApplyVSync(Index == 0); });
	}

	{
		const int32 AALevel = Settings ? Settings->GetAntiAliasingQuality() : 2;
		AddComboRow(PicturePage, TEXT("抗锯齿质量"), QualityNames(), FMath::Clamp(AALevel, 0, 4),
			[this](int32 Index) { ApplyAntiAliasing(Index); });
	}

	// ---------------------------------------------------------------- 声音
	auto AddSliderRow = [this](UVerticalBox* Parent, const TCHAR* LabelText, const TCHAR* Key, float DefaultValue)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 10.0f));
		}

		UTextBlock* Label = MakeLabel(WidgetTree, LabelText, 32, FLinearColor(0.1f, 0.1f, 0.11f));
		Label->SetMinDesiredWidth(260.0f);
		Row->AddChildToHorizontalBox(Label);

		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetValue(LoadFloat(Key, DefaultValue));
		if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider))
		{
			SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SliderSlot->SetVerticalAlignment(VAlign_Center);
			SliderSlot->SetPadding(FMargin(10.0f, 0.0f));
		}

		UTextBlock* ValueLabel = MakeLabel(WidgetTree,
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(LoadFloat(Key, DefaultValue) * 100.0f)),
			28, FLinearColor(0.25f, 0.25f, 0.27f));
		ValueLabel->SetMinDesiredWidth(110.0f);
		Row->AddChildToHorizontalBox(ValueLabel);

		UMyBondageLifeSettingsActionProxy* SliderProxy = NewObject<UMyBondageLifeSettingsActionProxy>(this);
		SliderProxy->OnValueChanged = [ValueLabel, Key](float NewValue)
		{
			ValueLabel->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(NewValue * 100.0f))));
			SaveFloat(Key, NewValue);
		};
		ActionProxies.Add(SliderProxy);
		Slider->OnValueChanged.AddDynamic(SliderProxy, &UMyBondageLifeSettingsActionProxy::HandleSliderChanged);
	};

	AddSliderRow(SoundPage, TEXT("总音量"), TEXT("MasterVolume"), 1.0f);
	AddSliderRow(SoundPage, TEXT("背景音量"), TEXT("MusicVolume"), 0.8f);
	AddSliderRow(SoundPage, TEXT("音效音量"), TEXT("SFXVolume"), 1.0f);
	AddSliderRow(SoundPage, TEXT("UI音量"), TEXT("UIVolume"), 1.0f);

	// ---------------------------------------------------------------- 语言
	{
		const FString SavedLanguage = LoadString(TEXT("Language"), TEXT("zh-Hans"));
		int32 LanguageIndex = 0;
		for (int32 Index = 0; Index < LanguageNames().Num(); ++Index)
		{
			if (SavedLanguage.Equals(LanguageCode(Index), ESearchCase::IgnoreCase))
			{
				LanguageIndex = Index;
				break;
			}
		}

		AddComboRow(LanguagePage, TEXT("语言"), LanguageNames(), LanguageIndex,
			[](int32 Index)
			{
				const TCHAR* Code = LanguageCode(Index);
				SaveString(TEXT("Language"), FString(Code));
				FInternationalization::Get().SetCurrentLanguage(Code);
			});
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeSettingsWidget::AddQualityRow(UVerticalBox* Parent, const TCHAR* Label, TFunction<int32()> Getter, TFunction<void(int32)> Setter)
{
	AddComboRow(Parent, Label, QualityNames(), FMath::Clamp(Getter(), 0, 4), MoveTemp(Setter));
}

void UMyBondageLifeSettingsWidget::AddComboRow(UVerticalBox* Parent, const TCHAR* Label, const TArray<FString>& Options,
	int32 CurrentIndex, TFunction<void(int32)> OnChanged)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Parent->AddChildToVerticalBox(Row);

	UTextBlock* LabelText = MakeLabel(WidgetTree, Label, 32, FLinearColor(0.1f, 0.1f, 0.11f));
	LabelText->SetMinDesiredWidth(300.0f);
	Row->AddChildToHorizontalBox(LabelText);

	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	for (const FString& Option : Options)
	{
		Combo->AddOption(Option);
	}
	Combo->SetSelectedIndex(FMath::Clamp(CurrentIndex, 0, Options.Num() - 1));

	if (UHorizontalBoxSlot* ComboSlot = Row->AddChildToHorizontalBox(Combo))
	{
		ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ComboSlot->SetPadding(FMargin(10.0f, 0.0f));
	}

	UMyBondageLifeSettingsActionProxy* ComboProxy = NewObject<UMyBondageLifeSettingsActionProxy>(this);
	ComboProxy->Options = Options;
	ComboProxy->OnIndexChanged = OnChanged;
	ActionProxies.Add(ComboProxy);
	Combo->OnSelectionChanged.AddDynamic(ComboProxy, &UMyBondageLifeSettingsActionProxy::HandleComboChanged);
}

void UMyBondageLifeSettingsWidget::ApplyQualityLevel(int32 Level)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetOverallScalabilityLevel(FMath::Clamp(Level, 0, 4));
		Settings->SetAntiAliasingQuality(FMath::Clamp(Level, 0, 4));
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

void UMyBondageLifeSettingsWidget::ApplyResolution(const FString& ResolutionText)
{
	FString Left;
	FString Right;
	if (!ResolutionText.Split(TEXT("x"), &Left, &Right))
	{
		return;
	}

	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetScreenResolution(FIntPoint(FCString::Atoi(*Left), FCString::Atoi(*Right)));
		Settings->ApplyResolutionSettings(false);
		Settings->SaveSettings();
	}
}

void UMyBondageLifeSettingsWidget::ApplyWindowMode(int32 Mode)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type NewMode =
			Mode == 0 ? EWindowMode::Fullscreen :
			(Mode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);

		Settings->SetFullscreenMode(NewMode);
		Settings->ApplyResolutionSettings(false);
		Settings->SaveSettings();
	}
}

void UMyBondageLifeSettingsWidget::ApplyVSync(bool bEnabled)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetVSyncEnabled(bEnabled);
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
	}
}

void UMyBondageLifeSettingsWidget::ApplyAntiAliasing(int32 Level)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetAntiAliasingQuality(FMath::Clamp(Level, 0, 4));
		Settings->ApplyNonResolutionSettings();
		Settings->SaveSettings();
	}
}

void UMyBondageLifeSettingsWidget::SwitchTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(FMath::Clamp(TabIndex, 0, 2));
	}

	for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
	{
		if (TabButtons[Index])
		{
			TabButtons[Index]->SetBackgroundColor(Index == TabIndex
				? FLinearColor(0.95f, 0.85f, 0.72f, 1.0f)
				: FLinearColor(0.85f, 0.82f, 0.86f, 1.0f));
		}
	}
}

void UMyBondageLifeSettingsWidget::OpenPanel()
{
	SetVisibility(ESlateVisibility::Visible);
	SwitchTab(0);
}

void UMyBondageLifeSettingsWidget::ClosePanel()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeSettingsWidget::TogglePanel()
{
	if (IsPanelOpen())
	{
		ClosePanel();
	}
	else
	{
		OpenPanel();
	}
}

bool UMyBondageLifeSettingsWidget::IsPanelOpen() const
{
	return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UMyBondageLifeSettingsWidget::HandleCloseClicked()
{
	ClosePanel();
}

void UMyBondageLifeSettingsWidget::HandleTab0Clicked() { SwitchTab(0); }
void UMyBondageLifeSettingsWidget::HandleTab1Clicked() { SwitchTab(1); }
void UMyBondageLifeSettingsWidget::HandleTab2Clicked() { SwitchTab(2); }

const TCHAR* UMyBondageLifeSettingsWidget::LanguageCode(int32 Index)
{
	static const TCHAR* Codes[] = { TEXT("zh-Hans"), TEXT("zh-Hant"), TEXT("en"), TEXT("ja"), TEXT("ko") };
	return Codes[FMath::Clamp(Index, 0, 4)];
}

float UMyBondageLifeSettingsWidget::LoadFloat(const TCHAR* Key, float DefaultValue)
{
	float Value = DefaultValue;
	if (GConfig)
	{
		GConfig->GetFloat(SettingsSection, Key, Value, SettingsFile);
	}
	return Value;
}

void UMyBondageLifeSettingsWidget::SaveFloat(const TCHAR* Key, float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(SettingsSection, Key, Value, SettingsFile);
		GConfig->Flush(false, SettingsFile);
	}
}

FString UMyBondageLifeSettingsWidget::LoadString(const TCHAR* Key, const FString& DefaultValue)
{
	FString Value = DefaultValue;
	if (GConfig)
	{
		GConfig->GetString(SettingsSection, Key, Value, SettingsFile);
	}
	return Value;
}

void UMyBondageLifeSettingsWidget::SaveString(const TCHAR* Key, const FString& Value)
{
	if (GConfig)
	{
		GConfig->SetString(SettingsSection, Key, *Value, SettingsFile);
		GConfig->Flush(false, SettingsFile);
	}
}

void UMyBondageLifeSettingsActionProxy::HandleComboChanged(FString SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (!OnIndexChanged)
	{
		return;
	}

	const int32 Index = Options.IndexOfByKey(SelectedItem);
	if (Index != INDEX_NONE)
	{
		OnIndexChanged(Index);
	}
}

void UMyBondageLifeSettingsActionProxy::HandleSliderChanged(float NewValue)
{
	if (OnValueChanged)
	{
		OnValueChanged(NewValue);
	}
}
