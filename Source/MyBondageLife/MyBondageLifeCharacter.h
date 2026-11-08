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

	/** Server-computed mesh transform, replicated verbatim so every client matches. */
	UPROPERTY(ReplicatedUsing = OnRep_MeshPlacement, BlueprintReadOnly, Category="Identity", meta = (AllowPrivateAccess = "true"))
	FVector ReplicatedMeshRelativeLocation = FVector::ZeroVector;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Identity", meta = (AllowPrivateAccess = "true"))
	FVector ReplicatedMeshScale = FVector::OneVector;

	UFUNCTION()
	void OnRep_MeshPlacement();

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

	/** 衣物：槽位与道具 id（并行数组）。 */
	UFUNCTION(BlueprintCallable, Category="Outfit")
	void SetOutfits(const TArray<int32>& InSlots, const TArray<FName>& InItems);

	/** 每个槽位 3 个通道的 #RRGGBB（与 OutfitItemIds 对应，3 个一组）。 */
	UFUNCTION(BlueprintCallable, Category="Outfit")
	void SetOutfitColors(const TArray<FString>& InColors);

	void ApplyOutfitColors();

	/** 当前穿着的衣物件数（用于 UI 空数据保护）。 */
	UFUNCTION(BlueprintPure, Category = "Outfit")
	int32 GetWornOutfitCount() const { return OutfitItemIds.Num(); }

	// ---------------------------------------------------------------- sitting
	UFUNCTION(Server, Reliable)
	void ServerSetSitting(bool bSit, FVector SeatLocation, FRotator SeatRotation, float SeatOffset = 0.0f, AActor* SeatActor = nullptr);

	// ---------------------------------------------------------------- emote
	UFUNCTION(BlueprintCallable, Category="Emote")
	void PlayEmote(class UAnimMontage* Montage);

	UFUNCTION(BlueprintCallable, Category="Emote")
	void StopEmote();

	UFUNCTION(Server, Reliable)
	void ServerPlayEmote(const FString& MontagePath);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEmote(const FString& MontagePath);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSystemMessage(const FString& Text);

	UFUNCTION(Server, Reliable)
	void ServerAnnounceSystemMessage(const FString& Text);

	UFUNCTION(BlueprintPure, Category="Emote")
	bool IsEmotePlaying() const { return CurrentEmote != nullptr; }

	UPROPERTY(Transient)
	TObjectPtr<class UAnimMontage> CurrentEmote;

	UFUNCTION(BlueprintPure, Category="Sitting")
	bool IsSitting() const { return bSitting; }

	void ApplySitting();

	/** 拘束染色：按槽位/通道应用到拘束网格材质。 */
	UFUNCTION(BlueprintCallable, Category="Restraint")
	void SetRestraintColorChannel(int32 Slot, int32 Channel, const FString& HexColor);

	void ApplyRestraintColors();

	/** 远端同步用：槽位×3 的扁平颜色数组（空串=未染色）。 */
	UFUNCTION(BlueprintCallable, Category="Restraint")
	void SetRestraintColors(const TArray<FString>& Colors);

	UPROPERTY(ReplicatedUsing = OnRep_RestraintColors)
	TArray<FString> RestraintColorArray;

	UFUNCTION()
	void OnRep_RestraintColors();

	TMap<int32, TMap<int32, FString>> RestraintColorHex;

	UPROPERTY(Replicated)
	float SittingOffset = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Sitting, BlueprintReadOnly, Category="Sitting")
	bool bSitting = false;

	UPROPERTY(Replicated)
	FVector SittingLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FRotator SittingRotation = FRotator::ZeroRotator;

	UFUNCTION()
	void OnRep_Sitting();

	float SittingMeshDrop = 0.0f;

	/** 当前坐着的椅子（用于占座判定）。 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Sitting")
	TObjectPtr<AActor> OccupiedSeat;

	UFUNCTION(BlueprintPure, Category="Sitting")
	AActor* GetOccupiedSeat() const { return OccupiedSeat; }
	/** 设置某槽位某通道的颜色（空字符串=清除）。 */
	UFUNCTION(BlueprintCallable, Category="Outfit")
	void SetOutfitColorChannel(int32 Slot, int32 Channel, const FString& HexColor);

	/** 账号名（复制到其他客户端，用来拉取该玩家的衣物）。 */
	UFUNCTION(Server, Reliable)
	void ServerSetAccountName(const FString& InAccount);

	UFUNCTION(BlueprintPure, Category="Identity")
	FString GetAccountName() const { return AccountName; }

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Identity")
	FString AccountName;

	/** Rebuilds the worn meshes, the derived flags and the pose montage. */
	UFUNCTION(BlueprintCallable, Category="Restraint")
	void ApplyRestraints();

	/** Public mirror of the restraint state (used by other widgets). */
	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsArmsRestrained() const { return bRestraintArm; }

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

	/** Captures the unscaled mesh/capsule reference exactly once (constructor). */
	void CaptureBasePose();

	bool bBasePoseCaptured = false;

	/** Temporary switch: turn height scaling off so every pawn renders at 1.0. */
	UPROPERTY(EditDefaultsOnly, Category="Identity")
	bool bHeightScalingDisabled = true;

	/** Per-preset Z correction (heightScale -> cm). Tuned from in-game measurements. */
	UPROPERTY(EditDefaultsOnly, Category="Identity")
	TMap<float, float> HeightZOffsets;

