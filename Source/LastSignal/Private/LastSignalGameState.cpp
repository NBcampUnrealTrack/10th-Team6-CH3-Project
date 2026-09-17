#include "LastSignalGameState.h"

void ALastSignalGameState::BeginPlay()
{
	Super::BeginPlay();
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("GameState BeginPlay")); // 테스트 로직입니다.
}

void ALastSignalGameState::AddKillCount() //  킬카운트 
{
    KillCount++;

	if (OnKillCountChanged.IsBound())
    {
        OnKillCountChanged.Broadcast(KillCount);
    }
}