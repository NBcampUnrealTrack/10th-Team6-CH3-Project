#include "LastSignalGameState.h"

void ALastSignalGameState::BeginPlay()
{
	Super::BeginPlay();
}

void ALastSignalGameState::AddKillCount() //  킬카운트 
{
    KillCount++;

	if (OnKillCountChanged.IsBound())
    {
        OnKillCountChanged.Broadcast(KillCount);
    }
}