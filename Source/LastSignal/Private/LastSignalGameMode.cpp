#include "LastSignalGameMode.h"
#include "LastSignalGameState.h"
#include "LastSignalGameInstance.h"
#include "PlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"

#include "LastSignalPlayerController.h"
#include "LastSignalPlayerHUDComponent.h"
#include "MainMenuWidget.h"

ALastSignalGameMode::ALastSignalGameMode()
{
	GameStateClass = ALastSignalGameState::StaticClass();

	// 레벨별 시작 목표 문구 (BP_LastSignalGameMode Class Defaults에서 수정 가능)
	LevelObjectives.Add(TEXT("L_SafeZone_Spawn"), FText::FromString(TEXT("구조 신호를 따라 지하철로 이동하자")));
	LevelObjectives.Add(TEXT("L_Subway"), FText::FromString(TEXT("지하철을 통과해 목표 지점으로 이동하자")));
	LevelObjectives.Add(TEXT("L_SafeZone_Building"), FText::FromString(TEXT("무전기를 찾아라")));
	LevelObjectives.Add(TEXT("Industrial_Warehouse"), FText::FromString(TEXT("무전기를 찾아라"))); // 새 안전지대 맵 (Scene_Warehouse/Maps)
	LevelObjectives.Add(TEXT("L_RuinedBuilding"), FText::FromString(TEXT("제한시간 내에 옥상으로 올라가자")));
	LevelObjectives.Add(TEXT("L_Rooftop"), FText::FromString(TEXT("헬기 착륙 지점으로 이동하자")));
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

void ALastSignalGameMode::InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // 첫 레벨로 들어오면 새 게임 → 이전 판 저장값 초기화
    // (InitGame은 모든 액터 BeginPlay보다 먼저 실행 → 캐릭터가 무기/탄약을 복원하기 전에 지워짐)
    ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>();
    if (UGameplayStatics::GetCurrentLevelName(this) == TEXT("L_SafeZone_Spawn"))
    {
        if (CurrentGameInstance)
        {
            CurrentGameInstance->ResetSaveData();
            CurrentGameInstance->GameStartRealSeconds = FPlatformTime::Seconds(); // 크레딧의 PLAY TIME 기준
            UE_LOG(LogTemp, Log, TEXT("[GameMode] 새 게임 시작 - GameInstance 저장값 초기화"));
        }
    }
    else if (CurrentGameInstance && CurrentGameInstance->GameStartRealSeconds <= 0.0)
    {
        CurrentGameInstance->GameStartRealSeconds = FPlatformTime::Seconds(); // 중간 레벨에서 바로 PIE를 시작한 경우
    }
}

