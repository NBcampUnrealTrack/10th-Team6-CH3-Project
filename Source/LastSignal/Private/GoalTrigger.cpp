#include "GoalTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LastSignalGameMode.h"

AGoalTrigger::AGoalTrigger()
{
 	PrimaryActorTick.bCanEverTick = false;
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

}

void AGoalTrigger::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGoalTrigger::OnBoxBeginOverlap);
	
}

void AGoalTrigger::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
		return;

	ALastSignalGameMode* GM = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this));

	if (GM)
		GM->OnGoalReached();
}

