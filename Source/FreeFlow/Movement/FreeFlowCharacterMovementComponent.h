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
	UFUNCTION(BlueprintPure, Category = "Character Movement (Rotation Settings)")
	EFreeFlowRotationMode GetRotationMode() const { return RotationMode; }

	UFUNCTION(BlueprintCallable, Category = "Character Movement (Rotation Settings)")
	void SetRotationMode(EFreeFlowRotationMode NewRotationMode);

protected:
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement (Rotation Settings)", meta = (EditCondition = "bOrientRotationToMovement"))
	EFreeFlowRotationMode RotationMode = EFreeFlowRotationMode::VelocityDirection;
};
