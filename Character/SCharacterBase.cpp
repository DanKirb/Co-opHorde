// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacterBase.h"
#include "SWeapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../CoopHorde.h"
#include "Components/CapsuleComponent.h"
#include "Components/SHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASCharacterBase::ASCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECollisionResponse::ECR_Ignore);
		
	ADSSpeed = 20.f;
	bIsFiring = false;
	bAimDownSight = false;
	bWantsToRun = false;
	bIsEquipping = false;

	WeaponAttachSocketName = "WeaponSocket";
	UnequippedWeaponSocketName = "UnequippedWeaponSocket";

	HealthComponent->OnHealthChanged.AddDynamic(this, &ASCharacterBase::OnHealthChanged);

	MaxWalkSpeed = 400.f;
	SprintSpeed = 600.f;

	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 250.f;

	HandIKAlpha = 1.f;
}

// Called when the game starts or when spawned
void ASCharacterBase::BeginPlay()
{
	Super::BeginPlay();	

	if (HasAuthority()) // Only spawn default weapon on server
	{
		// Spawn default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;

		PrimaryWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterPrimaryWeaponClass, FVector(0.f), FRotator(0.f), SpawnParams);
		if (PrimaryWeapon)
		{			
			ChangeCurrentEquippedWeapon(PrimaryWeapon);
			CurrentEquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);			
			CurrentEquippedWeapon->PickupWeapon(this);
		}

		SecondaryWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterSecondaryWeaponClass, FVector(0.f), FRotator(0.f), SpawnParams);
		if (SecondaryWeapon)
		{
			SecondaryWeapon = SecondaryWeapon;
			SecondaryWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, UnequippedWeaponSocketName);
			SecondaryWeapon->PickupWeapon(this);
		}
	}
}

// Called every frame
void ASCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bWantsToRun)
	{
		if (IsSprinting())
			SetWantsToSprint(true);
	}	
}

void ASCharacterBase::BeginCrouch()
{
	if (IsSprinting())
		SetWantsToSprint(false);

	Crouch();
}

void ASCharacterBase::EndCrouch()
{
	UnCrouch();
}

void ASCharacterBase::BeginADS()
{
	if (IsSprinting())
		return;

	SetAimingDownSights(true);
}

void ASCharacterBase::EndADS()
{
	SetAimingDownSights(false);
}

void ASCharacterBase::StartFire()
{
	if (CurrentEquippedWeapon && !IsSprinting())
	{
		CurrentEquippedWeapon->StartFire();
		SetIsFiring(true);
	}
}

void ASCharacterBase::StopFire()
{
	if (CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->StopFire();
		SetIsFiring(false);
	}
}

void ASCharacterBase::OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.f && !bDied)
	{
		bDied = true;
		StopFire();
		Die();
	}
}

void ASCharacterBase::SetAimingDownSights(bool NewAimingDownSight)
{
	bAimDownSight = NewAimingDownSight;
	if (!HasAuthority())
	{
		ServerSetAimingDownSights(NewAimingDownSight);
	}
}

void ASCharacterBase::ServerSetAimingDownSights_Implementation(bool NewAimingDownSight)
{
	SetAimingDownSights(NewAimingDownSight);
}

bool ASCharacterBase::ServerSetAimingDownSights_Validate(bool NewAimingDownSight)
{
	return true;
}

void ASCharacterBase::SetIsFiring(bool NewIsFiring)
{
	bIsFiring = NewIsFiring;

	if (!HasAuthority())
	{
		ServerSetIsFiring(NewIsFiring);
	}
}

void ASCharacterBase::StartSprint()
{
	SetWantsToSprint(true);
}

void ASCharacterBase::StopSprint()
{
	SetWantsToSprint(false);
}

