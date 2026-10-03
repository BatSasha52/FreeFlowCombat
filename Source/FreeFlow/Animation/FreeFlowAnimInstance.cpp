#include "Animation/FreeFlowAnimInstance.h"
#include "GameFramework/Character.h"
#include "KismetAnimationLibrary.h"

void UFreeFlowAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());
	Locomotion = Character ? Character->FindComponentByClass<UFreeFlowLocomotionComponent>() : nullptr;
}

void UFreeFlowAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !Locomotion)
	{
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	Speed = Velocity.Size2D();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, Character->GetActorRotation());
	bIsMoving = Speed > MovingSpeedThreshold;

	LocomotionState = Locomotion->GetState();
	ActionTime = Locomotion->GetActionAnimationTime();

	const bool bStanding = LocomotionState == EFreeFlowLocomotionState::Standing;
	bIsRunning = LocomotionState == EFreeFlowLocomotionState::Running;
	bIsCrouching = LocomotionState == EFreeFlowLocomotionState::Crouching;
	bIsSliding = LocomotionState == EFreeFlowLocomotionState::Sliding;
	bIsRolling = LocomotionState == EFreeFlowLocomotionState::Rolling;
	bIsFalling = LocomotionState == EFreeFlowLocomotionState::Falling;

	bIsIdle = bStanding && !bIsMoving;
	bIsWalking = bStanding && bIsMoving;
	bIsUpright = bStanding || bIsRunning;
	bIsCrouchIdle = bIsCrouching && !bIsMoving;
	bIsCrouchWalking = bIsCrouching && bIsMoving;
}
