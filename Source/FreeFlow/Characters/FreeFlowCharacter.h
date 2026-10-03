// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "FreeFlowCharacter.generated.h"

class UFreeFlowCharacterMovementComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AFreeFlowCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFreeFlowCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	UFreeFlowCharacterMovementComponent* GetFreeFlowCharacterMovement() const;
			

protected:

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

