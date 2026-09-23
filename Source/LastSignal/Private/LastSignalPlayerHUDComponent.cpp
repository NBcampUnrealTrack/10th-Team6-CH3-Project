// LastSignalPlayerHUDComponent.cpp

#include "LastSignalPlayerHUDComponent.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

ULastSignalPlayerHUDComponent::ULastSignalPlayerHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULastSignalPlayerHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void ULastSignalPlayerHUDComponent::ApplyDamage(float DamageAmount)
{
	CurrentHP = FMath::Clamp(CurrentHP - DamageAmount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.f)
	{
		TriggerGameOver();
	}
}

void ULastSignalPlayerHUDComponent::HealToFull()
{
	// [안전지대:재정비] 진입 시 HP 100% 회복
	CurrentHP = MaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void ULastSignalPlayerHUDComponent::SetHP(float NewHP)
{
    CurrentHP = FMath::Clamp(NewHP, 0.f, MaxHP);
    OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void ULastSignalPlayerHUDComponent::SetWeapon(const FLastSignalWeaponHUDData& NewWeapon)
{
	CurrentWeapon = NewWeapon;
	OnAmmoChanged.Broadcast(CurrentWeapon);
}

void ULastSignalPlayerHUDComponent::ConsumeAmmo(int32 Amount)
{
	CurrentWeapon.CurrentAmmo = FMath::Max(0, CurrentWeapon.CurrentAmmo - Amount);
	OnAmmoChanged.Broadcast(CurrentWeapon);
}

void ULastSignalPlayerHUDComponent::ReloadWeapon()
{
	CurrentWeapon.CurrentAmmo = CurrentWeapon.MagazineSize;
	OnAmmoChanged.Broadcast(CurrentWeapon);
}

void ULastSignalPlayerHUDComponent::AddScore(int32 Amount)
{
	Score += Amount;
	OnScoreChanged.Broadcast(Score);
}

void ULastSignalPlayerHUDComponent::RegisterKill(const FText &VictimName)
{
    // Score를 처치 수로 사용
    Score++;

    OnScoreChanged.Broadcast(Score);

    // 특수공격 게이지 증가
    SpecialGaugePercent =
        FMath::Clamp(
            SpecialGaugePercent + GaugePerKill,
            0.f,
            100.f);

    OnSpecialGaugeChanged.Broadcast(SpecialGaugePercent);

    // 킬로그
    OnKillConfirmed.Broadcast(VictimName);
}

void ULastSignalPlayerHUDComponent::SetMissionObjective(FText NewObjective)
{
	OnMissionObjectiveChanged.Broadcast(NewObjective);
}

void ULastSignalPlayerHUDComponent::SwitchToCountdown(float CountdownSeconds)
{
	TimerMode = ELastSignalTimerMode::Countdown;
	TimeValue = CountdownSeconds;
	OnTimerModeChanged.Broadcast(TimerMode);
	OnTimerUpdated.Broadcast(TimeValue, TimerMode);
}

void ULastSignalPlayerHUDComponent::ResetStopwatch()
{
	TimerMode = ELastSignalTimerMode::Stopwatch;
	TimeValue = 0.f;
	OnTimerModeChanged.Broadcast(TimerMode);
	OnTimerUpdated.Broadcast(TimeValue, TimerMode);
}

void ULastSignalPlayerHUDComponent::UpdateTimerFromGameState(float NewTimeValue, ELastSignalTimerMode NewMode)
{
    if (TimerMode != NewMode)
    {
        TimerMode = NewMode;
        OnTimerModeChanged.Broadcast(TimerMode);
    }

    TimeValue = FMath::Max(0.f, NewTimeValue);
    OnTimerUpdated.Broadcast(TimeValue, TimerMode);
}

bool ULastSignalPlayerHUDComponent::TryActivateSpecialAttack()
{
	if (bSpecialAttackActive || SpecialGaugePercent < 100.f)
	{
		return false;
	}

	bSpecialAttackActive = true;
	SpecialGaugePercent = 0.f;
	OnSpecialGaugeChanged.Broadcast(SpecialGaugePercent);
	OnSpecialAttackActivated.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(
		SpecialAttackTimerHandle,
		this,
		&ULastSignalPlayerHUDComponent::EndSpecialAttack,
		SpecialAttackDuration,
		false);

	return true;
}

void ULastSignalPlayerHUDComponent::EndSpecialAttack()
{
	bSpecialAttackActive = false;
	OnSpecialAttackEnded.Broadcast();
}

void ULastSignalPlayerHUDComponent::RequestHitMarker()
{
	OnHitMarkerRequested.Broadcast();
}

void ULastSignalPlayerHUDComponent::TriggerGameOver()
{
	OnGameOverRequested.Broadcast();
}

void ULastSignalPlayerHUDComponent::SetInteractPromptVisible(bool bVisible)
{
    OnInteractPromptChanged.Broadcast(bVisible);
}
