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

protected:
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement (Rotation Settings)", meta = (EditCondition = "bOrientRotationToMovement"))
	EFreeFlowRotationMode RotationMode = EFreeFlowRotationMode::VelocityDirection;

private:
	bool bInScriptedMovement = false;
	float SavedGroundFriction = 0.f;
	float SavedBrakingDecelerationWalking = 0.f;
};
