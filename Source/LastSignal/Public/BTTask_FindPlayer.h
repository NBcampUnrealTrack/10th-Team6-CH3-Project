#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPlayer.generated.h"

UCLASS()
class LASTSIGNAL_API UBTTask_FindPlayer : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
        UBTTask_FindPlayer();

		protected:
        virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
		//task가 실행될때 호출되는 함수
};
