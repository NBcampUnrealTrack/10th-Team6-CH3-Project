#include "SkillComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == ESkillState::Active || CurrentState == ESkillState::Ready)
    {
        return;
    }

    if (CurrentState == ESkillState::Charging)
    {
        CurrentSkillValue += ChargeRatePerSecond * DeltaTime;
        CurrentSkillValue = FMath::Min(CurrentSkillValue, MaxSkillValue);

        OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

        if (CurrentSkillValue >= MaxSkillValue)
        {
            CurrentState = ESkillState::Ready;
            OnSkillStateChanged.Broadcast(CurrentState);
        }
    }
}

void USkillComponent::OnZombieKilled()
{
    if (CurrentState != ESkillState::Charging) return;

    CurrentSkillValue = FMath::Min(CurrentSkillValue + KillBonusValue, MaxSkillValue);
    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    if (CurrentSkillValue >= MaxSkillValue)
    {
        CurrentState = ESkillState::Ready;
        OnSkillStateChanged.Broadcast(CurrentState);
    }
}

bool USkillComponent::CanActivateSkill() const
{
    return CurrentState == ESkillState::Ready;
}

void USkillComponent::ActivateSkill()
{
    if (!CanActivateSkill()) return;

    CurrentState = ESkillState::Active;
    CurrentSkillValue = 0.0f;

    OnSkillStateChanged.Broadcast(CurrentState);
    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SkillDurationTimerHandle, this, &USkillComponent::DeactivateSkill, SkillDuration, false);
    }
}

void USkillComponent::DeactivateSkill()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkillDurationTimerHandle);
    }

    CurrentState = ESkillState::Charging;
    OnSkillStateChanged.Broadcast(CurrentState);
}

