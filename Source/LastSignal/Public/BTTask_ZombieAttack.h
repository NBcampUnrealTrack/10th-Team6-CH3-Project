#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"
#include "BTTask_ZombieAttack.generated.h"

UCLASS()
class LASTSIGNAL_API UBTTask_ZombieAttack : public UBTTaskNode
{
    GENERATED_BODY()

  public:
    UBTTask_ZombieAttack();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
};