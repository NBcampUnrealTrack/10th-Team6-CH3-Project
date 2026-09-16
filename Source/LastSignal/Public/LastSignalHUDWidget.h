// LastSignalHUDWidget.h
// LAST SIGNAL - 전투 HUD 메인 위젯의 C++ 베이스 클래스.
// 실제 UMG 디자인은 이 클래스를 부모로 하는 블루프린트 위젯(WBP_LastSignalHUD)에서 작업한다.
// (가이드 문서: UI_Blueprint_설정가이드.md 참고)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastSignalHUDTypes.h"
#include "LastSignalHUDWidget.generated.h"

class ULastSignalPlayerHUDComponent;

UCLASS(Abstract)
class LASTSIGNAL_API ULastSignalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// PlayerController가 위젯 생성 직후 반드시 호출 -> 컴포넌트 델리게이트 전부 연결
	UFUNCTION(BlueprintCallable, Category = "LastSignal|HUD")
	void BindHUDComponent(ULastSignalPlayerHUDComponent* InComponent);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|HUD")
	TObjectPtr<ULastSignalPlayerHUDComponent> BoundComponent;

	// ===================================================================
	// 아래 이벤트들은 전부 WBP_LastSignalHUD의 이벤트그래프에서 구현(오버라이드)한다.
	// 목업 레이아웃 기준 대응 위치를 주석으로 표기.
	// ===================================================================

	// 좌하단 HP바
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnHPUpdated(float CurrentHP, float MaxHP, float Percent01);

	// 하단중앙(무기아이콘) + 우하단(탄창 수)
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnAmmoUpdated(const FLastSignalWeaponHUDData& WeaponData);

	// 우상단 점수
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnScoreUpdated(int32 NewScore);

	// 좌상단 미션 목표/진행상황
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnMissionObjectiveUpdated(const FText& Objective);

	// 우상단 시간 표시. bUseRedCountdown = true 이면 목업처럼 빨간색 카운트다운으로 스타일 전환
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnTimerUpdated(const FText& FormattedTime, bool bUseRedCountdown);

	// 우하단 특수공격 게이지 (%). bReady = true면 발동 가능 상태(테두리 강조 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnSpecialGaugeUpdated(float Percent, bool bReady);

	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnSpecialAttackActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnSpecialAttackEnded();

	// 좌상단 아래쪽 킬로그(킬확정 표시) - 항목 추가 후 일정 시간 뒤 Fade-out은 블루프린트/자식위젯에서 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnKillFeedEntryAdded(const FText& VictimName);

	// 중앙 조준점 근처 히트마커 플래시
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnHitMarkerShown();

	// HP 0 또는 타이머 0 도달 시 게임오버 화면
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|HUD")
	void OnGameOverShown();

private:
	UFUNCTION() void HandleHPChanged(float CurrentHP, float MaxHP);
	UFUNCTION() void HandleAmmoChanged(FLastSignalWeaponHUDData WeaponData);
	UFUNCTION() void HandleScoreChanged(int32 NewScore);
	UFUNCTION() void HandleMissionObjectiveChanged(FText NewObjective);
	UFUNCTION() void HandleTimerUpdated(float TimeValue, ELastSignalTimerMode Mode);
	UFUNCTION() void HandleSpecialGaugeChanged(float GaugePercent);
	UFUNCTION() void HandleSpecialAttackActivated();
	UFUNCTION() void HandleSpecialAttackEnded();
	UFUNCTION() void HandleKillConfirmed(FText VictimName);
	UFUNCTION() void HandleHitMarkerRequested();
	UFUNCTION() void HandleGameOverRequested();

	// 초 -> "MM:SS" 텍스트로 변환
	static FText FormatTime(float Seconds);
};
