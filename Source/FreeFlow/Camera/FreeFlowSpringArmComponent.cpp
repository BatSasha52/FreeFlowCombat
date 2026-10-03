#include "Camera/FreeFlowSpringArmComponent.h"

UFreeFlowSpringArmComponent::UFreeFlowSpringArmComponent()
{
	ExplorationProfile.ArmLength = 200.f;
	ExplorationProfile.SocketOffset = FVector(0.f, 60.f, 10.f);

	ActionProfile.ArmLength = 400.f;
	ActionProfile.SocketOffset = FVector(0.f, 0.f, 30.f);
}

void UFreeFlowSpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	const FFreeFlowCameraProfile& Profile = GetActiveProfile();
	TargetArmLength = Profile.ArmLength;
	SocketOffset = Profile.SocketOffset;
}

void UFreeFlowSpringArmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	const FFreeFlowCameraProfile& Profile = GetActiveProfile();
	TargetArmLength = FMath::FInterpTo(TargetArmLength, Profile.ArmLength, DeltaTime, ProfileBlendSpeed);
	SocketOffset = FMath::VInterpTo(SocketOffset, Profile.SocketOffset, DeltaTime, ProfileBlendSpeed);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UFreeFlowSpringArmComponent::SetCameraMode(EFreeFlowCameraMode NewMode)
{
	CameraMode = NewMode;
}

const FFreeFlowCameraProfile& UFreeFlowSpringArmComponent::GetActiveProfile() const
{
	return CameraMode == EFreeFlowCameraMode::Action ? ActionProfile : ExplorationProfile;
}
