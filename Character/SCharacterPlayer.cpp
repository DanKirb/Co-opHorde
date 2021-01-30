// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "SWeapon.h"
#include "Net/UnrealNetwork.h"

ASCharacterPlayer::ASCharacterPlayer()
{
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(GetRootComponent());

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	AimDownSightFOV = 55.f;
}

void ASCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;
}

void ASCharacterPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bAimDownSight ? AimDownSightFOV : DefaultFOV;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ADSSpeed);

	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacterPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacterPlayer::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacterPlayer::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacterPlayer::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", EInputEvent::IE_Released, this, &ASCharacterPlayer::EndCrouch);

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::Jump);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ASCharacterPlayer::StopJumping);

	PlayerInputComponent->BindAction("AimDownSight", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::BeginADS);
	PlayerInputComponent->BindAction("AimDownSight", EInputEvent::IE_Released, this, &ASCharacterPlayer::EndADS);

	PlayerInputComponent->BindAction("Fire", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::StartFire);
	PlayerInputComponent->BindAction("Fire", EInputEvent::IE_Released, this, &ASCharacterPlayer::StopFire);

	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::StartSprint);
	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Released, this, &ASCharacterPlayer::StopSprint);

	PlayerInputComponent->BindAction("ReloadWeapon", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::ReloadWeapon);

	PlayerInputComponent->BindAction("EquipPrimaryWeapon", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::EquipPrimaryWeapon);
	PlayerInputComponent->BindAction("EquipSecondaryWeapon", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::EquipSecondaryWeapon);
	PlayerInputComponent->BindAction("SwitchWeapon", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::StartEquip);

	PlayerInputComponent->BindAction("Interact", EInputEvent::IE_Pressed, this, &ASCharacterPlayer::Interact);
}

void ASCharacterPlayer::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacterPlayer::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ASCharacterPlayer::SetWantsToSprint(bool WantsToSprint)
{
	Super::SetWantsToSprint(WantsToSprint);

	UpdateCrosshairSprinting(WantsToSprint);
}

void ASCharacterPlayer::SetAimingDownSights(bool NewAimingDownSight)
{
	Super::SetAimingDownSights(NewAimingDownSight);

	UpdateCrosshairADS(NewAimingDownSight);
}

FVector ASCharacterPlayer::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void ASCharacterPlayer::SetOverlappingWeaponPickup(ASWeapon* WeaponPickup)
{
	OverlappingWeaponPickup = WeaponPickup;

	if (OverlappingWeaponPickup)
	{
		UClass* OverlappingWeaponClass = OverlappingWeaponPickup->GetClass();
		if (PrimaryWeapon->IsA(OverlappingWeaponClass))
		{
			PrimaryWeapon->AddToCurrentAmmo(30);
			OverlappingWeaponPickup->Destroy();
		}
		else if (SecondaryWeapon->IsA(OverlappingWeaponClass))
		{
			SecondaryWeapon->AddToCurrentAmmo(30);
			OverlappingWeaponPickup->Destroy();
		}
		else
		{
			OnOverlapUI(true, OverlappingWeaponPickup->GetWeaponName());
		}			
	}
}

void ASCharacterPlayer::RemoveOverlappingWeaponPickup(ASWeapon* WeaponPickup)
{
	if (OverlappingWeaponPickup == WeaponPickup)
	{
		OverlappingWeaponPickup = nullptr;
	}

	OnOverlapUI(false, FName());
}

void ASCharacterPlayer::Interact()
{
	if (!HasAuthority())
	{
		ServerInteract();
	}

	if (OverlappingWeaponPickup)
	{
		UClass* OverlappingWeaponClass = OverlappingWeaponPickup->GetClass();

		if (!PrimaryWeapon->IsA(OverlappingWeaponClass) && !SecondaryWeapon->IsA(OverlappingWeaponClass))
		{			
			ASWeapon* NewWeapon;
			if (CurrentEquippedWeapon == PrimaryWeapon)
			{
				PrimaryWeapon = OverlappingWeaponPickup;
				EquipWeapon(PrimaryWeapon);
				NewWeapon = PrimaryWeapon;
			}
			else
			{
				SecondaryWeapon = OverlappingWeaponPickup;
				EquipWeapon(SecondaryWeapon);
				NewWeapon = SecondaryWeapon;
			}

			DropCurrentWeapon();
			ChangeCurrentEquippedWeapon(NewWeapon);
			CurrentEquippedWeapon->PickupWeapon(this);
		}
		else {
			
		}
		
		OverlappingWeaponPickup = nullptr;
	}
}

void ASCharacterPlayer::ServerInteract_Implementation()
{
	Interact();
}

void ASCharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASCharacterPlayer, OverlappingWeaponPickup, COND_SkipOwner);
}