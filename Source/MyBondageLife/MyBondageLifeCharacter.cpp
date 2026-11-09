// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeCharacter.h"

#include "Core/MyBondageLifeHairCatalog.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Core/MyBondageLifeOutfitCatalog.h"
#include "Core/MyBondageLifeSkinCatalog.h"
#include "Core/Garbler/MyBondageLifeGagTypeMap.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MyBondageLife.h"
#include "MyBondageLifePlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** Bone the nameplate is anchored to when the skeleton provides it. */
	const FName NameplateAttachBone = TEXT("head");

	/** Height above the head bone, in centimetres (clears the model's head). */
	constexpr float NameplateHeadOffset = 45.0f;
}

AMyBondageLifeCharacter::AMyBondageLifeCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// The animation blueprint is owned by the blueprint asset (BP_PlayerCharc). Set
	// AnimBlueprintOverridePath in the class defaults only if it must be forced from code.
	AnimBlueprintOverridePath = FSoftClassPath();

	RestraintMontagePath = FSoftObjectPath(
		TEXT("/Game/Characters/Animation/Restraint/AM_Restraint_HandBack_F.AM_Restraint_HandBack_F"));
	IdleSequencePath = FSoftObjectPath(TEXT("/Game/Characters/Animation/Normal/Idle.Idle"));
	WalkSequencePath = FSoftObjectPath(TEXT("/Game/Characters/Animation/Normal/Walk.Walk"));
	RunSequencePath = FSoftObjectPath(TEXT("/Game/Characters/Animation/Normal/Run.Run"));

		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// The follow camera must not collide with characters: only the world blocks it.
	// The spring arm probes on ECC_Camera, so characters ignore that channel.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;

	// Walking is the default pace; holding Shift switches to running (see StartRunning).
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Jumping is not part of the design: no input is bound and JumpMaxCount stays 0.
	JumpMaxCount = 0;


	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Placeholder character name. The server will overwrite this once accounts and
	// character data exist; it is replicated so every client sees the same value.
	CharacterName = FText::FromString(TEXT("\u89D2\u8272\u540D"));
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

FVector AMyBondageLifeCharacter::GetNameplateLocation() const
{
	if (const USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (MeshComponent->DoesSocketExist(NameplateAttachBone))
		{
			// Follow the actual head bone so the plate tracks animation.
			return MeshComponent->GetSocketLocation(NameplateAttachBone) + FVector(0.0f, 0.0f, NameplateHeadOffset * HeightScale);
		}
	}

	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
	return GetActorLocation() + FVector(0.0f, 0.0f, CapsuleHalfHeight + NameplateHeadOffset * HeightScale);
}

FString AMyBondageLifeCharacter::GetNameplateText() const
{
	// Only the character's own name: the account id is no longer shown.
	return CharacterName.IsEmpty() ? FString(TEXT("\u89D2\u8272\u540D")) : CharacterName.ToString();
}
void AMyBondageLifeCharacter::SetCharacterName(const FText& InCharacterName)
{
	CharacterName = InCharacterName;
}

void AMyBondageLifeCharacter::SetCharacterIdentity(const FText& InCharacterName, int32 InCharacterClass, int64 InCreatedAtUnix, float InHeightScale, int32 InHairIndex)
{
	CharacterName = InCharacterName;
	CharacterClass = FMath::Clamp(InCharacterClass, 1, 4);
	CreatedAtUnix = InCreatedAtUnix;
	HeightScale = FMath::Clamp(InHeightScale, 0.5f, 2.0f);
	HairIndex = InHairIndex > 0 ? InHairIndex : 1;

	ApplyHeightScale();
	ApplyHairStyle();
}

void AMyBondageLifeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyBondageLifeCharacter, CharacterName);
	DOREPLIFETIME(AMyBondageLifeCharacter, CharacterClass);
	DOREPLIFETIME(AMyBondageLifeCharacter, CreatedAtUnix);
	DOREPLIFETIME(AMyBondageLifeCharacter, HeightScale);
	DOREPLIFETIME(AMyBondageLifeCharacter, ReplicatedMeshRelativeLocation);
	DOREPLIFETIME(AMyBondageLifeCharacter, ReplicatedMeshScale);
	DOREPLIFETIME(AMyBondageLifeCharacter, HairIndex);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintPoseActive);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRunning);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintSlots);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintItemIds);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintArm);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintInGag);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintBlind);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintBlindLevel);
	DOREPLIFETIME(AMyBondageLifeCharacter, OutfitSlots);
	DOREPLIFETIME(AMyBondageLifeCharacter, OutfitItemIds);
	DOREPLIFETIME(AMyBondageLifeCharacter, OutfitColorHex);
	DOREPLIFETIME(AMyBondageLifeCharacter, AccountName);
	DOREPLIFETIME(AMyBondageLifeCharacter, SkinId);
	DOREPLIFETIME(AMyBondageLifeCharacter, bSitting);
	DOREPLIFETIME(AMyBondageLifeCharacter, SittingLocation);
	DOREPLIFETIME(AMyBondageLifeCharacter, SittingRotation);
	DOREPLIFETIME(AMyBondageLifeCharacter, SittingOffset);
	DOREPLIFETIME(AMyBondageLifeCharacter, OccupiedSeat);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintColorArray);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintLeg);
	DOREPLIFETIME(AMyBondageLifeCharacter, bCantMove);
	DOREPLIFETIME(AMyBondageLifeCharacter, bInPetMode);
}
void AMyBondageLifeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	// Mouse wheel zoom goes through the plain input component: the wheel is not
	// mapped as an action in the default mapping context.
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AMyBondageLifeCharacter::ZoomIn);
		PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AMyBondageLifeCharacter::ZoomOut);
	}
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping is intentionally not bound: the character cannot jump.

		// Sprint: Shift is not part of the mapping context, so bind the key directly.
		PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AMyBondageLifeCharacter::StartRunning);
		PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AMyBondageLifeCharacter::StopRunning);
		PlayerInputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AMyBondageLifeCharacter::StartRunning);
		PlayerInputComponent->BindKey(EKeys::RightShift, IE_Released, this, &AMyBondageLifeCharacter::StopRunning);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyBondageLifeCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMyBondageLifeCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyBondageLifeCharacter::Look);
	}
	else
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMyBondageLifeCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMyBondageLifeCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMyBondageLifeCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMyBondageLifeCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMyBondageLifeCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMyBondageLifeCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMyBondageLifeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Reference values must be taken before anything is scaled.
	CaptureBasePose();

	// Reference sizes for the body height multiplier: everything is scaled relative
	// to this designed size.
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		BaseCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		BaseCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
	}
	if (const USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		BaseMeshRelativeLocation = MeshComponent->GetRelativeLocation();
		BaseMeshScale = MeshComponent->GetRelativeScale3D();
	}
	ApplyHeightScale();
	ApplyHairStyle();

	// Use the copied AnimBP from now on, then restore the replicated pose state.
	ApplyAnimBlueprintOverride();
	ApplySkin();
	RestraintMontagePath.TryLoad();   // warm the asset so the first toggle does not hitch
	RefreshRestraintPose();
	ApplyRestraintMovementLimits();

	// Remember the designed camera distance: the wheel zoom keeps the range at
	// 0.5x - 1.5x of it.
	if (const USpringArmComponent* Boom = GetCameraBoom())
	{
		DefaultArmLength = Boom->TargetArmLength;
		TargetArmLength = DefaultArmLength;
	}
}

