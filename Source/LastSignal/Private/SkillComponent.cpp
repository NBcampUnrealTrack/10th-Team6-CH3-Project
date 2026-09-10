#include "SkillComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CurrentSkillValue = 0.0f;
	MaxSkillValue = 100.0f;
	ChargeRatePerSecond = 1.0f;
	KillBonusValue = 1.0f;
	SkillDuration = 10.0f;
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