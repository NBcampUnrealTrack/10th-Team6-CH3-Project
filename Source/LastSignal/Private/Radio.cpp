#include "Radio.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LastSignalGameMode.h"


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
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) //다른 액터말고 플레이어만 걸러지게 함
        return;

    
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Radio: in range"));  //  나중에 교체
}

void ARadio::OnBoxEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                             UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
        return;

    
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Radio: out of range")); // 나중에 교ㅕ체
}

void ARadio::Interact()
{
    // GetGameMode()는 AGameModeBase*를 돌려주기 때문에, 우리 GameMode의 함수(StartCountdown)를
    // 쓰려면 실제 타입으로 캐스팅해야 함
    ALastSignalGameMode *GameMode = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this));

    if (GameMode)
    {
        GameMode->StartCountdown(900.f); // 15분(900초) 카운트다운 시작
        OnRadioActivated();              // 블루프린트 쪽 연출 시작 신호
    }
}


