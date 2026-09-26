#include "Radio.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LastSignalGameMode.h"
#include "PlayerCharacter.h"

// UI 추가
#include "LastSignalPlayerController.h"      
#include "LastSignalPlayerHUDComponent.h"    
#include "GameFramework/Pawn.h"

ARadio::ARadio()
{
 	
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox")); // 감지 범위, 크기는 임시
    SetRootComponent(TriggerBox);
    TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

    ObjectiveAfterActivation = FText::FromString(TEXT("제한시간 내에 옥상으로 올라가자."));
}


void ARadio::BeginPlay()
{
    Super::BeginPlay();

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ARadio::OnBoxBeginOverlap); // 함수 등록 : 오버랩 터지면 함수 실행
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ARadio::OnBoxEndOverlap);
}

void ARadio::OnBoxBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
     UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) //다른 액터말고 플레이어만 걸러지게 함
        return;

    
     if (APlayerCharacter *PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
        PlayerCharacter->NearbyInteractable = this;
}

void ARadio::OnBoxEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                             UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
        return;

    
    if (APlayerCharacter *PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
        PlayerCharacter->NearbyInteractable = nullptr;
}

void ARadio::Interact_Implementation(AActor *Interactor)
{
    ALastSignalGameMode *GameMode = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this));

    if (GameMode)
    {
        GameMode->StartCountdown(900.f); // 15분(900초) 카운트다운 시작
        OnRadioActivated();              // 블루프린트 쪽 연출 시작 신호
    }

    // UI 추가: 플레이어 HUD 컴포넌트에 접근하여 HP 회복 및 카운트다운 시작
    if (APawn *PlayerPawn = Cast<APawn>(Interactor))
    {
        if (ALastSignalPlayerController *PC =
                Cast<ALastSignalPlayerController>(PlayerPawn->GetController()))
        {
            if (ULastSignalPlayerHUDComponent *HUD = PC->GetHUDComponent())
            {
                HUD->HealToFull();
                HUD->SwitchToCountdown(900.f);
                HUD->SetMissionObjective(ObjectiveAfterActivation);
            }
        }
    }
    // 여기까지
}

