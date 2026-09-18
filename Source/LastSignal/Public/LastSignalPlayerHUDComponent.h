// LastSignalPlayerHUDComponent.h
// LAST SIGNAL - PlayerController(or Pawn)에 부착하는 HUD 데이터/로직 컴포넌트.
// 실제 UI를 그리지 않고, "상태값 + 델리게이트"만 제공한다.
// 블루프린트 위젯(WBP)은 이 델리게이트를 Bind Event로 연결해서 화면을 갱신하면 된다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LastSignalHUDTypes.h"
#include "LastSignalPlayerHUDComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, FLastSignalWeaponHUDData, WeaponData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionObjectiveChanged, FText, NewObjective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimerUpdated, float, TimeValue, ELastSignalTimerMode, Mode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerModeChanged, ELastSignalTimerMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpecialGaugeChanged, float, GaugePercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpecialAttackActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpecialAttackEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillConfirmed, FText, VictimName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitMarkerRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverRequested);

UCLASS(ClassGroup = (LastSignal), meta = (BlueprintSpawnableComponent))
class LASTSIGNAL_API ULastSignalPlayerHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastSignalPlayerHUDComponent();

	// ================= HP (좌하단 HP바) =================
	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|HP")
	float CurrentHP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LastSignal|HP")
	float MaxHP = 100.f;

	UFUNCTION(BlueprintCallable, Category = "LastSignal|HP")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "LastSignal|HP")
	void HealToFull();

	// ================= 무기 / 탄약 (하단중앙, 우하단) =================
	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Weapon")
	FLastSignalWeaponHUDData CurrentWeapon;

	// 보급 상자 상호작용 시 3종 총기 중 선택되어 호출됨
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Weapon")
	void SetWeapon(const FLastSignalWeaponHUDData& NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Weapon")
	void ConsumeAmmo(int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Weapon")
	void ReloadWeapon();

	// ================= 스코어 / 킬 (우상단 점수, 킬확정 표시) =================
	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Score")
	int32 Score = 0;

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Score")
	void AddScore(int32 Amount);

	// 좀비 처치 시 호출 -> 스코어 증가 + 킬로그 표시 + 특수게이지 상승
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Score")
        void RegisterKill(const FText &VictimName);

	// ================= 미션 목표 (좌상단) =================
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Mission")
	void SetMissionObjective(FText NewObjective);

	// ================= 타이머 (우상단 시간) =================
	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Timer")
	ELastSignalTimerMode TimerMode = ELastSignalTimerMode::Stopwatch;

	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Timer")
	float TimeValue = 0.f;

	// [안전지대:재정비] 라디오 상호작용 시 호출 : 스톱워치 -> 카운트다운(빨간색) 전환
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Timer")
	void SwitchToCountdown(float CountdownSeconds);

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Timer")
	void ResetStopwatch();

    // GameMode가 1초마다 호출해서 화면 표시값을 동기화한다.
    UFUNCTION(BlueprintCallable, Category = "LastSignal|Timer")
        void UpdateTimerFromGameState(float NewTimeValue, ELastSignalTimerMode NewMode);

	// ================= 특수공격 게이지 (아드레날린, 우하단) =================
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LastSignal|Special")
	float GaugePerSecond = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LastSignal|Special")
	float GaugePerKill = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LastSignal|Special")
	float SpecialAttackDuration = 6.f;

	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Special")
	float SpecialGaugePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|Special")
	bool bSpecialAttackActive = false;

	// 게이지 100% + 버튼 입력 시 PlayerController에서 호출
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Special")
	bool TryActivateSpecialAttack();

	// ================= 히트마커 / 게임오버 =================
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Combat")
	void RequestHitMarker();

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Combat")
	void TriggerGameOver();

	// ================= 델리게이트 (WBP에서 Bind Event to ... 로 연결) =================
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnHPChanged OnHPChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnAmmoChanged OnAmmoChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnScoreChanged OnScoreChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnMissionObjectiveChanged OnMissionObjectiveChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnTimerUpdated OnTimerUpdated;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnTimerModeChanged OnTimerModeChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnSpecialGaugeChanged OnSpecialGaugeChanged;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnSpecialAttackActivated OnSpecialAttackActivated;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnSpecialAttackEnded OnSpecialAttackEnded;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnKillConfirmed OnKillConfirmed;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnHitMarkerRequested OnHitMarkerRequested;
	UPROPERTY(BlueprintAssignable, Category = "LastSignal|Delegates") FOnGameOverRequested OnGameOverRequested;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FTimerHandle SpecialAttackTimerHandle;
	void EndSpecialAttack();
};
