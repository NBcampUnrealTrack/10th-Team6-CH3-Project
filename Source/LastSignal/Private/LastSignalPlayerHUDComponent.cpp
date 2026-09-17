// LastSignalPlayerHUDComponent.cpp

#include "LastSignalPlayerHUDComponent.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

ULastSignalPlayerHUDComponent::ULastSignalPlayerHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULastSignalPlayerHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ---- 타이머 갱신 (스톱워치 : 증가 / 카운트다운 : 감소) ----
	if (TimerMode == ELastSignalTimerMode::Stopwatch)
	{
		TimeValue += DeltaTime;
	}
	else // Countdown
	{
		TimeValue = FMath::Max(0.f, TimeValue - DeltaTime);
		if (TimeValue <= 0.f)
		{
			TriggerGameOver();
		}
	}
	OnTimerUpdated.Broadcast(TimeValue, TimerMode);

	// ---- 특수공격(아드레날린) 게이지는 시간이 지날수록 자동 상승 ----
	if (!bSpecialAttackActive && SpecialGaugePercent < 100.f)
	{
		SpecialGaugePercent = FMath::Clamp(SpecialGaugePercent + GaugePerSecond * DeltaTime, 0.f, 100.f);
		OnSpecialGaugeChanged.Broadcast(SpecialGaugePercent);
	}
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

void ULastSignalPlayerHUDComponent::RegisterKill(FText VictimName, int32 ScoreForKill)
{
	KillCount++;
	AddScore(ScoreForKill);

	SpecialGaugePercent = FMath::Clamp(SpecialGaugePercent + GaugePerKill, 0.f, 100.f);
	OnSpecialGaugeChanged.Broadcast(SpecialGaugePercent);

	// 좌하단이 아니라 킬확정 로그(킬로그) - 몬스터 처치 시 노출 후 Fade-out
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
