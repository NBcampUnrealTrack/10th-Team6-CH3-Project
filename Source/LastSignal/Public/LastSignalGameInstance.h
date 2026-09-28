#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LastSignalGameInstance.generated.h"

class APrimaryWeapon;
class UAudioComponent;
class USoundBase;

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

    // ===== 사운드: 음악 채널 + 환경음 채널 (레벨이 바뀌어도 이어짐, 바꿀 때 서서히 교차) =====
    // 같은 곡이 이미 나오고 있으면 그대로 둔다. Sound가 없으면(에셋 경로 오류 등) 아무것도 안 함
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayMusic(USoundBase *Sound, float FadeSeconds = 2.0f, float Volume = 0.5f);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void StopMusic(float FadeSeconds = 2.0f);

    // 헬기 소리 등. VolumeAfterFade/FadeAwaySeconds로 "점점 멀어지는" 연출 (0이면 그대로 유지)
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayAmbience(USoundBase *Sound, float Volume = 0.7f, float FadeAwaySeconds = 0.0f, float VolumeAfterFade = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void StopAmbience(float FadeSeconds = 2.0f);

    // /Game/Sounds/Music/BGM_* 같은 경로로 사운드 불러오기 (반복 재생되게 설정)
    static USoundBase *LoadLoopingSound(const TCHAR *Path);

  private:
    // 이름이 Audio로 시작하는 UPROPERTY는 ResetSaveData에서 건너뜀 (재생 중인 소리를 잃지 않게)
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioMusicComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioAmbienceComponent;

	virtual void Init() override;
};
