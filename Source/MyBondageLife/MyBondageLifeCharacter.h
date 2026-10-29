// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyBondageLifeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class APlayerState;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMyBondageLifeCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/**
	 *  Character display name. Replicated so a server-provided name would reach
	 *  every client; for now it keeps its default placeholder.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Identity", meta = (AllowPrivateAccess = "true"))
	FText CharacterName;

	/** Character class (1-4). Replicated so others can inspect this character. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Identity", meta = (AllowPrivateAccess = "true"))
	int32 CharacterClass = 1;

	/** Account creation time (seconds since 1970, UTC), also replicated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Identity", meta = (AllowPrivateAccess = "true"))
	int64 CreatedAtUnix = 0;

	/**
	 *  Body height multiplier (0.9 / 1.0 / 1.1). Replicated so every client scales
	 *  this character the same way; the server only stores the ratio.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_HeightScale, Category = "Identity", meta = (AllowPrivateAccess = "true"))
	float HeightScale = 1.0f;

	/** Hair style ID from DT_Hair (1 based). Replicated so others see the same hair. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_HairIndex, Category = "Identity", meta = (AllowPrivateAccess = "true"))
	int32 HairIndex = 1;

	/**
	 *  Temporary test pose: while true the character plays the restraint sequence
	 *  instead of the locomotion AnimBP, so the shoulders (clavicles) follow it.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_RestraintPose, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bRestraintPoseActive = false;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AMyBondageLifeCharacter();	

	/** World position the nameplate is drawn at: a bit above the head. */
	UFUNCTION(BlueprintPure, Category="Nameplate")
	FVector GetNameplateLocation() const;

	/** Text drawn on the nameplate: "角色名 <account>" until the server provides names. */
	UFUNCTION(BlueprintPure, Category="Nameplate")
	FString GetNameplateText() const;

	/** Display name of this character (placeholder until the server supplies one). */
	UFUNCTION(BlueprintPure, Category="Identity")
	FText GetCharacterName() const { return CharacterName; }

	/** Server-side hook for the real character name once the backend exists. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void SetCharacterName(const FText& InCharacterName);

	UFUNCTION(BlueprintPure, Category="Identity")
	int32 GetCharacterClass() const { return CharacterClass; }

	UFUNCTION(BlueprintPure, Category="Identity")
	int64 GetCreatedAtUnix() const { return CreatedAtUnix; }

	/** Applies name, class, account creation time, body height and hair in one call. */
	void SetCharacterIdentity(const FText& InCharacterName, int32 InCharacterClass, int64 InCreatedAtUnix, float InHeightScale, int32 InHairIndex);

	/** Current body height multiplier (0.9 / 1.0 / 1.1). */
	UFUNCTION(BlueprintPure, Category="Identity")
	float GetHeightScale() const { return HeightScale; }

	/** Hair style ID currently worn. */
	UFUNCTION(BlueprintPure, Category="Identity")
	int32 GetHairIndex() const { return HairIndex; }

	/** Rebuilds the hair mesh from DT_Hair and attaches it to the body. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyHairStyle();

	//~ Animation -----------------------------------------------------------

	/** Enables or disables the temporary restraint pose. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetRestraintPose(bool bActive);

	UFUNCTION(BlueprintPure, Category="Animation")
	bool IsRestraintPoseActive() const { return bRestraintPoseActive; }

	/** Switches the mesh to the copied AnimBP (ABP_Unarmed_Restraint). */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void ApplyAnimBlueprintOverride();

	/** Runs the pose animation, or hands control back to the AnimBP. */
	void RefreshRestraintPose();

	/** Rescales the mesh and capsule to match HeightScale. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyHeightScale();

protected:

	/** Re-applied on clients whenever the replicated height changes. */
	UFUNCTION()
	void OnRep_HeightScale();

	/** Re-applied on clients whenever the replicated hair changes. */
	UFUNCTION()
	void OnRep_HairIndex();

	/** Extra skeletal mesh worn on the head, created on demand. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> HairComponent;

	/** Re-applied on clients whenever the replicated pose state flips. */
	UFUNCTION()
	void OnRep_RestraintPose();

	/** AnimBP used instead of the one assigned on the blueprint mesh. */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftClassPath AnimBlueprintOverridePath;

	/** Montage played through the AnimBP slot while the pose is active. */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftObjectPath RestraintMontagePath;

	/** Unscaled reference sizes captured on BeginPlay. */
	float BaseCapsuleHalfHeight = 0.0f;
	float BaseCapsuleRadius = 0.0f;
	FVector BaseMeshRelativeLocation = FVector::ZeroVector;
	FVector BaseMeshScale = FVector::OneVector;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Replicates CharacterName so every client sees the same name. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Mouse wheel: pull the camera in / push it out (0.5x - 1.5x of the default). */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void ZoomIn();

	UFUNCTION(BlueprintCallable, Category="Camera")
	void ZoomOut();

	/** Default boom length captured on BeginPlay, used as the zoom reference. */
	float DefaultArmLength = 0.0f;

	/** Boom length the smooth interpolation is heading to. */
	float TargetArmLength = 0.0f;

	/** How far one wheel notch moves the camera. */
	UPROPERTY(EditAnywhere, Category="Camera")
	float ZoomStep = 40.0f;

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

