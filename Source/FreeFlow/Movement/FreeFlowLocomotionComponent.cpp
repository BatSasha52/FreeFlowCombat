#include "Movement/FreeFlowLocomotionComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"
#include "Movement/FreeFlowCharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFreeFlowLocomotion, Log, All);

float FFreeFlowLocomotionAction::GetDuration() const
{
	return Animation ? Animation->GetPlayLength() / PlayRate : 0.f;
}

float FFreeFlowLocomotionAction::GetSpeedAt(float Time) const
{
	const float Duration = GetDuration();
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	const float StartSpeed = Distance / (Duration * (1.f - 0.5f * SpeedFalloff));
	const float Alpha = FMath::Clamp(Time / Duration, 0.f, 1.f);
	return StartSpeed * (1.f - SpeedFalloff * Alpha);
}

UFreeFlowLocomotionComponent::UFreeFlowLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	Slide.PlayRate = 1.f;
	Slide.Distance = 450.f;
	Slide.SpeedFalloff = 0.8f;

	Roll.PlayRate = 1.4f;
	Roll.Distance = 350.f;
	Roll.SpeedFalloff = 0.3f;
}

void UFreeFlowLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	Movement = Character ? Cast<UFreeFlowCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;

	if (!Movement)
	{
		SetComponentTickEnabled(false);
		return;
	}

	Movement->AddTickPrerequisiteComponent(this);
	EnterState(State);
}

void UFreeFlowLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeInState += DeltaTime;
	UpdateState();
	UpdateAction();
}

void UFreeFlowLocomotionComponent::SetWantsToSprint(bool bWants)
{
	bWantsToSprint = bWants;
	if (Movement)
	{
		UpdateState();
	}
}

void UFreeFlowLocomotionComponent::SetWantsToCrouch(bool bWants)
{
	bWantsToCrouch = bWants;
	if (Movement)
	{
		UpdateState();
	}
}

bool UFreeFlowLocomotionComponent::TryRoll()
{
	const bool bCanRoll = Movement
		&& Roll.Animation
		&& Movement->IsMovingOnGround()
		&& (State == EFreeFlowLocomotionState::Standing || State == EFreeFlowLocomotionState::Running || State == EFreeFlowLocomotionState::Crouching);

	if (bCanRoll)
	{
		SetState(EFreeFlowLocomotionState::Rolling);
	}
	return bCanRoll;
}

float UFreeFlowLocomotionComponent::GetActionAnimationTime() const
{
	const FFreeFlowLocomotionAction* Action = GetActiveAction();
	return Action && Action->Animation ? FMath::Min(TimeInState * Action->PlayRate, Action->Animation->GetPlayLength()) : 0.f;
}

bool UFreeFlowLocomotionComponent::HasMoveInput() const
{
	return Character && !Character->GetLastMovementInputVector().IsNearlyZero();
}

void UFreeFlowLocomotionComponent::UpdateState()
{
	if (Movement->IsFalling())
	{
		SetState(EFreeFlowLocomotionState::Falling);
		return;
	}

	switch (State)
	{
	case EFreeFlowLocomotionState::Sliding:
	case EFreeFlowLocomotionState::Rolling:
		if (TimeInState >= GetActiveAction()->GetDuration())
		{
			SetState(SelectGroundedState());
		}
		return;

	case EFreeFlowLocomotionState::Running:
		if (bWantsToCrouch && Slide.Animation && Movement->Velocity.Size2D() >= MinSlideEntrySpeed)
		{
			SetState(EFreeFlowLocomotionState::Sliding);
			return;
		}
		break;

	default:
		break;
	}

	SetState(SelectGroundedState());
}

EFreeFlowLocomotionState UFreeFlowLocomotionComponent::SelectGroundedState() const
{
	if (bWantsToCrouch || !Movement->CanStandUp())
	{
		return EFreeFlowLocomotionState::Crouching;
	}

	if (bWantsToSprint && HasMoveInput())
	{
		return EFreeFlowLocomotionState::Running;
	}

	return EFreeFlowLocomotionState::Standing;
}

