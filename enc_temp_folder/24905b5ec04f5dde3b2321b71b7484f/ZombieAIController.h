#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"


class UAIPerceptionComponent; // 헤더에서 포인터만 사용할 클래스를 미리 알려주는 전방 선언
class UAISenseConfig_Sight;// 같은내용

#include "ZombieAIController.generated.h"
UCLASS()
class LASTSIGNAL_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

  public:
    AZombieAIController();

  protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent *AIPerception;
    // 

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Sight *SightConfig;

    UFUNCTION()
    void OnPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus);

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn *InPawn) override;
    // AI가 Pawn을 조종하기 시작할때 호출되는 함수를 재정의 한다고 선언

  private:
    void MoveToRandomLocation();
    FTimerHandle RandomMoveTimer; // 타이머를 나중에중지하거나 관리하게 위해 사용하는 핸들

    UPROPERTY(EditAnywhere, Category = "AI")
    float MoveRadius = 1000.0f;
};