void AMyBondageLifeCharacter::ZoomIn()
{
	if (DefaultArmLength <= 0.0f || !GetCameraBoom())
	{
		return;
	}

	// Closest allowed distance is half of the default.
	TargetArmLength = FMath::Max(TargetArmLength - ZoomStep, DefaultArmLength * 0.5f);
}

void AMyBondageLifeCharacter::ZoomOut()
{
	if (DefaultArmLength <= 0.0f || !GetCameraBoom())
	{
		return;
	}

	// Farthest allowed distance is 1.5x of the default.
	TargetArmLength = FMath::Min(TargetArmLength + ZoomStep, DefaultArmLength * 1.5f);
}

void AMyBondageLifeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USpringArmComponent* Boom = GetCameraBoom();
	if (Boom && TargetArmLength > 0.0f)
	{
		// Smooth so a single notch does not snap the camera.
		Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetArmLength, DeltaSeconds, 8.0f);
	}

	// The AnimBP owns locomotion; only the restraint montage state is tracked here.
	if (bRestraintPoseActive)
	{
		RefreshRestraintPose();
	}

	if (bOutfitBaseCaptured && !bSitting)
	{
		ApplyOutfitCapsule();
	}

	// 坐着时保持坐下偏移（本地与远端客户端都生效，换鞋也不会重置）
	if (bSitting)
	{
		if (USkeletalMeshComponent* SittingMesh = GetMesh())
		{
			// SitOffset 按标准身高标定：局部空间要除以 Actor 缩放，
			// 让世界空间的位移保持不变（身高越高，需要下沉的相对量越小）。
			const float ScaleZ = (GetCapsuleComponent() && BaseCapsuleHalfHeight > 1.0f)
				? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() / BaseCapsuleHalfHeight
				: 1.0f;
			const float TargetZ = OutfitBaseMeshZ + SittingOffset * FMath::Max(BaseCapsuleHalfHeight, 1.0f)
				/ FMath::Max(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 1.0f);
			FVector Location = SittingMesh->GetRelativeLocation();
			if (!FMath::IsNearlyEqual(Location.Z, TargetZ, 0.1f))
			{
				Location.Z = TargetZ;
				SittingMesh->SetRelativeLocation(Location);
			}
		}
	}

	// 移动时停止表情动作。
	if (CurrentEmote)
	{
		if (GetVelocity().SizeSquared2D() > 100.0f || GetPendingMovementInputVector().SizeSquared() > 0.01f)
		{
			StopEmote();
		}
	
		if (UAnimInstance* EmoteAnim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (EmoteAnim->Montage_IsPlaying(CurrentEmote) == false)
			{
				CurrentEmote = nullptr;
			}
		}
	}

	// 坐着时按移动键（WASD）立即起身。
	if (bSitting && GetWorld())
	{
		const APlayerController* OwningPC = Cast<APlayerController>(GetController());
		const bool bMoveKeyDown = OwningPC && (
			OwningPC->IsInputKeyDown(EKeys::W) || OwningPC->IsInputKeyDown(EKeys::A)
			|| OwningPC->IsInputKeyDown(EKeys::S) || OwningPC->IsInputKeyDown(EKeys::D));

		if (bMoveKeyDown)
		{
			const FVector StandLocation = GetActorLocation() + GetActorForwardVector() * 150.0f;
			ServerSetSitting(false, StandLocation, GetActorRotation());
		}
	}

	// 隐藏材质兜底重申（每 3 秒）
	if (RestraintHiddenParts.Num() + OutfitHiddenParts.Num() > 0)
	{
		HideReassertAccumulator += DeltaSeconds;
		if (HideReassertAccumulator >= 3.0f)
		{
			HideReassertAccumulator = 0.0f;
			ApplyBodyPartHiding();
		}
	}
	ApplyBlindEffect(DeltaSeconds);

	// 高潮期间锁定镜头距离，避免被其它逻辑（坐姿/滚轮）改回去。
	if (bClimaxActive && ClimaxLockedArmLength > 0.0f)
	{
		if (USpringArmComponent* LockedBoom = GetCameraBoom())
		{
			LockedBoom->TargetArmLength = ClimaxLockedArmLength;
		}
	}

	if (bCantMove)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->MaxWalkSpeed = 0.0f;
		}
	}
}

void AMyBondageLifeCharacter::OnRep_HeightScale()
{
	ApplyHeightScale();
}

void AMyBondageLifeCharacter::ApplyHeightScale()
{
	// Actor-scale approach: scale the whole pawn uniformly. The capsule, the mesh and
	// every attached restraint scale together, so the feet stay on the capsule bottom
	// by construction and the value replicates with the actor transform (no custom
	// replication, no offset maths).
	// Always on: a blueprint may still carry the old disabled default.
	const float Scale = FMath::Clamp(HeightScale, 0.5f, 2.0f);

	UE_LOG(LogMyBondageLife, Log, TEXT("[Height] apply requested=%.2f current=%.2f"), Scale, GetActorScale3D().Z);

	if (!FMath::IsNearlyEqual(GetActorScale3D().Z, Scale, 0.001f))
	{
		SetActorScale3D(FVector(Scale));
	}

	// 坐着时改身高：重新按新缩放贴合坐点，否则坐姿会错位。
	if (bSitting)
	{
		ApplySitting();
	}
}

void AMyBondageLifeCharacter::OnRep_HairIndex()
{
	ApplyHairStyle();
}

