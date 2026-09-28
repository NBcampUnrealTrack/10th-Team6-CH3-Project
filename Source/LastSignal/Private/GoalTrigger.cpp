#include "GoalTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "LastSignalGameMode.h"

AGoalTrigger::AGoalTrigger()
{
 	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox")); // 트리커 크기 임시 설정입니다.
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

}

void AGoalTrigger::BeginPlay()
{
        Super::BeginPlay();

        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGoalTrigger::OnBoxBeginOverlap);
}

void AGoalTrigger::OnBoxBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                                     UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                     const FHitResult &SweepResult)
{
        APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor);
        if (!Player)
            return;

        Player->SaveStateToGameInstance();

        ALastSignalGameMode *GameMode = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this));
        if (GameMode)
        {
            GameMode->OnGoalReached(NextLevelName);
        }
}