void ASCharacterBase::SetWantsToSprint(bool WantsToSprint)
{
	bWantsToRun = WantsToSprint;

	if (bWantsToRun)
	{
		if (bIsCrouched)
			UnCrouch();

		if (IsFiring())
			StopFire();

		if (IsAimingDownSights())
			EndADS();
	}

	// Change the max walk speed if we are now sprinting
	if (IsSprinting())
		ChangeMaxWalkSpeed(SprintSpeed);
	else
		ChangeMaxWalkSpeed(MaxWalkSpeed);

	if (!HasAuthority())
	{
		ServerStartSprint(WantsToSprint);
	}
}

void ASCharacterBase::ReloadWeapon()
{
	if (CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->TryReload();
	}
}

void ASCharacterBase::EquipPrimaryWeapon()
{
	if (CurrentEquippedWeapon == PrimaryWeapon) // Weapon is already equipped
		return;

	StartEquip();
}

void ASCharacterBase::EquipSecondaryWeapon()
{
	if (CurrentEquippedWeapon == SecondaryWeapon) // Weapon is already equipped
		return;

	StartEquip();
}

void ASCharacterBase::SwitchWeapon()
{	
	UnequipWeapon(CurrentEquippedWeapon == PrimaryWeapon ? SecondaryWeapon : PrimaryWeapon);
	EquipWeapon(CurrentEquippedWeapon == PrimaryWeapon ? PrimaryWeapon : SecondaryWeapon);

	if (!HasAuthority())
	{
		ServerSwitchWeapon();
	}
}

void ASCharacterBase::ServerSwitchWeapon_Implementation()
{
	SwitchWeapon();
}

void ASCharacterBase::EquipWeapon(ASWeapon* WeaponToEquip)
{	
	WeaponToEquip->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
}

void ASCharacterBase::UnequipWeapon(ASWeapon* WeaponToUnequip)
{
	WeaponToUnequip->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, UnequippedWeaponSocketName);
}

void ASCharacterBase::StartEquip()
{
	if (bIsEquipping)
		return;

	if (HasAuthority())
	{
		NetMulticastStartEquip();
	}
	else
	{
		ServerStartEquip();
	}
}

void ASCharacterBase::ServerStartEquip_Implementation()
{
	NetMulticastStartEquip();
}

void ASCharacterBase::NetMulticastStartEquip_Implementation()
{
	CurrentEquippedWeapon->ResetWeaponState();

	ChangeCurrentEquippedWeapon(CurrentEquippedWeapon == PrimaryWeapon ? SecondaryWeapon : PrimaryWeapon);

	bIsEquipping = true;
	HandIKAlpha = 0.f;
	float AnimDuration = PlayAnimMontage(EquipAnimation);

	FTimerHandle TimerHandle_EquipWeapon;
	GetWorldTimerManager().SetTimer(TimerHandle_EquipWeapon, this, &ASCharacterBase::FinishEquipAnimation, AnimDuration - 0.2f);
}

void ASCharacterBase::FinishEquipAnimation()
{
	bIsEquipping = false;
	HandIKAlpha = 1.f;
}

void ASCharacterBase::Die()
{	
	if (HasAuthority())
	{
		SetReplicateMovement(false);
		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.f);
		if (DeathSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
		}
		
		NetMulticastDie();
	}
	else
	{
		ServerDie();
	}
}

void ASCharacterBase::ServerDie_Implementation()
{
	Die();
}

void ASCharacterBase::NetMulticastDie_Implementation()
{
	/* Disable all collision on capsule */
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	SetActorEnableCollision(true);

	// Ragdoll
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;

	UCharacterMovementComponent* CharacterComp = Cast<UCharacterMovementComponent>(GetMovementComponent());
	if (CharacterComp)
	{
		CharacterComp->StopMovementImmediately();
		CharacterComp->DisableMovement();
		CharacterComp->SetComponentTickEnabled(false);
	}
}

void ASCharacterBase::ChangeCurrentEquippedWeapon(ASWeapon* NewEquippedWeapon)
{
	CurrentEquippedWeapon = NewEquippedWeapon;

	if (!HasAuthority())
	{
		ServerChangeCurrentEquippedWeapon(NewEquippedWeapon);
	}
}

