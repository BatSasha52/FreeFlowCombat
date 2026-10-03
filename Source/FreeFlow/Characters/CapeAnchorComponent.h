#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CapeAnchorComponent.generated.h"

class UPhysicsAsset;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (FreeFlow), meta = (BlueprintSpawnableComponent))
class FREEFLOW_API UCapeAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCapeAnchorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Cape", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent"))
	FComponentReference Cape;

	UPROPERTY(EditAnywhere, Category = "Cape|Follow")
	TArray<FName> AnchorBones;

	UPROPERTY(EditAnywhere, Category = "Cape|Follow")
	FName FollowWeightCurve = TEXT("CapeFollow");

	UPROPERTY(EditAnywhere, Category = "Cape|Upright")
	FName UprightWeightCurve = TEXT("CapeUpright");

	UPROPERTY(EditAnywhere, Category = "Cape|Upright")
	FName UprightPivotBone = TEXT("Neck");

	UPROPERTY(EditAnywhere, Category = "Cape|Collision")
	TObjectPtr<UPhysicsAsset> BodyCollision;

private:
	bool GetCurrentAnchorLocation(FVector& OutLocation) const;
	bool GetAnchorComponentLocation(FVector& OutLocation) const;
	bool GetUprightPivot(FVector& OutWorldLocation) const;
	float GetCurveWeight(FName CurveName) const;
	void ApplyUpright(float Weight);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CapeComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	int32 ParentBoneIndex = INDEX_NONE;
	int32 UprightPivotBoneIndex = INDEX_NONE;
	TArray<int32> AnchorBoneIndices;
	FVector RestAnchorLocation = FVector::ZeroVector;
	FVector BaseRelativeLocation = FVector::ZeroVector;
	FRotator BaseRelativeRotation = FRotator::ZeroRotator;
};
