#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastSignalGameState.generated.h"


UCLASS()
class LASTSIGNAL_API ALastSignalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "Stats") //킬 카운트 추가 입니다.
	int32 KillCount = 0;
    
	void AddKillCount();

private:
	virtual void BeginPlay() override;
};
