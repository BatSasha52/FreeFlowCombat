// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/FreeFlowCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

UFreeFlowCharacterMovementComponent::UFreeFlowCharacterMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
	bCanWalkOffLedgesWhenCrouching = true;
	SetCrouchedHalfHeight(60.f);
}

void UFreeFlowCharacterMovementComponent::SetRotationMode(EFreeFlowRotationMode NewRotationMode)
{
	RotationMode = NewRotationMode;
}

bool UFreeFlowCharacterMovementComponent::CanStandUp() const
{
	if (!CharacterOwner || !CharacterOwner->bIsCrouched || !UpdatedComponent)
	{
		return true;
	}

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const UCapsuleComponent* DefaultCapsule = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
	const float Scale = Capsule->GetShapeScale();
	const float StandingHalfHeight = DefaultCapsule->GetUnscaledCapsuleHalfHeight() * Scale;
	const float CurrentHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const float FloorClearance = 2.f;
	const FVector StandingCenter = UpdatedComponent->GetComponentLocation() + FVector(0.f, 0.f, StandingHalfHeight - CurrentHalfHeight + 0.5f * FloorClearance);
	const FCollisionShape StandingShape = FCollisionShape::MakeCapsule(FMath::Max(Radius - 1.f, 1.f), StandingHalfHeight - 0.5f * FloorClearance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FreeFlowCanStandUp), false, CharacterOwner);
	FCollisionResponseParams ResponseParams;
	InitCollisionParams(QueryParams, ResponseParams);

	return !GetWorld()->OverlapBlockingTestByChannel(StandingCenter, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), StandingShape, QueryParams, ResponseParams);
}

void UFreeFlowCharacterMovementComponent::BeginScriptedMovement()
{
	if (bInScriptedMovement)
	{
		return;
	}

	bInScriptedMovement = true;
	SavedGroundFriction = GroundFriction;
	SavedBrakingDecelerationWalking = BrakingDecelerationWalking;
	GroundFriction = 0.f;
	BrakingDecelerationWalking = 0.f;
}

void UFreeFlowCharacterMovementComponent::EndScriptedMovement()
{
	if (!bInScriptedMovement)
	{
		return;
	}

	bInScriptedMovement = false;
	GroundFriction = SavedGroundFriction;
	BrakingDecelerationWalking = SavedBrakingDecelerationWalking;
}

void UFreeFlowCharacterMovementComponent::SetScriptedHorizontalVelocity(const FVector& HorizontalVelocity)
{
	Velocity = FVector(HorizontalVelocity.X, HorizontalVelocity.Y, Velocity.Z);
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

void UFreeFlowCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!bRotationLocked)
	{
		Super::PhysicsRotation(DeltaTime);
	}
}

FVector UFreeFlowCharacterMovementComponent::ScaleInputAcceleration(const FVector& InputAcceleration) const
{
	return bInScriptedMovement ? FVector::ZeroVector : Super::ScaleInputAcceleration(InputAcceleration);
}
