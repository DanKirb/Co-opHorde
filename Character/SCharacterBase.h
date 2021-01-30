// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacterBase.generated.h"

class ASWeapon;
class USHealthComponent;

UCLASS()
class COOPHORDE_API ASCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacterBase();

protected:
	
	/** How fast the FOV changes when going between ADS and normal */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS, meta = (ClampMin = 0.1, ClampMax = 100))
	float ADSSpeed;
	
	/** The primary weapon that is spawned and attached on begin play */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<ASWeapon> StarterPrimaryWeaponClass;

	/** The secondary weapon that is spawned and attached on begin play */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<ASWeapon> StarterSecondaryWeaponClass;

	/** The current equipped weapon */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	ASWeapon* CurrentEquippedWeapon;

	/** The primary weapon the character is holding */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	ASWeapon* PrimaryWeapon;

	/** The secondary weapon the character is holding */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	ASWeapon* SecondaryWeapon;

	/** The actors USHealthComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USHealthComponent* HealthComponent;

	/** Whether the character is aiming down sights */
	UPROPERTY(Transient, Replicated)
	bool bAimDownSight;	

	/** The socket the equipped weapon will be attached to */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	FName WeaponAttachSocketName;
	
	/** The socket the unequipped weapon will be attached to */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	FName UnequippedWeaponSocketName;

	/** Pawn died previously */
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bDied;

	UPROPERTY(Replicated)
	bool bIsFiring;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Movement)
	bool bWantsToRun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement)
	float SprintSpeed;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement)
	float MaxWalkSpeed;

	/** Animation played when equipping a weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	UAnimMontage* EquipAnimation;

	/** Whether or not the character is currently equipping a weapon */
	UPROPERTY(Replicated)
	bool bIsEquipping;

	/** Whether the character is currently ragdolling */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animation)
	bool bIsRagdoll;

	/** Used to controller the aplpha of the hand IK in the AnimBP */
	UPROPERTY(Replicated)
	float HandIKAlpha;

	/** Sound played when the character dies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Death)
	USoundBase* DeathSound;

	/** Animation played when picking up a weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	UAnimMontage* PickupWeaponAnimation;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Crouches the character */
	UFUNCTION(BlueprintCallable)
	void BeginCrouch();

	/** Uncrouches the character */
	UFUNCTION(BlueprintCallable)
	void EndCrouch();

	/** Sets bAimDownSight to true, AimDownSight logic in Tick() */
	UFUNCTION(BlueprintCallable)
	void BeginADS();

	/** Sets bAimDownSight to false, AimDownSight logic in Tick() */
	UFUNCTION(BlueprintCallable)
	void EndADS();

	/** Event that is bound to HealthComponent->OnHealthChanged, handles charater death if it's health reaches 0 */
	UFUNCTION()
	void OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	virtual void SetAimingDownSights(bool NewAimingDownSight);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAimingDownSights(bool NewAimingDownSight);

	void SetIsFiring(bool NewIsFiring);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetIsFiring(bool NewIsFiring);

	virtual void SetWantsToSprint(bool WantsToSprint);

	/** Calls SetWantsToSprint() on the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartSprint(bool WantsToSprint);
	
	/** Tries to reload the CurrentEquippedWeapon */
	UFUNCTION(BlueprintCallable)
	void ReloadWeapon();

	void EquipPrimaryWeapon();

	void EquipSecondaryWeapon();

	/** Switches CurrentEquippedWeapon to either PrimaryWeapon or SecondaryWeapon */
	UFUNCTION(BlueprintCallable)
	void SwitchWeapon();

	/** Attaches WeaponToEquip to the WeaponAttachSocketName of the character */
	void EquipWeapon(ASWeapon* WeaponToEquip);

	/** Attaches WeaponToUequip to the UnequippedWeaponSocketName of the character */
	void UnequipWeapon(ASWeapon* WeaponToUnequip);

	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapon();

	/** Plays the EquipAnimation which calls SwitchWeapon() via an Anim Notify */
	void StartEquip();

	/** Used to call NetMulticastStartEquip() from the server if client */
	UFUNCTION(Server, Reliable)
	void ServerStartEquip();

	/** Starts equipping on all clients and server */
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastStartEquip();

	/** Called when the EquipAnimation if finished */
	void FinishEquipAnimation();

	/** Called when the character has no health, handles ragdoll and destruction */
	void Die();

	UFUNCTION(Server, Reliable)
	void ServerDie();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastDie();

	/** Sets CurrentEquippedWeapon to NewEquippedWeapon and calls the Server version if required */
	void ChangeCurrentEquippedWeapon(ASWeapon* NewEquippedWeapon);

	/** Server version of ChangeCurrentEquippedWeapon() */
	UFUNCTION(Server, Reliable)
	void ServerChangeCurrentEquippedWeapon(ASWeapon* NewEquippedWeapon);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	/** Changes the MaxWalkSpeed in the CharacterMovementComponent */
	UFUNCTION(BlueprintCallable, Category = "Event")
	void ChangeMaxWalkSpeed(float NewSpeed);

	/** Calls ChangeMaxWalkSpeed() on the server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Event")
	void ServerChangeMaxWalkSpeed(float NewSpeed);

	/** Calls StartFire on the CurrentEquippedWeapon */
	UFUNCTION(BlueprintCallable, Category = Character)
	void StartFire();

	/** Calls StopFire on the CurrentEquippedWeapon */
	UFUNCTION(BlueprintCallable, Category = Character)
	void StopFire();

	/** Tries to sprint */
	UFUNCTION(BlueprintCallable, Category = Character)
	void StartSprint();

	/** Stops sprinting */
	UFUNCTION(BlueprintCallable, Category = Character)
	void StopSprint();

	/* Retrieve Pitch/Yaw from current camera */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	FRotator GetAimOffsets() const;

	/** returns bAimDownSight */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsAimingDownSights() const;

	/** returns bIsFiring */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Targeting")
	bool IsFiring();

	/** Sets HandIKAlpha */
	FORCEINLINE void SetHandIKAlpha(float Alpha) { HandIKAlpha = Alpha; }

	/** returns HandIKAlpha */
	UFUNCTION(BlueprintCallable, Category = "IK")
	float GetHandIKAlpha() const;

	/** Returns the socket on the gun to attack the left hand to */
	UFUNCTION(BlueprintCallable, Category = "IK")
	FVector GetGunSocketLocation() const;

	/** Returns the socket on the gun to attack the left hand to */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsSprinting() const;

	/** Returns bIsEquipping */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsEquipping();

	/** Checks if the passed in Weapon is the CurrentEquippedWeapon */
	bool IsWeaponEquipped(ASWeapon* Weapon);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	void DropCurrentWeapon();
};
