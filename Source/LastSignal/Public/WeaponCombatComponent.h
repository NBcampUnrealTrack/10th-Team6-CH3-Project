#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TimerManager.h"
#include "WeaponCombatComponent.generated.h"

class APawn;
class USceneComponent;

// 반자동과 자동은 산탄 여부와 별개의 설정이다.
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
    SemiAuto,
    Automatic
};

// 일반탄과 산탄의 실행 경로를 명확하게 구분한다.
UENUM(BlueprintType)
enum class EWeaponAttackType : uint8
{
    Single,
    Shotgun
};

// 탄창 교체와 한 발씩 넣는 재장전을 구분한다.
UENUM(BlueprintType)
enum class EWeaponReloadType : uint8
{
    Magazine,
    PerShell
};

// 각 총기의 생성자에서 설정하는 고유 스탯.
USTRUCT(BlueprintType)
struct FWeaponStats
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
    EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
    EWeaponAttackType AttackType = EWeaponAttackType::Single;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload")
    EWeaponReloadType ReloadType = EWeaponReloadType::Magazine;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire",
              meta = (ClampMin = "0.0"))
    float Damage = 20.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire",
              meta = (ClampMin = "1.0"))
    float RPM = 600.0f;

    // 언리얼 기본 거리 단위는 cm다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire",
              meta = (ClampMin = "1.0"))
    float Range = 4000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo",
              meta = (ClampMin = "1"))
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo",
              meta = (ClampMin = "0"))
    int32 InitialReserveAmmo = 90;

    // 탄창식: 전체 교체 시간 / 개별 장전: 한 발 삽입 시간.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload",
              meta = (ClampMin = "0.01"))
    float ReloadTime = 2.5f;

    // 약실에 총알이 남은 상태는 전술 재장전 시간
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.01"))
    float TacReloadTime = 1.8f;

    // 산탄총의 첫 장전 동작 이후, 한 발씩 반복 삽입하는 시간.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload",
              meta = (ClampMin = "0.01",
                      DisplayName = "Shell Insert Time (반복 삽입 시간)"))
    float ShellInsertTime = 0.65f;

    // 산탄 구현용 정보. 지금은 실제 발사에 사용하지 않는다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun",
              meta = (ClampMin = "1"))
    int32 PelletCount = 9;

    // X는 최소값, Y는 최대값. 음수 Pitch는 입력 설정에 따라 위로 향한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
    FVector2D PitchKick = FVector2D(-1.2f, -1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
    FVector2D YawKick = FVector2D(-0.8f, 0.8f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01"))
    float CameraKickSpeed = 25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "100.0",
                      DisplayName = "Camera Return Speed (Hip Fire)"))
    float CameraReturnSpeed = 8.0f;

    // Zero inherits the existing return speed, preserving saved weapon tuning.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "100.0",
                      DisplayName = "ADS Camera Return Speed",
                      ToolTip = "ADS recovery speed. 0 uses the Hip Fire return speed. Higher values recover faster."))
    float ADSCameraReturnSpeed = 0.0f;

    // 총마다 메시 반동의 전체 세기를 조절한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0"))
    float VisualKickScale = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01"))
    float VisualKickSpeed = 25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "100.0",
                      DisplayName = "Visual Return Speed (Hip Fire)"))
    float VisualReturnSpeed = 12.0f;

    // Zero inherits the existing return speed, preserving saved weapon tuning.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "100.0",
                      DisplayName = "ADS Visual Return Speed",
                      ToolTip = "ADS recovery speed. 0 uses the Hip Fire return speed. Higher values recover faster."))
    float ADSVisualReturnSpeed = 0.0f;

    // 부모 좌표계 기준, 단위 cm.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (DisplayName = "Location Kick Min (위치 반동 최솟값)"))
    FVector LocationKickMin = FVector(-10.0f, -2.0f, -0.6f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (DisplayName = "Location Kick Max (위치 반동 최댓값)"))
    FVector LocationKickMax = FVector(-5.0f, 2.0f, 0.6f);

    // Additional ADS multiplier for parent-space X translation only.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0",
                      DisplayName = "ADS X Location Kick Multiplier",
                      ToolTip = "Extra X location kick multiplier after the existing ADS reduction. 1 preserves current recoil; 0 disables new ADS X kicks."))
    float ADSXLocationKickMultiplier = 1.0f;

    // 발당 상하 랜덤 회전 범위: -설정값 ~ +설정값.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0",
                      DisplayName = "Visual Roll Kick (총 수직 랜덤 반동 폭)"))
    float VisualRollKick = 2.0f;

    // 발당 좌우 랜덤 회전 범위: -설정값 ~ +설정값.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "15.0",
                      DisplayName = "Visual Yaw Kick (총 좌우 반동 폭)"))
    float VisualYawKick = 7.5f;

    // Maximum shotgun angle from the muzzle direction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spread",
              meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxSpreadAngle = 4.0f;

    // 한 발마다 추가하는 Z 회전의 최대 각도.
    // 0이면 비활성화. 이 총에서는 음수가 총구를 드는 방향.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (UIMin = "-10.0", UIMax = "10.0",
                      DisplayName = "Shot Vertical Angle (발당 수직 회전 각도)"))
    float ShotVerticalAngle = 0.0f;

    // Rise and return multipliers independently scale their RPM-based durations.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "5.0",
                      DisplayName = "Vertical Pulse Kick Speed (Hip Fire)",
                      ToolTip = "Rise speed multiplier: 1 preserves original timing, 2 rises twice as fast, 0.5 rises half as fast. Does not change angle or return duration. Selected when firing."))
    float VerticalPulseKickSpeed = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0",
                      DisplayName = "ADS Vertical Pulse Kick Speed",
                      ToolTip = "ADS pulse rise speed multiplier. 0 uses the Hip Fire value. Selected when firing."))
    float ADSVerticalPulseKickSpeed = 0.0f;

    // Scales the falling portion only, independently of the rise speed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "5.0",
                      DisplayName = "Vertical Pulse Return Speed (Hip Fire)",
                      ToolTip = "Recovery speed multiplier: 1 preserves original timing, 2 returns twice as fast, 0.5 returns half as fast. Selected when firing."))
    float VerticalPulseReturnSpeed = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil",
              meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0",
                      DisplayName = "ADS Vertical Pulse Return Speed",
                      ToolTip = "ADS pulse recovery speed multiplier. 0 uses the Hip Fire value. Selected when firing."))
    float ADSVerticalPulseReturnSpeed = 0.0f;

    // 무기 스왑시 발사 불가능한 시간
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
    float SwapDelay = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire",
              meta = (ClampMin = "0.0", DisplayName = "Sprint To Fire Delay"))
    float SprintToFireDelay = 0.15f;
};

