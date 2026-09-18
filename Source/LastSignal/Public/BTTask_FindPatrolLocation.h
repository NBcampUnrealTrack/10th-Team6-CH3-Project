#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "CoreMinimal.h"
#include "BTTask_FindPatrolLocation.generated.h"

UCLASS()
class LASTSIGNAL_API UBTTask_FindPatrolLocation : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

  public:
    UBTTask_FindPatrolLocation();

    UPROPERTY(EditAnywhere, Category = "Patrol")
    float PatrolRadius = 1000.f;

  protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
};