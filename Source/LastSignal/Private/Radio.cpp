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
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) // 다른 액터말고 플레이어만 걸러지게 함
        return;

    if (APlayerCharacter *PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
    {
        PlayerCharacter->NearbyInteractable = this;

        // UI 추가
        if (ALastSignalPlayerController *PC =
                Cast<ALastSignalPlayerController>(PlayerCharacter->GetController()))
        {
            if (ULastSignalPlayerHUDComponent *HUD = PC->GetHUDComponent())
            {
                HUD->SetInteractPromptVisible(true);
            }
        }
        // 여기까지
    }
}

void ARadio::OnBoxEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                             UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
        return;

    if (APlayerCharacter *PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
    {
        PlayerCharacter->NearbyInteractable = nullptr;

        // UI 추가
        if (ALastSignalPlayerController *PC =
                Cast<ALastSignalPlayerController>(PlayerCharacter->GetController()))
        {
            if (ULastSignalPlayerHUDComponent *HUD = PC->GetHUDComponent())
            {
                HUD->SetInteractPromptVisible(false);
            }
        }
        // 여기까지
    }
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
                HUD->SetMissionObjective(
                    FText::FromString(TEXT("15분 후 구조 헬기가 옥상에 도착한다")));
            }
        }
    }
    // 여기까지
}

