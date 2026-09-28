#include "EscapeTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LastSignalGameMode.h"
#include "LastSignalPlayerController.h"
#include "LastSignalPlayerHUDComponent.h"

AEscapeTrigger::AEscapeTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    SetRootComponent(TriggerBox);
    TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f)); // 크기 임시값, 헬기존 배치 보고 조절
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

    ObjectiveAfterStart = FText::FromString(TEXT("헬기가 내려올 때까지 생존해라"));
}

void AEscapeTrigger::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEscapeTrigger::OnBoxBeginOverlap);
	
}

void AEscapeTrigger::OnBoxBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                                       UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                       const FHitResult &SweepResult)
{
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
        return;
    
    if (bEscapeStarted) // 이미 시작됐으면 무시 (재진입으로 타이머 리셋 방지)
        return;

    ALastSignalGameMode *GameMode = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this));

    if (GameMode)
    {
        bEscapeStarted = true;
        GameMode->StartEscapeTimer(180.f); // 3분(180초)
    }

    if (ALastSignalPlayerController *PlayerController = Cast<ALastSignalPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        if (ULastSignalPlayerHUDComponent *HUD = PlayerController->GetHUDComponent())
            HUD->SetMissionObjective(ObjectiveAfterStart);
    }
}
