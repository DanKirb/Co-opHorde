// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

// OnDamageDealth Event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnDamageDealtSignature, AActor*, DamagedActor, float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
// OnWeaponFired Event
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFiredSignature);

class USkeletalMeshComponent;
class USoundBase;
class ASCharacterBase;
class UNiagaraSystem;
class USphereComponent;

/**
* Used to determine what the weapon is currently doing
*/
UENUM()
enum class EWeaponState : uint8
{
	EWS_Idle,
	EWS_Firing,
	EWS_Reloading
};

UCLASS()
class COOPHORDE_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

	/** The DamageType this weapon will cause */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<UDamageType> DamageType;

	/** Event that is broadcast when the weapon is fired */
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnWeaponFiredSignature OnWeaponFired;

protected:

	/** The weapons mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USkeletalMeshComponent* Mesh;

	/** Used when the weapon is on the ground to overlap for the  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
	USphereComponent* CollisionSphere;

	/** The pawn that has this weapon equipped */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	ASCharacterBase* OwningPawn;

	/** What the weapon is currently doing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	EWeaponState WeaponState;

	/** The name of the muzzle socket on the Mesh */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName MuzzleSocketName;

	/** The effect used when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	UParticleSystem* MuzzleEffect;
	
	/** The default impact effect used when the weapon is fired at something */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	UParticleSystem* DefaultImpactEffect;
	
	/** The default impact effect used when the weapon is fired at the flesh Physical Surface */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	UParticleSystem* FleshImpactEffect;

	/** The default impact effect used when the weapon is fired at the metal Physical Surface */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	UNiagaraSystem* MetalImpactEffect;

	/** Decal spawned when weapon fires at objects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	UMaterialInterface* BulletHitDecal;

	/** The sound that is play everytime the weapon is shot */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* FireSound;

	/** The sound that is played when the weapon's shot hits something */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* ImpactSoundFlesh;
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* ImpactSoundMetal;
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundBase* ImpactSoundDefault;

	/** Camera shake used when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<UCameraShake> FireCamShake;

	/** The base damage each shot of the weapon does */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Weapon)
	float BaseDamage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	float CurrentDamage;

	/** This handles the timing between each shot */
	FTimerHandle TimeHandle_TimeBetweenShots;

	/** The time the last shot was fired */
	float LastFireTime;

	/** RPM - Bullets per minute fired */
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float RateOfFire;

	/** Derived from RateOfFire */
	float TimeBetweenShots;
	
	/** The socket the characters left hand will be attached to */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	FName GunHandSocket;

	/** The amount of crosshair spread added when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float FireCrosshairSpreadAdditive;

	/** The maximum amount of crosshair spread added when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float MaxFireCrosshairSpreadAdditive;

	/** The minimum distance from centre of the screen to the crosshair */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float CrosshairMinDistanceToCentre;

	/** The maximum distance from centre of the screen to the crosshair */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float CrosshairMaxDistanceToCentre;

	/** The minimum distance from centre of the screen to the crosshair when aiming down sights */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float CrosshairMinDistanceToCentreADS;

	/** The maximum distance from centre of the screen to the crosshair when aiming down sights */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Crosshair, meta = (ClampMin = 0.f))
	float CrosshairMaxDistanceToCentreADS;

	/** The current ammo no in the clip */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo, meta = (ClampMin = 0.f))
	int32 CurrentAmmo;

	/** The current ammo in the clip */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo, meta = (ClampMin = 0.f))
	int32 CurrentAmmoInClip;

	/** The maximum ammo per clip */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo, meta = (ClampMin = 0.f))
	int32 MaxAmmoPerClip;

	FTimerHandle TimerHandle_ReloadAnimation;

	/** The maximum ammo for this weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo, meta = (ClampMin = 0.f))
	int32 MaxAmmo;

	/** Used to try and fire the weapon */
	UPROPERTY(BlueprintReadOnly)
	bool bTryToFire;

	/** Used to try and reload the weapon */
	bool bPendingReload;

	/** Animation played when reloading */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	UAnimMontage* ReloadAnimation;

	/** The amount of impulse added to an actor when killed by this weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	float KillImpluseAmount;

	/** Whether or not the weapon is on the ground or attached to a character */
	bool bIsOnGround;

	/** The name of the weapon, used in game */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName WeaponName;

	/** The amount the weapon damage is scaled by when held by an AI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = 0.01f, ClampMax = 1.f))
	float AIDamageScaler;

	/** The animation played on the owner when the weapon is fired from the hip */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = 0.01f, ClampMax = 1.f))
	UAnimMontage* RecoilAnimationFromHip;

	/** The animation played on the owner when the weapon is fired ADS */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = 0.01f, ClampMax = 1.f))
	UAnimMontage* RecoilAnimationADS;

