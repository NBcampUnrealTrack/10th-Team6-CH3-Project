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
    if (Zombie)
    {
        AIController->StopMovement();
        Zombie->Attack();

        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}