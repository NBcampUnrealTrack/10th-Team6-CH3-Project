#include "BTService_CombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
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
    APawn *AIPawn = AIController->GetPawn();
    APawn *playerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    float Distance = FVector::Distance(AIPawn->GetActorLocation(), playerPawn->GetActorLocation());

    float CombatDistance = 500;

    if (Distance <= CombatDistance)
    {
        OwnerComp.GetBlackboardComponent()->SetValueAsBool(TEXT("IsCombat"), true);
    }
    else
    {
        OwnerComp.GetBlackboardComponent()->SetValueAsBool(TEXT("IsCombat"), false);

    }

}
