// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SuperHiro.h"
#include "SuperHiroGameMode.h"
#include "SuperHiroCharacter.h"
#include "SuperHiroHUD.h"

ASuperHiroGameMode::ASuperHiroGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		HUDClass = ASuperHiroHUD::StaticClass();
	}
}