void AMyBondageLifeCharacter::ApplyHairStyle()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	USkeletalMesh* HairMesh = FMyBondageLifeHairCatalog::LoadMesh(HairIndex);
	if (!HairMesh)
	{
		if (HairComponent)
		{
			HairComponent->SetSkeletalMeshAsset(nullptr);
			HairComponent->SetVisibility(false);
		}
		return;
	}

	// Created on demand: only characters that actually wear hair pay for it.
	if (!HairComponent)
	{
		HairComponent = NewObject<USkeletalMeshComponent>(this, TEXT("HairComponent"));
		if (!HairComponent)
		{
			return;
		}

		HairComponent->SetupAttachment(BodyMesh);
		HairComponent->RegisterComponent();
		// 布料模拟依赖组件的碰撞：NoCollision 时头发不会与身体发生碰撞。
		HairComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HairComponent->SetCollisionProfileName(TEXT("CharacterMesh"));
		HairComponent->SetGenerateOverlapEvents(false);
		HairComponent->SetCastShadow(true);
	}

	HairComponent->SetSkeletalMeshAsset(HairMesh);

	// Hair that shares the body skeleton follows the animation through master pose;
	// anything else is parented to the head socket instead.
	const USkeletalMesh* HairAsset = HairComponent->GetSkeletalMeshAsset();
	const bool bSameSkeleton = HairAsset && BodyMesh->GetSkeletalMeshAsset()
		&& HairAsset->GetSkeleton() == BodyMesh->GetSkeletalMeshAsset()->GetSkeleton();

	if (bSameSkeleton)
	{
		HairComponent->SetMasterPoseComponent(BodyMesh);
		HairComponent->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		HairComponent->SetMasterPoseComponent(nullptr);
		HairComponent->AttachToComponent(
			BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("head"));
	}

	HairComponent->SetVisibility(true, true);
	HairComponent->SetHiddenInGame(false, true);

	UE_LOG(LogMyBondageLife, Log,
		TEXT("Hair: style %d -> %s (sameSkeleton=%d, headSocket=%d, extent=%s)"),
		HairIndex, *GetNameSafe(HairAsset), bSameSkeleton ? 1 : 0,
		BodyMesh->DoesSocketExist(TEXT("head")) ? 1 : 0,
		*HairAsset->GetBounds().BoxExtent.ToString());
}

void AMyBondageLifeCharacter::SetRestraintPose(bool bActive)
{
	if (bRestraintPoseActive == bActive)
	{
		return;
	}

	bRestraintPoseActive = bActive;
	RefreshRestraintPose();
	ApplyRestraintMovementLimits();
}

void AMyBondageLifeCharacter::OnRep_RestraintPose()
{
	RefreshRestraintPose();
	ApplyRestraintMovementLimits();
}

void AMyBondageLifeCharacter::ApplyAnimBlueprintOverride()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	// No override configured: keep whatever Anim Class the blueprint set on the mesh.
	if (AnimBlueprintOverridePath.IsNull())
	{
		return;
	}

	UClass* AnimClass = AnimBlueprintOverridePath.TryLoadClass<UAnimInstance>();
	if (!AnimClass)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Animation: '%s' not found."),
			*AnimBlueprintOverridePath.ToString());
		return;
	}

	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	MeshComponent->SetAnimInstanceClass(AnimClass);
	UE_LOG(LogMyBondageLife, Log, TEXT("Animation: using %s."), *AnimClass->GetName());
}

void AMyBondageLifeCharacter::UpdateLocomotionAnimation()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	// The restraint pose owns the mesh while it is active.
	if (bRestraintPoseActive)
	{
		return;
	}

	const float Speed2D = GetVelocity().Size2D();
	const FSoftObjectPath* ChosenPath = &IdleSequencePath;

	if (Speed2D >= 10.0f)
	{
		const float WalkLimit = (WalkSpeed + RunSpeed) * 0.5f;
		ChosenPath = Speed2D < WalkLimit ? &WalkSequencePath : &RunSequencePath;
	}

	UAnimSequence* Sequence = Cast<UAnimSequence>(ChosenPath->TryLoad());
	if (!Sequence || Sequence == CurrentLocomotionSequence)
	{
		return;
	}

	CurrentLocomotionSequence = Sequence;
	MeshComponent->PlayAnimation(Sequence, true);

	UE_LOG(LogMyBondageLife, Verbose, TEXT("Animation: locomotion -> %s (speed %.1f)."),
		*Sequence->GetName(), Speed2D);
}

void AMyBondageLifeCharacter::RefreshRestraintPose()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// The equipped items decide the pose; the debug path is only a fallback.
	// The pose is derived from the replicated item list, never from a stale flag:
	// a desynced bool used to leave remote clients stuck in the old montage.
	if (!ActiveUpAnmi)
	{
		// Stop only the montage we started: the gag slot may be playing on top.
		if (PlayingPoseAnmi && AnimInstance->Montage_IsPlaying(PlayingPoseAnmi))
		{
			AnimInstance->Montage_Stop(0.2f, PlayingPoseAnmi);
		}
		PlayingPoseAnmi = nullptr;
		UE_LOG(LogMyBondageLife, Log, TEXT("Animation: restraint pose off."));
		return;
	}

	UAnimMontage* Montage = ActiveUpAnmi.Get();
	if (!Montage)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Animation: montage not found."));
		return;
	}

	// 换装时同一槽位会替换蒙太奇：先把旧姿势停掉，否则新旧两段会叠加混合。
	if (PlayingPoseAnmi && PlayingPoseAnmi != Montage)
	{
		AnimInstance->Montage_Stop(0.12f, PlayingPoseAnmi);
		PlayingPoseAnmi = nullptr;
	}

	if (!AnimInstance->Montage_IsPlaying(Montage))
	{
		// Played through the AnimBP slot, so locomotion keeps running underneath;
		// bStopAllMontages = false keeps the gag slot playing on top.
		AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
		PlayingPoseAnmi = Montage;
		UE_LOG(LogMyBondageLife, Log, TEXT("Animation: restraint pose on."));
	}
}
void AMyBondageLifeCharacter::StartRunning()
{
	SetRunning(true);
}

void AMyBondageLifeCharacter::StopRunning()
{
	SetRunning(false);
}

void AMyBondageLifeCharacter::SetRunning(bool bNewRunning)
{
	if (bRunning == bNewRunning)
	{
		return;
	}

	bRunning = bNewRunning;
	ApplyMoveSpeed();

	if (!HasAuthority())
	{
		ServerSetRunning(bNewRunning);
	}
}

void AMyBondageLifeCharacter::ServerSetRunning_Implementation(bool bNewRunning)
{
	SetRunning(bNewRunning);
}

void AMyBondageLifeCharacter::OnRep_Running()
{
	ApplyMoveSpeed();
}

void AMyBondageLifeCharacter::ApplyMoveSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Leg restraints make running impossible; CantMove stops the pawn entirely.
	if (bCantMove)
	{
		Movement->MaxWalkSpeed = 0.0f;
		SetRunning(false);
		return;
	}

	if (bRestraintLeg)
	{
		SetRunning(false);
	}

	Movement->MaxWalkSpeed = bRunning ? RunSpeed : WalkSpeed;
}

// ---------------------------------------------------------------------------
// Restraints
// ---------------------------------------------------------------------------

void AMyBondageLifeCharacter::SetRestraints(const TArray<int32>& InSlots, const TArray<FName>& InItems)
{
	RestraintSlots = InSlots;
	RestraintItemIds = InItems;

	ApplyRestraints();
}

void AMyBondageLifeCharacter::OnRep_Restraints()
{
	ApplyRestraints();
}

