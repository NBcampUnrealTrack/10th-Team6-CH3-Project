#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "WeaponImpactEffects.h"
#include "PrimaryWeapon.generated.h"


class APawn;
class APlayerController;
class USceneComponent;
class USkeletalMeshComponent;
class UWeaponCombatComponent;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
class UTexture2D;

// UI 추가
class ALastSignalPlayerController;

// 주무기만 상속하는 부모.
// 나중에 보조무기는 이 클래스를 상속하지 않는다.
UCLASS(Abstract)
class LASTSIGNAL_API APrimaryWeapon : public AActor
{
    GENERATED_BODY()

  public:
    APrimaryWeapon();

    // 캐릭터 또는 장비 시스템이 장착 시 한 번 호출한다.
    // 입력 연결과 전투 활성화는 무기가 직접 수행한다.
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool Equip(
        APawn *NewOwner,
        USceneComponent *AttachParent,
        FName SocketName);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Unequip();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    UWeaponCombatComponent *GetCombatComponent() const
    {
        return Combat;
    }

    FORCEINLINE TSoftObjectPtr<UTexture2D> GetWeaponIcon() const 
    { 
        return WeaponIcon; 
    }

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateHUD();

    UFUNCTION(BlueprintPure, Category = "Weapon|Aim")
    bool IsAiming() const;

    // 본체뿐 아니라 좀비 소유의 별도 피격 액터도 같은 기준으로 분류한다.
    UFUNCTION(BlueprintPure, Category = "Weapon|Impact")
    bool IsEnemyHit(const FHitResult &HitResult) const;

  protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

    // 루트와 총 메시를 분리해야 반동이 장착 위치 자체를 흔들지 않는다.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<USceneComponent> WeaponRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<UWeaponCombatComponent> Combat;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UTexture2D> WeaponIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Impact",
              meta = (DisplayName = "환경 탄착"))
    FWeaponImpactEffects EnvironmentImpactEffects;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Impact",
              meta = (DisplayName = "적 탄착"))
    FWeaponImpactEffects EnemyImpactEffects;

    // 기본 키는 생성자에서 지정한다.
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Input")
    FKey FireKey;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Input")
    FKey ReloadKey;

    // 소리·머즐 플래시·발사 애니메이션 연결 지점.
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Effects")
    void PlayShotEffects(bool bHit, const FHitResult &HitResult);

    // 실제 명중마다 탄착 파티클·데칼·소리를 연결한다. 빗나간 경우 호출하지 않는다.
    // 산탄총은 명중한 펠릿마다 호출하므로 머즐 플래시·발사음은 여기에 연결하지 않는다.
    // 기본 구현은 클래스 기본값의 환경/적 탄착 설정을 재생한다.
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon|Effects")
    void PlayImpactEffects(const FHitResult &HitResult);
    virtual void PlayImpactEffects_Implementation(const FHitResult &HitResult);

    // true: 재장전 시작 / false: 완료 또는 취소.
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Effects")
    void PlayReloadEffects(bool bReloading);

    // 기본 조준 키는 우클릭이다.
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Input")
    FKey AimKey;

    // 조준 상태가 바뀔 때만 호출한다.
    // 총과 팔의 조준 전환 연출을 연결하는 지점이다.
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Effects")
    void PlayAimEffects(bool bAiming);

  private:
    void FirePressed();
    void FireReleased();
    void ReloadPressed();

    UFUNCTION()
    void HandleShot(bool bHit, const FHitResult &HitResult);

    UFUNCTION()
    void HandleImpact(const FHitResult &HitResult);

    UFUNCTION()
    void HandleReload(bool bReloading);

    // UI에 노출되는 탄수
    UFUNCTION()
    void HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo);

    // 이 무기 전용 입력 컴포넌트와 매핑을 사용한다.
    UPROPERTY()
    TObjectPtr<UEnhancedInputComponent> WeaponInput;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> WeaponMapping;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> FireAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ReloadAction;

    TWeakObjectPtr<APlayerController> EquippedController;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;

    bool bEquipped = false;

    void AimPressed();
    void AimReleased();

    // 입력 상태와 재장전 상태를 바탕으로 실제 조준 상태를 갱신한다.
    void RefreshAiming();

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> AimAction;

    // 우클릭 유지 여부와 실제 조준 상태를 구분한다.
    // 재장전 중에도 우클릭 유지 여부는 기억한다.
    bool bAimHeld = false;
};