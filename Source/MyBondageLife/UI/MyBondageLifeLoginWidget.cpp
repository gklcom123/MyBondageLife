// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeLoginWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "MyBondageLife.h"
#include "UI/MyBondageLifeUIStyle.h"

namespace
{
	constexpr float FieldWidth = 460.0f;
	constexpr float FieldHeight = 52.0f;
	constexpr float ButtonHeight = 58.0f;

	const TCHAR* TextAccountId = TEXT("\u8D26\u53F7ID");
	const TCHAR* TextPassword = TEXT("\u5BC6\u7801");
	const TCHAR* TextRegister = TEXT("\u6CE8\u518C\u8D26\u53F7");
	const TCHAR* TextRegisterAction = TEXT("\u6CE8\u518C");
	const TCHAR* TextBack = TEXT("\u8FD4\u56DE");
	const TCHAR* TextLogin = TEXT("\u767B\u5F55");
	const TCHAR* TextConnecting = TEXT("\u6B63\u5728\u8FDE\u63A5\u670D\u52A1\u5668\u2026");
}

UMyBondageLifeLoginWidget::UMyBondageLifeLoginWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UMyBondageLifeLoginWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

UButton* UMyBondageLifeLoginWidget::CreateActionButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive)
{
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonSize->SetWidthOverride(FieldWidth);
	ButtonSize->SetHeightOverride(Height);
	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(ButtonSize))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetIsEnabled(bInteractive);
	ButtonSize->AddChild(Button);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(LabelText));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(bInteractive
		? FLinearColor(0.02f, 0.02f, 0.02f)
		: FLinearColor(0.45f, 0.45f, 0.48f)));
	{
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 24;
		Label->SetFont(LabelFont);
	}
	Button->AddChild(Label);

	return Button;
}

void UMyBondageLifeLoginWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.94f));
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(0);
	}

	// ---------------------------------------------------------------- login
	LoginPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoginPanel"));
	if (UCanvasPanelSlot* LoginSlot = RootCanvas->AddChildToCanvas(LoginPanel))
	{
		LoginSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		LoginSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		LoginSlot->SetAutoSize(true);
		LoginSlot->SetZOrder(1);
	}

	UTextBlock* TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleLabel"));
	TitleLabel->SetText(FText::FromString(TEXT("MyBondageLife")));
	TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo TitleFont = TitleLabel->GetFont();
		TitleFont.Size = 46;
		TitleLabel->SetFont(TitleFont);
	}
	if (UVerticalBoxSlot* TitleSlot = LoginPanel->AddChildToVerticalBox(TitleLabel))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UTextBlock* SubtitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleLabel"));
	// "账号登录"
	SubtitleLabel->SetText(FText::FromString(TEXT("\u8D26\u53F7\u767B\u5F55")));
	SubtitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.78f, 0.85f)));
	SubtitleLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo SubtitleFont = SubtitleLabel->GetFont();
		SubtitleFont.Size = 22;
		SubtitleLabel->SetFont(SubtitleFont);
	}
	if (UVerticalBoxSlot* SubtitleSlot = LoginPanel->AddChildToVerticalBox(SubtitleLabel))
	{
		SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
		SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	// account field
	USizeBox* AccountSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AccountSize"));
	AccountSize->SetWidthOverride(FieldWidth);
	AccountSize->SetHeightOverride(FieldHeight);
	if (UVerticalBoxSlot* AccountSlot = LoginPanel->AddChildToVerticalBox(AccountSize))
	{
		AccountSlot->SetHorizontalAlignment(HAlign_Center);
		AccountSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	AccountBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AccountBox"));
	AccountBox->SetHintText(FText::FromString(TextAccountId));
	MyBondageLifeUI::ApplyReadableTextInputStyle(AccountBox);
	AccountSize->AddChild(AccountBox);

	// password field
	USizeBox* PasswordSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PasswordSize"));
	PasswordSize->SetWidthOverride(FieldWidth);
	PasswordSize->SetHeightOverride(FieldHeight);
	if (UVerticalBoxSlot* PasswordSlot = LoginPanel->AddChildToVerticalBox(PasswordSize))
	{
		PasswordSlot->SetHorizontalAlignment(HAlign_Center);
		PasswordSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}
	PasswordBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("PasswordBox"));
	PasswordBox->SetHintText(FText::FromString(TextPassword));
	MyBondageLifeUI::ApplyReadableTextInputStyle(PasswordBox);
	PasswordBox->SetIsPassword(true);
	PasswordBox->OnTextCommitted.AddDynamic(this, &UMyBondageLifeLoginWidget::HandlePasswordCommitted);
	PasswordSize->AddChild(PasswordBox);

	LoginButton = CreateActionButton(LoginPanel, TextLogin, ButtonHeight, true);
	LoginButton->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleLoginClicked);

	OpenRegisterButton = CreateActionButton(LoginPanel, TextRegister, ButtonHeight, true);
	OpenRegisterButton->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleOpenRegisterClicked);

	LoginStatusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoginStatusLabel"));
	LoginStatusLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.45f, 0.45f)));
	LoginStatusLabel->SetJustification(ETextJustify::Center);
	LoginStatusLabel->SetAutoWrapText(true);
	{
		FSlateFontInfo StatusFont = LoginStatusLabel->GetFont();
		StatusFont.Size = 17;
		LoginStatusLabel->SetFont(StatusFont);
	}
	if (UVerticalBoxSlot* StatusSlot = LoginPanel->AddChildToVerticalBox(LoginStatusLabel))
	{
		StatusSlot->SetHorizontalAlignment(HAlign_Center);
		StatusSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}

	// ----------------------------------------------------------- registration
	RegisterPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RegisterPanel"));
	if (UCanvasPanelSlot* RegisterSlot = RootCanvas->AddChildToCanvas(RegisterPanel))
	{
		RegisterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		RegisterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RegisterSlot->SetAutoSize(true);
		RegisterSlot->SetZOrder(2);
	}
	RegisterPanel->SetVisibility(ESlateVisibility::Collapsed);

	UTextBlock* RegisterTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RegisterTitle"));
	RegisterTitle->SetText(FText::FromString(TextRegister));
	RegisterTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	RegisterTitle->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo RegisterTitleFont = RegisterTitle->GetFont();
		RegisterTitleFont.Size = 38;
		RegisterTitle->SetFont(RegisterTitleFont);
	}
	if (UVerticalBoxSlot* RegisterTitleSlot = RegisterPanel->AddChildToVerticalBox(RegisterTitle))
	{
		RegisterTitleSlot->SetHorizontalAlignment(HAlign_Center);
		RegisterTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
	}

	auto AddTextField = [this](UVerticalBox* ParentPanel, const TCHAR* HintText, bool bIsPassword)
	{
		USizeBox* FieldSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		FieldSize->SetWidthOverride(FieldWidth);
		FieldSize->SetHeightOverride(FieldHeight);
		if (UVerticalBoxSlot* FieldSlot = ParentPanel->AddChildToVerticalBox(FieldSize))
		{
			FieldSlot->SetHorizontalAlignment(HAlign_Center);
			FieldSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		Box->SetHintText(FText::FromString(HintText));
		MyBondageLifeUI::ApplyReadableTextInputStyle(Box);
		if (bIsPassword)
		{
			Box->SetIsPassword(true);
		}
		FieldSize->AddChild(Box);
		return Box;
	};

	// "账号ID（6-24 位字母/数字/下划线）"
	RegisterAccountBox = AddTextField(RegisterPanel,
		TEXT("\u8D26\u53F7ID\uFF086-24 \u4F4D\uFF09"), false);
	// "密码（6-24 位，不含中文）"
	RegisterPasswordBox = AddTextField(RegisterPanel,
		TEXT("\u5BC6\u7801\uFF086-24 \u4F4D\uFF0C\u4E0D\u542B\u4E2D\u6587\uFF09"), true);
	// "角色名（最多 8 个字符）"
	RegisterCharacterNameBox = AddTextField(RegisterPanel,
		TEXT("\u89D2\u8272\u540D\uFF08\u6700\u591A 8 \u4E2A\u5B57\u7B26\uFF09"), false);
	// 角色类别：圆形单选（支配者 / 转换者 / 臣服者 / 奴隶）
	BuildClassSelector(RegisterPanel);

	RegisterButton = CreateActionButton(RegisterPanel, TextRegisterAction, ButtonHeight, true);
	RegisterButton->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleRegisterClicked);

	BackToLoginButton = CreateActionButton(RegisterPanel, TextBack, ButtonHeight, true);
	BackToLoginButton->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleBackToLoginClicked);

	RegisterStatusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RegisterStatusLabel"));
	RegisterStatusLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.45f, 0.45f)));
	RegisterStatusLabel->SetJustification(ETextJustify::Center);
	RegisterStatusLabel->SetAutoWrapText(true);
	{
		FSlateFontInfo RegisterStatusFont = RegisterStatusLabel->GetFont();
		RegisterStatusFont.Size = 17;
		RegisterStatusLabel->SetFont(RegisterStatusFont);
	}
	if (UVerticalBoxSlot* RegisterStatusSlot = RegisterPanel->AddChildToVerticalBox(RegisterStatusLabel))
	{
		RegisterStatusSlot->SetHorizontalAlignment(HAlign_Center);
		RegisterStatusSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	}
}

void UMyBondageLifeLoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnAuthSucceeded.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleAuthSucceeded);
			Account->OnAuthFailed.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleAuthFailed);
		}
	}

	if (AccountBox)
	{
		AccountBox->SetKeyboardFocus();
	}
}

void UMyBondageLifeLoginWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnAuthSucceeded.RemoveDynamic(this, &UMyBondageLifeLoginWidget::HandleAuthSucceeded);
			Account->OnAuthFailed.RemoveDynamic(this, &UMyBondageLifeLoginWidget::HandleAuthFailed);
		}
	}

	Super::NativeDestruct();
}

void UMyBondageLifeLoginWidget::ShowRegisterPanel()
{
	if (LoginPanel)
	{
		LoginPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RegisterPanel)
	{
		RegisterPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (RegisterStatusLabel)
	{
		RegisterStatusLabel->SetText(FText::GetEmpty());
	}
	if (RegisterAccountBox)
	{
		RegisterAccountBox->SetKeyboardFocus();
	}
}

void UMyBondageLifeLoginWidget::ShowLoginPanel()
{
	if (RegisterPanel)
	{
		RegisterPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (LoginPanel)
	{
		LoginPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (AccountBox)
	{
		AccountBox->SetKeyboardFocus();
	}
}

void UMyBondageLifeLoginWidget::SetStatus(const FString& Message, bool bIsError)
{
	UTextBlock* Target = (RegisterPanel && RegisterPanel->GetVisibility() == ESlateVisibility::Visible)
		? RegisterStatusLabel
		: LoginStatusLabel;

	if (Target)
	{
		Target->SetText(FText::FromString(Message));
		Target->SetColorAndOpacity(FSlateColor(bIsError
			? FLinearColor(0.88f, 0.42f, 0.42f)
			: FLinearColor(0.62f, 0.72f, 0.9f)));
	}
}

void UMyBondageLifeLoginWidget::SetButtonsEnabled(bool bEnabled)
{
	if (LoginButton) { LoginButton->SetIsEnabled(bEnabled); }
	if (OpenRegisterButton) { OpenRegisterButton->SetIsEnabled(bEnabled); }
	if (RegisterButton) { RegisterButton->SetIsEnabled(bEnabled); }
	if (BackToLoginButton) { BackToLoginButton->SetIsEnabled(bEnabled); }
}

bool UMyBondageLifeLoginWidget::ValidateAccount(const FString& Account, FString& OutError)
{
	const FString Trimmed = Account.TrimStartAndEnd();

	// "账号ID 需为 6-24 位字母、数字或下划线"
	if (Trimmed.Len() < 6 || Trimmed.Len() > 24)
	{
		OutError = TEXT("\u8D26\u53F7ID \u9700\u4E3A 6-24 \u4F4D\u5B57\u6BCD\u3001\u6570\u5B57\u6216\u4E0B\u5212\u7EBF");
		return false;
	}

	for (const TCHAR Char : Trimmed)
	{
		const bool bAllowed = (Char >= 'A' && Char <= 'Z')
			|| (Char >= 'a' && Char <= 'z')
			|| (Char >= '0' && Char <= '9')
			|| Char == '_';

		if (!bAllowed)
		{
			OutError = TEXT("\u8D26\u53F7ID \u53EA\u80FD\u5305\u542B\u5B57\u6BCD\u3001\u6570\u5B57\u6216\u4E0B\u5212\u7EBF\uFF0C\u4E0D\u80FD\u4E2D\u6587");
			return false;
		}
	}

	return true;
}

bool UMyBondageLifeLoginWidget::ValidatePassword(const FString& Password, FString& OutError)
{
	// "密码需为 6-24 位，且不能含中文"
	if (Password.Len() < 6 || Password.Len() > 24)
	{
		OutError = TEXT("\u5BC6\u7801\u9700\u4E3A 6-24 \u4F4D\uFF0C\u4E14\u4E0D\u80FD\u542B\u4E2D\u6587");
		return false;
	}

	for (const TCHAR Char : Password)
	{
		if (Char < 0x21 || Char > 0x7E)
		{
			OutError = TEXT("\u5BC6\u7801\u53EA\u80FD\u4F7F\u7528\u5B57\u6BCD\u3001\u6570\u5B57\u548C\u5E38\u89C1\u7B26\u53F7");
			return false;
		}
	}

	return true;
}

bool UMyBondageLifeLoginWidget::ValidateCharacterName(const FString& CharacterName, FString& OutError)
{
	const FString Trimmed = CharacterName.TrimStartAndEnd();

	// "角色名需为 1-8 个字符"
	if (Trimmed.IsEmpty() || Trimmed.Len() > 8)
	{
		OutError = TEXT("\u89D2\u8272\u540D\u9700\u4E3A 1-8 \u4E2A\u5B57\u7B26");
		return false;
	}

	for (const TCHAR Char : Trimmed)
	{
		if (Char < 0x20)
		{
			OutError = TEXT("\u89D2\u8272\u540D\u5305\u542B\u975E\u6CD5\u5B57\u7B26");
			return false;
		}
	}

	return true;
}

void UMyBondageLifeLoginWidget::HandleLoginClicked()
{
	SubmitLogin();
}

void UMyBondageLifeLoginWidget::HandleOpenRegisterClicked()
{
	ShowRegisterPanel();
}

void UMyBondageLifeLoginWidget::HandleRegisterClicked()
{
	SubmitRegistration();
}

void UMyBondageLifeLoginWidget::HandleBackToLoginClicked()
{
	ShowLoginPanel();
}

void UMyBondageLifeLoginWidget::HandlePasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SubmitLogin();
	}
}

void UMyBondageLifeLoginWidget::SubmitLogin()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (!Account)
	{
		SetStatus(TEXT("\u8D26\u53F7\u7CFB\u7EDF\u672A\u5C31\u7EEA"), true);
		return;
	}

	if (Account->IsAuthRequestInFlight())
	{
		return;
	}

	const FString AccountId = AccountBox ? AccountBox->GetText().ToString() : FString();
	const FString Password = PasswordBox ? PasswordBox->GetText().ToString() : FString();

	FString Error;
	if (!ValidateAccount(AccountId, Error))
	{
		SetStatus(Error, true);
		return;
	}
	if (!ValidatePassword(Password, Error))
	{
		SetStatus(Error, true);
		return;
	}

	SetStatus(TextConnecting, false);
	SetButtonsEnabled(false);

	Account->RequestLogin(AccountId, Password);
}