void AMyBondageLifeCharacter::ApplyRestraints()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	// Rebuild from scratch: the list is small and this keeps the state consistent.
	for (USkeletalMeshComponent* Component : RestraintComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	RestraintComponents.Reset();

	bRestraintArm = false;
	bRestraintInGag = false;
	bRestraintBlind = false;
	RestraintBlindLevel = 0;
	bRestraintLeg = false;
	bCantMove = false;
	bInPetMode = false;
	ActiveUpAnmi = nullptr;
	ActiveGagAnmi = nullptr;

	int32 BestSlot = MAX_int32;
	int32 BestGagSlot = MAX_int32;

	TSet<FName> HiddenParts;

	for (int32 Index = 0; Index < RestraintItemIds.Num(); ++Index)
	{
		const FMyBondageLifeRestraintItem* Item = FMyBondageLifeRestraintCatalog::Find(RestraintItemIds[Index]);
		if (!Item)
		{
			continue;
		}

		// The derived states are a logical OR across every worn item.
		bRestraintArm |= Item->bRestraintArm;
		bRestraintInGag |= Item->bInGag;
		for (const FString& PartName : Item->HideBodyParts)
		{
			HiddenParts.Add(FName(*PartName));
		}

		if (Item->bIsBlind)
		{
			bRestraintBlind = true;
			RestraintBlindLevel = FMath::Max(RestraintBlindLevel, FMath::Clamp(Item->BlindLevel, 1, 3));
		}
		bRestraintLeg |= Item->bRestraintLeg;
		bCantMove |= Item->bCantMove;
		bInPetMode |= Item->bInPetMode;

		const int32 Slot = RestraintSlots.IsValidIndex(Index) ? RestraintSlots[Index] : Item->Slot;

		// Several items may bring a pose: the lowest slot wins (configuration error).
		if (UAnimMontage* Montage = Item->UpAnmi.LoadSynchronous())
		{
			if (Slot < BestSlot)
			{
				ActiveUpAnmi = Montage;
				BestSlot = Slot;
			}
		}

		// Head only montage: independent of UpAnmi, lowest slot wins as well.
		if (UAnimMontage* GagMontage = Item->GagAnmi.LoadSynchronous())
		{
			if (Slot < BestGagSlot)
			{
				ActiveGagAnmi = GagMontage;
				BestGagSlot = Slot;
			}
		}

		if (USkeletalMesh* WornMesh = Item->Mesh.LoadSynchronous())
		{
			USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(this);
			if (!Component)
			{
				continue;
			}

			Component->SetupAttachment(BodyMesh);
			Component->RegisterComponent();
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionProfileName(TEXT("CharacterMesh"));

		// 用实际穿着的槽位做 key（DT 里的 Slot 可能和穿着槽位不一致）
		const int32 WornSlotIndex = OutfitSlots.IsValidIndex(Index) ? OutfitSlots[Index] : Item->Slot;
		OutfitComponents.Add(WornSlotIndex, Component);
			Component->SetGenerateOverlapEvents(false);
			Component->SetSkeletalMeshAsset(WornMesh);

			const bool bSameSkeleton = BodyMesh->GetSkeletalMeshAsset()
				&& WornMesh->GetSkeleton() == BodyMesh->GetSkeletalMeshAsset()->GetSkeleton();

			if (bSameSkeleton)
			{
				Component->SetMasterPoseComponent(BodyMesh);
				Component->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
			else
			{
				Component->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_03"));
			}

			RestraintComponents.Add(Component);
		}
	}

	// Drive the pose from the equipped items: any item with UpAnmi wins, the lowest
	// slot plays first; none means back to the normal pose.
	SetRestraintPose(ActiveUpAnmi != nullptr);

	// The owning client must apply it immediately as well (replication may be a tick late).
	RefreshRestraintPose();
	ApplyRestraintMovementLimits();
	RefreshGagPose();

	UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: %d item(s) applied (arm=%d leg=%d cantmove=%d pet=%d)."),
		RestraintItemIds.Num(), bRestraintArm ? 1 : 0, bRestraintLeg ? 1 : 0, bCantMove ? 1 : 0, bInPetMode ? 1 : 0);

	RestraintHiddenParts = HiddenParts;
	ApplySkin();
	ApplyRestraintColors();
	ApplyBodyPartHiding();
}

FName AMyBondageLifeCharacter::GetGagProfileId() const
{
	// 找第一件"算作堵嘴"的已穿戴道具，用它的 Enum_GagType 决定用哪个混淆档位。
	TSet<FName> HiddenParts;

	for (int32 Index = 0; Index < RestraintItemIds.Num(); ++Index)
	{
		const FMyBondageLifeRestraintItem* Item = FMyBondageLifeRestraintCatalog::Find(RestraintItemIds[Index]);
		if (!Item || !Item->bInGag)
		{
			continue;
		}

		if (Item->bHasGagType)
		{
			const FName Profile = FMyBondageLifeGagTypeMap::ToProfileId(Item->GagType);
			if (!Profile.IsNone())
			{
				return Profile;
			}
		}

		// 旧数据没配 GagType：退回默认口球档位，避免"戴着却还能正常说话"。
		return FMyBondageLifeGagTypeMap::GetDefaultProfileId();
	}

	return NAME_None;
}

void AMyBondageLifeCharacter::RefreshGagPose()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (!ActiveGagAnmi)
	{
		if (PlayingGagAnmi && AnimInstance->Montage_IsPlaying(PlayingGagAnmi))
		{
			AnimInstance->Montage_Stop(0.2f, PlayingGagAnmi);
		}
		PlayingGagAnmi = nullptr;
		return;
	}

	if (PlayingGagAnmi == ActiveGagAnmi && AnimInstance->Montage_IsPlaying(ActiveGagAnmi))
	{
		return;
	}

	if (PlayingGagAnmi && AnimInstance->Montage_IsPlaying(PlayingGagAnmi))
	{
		AnimInstance->Montage_Stop(0.15f, PlayingGagAnmi);
	}

	// The montage carries its own slot (GagSlot), so it layers on top of locomotion
	// and of the UpAnmi pose.
	// bStopAllMontages = false: the body pose may be playing on top of this one.
	AnimInstance->Montage_Play(ActiveGagAnmi, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
	PlayingGagAnmi = ActiveGagAnmi;

	// Hold the pose by looping its first section.
	if (ActiveGagAnmi->CompositeSections.Num() > 0)
	{
		const FName SectionName = ActiveGagAnmi->CompositeSections[0].SectionName;
		AnimInstance->Montage_SetNextSection(SectionName, SectionName, ActiveGagAnmi);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Animation: gag pose on (%s)."), *ActiveGagAnmi->GetName());
}
void AMyBondageLifeCharacter::OnRep_MeshPlacement()
{
	// Applying the replica before the reference exists would poison the base capture.
	if (!bBasePoseCaptured)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent || ReplicatedMeshRelativeLocation.IsZero())
	{
		return;
	}

	MeshComponent->SetRelativeScale3D(ReplicatedMeshScale);
	MeshComponent->SetRelativeLocation(ReplicatedMeshRelativeLocation);
}
void AMyBondageLifeCharacter::CaptureBasePose()
{
	if (bBasePoseCaptured)
	{
		return;
	}

	USkeletalMeshComponent* RefMesh = GetMesh();
	UCapsuleComponent* RefCapsule = GetCapsuleComponent();
	if (!RefMesh || !RefCapsule)
	{
		return;
	}

	BaseCapsuleHalfHeight = RefCapsule->GetUnscaledCapsuleHalfHeight();
	BaseCapsuleRadius = RefCapsule->GetUnscaledCapsuleRadius();
	BaseMeshRelativeLocation = RefMesh->GetRelativeLocation();
	BaseMeshScale = RefMesh->GetRelativeScale3D();
	bBasePoseCaptured = true;
}
void AMyBondageLifeCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Runs after the blueprint's component values are applied but before any scaling,
	// so this is the only place where the reference is guaranteed to be correct.
	CaptureBasePose();
}

