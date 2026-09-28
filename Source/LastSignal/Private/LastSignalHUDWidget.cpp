// LastSignalHUDWidget.cpp

#include "LastSignalHUDWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "Components/TextBlock.h"

void ULastSignalHUDWidget::BindHUDComponent(ULastSignalPlayerHUDComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	BoundComponent = InComponent;

	BoundComponent->OnHPChanged.AddDynamic(this, &ULastSignalHUDWidget::HandleHPChanged);
	BoundComponent->OnAmmoChanged.AddDynamic(this, &ULastSignalHUDWidget::HandleAmmoChanged);
	BoundComponent->OnScoreChanged.AddDynamic(this, &ULastSignalHUDWidget::HandleScoreChanged);
	BoundComponent->OnMissionObjectiveChanged.AddDynamic(this, &ULastSignalHUDWidget::HandleMissionObjectiveChanged);
	BoundComponent->OnTimerUpdated.AddDynamic(this, &ULastSignalHUDWidget::HandleTimerUpdated);
	BoundComponent->OnSpecialGaugeChanged.AddDynamic(this, &ULastSignalHUDWidget::HandleSpecialGaugeChanged);
	BoundComponent->OnSpecialAttackActivated.AddDynamic(this, &ULastSignalHUDWidget::HandleSpecialAttackActivated);
	BoundComponent->OnSpecialAttackEnded.AddDynamic(this, &ULastSignalHUDWidget::HandleSpecialAttackEnded);
	BoundComponent->OnKillConfirmed.AddDynamic(this, &ULastSignalHUDWidget::HandleKillConfirmed);
	BoundComponent->OnHitMarkerRequested.AddDynamic(this, &ULastSignalHUDWidget::HandleHitMarkerRequested);
	BoundComponent->OnGameOverRequested.AddDynamic(this, &ULastSignalHUDWidget::HandleGameOverRequested);

	// 초기 상태 1회 갱신 (위젯이 늦게 추가되는 경우 대비)
	HandleHPChanged(BoundComponent->CurrentHP, BoundComponent->MaxHP);
	HandleAmmoChanged(BoundComponent->CurrentWeapon);
	HandleScoreChanged(BoundComponent->Score);
	HandleTimerUpdated(BoundComponent->TimeValue, BoundComponent->TimerMode);
	HandleSpecialGaugeChanged(BoundComponent->SpecialGaugePercent);
}

void ULastSignalHUDWidget::HandleHPChanged(float CurrentHP, float MaxHP)
{
	const float Percent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
	OnHPUpdated(CurrentHP, MaxHP, Percent);
}

void ULastSignalHUDWidget::HandleAmmoChanged(FLastSignalWeaponHUDData WeaponData)
{
	OnAmmoUpdated(WeaponData);

	// BP는 "현재 / 탄창 크기"로 써서 약실 1발이 있으면 31/30처럼 보임 → "현재 탄창 / 남은 총알"로 덮어씀 (BP 수정 없이)
	if (UTextBlock *AmmoText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_AmmoCount"))))
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), WeaponData.CurrentAmmo, WeaponData.ReserveAmmo)));
}

void ULastSignalHUDWidget::HandleScoreChanged(int32 NewScore)
{
	OnScoreUpdated(NewScore);
}

void ULastSignalHUDWidget::HandleMissionObjectiveChanged(FText NewObjective)
{
	OnMissionObjectiveUpdated(NewObjective);
}

void ULastSignalHUDWidget::HandleTimerUpdated(float TimeValue, ELastSignalTimerMode Mode)
{
	const bool bRed = (Mode == ELastSignalTimerMode::Countdown);
	OnTimerUpdated(FormatTime(TimeValue), bRed);
}

void ULastSignalHUDWidget::HandleSpecialGaugeChanged(float GaugePercent)
{
	OnSpecialGaugeUpdated(GaugePercent, GaugePercent >= 100.f);
}

void ULastSignalHUDWidget::HandleSpecialAttackActivated()
{
	OnSpecialAttackActivated();
}

void ULastSignalHUDWidget::HandleSpecialAttackEnded()
{
	OnSpecialAttackEnded();
}

void ULastSignalHUDWidget::HandleKillConfirmed(FText VictimName)
{
	// 제출 빌드에서 "적 처치: 좀비" 킬 로그는 뺌 (처치 수는 오른쪽 위 손목시계 KILLS로 보여 줌)
	// 다시 쓰려면 아래 줄 주석 해제
	// OnKillFeedEntryAdded(VictimName);
}

void ULastSignalHUDWidget::HandleHitMarkerRequested()
{
	OnHitMarkerShown();
}

void ULastSignalHUDWidget::HandleGameOverRequested()
{
	OnGameOverShown();
}

FText ULastSignalHUDWidget::FormatTime(float Seconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Secs = TotalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Secs));
}
