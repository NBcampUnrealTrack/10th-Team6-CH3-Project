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

    // 새 게임 시작 시 위의 저장값을 전부 기본값으로 되돌린다 (메인메뉴 → 다시 시작해도 이전 판이 안 이어지게)
    void ResetSaveData();

    // 새 게임을 시작한 실제 시각 (FPlatformTime::Seconds). 크레딧의 PLAY TIME 계산용 — UPROPERTY 아님(ResetSaveData 대상 아님)
    double GameStartRealSeconds = 0.0;

  private:
	virtual void Init() override;
};
