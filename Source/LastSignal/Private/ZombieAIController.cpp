#include "ZombieAIController.h" // 헤더 파일 최상단 포함
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AZombieAIController::AZombieAIController()
{
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AZombieAIController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    if (InPawn)
    {
        StartBehaviorTree();
    }
}

void AZombieAIController::StartBehaviorTree()
{
    if (BehaviorTreeAsset && BlackboardAsset)
    {
        UBlackboardComponent *BlackboardComp = GetBlackboardComponent();

        if (UseBlackboard(BlackboardAsset, BlackboardComp))
        {
            RunBehaviorTree(BehaviorTreeAsset);
            UE_LOG(LogTemp, Warning, TEXT("[Zombie] Behavior Tree started successfully!"));

            GetWorldTimerManager().SetTimer(
                PlayerTargetTimer, this, &AZombieAIController::InitializePlayerTarget, 0.1f, false);
        }
    }
}
void AZombieAIController::InitializePlayerTarget()
{
    // 사망 직후나 다른 Pawn으로 바뀐 뒤에는 이전 참조를 사용하지 않는다.
    if (!IsValid(GetPawn()))
        return;

    UBlackboardComponent *BlackboardComp = GetBlackboardComponent();
    APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (BlackboardComp && IsValid(PlayerPawn))
    {
        BlackboardComp->SetValueAsObject(FName("PlayerObj"), PlayerPawn);
        BlackboardComp->SetValueAsVector(FName("PlayerVector"), PlayerPawn->GetActorLocation());
    }
}

void AZombieAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(PlayerTargetTimer);
    Super::OnUnPossess();
}

void AZombieAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(PlayerTargetTimer);
    Super::EndPlay(EndPlayReason);
}