void AMyBondageLifeCharacter::ApplyRestraintMovementLimits()
{
	ApplyMoveSpeed();

	// CantMove also stops the camera/character from turning: drop look input on the owner.
	if (AMyBondageLifePlayerController* PC = Cast<AMyBondageLifePlayerController>(GetController()))
	{
		PC->SetRestraintMovementBlocked(bCantMove);
	}
}

void AMyBondageLifeCharacter::ApplyBlindEffect(float InDeltaTime)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UCameraComponent* Camera = FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}

	const float Target = static_cast<float>(GetBlindLevel());
	BlindVisualLevel = FMath::FInterpTo(BlindVisualLevel, Target, InDeltaTime, 5.0f);
	LibidoVisualLevel = FMath::FInterpTo(LibidoVisualLevel, static_cast<float>(LibidoScreenLevel), InDeltaTime, 4.0f);

	// 状态没变化就不每帧写后处理（省 CPU）。
	if (FMath::IsNearlyEqual(BlindVisualLevel, LastAppliedBlind) &&
		FMath::IsNearlyEqual(LibidoVisualLevel, LastAppliedLibido))
	{
		return;
	}
	LastAppliedBlind = BlindVisualLevel;
	LastAppliedLibido = LibidoVisualLevel;

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	const bool bActive = BlindVisualLevel > 0.01f || LibidoVisualLevel > 0.01f;
	const bool bBlind  = BlindVisualLevel > 0.01f;
	const bool bLibido = LibidoVisualLevel > 0.01f;

	// 蒙眼与性欲是两套效果，叠在同一台相机上：
	//  · 暗角颜色：蒙眼优先保持黑色（不会因为性欲变粉而变亮），只有单纯性欲时才用粉色；
	//  · 性欲的粉色主要通过 SceneColorTint + 额外暗角强度体现，两者可同时可见。
	PP.bOverride_VignetteIntensity = bActive;
	PP.VignetteIntensity = 2.4f * BlindVisualLevel + 2.2f * LibidoVisualLevel;

	PP.bOverride_VignetteColor = bActive;
	PP.VignetteColor = bBlind ? FLinearColor(0.0f, 0.0f, 0.0f) : FLinearColor(1.0f, 0.68f, 0.84f);

	PP.bOverride_AutoExposureBias = bActive;
	PP.AutoExposureBias = -0.55f * BlindVisualLevel;

	// 模糊：性欲比蒙眼更明显地拉近焦平面并压低 F 值。
	PP.bOverride_DepthOfFieldFstop = bActive;
	PP.DepthOfFieldFstop = FMath::Max(0.45f, 2.4f - 0.5f * BlindVisualLevel - 0.9f * LibidoVisualLevel);

	PP.bOverride_DepthOfFieldFocalDistance = bActive;
	PP.DepthOfFieldFocalDistance = FMath::Max(140.0f, 320.0f - 90.0f * LibidoVisualLevel);

	PP.bOverride_DepthOfFieldSensorWidth = bActive;
	PP.DepthOfFieldSensorWidth = 24.0f;

	// 性欲的粉色染色：越强越明显（不影响蒙眼的黑色暗角）。
	PP.bOverride_SceneColorTint = bLibido;
	PP.SceneColorTint = FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor(1.0f, 0.84f, 0.93f), FMath::Clamp(LibidoVisualLevel * 0.5f, 0.0f, 1.0f));

}
void AMyBondageLifeCharacter::ApplyBodyPartHiding()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	// 透明材质（工程里创建：/Game/Effects/M_HiddenBodyPart）
	if (!HiddenBodyMaterial)
	{
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/Effects/M_HiddenBodyPart.M_HiddenBodyPart"));
		if (BaseMaterial)
		{
			HiddenBodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		}
	}

	// 先恢复所有已替换的槽位，再按当前生效的集合重新隐藏。
	for (const TPair<FName, TObjectPtr<UMaterialInterface>>& Pair : OriginalBodyMaterials)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Pair.Key);
		if (MaterialIndex != INDEX_NONE && Pair.Value)
		{
			// 该槽位已被皮肤接管时，还原成皮肤材质而不是旧材质。
			if (const TObjectPtr<UMaterialInstanceDynamic>* SkinMaterial = SkinBodyMaterials.Find(Pair.Key))
			{
				MeshComponent->SetMaterial(MaterialIndex, *SkinMaterial);
			}
			else
			{
				MeshComponent->SetMaterial(MaterialIndex, Pair.Value);
			}
		}
	}
	OriginalBodyMaterials.Reset();

	if (!HiddenBodyMaterial)
	{
		return;
	}

	TSet<FName> AllParts = RestraintHiddenParts;
	AllParts.Append(OutfitHiddenParts);

	for (const FName& PartName : AllParts)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(PartName);
		if (MaterialIndex == INDEX_NONE)
		{
			continue;
		}

		OriginalBodyMaterials.Add(PartName, MeshComponent->GetMaterial(MaterialIndex));
		MeshComponent->SetMaterial(MaterialIndex, HiddenBodyMaterial);

		UE_LOG(LogMyBondageLife, Verbose, TEXT("Hide: %s -> material %d"), *PartName.ToString(), MaterialIndex);
	}
}

// ---------------------------------------------------------------------------
// Outfits
// ---------------------------------------------------------------------------

void AMyBondageLifeCharacter::SetOutfits(const TArray<int32>& InSlots, const TArray<FName>& InItems)
{
	OutfitSlots = InSlots;
	OutfitItemIds = InItems;
	ApplyOutfits();
}

void AMyBondageLifeCharacter::OnRep_Outfits()
{
	ApplyOutfits();
}

