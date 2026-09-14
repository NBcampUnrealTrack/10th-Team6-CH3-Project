#include "ZombieAIController.h" // 헤더 파일 최상단 포함
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AZombieAIController::AZombieAIController()
{
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AZombieAIController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    if (InPawn)
    {
        StartBehaviorTree();
    }
}

void AZombieAIController::StartBehaviorTree()
{
    if (BehaviorTreeAsset && BlackboardAsset)
    {
        UBlackboardComponent *BlackboardComp = GetBlackboardComponent();

        if (UseBlackboard(BlackboardAsset, BlackboardComp))
        {
            RunBehaviorTree(BehaviorTreeAsset);
            UE_LOG(LogTemp, Warning, TEXT("[Zombie] Behavior Tree started successfully!"));

            FTimerHandle SetPlayerTimer;
            GetWorldTimerManager().SetTimer(SetPlayerTimer, FTimerDelegate::CreateLambda([this, BlackboardComp]()
                                                                                         {
                APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
                if (PlayerPawn && BlackboardComp)
                {
                    BlackboardComp->SetValueAsObject(FName("PlayerObj"), PlayerPawn);
                    BlackboardComp->SetValueAsVector(FName("PlayerVector"), PlayerPawn->GetActorLocation());
                    
                    UE_LOG(LogTemp, Warning, TEXT("[Zombie] Successfully set PlayerObj: %s"), *PlayerPawn->GetName());
                } }),
                                            0.1f, false);
        }
    }
}