void UMyBondageLifeLoginWidget::SubmitRegistration()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeAccountSubsystem* Account = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;

	if (!Account)
	{
		SetStatus(TEXT("\u8D26\u53F7\u7CFB\u7EDF\u672A\u5C31\u7EEA"), true);
		return;
	}

	if (Account->IsAuthRequestInFlight())
	{
		return;
	}

	const FString AccountId = RegisterAccountBox ? RegisterAccountBox->GetText().ToString() : FString();
	const FString Password = RegisterPasswordBox ? RegisterPasswordBox->GetText().ToString() : FString();
	const FString CharacterName = RegisterCharacterNameBox ? RegisterCharacterNameBox->GetText().ToString() : FString();

	FString Error;
	if (!ValidateAccount(AccountId, Error))
	{
		SetStatus(Error, true);
		return;
	}
	if (!ValidatePassword(Password, Error))
	{
		SetStatus(Error, true);
		return;
	}
	if (!ValidateCharacterName(CharacterName, Error))
	{
		SetStatus(Error, true);
		return;
	}


	SetStatus(TextConnecting, false);
	SetButtonsEnabled(false);

	Account->RequestRegister(AccountId, Password, CharacterName, SelectedCharacterClass);
}

void UMyBondageLifeLoginWidget::HandleAuthSucceeded()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Login: authentication succeeded, entering the main scene."));
	OnLoginCompleted.Broadcast();
}