public:
	
	/** Sets OwningPawn */
	void SetOwningPawn(ASCharacterBase* Pawn);

	/** Calls Fire() on weapon if TimeBetweenShots has passed since the last shot */
	virtual void StartFire();

	/** Clears TimeHandle_TimeBetweenShots */
	virtual void StopFire();

	/** Returns Mesh */
	USkeletalMeshComponent* GetMesh();

	/** Tries to reload if possible */
	void TryReload();

	/** Resets the weapons states/values */
	void ResetWeaponState();

	/** Plays reload animation selected for this weapon */
	void PlayReloadAnimation();
	UFUNCTION(Server, Reliable)
	void ServerPlayReloadAnimation();
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastPlayReloadAnimation();

	UFUNCTION(BlueprintCallable)
	void AddToCurrentAmmo(int32 AmountToAdd);

	/** Detaches the weapon from the owner, places it on the ground and readys it for being pickable */
	void DropWeapon();

	/** Attaches the overlapping weapon to the character */
	void PickupWeapon(ASCharacterBase* PawnOwner);

	UFUNCTION(BlueprintCallable)
	void ResetDamage();

	UFUNCTION(BlueprintImplementableEvent)
	void SetupAttachmentToOwner();

	FORCEINLINE FName GetGunHandSocket() { return GunHandSocket; }

	FORCEINLINE EWeaponState GetWeaponState() { return WeaponState; }
	
	FORCEINLINE float GetKillImpulseAmount() { return KillImpluseAmount; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE USkeletalMeshComponent* GetSkeletalMeshComponent() { return Mesh; }
		
	FORCEINLINE bool GetIsOnGround() { return bIsOnGround; }
		
	FORCEINLINE void SetIsOnGround(bool NewIsOnGround) { bIsOnGround = NewIsOnGround; }

	FORCEINLINE FName GetWeaponName() { return WeaponName; }

protected:

	virtual void BeginPlay() override;

	/** Tries to fire weapon if possible */
	void HandleFiring();

	/** Fires the weapon using a line trace, and plays the corresponding impact effect depending on the Physics Surface hit */
	virtual void Fire();

	/** Calls Fire() on the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	/** Plays the selecetd effects when the weapon is fired */
	void PlayFireEffect();
	
	/** Removes the fire spread additive from the Crosshair */
	UFUNCTION(BlueprintImplementableEvent)
	void ResetWeaponFireSpread();

	/** Determines what EWeaponState the weapon is currently in and updates WeaponState */
	void DetermineWeaponState();

	/** Whether or not the weapon can be fired */
	bool CanFire();

	/** Reloads the current clip to it's max and takes the ammo away from the CurrentAmmo */
	void Reload();

	/** Whether or not the weapon can be reloaded */
	bool CanReload();

	/** Calls AddToCurrentAmmo on the client */
	UFUNCTION(Client, Reliable)
	void ClientAddToCurrentAmmo(int32 AmountToAdd);

	UFUNCTION()
	void OnCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCollisionOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
