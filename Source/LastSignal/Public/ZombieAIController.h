
#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "ZombieAIController.generated.h"

UCLASS()
class LASTSIGNAL_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

  public:
    AZombieAIController();

  protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn *InPawn) override;
    // AI가 Pawn을 조종하기 시작할때 호출되는 함수를 재정의 한다고 선언

  private:
    void MoveToRandomLocation();
    FTimerHandle RandomMoveTimer; // 타이머를 나중에중지하거나 관리하게 위해 사용하는 핸들

    UPROPERTY(EditAnywhere, Category = "AI")
    float MoveRadius = 1000.0f;
};
