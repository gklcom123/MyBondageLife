// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeCharacter.h"

#include "Core/MyBondageLifeHairCatalog.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MyBondageLife.h"
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

	// Animation assets used by the temporary restraint pose. The AnimBP is a copy of
	// ABP_Unarmed so the pose graph can be extended without touching the original.
	AnimBlueprintOverridePath = FSoftClassPath(
		TEXT("/Game/Characters/MainCharc/MainCharc_final_Skeleton_AnimBlueprint.MainCharc_final_Skeleton_AnimBlueprint_C"));
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
	DOREPLIFETIME(AMyBondageLifeCharacter, HairIndex);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintPoseActive);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRunning);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintSlots);
	DOREPLIFETIME(AMyBondageLifeCharacter, RestraintItemIds);
	DOREPLIFETIME(AMyBondageLifeCharacter, bRestraintArm);
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
	RestraintMontagePath.TryLoad();   // warm the asset so the first toggle does not hitch
	RefreshRestraintPose();

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
}

void AMyBondageLifeCharacter::OnRep_HeightScale()
{
	ApplyHeightScale();
}

void AMyBondageLifeCharacter::ApplyHeightScale()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!MeshComponent || !Capsule)
	{
		return;
	}

	// Capture the designed size once, before anything is scaled.
	if (BaseCapsuleHalfHeight <= 0.0f)
	{
		BaseCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		BaseCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
		BaseMeshRelativeLocation = MeshComponent->GetRelativeLocation();
		BaseMeshScale = MeshComponent->GetRelativeScale3D();
	}

	const float Scale = FMath::Clamp(HeightScale, 0.5f, 2.0f);

	// Mesh and capsule scale together so the collision shape keeps matching the body.
	MeshComponent->SetRelativeScale3D(BaseMeshScale * Scale);

	const float NewHalfHeight = BaseCapsuleHalfHeight * Scale;
	Capsule->SetCapsuleSize(BaseCapsuleRadius * Scale, NewHalfHeight, true);

	// Keep the feet on the capsule bottom: the mesh origin sits at the feet, so the
	// relative Z only follows the capsule's change.
	MeshComponent->SetRelativeLocation(FVector(
		BaseMeshRelativeLocation.X,
		BaseMeshRelativeLocation.Y,
		BaseMeshRelativeLocation.Z - (NewHalfHeight - BaseCapsuleHalfHeight)));
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
		HairComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
}

void AMyBondageLifeCharacter::OnRep_RestraintPose()
{
	RefreshRestraintPose();
}

void AMyBondageLifeCharacter::ApplyAnimBlueprintOverride()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	// The project authored an AnimBP for its own skeleton; use it for everything.
	UClass* AnimClass = AnimBlueprintOverridePath.TryLoadClass<UAnimInstance>();
	if (!AnimClass)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Animation: '%s' not found."),
			*AnimBlueprintOverridePath.ToString());
		return;
	}

	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	if (MeshComponent->GetAnimInstance() && MeshComponent->GetAnimInstance()->GetClass() == AnimClass)
	{
		return;
	}

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
	UAnimMontage* Montage = ActiveUpAnmi ? ActiveUpAnmi.Get() : Cast<UAnimMontage>(RestraintMontagePath.TryLoad());
	if (!Montage)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Animation: montage not found."));
		return;
	}

	if (bRestraintPoseActive)
	{
		if (!AnimInstance->Montage_IsPlaying(Montage))
		{
			// Played through the AnimBP slot, so locomotion keeps running underneath.
			AnimInstance->Montage_Play(Montage, 1.0f);
			UE_LOG(LogMyBondageLife, Log, TEXT("Animation: restraint pose on."));
		}
	}
	else
	{
		// The owning client may have started a different montage than the one the
		// data table points at now, so stop everything instead of one asset.
		AnimInstance->Montage_Stop(0.2f, nullptr);
		UE_LOG(LogMyBondageLife, Log, TEXT("Animation: restraint pose off."));
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
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bRunning ? RunSpeed : WalkSpeed;
	}
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
	bRestraintLeg = false;
	bCantMove = false;
	bInPetMode = false;
	ActiveUpAnmi = nullptr;

	int32 BestSlot = MAX_int32;

	for (int32 Index = 0; Index < RestraintItemIds.Num(); ++Index)
	{
		const FMyBondageLifeRestraintItem* Item = FMyBondageLifeRestraintCatalog::Find(RestraintItemIds[Index]);
		if (!Item)
		{
			continue;
		}

		// The derived states are a logical OR across every worn item.
		bRestraintArm |= Item->bRestraintArm;
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

		if (USkeletalMesh* WornMesh = Item->Mesh.LoadSynchronous())
		{
			USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(this);
			if (!Component)
			{
				continue;
			}

			Component->SetupAttachment(BodyMesh);
			Component->RegisterComponent();
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: %d item(s) applied (arm=%d leg=%d cantmove=%d pet=%d)."),
		RestraintItemIds.Num(), bRestraintArm ? 1 : 0, bRestraintLeg ? 1 : 0, bCantMove ? 1 : 0, bInPetMode ? 1 : 0);
}