void UFreeFlowLocomotionComponent::SetState(EFreeFlowLocomotionState NewState)
{
	if (NewState == State)
	{
		return;
	}

	const EFreeFlowLocomotionState PreviousState = State;
	ExitState(PreviousState);
	State = NewState;
	TimeInState = 0.f;
	EnterState(NewState);

	UE_LOG(LogFreeFlowLocomotion, Log, TEXT("%s: %s -> %s"), *GetNameSafe(GetOwner()),
		*StaticEnum<EFreeFlowLocomotionState>()->GetNameStringByValue(static_cast<int64>(PreviousState)),
		*StaticEnum<EFreeFlowLocomotionState>()->GetNameStringByValue(static_cast<int64>(NewState)));

	OnStateChanged.Broadcast(PreviousState, NewState);
}

void UFreeFlowLocomotionComponent::EnterState(EFreeFlowLocomotionState NewState)
{
	switch (NewState)
	{
	case EFreeFlowLocomotionState::Standing:
		Movement->MaxWalkSpeed = WalkSpeed;
		Movement->SetRotationMode(EFreeFlowRotationMode::LookingDirection);
		break;

	case EFreeFlowLocomotionState::Running:
		Movement->MaxWalkSpeed = RunSpeed;
		Movement->SetRotationMode(EFreeFlowRotationMode::VelocityDirection);
		break;

	case EFreeFlowLocomotionState::Crouching:
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
		Movement->SetRotationMode(EFreeFlowRotationMode::VelocityDirection);
		break;

	case EFreeFlowLocomotionState::Sliding:
		BeginAction(GetSlideDirection());
		break;

	case EFreeFlowLocomotionState::Rolling:
		BeginAction(GetRollDirection());
		break;

	case EFreeFlowLocomotionState::Falling:
		Movement->SetRotationMode(EFreeFlowRotationMode::VelocityDirection);
		break;
	}

	UpdateCapsule();
}

void UFreeFlowLocomotionComponent::ExitState(EFreeFlowLocomotionState OldState)
{
	if (OldState == EFreeFlowLocomotionState::Sliding || OldState == EFreeFlowLocomotionState::Rolling)
	{
		Movement->EndScriptedMovement();
	}
}

void UFreeFlowLocomotionComponent::BeginAction(const FVector& Direction)
{
	ActionDirection = Direction;
	Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	Movement->SetRotationMode(EFreeFlowRotationMode::VelocityDirection);
	Movement->BeginScriptedMovement();
	UpdateAction();
}

void UFreeFlowLocomotionComponent::UpdateAction()
{
	if (const FFreeFlowLocomotionAction* Action = GetActiveAction())
	{
		Movement->SetScriptedHorizontalVelocity(ActionDirection * Action->GetSpeedAt(TimeInState));
	}
}

void UFreeFlowLocomotionComponent::UpdateCapsule() const
{
	const bool bLowCapsule = State == EFreeFlowLocomotionState::Crouching
		|| State == EFreeFlowLocomotionState::Sliding
		|| State == EFreeFlowLocomotionState::Rolling;

	if (bLowCapsule)
	{
		Character->Crouch();
	}
	else
	{
		Character->UnCrouch();
	}
}

const FFreeFlowLocomotionAction* UFreeFlowLocomotionComponent::GetActiveAction() const
{
	switch (State)
	{
	case EFreeFlowLocomotionState::Sliding:
		return &Slide;
	case EFreeFlowLocomotionState::Rolling:
		return &Roll;
	default:
		return nullptr;
	}
}

FVector UFreeFlowLocomotionComponent::GetRollDirection() const
{
	const FVector InputDirection = Character->GetLastMovementInputVector().GetSafeNormal2D();
	return InputDirection.IsNearlyZero() ? Character->GetActorForwardVector().GetSafeNormal2D() : InputDirection;
}

FVector UFreeFlowLocomotionComponent::GetSlideDirection() const
{
	const FVector VelocityDirection = Movement->Velocity.GetSafeNormal2D();
	return VelocityDirection.IsNearlyZero() ? Character->GetActorForwardVector().GetSafeNormal2D() : VelocityDirection;
}
