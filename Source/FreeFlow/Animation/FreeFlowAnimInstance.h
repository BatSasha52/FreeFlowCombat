#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Movement/FreeFlowLocomotionComponent.h"
#include "FreeFlowAnimInstance.generated.h"

class ACharacter;

UCLASS()
class FREEFLOW_API UFreeFlowAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EFreeFlowLocomotionState LocomotionState = EFreeFlowLocomotionState::Standing;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float ActionTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsIdle = true;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsWalking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsUpright = true;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsCrouchIdle = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsCrouchWalking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsSliding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsRolling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|States")
	bool bIsFalling = false;

	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0", Units = "cm/s"))
	float MovingSpeedThreshold = 10.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UFreeFlowLocomotionComponent> Locomotion;
};
