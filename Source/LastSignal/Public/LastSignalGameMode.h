#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LastSignalGameMode.generated.h"


UCLASS()
class LASTSIGNAL_API ALastSignalGameMode : public AGameModeBase
{
	
	GENERATED_BODY()

public:
	ALastSignalGameMode();
	void OnGoalReached(FName NextLevel);  // 트리거 다음맵 때문에 추가
    void OnZombieKilled(); // 좀비카운트 킬 호출 받는거

protected:

	virtual void BeginPlay() override;

};
