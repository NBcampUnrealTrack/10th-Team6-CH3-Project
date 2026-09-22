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
    
	UFUNCTION()
	void OnPlayerDied(); // HP 0 됐을 때 호출되는 함수, 게임오버 처리용

	UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> GameOverClass; // 게임오버 위젯 BP 지정용

	FTimerHandle TimerHandle; // 타이머 취소,갱신할 때 쓰는 꼬리표

	void StartStopwatch();                        // 게임 시작 시 호출 — 0부터 증가
    void StartCountdown(float DurationSeconds);   // 라디오 상호작용 시 호출 — 감소 시작
    void StartEscapeTimer(float DurationSeconds); // 헬기 구역 도착 시 호출 — 감소 시작

    void UpdateTimer(); // 1초마다 자동으로 반복 호출됨


protected:

	virtual void BeginPlay() override;

	 class ULastSignalPlayerHUDComponent *GetLocalHUDComponent() const;

};
