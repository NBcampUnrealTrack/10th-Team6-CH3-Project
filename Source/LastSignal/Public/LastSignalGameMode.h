#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "LastSignalGameMode.generated.h"


class UUserWidget;

UCLASS()
class LASTSIGNAL_API ALastSignalGameMode : public AGameModeBase
{
	
	GENERATED_BODY()

public:
	ALastSignalGameMode();
	void OnGoalReached(FName NextLevel);  // 트리거 다음맵 때문에 추가
    void OnZombieKilled(); // 좀비카운트 킬 호출 받는거

	void OnGameOver();      // 게임오버(카운트다운 실패 or 플레이어 사망) 처리, 위젯 표시
    void OnEscapeSuccess();   // 탈출 타이머 0 = 클리어(성공) — 엔딩 연출은 나중에 붙일 예정 (제 예상은 클리어 뜨고 바로 엔딩 연출 on)

	UFUNCTION(BlueprintImplementableEvent, Category = "Ending")
	void OnEndingStarted(); // 탈출 성공 시점 알림 — 엔딩 슬라이드쇼는 BP_LastSignalGameMode에서 구현

	// 엔딩 슬라이드쇼가 끝나면 BP에서 호출: 기록(플레이 타임, 처치 수) → 올라가는 크레딧 → 메인메뉴
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void ShowCredits();
    
	UFUNCTION()
	void OnPlayerDied(); // HP 0 됐을 때 호출되는 함수, 게임오버 처리용

	UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> GameOverClass; // 게임오버 위젯 BP 지정용

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TMap<FName, FText> LevelObjectives; // 레벨 이름 → 시작 목표 문구 (PlayerController가 HUD 만들 때 읽어감)

	FTimerHandle TimerHandle; // 타이머 취소,갱신할 때 쓰는 꼬리표

	double ClearRealSeconds = 0.0; // 탈출 성공한 실제 시각 — 크레딧 플레이 시간을 여기서 고정 (엔딩 슬라이드쇼 시간 제외)

	void StartStopwatch();                        // 게임 시작 시 호출 — 0부터 증가
    void StartCountdown(float DurationSeconds);   // 라디오 상호작용 시 호출 — 감소 시작
    void StartEscapeTimer(float DurationSeconds); // 헬기 구역 도착 시 호출 — 감소 시작

    void UpdateTimer(); // 1초마다 자동으로 반복 호출됨

    // 지금 시계(스톱워치/카운트다운 + 값)를 GameInstance에 저장 → 다음 레벨이나 RETRY에서 이어짐 (탈출 타이머 중이면 저장 안 함)
    void SaveTimerToGameInstance();


protected:

	virtual void InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage) override;
	virtual void BeginPlay() override;

	 class ULastSignalPlayerHUDComponent *GetLocalHUDComponent() const;

};
