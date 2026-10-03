#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFreeFlowEditor, Log, All);

namespace FreeFlowEditor
{
	static void MakeAnimationInPlace(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogFreeFlowEditor, Warning, TEXT("Usage: FreeFlow.MakeAnimationInPlace <AnimSequencePath> [BoneName=Hips]"));
			return;
		}

		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Args[0]);
		if (!Sequence)
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("MakeAnimationInPlace: '%s' is not an AnimSequence."), *Args[0]);
			return;
		}

		const FName BoneName = Args.IsValidIndex(1) ? FName(*Args[1]) : FName(TEXT("Hips"));
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Model || !Model->IsValidBoneTrackName(BoneName))
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("MakeAnimationInPlace: '%s' has no track for bone '%s'."), *Sequence->GetName(), *BoneName.ToString());
			return;
		}

		TArray<FTransform> Keys;
		Model->GetBoneTrackTransforms(BoneName, Keys);
		if (Keys.IsEmpty())
		{
			return;
		}

		const FVector Start = Keys[0].GetLocation();
		const FVector End = Keys.Last().GetLocation();

		TArray<FVector> Positions;
		TArray<FQuat> Rotations;
		TArray<FVector> Scales;
		Positions.Reserve(Keys.Num());
		Rotations.Reserve(Keys.Num());
		Scales.Reserve(Keys.Num());

		for (const FTransform& Key : Keys)
		{
			Positions.Add(FVector(Start.X, Start.Y, Key.GetLocation().Z));
			Rotations.Add(Key.GetRotation());
			Scales.Add(Key.GetScale3D());
		}

		IAnimationDataController& Controller = Sequence->GetController();
		Controller.OpenBracket(NSLOCTEXT("FreeFlowEditor", "MakeAnimationInPlace", "Make Animation In Place"));
		const bool bSuccess = Controller.SetBoneTrackKeys(BoneName, Positions, Rotations, Scales);
		Controller.CloseBracket();

		if (!bSuccess)
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("MakeAnimationInPlace: failed to write keys for '%s'."), *Sequence->GetName());
			return;
		}

		Sequence->MarkPackageDirty();

		const FVector Travel = End - Start;
		UE_LOG(LogFreeFlowEditor, Display, TEXT("MakeAnimationInPlace: %s - removed horizontal travel of %s: %.1f cm (X %.1f, Y %.1f) over %.3f s."),
			*Sequence->GetName(), *BoneName.ToString(), FVector2D(Travel.X, Travel.Y).Size(), Travel.X, Travel.Y, Sequence->GetPlayLength());
	}

	static FAutoConsoleCommand MakeAnimationInPlaceCommand(
		TEXT("FreeFlow.MakeAnimationInPlace"),
		TEXT("Removes the horizontal (X/Y) travel of a bone track, keeping its height. Usage: FreeFlow.MakeAnimationInPlace <AnimSequencePath> [BoneName=Hips]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&MakeAnimationInPlace));
}
