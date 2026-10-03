// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/FreeFlowCharacter.h"
#include "Movement/FreeFlowLocomotionComponent.h"
#include "PlayerCharacter.generated.h"

/**
 * 
 */
class UFreeFlowSpringArmComponent;
class UCameraComponent;
class UCapeAnchorComponent;
class UFreeFlowLocomotionComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class FREEFLOW_API APlayerCharacter : public AFreeFlowCharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UFreeFlowSpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cape, meta = (AllowPrivateAccess = "true"))
	UCapeAnchorComponent* CapeAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UFreeFlowLocomotionComponent* Locomotion;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

public:
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere)
	float Range = 400.f;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (ClampMin = "0", Units = "s"))
	float CombatCameraDuration = 4.f;

	UPROPERTY(EditAnywhere, Category = Input, meta = (ClampMin = "0", Units = "s"))
	float RollDoubleTapWindow = 0.3f;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void SprintStarted();

	void SprintCompleted();

	void CrouchStarted();

	void CrouchCompleted();

	void Attack();

	void StopAttacking();

	bool bPressedAttack = false;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleLocomotionStateChanged(EFreeFlowLocomotionState PreviousState, EFreeFlowLocomotionState NewState);

	void EnterCombat();
	void ExitCombat();
	void RefreshCameraMode();

	bool bInCombat = false;
	bool bLocomotionWantsActionCamera = false;
	double LastSprintPressTime = -1.0;
	FTimerHandle CombatTimer;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	TArray<AActor*> FindEnemiesWithinRange();
	AActor* FindBestEnemyToAttack(const TArray<AActor*>& Enemies);
	float FindDotProductBetweenEnemies(AActor* Enemy);
	float GetMovementDotProduct(AActor* Enemy);
	float GetCameraDotProduct(AActor* Enemy);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class UFreeFlowSpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	void OnMoveCompleted();

};