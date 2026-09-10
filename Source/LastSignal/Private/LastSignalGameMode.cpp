#include "LastSignalGameMode.h"
#include "LastSignalGameState.h"

ALastSignalGameMode::ALastSignalGameMode()
{
	GameStateClass = ALastSignalGameState::StaticClass();
}

void ALastSignalGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode BeginPlay"));
}

void ALastSignalGameMode::OnGoalReached()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("CLEAR!"));
}