void AMyBondageLifeCharacter::ApplyOutfits()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	// 现有衣物组件（槽位 -> 组件）
	static const FName OutfitComponentTag(TEXT("OutfitPart"));

	TArray<USkeletalMeshComponent*> Existing;
	GetComponents<USkeletalMeshComponent>(Existing);
	for (USkeletalMeshComponent* Component : Existing)
	{
		if (Component && Component->ComponentHasTag(OutfitComponentTag))
		{
			Component->DestroyComponent();
		}
	}

	float FeetOffset = 0.0f;
	OutfitHiddenParts.Reset();

	for (int32 Index = 0; Index < OutfitItemIds.Num(); ++Index)
	{
		const FMyBondageLifeOutfitItem* Item = FMyBondageLifeOutfitCatalog::Find(OutfitItemIds[Index]);
		if (!Item)
		{
			continue;
		}

		USkeletalMesh* MeshAsset = Item->Mesh.LoadSynchronous();
		if (!MeshAsset)
		{
			continue;
		}

		FeetOffset += Item->FeetOffset;

		for (const FString& HidePart : Item->HideBodyParts)
		{
			OutfitHiddenParts.Add(FName(*HidePart));
		}

		USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(this);
		if (!Component)
		{
			continue;
		}

		Component->ComponentTags.Add(OutfitComponentTag);
		Component->SetSkeletalMeshAsset(MeshAsset);
		Component->SetupAttachment(BodyMesh);
		Component->RegisterComponent();
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionProfileName(TEXT("CharacterMesh"));

		// 用实际穿着的槽位做 key（DT 里的 Slot 可能和穿着槽位不一致）
		const int32 WornSlotIndex = OutfitSlots.IsValidIndex(Index) ? OutfitSlots[Index] : Item->Slot;
		OutfitComponents.Add(WornSlotIndex, Component);
		Component->SetGenerateOverlapEvents(false);

		// 同骨骼：跟随身体动画；否则挂到同名 socket。
		const bool bSameSkeleton = MeshAsset->GetSkeleton() == BodyMesh->GetSkeletalMeshAsset()->GetSkeleton();
		if (bSameSkeleton)
		{
			Component->SetMasterPoseComponent(BodyMesh);
		}
		else
		{
			Component->AttachToComponent(BodyMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	OutfitFeetOffset = FeetOffset;
	ApplyOutfitCapsule();
	ApplySkin();
	ApplyOutfitColors();
	ApplyBodyPartHiding();
}
void AMyBondageLifeCharacter::ServerSetAccountName_Implementation(const FString& InAccount)
{
	AccountName = InAccount;
}
void AMyBondageLifeCharacter::ApplyOutfitCapsule()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!Capsule || !BodyMesh)
	{
		return;
	}

	if (!bOutfitBaseCaptured)
	{
		OutfitBaseHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		OutfitBaseMeshZ = BodyMesh->GetRelativeLocation().Z;
		bOutfitBaseCaptured = true;
	}

	const float NewHalfHeight = OutfitBaseHalfHeight + OutfitFeetOffset;
	Capsule->SetCapsuleSize(Capsule->GetUnscaledCapsuleRadius(), NewHalfHeight, true);
	BodyMesh->SetRelativeLocation(FVector(
		BodyMesh->GetRelativeLocation().X,
		BodyMesh->GetRelativeLocation().Y,
		OutfitBaseMeshZ));
}

void AMyBondageLifeCharacter::OnRep_Skin()
{
	ApplySkin();
}

void AMyBondageLifeCharacter::ApplySkin()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	const FMyBondageLifeSkinItem* Skin = FMyBondageLifeSkinCatalog::Find(SkinId);
	UMaterialInterface* BodyMaterial = Skin ? Skin->BodyMaterial.LoadSynchronous() : nullptr;
	UMaterialInterface* YatoeMaterial = Skin ? Skin->YatoeMaterial.LoadSynchronous() : nullptr;

	SkinBodyMaterials.Reset();

	static const FName BodySlots[] = { TEXT("upbody"), TEXT("leg"), TEXT("hand"), TEXT("feet") };

	// 身体网格 + 所有挂载网格（头发/衣物/拘束）里同名的材质槽一起换。
	TArray<USkeletalMeshComponent*> Components;
	GetComponents<USkeletalMeshComponent>(Components);

	for (USkeletalMeshComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (BodyMaterial)
		{
			for (const FName& SlotName : BodySlots)
			{
				const int32 Index = Component->GetMaterialIndex(SlotName);
				if (Index == INDEX_NONE)
				{
					continue;
				}

				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BodyMaterial, this);
				if (Mid)
				{
					if (Component == BodyMesh)
					{
						SkinBodyMaterials.Add(SlotName, Mid);
					}
					Component->SetMaterial(Index, Mid);
				}
			}
		}

		if (YatoeMaterial)
		{
			const int32 YatoeIndex = Component->GetMaterialIndex(TEXT("yatoe"));
			if (YatoeIndex != INDEX_NONE)
			{
				if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(YatoeMaterial, this))
				{
					if (Component == BodyMesh)
					{
						SkinBodyMaterials.Add(TEXT("yatoe"), Mid);
					}
					Component->SetMaterial(YatoeIndex, Mid);
				}
			}
		}
	}

	// 隐藏材质优先级最高：皮肤铺完之后再盖透明材质（只作用于身体）。
	ApplyBodyPartHiding();
}
void AMyBondageLifeCharacter::ServerSetSkin_Implementation(int32 InSkinId)
{
	SkinId = FMath::Clamp(InSkinId, 1, 64);
	ApplySkin();
}

void AMyBondageLifeCharacter::SetOutfitColors(const TArray<FString>& InColors)
{
	OutfitColorHex = InColors;
	ApplyOutfitColors();
}

void AMyBondageLifeCharacter::ApplyOutfitColors()
{
	for (const TPair<int32, TObjectPtr<USkeletalMeshComponent>>& Pair : OutfitComponents)
	{
		USkeletalMeshComponent* Component = Pair.Value;
		if (!Component)
		{
			continue;
		}

		for (int32 Channel = 1; Channel <= 3; ++Channel)
		{
			const int32 ColorIndex = Pair.Key * 3 + (Channel - 1);
			if (!OutfitColorHex.IsValidIndex(ColorIndex))
			{
				continue;
			}

			const FString Hex = OutfitColorHex[ColorIndex];
			if (Hex.IsEmpty())
			{
				continue;
			}

			const FColor Parsed = FColor::FromHex(Hex);
			const FLinearColor Color = FLinearColor::FromSRGBColor(Parsed);
			const FName ParameterName(*FString::Printf(TEXT("Color%d"), Channel));

			for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
			{
				UMaterialInterface* Material = Component->GetMaterial(MaterialIndex);
				if (!Material)
				{
					continue;
				}

				// Only tint materials that actually expose ColorN.
				FLinearColor DummyColor;
				if (!Material->GetVectorParameterValue(FMaterialParameterInfo(ParameterName), DummyColor))
				{
					continue;
				}

				UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
				if (!Mid)
				{
					Mid = UMaterialInstanceDynamic::Create(Material, this);
					if (!Mid)
					{
						continue;
					}
					Component->SetMaterial(MaterialIndex, Mid);
				}
				Mid->SetVectorParameterValue(ParameterName, Color);
			}
		}
	}
}

