#include "BTService_CombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UBTService_CombatState::UBTService_CombatState()
{
    NodeName = "Update CombatState";
}

void UBTService_CombatState::TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
        return;

    APawn *AIPawn = AIController->GetPawn();
    APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    UBlackboardComponent *BB = OwnerComp.GetBlackboardComponent();

    if (!AIPawn || !PlayerPawn || !BB)
        return;

    const float Distance = FVector::Distance(AIPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
    const bool bInRange = Distance <= CombatDistance;

    bool bHasLineOfSight = false;
    if (bInRange)
    {
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(AIPawn);
        Params.AddIgnoredActor(PlayerPawn);

        // 눈높이(대략 캡슐 절반 위)에서 트레이스
        FVector EyeLocation = AIPawn->GetActorLocation() + FVector(0, 0, 50.f);
        FVector PlayerLocation = PlayerPawn->GetActorLocation() + FVector(0, 0, 50.f);

        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
            Hit, EyeLocation, PlayerLocation, ECC_Visibility, Params);

        bHasLineOfSight = !bBlocked;
    }

    const bool bWasCombat = BB->GetValueAsBool(TEXT("IsCombat"));

    if (bHasLineOfSight)
    {
        BB->SetValueAsBool(TEXT("IsCombat"), true);
        BB->SetValueAsBool(TEXT("IsSearching"), false);
        BB->SetValueAsVector(TEXT("LastKnownPlayerLocation"), PlayerPawn->GetActorLocation());
        LastSeenTime = GetWorld()->GetTimeSeconds();
    }
    else
    {
        BB->SetValueAsBool(TEXT("IsCombat"), false);

        // 방금 전투 중이었다면(=방금 시야를 놓쳤다면) 수색 모드 진입
        if (bWasCombat)
        {
            BB->SetValueAsBool(TEXT("IsSearching"), true);
        }
        // 수색 시간 초과되면 수색 종료 → Patrol로 폴백
        else if (BB->GetValueAsBool(TEXT("IsSearching")))
        {
            const float TimeSinceSeen = GetWorld()->GetTimeSeconds() - LastSeenTime;
            if (TimeSinceSeen > SearchDuration)
            {
                BB->SetValueAsBool(TEXT("IsSearching"), false);
            }
        }
    }
}