void ASCharacterBase::ServerChangeCurrentEquippedWeapon_Implementation(ASWeapon* NewEquippedWeapon)
{
	ChangeCurrentEquippedWeapon(NewEquippedWeapon);
}

void ASCharacterBase::ServerStartSprint_Implementation(bool WantsToSprint)
{
	SetWantsToSprint(WantsToSprint);
}

bool ASCharacterBase::ServerStartSprint_Validate(bool WantsToSprint)
{
	return true;
}

void ASCharacterBase::ServerSetIsFiring_Implementation(bool NewIsFiring)
{
	SetIsFiring(NewIsFiring);
}

bool ASCharacterBase::ServerSetIsFiring_Validate(bool NewIsFiring)
{
	return true;
}

void ASCharacterBase::ChangeMaxWalkSpeed(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;

	if (!HasAuthority())
	{
		ServerChangeMaxWalkSpeed(NewSpeed);
	}
}

void ASCharacterBase::ServerChangeMaxWalkSpeed_Implementation(float NewSpeed)
{
	ChangeMaxWalkSpeed(NewSpeed);
}

bool ASCharacterBase::ServerChangeMaxWalkSpeed_Validate(float NewSpeed)
{
	return true;
}

FRotator ASCharacterBase::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool ASCharacterBase::IsAimingDownSights() const
{
	return bAimDownSight;
}

bool ASCharacterBase::IsFiring()
{
	return CurrentEquippedWeapon && CurrentEquippedWeapon->GetWeaponState() == EWeaponState::EWS_Firing;
}

float ASCharacterBase::GetHandIKAlpha() const
{
	return HandIKAlpha;
}

FVector ASCharacterBase::GetGunSocketLocation() const
{
	if (CurrentEquippedWeapon)
	{
		FVector GunSocketLocation = CurrentEquippedWeapon->GetMesh()->GetSocketLocation(CurrentEquippedWeapon->GetGunHandSocket());

		return GunSocketLocation;
	}

	return FVector(0.f);
}

bool ASCharacterBase::IsSprinting() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return bWantsToRun && !IsAimingDownSights() && !GetVelocity().IsZero()
		// Don't allow sprint while strafing sideways or standing still (1.0 is straight forward, -1.0 is backward while near 0 is sideways or standing still)
		&& (FVector::DotProduct(GetVelocity().GetSafeNormal2D(), GetActorRotation().Vector()) > 0.8); // Changing this value to 0.1 allows for diagonal sprinting. (holding W+A or W+D keys)
}

bool ASCharacterBase::IsEquipping()
{
	return bIsEquipping;
}

bool ASCharacterBase::IsWeaponEquipped(ASWeapon* Weapon)
{
	return Weapon == CurrentEquippedWeapon;
}

float ASCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		
	if (HealthComponent->IsDead())
	{
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			FPointDamageEvent* const PointDamageEvent = (FPointDamageEvent*)&DamageEvent;

			float ImpulseAmount = Cast<ASCharacterBase>(DamageCauser)->CurrentEquippedWeapon->GetKillImpulseAmount();
			GetMesh()->AddImpulseAtLocation(PointDamageEvent->ShotDirection * ImpulseAmount, PointDamageEvent->HitInfo.ImpactPoint, PointDamageEvent->HitInfo.BoneName);
		}
	}

	return DamageAmount;
}

void ASCharacterBase::DropCurrentWeapon()
{
	CurrentEquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentEquippedWeapon->DropWeapon();
}

void ASCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacterBase, CurrentEquippedWeapon);
	DOREPLIFETIME(ASCharacterBase, PrimaryWeapon);
	DOREPLIFETIME(ASCharacterBase, SecondaryWeapon);
	DOREPLIFETIME(ASCharacterBase, bDied);
	DOREPLIFETIME_CONDITION(ASCharacterBase, bAimDownSight, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCharacterBase, bIsFiring, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCharacterBase, bWantsToRun, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASCharacterBase, HandIKAlpha, COND_SkipOwner);
}