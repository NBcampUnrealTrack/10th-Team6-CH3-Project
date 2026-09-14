#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastSignalGameState.generated.h"

UENUM(BlueprintType) // 타이머 상태값
enum class ETimerMode : uint8
{
    Stopwatch,
    Countdown,
    Escape
};

UCLASS()
class LASTSIGNAL_API ALastSignalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "Stats") //킬 카운트 추가 입니다.
	int32 KillCount = 0;
    
	void AddKillCount();

	UPROPERTY(BlueprintReadOnly, Category = "Timer") // 화면에 표시할 현재 타이머 모드
    ETimerMode TimerMode = ETimerMode::Stopwatch;

    UPROPERTY(BlueprintReadOnly, Category = "Timer") // 화면에 표시할 현재 시간(초)
    float TimerValue = 0.f;


private:
	virtual void BeginPlay() override;
};
