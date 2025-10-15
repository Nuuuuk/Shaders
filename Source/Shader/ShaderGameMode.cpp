// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShaderGameMode.h"
#include "ShaderCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShaderGameMode::AShaderGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ShaderCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
