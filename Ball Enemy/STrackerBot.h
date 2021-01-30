// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class USHealthComponent;
class USphereComponent;
class USScoreComponent;

UCLASS()
class COOPHORDE_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = Components)
	UStaticMeshComponent* Mesh;

	/** Collision that triggers self destruct */
	UPROPERTY(VisibleDefaultsOnly, Category = Components)
	USphereComponent* SelfDestructSphere;

	/** Next point in navigation path */
	FVector NextPathPoint;

	/** The amount of force used with adding force on movement */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	float MovementForce;

	/** Whether to change the velocity when adding a force */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	bool bUseVelocityChange;

	/** The minimum distance to the next Path Point for it to move to the next Path Point */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	float RequiredDistanceToPathPoint;

	/** This actors HealthComponent */
	UPROPERTY(VisibleAnywhere, Category = HealthComponent)
	USHealthComponent* HealthComponent;

	/** This actors HealthComponent */
	UPROPERTY(VisibleAnywhere, Category = ScoreComponent)
	USScoreComponent* ScoreComponent;

	/** Dynamic material to pulse on damage */
	UMaterialInstanceDynamic* MatInst;

	/** The Particle effects spawned when exploding */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* ExplosionEffect;

	/** Whether of not the actor has exploded */
	bool bExploded;

	/** The radius of the Radial Damage when exploding */
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float ExplosionRadius;

	/** The base damage exploding will do */
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float ExplosionDamage;

	/** TimerHandle for calling DamageSelf() */
	FTimerHandle TimerHandle_SelfDamage;

	/** The time between calling DamageSelf() when self destructing */
	float SelfDamageInterval;

	/** Whether self destruct has started */
	bool bStartedSelfDestruction;

	/** Sound spawned when self destruct begins */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	USoundBase* SelfDestructSound;

	/** Spawned when exploding */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	USoundBase* ExplosionSound;

	/** ExplosionDamage is multiplied by PowerLevel when exploding */
	UPROPERTY(ReplicatedUsing=OnRep_PowerLevel, VisibleAnywhere, BlueprintReadOnly, Category = Effects)
	int32 PowerLevel;

	/** The Max number the PowerLevel can reach */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	int32 MaxPowerLevel;

	/** The radius used when checking for near by TrackerBots */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	float SwarmCollisionRadius;

	/** TimerHandle for check for other TrackerBots */
	FTimerHandle TimerHandle_CheckForTrackerBots;

	/** The time between each check */
	float CheckForTrackerBotsInterval;

	FTimerHandle TimerHandle_RefreshPath;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Find path to player and get the next PathPoint */
	FVector GetNextPathPoint();

	/** Bound to HealthComponent->OnHealthChanged */
	UFUNCTION()
	void HandleTakeDamage(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	/** Explode, applying radial damage and spawning effects */
	void SelfDestruct();

	/** Apply damage to self */
	void DamageSelf();

	/** Checks for other TrackerBots near by and updates the PowerLevel */
	void CheckForTrackerBot();

	/** Increment the PowerLevel by 1 */
	void IncrementPowerLevel();

	/** Reset the PowerLevel to 1 */
	void ResetPowerLevel();

	/** Called when PowerLevel is replicated */
	UFUNCTION()
	void OnRep_PowerLevel();

	AActor* FindBestTarget();

	void RefreshPath();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Trigger self destruct if overlapping with a ASCharacterBase */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:

	/** Set Scaler Parameter Value on the MatInst */
	void SetMaterialInstanceScalerParameter(FName ParamName, float Value);

};
