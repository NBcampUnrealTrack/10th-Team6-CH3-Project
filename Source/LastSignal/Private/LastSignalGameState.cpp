#include "LastSignalGameState.h"

void ALastSignalGameState::BeginPlay()
{
	Super::BeginPlay();
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("GameState BeginPlay"));
}