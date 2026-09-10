#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastSignalGameInstance.generated.h"


UCLASS()
class LASTSIGNAL_API ULastSignalGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	
private:
	virtual void Init() override;
};
