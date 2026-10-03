#include "Characters/CapeAnchorComponent.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "PhysicsEngine/PhysicsAsset.h"

UCapeAnchorComponent::UCapeAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	AnchorBones = { TEXT("LeftArm"), TEXT("RightArm") };
}

void UCapeAnchorComponent::BeginPlay()
{
	Super::BeginPlay();

	CapeComponent = Cast<USceneComponent>(Cape.GetComponent(GetOwner()));
	BodyMesh = CapeComponent ? Cast<USkeletalMeshComponent>(CapeComponent->GetAttachParent()) : nullptr;

	if (!BodyMesh || !BodyMesh->GetSkinnedAsset())
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (USkeletalMeshComponent* ClothCape = Cast<USkeletalMeshComponent>(CapeComponent); ClothCape && BodyCollision)
	{
		ClothCape->AddClothCollisionSource(BodyMesh, BodyCollision);
	}

	const FReferenceSkeleton& RefSkeleton = BodyMesh->GetSkinnedAsset()->GetRefSkeleton();
	ParentBoneIndex = RefSkeleton.FindBoneIndex(CapeComponent->GetAttachSocketName());
	UprightPivotBoneIndex = RefSkeleton.FindBoneIndex(UprightPivotBone);

	AnchorBoneIndices.Reset();
	for (const FName& BoneName : AnchorBones)
	{
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
		if (BoneIndex != INDEX_NONE)
		{
			AnchorBoneIndices.Add(BoneIndex);
		}
	}

	if (ParentBoneIndex == INDEX_NONE || AnchorBoneIndices.IsEmpty())
	{
		SetComponentTickEnabled(false);
		return;
	}

	const FTransform ParentRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, ParentBoneIndex);
	FVector RestSum = FVector::ZeroVector;
	for (const int32 BoneIndex : AnchorBoneIndices)
	{
		RestSum += FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, BoneIndex).GetRelativeTransform(ParentRest).GetLocation();
	}
	RestAnchorLocation = RestSum / AnchorBoneIndices.Num();
	BaseRelativeLocation = CapeComponent->GetRelativeLocation();
	BaseRelativeRotation = CapeComponent->GetRelativeRotation();

	AddTickPrerequisiteComponent(BodyMesh);
	CapeComponent->AddTickPrerequisiteComponent(this);
}

void UCapeAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USkeletalMeshComponent* ClothCape = Cast<USkeletalMeshComponent>(CapeComponent); ClothCape && BodyMesh && BodyCollision)
	{
		ClothCape->RemoveClothCollisionSource(BodyMesh, BodyCollision);
	}

	Super::EndPlay(EndPlayReason);
}

void UCapeAnchorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector AnchorLocation;
	if (!CapeComponent || !GetCurrentAnchorLocation(AnchorLocation))
	{
		return;
	}

	const float FollowWeight = FollowWeightCurve.IsNone() ? 1.f : GetCurveWeight(FollowWeightCurve);
	CapeComponent->SetRelativeLocationAndRotation(BaseRelativeLocation + (AnchorLocation - RestAnchorLocation) * FollowWeight, BaseRelativeRotation);

	const float UprightWeight = GetCurveWeight(UprightWeightCurve);
	if (UprightWeight > 0.f)
	{
		ApplyUpright(UprightWeight);
	}
}

void UCapeAnchorComponent::ApplyUpright(float Weight)
{
	FVector Pivot;
	if (!GetUprightPivot(Pivot))
	{
		return;
	}

	const FQuat CurrentRotation = CapeComponent->GetComponentQuat();
	const FQuat UprightRotation = FQuat::Slerp(CurrentRotation, BodyMesh->GetComponentQuat(), Weight);
	const FQuat Delta = UprightRotation * CurrentRotation.Inverse();
	const FVector Location = Pivot + Delta.RotateVector(CapeComponent->GetComponentLocation() - Pivot);

	CapeComponent->SetWorldLocationAndRotation(Location, UprightRotation);
}

bool UCapeAnchorComponent::GetCurrentAnchorLocation(FVector& OutLocation) const
{
	if (!BodyMesh)
	{
		return false;
	}

	const TArray<FTransform>& Pose = BodyMesh->GetComponentSpaceTransforms();
	if (!Pose.IsValidIndex(ParentBoneIndex))
	{
		return false;
	}

	const FTransform& Parent = Pose[ParentBoneIndex];
	FVector Sum = FVector::ZeroVector;
	for (const int32 BoneIndex : AnchorBoneIndices)
	{
		if (!Pose.IsValidIndex(BoneIndex))
		{
			return false;
		}
		Sum += Pose[BoneIndex].GetRelativeTransform(Parent).GetLocation();
	}

	OutLocation = Sum / AnchorBoneIndices.Num();
	return true;
}

bool UCapeAnchorComponent::GetUprightPivot(FVector& OutWorldLocation) const
{
	FVector ComponentLocation;
	const TArray<FTransform>& Pose = BodyMesh->GetComponentSpaceTransforms();
	if (Pose.IsValidIndex(UprightPivotBoneIndex))
	{
		ComponentLocation = Pose[UprightPivotBoneIndex].GetLocation();
	}
	else if (!GetAnchorComponentLocation(ComponentLocation))
	{
		return false;
	}

	OutWorldLocation = BodyMesh->GetComponentTransform().TransformPosition(ComponentLocation);
	return true;
}

bool UCapeAnchorComponent::GetAnchorComponentLocation(FVector& OutLocation) const
{
	const TArray<FTransform>& Pose = BodyMesh->GetComponentSpaceTransforms();
	FVector Sum = FVector::ZeroVector;
	for (const int32 BoneIndex : AnchorBoneIndices)
	{
		if (!Pose.IsValidIndex(BoneIndex))
		{
			return false;
		}
		Sum += Pose[BoneIndex].GetLocation();
	}

	OutLocation = Sum / AnchorBoneIndices.Num();
	return true;
}

float UCapeAnchorComponent::GetCurveWeight(FName CurveName) const
{
	const UAnimInstance* AnimInstance = BodyMesh && !CurveName.IsNone() ? BodyMesh->GetAnimInstance() : nullptr;
	return AnimInstance ? FMath::Clamp(AnimInstance->GetCurveValue(CurveName), 0.f, 1.f) : 0.f;
}
