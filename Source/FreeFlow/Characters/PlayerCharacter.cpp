// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Movement/FreeFlowCharacterMovementComponent.h"
#include "Characters/CapeAnchorComponent.h"
#include "Camera/FreeFlowSpringArmComponent.h"
#include "TimerManager.h"
#include "Movement/FreeFlowLocomotionComponent.h"
#include "Animation/FreeFlowAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<UFreeFlowSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	CapeAnchor = CreateDefaultSubobject<UCapeAnchorComponent>(TEXT("CapeAnchor"));

	Locomotion = CreateDefaultSubobject<UFreeFlowLocomotionComponent>(TEXT("Locomotion"));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	Locomotion->OnStateChanged.AddDynamic(this, &APlayerCharacter::HandleLocomotionStateChanged);
}

void APlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::SprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::SprintCompleted);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::CrouchStarted);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::CrouchCompleted);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopAttacking);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayerCharacter::SprintStarted()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bDoubleTap = LastSprintPressTime >= 0.0 && Now - LastSprintPressTime <= RollDoubleTapWindow;
	LastSprintPressTime = bDoubleTap ? -1.0 : Now;

	Locomotion->SetWantsToSprint(true);

	if (bDoubleTap)
	{
		Locomotion->TryRoll();
	}
}

void APlayerCharacter::SprintCompleted()
{
	Locomotion->SetWantsToSprint(false);
}

void APlayerCharacter::CrouchStarted()
{
	Locomotion->SetWantsToCrouch(true);
}

void APlayerCharacter::CrouchCompleted()
{
	Locomotion->SetWantsToCrouch(false);
}

void APlayerCharacter::Attack()  
{  
	UE_LOG(LogTemp, Warning, TEXT("Attack Started"));
	if (!CanStartPunch() || !Locomotion->CanAttack())
	{
		return;
	}

   bPressedAttack = true;  
   TArray<AActor*> Enemies = FindEnemiesWithinRange();
   AActor* TargetEnemy = FindBestEnemyToAttack(Enemies);

	if (TargetEnemy == nullptr){
		UE_LOG(LogTemp, Warning, TEXT("No valid enemy found. Attack cancelled."));
		return;
	}
   UE_LOG(LogTemp, Warning, TEXT("Target Enemy: %s"), *TargetEnemy->GetName());
   EnterCombat();
	
   FVector TargetEnemyLocation = TargetEnemy->GetActorLocation();
   PlayPunch();
   FRotator TargetEnemyRotation = TargetEnemy->GetActorRotation();
   FVector EnemyForwardVector = UKismetMathLibrary::GetForwardVector(TargetEnemyRotation) * 35.f;

   FVector TargetLocation = EnemyForwardVector + TargetEnemyLocation;

   const FRotator FaceEnemyRotation(0.f, (TargetEnemyLocation - GetActorLocation()).Rotation().Yaw, 0.f);

   UKismetSystemLibrary::MoveComponentTo(  
       Cast<USceneComponent>(GetCapsuleComponent()),  
	   TargetLocation,
	   FaceEnemyRotation,
       false,
       false, 
       0.6f,
	   true,
       EMoveComponentAction::Move,
	   MakeAttackMoveLatentInfo());
   UE_LOG(LogTemp, Warning, TEXT("Character should've moved"));
}

bool APlayerCharacter::PlayPunch()
{
	UAnimInstance* AnimInstance = GetBodyAnimInstance();
	if (!AnimInstance || PunchAnimations.IsEmpty())
	{
		return false;
	}

	int32 Index = FMath::RandRange(0, PunchAnimations.Num() - 1);
	if (PunchAnimations.Num() > 1 && Index == LastPunchIndex)
	{
		Index = (Index + 1) % PunchAnimations.Num();
	}
	LastPunchIndex = Index;

	UAnimSequenceBase* Punch = PunchAnimations[Index];
	ActivePunchMontage = Punch ? AnimInstance->PlaySlotAnimationAsDynamicMontage(Punch, PunchSlotName, PunchBlendTime, PunchBlendTime, PunchPlayRate) : nullptr;
	if (!ActivePunchMontage)
	{
		return false;
	}

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(this, &APlayerCharacter::OnPunchBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, ActivePunchMontage);

	GetFreeFlowCharacterMovement()->SetRotationLocked(true);
	return true;
}

bool APlayerCharacter::CanStartPunch() const
{
	const UAnimInstance* AnimInstance = IsPunching() ? GetBodyAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return true;
	}

	const float RemainingTime = (ActivePunchMontage->GetPlayLength() - AnimInstance->Montage_GetPosition(ActivePunchMontage)) / PunchPlayRate;
	return RemainingTime <= PunchComboWindow;
}

void APlayerCharacter::StopPunch()
{
	if (!IsPunching())
	{
		return;
	}

	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), GetActorLocation(), GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Stop, MakeAttackMoveLatentInfo());

	if (UAnimInstance* AnimInstance = GetBodyAnimInstance())
	{
		AnimInstance->Montage_Stop(PunchBlendTime, ActivePunchMontage);
	}
}

