#include "LastSignalGameMode.h"
#include "LastSignalGameState.h"
#include "LastSignalGameInstance.h"
#include "PlayerCharacter.h"
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

    ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>();
    
if (CurrentGameInstance) // 저장된 값이 있으면 KillCount/HP부터 복원
    {
        if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
            CurrentGameState->KillCount = CurrentGameInstance->SavedKillCount;

        if (APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            CurrentPlayerCharacter->SetCurrentHealth(CurrentGameInstance->SavedHP);
    }

    if (CurrentGameInstance && CurrentGameInstance->bTimeLimitStarted)
    {
        StartCountdown(CurrentGameInstance->SavedRemainingTime); // 카운트다운 흐르던 중이었으면 이어서 시작
    }
    else
    {
        StartStopwatch(); // 게임시작하고 스톱워치 시작

        if (CurrentGameInstance)
        {
            if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
                CurrentGameState->TimerValue = CurrentGameInstance->SavedPlayTime; // 이어서 흐르던 플레이타임 복구
        }
    }
}

void ALastSignalGameMode::OnGoalReached(FName NextLevel) // 클리어 트리거
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("CLEAR!"));

    // 레벨 넘어가기 직전, 지금 상태를 GameInstance에 스냅샷으로 저장 (GameInstance만 레벨 전환에서 안 죽고 살아남음)
    if (ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
    {
        if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
        {
            CurrentGameInstance->SavedKillCount = CurrentGameState->KillCount;

            // 지금이 스톱워치 단계인지 카운트다운/탈출 단계인지에 따라, 다음 레벨에서 뭘로 이어야 하는지가 갈림
            CurrentGameInstance->bTimeLimitStarted = (CurrentGameState->TimerMode != ETimerMode::Stopwatch);

            if (CurrentGameState->TimerMode == ETimerMode::Stopwatch)
                CurrentGameInstance->SavedPlayTime = CurrentGameState->TimerValue; // 스톱워치였으면 흐른 시간 저장
            else
                CurrentGameInstance->SavedRemainingTime = CurrentGameState->TimerValue; // 카운트다운/탈출이었으면 저장
        }

        // 지금 HP도 같이 저장 — 안 그러면 다음 레벨에서 무조건 풀피로 리스폰됨
        if (APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            CurrentGameInstance->SavedHP = CurrentPlayerCharacter->GetCurrentHealth();
    }
    UGameplayStatics::OpenLevel(this, NextLevel);
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
            CurrentGameState->TimerValue = 0.f;

            GetWorldTimerManager().ClearTimer(TimerHandle); // 0 이후에도 계속 호출되는거 방지
            OnCountdownFailed();
        }
        break;

    case ETimerMode::Escape:
        CurrentGameState->TimerValue -= 1.f;
        if (CurrentGameState->TimerValue <= 0.f)
        {
            CurrentGameState->TimerValue = 0.f;
        
            GetWorldTimerManager().ClearTimer(TimerHandle); // 0 이후에도 계속 호출되는 거 방지
            OnEscapeSuccess();
        }
        break;
    }
}

void ALastSignalGameMode::OnCountdownFailed() // 카운트다운 끝나면 게임 오버 함수 구현
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GAME OVER"));

    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this))); // 현재 레벨 재시작
}

void ALastSignalGameMode::OnEscapeSuccess() // 탈출 타이머 끝나면 클리어 함수 구현 (엔딩 연출은 나중에 결정)
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ESCAPE SUCCESS!"));
}