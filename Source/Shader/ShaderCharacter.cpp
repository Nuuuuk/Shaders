// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShaderCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AShaderCharacter

AShaderCharacter::AShaderCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShaderCharacter::DummyDebugFunction()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("DummyDebugFunction CALLED"));
}

UCameraComponent* AShaderCharacter::GetPlayerCamera()
{
	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);

	for (UCameraComponent* CAM : Cameras)
	{
		if (CAM && CAM->IsActive())
		{
			CachedCamera = CAM;
			return CachedCamera;
		}
	}

	return nullptr;
}

bool AShaderCharacter::ProjectCrosshairToPaintSurface(
	FName CanvasTag,
	float TraceDistance,
	float ConeRadius,
	int32 NumRays,
	UPARAM(ref) FHitResult& OutHit,
	UPARAM(ref) TArray<FHitResult>& OutExtraHits
)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ProjectCrosshairToPaintSurface CALLED"));
	UCameraComponent* Camera = GetPlayerCamera();
	if (!Camera) return false;

	OutExtraHits.Reset();

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); 
	CollisionParams.bReturnFaceIndex = true;
	CollisionParams.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionParams);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);

	UE_LOG(LogTemp, Warning, TEXT("%d"), Hit.FaceIndex);
	if (bHit && Hit.GetActor() && Hit.GetActor()->ActorHasTag(CanvasTag))
	{
		if (Hit.bBlockingHit && Hit.FaceIndex != INDEX_NONE && Hit.Component.IsValid())
		{
			UPrimitiveComponent* Pirm = Hit.GetComponent();
			OutHit = Hit;
			//Cone tracing
			FVector HitPoint = Hit.ImpactPoint;
			FVector HitNormal = Hit.ImpactNormal;

			if (NumRays <= 0) return false;
			
			for (int32 i = 0; i < NumRays; ++i)
			{
				float Angle = (2. * PI / NumRays) * i;
				FVector offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * ConeRadius;

				FVector ConeStart = Start;
				FVector ConeEnd = HitPoint + offset;

				FHitResult ConeHit;
				GetWorld()->LineTraceSingleByChannel(ConeHit, ConeStart, ConeEnd, ECC_Visibility, CollisionParams);
				if (ConeHit.bBlockingHit && ConeHit.GetActor()->ActorHasTag(CanvasTag))
				{
					OutExtraHits.Add(ConeHit);
					DrawDebugPoint(GetWorld(), ConeHit.ImpactPoint, 5.f, FColor::Blue, false, 0.1f);
				}
			}
			return true;
		}
	}
	return false;
}
/////////////////////////////////// UE ///////////////////////////////////

void AShaderCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AShaderCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShaderCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShaderCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AShaderCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AShaderCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
