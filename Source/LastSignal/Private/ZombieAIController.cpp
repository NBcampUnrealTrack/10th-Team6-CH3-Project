#include "ZombieAIController.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

AZombieAIController::AZombieAIController()
{
}
void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();

    // TimerManager에서 타이머를 등록하여 일정시간 뒤 또는 반복적으로 함수 호출
    GetWorldTimerManager().SetTimer(RandomMoveTimer, //
                                    this,
                                    &AZombieAIController::MoveToRandomLocation,
                                    3.0f,
                                    true,
                                    1.0f);
}

void AZombieAIController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    if (InPawn)
    {
    }
}

void AZombieAIController::MoveToRandomLocation()
{
    APawn *MyPawn = GetPawn();

    if (MyPawn)
    {

    }
    // 현재 월드에서 사용중인 NavgationSystem을 가져온다
    UNavigationSystemV1 *NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());

    // Navigation Ststem이 반환하는 위치정보를 저장할 구조체 선언
    FNavLocation RandomLocation;
    bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
        MyPawn->GetActorLocation(),
        MoveRadius,
        RandomLocation
    );

    if (bFoundLocation)
    {
        MoveToLocation(RandomLocation.Location);
        // 월드의 특정 좌표를 목적지로 삼아 AI에게 이동명령
    }
}