// 명중 여부와 결과를 발사 연출에 전달한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWeaponShotEvent,
    bool, bHit,
    const FHitResult &, HitResult);

// 실제 명중마다 호출한다. 산탄총은 명중한 펠릿 수만큼 호출한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWeaponImpactEvent,
    const FHitResult &, HitResult);

// UI가 탄약 변경을 전달받을 수 있다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWeaponAmmoEvent,
    int32, CurrentAmmo,
    int32, ReserveAmmo);

// 재장전 시작·종료 시 애니메이션을 연결할 수 있다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FWeaponReloadEvent,
    bool, bReloading);

UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class LASTSIGNAL_API UWeaponCombatComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UWeaponCombatComponent();

    // 자식 총기 생성자에서 이 값을 설정한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FWeaponStats Stats;

    // 무기 부모가 장착 시 호출한다.
    // VisualComponent는 반동으로 움직일 총 또는 팔 컴포넌트다.
    void ActivateWeapon(APawn *NewPawn, USceneComponent *VisualComponent);

    // 무기 해제 시 입력 상태·타이머·반동을 정리한다.
    void DeactivateWeapon();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartReload();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetCurrentAmmo() const { return CurrentAmmo; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsReloading() const { return bReloading; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsSwapping() const { return bSwapping; }

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FWeaponShotEvent OnShot;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FWeaponImpactEvent OnImpact;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FWeaponAmmoEvent OnAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FWeaponReloadEvent OnReloadChanged;

    // 무기 입력에서 요청한 조준 상태를 적용한다.
    void SetAiming(bool bNewAiming);

    // 애니메이션과 무기가 같은 조준 상태를 사용한다.
    UFUNCTION(BlueprintPure, Category = "Weapon|Aim")
    bool IsAiming() const { return bAiming; }

    // 조준 중에는 기존 반동의 65%를 적용한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Aim",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ADSRecoilMultiplier = 0.65f;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetAmmo(int32 InCurrent, int32 InReserve);

  protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction *ThisTickFunction) override;

    // 일반탄과 산탄은 서로 다른 함수에서 처리한다.
    // 반환값은 실제 공격이 실행됐는지를 의미한다.
    virtual bool FireSingleShot(FHitResult &OutHit, bool &bOutHit);
    virtual bool FireShotgun(FHitResult &OutHit, bool &bOutHit);

  private:
    void TryFire();
    // Returns true once when the sprint fire lock expires.
    bool UpdateSprintFireLock();
    void ApplyDamage(const FHitResult &HitResult);
    void ReloadStep();
    void EndReload();

    void AddRecoil();
    void UpdateRecoil(float DeltaTime);
    void ResetRecoil();
    void NotifyAmmo();
    void EndSwap();
    // 실제 적용 중인 조준 상태.
    bool bAiming = false;

    TWeakObjectPtr<APawn> Shooter;
    TWeakObjectPtr<USceneComponent> RecoilVisual;

    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    FTimerHandle SwapTimer;

    int32 CurrentAmmo = 0;
    int32 ReserveAmmo = 0;

    bool bEquipped = false;
    bool bTriggerHeld = false;
    bool bReloading = false;
    bool bSwapping = false;
    bool bSprintFireLocked = false;
    bool bSprintKeyWasDown = false;
    double SprintFireUnlockTime = 0.0;

    double VerticalPulseStartTime = -1.0;
    float VerticalPulseDuration = 0.1f;
    float VerticalPulseRiseDuration = 0.025f;
    float VerticalPulseAngle = 0.0f;
    double NextFireTime = 0.0;

    // 반동 목표값과 현재 적용된 반동값.
    FVector2D CameraTarget = FVector2D::ZeroVector;
    FVector2D CameraCurrent = FVector2D::ZeroVector;

    // 기본 자세는 장착 시 저장한다.
    FVector BaseVisualLocation = FVector::ZeroVector;
    FRotator BaseVisualRotation = FRotator::ZeroRotator;

    FVector VisualLocationTarget = FVector::ZeroVector;
    FVector VisualLocationCurrent = FVector::ZeroVector;

    FRotator VisualRotationTarget = FRotator::ZeroRotator;
    FRotator VisualRotationCurrent = FRotator::ZeroRotator;
};