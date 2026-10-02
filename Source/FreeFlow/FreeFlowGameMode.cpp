// Copyright Epic Games, Inc. All Rights Reserved.

#include "FreeFlowGameMode.h"
#include "FreeFlowCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFreeFlowGameMode::AFreeFlowGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
