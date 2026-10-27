// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "MyBondageLifeLoginWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

/** Raised once the player signed in (or registered) successfully. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeLoginCompleted);

/**
 *  Account screen: login plus a registration sub-panel.
 *
 *  Validation happens here (length, character set, class range); anything the
 *  server rejects is shown in the status line as a translated message.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeLoginWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeLoginWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Login")
	FMyBondageLifeLoginCompleted OnLoginCompleted;

	/** Sends the login request with the current field values. */
	UFUNCTION(BlueprintCallable, Category = "Login")
	void SubmitLogin();

	/** Sends the registration request with the current field values. */
	UFUNCTION(BlueprintCallable, Category = "Login")
	void SubmitRegistration();

	UFUNCTION(BlueprintCallable, Category = "Login")
	void ShowRegisterPanel();

	UFUNCTION(BlueprintCallable, Category = "Login")
	void ShowLoginPanel();

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();

	/** Fixed width button with a centred label; bPrimary only tints the text. */
	UButton* CreateActionButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive);

	/** Row of four circular radio options for the character class. */
	void BuildClassSelector(UVerticalBox* Parent);

	/** One radio option: a round indicator plus its label. */
	UButton* CreateClassOption(UHorizontalBox* Row, const TCHAR* LabelText);

	/** Highlights the chosen class (1-4). */
	void SetSelectedClass(int32 ClassValue);

	UFUNCTION()
	void HandleLoginClicked();

	UFUNCTION()
	void HandleOpenRegisterClicked();

	UFUNCTION()
	void HandleRegisterClicked();

	UFUNCTION()
	void HandleBackToLoginClicked();

	UFUNCTION()
	void HandleClass1Clicked();

	UFUNCTION()
	void HandleClass2Clicked();

	UFUNCTION()
	void HandleClass3Clicked();

	UFUNCTION()
	void HandleClass4Clicked();

	UFUNCTION()
	void HandlePasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleAuthSucceeded();

	UFUNCTION()
	void HandleAuthFailed(const FText& ErrorMessage);

	void SetStatus(const FString& Message, bool bIsError);
	void SetButtonsEnabled(bool bEnabled);

	/** Client side rules, mirroring the fields the service expects. */
	static bool ValidateAccount(const FString& Account, FString& OutError);
	static bool ValidatePassword(const FString& Password, FString& OutError);
	static bool ValidateCharacterName(const FString& CharacterName, FString& OutError);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	//~ Login panel
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> LoginPanel;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> AccountBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> PasswordBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OpenRegisterButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LoginStatusLabel;

	//~ Registration panel
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RegisterPanel;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegisterAccountBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegisterPasswordBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RegisterCharacterNameBox;

	/** Round indicators of the four class options, in class order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> ClassIndicators;

	/** Labels of the four class options, in class order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ClassLabels;

	/** Currently picked character class (1-4). */
	int32 SelectedCharacterClass = 1;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RegisterButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackToLoginButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RegisterStatusLabel;
};
