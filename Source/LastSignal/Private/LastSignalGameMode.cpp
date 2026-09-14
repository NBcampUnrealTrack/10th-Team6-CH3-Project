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

void ALastSignalGameMode::StartStopwatch() // 스톱워치 시작
{
    if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
    {
        CurrentGameState->TimerMode = ETimerMode::Stopwatch;
        CurrentGameState->TimerValue = 0.f;

        GetWorldTimerManager().SetTimer(TimerHandle, this, &ALastSignalGameMode::UpdateTimer, 1.0f, true);
    }
}

void ALastSignalGameMode::StartCountdown(float DurationSeconds) // 15분 카운트다운 시작 (15분 일단 임의값으로 지정했어요)
{
    if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
    {
        CurrentGameState->TimerMode = ETimerMode::Countdown;
        CurrentGameState->TimerValue = DurationSeconds;

        GetWorldTimerManager().SetTimer(TimerHandle, this, &ALastSignalGameMode::UpdateTimer, 1.0f, true);
    }
}

void ALastSignalGameMode::StartEscapeTimer(float DurationSeconds) // 3분 탈출 타이머 시작 (3분도 임의값입니다)
{
    if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
    {
        CurrentGameState->TimerMode = ETimerMode::Escape;
        CurrentGameState->TimerValue = DurationSeconds;

        GetWorldTimerManager().SetTimer(TimerHandle, this, &ALastSignalGameMode::UpdateTimer, 1.0f, true);
    }
}

void ALastSignalGameMode::UpdateTimer() // 1초마다 실행되는 실제 갱신 로직
{
    ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>();
    if (!CurrentGameState)
        return;

    switch (CurrentGameState->TimerMode)
    {
    case ETimerMode::Stopwatch:
        CurrentGameState->TimerValue += 1.f;
        break;

    case ETimerMode::Countdown:
        CurrentGameState->TimerValue -= 1.f;
        if (CurrentGameState->TimerValue <= 0.f)
        {
            CurrentGameState->TimerValue = 0.f; // 게임오버 처리 연결 필요
        }
        break;

    case ETimerMode::Escape:
        CurrentGameState->TimerValue -= 1.f;
        if (CurrentGameState->TimerValue <= 0.f)
        {
            CurrentGameState->TimerValue = 0.f; // // 클리어 처리 연결 필요
        }
        break;
    }
}