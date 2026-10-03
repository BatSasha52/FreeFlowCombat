// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/FreeFlowCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

void UFreeFlowCharacterMovementComponent::SetRotationMode(EFreeFlowRotationMode NewRotationMode)
{
	RotationMode = NewRotationMode;
}

FRotator UFreeFlowCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	if (RotationMode == EFreeFlowRotationMode::LookingDirection && !Acceleration.IsNearlyZero())
	{
		if (const AController* Controller = CharacterOwner ? CharacterOwner->GetController() : nullptr)
		{
			return Controller->GetDesiredRotation();
		}
	}

	return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}
