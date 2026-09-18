#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_CombatState.generated.h"

UCLASS()
class LASTSIGNAL_API UBTService_CombatState : public UBTService_BlackboardBase
{
	GENERATED_BODY()
	
	public:
        UBTService_CombatState();
          UPROPERTY(EditAnywhere, Category = "AI")
          float CombatDistance = 2000.f;
        protected:
        virtual void TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;
};
