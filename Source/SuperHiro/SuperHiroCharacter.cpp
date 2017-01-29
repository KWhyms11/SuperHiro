// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SuperHiro.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "SuperHiroCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"

//////////////////////////////////////////////////////////////////////////
// ASuperHiroCharacter

ASuperHiroCharacter::ASuperHiroCharacter() {
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	bIsRunning = false;
	bIsAiming = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASuperHiroCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASuperHiroCharacter::FlyJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("LaserEyes", IE_Pressed, this, &ASuperHiroCharacter::LaserEyes);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASuperHiroCharacter::ToggleSprint);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASuperHiroCharacter::ToggleAimOn);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASuperHiroCharacter::ToggleAimOff);
	PlayerInputComponent->BindAction("RT", IE_Pressed, this, &ASuperHiroCharacter::ToggleAltAimOn);
	PlayerInputComponent->BindAction("RT", IE_Released, this, &ASuperHiroCharacter::ToggleAltAimOff);
	PlayerInputComponent->BindAction("RT", IE_Pressed, this, &ASuperHiroCharacter::RTAction);
	PlayerInputComponent->BindAction("TeleThrow", IE_Pressed, this, &ASuperHiroCharacter::TeleThrow);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASuperHiroCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASuperHiroCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASuperHiroCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASuperHiroCharacter::LookUpAtRate);

	PlayerInputComponent->BindAxis("FlyAscend", this, &ASuperHiroCharacter::FlyUp);
	PlayerInputComponent->BindAxis("FlyDescend", this, &ASuperHiroCharacter::FlyUp);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ASuperHiroCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ASuperHiroCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ASuperHiroCharacter::OnResetVR);
}


void ASuperHiroCharacter::OnResetVR() {
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ASuperHiroCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location) {
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1) {
		Jump();
	}
}

void ASuperHiroCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location) {
	if (FingerIndex == ETouchIndex::Touch1) {
		StopJumping();
	}
}

void ASuperHiroCharacter::TurnAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	LookTrace();
}

void ASuperHiroCharacter::LookUpAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	LookTrace();
}

void ASuperHiroCharacter::MoveForward(float Value) {
	if ((Controller != NULL) && (Value != 0.0f)) {
		if (bIsRunning) {
			Value = Value * 5000;
		}

		GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Red, "Is Running? : " + FString::FromInt(Value));

		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASuperHiroCharacter::MoveRight(float Value) {
	if ((Controller != NULL) && (Value != 0.0f)) {
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ASuperHiroCharacter::ToggleSprint() {
	bIsRunning = !bIsRunning;
}

void ASuperHiroCharacter::ToggleAimOn() {
	bIsAiming = true;
}

void ASuperHiroCharacter::ToggleAimOff() {
	bIsAiming = false;
}

void ASuperHiroCharacter::ToggleAltAimOn() {
	if (bIsAiming) {
		bIsAltAiming = true;
	}
}

void ASuperHiroCharacter::ToggleAltAimOff() {
	bIsAltAiming = false;
}

void ASuperHiroCharacter::RTAction() {
	if (!bIsAiming) {
		GEngine->AddOnScreenDebugMessage(-1, -1, FColor::White, "Hello!!!");
	}
}

void ASuperHiroCharacter::FlyJump() {
	if (GetCharacterMovement()->IsFalling()) {
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->GravityScale = 0.0f;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	else if (GetCharacterMovement()->IsFlying()) {
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->GravityScale = 1.0f;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
	else {
		Jump();
	}
}

void ASuperHiroCharacter::FlyUp(float f) {
	if (GetCharacterMovement()->MovementMode == MOVE_Flying) {
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Z);

		// add movement in that direction
		AddMovementInput(Direction, f);
	}

}

void ASuperHiroCharacter::LaserEyes() {
	float CamRotX = GetActorRotation().Vector().X;
	float CamRotY = GetActorRotation().Vector().Y;
	float ControlRot = this->GetControlRotation().Vector().Z;
	FVector Start = GetActorLocation() + FVector(0, 0, 65);
	FVector End = (FVector(CamRotX, CamRotY, ControlRot) * 5000.0f) + Start;

	FHitResult* Hit = new FHitResult;
	FCollisionQueryParams* CQParams = new FCollisionQueryParams(FName(TEXT("MyTrace")), true, this);
	FCollisionResponseParams* CRParams = new FCollisionResponseParams;

	UWorld* world = GetWorld();

	if (bIsAiming && !bIsAltAiming) {
		if (world->LineTraceSingleByChannel(*Hit, Start, End, ECC_Camera, *CQParams, *CRParams)) {
			End = Hit->Location;

			AActor* Actor = Hit->GetActor();
			if (Actor != NULL && !Actor->GetName().Contains("Landscape")) {
				Actor->Destroy();
			}
		}
	}

	delete Hit;
	delete CRParams;
	delete CQParams;
}

void ASuperHiroCharacter::TeleThrow() {
	float CamRotX = GetActorRotation().Vector().X;
	float CamRotY = GetActorRotation().Vector().Y;
	float ControlRot = this->GetControlRotation().Vector().Z;

	FVector Start = GetActorLocation() + FVector(0, 0, 65);
	FVector End = (FVector(CamRotX, CamRotY, ControlRot) * 5000.0f) + Start;

	FHitResult* Hit = new FHitResult;
	FCollisionQueryParams* CQParams = new FCollisionQueryParams(FName(TEXT("MyTrace")), true, this);
	FCollisionResponseParams* CRParams = new FCollisionResponseParams;

	UWorld* world = GetWorld();

	if (bIsAiming && !bIsAltAiming) {
		if (world->LineTraceSingleByChannel(*Hit, Start, End, ECC_Camera, *CQParams, *CRParams)) {
			End = Hit->Location;

			AActor* Actor = Hit->GetActor();

			if (Actor != NULL && !Actor->GetName().Contains("Landscape")) {
				UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(Actor->GetRootComponent());
				SM->AddImpulse(End * GetActorForwardVector() * 200.0f); // The float determines how hard you throw
			}
		}
	}

	delete Hit;
	delete CRParams;
	delete CQParams;
}

void ASuperHiroCharacter::LookTrace() {
	FRotator ControlRot = this->GetActorRotation();
	FVector Start = GetActorLocation();
	FVector End = (GetActorRotation().Vector() * 1000.0f) + Start;

	FHitResult* Hit = new FHitResult;
	FCollisionQueryParams* CQParams = new FCollisionQueryParams(FName(TEXT("MyTrace")), true, this);
	FCollisionResponseParams* CRParams = new FCollisionResponseParams;

	UWorld* world = GetWorld();

	if (world->LineTraceSingleByChannel(*Hit, Start, End, ECC_Camera, *CQParams, *CRParams)) {
		End = Hit->Location;

		AActor* Actor = Hit->GetActor();
		if (!Actor == NULL && !Actor->GetName().Contains("Landscape")) {
			GEngine->AddOnScreenDebugMessage(-1, -1, FColor::Green, Actor->GetName());
		}
	}

	delete Hit;
	delete CRParams;
	delete CQParams;
}
