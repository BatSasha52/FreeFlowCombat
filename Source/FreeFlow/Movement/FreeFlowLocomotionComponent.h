#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FreeFlowLocomotionComponent.generated.h"

class ACharacter;
class UAnimSequence;
class UFreeFlowCharacterMovementComponent;

UENUM(BlueprintType)
enum class EFreeFlowLocomotionState : uint8
{
	Standing,
	Running,
	Crouching,
	Sliding,
	Rolling,
	Falling
};

USTRUCT(BlueprintType)
struct FFreeFlowLocomotionAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	TObjectPtr<UAnimSequence> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0", Units = "cm"))
	float Distance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0", ClampMax = "1"))
	float SpeedFalloff = 0.f;

	float GetDuration() const;
	float GetSpeedAt(float Time) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFreeFlowLocomotionStateChangedSignature, EFreeFlowLocomotionState, PreviousState, EFreeFlowLocomotionState, NewState);

UCLASS(ClassGroup = (FreeFlow), meta = (BlueprintSpawnableComponent))
class FREEFLOW_API UFreeFlowLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFreeFlowLocomotionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetWantsToSprint(bool bWants);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetWantsToCrouch(bool bWants);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	bool TryRoll();

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	EFreeFlowLocomotionState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float GetTimeInState() const { return TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	float GetActionAnimationTime() const;

	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool HasMoveInput() const;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FFreeFlowLocomotionStateChangedSignature OnStateChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speeds", meta = (ClampMin = "0", Units = "cm/s"))
	float WalkSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speeds", meta = (ClampMin = "0", Units = "cm/s"))
	float RunSpeed = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speeds", meta = (ClampMin = "0", Units = "cm/s"))
	float CrouchSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Slide", meta = (ClampMin = "0", Units = "cm/s"))
	float MinSlideEntrySpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Slide")
	FFreeFlowLocomotionAction Slide;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Roll")
	FFreeFlowLocomotionAction Roll;

private:
	void UpdateState();
	EFreeFlowLocomotionState SelectGroundedState() const;
	void SetState(EFreeFlowLocomotionState NewState);
	void EnterState(EFreeFlowLocomotionState NewState);
	void ExitState(EFreeFlowLocomotionState OldState);
	void BeginAction(const FVector& Direction);
	void UpdateAction();
	void UpdateCapsule() const;

	const FFreeFlowLocomotionAction* GetActiveAction() const;
	FVector GetRollDirection() const;
	FVector GetSlideDirection() const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UFreeFlowCharacterMovementComponent> Movement;

	EFreeFlowLocomotionState State = EFreeFlowLocomotionState::Standing;
	float TimeInState = 0.f;
	bool bWantsToSprint = false;
	bool bWantsToCrouch = false;
	FVector ActionDirection = FVector::ForwardVector;
};
