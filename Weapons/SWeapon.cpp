// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "SCharacterBase.h"
#include "SCharacterPlayer.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"
#include "../CoopHorde.h"

// Sets default values
ASWeapon::ASWeapon()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("GroundCollisionSphere"));
	CollisionSphere->SetupAttachment(GetRootComponent());
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	MuzzleSocketName = "MuzzleFlashSocket";	

	BaseDamage = 20.f;
	CurrentDamage = BaseDamage;
	RateOfFire = 600.f;
	AIDamageScaler = .3f;

	SetReplicates(true);

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// Crosshair values
	FireCrosshairSpreadAdditive = 20.f;
	MaxFireCrosshairSpreadAdditive = 60.f;
	CrosshairMinDistanceToCentre = 50.f;
	CrosshairMaxDistanceToCentre = 100.f;
	CrosshairMinDistanceToCentreADS = 15.f;
	CrosshairMaxDistanceToCentreADS = 45.f;

	MaxAmmo = 999;
	CurrentAmmo = MaxAmmo;
	MaxAmmoPerClip = 30;
	CurrentAmmoInClip = MaxAmmoPerClip;

	KillImpluseAmount = 50000.f;

	WeaponState = EWeaponState::EWS_Idle;

	GunHandSocket = "GunHandIK";
		
	bIsOnGround = false;

	SetReplicateMovement(true);
}

void ASWeapon::ResetWeaponState()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadAnimation);
	bPendingReload = false;
	bTryToFire = false;
	DetermineWeaponState();
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	TimeBetweenShots = 60 / RateOfFire;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASWeapon::OnCollisionOverlapBegin);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ASWeapon::OnCollisionOverlapEnd);

	SetupAttachmentToOwner();
}

void ASWeapon::HandleFiring()
{
	DetermineWeaponState();

	if (CanFire())
	{
		Fire();
		CurrentAmmoInClip--;
	}
	else
	{
		TryReload();
	}
}

void ASWeapon::Fire()
{	
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::SetOwningPawn(ASCharacterBase* Pawn)
{
	OwningPawn = Pawn;
}

void ASWeapon::StartFire()
{
	bTryToFire = true;
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.f);
	GetWorldTimerManager().SetTimer(TimeHandle_TimeBetweenShots, this, &ASWeapon::HandleFiring, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	bTryToFire = false;
	GetWorldTimerManager().ClearTimer(TimeHandle_TimeBetweenShots);

	DetermineWeaponState();
}

USkeletalMeshComponent* ASWeapon::GetMesh()
{
	return Mesh;
}

void ASWeapon::PlayFireEffect()
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, Mesh, MuzzleSocketName);
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());

		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// Recoil Animation
	if (OwningPawn->IsAimingDownSights())
	{
		if (RecoilAnimationADS)
		{
			OwningPawn->PlayAnimMontage(RecoilAnimationADS);
		}
	}
	else
	{
		if (RecoilAnimationFromHip)
		{
			OwningPawn->PlayAnimMontage(RecoilAnimationFromHip);
		}
	}
}

void ASWeapon::DetermineWeaponState()
{
	if (bPendingReload && CanReload())
	{
		WeaponState = EWeaponState::EWS_Reloading;
	}
	else if (bTryToFire && CanFire())
	{
		WeaponState = EWeaponState::EWS_Firing;
	}
	else
	{
		WeaponState = EWeaponState::EWS_Idle;
	}
}

bool ASWeapon::CanFire()
{
	return CurrentAmmoInClip > 0 
		&& !bPendingReload 
		&& !OwningPawn->IsEquipping()
		&& OwningPawn->IsWeaponEquipped(this);
}

void ASWeapon::TryReload()
{
	if (!bPendingReload && CanReload())
	{
		bPendingReload = true;
		
		PlayReloadAnimation();

		DetermineWeaponState();
	}
}

void ASWeapon::Reload()
{
	if (CurrentAmmo > 0 && CurrentAmmoInClip < MaxAmmoPerClip)
	{
		const int32 TempCurrentAmmo = CurrentAmmo;
		// Remove reloaded ammo from current ammo
		CurrentAmmo = FMath::Max(CurrentAmmo - (MaxAmmoPerClip - CurrentAmmoInClip), 0);
		// Reload current clip
		CurrentAmmoInClip = FMath::Min(MaxAmmoPerClip, CurrentAmmoInClip + TempCurrentAmmo);
	}

	bPendingReload = false;
	OwningPawn->SetHandIKAlpha(1.f);

	DetermineWeaponState();
}

bool ASWeapon::CanReload()
{
	return CurrentAmmo > 0 
		&& CurrentAmmoInClip < MaxAmmoPerClip
		&& !OwningPawn->IsEquipping()
		&& OwningPawn->IsWeaponEquipped(this);
}

void ASWeapon::PlayReloadAnimation()
{
	if (HasAuthority())
	{
		NetMulticastPlayReloadAnimation();
	}
	else
	{
		ServerPlayReloadAnimation();
	}
}

void ASWeapon::AddToCurrentAmmo(int32 AmountToAdd)
{
 	if (OwningPawn->IsLocallyControlled())
	{
		CurrentAmmo = FMath::Min(CurrentAmmo + AmountToAdd, MaxAmmo);
	}
	else
	{
		ClientAddToCurrentAmmo(AmountToAdd);
	}
}

void ASWeapon::DropWeapon()
{	
	// Put weapon on ground
	FHitResult Hit;
	FVector TraceEnd = GetActorLocation() + FVector(0.f, 0.f, -300.f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), TraceEnd, ECollisionChannel::ECC_Camera, QueryParams);

	if (Hit.bBlockingHit)
	{
		SetActorLocation(Hit.Location);
		SetActorRotation(FRotator(90.f, GetActorRotation().Yaw, 0.f));
	}

	SetIsOnGround(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetLifeSpan(120.f);
}

void ASWeapon::PickupWeapon(ASCharacterBase* PawnOwner)
{
	SetOwningPawn(PawnOwner);
	SetOwner(PawnOwner);
	SetupAttachmentToOwner();
	ResetDamage();
	SetLifeSpan(0);
}

void ASWeapon::ResetDamage()
{		
{	
	CurrentDamage = OwningPawn->IsPlayerControlled() ? BaseDamage : BaseDamage * AIDamageScaler;
}

void ASWeapon::ClientAddToCurrentAmmo_Implementation(int32 AmountToAdd)
{
	AddToCurrentAmmo(AmountToAdd);
}

void ASWeapon::ServerPlayReloadAnimation_Implementation()
{
	NetMulticastPlayReloadAnimation();
}

void ASWeapon::NetMulticastPlayReloadAnimation_Implementation()
{
	OwningPawn->SetHandIKAlpha(0.f);

	float AnimDuration = OwningPawn->PlayAnimMontage(ReloadAnimation);
	// Reload weapon after animation has finished
	GetWorldTimerManager().SetTimer(TimerHandle_ReloadAnimation, this, &ASWeapon::Reload, AnimDuration - 0.2f);
}

void ASWeapon::OnCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASCharacterPlayer* Player = Cast<ASCharacterPlayer>(OtherActor);
	if (Player)
	{
		Player->SetOverlappingWeaponPickup(this);
	}
}

void ASWeapon::OnCollisionOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASCharacterPlayer* Player = Cast<ASCharacterPlayer>(OtherActor);
	if (Player)
	{
		Player->RemoveOverlappingWeaponPickup(this);
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWeapon, OwningPawn);
}