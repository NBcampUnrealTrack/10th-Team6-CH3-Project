#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastSignalGameInstance.generated.h"


UCLASS()
class LASTSIGNAL_API ULastSignalGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Save")
    float SavedHP = 100.f;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    int32 SavedKillCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    float SavedPlayTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    float SavedRemainingTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    bool bTimeLimitStarted = false;

	
private:
	virtual void Init() override;
};
