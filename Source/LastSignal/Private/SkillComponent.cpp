#include "SkillComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

USkillComponent::USkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    CurrentSkillValue = 0.0f;     // ó�� ��ų ������
    MaxSkillValue = 100.0f;       // �ִ� ��ų ������
    SkillDuration = 20.0f;       // ���� �ð�
    ChargeRatePerSecond = 1.0f;  // �ʴ� �нú� ������
    KillBonusValue = 5.0f;      // ųī��Ʈ 1�� ������

    CurrentState = ESkillState::Charging;
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Active �����̰ų� Ready ������ ���� �нú� ������ ������ �Ͻ� ����

    if (CurrentState == ESkillState::Active || CurrentState == ESkillState::Ready)
    {
        return;
    }

    // Charging ���½� �� ���� �нú� ���� ����

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
    // ���� ��� �̺�Ʈ ���ε� ���� Charging ������ ���� ������ ���� �ջ�

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
    // �÷��̾� �Է��� ���ؼ� ��ų ��� ���� �˻�

    return CurrentState == ESkillState::Ready;
}

void USkillComponent::ActivateSkill()
{
    if (!CanActivateSkill()) return;

    // ��ų �ߵ� �� ������ 0% ���� �ϰ� Ÿ�̾� �����Ѵ�.

    CurrentState = ESkillState::Active;
    CurrentSkillValue = 0.0f;

    OnSkillStateChanged.Broadcast(CurrentState);
    OnSkillValueChanged.Broadcast(CurrentSkillValue, MaxSkillValue);

    // SkillDuration �� �� DeactivateSkill �ڵ� ȣ���Ѵ�.

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SkillDurationTimerHandle, this, &USkillComponent::DeactivateSkill, SkillDuration, false);
    }
}

void USkillComponent::DeactivateSkill()
{
    // ���ӽð� ���� �Ǵ� ĳ���� ����ҽ� ��ų ���� ����
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkillDurationTimerHandle);
    }

    CurrentState = ESkillState::Charging;
    OnSkillStateChanged.Broadcast(CurrentState);
}

