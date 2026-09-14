#include "BTTask_ZombieAttack.h"
#include "AIController.h"
#include "ZombieAICharacter.h"

UBTTask_ZombieAttack::UBTTask_ZombieAttack()
{
    NodeName = TEXT("Zombie Attack");
}

EBTNodeResult::Type UBTTask_ZombieAttack::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
        return EBTNodeResult::Failed;

    AZombieAICharacter *Zombie = Cast<AZombieAICharacter>(AIController->GetPawn());
    if (!Zombie)
        return EBTNodeResult::Failed;

    // 좀비의 공격 함수 실행
    Zombie->Attack();

    return EBTNodeResult::Succeeded;
}