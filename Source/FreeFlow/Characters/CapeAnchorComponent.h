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

	UPROPERTY(EditAnywhere, Category = "Cape|Collision")
	TObjectPtr<UPhysicsAsset> BodyCollision;

private:
	bool GetCurrentAnchorLocation(FVector& OutLocation) const;
	float GetFollowWeight() const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CapeComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	int32 ParentBoneIndex = INDEX_NONE;
	TArray<int32> AnchorBoneIndices;
	FVector RestAnchorLocation = FVector::ZeroVector;
	FVector BaseRelativeLocation = FVector::ZeroVector;
};
