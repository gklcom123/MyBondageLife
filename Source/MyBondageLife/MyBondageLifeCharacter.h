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

	/** Server side setter; clients receive the arrays through replication. */
	UFUNCTION(BlueprintCallable, Category="Restraint")
	void SetRestraints(const TArray<int32>& InSlots, const TArray<FName>& InItems);

	/** Rebuilds the worn meshes, the derived flags and the pose montage. */
	UFUNCTION(BlueprintCallable, Category="Restraint")
	void ApplyRestraints();

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

	/** Picks idle / walk / run from the current speed and plays it in single-node mode. */
	void UpdateLocomotionAnimation();

	/** Re-applied on clients whenever the replicated hair changes. */
	UFUNCTION()
	void OnRep_HairIndex();

	/** Extra skeletal mesh worn on the head, created on demand. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> HairComponent;

	/** Re-applied on clients whenever the replicated pose state flips. */
	UFUNCTION()
	void OnRep_RestraintPose();

	//~ Restraints ---------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintArmActive() const { return bRestraintArm; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintLegActive() const { return bRestraintLeg; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsCantMoveActive() const { return bCantMove; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsInPetModeActive() const { return bInPetMode; }

	UFUNCTION()
	void OnRep_Restraints();

	/** Slot index per equipped item (parallel to RestraintItemIds). */
	UPROPERTY(ReplicatedUsing = OnRep_Restraints, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<int32> RestraintSlots;

	/** DT_Restraint row names of the equipped items. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<FName> RestraintItemIds;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bRestraintArm = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bRestraintLeg = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bCantMove = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bInPetMode = false;

	/** Montage driven by the equipped items (lowest slot wins). */
	UPROPERTY(Transient)
	TObjectPtr<class UAnimMontage> ActiveUpAnmi;

	/** Spawned components for the worn restraint meshes. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> RestraintComponents;

	/** AnimBP used instead of the one assigned on the blueprint mesh. */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftClassPath AnimBlueprintOverridePath;

	/** Montage played through the AnimBP slot while the pose is active. */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftObjectPath RestraintMontagePath;

	/** Locomotion clips from /Game/Characters/Animation/Normal. */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftObjectPath IdleSequencePath;

	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftObjectPath WalkSequencePath;

	UPROPERTY(EditDefaultsOnly, Category="Animation")
	FSoftObjectPath RunSequencePath;

	/** Sequence currently playing in single-node mode. */
	UPROPERTY(Transient)
	TObjectPtr<class UAnimSequence> CurrentLocomotionSequence;

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

	//~ Walk / run -----------------------------------------------------------

	/** Shift pressed: switch to the running pace. */
	void StartRunning();

	/** Shift released: back to walking. */
	void StopRunning();

	/** Applies the running flag locally and tells the server about it. */
	void SetRunning(bool bNewRunning);

	/** Writes the matching max walk speed into the movement component. */
	void ApplyMoveSpeed();

	UFUNCTION(Server, Reliable)
	void ServerSetRunning(bool bNewRunning);

	UFUNCTION()
	void OnRep_Running();

	/** Default pace (no modifier). */
	UPROPERTY(EditAnywhere, Category="Movement")
	float WalkSpeed = 125.0f;

	/** Pace while Shift is held. */
	UPROPERTY(EditAnywhere, Category="Movement")
	float RunSpeed = 300.0f;

	/** Replicated so other clients see the same pace. */
	UPROPERTY(ReplicatedUsing = OnRep_Running, BlueprintReadOnly, Category="Movement", meta = (AllowPrivateAccess = "true"))
	bool bRunning = false;


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

