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
        }
    }
}

void USkillComponent::OnZombieKilled()
{
    // 좀비 사망 이벤트 바인딩 지점 Charging 상태일 때만 게이지 점수 합산

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
    // 플레이어 입력을 통해서 스킬 사용 조건 검사

    return CurrentState == ESkillState::Ready;
}

void USkillComponent::ActivateSkill()
{
    if (!CanActivateSkill()) return;

    // 스킬 발동 시 게이지 0% 고정 하고 타이어 시작한다.

    CurrentState = ESkillState::Active;
    CurrentSkillValue = 0.0f;

    OnSkillStateChanged.Broadcast(CurrentState);
    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    // SkillDuration 초 후 DeactivateSkill 자동 호출한다.

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SkillDurationTimerHandle, this, &USkillComponent::DeactivateSkill, SkillDuration, false);
    }
}

void USkillComponent::DeactivateSkill()
{
    // 지속시간 종료 또는 캐릭터 사망할시 스킬 상태 리셋
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkillDurationTimerHandle);
    }

    CurrentState = ESkillState::Charging;
    OnSkillStateChanged.Broadcast(CurrentState);
}