void AMyBondageLifeCharacter::SetOutfitColorChannel(int32 SlotIndex, int32 Channel, const FString& HexColor)
{
	const int32 Index = SlotIndex * 3 + (Channel - 1);
	if (Index < 0)
	{
		return;
	}

	while (OutfitColorHex.Num() <= Index)
	{
		OutfitColorHex.Add(FString());
	}

	OutfitColorHex[Index] = HexColor;
	ApplyOutfitColors();
}
// ---------------------------------------------------------------------------
// Sitting
// ---------------------------------------------------------------------------

void AMyBondageLifeCharacter::ServerSetSitting_Implementation(bool bSit, FVector SeatLocation, FRotator SeatRotation, float SeatOffset, AActor* SeatActor)
{
	bSitting = bSit;
	SittingLocation = SeatLocation;
	SittingRotation = SeatRotation;
	SittingOffset = SeatOffset;
	OccupiedSeat = bSit ? SeatActor : nullptr;
	ApplySitting();
}

void AMyBondageLifeCharacter::OnRep_Sitting()
{
	ApplySitting();
}

void AMyBondageLifeCharacter::ApplySitting()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		// AnimBP 里的 BOOL 变量 InSit
		if (FBoolProperty* Property = CastField<FBoolProperty>(
			AnimInstance->GetClass()->FindPropertyByName(TEXT("InSit"))))
		{
			Property->SetPropertyValue_InContainer(AnimInstance, bSitting);
		}
	}

	if (bSitting)
	{
		// 坐着时关掉弹簧臂碰撞并把镜头拉近，避免摄像机卡进身体/椅子。
		if (USpringArmComponent* SittingBoom = GetCameraBoom())
		{
			SittingBoom->bDoCollisionTest = false;
			SittingBoom->TargetArmLength = 220.0f;
		}

		// 坐着时忽略高跟鞋 FeetOffset：先把胶囊恢复成基础半高，
		// 否则鞋子抬高会把坐姿整体顶起来。
		if (UCapsuleComponent* SitCapsule = GetCapsuleComponent())
		{
			SitCapsule->SetCapsuleSize(SitCapsule->GetUnscaledCapsuleRadius(),
				OutfitBaseHalfHeight, true);
		}

		// 1) 站到坐点上：胶囊底部贴合 SitPoint（身高缩放要一起算进去）
		const float ActorScaleZ = GetActorScale3D().Z;
		const float CapsuleHalfHeight = OutfitBaseHalfHeight * ActorScaleZ;
		SetActorLocation(SittingLocation + FVector(0.0f, 0.0f, CapsuleHalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
		SetActorRotation(SittingRotation, ETeleportType::TeleportPhysics);

		// 2) 动画（InSit 已在上面设置）
		// 3) 偏移：SitOffset 正数上抬、负数下沉
		SittingMeshDrop = -SittingOffset;

		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}

		// 坐下时把骨骼往下压到坐垫上（胶囊不动机器人就不会悬空）。
		if (USkeletalMeshComponent* BodyMesh = GetMesh())
		{
			if (!bOutfitBaseCaptured)
			{
				OutfitBaseMeshZ = BodyMesh->GetRelativeLocation().Z;
				bOutfitBaseCaptured = true;
			}

			SittingMeshDrop = -SittingOffset * FMath::Max(BaseCapsuleHalfHeight, 1.0f)
				/ FMath::Max(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 1.0f);

			UE_LOG(LogMyBondageLife, Log, TEXT("Sit: capsuleHalf=%.1f offset=%.1f drop=%.1f"),
				GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), SittingOffset, SittingMeshDrop);
			BodyMesh->SetRelativeLocation(FVector(
				BodyMesh->GetRelativeLocation().X,
				BodyMesh->GetRelativeLocation().Y,
				OutfitBaseMeshZ - SittingMeshDrop));
		}
	}
	else
	{
		if (USkeletalMeshComponent* BodyMesh = GetMesh())
		{
			BodyMesh->SetRelativeLocation(FVector(
				BodyMesh->GetRelativeLocation().X,
				BodyMesh->GetRelativeLocation().Y,
				OutfitBaseMeshZ));
		}
		SittingMeshDrop = 0.0f;

		ApplyOutfitCapsule();

		if (USpringArmComponent* StandingBoom = GetCameraBoom())
		{
			StandingBoom->bDoCollisionTest = true;
		}

		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
}

void AMyBondageLifeCharacter::SetRestraintColorChannel(int32 Slot, int32 Channel, const FString& HexColor)
{
	RestraintColorHex.FindOrAdd(Slot).Add(Channel, HexColor);
	ApplyRestraintColors();
}

void AMyBondageLifeCharacter::ApplyRestraintColors()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	// 拘束网格 = 除身体/头发/衣物之外的骨骼网格组件
	TArray<USkeletalMeshComponent*> Components;
	GetComponents<USkeletalMeshComponent>(Components);

	for (USkeletalMeshComponent* Component : Components)
	{
		if (!Component || Component == BodyMesh || Component->ComponentHasTag(TEXT("OutfitPart")))
		{
			continue;
		}

		for (const TPair<int32, TMap<int32, FString>>& SlotPair : RestraintColorHex)
		{
			for (const TPair<int32, FString>& ChannelPair : SlotPair.Value)
			{
				const FString& Hex = ChannelPair.Value;
				if (Hex.Len() != 7)
				{
					continue;
				}

				const FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
				const FName ParameterName(*FString::Printf(TEXT("Color%d"), ChannelPair.Key));

				for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
				{
					UMaterialInterface* Material = Component->GetMaterial(MaterialIndex);
					if (!Material)
					{
						continue;
					}

					FLinearColor Dummy;
					if (!Material->GetVectorParameterValue(FMaterialParameterInfo(ParameterName), Dummy))
					{
						continue;
					}

					UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
					if (!Mid)
					{
						Mid = UMaterialInstanceDynamic::Create(Material, this);
						if (!Mid)
						{
							continue;
						}
						Component->SetMaterial(MaterialIndex, Mid);
					}
					Mid->SetVectorParameterValue(ParameterName, Color);
				}
			}
		}
	}
}
void AMyBondageLifeCharacter::SetRestraintColors(const TArray<FString>& Colors)
{
	RestraintColorArray = Colors;
	RestraintColorHex.Reset();

	for (int32 Index = 0; Index < Colors.Num(); ++Index)
	{
		if (Colors[Index].IsEmpty())
		{
			continue;
		}
		const int32 SlotIndex = Index / 3;
		const int32 Channel = (Index % 3) + 1;
		RestraintColorHex.FindOrAdd(SlotIndex).Add(Channel, Colors[Index]);
	}

	ApplyRestraintColors();
}

void AMyBondageLifeCharacter::OnRep_RestraintColors()
{
	SetRestraintColors(RestraintColorArray);
}
void AMyBondageLifeCharacter::PlayEmote(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	// 同一时间只播放一个表情：先停掉当前。
	if (CurrentEmote && AnimInstance->Montage_IsPlaying(CurrentEmote))
	{
		AnimInstance->Montage_Stop(0.15f, CurrentEmote);
	}

	AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
	CurrentEmote = Montage;
}

