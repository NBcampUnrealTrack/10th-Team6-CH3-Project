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

void ALastSignalGameMode::OnGoalReached(FName NextLevel)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("CLEAR!"));

		UGameplayStatics::OpenLevel(this, NextLevel); // 넘겹다은 이름의 레벨을 오픈한다.
}