void UMyBondageLifeLoginWidget::HandleAuthFailed(const FText& ErrorMessage)
{
	SetButtonsEnabled(true);
	SetStatus(ErrorMessage.ToString(), true);
}

void UMyBondageLifeLoginWidget::BuildClassSelector(UVerticalBox* Parent)
{
	UHorizontalBox* ClassRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ClassRow"));
	if (UVerticalBoxSlot* ClassRowSlot = Parent->AddChildToVerticalBox(ClassRow))
	{
		ClassRowSlot->SetHorizontalAlignment(HAlign_Center);
		ClassRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UButton* Option1 = CreateClassOption(ClassRow, TEXT("\u652F\u914D\u8005"));
	UButton* Option2 = CreateClassOption(ClassRow, TEXT("\u8F6C\u6362\u8005"));
	UButton* Option3 = CreateClassOption(ClassRow, TEXT("\u81E3\u670D\u8005"));
	UButton* Option4 = CreateClassOption(ClassRow, TEXT("\u5974\u96B6"));

	Option1->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleClass1Clicked);
	Option2->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleClass2Clicked);
	Option3->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleClass3Clicked);
	Option4->OnClicked.AddDynamic(this, &UMyBondageLifeLoginWidget::HandleClass4Clicked);

	SetSelectedClass(1);
}

UButton* UMyBondageLifeLoginWidget::CreateClassOption(UHorizontalBox* Row, const TCHAR* LabelText)
{
	UButton* Option = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	{
		// Invisible button chrome: only the round indicator and the label matter.
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Option->SetStyle(ButtonStyle);
	}

	if (UHorizontalBoxSlot* OptionSlot = Row->AddChildToHorizontalBox(Option))
	{
		OptionSlot->SetVerticalAlignment(VAlign_Center);
		OptionSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Option->AddChild(Content);

	USizeBox* CircleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CircleSize->SetWidthOverride(16.0f);
	CircleSize->SetHeightOverride(16.0f);
	CircleSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UHorizontalBoxSlot* CircleSlot = Content->AddChildToHorizontalBox(CircleSize))
	{
		CircleSlot->SetVerticalAlignment(VAlign_Center);
		CircleSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}

	UBorder* Indicator = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Indicator->SetPadding(FMargin(0.0f));
	// The decorative circle must not eat clicks aimed at the option button.
	Indicator->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CircleSize->AddChild(Indicator);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(LabelText));
	{
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 18;
		Label->SetFont(LabelFont);
	}
	if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(Label))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	ClassIndicators.Add(Indicator);
	ClassLabels.Add(Label);

	return Option;
}

void UMyBondageLifeLoginWidget::SetSelectedClass(int32 ClassValue)
{
	SelectedCharacterClass = FMath::Clamp(ClassValue, 1, 4);

	for (int32 Index = 0; Index < ClassIndicators.Num(); ++Index)
	{
		const bool bSelected = (Index + 1) == SelectedCharacterClass;

		if (UBorder* Indicator = ClassIndicators[Index])
		{
			// A rounded box with a fixed corner radius renders as a circle.
			FSlateBrush CircleBrush;
			CircleBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			CircleBrush.TintColor = bSelected
				? FLinearColor(0.34f, 0.72f, 1.0f, 1.0f)
				: FLinearColor(0.08f, 0.09f, 0.12f, 1.0f);
			CircleBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
			CircleBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			CircleBrush.OutlineSettings.Width = 2.0f;
			CircleBrush.OutlineSettings.Color = FSlateColor(bSelected
				? FLinearColor(0.72f, 0.90f, 1.0f, 1.0f)
				: FLinearColor(0.55f, 0.58f, 0.66f, 1.0f));
			Indicator->SetBrush(CircleBrush);
		}

		if (UTextBlock* Label = ClassLabels[Index])
		{
			Label->SetColorAndOpacity(FSlateColor(bSelected
				? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
				: FLinearColor(0.70f, 0.73f, 0.80f, 1.0f)));
		}
	}
}

void UMyBondageLifeLoginWidget::HandleClass1Clicked()
{
	SetSelectedClass(1);
}

void UMyBondageLifeLoginWidget::HandleClass2Clicked()
{
	SetSelectedClass(2);
}

void UMyBondageLifeLoginWidget::HandleClass3Clicked()
{
	SetSelectedClass(3);
}

void UMyBondageLifeLoginWidget::HandleClass4Clicked()
{
	SetSelectedClass(4);
}