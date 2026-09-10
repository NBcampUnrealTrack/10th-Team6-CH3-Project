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
	void OnGoalReached();

protected:

	virtual void BeginPlay() override;

};
