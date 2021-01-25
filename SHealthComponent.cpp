// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SHealthComponent.h"
#include "SHordeGameMode.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	DefaultHealth = 100.f;

	SetIsReplicatedByDefault(true);

	TeamNum = 255;

	bCanRegenerateHealth = false;
	DelayBeforeRegeneration = 5.f;
	HealthRegeneratedPerRegenTick = 10.f;
	RegenTick = 1.f;
}


void USHealthComponent::Heal(float HealAmount)
{
	// Don't heal if HealAmount it 0 or if actor is dead
	if (HealAmount <= 0.f || IsDead())
		return;

	Health = FMath::Clamp(Health + HealAmount, 0.f, DefaultHealth);

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);
}

float USHealthComponent::GetHealth() const
{
	return Health;
}

bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	// Assume Friendly
	if (ActorA == nullptr || ActorA == nullptr)
		return true;

	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	// Assume Friendly
	if (HealthCompA == nullptr || HealthCompB == nullptr)
		return true;

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}

// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* MyOwner = GetOwner();
	// Only bind if we are server
	if (MyOwner && MyOwner->HasAuthority())
	{
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
	}

	Health = DefaultHealth;
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || IsDead())
		return;

	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser))
		return;

	// Decrease health 
	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);

	// Broadcast health change
	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
	
	if (IsDead())
	{
		ASHordeGameMode* GM = Cast<ASHordeGameMode>(GetWorld()->GetAuthGameMode());

		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
	
	if (bCanRegenerateHealth)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Regeneration);

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Regeneration, this, &USHealthComponent::RegenerateHealth, RegenTick, true, DelayBeforeRegeneration);
	}
}

void USHealthComponent::OnRep_Health(float OldHealth)
{
	float Damage = Health - OldHealth;
	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::RegenerateHealth()
{	
	if (Health == DefaultHealth)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Regeneration);
	}
	else if ((Health + HealthRegeneratedPerRegenTick) < DefaultHealth)
	{
		Heal(HealthRegeneratedPerRegenTick);
	}
	else
	{
		float AmountLeftToHeal = DefaultHealth - Health;
		Heal(AmountLeftToHeal);
	}
	
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);
}