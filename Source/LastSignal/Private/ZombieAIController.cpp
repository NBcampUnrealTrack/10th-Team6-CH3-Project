#include "ZombieAIController.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/Engine.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AZombieAIController::AZombieAIController()
{
 
}

void AZombieAIController::StartBehaviorTree()
{
    if (BehaviorTreeAsset) // 해당 에셋이 있으면
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StartBehaviorTree"));
        RunBehaviorTree(BehaviorTreeAsset); //지정한 BT에셋을 실행하는 함수
        UE_LOG(LogTemp, Warning, TEXT("[Zombie] Behavior Tree started"));
    }

}



void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();

   StartBehaviorTree();

}

void AZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn)
    {

    }
}



