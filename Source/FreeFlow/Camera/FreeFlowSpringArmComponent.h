#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "FreeFlowSpringArmComponent.generated.h"

UENUM(BlueprintType)
enum class EFreeFlowCameraMode : uint8
{
	Exploration,
	Action
};

USTRUCT(BlueprintType)
struct FFreeFlowCameraProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", Units = "cm"))
	float ArmLength = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SocketOffset = FVector::ZeroVector;
};

UCLASS(ClassGroup = (FreeFlow), meta = (BlueprintSpawnableComponent))
class FREEFLOW_API UFreeFlowSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UFreeFlowSpringArmComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Camera Modes")
	void SetCameraMode(EFreeFlowCameraMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Camera Modes")
	EFreeFlowCameraMode GetCameraMode() const { return CameraMode; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Modes")
	FFreeFlowCameraProfile ExplorationProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Modes")
	FFreeFlowCameraProfile ActionProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Modes", meta = (ClampMin = "0"))
	float ProfileBlendSpeed = 4.f;

private:
	const FFreeFlowCameraProfile& GetActiveProfile() const;

	EFreeFlowCameraMode CameraMode = EFreeFlowCameraMode::Exploration;
};
