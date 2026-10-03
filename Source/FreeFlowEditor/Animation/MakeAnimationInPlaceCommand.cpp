#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/Skeleton.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFreeFlowEditor, Log, All);

namespace FreeFlowEditor
{
	static UAnimSequence* LoadSequence(const FString& Path)
	{
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
		if (!Sequence)
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("'%s' is not an AnimSequence."), *Path);
		}
		return Sequence;
	}

	static bool GetBoneTrack(const UAnimSequence* Sequence, FName BoneName, TArray<FTransform>& OutKeys)
	{
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Model || !Model->IsValidBoneTrackName(BoneName))
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("'%s' has no track for bone '%s'."), *Sequence->GetName(), *BoneName.ToString());
			return false;
		}

		Model->GetBoneTrackTransforms(BoneName, OutKeys);
		return !OutKeys.IsEmpty();
	}

	static bool GetReferenceLocation(const UAnimSequence* Sequence, FName BoneName, FVector& OutLocation)
	{
		const USkeleton* Skeleton = Sequence->GetSkeleton();
		const int32 BoneIndex = Skeleton ? Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName) : INDEX_NONE;
		if (BoneIndex == INDEX_NONE)
		{
			return false;
		}

		OutLocation = Skeleton->GetReferenceSkeleton().GetRefBonePose()[BoneIndex].GetLocation();
		return true;
	}

	static bool GetComponentSpaceTrack(const UAnimSequence* Sequence, FName BoneName, TArray<FTransform>& OutKeys)
	{
		const USkeleton* Skeleton = Sequence->GetSkeleton();
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		if (!Skeleton || !Model)
		{
			return false;
		}

		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("Skeleton has no bone '%s'."), *BoneName.ToString());
			return false;
		}

		const int32 NumKeys = Model->GetNumberOfKeys();
		OutKeys.Init(FTransform::Identity, NumKeys);

		for (; BoneIndex != INDEX_NONE; BoneIndex = RefSkeleton.GetParentIndex(BoneIndex))
		{
			const FName ChainBone = RefSkeleton.GetBoneName(BoneIndex);
			TArray<FTransform> LocalKeys;
			if (Model->IsValidBoneTrackName(ChainBone))
			{
				Model->GetBoneTrackTransforms(ChainBone, LocalKeys);
			}

			for (int32 Key = 0; Key < NumKeys; ++Key)
			{
				const FTransform& Local = LocalKeys.IsValidIndex(Key) ? LocalKeys[Key] : RefSkeleton.GetRefBonePose()[BoneIndex];
				OutKeys[Key] = OutKeys[Key] * Local;
			}
		}

		return NumKeys > 0;
	}

	static void MakeAnimationInPlace(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogFreeFlowEditor, Warning, TEXT("Usage: FreeFlow.MakeAnimationInPlace <AnimSequencePath> [BoneName=Hips] [Anchor=RefPose|FirstFrame]"));
			return;
		}

		UAnimSequence* Sequence = LoadSequence(Args[0]);
		const FName BoneName = Args.IsValidIndex(1) ? FName(*Args[1]) : FName(TEXT("Hips"));
		const bool bAnchorToFirstFrame = Args.IsValidIndex(2) && Args[2].Equals(TEXT("FirstFrame"), ESearchCase::IgnoreCase);

		TArray<FTransform> Keys;
		if (!Sequence || !GetBoneTrack(Sequence, BoneName, Keys))
		{
			return;
		}

		const FVector FirstFrame = Keys[0].GetLocation();
		const FVector LastFrame = Keys.Last().GetLocation();

		FVector Anchor = FirstFrame;
		if (!bAnchorToFirstFrame && !GetReferenceLocation(Sequence, BoneName, Anchor))
		{
			UE_LOG(LogFreeFlowEditor, Error, TEXT("MakeAnimationInPlace: no reference pose for '%s'."), *BoneName.ToString());
			return;
		}

		TArray<FVector> Positions;
		TArray<FQuat> Rotations;
		TArray<FVector> Scales;
		Positions.Reserve(Keys.Num());
		Rotations.Reserve(Keys.Num());
		Scales.Reserve(Keys.Num());

		for (const FTransform& Key : Keys)
		{
			Positions.Add(FVector(Anchor.X, Anchor.Y, Key.GetLocation().Z));
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

		const FVector Travel = LastFrame - FirstFrame;
		UE_LOG(LogFreeFlowEditor, Display, TEXT("MakeAnimationInPlace: %s - %s locked to %s XY (%.1f, %.1f); removed travel %.1f cm (X %.1f, Y %.1f) over %.3f s."),
			*Sequence->GetName(), *BoneName.ToString(), bAnchorToFirstFrame ? TEXT("first frame") : TEXT("reference pose"), Anchor.X, Anchor.Y,
			FVector2D(Travel.X, Travel.Y).Size(), Travel.X, Travel.Y, Sequence->GetPlayLength());
	}

	static void DescribeAnimationBone(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogFreeFlowEditor, Warning, TEXT("Usage: FreeFlow.DescribeAnimationBone <AnimSequencePath> [BoneName=Hips] [Local|Component]"));
			return;
		}

		const UAnimSequence* Sequence = LoadSequence(Args[0]);
		const FName BoneName = Args.IsValidIndex(1) ? FName(*Args[1]) : FName(TEXT("Hips"));
		const bool bComponentSpace = Args.IsValidIndex(2) && Args[2].Equals(TEXT("Component"), ESearchCase::IgnoreCase);

		TArray<FTransform> Keys;
		if (!Sequence || !(bComponentSpace ? GetComponentSpaceTrack(Sequence, BoneName, Keys) : GetBoneTrack(Sequence, BoneName, Keys)))
		{
			return;
		}

		FBox Bounds(ForceInit);
		for (const FTransform& Key : Keys)
		{
			Bounds += Key.GetLocation();
		}

		FVector Reference = FVector::ZeroVector;
		GetReferenceLocation(Sequence, BoneName, Reference);

		UE_LOG(LogFreeFlowEditor, Display, TEXT("DescribeAnimationBone: %s %s | ref %s | first %s | last %s | min %s | max %s | %d keys, %.3f s"),
			*Sequence->GetName(), *BoneName.ToString(), *Reference.ToCompactString(), *Keys[0].GetLocation().ToCompactString(),
			*Keys.Last().GetLocation().ToCompactString(), *Bounds.Min.ToCompactString(), *Bounds.Max.ToCompactString(), Keys.Num(), Sequence->GetPlayLength());
	}

	static FAutoConsoleCommand MakeAnimationInPlaceCommand(
		TEXT("FreeFlow.MakeAnimationInPlace"),
		TEXT("Locks the horizontal (X/Y) position of a bone track, keeping its height. Usage: FreeFlow.MakeAnimationInPlace <AnimSequencePath> [BoneName=Hips] [Anchor=RefPose|FirstFrame]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&MakeAnimationInPlace));

	static FAutoConsoleCommand DescribeAnimationBoneCommand(
		TEXT("FreeFlow.DescribeAnimationBone"),
		TEXT("Logs the reference, first, last, min and max location of a bone track. Usage: FreeFlow.DescribeAnimationBone <AnimSequencePath> [BoneName=Hips] [Local|Component]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DescribeAnimationBone));
}
