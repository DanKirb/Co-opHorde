// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SHitScanWeapon.generated.h"

class UNiagaraSystem;

/**
** Contains information of a single hitscan weapon linetrace 
*/
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	/** The surface the linetrace hit */
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	/** The location the linetrace hit */
	UPROPERTY()
	FVector_NetQuantize TraceEnd;

	/** Used to force replication even if the other values haven't changed */
	UPROPERTY()
	uint8 ReplicationCount;
};


UCLASS()
class COOPHORDE_API ASHitScanWeapon : public ASWeapon
{
	GENERATED_BODY()
	
public:

	ASHitScanWeapon();

protected:

	/** The parameter name in Target of the TracerEffect */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName TracerTargetName;

	/** Tracer effect used when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	UNiagaraSystem* TracerEffect;

	/** Information about a single hitscan line trace */
	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	/** The maximum distance the shot line trace will go */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats)
	float MaxShotDistance;

	/** Time needed between weapon fire for the shot to be accurate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats, meta = (ClampMin = 0.f))
	float TimeBetweenAccurateShots;

	/** Whether or not the first shot will be 100% accurate to where the player is aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats)
	bool bHasFirstShotAccuracy;

	/** If true every show will become increasingly less accurate until BulletSpread or BulletSpreadADS is reached, if false the max values will be used from the first shot*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats)
	bool bDecreaseAccuracyPerShot;

	/** The number of shots fired in the current spray, used to reduce accuracy the longer the weapon is fired */
	float ShotsFiredThisSpread;

	/** TimerHandle for shot accuracy spread */
	FTimerHandle TimerHandle_FirstShotAccuracy;

	/** Bullet spread in degrees */
	UPROPERTY(EditDefaultsOnly, Category = Stats, meta = (ClampMin = 0.f))
	float BulletSpread;

	/** Bullet spread in degrees when aiming downing sights */
	UPROPERTY(EditDefaultsOnly, Category = Stats, meta = (ClampMin = 0.f))
	float BulletSpreadADS;

	/** The number of bullets shot every time the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category = Stats, meta = (ClampMin = 0.f))
	int32 BulletsPerFire;

	float CurrentBulletSpread;

protected:

	virtual void BeginPlay() override;

	/** Called when the weapon is fired */
	virtual void Fire() override;

	/** Fires a bullet for BulletsPerFire */
	void FireBullet(FVector ShotDirection, FVector EyeLocation, FVector MuzzleLocation);

	/** Plays the ImpactEffect at the line trace */
	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	/** Plays the TracerEffect from the weapon to line trace hit location */
	void PlayTracerEffects(FVector TraceEnd);

	/** Replication function for HitScanTrace */
	UFUNCTION()
	void OnRep_HitScanTrace();

	/** Add bullet spread to shot and handle first shot accuracy */
	FVector AddBulletSpread(FVector ShotDirection);

	/** Create line trace */
	FHitResult LineTraceShot(AActor* MyOwner, FVector& StartLocation, FVector& EndLocation);

	void ResetFirstShotAccuracy();

	bool SurfaceTypeIsVunerable(EPhysicalSurface PhysicalSurface);

	void SpawnImpactDecal(FHitResult HitResult);
};
