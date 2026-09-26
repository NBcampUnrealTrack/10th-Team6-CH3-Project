#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastSignalGameInstance.generated.h"

class APrimaryWeapon;

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

	UPROPERTY(BlueprintReadWrite, Category = "Save")
    bool bSavedIsSwapUnlocked = false;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    TSubclassOf<APrimaryWeapon> SavedPrimaryWeaponClass = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    int32 SavedEquippedSlotIndex = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    int32 SavedPrimaryCurrentAmmo = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    int32 SavedPrimaryReserveAmmo = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    int32 SavedSecondaryCurrentAmmo = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    int32 SavedSecondaryReserveAmmo = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Save")
    float SavedSkillGauge = 0.0f;

  private:
	virtual void Init() override;
};
