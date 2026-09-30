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

    void StartBehaviorTree();

  protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializePlayerTarget();
    FTimerHandle PlayerTargetTimer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    class UBehaviorTree *BehaviorTreeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    class UBlackboardData *BlackboardAsset;
};