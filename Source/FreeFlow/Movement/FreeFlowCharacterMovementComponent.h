#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FreeFlowCharacterMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EFreeFlowRotationMode : uint8
{
	VelocityDirection,
	LookingDirection
};

UCLASS()
class FREEFLOW_API UFreeFlowCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UFreeFlowCharacterMovementComponent();

	UFUNCTION(BlueprintPure, Category = "Character Movement (Rotation Settings)")
	EFreeFlowRotationMode GetRotationMode() const { return RotationMode; }

	UFUNCTION(BlueprintCallable, Category = "Character Movement (Rotation Settings)")
	void SetRotationMode(EFreeFlowRotationMode NewRotationMode);

	bool CanStandUp() const;

	void BeginScriptedMovement();
	void EndScriptedMovement();
	void SetScriptedHorizontalVelocity(const FVector& HorizontalVelocity);
	bool IsInScriptedMovement() const { return bInScriptedMovement; }

	void SetRotationLocked(bool bLocked) { bRotationLocked = bLocked; }

protected:
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const override;
	virtual void PhysicsRotation(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement (Rotation Settings)", meta = (EditCondition = "bOrientRotationToMovement"))
	EFreeFlowRotationMode RotationMode = EFreeFlowRotationMode::VelocityDirection;

private:
	bool bInScriptedMovement = false;
	bool bRotationLocked = false;
	float SavedGroundFriction = 0.f;
	float SavedBrakingDecelerationWalking = 0.f;
};