void APlayerCharacter::OnPunchBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ActivePunchMontage)
	{
		ActivePunchMontage = nullptr;
		GetFreeFlowCharacterMovement()->SetRotationLocked(false);
	}
}

FLatentActionInfo APlayerCharacter::MakeAttackMoveLatentInfo()
{
	FLatentActionInfo LatentInfo;
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = GetUniqueID();
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName("OnMoveCompleted");
	return LatentInfo;
}

UAnimInstance* APlayerCharacter::GetBodyAnimInstance() const
{
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(this);
	for (const USkeletalMeshComponent* SkeletalMesh : Meshes)
	{
		if (UFreeFlowAnimInstance* AnimInstance = Cast<UFreeFlowAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			return AnimInstance;
		}
	}
	return nullptr;
}

void APlayerCharacter::EnterCombat()
{
	bInCombat = true;
	GetWorldTimerManager().SetTimer(CombatTimer, this, &APlayerCharacter::ExitCombat, CombatCameraDuration);
	RefreshCameraMode();
}

void APlayerCharacter::ExitCombat()
{
	bInCombat = false;
	RefreshCameraMode();
}

void APlayerCharacter::HandleLocomotionStateChanged(EFreeFlowLocomotionState PreviousState, EFreeFlowLocomotionState NewState)
{
	if (NewState == EFreeFlowLocomotionState::Rolling || NewState == EFreeFlowLocomotionState::Sliding || NewState == EFreeFlowLocomotionState::Falling)
	{
		StopPunch();
	}

	switch (NewState)
	{
	case EFreeFlowLocomotionState::Standing:
		bLocomotionWantsActionCamera = false;
		break;
	case EFreeFlowLocomotionState::Running:
	case EFreeFlowLocomotionState::Crouching:
	case EFreeFlowLocomotionState::Sliding:
		bLocomotionWantsActionCamera = true;
		break;
	default:
		break;
	}

	RefreshCameraMode();
}

void APlayerCharacter::RefreshCameraMode()
{
	const bool bWantsActionCamera = bInCombat || bLocomotionWantsActionCamera;
	CameraBoom->SetCameraMode(bWantsActionCamera ? EFreeFlowCameraMode::Action : EFreeFlowCameraMode::Exploration);
}

void APlayerCharacter::StopAttacking()
{
	bPressedAttack = false;
	//ResetAttackState();
}

TArray<AActor*> APlayerCharacter::FindEnemiesWithinRange()
{
	TArray<AActor*> OutActors;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		Range,
		ObjectTypes,
		AFreeFlowCharacter::StaticClass(),
		ActorsToIgnore,
		OutActors
	);

	return OutActors;
}

AActor* APlayerCharacter::FindBestEnemyToAttack(const TArray<AActor*>& Enemies)
{
	float BestDot = -2.f;
	AActor* BestEnemy = nullptr;

	for (AActor* CurrEnemy : Enemies) {
		if (CurrEnemy) {
			UE_LOG(LogTemp, Warning, TEXT("Checking Enemy: %s"), *CurrEnemy->GetName());
			const float CurrDot = FindDotProductBetweenEnemies(CurrEnemy);

			if (CurrDot > BestDot)
			{
				BestDot = CurrDot;
				BestEnemy = CurrEnemy;
			}
		}
	}

	if (BestEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("The best enemy is: %s"), *BestEnemy->GetName());
	}
	
	return BestEnemy;
}

float APlayerCharacter::FindDotProductBetweenEnemies(AActor* Enemy)
{

	if (!Enemy || !GetController() || !GetCharacterMovement())
	{
		return -2.0f;
	}

	const FVector LastInput = GetCharacterMovement()->GetLastInputVector();

	if (LastInput.IsNearlyZero())
	{
		return GetCameraDotProduct(Enemy);
	}
	else
	{
		return GetMovementDotProduct(Enemy);
	}
}

float APlayerCharacter::GetMovementDotProduct(AActor* Enemy)
{
	if (!Enemy || !GetController() || !GetCharacterMovement())
	{
		return -2.0f;
	}

	// Move() already converts the input into world space using the camera yaw,
	// so the last input vector is the world direction the player is pushing towards.
	const FVector DesiredDirection = GetCharacterMovement()->GetLastInputVector().GetSafeNormal2D();

	const FVector DirectionToEnemy = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	return FVector::DotProduct(DesiredDirection, DirectionToEnemy);
}

float APlayerCharacter::GetCameraDotProduct(AActor* Enemy)
{
	if (!Enemy)
	{
		return -2.0f;
	}

	FVector EyeLocation;
	FRotator EyeRotation;

	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector DirectionToEnemy = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	const FVector CameraDirection = EyeRotation.Vector().GetSafeNormal2D();

	return FVector::DotProduct(CameraDirection, DirectionToEnemy);
}

void APlayerCharacter::OnMoveCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("Move completed!"));
}