void AMyBondageLifeCharacter::StopEmote()
{
	if (!CurrentEmote)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AnimInstance->Montage_IsPlaying(CurrentEmote))
		{
			AnimInstance->Montage_Stop(0.2f, CurrentEmote);
		}
	}
	CurrentEmote = nullptr;
}
void AMyBondageLifeCharacter::ServerPlayEmote_Implementation(const FString& MontagePath)
{
	MulticastPlayEmote(MontagePath);
}

void AMyBondageLifeCharacter::MulticastPlayEmote_Implementation(const FString& MontagePath)
{
	if (MontagePath.IsEmpty())
	{
		return;
	}

	if (UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontagePath))
	{
		PlayEmote(Montage);
	}
}
void AMyBondageLifeCharacter::ServerAnnounceSystemMessage_Implementation(const FString& Text)
{
	MulticastSystemMessage(Text);
}

void AMyBondageLifeCharacter::MulticastSystemMessage_Implementation(const FString& Text)
{
	if (AMyBondageLifePlayerController* ChatController = Cast<AMyBondageLifePlayerController>(GetController()))
	{
		ChatController->ShowSystemChatMessage(Text);
	}
	else if (AMyBondageLifePlayerController* LocalController = Cast<AMyBondageLifePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		LocalController->ShowSystemChatMessage(Text);
	}
}
void AMyBondageLifeCharacter::SetLibidoScreenLevel(int32 InLevel)
{
	LibidoScreenLevel = FMath::Clamp(InLevel, 0, 2);
}

void AMyBondageLifeCharacter::BeginClimax()
{
	if (bClimaxActive)
	{
		return;
	}
	bClimaxActive = true;

	// 动画由服务器广播，所有客户端（含自己）都会设置 OnSex。
	ServerSetClimax(true);

	// 镜头临时拉近
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		if (ClimaxSavedArmLength < 0.0f)
		{
			ClimaxSavedArmLength = Boom->TargetArmLength;
		}

		// 无论当前镜头多远，都固定到一个较近的距离。
		const float BaseArm = DefaultArmLength > 0.0f ? DefaultArmLength : 300.0f;
		ClimaxLockedArmLength = FMath::Clamp(BaseArm * 0.45f, 120.0f, 220.0f);
		Boom->TargetArmLength = ClimaxLockedArmLength;
	}

	// 音效由 MulticastSetClimax 在所有客户端播放（见下）。
}

void AMyBondageLifeCharacter::EndClimax()
{
	if (!bClimaxActive)
	{
		return;
	}
	bClimaxActive = false;

	ServerSetClimax(false);

	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		if (ClimaxSavedArmLength >= 0.0f)
		{
			Boom->TargetArmLength = ClimaxSavedArmLength;
			ClimaxSavedArmLength = -1.0f;
			ClimaxLockedArmLength = -1.0f;
		}
	}
}

void AMyBondageLifeCharacter::ApplyClimaxAnim(bool bActive)
{
	if (const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (FBoolProperty* SexProperty = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("OnSex")))
		{
			SexProperty->SetPropertyValue_InContainer(const_cast<UAnimInstance*>(AnimInstance), bActive);
		}
	}
}

void AMyBondageLifeCharacter::ServerSetClimax_Implementation(bool bActive)
{
	// 服务器把状态广播给所有客户端（包括发起者），让所有人都播放高潮动画。
	MulticastSetClimax(bActive);
}

void AMyBondageLifeCharacter::MulticastSetClimax_Implementation(bool bActive)
{
	ApplyClimaxAnim(bActive);

	if (bActive)
	{
		// 3D 播放：房间内所有客户端都能听到。
		if (USoundBase* ClimaxSound = LoadObject<USoundBase>(nullptr,
			TEXT("/Game/Characters/Sound/Sex/Sound_Climax_Cue.Sound_Climax_Cue")))
		{
			UGameplayStatics::PlaySoundAtLocation(this, ClimaxSound, GetActorLocation());
		}
	}
}

// ---------------------------------------------------------------------------
// 拘束穿戴流程
// ---------------------------------------------------------------------------

void AMyBondageLifeCharacter::PlayBondageMontage()
{
	static const TCHAR* MontagePath =
		TEXT("/Game/Characters/MainCharc/FinalYab/Animation/MainCharcF_Bondage_Montage.MainCharcF_Bondage_Montage");

	if (UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, MontagePath))
	{
		PlayAnimMontage(Montage);
	}

	if (USoundBase* Cue = LoadObject<USoundBase>(nullptr,
		TEXT("/Game/Characters/Sound/Sex/Bondage_Cue.Bondage_Cue")))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Cue, GetActorLocation());
	}
}

void AMyBondageLifeCharacter::ServerStartBondage_Implementation(AMyBondageLifeCharacter* Target, float Duration, bool bSelfTarget, int32 SlotIndex, FName ItemId)
{
	if (!Target || bBondageInProgress)
	{
		return;
	}

	bBondageInProgress = true;
	BondageBinder = this;
	BondageTarget = Target;
	BondageSlot = SlotIndex;
	BondageItem = ItemId;
	Target->bBondageInProgress = true;
	Target->BondageBinder = this;

	MulticastBondageStart(Duration, bSelfTarget, SlotIndex, ItemId);

	GetWorldTimerManager().SetTimer(BondageTimer, [this]()
	{
		if (bBondageInProgress)
		{
			bBondageInProgress = false;
			if (AMyBondageLifeCharacter* TargetChar = BondageTarget.Get())
			{
				TargetChar->bBondageInProgress = false;
			}
			MulticastBondageEnd(true, false);
		}
	}, FMath::Max(0.1f, Duration), false);
}

void AMyBondageLifeCharacter::MulticastBondageStart_Implementation(float Duration, bool bSelfTarget, int32 SlotIndex, FName ItemId)
{
	PlayBondageMontage();

	if (AMyBondageLifePlayerController* PC = Cast<AMyBondageLifePlayerController>(GetController()))
	{
		PC->ClientBondageStarted(Duration, bSelfTarget, SlotIndex, ItemId);
	}
}

void AMyBondageLifeCharacter::MulticastBondageEnd_Implementation(bool bSuccess, bool bResisted)
{
	if (AMyBondageLifePlayerController* PC = Cast<AMyBondageLifePlayerController>(GetController()))
	{
		PC->ClientBondageEnded(bSuccess, bResisted);
	}
}

void AMyBondageLifeCharacter::ServerCancelBondage_Implementation()
{
	if (!bBondageInProgress)
	{
		return;
	}

	const bool bResisted = (BondageBinder != this);
	bBondageInProgress = false;
	if (AMyBondageLifeCharacter* TargetChar = BondageTarget.Get())
	{
		TargetChar->bBondageInProgress = false;
	}

	GetWorldTimerManager().ClearTimer(BondageTimer);
	MulticastBondageEnd(false, bResisted);
}