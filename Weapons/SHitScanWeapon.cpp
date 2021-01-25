// Fill out your copyright notice in the Description page of Project Settings.


#include "SHitScanWeapon.h"
#include "SCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Components/DecalComponent.h"
#include "../CoopHorde.h"

ASHitScanWeapon::ASHitScanWeapon()
{
	TracerTargetName = "TraceEnd";
	MaxShotDistance = 100000;

	BulletSpread = 3.f;
	BulletSpreadADS = 1.f;

	TimeBetweenAccurateShots = 0.5f;
	ShotsFiredThisSpread = 0.f;

	bHasFirstShotAccuracy = true;
	bDecreaseAccuracyPerShot = true;
	BulletsPerFire = 1;

	CurrentBulletSpread = 0.f;
}

void ASHitScanWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void ASHitScanWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (!HasAuthority()) // If client, call server
	{
		ServerFire();
	}

	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		
		FVector MuzzleLocation = Mesh->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = EyeRotation.Vector();

		for (int32 i = 0; i < BulletsPerFire; i++)
		{
			FireBullet(ShotDirection, EyeLocation, MuzzleLocation);
		}

		PlayFireEffect();

		LastFireTime = GetWorld()->TimeSeconds;

		// Broadcast OnWeaponFired Event
		OnWeaponFired.Broadcast();
	}
}

void ASHitScanWeapon::FireBullet(FVector ShotDirection, FVector EyeLocation, FVector MuzzleLocation)
{
	// Handle bullet spread
	ShotDirection = AddBulletSpread(ShotDirection);

	// End of trace if nothing is hit
	FVector TraceEnd = EyeLocation + (ShotDirection * MaxShotDistance);
	FVector TracerEndPoint = TraceEnd;

	EPhysicalSurface SurfaceType = SurfaceType_Default;

	// Do line trace from camera to find closest object that can be hit
	FHitResult ClosestObject = LineTraceShot(GetOwner(), EyeLocation, TraceEnd);
	if (ClosestObject.bBlockingHit)
	{
		TraceEnd = ClosestObject.ImpactPoint;
	}

	// Actually bullet line trace from weapon to closest object that can be hit
	FVector NewEnd = TraceEnd + (ShotDirection * 10);
	FHitResult HitResult = LineTraceShot(GetOwner(), MuzzleLocation, NewEnd);
	if (HitResult.bBlockingHit)
	{
		SurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());

		float ActualDamage = CurrentDamage;
		if (SurfaceType == SURFACE_FLESHVULNERABLE) // Multiply damage if hit a vulnerable spot
		{
			ActualDamage *= 2.f;
		}

		AActor* HitActor = HitResult.GetActor();
		// Apply damage to hit
		UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, HitResult, GetOwner()->GetInstigatorController(), GetOwner(), DamageType);

		SpawnImpactDecal(HitResult);

		ClosestObject = HitResult;
		TraceEnd = HitResult.ImpactPoint;
	}

	PlayTracerEffects(TraceEnd);
	PlayImpactEffects(SurfaceType, ClosestObject.ImpactPoint);

	// Server replicates effects to clients using HitScanTrace and OnRep_HitScanTrace()
	if (HasAuthority())
	{
		HitScanTrace.TraceEnd = TraceEnd;
		HitScanTrace.SurfaceType = SurfaceType;
		HitScanTrace.ReplicationCount++; // Increment to force replication incase the other values don't change
	}
}

void ASHitScanWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	if (SurfaceType == SURFACE_METALDEFAULT || SurfaceType == SURFACE_METALVULNERABLE)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MetalImpactEffect, ImpactPoint);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSoundMetal, ImpactPoint);
		return;
	}

	UParticleSystem* SelectedParticleEffect;
	USoundBase* SelectedSoundEffect;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
		SelectedParticleEffect = FleshImpactEffect;
		SelectedSoundEffect = ImpactSoundFlesh;
		break;
	case SURFACE_FLESHVULNERABLE:
		SelectedParticleEffect = FleshImpactEffect;
		SelectedSoundEffect = ImpactSoundFlesh;
		break;
	default:
		SelectedParticleEffect = DefaultImpactEffect;
		SelectedSoundEffect = ImpactSoundDefault;
		break;
	}

	if (SelectedParticleEffect)
	{
		FVector MuzzleLocation = GetMesh()->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedParticleEffect, ImpactPoint, ShotDirection.Rotation());
	}

	if (SelectedSoundEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SelectedSoundEffect, ImpactPoint);
	}
}

void ASHitScanWeapon::PlayTracerEffects(FVector TraceEnd)
{
	if (TracerEffect)
	{		
		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TracerEffect, Mesh->GetSocketLocation(MuzzleSocketName));
		
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}
}

void ASHitScanWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic effects
	PlayFireEffect();
	PlayTracerEffects(HitScanTrace.TraceEnd);

	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceEnd);
}

FVector ASHitScanWeapon::AddBulletSpread(FVector ShotDirection)
{
	ShotsFiredThisSpread++;

	// Restart the first shot accuracy timer
	GetWorldTimerManager().ClearTimer(TimerHandle_FirstShotAccuracy);
	GetWorldTimerManager().SetTimer(TimerHandle_FirstShotAccuracy, this, &ASHitScanWeapon::ResetFirstShotAccuracy, TimeBetweenAccurateShots);

	if (ShotsFiredThisSpread > 1 || !OwningPawn->IsAimingDownSights() || !bHasFirstShotAccuracy)
	{
		if (bDecreaseAccuracyPerShot)
		{
			if (OwningPawn->IsAimingDownSights())
			{
				if (CurrentBulletSpread < BulletSpreadADS)
				{
					CurrentBulletSpread += 0.1;
				}

				if (CurrentBulletSpread > BulletSpreadADS)
				{
					CurrentBulletSpread = BulletSpreadADS;
				}
			}
			else
			{
				if (CurrentBulletSpread < BulletSpread)
				{
					CurrentBulletSpread += 0.1;
				}

				if (CurrentBulletSpread > BulletSpread)
				{
					CurrentBulletSpread = BulletSpread;
				}
			}
		}
		else
		{
			if (OwningPawn->IsAimingDownSights())
			{
				CurrentBulletSpread = BulletSpreadADS;
			}
			else
			{
				CurrentBulletSpread = BulletSpread;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Spead: %f"), CurrentBulletSpread);

		float HalfRad = FMath::DegreesToRadians(CurrentBulletSpread);
		return FMath::VRandCone(ShotDirection, HalfRad, HalfRad);
	}

	return ShotDirection;
}

FHitResult ASHitScanWeapon::LineTraceShot(AActor* MyOwner, FVector& StartLocation, FVector& EndLocation)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, COLLISION_WEAPON, QueryParams);

	return HitResult;
}


void ASHitScanWeapon::ResetFirstShotAccuracy()
{
	ShotsFiredThisSpread = 0;
	ResetWeaponFireSpread();
}

bool ASHitScanWeapon::SurfaceTypeIsVunerable(EPhysicalSurface PhysicalSurface)
{
	return PhysicalSurface == SURFACE_FLESHVULNERABLE 
		|| PhysicalSurface == SURFACE_METALVULNERABLE;
}

void ASHitScanWeapon::SpawnImpactDecal(FHitResult HitResult)
{
	if (BulletHitDecal)
	{
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(BulletHitDecal, FVector(2.5f), HitResult.Component.Get(), HitResult.BoneName, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), EAttachLocation::KeepWorldPosition);
		if (Decal)
		{
			Decal->SetFadeScreenSize(0.002f);
			Decal->SetFadeOut(2.f, 2.f, false);
		}
	}
}

void ASHitScanWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASHitScanWeapon, HitScanTrace, COND_SkipOwner);
}
