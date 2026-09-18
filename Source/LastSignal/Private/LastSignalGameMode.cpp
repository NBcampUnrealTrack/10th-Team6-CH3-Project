#include "LastSignalGameMode.h"
#include "LastSignalGameState.h"
#include "LastSignalGameInstance.h"
#include "PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

#include "LastSignalPlayerController.h"
#include "LastSignalPlayerHUDComponent.h"

ALastSignalGameMode::ALastSignalGameMode()
{
	GameStateClass = ALastSignalGameState::StaticClass();
}

ULastSignalPlayerHUDComponent *ALastSignalGameMode::GetLocalHUDComponent() const
{
    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (ALastSignalPlayerController *LastSignalPC = Cast<ALastSignalPlayerController>(PC))
        {
            return LastSignalPC->GetHUDComponent();
        }
    }

    return nullptr;
}

void ALastSignalGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode BeginPlay"));

    ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>();

    APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); // PlayerCharacter를 if문 밖에서 미리 캐스팅 — 아래 HP복원이랑 OnDied 구독 둘 다에서 씀 (중복 캐스팅 방지)

    if (CurrentPlayerCharacter) 
        CurrentPlayerCharacter->OnDied.AddDynamic(this, &ALastSignalGameMode::OnPlayerDied); // HP 0 이벤트 구독은 세이브 데이터 유무랑 상관없이 매번 걸려야 함 (그래서 아래 if(CurrentGameInstance) 블록 밖에 둠)
    
if (CurrentGameInstance) // 저장된 값이 있으면 KillCount/HP부터 복원
    {
        if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
            CurrentGameState->KillCount = CurrentGameInstance->SavedKillCount;

        if (CurrentPlayerCharacter)
            CurrentPlayerCharacter->SetCurrentHealth(CurrentGameInstance->SavedHP);  // 위에서 이미 캐스팅한 CurrentPlayerCharacter 재사용
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

        if (ULastSignalPlayerHUDComponent *HUD = GetLocalHUDComponent())
        {
            HUD->UpdateTimerFromGameState(0.f, ELastSignalTimerMode::Stopwatch);
        }
    }
}

void ALastSignalGameMode::StartCountdown(float DurationSeconds) // 15분 카운트다운 시작 (15분 일단 임의값으로 지정했어요)
{
    if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
    {
        CurrentGameState->TimerMode = ETimerMode::Countdown;
        CurrentGameState->TimerValue = DurationSeconds;

        GetWorldTimerManager().SetTimer(TimerHandle, this, &ALastSignalGameMode::UpdateTimer, 1.0f, true);

        if (ULastSignalPlayerHUDComponent *HUD = GetLocalHUDComponent())
        {
            HUD->UpdateTimerFromGameState(DurationSeconds, ELastSignalTimerMode::Countdown);
        }
    }
}

void ALastSignalGameMode::StartEscapeTimer(float DurationSeconds) // 3분 탈출 타이머 시작 (3분도 임의값입니다)
{
    if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
    {
        CurrentGameState->TimerMode = ETimerMode::Escape;
        CurrentGameState->TimerValue = DurationSeconds;

        GetWorldTimerManager().SetTimer(TimerHandle, this, &ALastSignalGameMode::UpdateTimer, 1.0f, true);

        if (ULastSignalPlayerHUDComponent *HUD = GetLocalHUDComponent())
        {
            HUD->UpdateTimerFromGameState(DurationSeconds, ELastSignalTimerMode::Countdown);
        }
    }
}

void ALastSignalGameMode::UpdateTimer() // 1초마다 실행되는 실제 갱신 로직
{
    ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>();
    if (!CurrentGameState)
        return;

     if (GEngine) // 임시: TimerValue/KillCount/HP 확인용
    {
        float CurrentHP = 0.f;
        if (APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            CurrentHP = CurrentPlayerCharacter->GetCurrentHealth();

        GEngine->AddOnScreenDebugMessage(1, 1.1f, FColor::White,
            FString::Printf(TEXT("Timer: %.0f | Kill: %d | HP: %.0f"),
                CurrentGameState->TimerValue, CurrentGameState->KillCount, CurrentHP));
    }

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
            OnGameOver();
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

    if (ULastSignalPlayerHUDComponent *HUD = GetLocalHUDComponent())
    {
        const ELastSignalTimerMode DisplayMode =
            (CurrentGameState->TimerMode == ETimerMode::Stopwatch)
                ? ELastSignalTimerMode::Stopwatch
                : ELastSignalTimerMode::Countdown;

        HUD->UpdateTimerFromGameState(CurrentGameState->TimerValue, DisplayMode);
    }
}

void ALastSignalGameMode::OnGameOver() // 게임오버 처리 (카운트다운 실패 / 플레이어 사망 공용)
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GAME OVER"));

    if (!GameOverClass) // 위젯 BP 안 지정돼 있으면 그냥 무시 (BP_LastSignalGameMode에 아직 설정 안 했을 때 대비)
        return;

    APlayerController *CurrentPlayerController = GetWorld()->GetFirstPlayerController(); // 위젯 만들고 입력모드 바꾸려면 PlayerController 필요 , PlayerController를 가져오는 것
    if (!CurrentPlayerController)
        return;

    UUserWidget *GameOverWidget = CreateWidget<UUserWidget>(CurrentPlayerController, GameOverClass); // GameOverClass에 꽂힌 WBP_GameOver로 위젯 인스턴스 생성
    if (!GameOverWidget)
        return;

    GameOverWidget->AddToViewport();                           // 실제 화면에 그려지게 뷰포트에 추가
    CurrentPlayerController->SetInputMode(FInputModeUIOnly()); // 조작 입력을 UI(버튼)로만 받게 전환
    CurrentPlayerController->bShowMouseCursor = true;          // 버튼 클릭하려면 마우스 커서 보여야 함
}

void ALastSignalGameMode::OnEscapeSuccess() // 탈출 타이머 끝나면 클리어 함수 구현 (엔딩 연출은 나중에 결정)
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ESCAPE SUCCESS!"));
}

void ALastSignalGameMode::OnPlayerDied() // 플레이어 HP 0 = 게임오버, OnGameOver 재사용
{
    // 새로 로직 안 만들고 그대로 위임 (게임오버 위젯 표시)
    OnGameOver();
}