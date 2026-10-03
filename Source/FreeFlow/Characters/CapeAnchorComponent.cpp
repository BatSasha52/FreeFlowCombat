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
	if (CapeComponent && GetCurrentAnchorLocation(AnchorLocation))
	{
		CapeComponent->SetRelativeLocation(BaseRelativeLocation + (AnchorLocation - RestAnchorLocation) * GetFollowWeight());
	}
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

float UCapeAnchorComponent::GetFollowWeight() const
{
	if (FollowWeightCurve.IsNone())
	{
		return 1.f;
	}

	const UAnimInstance* AnimInstance = BodyMesh ? BodyMesh->GetAnimInstance() : nullptr;
	return AnimInstance ? FMath::Clamp(AnimInstance->GetCurveValue(FollowWeightCurve), 0.f, 1.f) : 0.f;
}
