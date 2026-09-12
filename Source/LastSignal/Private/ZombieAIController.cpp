#include "ZombieAIController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AZombieAIController::AZombieAIController()
{
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SetPerceptionComponent(*AIPerception);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f; //AI가 새로운 대상을 시각으로 감지할 수 있는 최대 거리
    SightConfig->LoseSightRadius = 2000.0f; 
    SightConfig->PeripheralVisionAngleDegrees = 90.0f; //AI의 좌우 시야각 범위
    SightConfig->SetMaxAge(5.0f);

    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AZombieAIController::OnPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus)
{
    if (Stimulus.WasSuccessfullySensed())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Zombie] Saw something! %s"), *Actor->GetName());

        DrawDebugString(
            GetWorld(),
            Actor->GetActorLocation() + FVector(0, 0, 100),
            FString ::Printf(TEXT("Saw: %s"), *Actor->GetName()),
            nullptr,
            FColor ::Red,
            2.0f,
            true

        );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Zombie] Missed it! %s"), *Actor->GetName());

        DrawDebugString(
            GetWorld(),
            Actor->GetActorLocation() + FVector(0, 0, 100),
            FString ::Printf(TEXT("Missed: %s"), *Actor->GetName()),
            nullptr,
            FColor ::Green,
            2.0f,
            true);
    }
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();

    if (AIPerception)
    {
        AIPerception->OnTargetPerceptionUpdated.AddDynamic(
            this,
            &AZombieAIController :: OnPerceptionUpdated

        );
}


    GetWorldTimerManager().SetTimer(RandomMoveTimer, this,
        &AZombieAIController::MoveToRandomLocation,
        3.0f, true, 1.0f);

}

void AZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn)
    {

    }
}


void AZombieAIController::MoveToRandomLocation()
{
    APawn* MyPawn = GetPawn();

    if (MyPawn)
    {

    }
    // 현재 월드에서 사용중이 네비시스템을 가져온다.

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());


    // Navigation system이 반환하는 위치정보를 저장할 구조체를 선언
    FNavLocation RandomLocation;

    bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
        MyPawn->GetActorLocation(),
        MoveRadius,
        RandomLocation
    );

    if (bFoundLocation)
    {
            MoveToLocation(RandomLocation.Location); 
            //월드의 특정좌표를 목적지로 삼아 AI에게 이동명령
    }
}
