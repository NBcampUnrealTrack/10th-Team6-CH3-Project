#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
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

    // 기본 키는 생성자에서 지정한다.
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Input")
    FKey FireKey;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Input")
    FKey ReloadKey;

    // 소리·머즐 플래시·발사 애니메이션 연결 지점.
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Effects")
    void PlayShotEffects(bool bHit, const FHitResult &HitResult);

    // true: 재장전 시작 / false: 완료 또는 취소.
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Effects")
    void PlayReloadEffects(bool bReloading);

  private:
    void FirePressed();
    void FireReleased();
    void ReloadPressed();

    UFUNCTION()
    void HandleShot(bool bHit, const FHitResult &HitResult);

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
};