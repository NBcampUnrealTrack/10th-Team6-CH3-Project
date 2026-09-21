#include "SkillComponent.h"
#include "LastSignalGameState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LastSignalGameInstance.h"
#include "TimerManager.h"

// 디버그 출력용
void DisplaySkillStateDebug(ESkillState NewState)
{
    if (!GEngine)
        return;

    FString DebugMessage;
    FColor MessageColor = FColor::White;

    switch (NewState)
    {
    case ESkillState::Charging:
        DebugMessage = TEXT("스킬 상태: 충전 중...");
        MessageColor = FColor::Yellow;
        break;

    case ESkillState::Ready:
        DebugMessage = TEXT("스킬 준비 완료~!");
        MessageColor = FColor::Green;
        break;

    case ESkillState::Active:
        DebugMessage = TEXT("스킬 발동 중!");
        MessageColor = FColor::Red;
        break;
    }

    GEngine->AddOnScreenDebugMessage(1, 5.0f, MessageColor, DebugMessage);
}

USkillComponent::USkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    CurrentSkillValue = 0.0f;     // 처음 스킬 게이지
    MaxSkillValue = 100.0f;       // 최대 스킬 게이지
    SkillDuration = 20.0f;       // 지속 시간
    ChargeRatePerSecond = 1.0f;  // 초당 패시브 충전량
    KillBonusValue = 5.0f;      // 킬카운트 1당 충전량

    CurrentState = ESkillState::Charging;
}

void USkillComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld *World = GetWorld())
    {
        if (ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(World->GetGameInstance()))
        {
            CurrentSkillValue = GI->SavedSkillGauge;

            if (CurrentSkillValue >= MaxSkillValue)
            {
                CurrentSkillValue = MaxSkillValue;
                CurrentState = ESkillState::Ready;
            }

            OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);
            OnSkillStateChanged.Broadcast(CurrentState);
        }

        if (ALastSignalGameState *GS = World->GetGameState<ALastSignalGameState>())
        {
            GS->OnKillCountChanged.AddDynamic(this, &USkillComponent::HandleZombieKilled);
        }
    }
}

void USkillComponent::HandleZombieKilled(int32 NewKillCount)
{
    OnZombieKilled();
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Active 상태이거나 Ready 상태일 때는 패시브 게이지 충전을 일시 정지

    if (CurrentState == ESkillState::Active || CurrentState == ESkillState::Ready)
    {
        return;
    }

    // Charging 상태시 초 마다 패시브 충전 진행

    if (CurrentState == ESkillState::Charging)
    {
        CurrentSkillValue += ChargeRatePerSecond * DeltaTime;
        CurrentSkillValue = FMath::Min(CurrentSkillValue, MaxSkillValue);

        OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

        if (CurrentSkillValue >= MaxSkillValue)
        {
            CurrentState = ESkillState::Ready;
            OnSkillStateChanged.Broadcast(CurrentState);
            DisplaySkillStateDebug(CurrentState); // 디버그 텍스트 출력용
        }
    }
}

void USkillComponent::OnZombieKilled()
{

    if (CurrentState != ESkillState::Charging)
        return;

    CurrentSkillValue = FMath::Min(CurrentSkillValue + KillBonusValue, MaxSkillValue);

    if (UWorld *World = GetWorld())
    {
        if (ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(World->GetGameInstance()))
        {
            GI->SavedSkillGauge = CurrentSkillValue;
        }
    }

    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    if (CurrentSkillValue >= MaxSkillValue)
    {
        CurrentState = ESkillState::Ready;
        OnSkillStateChanged.Broadcast(CurrentState);
        DisplaySkillStateDebug(CurrentState);
    }
}

bool USkillComponent::CanActivateSkill() const
{
    return CurrentState == ESkillState::Ready;
}

void USkillComponent::ActivateSkill()
{
    FString StateStr;
    switch (CurrentState)
    {
    case ESkillState::Charging:
        StateStr = TEXT("Charging(충전 중)");
        break;
    case ESkillState::Ready:
        StateStr = TEXT("Ready(준비 완료)");
        break;
    case ESkillState::Active:
        StateStr = TEXT("Active(발동 중)");
        break;
    }

    // Output Log 출력
    UE_LOG(LogTemp, Warning, TEXT("[SkillInput] 스킬 키 입력 받음! | 현재 상태: %s | 게이지: %.1f / %.1f"),
           *StateStr, CurrentSkillValue, MaxSkillValue);

    if (!CanActivateSkill())
    {
        FString FailReason = (CurrentState == ESkillState::Active) ? TEXT("이미 스킬이 활성화 상태입니다.") : TEXT("스킬 게이지가 모자랍니다.");
        UE_LOG(LogTemp, Error, TEXT("[SkillInput] 스킬 발동 실패! -> 사유: %s"), *FailReason);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SkillInput] ★ 스킬 발동 성공! ★"));

    CurrentState = ESkillState::Active;
    CurrentSkillValue = 0.0f;

    if (UWorld *World = GetWorld())
    {
        if (ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(World->GetGameInstance()))
        {
            GI->SavedSkillGauge = CurrentSkillValue;
        }
    }

    OnSkillStateChanged.Broadcast(CurrentState);
    DisplaySkillStateDebug(CurrentState);
    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SkillDurationTimerHandle, this, &USkillComponent::DeactivateSkill, SkillDuration, false);
    }
}

void USkillComponent::DeactivateSkill()
{
    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkillDurationTimerHandle);
    }

    CurrentState = ESkillState::Charging;
    OnSkillStateChanged.Broadcast(CurrentState);
    DisplaySkillStateDebug(CurrentState);
}

