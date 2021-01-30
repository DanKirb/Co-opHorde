// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SCharacterBase.h"
#include "SCharacterPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AWeaponPickup;

UCLASS()
class COOPHORDE_API ASCharacterPlayer : public ASCharacterBase
{
	GENERATED_BODY()
	
public:

	ASCharacterPlayer();

protected:

	/** The cameras field of view when not aiming down sights */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
	float DefaultFOV;

	/** The cameras field of view when aiming down sights */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
	float AimDownSightFOV;

	/** The third person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	UCameraComponent* CameraComp;

	/** The third person camera spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(Replicated)
	ASWeapon* OverlappingWeaponPickup;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Adds movement in the forward direction */
	void MoveForward(float Value);

	/** Adds movement in the right direction */
	void MoveRight(float Value);

	virtual void SetWantsToSprint(bool WantsToSprint) override;

	virtual void SetAimingDownSights(bool NewAimingDownSight) override;

	/** Updates the Crosshair depending on the state of bIsADS */
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateCrosshairADS(bool bIsADS);

	/** Updates the Crosshair depending on the state of bIsSprinting */
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateCrosshairSprinting(bool bIsSprinting);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Returns the location of the Camera Component */
	virtual FVector GetPawnViewLocation() const override;

	void SetOverlappingWeaponPickup(ASWeapon* WeaponPickup);

	void RemoveOverlappingWeaponPickup(ASWeapon* WeaponPickup);

	void Interact();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	UFUNCTION(BlueprintImplementableEvent)
	void OnOverlapUI(bool bShowUI, FName WeaponName);
};
