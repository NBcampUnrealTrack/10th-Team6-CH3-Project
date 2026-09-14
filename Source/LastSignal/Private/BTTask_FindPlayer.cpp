#include "BTTask_FindPlayer.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

UBTTask_FindPlayer::UBTTask_FindPlayer()
{
    NodeName = TEXT("Find Player Location");
}

EBTNodeResult::Type UBTTask_FindPlayer::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (BlackboardComp)
    {
        APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (PlayerPawn) // ★ Null 체크 필수
        {
            BlackboardComp->SetValueAsVector(TEXT("PlayerVector"), PlayerPawn->GetActorLocation());
            BlackboardComp->SetValueAsObject(TEXT("PlayerObj"), PlayerPawn);
            return EBTNodeResult::Succeeded;
        }
    }
    return EBTNodeResult::Failed; // ★ 찾지 못했으면 Failed 처리
}