#include "ZombieAIController.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"

AZombieAIController::AZombieAIController()
{
    // AIPerception 및 SightConfig 기본 세팅 (컴포넌트 생성)
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    if (SightConfig)
    {
        SightConfig->SightRadius = 1500.0f;                // 감지 반경
        SightConfig->LoseSightRadius = 1800.0f;            // 감지 해제 반경
        SightConfig->PeripheralVisionAngleDegrees = 60.0f; // 시야각
        SightConfig->SetMaxAge(5.0f);

        // 정찰 대상 설정 (모든 대상 감지)
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

        AIPerception->ConfigureSense(*SightConfig);
        AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
    }
}

void AZombieAIController::StartBehaviorTree()
{
    if (BehaviorTreeAsset)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("StartBehaviorTree"));
        RunBehaviorTree(BehaviorTreeAsset);
        UE_LOG(LogTemp, Warning, TEXT("[Zombie] Behavior Tree started"));
    }
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();
    // StartBehaviorTree()는 OnPossess에서 처리하므로 주석 처리 또는 제거
}

void AZombieAIController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn)
    {
        // Pawn을 성공적으로 조종할 때 비헤이비어 트리 실행
        StartBehaviorTree();
    }
}