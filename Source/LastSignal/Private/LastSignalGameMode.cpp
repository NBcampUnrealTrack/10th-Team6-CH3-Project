#include "LastSignalGameMode.h"
#include "LastSignalGameState.h"
#include "Kismet/GameplayStatics.h"

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

void ALastSignalGameMode::OnGoalReached(FName NextLevel) // 클리어 트리거
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("CLEAR!"));

		UGameplayStatics::OpenLevel(this, NextLevel); // 넘겨받은 이름의 레벨을 오픈한다.
}

void ALastSignalGameMode::OnZombieKilled() // 좀비 킬 카운트 추가 구현
{
        if (ALastSignalGameState* CurrentGameState = GetGameState<ALastSignalGameState>())
	{
        CurrentGameState->AddKillCount();
	}
}