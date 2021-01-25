// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, USHealthComponent*, HealthComp, float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

/**
* HeathComponent handles all the logic for the actors Health
* From taking damage to healing
*/

UCLASS( ClassGroup=(COOP), meta=(BlueprintSpawnableComponent) )
class COOPHORDE_API USHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USHealthComponent();

	/** Event that is broadcast in HandleTakeAnyDamage() */
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HealthComponent)
	uint8 TeamNum;

public:

	UFUNCTION(BlueprintCallable, Category = HealthComponent)
	void Heal(float HealAmount);

	float GetHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = HealthComponent)
	static bool IsFriendly(AActor* ActorA, AActor* ActorB);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = HealthComponent)
	FORCEINLINE bool IsDead() { return Health <= 0.f; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** The current health of the owning actor */
	UPROPERTY(ReplicatedUsing=OnRep_Health, BlueprintReadOnly, Category = HealthComponent)
	float Health;

	/** The default health of the owning actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HealthComponent)
	float DefaultHealth;

	/** Handles to timer for health regeneration */
	FTimerHandle TimerHandle_Regeneration;

	/** Whether or not the health should automatically regenerate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HealthRegeneration)
	bool bCanRegenerateHealth;

	/** The length of time after taking damage before regeneration starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HealthRegeneration)
	float DelayBeforeRegeneration;

	/** The amount of health regenerated every RegenTick */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HealthRegeneration)
	float HealthRegeneratedPerRegenTick;

	/** Time between each regeneration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HealthRegeneration)
	float RegenTick;
			
protected:

	/** Bound to the OnTakeAnyDamage event of the owning actor. Handles reducing the actors Health and broadcasting OnHealthChanged */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	void RegenerateHealth();
};
