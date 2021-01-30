// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/STrackerBot.h"
#include "SCharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Components/SHealthComponent.h"
#include "Components/SScoreComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBotDrawing(
	TEXT("COOP.DebugTrackerBot"),
	DebugTrackerBotDrawing,
	TEXT("Draw Debug Lines for TrackerBot"),
	ECVF_Cheat);

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetSimulatePhysics(true);
	SetRootComponent(Mesh);

	SelfDestructSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SelfDestructSphere"));
	SelfDestructSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelfDestructSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SelfDestructSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	SelfDestructSphere->SetSphereRadius(200.f);
	SelfDestructSphere->SetupAttachment(GetRootComponent());

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);
	HealthComponent->TeamNum = 1;

	ScoreComponent = CreateDefaultSubobject<USScoreComponent>(TEXT("ScoreComponent"));
	ScoreComponent->Score = 5.f;

	MovementForce = 500.f;
	bUseVelocityChange = false;
	RequiredDistanceToPathPoint = 100.f;

	SelfDamageInterval = 0.25f;
	ExplosionRadius = 350.f;
	ExplosionDamage = 40.f;
	bStartedSelfDestruction = false;

	PowerLevel = 0;
	MaxPowerLevel = 4;
	CheckForTrackerBotsInterval = 1.f;

	SwarmCollisionRadius = 600.f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// Find initial path point
		NextPathPoint = GetNextPathPoint();

		// Start timer for checking for other TrackerBots
		GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::CheckForTrackerBot, CheckForTrackerBotsInterval, true);
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	AActor* BestTarget = FindBestTarget();

	if (BestTarget)
	{
		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		// Find next path point if it is not reach in 5 seconds
		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0f, false);

		if (NavPath && NavPath->PathPoints.Num() > 1)
		{
			// Return next point in the path
			return NavPath->PathPoints[1];
		}
	}

	// No next Path point found
	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* HealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	SetMaterialInstanceScalerParameter(FName("LastTimeDamageTaken"), GetWorld()->TimeSeconds);

	if (Health <= 0.f)
	{
		SelfDestruct();
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
		return;

	bExploded = true;

	// Spawn effects
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::SpawnSoundAtLocation(this, ExplosionSound, GetActorLocation());

	// Hide mesh before destroying it, allowing for all effects to play on clients
	Mesh->SetVisibility(false);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority())
	{
		// Apply Radial Damage
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);
		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		SetLifeSpan(2.f);

		if (DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 1.0f);
		}
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::CheckForTrackerBot()
{
	// Create temporary collision
	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SwarmCollisionRadius);
	
	TArray<FOverlapResult> OverlapResult;
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);

	// Find overlapping pawns 
	GetWorld()->OverlapMultiByObjectType(OverlapResult, GetActorLocation(), FQuat::Identity, QueryParams, CollisionSphere);

	ResetPowerLevel();

	for (FOverlapResult Result : OverlapResult)
	{
		// Stop checking for TrackerBots if PowerLevel is at it's max
		if (PowerLevel == MaxPowerLevel)
			return;

		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		if (Bot && Bot != this)
		{
			IncrementPowerLevel();
		}
	}
}

void ASTrackerBot::IncrementPowerLevel()
{
	if (PowerLevel < MaxPowerLevel)
	{
		PowerLevel++;
		OnRep_PowerLevel();
	}
}

void ASTrackerBot::ResetPowerLevel()
{
	PowerLevel = 0;
	OnRep_PowerLevel();
}

void ASTrackerBot::OnRep_PowerLevel()
{
	SetMaterialInstanceScalerParameter(FName("PowerLevelAlpha"), PowerLevel / (float)MaxPowerLevel);
}

AActor* ASTrackerBot::FindBestTarget()
{
	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* Pawn = It->Get();
		if (Pawn == nullptr || USHealthComponent::IsFriendly(Pawn, this))
		{
			continue;
		}

		// Check if any none player controlled pawns are still alive
		USHealthComponent* HealthComp = Cast<USHealthComponent>(Pawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.f)
		{
			float Distance = (Pawn->GetActorLocation() - GetActorLocation()).Size();
			if (NearestTargetDistance > Distance)
			{
				BestTarget = Pawn;
				NearestTargetDistance = Distance;
			}
		}
	}

	return BestTarget;
}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToPathPoint)
		{
			NextPathPoint = GetNextPathPoint();
		}
		else
		{
			// Keep moving to next path point
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			Mesh->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
		}
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bStartedSelfDestruction && !bExploded)
		return;

	ASCharacterBase* Player = Cast<ASCharacterBase>(OtherActor);
	// Self destruct if close to an ASCharcter
	if (Player && !USHealthComponent::IsFriendly(Player, this))
	{
		// Start self destruct
		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true);
		}		

		bStartedSelfDestruction = true;

		UGameplayStatics::SpawnSoundAttached(SelfDestructSound, GetRootComponent());
	}
}

void ASTrackerBot::SetMaterialInstanceScalerParameter(FName ParamName, float Value)
{
	if (MatInst == nullptr)
	{
		MatInst = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mesh->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue(ParamName, Value);
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASTrackerBot, PowerLevel);
}