public:

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool GetGagActive() const { return bRestraintInGag; }

	/**
	 *  当前嘴部拘束对应的混淆档位（Garbler 的 id，如 "BallGag"）。
	 *  没戴 / 没配 GagType 时：没戴返回 None，配了旧数据则回退到默认口球档位。
	 */
	UFUNCTION(BlueprintPure, Category="Restraint")
	FName GetGagProfileId() const;

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool GetArmActive() const { return bRestraintArm; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool GetLegActive() const { return bRestraintLeg; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool GetCantMoveActive() const { return bCantMove; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool GetBlindActive() const { return bRestraintBlind; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	int32 GetBlindLevel() const { return bRestraintBlind ? RestraintBlindLevel : 0; }

protected:

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintArmActivePublic() const { return bRestraintArm; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintLegActivePublic() const { return bRestraintLeg; }

	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintCantMoveActivePublic() const { return bCantMove; }

protected:
	UFUNCTION(BlueprintPure, Category="Restraint")
	bool IsRestraintGagActive() const { return bRestraintInGag; }


	/** Re-applied on clients whenever the replicated height changes. */
	UFUNCTION()
	void OnRep_HeightScale();

	/** Picks idle / walk / run from the current speed and plays it in single-node mode. */
	void UpdateLocomotionAnimation();

	/** Plays / stops the head-only (gag) montage. */
	void RefreshGagPose();

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

	UFUNCTION()
	void OnRep_Outfits();

	/** 重建衣物网格并按 FeetOffset 调整胶囊。 */
	void ApplyOutfits();

	/** Slot index per equipped item (parallel to RestraintItemIds). */
	UPROPERTY(ReplicatedUsing = OnRep_Restraints, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<int32> RestraintSlots;

	/** DT_Restraint row names of the equipped items. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<FName> RestraintItemIds;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bRestraintArm = false;

	UPROPERTY(ReplicatedUsing = OnRep_Restraints, BlueprintReadOnly, Category="Restraint")
	bool bRestraintInGag = false;

	UPROPERTY(ReplicatedUsing = OnRep_Outfits, BlueprintReadOnly, Category="Outfit", meta = (AllowPrivateAccess = "true"))
	TArray<int32> OutfitSlots;

	UPROPERTY(ReplicatedUsing = OnRep_Outfits, BlueprintReadOnly, Category="Outfit", meta = (AllowPrivateAccess = "true"))
	TArray<FName> OutfitItemIds;

	UPROPERTY(ReplicatedUsing = OnRep_Outfits, BlueprintReadOnly, Category="Outfit", meta = (AllowPrivateAccess = "true"))
	TArray<FString> OutfitColorHex;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<class USkeletalMeshComponent>> OutfitComponents;

	UPROPERTY(ReplicatedUsing = OnRep_Restraints, BlueprintReadOnly, Category="Restraint")
	bool bRestraintBlind = false;

	UPROPERTY(ReplicatedUsing = OnRep_Restraints, BlueprintReadOnly, Category="Restraint")
	int32 RestraintBlindLevel = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bRestraintLeg = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bCantMove = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Restraint", meta = (AllowPrivateAccess = "true"))
	bool bInPetMode = false;

	UPROPERTY(Transient)
	TObjectPtr<class UAnimMontage> ActiveGagAnmi;

	UPROPERTY(Transient)
	TObjectPtr<class UAnimMontage> PlayingPoseAnmi;

	UPROPERTY(Transient)
	TObjectPtr<class UAnimMontage> PlayingGagAnmi;

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

	virtual void PostInitializeComponents() override;

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

	/** Restraint-driven movement limits: legs force walking, CantMove blocks movement. */
	void ApplyRestraintMovementLimits();

	/** 眼罩：按 BlindLevel 平滑收紧视野（仅本人可见）。 */
	void ApplyBlindEffect(float InDeltaTime);

	/** 性欲值屏幕效果等级：0 < 40%，1 >= 40%，2 >= 70%（本地玩家）。 */
public:
	void SetLibidoScreenLevel(int32 InLevel);

	/** 0 = 无，1 = 40%+，2 = 70%+（仅本地玩家）。 */
	int32 GetLibidoScreenLevel() const { return LibidoScreenLevel; }

protected:

	int32 LibidoScreenLevel = 0;
	float LibidoVisualLevel = 0.0f;

	/** 拘束的 HideBodyPart：把对应材质插槽换成透明材质（任一件生效即可）。 */
	void ApplyBodyPartHiding();

	/** 拘束与衣物各自要求隐藏的身体部位（并集生效）。 */
	TSet<FName> RestraintHiddenParts;
	TSet<FName> OutfitHiddenParts;

	/** 衣物 FeetOffset 合计，用于抬高胶囊。 */
	float OutfitFeetOffset = 0.0f;

	float HideReassertAccumulator = 0.0f;

	void ApplyOutfitCapsule();

	/** 皮肤：把 upbody/leg/hand/feet 换成 Skin_BodyMat，yatoe 换成 Skin_YatoeMat。 */
	void ApplySkin();

	UFUNCTION(BlueprintPure, Category="Skin")
	int32 GetSkinId() const { return SkinId; }

	UPROPERTY(ReplicatedUsing = OnRep_Skin, BlueprintReadOnly, Category="Skin")
	int32 SkinId = 1;

	UFUNCTION()
	void OnRep_Skin();

public:
	UFUNCTION(Server, Reliable)
	void ServerSetSkin(int32 InSkinId);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<class UMaterialInstanceDynamic>> SkinBodyMaterials;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> HiddenBodyMaterial;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<class UMaterialInterface>> OriginalBodyMaterials;

	float BlindVisualLevel = 0.0f;

	bool bOutfitBaseCaptured = false;
	float OutfitBaseHalfHeight = 96.0f;
	float OutfitBaseMeshZ = 0.0f;

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

	/** 高潮：动画蓝图 OnSex=true、镜头拉近、播放单次音效。 */
	UFUNCTION(BlueprintCallable, Category = "Sex")
	void BeginClimax();

	UFUNCTION(BlueprintCallable, Category = "Sex")
	void EndClimax();

	/** 通过服务器广播，让所有客户端都看到高潮动画。 */
	UFUNCTION(Server, Reliable)
	void ServerSetClimax(bool bActive);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetClimax(bool bActive);

	void ApplyClimaxAnim(bool bActive);

	bool bClimaxActive = false;
	float ClimaxSavedArmLength = -1.0f;
	float ClimaxLockedArmLength = -1.0f;

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

