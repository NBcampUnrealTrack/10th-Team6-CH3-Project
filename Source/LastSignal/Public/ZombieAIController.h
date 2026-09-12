#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZombieAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class LASTSIGNAL_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

  public:
    AZombieAIController();

    void StartBehaviorTree();

  protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent *AIPerception;
    // 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Sight *SightConfig;

    

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn *InPawn) override;
    // AI컨트롤러가 Pawn을 조종하기 시작할때 호출되는함수를 재정의하겠다고 선언
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    class UBehaviorTree *BehaviorTreeAsset;


  private:

    FTimerHandle RandomMoveTimer; // 타이머를 나중에 중지하거나 관리하기위해 사용하는 핸들

    UPROPERTY(EditAnywhere, Category = "AI")
    float MoveRadius = 1000.0f;
};