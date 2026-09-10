#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastSignalGameState.generated.h"


UCLASS()
class LASTSIGNAL_API ALastSignalGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

private:
	virtual void BeginPlay() override;
};