void ALastSignalGameMode::ShowCredits()
{
    APlayerController *PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
        return;

    UCreditsWidget *Credits = CreateWidget<UCreditsWidget>(PlayerController, UCreditsWidget::StaticClass());
    if (!Credits)
        return;

    if (const ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
        Credits->PlayTimeSeconds = static_cast<float>((ClearRealSeconds > 0.0 ? ClearRealSeconds : FPlatformTime::Seconds()) - CurrentGameInstance->GameStartRealSeconds);
    if (const ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
        Credits->KillCount = CurrentGameState->KillCount;

    Credits->AddToViewport(50);
}

void ALastSignalGameMode::BeginPlay()
{
	Super::BeginPlay();

    ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>();

    // 레벨별 배경음악: 인트로(스폰) 작게 → 지하철~폐건물은 음악 없음 → 옥상 (메인메뉴는 메인메뉴 위젯이 직접 틂)
    if (CurrentGameInstance)
    {
        const FString LevelName = UGameplayStatics::GetCurrentLevelName(this);
        if (LevelName == TEXT("L_SafeZone_Spawn"))
            CurrentGameInstance->PlayMusic(ULastSignalGameInstance::LoadLoopingSound(TEXT("/Game/Sounds/Music/BGM_Intro.BGM_Intro")), 2.0f, 0.3f); // 나레이션이 잘 들리게 작게
        else if (LevelName == TEXT("L_Rooftop"))
            CurrentGameInstance->PlayMusic(ULastSignalGameInstance::LoadLoopingSound(TEXT("/Game/Sounds/Music/BGM_Rooftop.BGM_Rooftop")), 3.0f, 0.4f);
        else if (LevelName != TEXT("L_MainMenu"))
            CurrentGameInstance->StopMusic(3.0f);
    }

    APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); // PlayerCharacter를 if문 밖에서 미리 캐스팅 — 아래 HP복원이랑 OnDied 구독 둘 다에서 씀 (중복 캐스팅 방지)

    if (CurrentPlayerCharacter) 
        CurrentPlayerCharacter->OnDied.AddDynamic(this, &ALastSignalGameMode::OnPlayerDied); // HP 0 이벤트 구독은 세이브 데이터 유무랑 상관없이 매번 걸려야 함 (그래서 아래 if(CurrentGameInstance) 블록 밖에 둠)
    
if (CurrentGameInstance) // 저장된 값이 있으면 KillCount/HP부터 복원
    {
        if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
            CurrentGameState->KillCount = CurrentGameInstance->SavedKillCount;

        if (CurrentPlayerCharacter)
            CurrentPlayerCharacter->SetCurrentHealth(CurrentGameInstance->SavedHP);  // 위에서 이미 캐스팅한 CurrentPlayerCharacter 재사용
    
        if (ULastSignalPlayerHUDComponent *HUD = GetLocalHUDComponent())
        {
            HUD->SetHP(CurrentGameInstance->SavedHP);
            HUD->AddScore(CurrentGameInstance->SavedKillCount); // 이전 레벨 점수 이어받기 (HUD Score는 레벨마다 0부터 시작)
        }

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
    // 레벨 넘어가기 직전, 지금 상태를 GameInstance에 스냅샷으로 저장 (GameInstance만 레벨 전환에서 안 죽고 살아남음)
    if (ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
    {
        if (ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
            CurrentGameInstance->SavedKillCount = CurrentGameState->KillCount;
        SaveTimerToGameInstance();

        // 지금 HP도 같이 저장 — 안 그러면 다음 레벨에서 무조건 풀피로 리스폰됨
        if (APlayerCharacter *CurrentPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            CurrentGameInstance->SavedHP = CurrentPlayerCharacter->GetCurrentHealth();
    }
    UGameplayStatics::OpenLevel(this, NextLevel);
}

void ALastSignalGameMode::SaveTimerToGameInstance()
{
    ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>();
    const ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>();
    if (!CurrentGameInstance || !CurrentGameState)
        return;

    // 탈출 타이머는 0이 되면 클리어라서 카운트다운으로 이어 붙이면 안 됨 → 레벨 시작 때 저장값 그대로 둠
    if (CurrentGameState->TimerMode == ETimerMode::Escape)
        return;
    // 카운트다운이 0이 돼서 죽은 거면 0부터 이어 붙이면 바로 또 게임오버 → 레벨 시작 때 저장값 그대로 둠
    if (CurrentGameState->TimerMode == ETimerMode::Countdown && CurrentGameState->TimerValue <= 0.f)
        return;

    // 지금이 스톱워치 단계인지 카운트다운 단계인지에 따라, 다음 레벨(또는 RETRY)에서 뭘로 이어야 하는지가 갈림
    CurrentGameInstance->bTimeLimitStarted = (CurrentGameState->TimerMode != ETimerMode::Stopwatch);

    if (CurrentGameState->TimerMode == ETimerMode::Stopwatch)
        CurrentGameInstance->SavedPlayTime = CurrentGameState->TimerValue; // 스톱워치였으면 흐른 시간 저장
    else
        CurrentGameInstance->SavedRemainingTime = CurrentGameState->TimerValue; // 카운트다운이었으면 남은 시간 저장
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

    // 인트로/엔딩 슬라이드쇼 + 인트로 뒤 페이드인 중에는 시간 멈춤
    if (const ALastSignalPlayerController *PlayerController = Cast<ALastSignalPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
        if (PlayerController->IsInCutscene())
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
    APlayerController *CurrentPlayerController = GetWorld()->GetFirstPlayerController(); // 위젯 만들고 입력모드 바꾸려면 PlayerController 필요 , PlayerController를 가져오는 것
    if (!CurrentPlayerController)
        return;

    // 코드로 만든 게임오버 화면(UGameOverWidget)을 쓴다. GameOverClass(WBP_GameOver)는 더 이상 사용 안 함
    UGameOverWidget *GameOverWidget = CreateWidget<UGameOverWidget>(CurrentPlayerController, UGameOverWidget::StaticClass());
    if (!GameOverWidget)
        return;

    // 기록 (SURVIVED, KILLS) — 크레딧과 같은 계산
    if (const ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
        GameOverWidget->PlayTimeSeconds = static_cast<float>(FPlatformTime::Seconds() - CurrentGameInstance->GameStartRealSeconds);
    if (const ALastSignalGameState *CurrentGameState = GetGameState<ALastSignalGameState>())
        GameOverWidget->KillCount = CurrentGameState->KillCount;

    GetWorldTimerManager().ClearTimer(TimerHandle);            // 게임오버 순간 시계(스톱워치/카운트다운) 멈춤
    GameOverWidget->AddToViewport(50);                         // 손목시계, HUD보다 위 → 블러가 시계까지 덮음
    CurrentPlayerController->SetInputMode(FInputModeUIOnly()); // 조작 입력을 UI(버튼)로만 받게 전환
    CurrentPlayerController->bShowMouseCursor = true;          // 버튼 클릭하려면 마우스 커서 보여야 함
}

void ALastSignalGameMode::OnEscapeSuccess() // 탈출 타이머 끝나면 클리어 함수 구현 (엔딩 연출은 나중에 결정)
{
    ClearRealSeconds = FPlatformTime::Seconds(); // 플레이 시간은 여기서 끝 (시계는 UpdateTimer에서 이미 멈춤)

    // 옥상 레벨에서 나던 소리(헬기, 좀비 등)는 전부 서서히 끔 → 엔딩 나레이션이 잘 들리게
    // (나레이션은 아래 OnEndingStarted 이후에 새로 재생되니까 영향 없음. 음악/환경음은 액터 소속이 아니라 여기 안 걸림)
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        TInlineComponentArray<UAudioComponent *> AudioComponents(*It);
        for (UAudioComponent *Audio : AudioComponents)
        {
            if (Audio->IsPlaying())
                Audio->FadeOut(1.5f, 0.0f);
        }
    }

    // 엔딩 음악 + 멀어지는 헬기 소리 (나레이션을 가리지 않게 작게 시작해서 20초 동안 더 작아짐). 크레딧이 뜨면 크레딧 음악으로 교체됨
    if (ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
    {
        CurrentGameInstance->PlayMusic(ULastSignalGameInstance::LoadLoopingSound(TEXT("/Game/Sounds/Music/BGM_Ending.BGM_Ending")), 2.0f, 0.35f);
        CurrentGameInstance->PlayAmbience(ULastSignalGameInstance::LoadLoopingSound(TEXT("/Game/Sounds/Helicopter.Helicopter")), 0.2f, 20.0f, 0.03f);
    }

    OnEndingStarted(); // BP로 신호 → BP_LastSignalGameMode의 Event On Ending Started 실행
}

void ALastSignalGameMode::OnPlayerDied() // 플레이어 HP 0 = 게임오버, OnGameOver 재사용
{
    // 새로 로직 안 만들고 그대로 위임 (게임오버 위젯 표시)
    OnGameOver();
}