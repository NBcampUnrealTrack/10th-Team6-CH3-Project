#include "WeaponCombatComponent.h"

// 사격 디버깅용 코드
#include "DrawDebugHelpers.h"
//
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieAICharacter.h"

UWeaponCombatComponent::UWeaponCombatComponent()
{
    // 사격은 타이머로 처리하고, 반동 보간만 Tick에서 처리한다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UWeaponCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    // 자식 총기의 생성자에서 설정한 스탯을 기준으로 초기화한다.
    Stats.MagazineSize = FMath::Max(1, Stats.MagazineSize);

    CurrentAmmo = Stats.MagazineSize;
    ReserveAmmo = FMath::Max(0, Stats.InitialReserveAmmo);
}

void UWeaponCombatComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    DeactivateWeapon();
    Super::EndPlay(EndPlayReason);
}

void UWeaponCombatComponent::ActivateWeapon(
    APawn *NewPawn,
    USceneComponent *VisualComponent)
{
    DeactivateWeapon();

    if (!IsValid(NewPawn))
    {
        return;
    }

    Shooter = NewPawn;
    RecoilVisual = VisualComponent;

    if (RecoilVisual.IsValid())
    {
        BaseVisualLocation = RecoilVisual->GetRelativeLocation();
        BaseVisualRotation = RecoilVisual->GetRelativeRotation();
    }

    bEquipped = true;
    SetComponentTickEnabled(true);
    NotifyAmmo();

    // 무기 스왑 딜레이
    bSwapping = true;
    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            SwapTimer,
            this,
            &UWeaponCombatComponent::EndSwap,
            FMath::Max(0.01f, Stats.SwapDelay),
            false);
    }
}

void UWeaponCombatComponent::DeactivateWeapon()
{
    bAiming = false;
    bEquipped = false;
    bTriggerHeld = false;
    bSwapping = false;
    bSprintFireLocked = false;
    bSprintKeyWasDown = false;
    SprintFireUnlockTime = 0.0;

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimer);
        World->GetTimerManager().ClearTimer(ReloadTimer);
        World->GetTimerManager().ClearTimer(SwapTimer);
    }

    if (bReloading)
    {
        bReloading = false;
        OnReloadChanged.Broadcast(false);
    }

    ResetRecoil();

    Shooter.Reset();
    RecoilVisual.Reset();

    SetComponentTickEnabled(false);

    // 발사 가능 시간은 유지한다.
    // 무기 교체로 발사 간격을 우회하지 못하게 한다.
}

void UWeaponCombatComponent::StartFire()
{
    UpdateSprintFireLock();
    if (!bEquipped || bTriggerHeld || bSwapping || bSprintFireLocked)
    {
        return;
    }

    if (bReloading)
    {
        // 한 발씩 장전하는 무기는 탄약이 있으면 장전을 끊고 발사한다.
        if (Stats.ReloadType == EWeaponReloadType::PerShell &&
            CurrentAmmo > 0)
        {
            EndReload();
        }
        else
        {
            return;
        }
    }

    bTriggerHeld = true;
    TryFire();
}

void UWeaponCombatComponent::StopFire()
{
    bTriggerHeld = false;

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimer);
    }

    // 반동은 즉시 초기화하지 않고 Tick에서 자연스럽게 복귀시킨다.
}

void UWeaponCombatComponent::ApplyDamage(const FHitResult &HitResult)
{
    AActor *HitActor = HitResult.GetActor();
    if (!IsValid(HitActor))
    {
        return;
    }

    AZombieAICharacter *Zombie = Cast<AZombieAICharacter>(HitActor);
    if (!Zombie && HitActor->GetOwner())
    {
        Zombie = Cast<AZombieAICharacter>(HitActor->GetOwner());
    }

    if (Zombie)
    {
        AController *InstigatorController = Shooter.IsValid() ? Shooter->GetController() : nullptr;

        float FinalDamage = FMath::Max(0.0f, Stats.Damage);

        // 헤드샷 판정: 맞은 컴포넌트가 HeadHitbox인지 확인
        const bool bHeadshot = (HitResult.GetComponent() == Zombie->GetHeadHitbox());
        if (bHeadshot)
        {
            FinalDamage *= Zombie->HeadshotMultiplier;
        }

        UGameplayStatics::ApplyDamage(
            Zombie,
            FinalDamage,
            InstigatorController,
            GetOwner(),
            UDamageType::StaticClass());

        UE_LOG(LogTemp, Warning, TEXT("[Player] Hit Zombie! Headshot: %s / Damage: %f"),
               bHeadshot ? TEXT("YES") : TEXT("NO"), FinalDamage);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Player] Hit Non-Zombie Actor: %s"), *HitActor->GetName());
    }
}

void UWeaponCombatComponent::TryFire()
{
    UpdateSprintFireLock();
    if (bSprintFireLocked || bSwapping)
    {
        return;
    }

    UWorld *World = GetWorld();

    if (!World || !bEquipped || !bTriggerHeld ||
        bReloading || CurrentAmmo <= 0 || !Shooter.IsValid())
    {
        return;
    }

    const double RemainingTime = NextFireTime - World->GetTimeSeconds();

    if (RemainingTime > 0.0)
    {
        if (Stats.FireMode == EWeaponFireMode::Automatic)
        {
            World->GetTimerManager().SetTimer(
                FireTimer,
                this,
                &UWeaponCombatComponent::TryFire,
                FMath::Max(static_cast<float>(RemainingTime), 0.001f),
                false);
        }

        return;
    }

    FHitResult Hit;
    bool bHit = false;
    bool bAttackExecuted = false;

    // 공격 방식을 명확하게 분리한다.
    switch (Stats.AttackType)
    {
    case EWeaponAttackType::Single:
        bAttackExecuted = FireSingleShot(Hit, bHit);
        break;

    case EWeaponAttackType::Shotgun:
        bAttackExecuted = FireShotgun(Hit, bHit);
        break;
    }

    // 조준 정보 획득 실패 및 미구현 공격 시 탄약 차감 방지 및 연사 중단
    if (!bAttackExecuted)
    {
        StopFire();
        return;
    }

    // 정상 사격 실행 시에만 탄약 차감
    --CurrentAmmo;

    const float Interval = 60.0f / FMath::Max(1.0f, Stats.RPM);
    NextFireTime = World->GetTimeSeconds() + Interval;

    // 명중 방향 계산이 끝난후 탄퍼짐(Heat) 누적
    AddRecoil();

    // 연사 시 다음 발사 예약
    if (Stats.FireMode == EWeaponFireMode::Automatic && CurrentAmmo > 0)
    {
        World->GetTimerManager().SetTimer(
            FireTimer,
            this,
            &UWeaponCombatComponent::TryFire,
            Interval,
            false);
    }

    // ApplyDamage 함수 호출
    if (Stats.AttackType == EWeaponAttackType::Single && bHit)
    {
        ApplyDamage(Hit);
    }

    NotifyAmmo();
    OnShot.Broadcast(bHit, Hit);
}

bool UWeaponCombatComponent::FireSingleShot(
    FHitResult &OutHit,
    bool &bOutHit)
{
    OutHit = FHitResult();
    bOutHit = false;

    UWorld *World = GetWorld();
    APawn *Pawn = Shooter.Get();

    // 현재 장착 코드에서 RecoilVisual에 WeaponMesh를 전달하고 있다.
    USceneComponent *GunMesh = RecoilVisual.Get();

    if (!World || !IsValid(Pawn) || !IsValid(GunMesh))
    {
        return false;
    }

    static const FName MuzzleSocketName(TEXT("Muzzle"));

    const bool bHasMuzzle =
        GunMesh->DoesSocketExist(MuzzleSocketName);

    // 소켓이 있으면 소켓의 월드 좌표계를 사용한다.
    // 없으면 총 메시 컴포넌트의 월드 좌표계를 사용한다.
    const FTransform FireTransform = bHasMuzzle
                                         ? GunMesh->GetSocketTransform(MuzzleSocketName, RTS_World)
                                         : GunMesh->GetComponentTransform();

    const FVector TraceStart = FireTransform.GetLocation();

    // 소켓 또는 메시의 로컬 +X축을 발사 방향으로 사용한다.
    // 카메라 방향과 랜덤 탄퍼짐은 사용하지 않는다.
    const FVector FireDirection =
        FireTransform.GetUnitAxis(EAxis::X);

    const FVector End =
        TraceStart + FireDirection * FMath::Max(1.0f, Stats.Range);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Pawn);

    bOutHit = World->LineTraceSingleByChannel(
        OutHit,
        TraceStart,
        End,
        ECC_Visibility,
        Params);

    //// 디버그: 실제 충돌 지점까지만 선을 표시한다.
    // const FVector DebugEnd = bOutHit ? OutHit.ImpactPoint : End;

    // DrawDebugLine(
    //     World,
    //     TraceStart,
    //     DebugEnd,
    //     bOutHit ? FColor::Green : FColor::Red,
    //     false,
    //     10.0f,
    //     1,
    //     4.0f);

    //// 디버그: 발사 시작점은 파란 구체로 표시한다.
    // DrawDebugSphere(
    //     World,
    //     TraceStart,
    //     3.0f,
    //     8,
    //     FColor::Blue,
    //     false,
    //     10.0f,
    //     1,
    //     1.0f);

    if (bOutHit)
    {
        DrawDebugPoint(
            World,
            OutHit.ImpactPoint,
            15.0f,
            FColor::Yellow,
            false,
            10.0f,
            1);
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[TraceDebug] Source=%s Hit=%d"),
        bHasMuzzle ? TEXT("MuzzleSocket") : TEXT("WeaponMesh"),
        bOutHit);

    // 빗나가도 발사는 실행됐으므로 탄약은 소비한다.
    return true;
    // 디버깅용 임시 주석처리
    // bOutHit = GetWorld()->LineTraceSingleByChannel(
    //   OutHit,
    // TraceStart,
    // End,
    // ECC_Visibility,
    // Params);

    // 명중하지 않아도 총알 한 발은 정상적으로 발사한 것이다.
    // return true;
}

bool UWeaponCombatComponent::FireShotgun(
    FHitResult &OutHit,
    bool &bOutHit)
{
    OutHit = FHitResult();
    bOutHit = false;

    UWorld *World = GetWorld();
    APawn *Pawn = Shooter.Get();
    USceneComponent *GunMesh = RecoilVisual.Get();

    if (!World || !IsValid(Pawn) || !IsValid(GunMesh))
    {
        return false;
    }

    // 총구 소켓이 없으면 총 메시의 위치와 방향을 사용한다.
    static const FName MuzzleSocketName(TEXT("Muzzle"));

    const bool bHasMuzzle =
        GunMesh->DoesSocketExist(MuzzleSocketName);

    const FTransform FireTransform = bHasMuzzle
                                         ? GunMesh->GetSocketTransform(MuzzleSocketName, RTS_World)
                                         : GunMesh->GetComponentTransform();

    const FVector TraceStart = FireTransform.GetLocation();
    const FVector FireDirection =
        FireTransform.GetUnitAxis(EAxis::X);

    const int32 NumPellets = FMath::Max(1, Stats.PelletCount);

    // 전체 펠릿 수의 절반을 올림한다.
    // 예: 9발 → 5발, 8발 → 4발.
    const int32 InnerPelletCount =
        NumPellets / 2 + NumPellets % 2;

    // 총구 중심 방향에서 벗어날 수 있는 최대 각도.
    // 누적 탄퍼짐과 기본 탄퍼짐은 사용하지 않는다.
    const float OuterAngleDegrees =
        FMath::Clamp(Stats.MaxSpreadAngle, 0.0f, 90.0f);

    const float InnerAngleDegrees = OuterAngleDegrees * 0.5f;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Pawn);

    int32 HitPelletCount = 0;

    for (int32 i = 0; i < NumPellets; ++i)
    {
        const float AngleDegrees = (i < InnerPelletCount)
                                       ? InnerAngleDegrees
                                       : OuterAngleDegrees;

        const FVector PelletDirection = FMath::VRandCone(
            FireDirection,
            FMath::DegreesToRadians(AngleDegrees));

        const FVector End =
            TraceStart +
            PelletDirection * FMath::Max(1.0f, Stats.Range);

        FHitResult PelletHit;
        const bool bPelletHit = World->LineTraceSingleByChannel(
            PelletHit,
            TraceStart,
            End,
            ECC_Visibility,
            Params);

        if (bPelletHit)
        {
            ++HitPelletCount;

            // 첫 명중 결과를 대표 이펙트용으로 전달한다.
            if (!bOutHit)
            {
                OutHit = PelletHit;
                bOutHit = true;
            }

            ApplyDamage(PelletHit);

            // 일반탄과 동일한 명중 디버그 표시.
            DrawDebugPoint(
                World,
                PelletHit.ImpactPoint,
                15.0f,
                FColor::Yellow,
                false,
                10.0f,
                1);
        }
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[ShotgunTraceDebug] Source=%s Pellets=%d Inner=%d Hits=%d"),
        bHasMuzzle ? TEXT("MuzzleSocket") : TEXT("WeaponMesh"),
        NumPellets,
        InnerPelletCount,
        HitPelletCount);

    return true;
}

void UWeaponCombatComponent::StartReload()
{
    // 약실에 탄약이 남아있는 경우 전술 재장전상태으로 간주한다.
    const bool bIsTactical = (CurrentAmmo > 0);
    const int32 MaxCapacity = bIsTactical ? (Stats.MagazineSize + 1) : Stats.MagazineSize;

    if (!bEquipped || bReloading || bSwapping ||
        CurrentAmmo >= MaxCapacity || ReserveAmmo <= 0)
    {
        return;
    }

    StopFire();
    bReloading = true;

    // 전술 재장전 여부에 따라 소요 시간 차등 적용한다.
    const float SelectedReloadTime = bIsTactical ? Stats.TacReloadTime : Stats.ReloadTime;

    GetWorld()->GetTimerManager().SetTimer(
        ReloadTimer,
        this,
        &UWeaponCombatComponent::ReloadStep,
        FMath::Max(0.01f, SelectedReloadTime),
        false);

    OnReloadChanged.Broadcast(true);
}

void UWeaponCombatComponent::EndSwap()
{
    bSwapping = false;
}

void UWeaponCombatComponent::ReloadStep()
{
    if (!bEquipped || !bReloading)
    {
        return;
    }

    const bool bIsTactical = (CurrentAmmo > 0);
    const int32 MaxCapacity = bIsTactical
                                  ? Stats.MagazineSize + 1
                                  : Stats.MagazineSize;

    const int32 Needed =
        FMath::Max(0, MaxCapacity - CurrentAmmo);

    int32 AmmoToLoad = FMath::Min(Needed, ReserveAmmo);

    // 산탄총은 한 번에 한 발만 장전한다.
    if (Stats.ReloadType == EWeaponReloadType::PerShell)
    {
        AmmoToLoad = FMath::Min(AmmoToLoad, 1);
    }

    CurrentAmmo += AmmoToLoad;
    ReserveAmmo -= AmmoToLoad;

    // 변수를 먼저 선언한 다음 아래에서 사용한다.
    const bool bContinueReload =
        Stats.ReloadType == EWeaponReloadType::PerShell &&
        CurrentAmmo < MaxCapacity &&
        ReserveAmmo > 0;

    if (bContinueReload)
    {
        GetWorld()->GetTimerManager().SetTimer(
            ReloadTimer,
            this,
            &UWeaponCombatComponent::ReloadStep,
            FMath::Max(0.01f, Stats.ShellInsertTime),
            false);
    }
    else
    {
        EndReload();
    }

    // 다음 삽입 애니메이션을 재생하는 BP 이벤트에도 전달된다.
    NotifyAmmo();
}

void UWeaponCombatComponent::EndReload()
{
    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }

    if (bReloading)
    {
        bReloading = false;
        OnReloadChanged.Broadcast(false);
    }
}

void UWeaponCombatComponent::NotifyAmmo()
{
    OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
}

void UWeaponCombatComponent::AddRecoil()
{
    // 발사 순간의 실제 앉기 상태를 확인한다.
    const ACharacter *Character = Cast<ACharacter>(Shooter.Get());
    const bool bCrouched = Character && Character->bIsCrouched;

    // 서 있으면 기존 반동, 앉으면 기존 반동의 70%를 적용한다.
    const float CrouchRecoilMultiplier = 0.7f;

    // 앉기와 조준 보정을 함께 적용한다.
    // 앉아서 조준하면 0.7 × 0.65 = 0.455배가 된다.
    const float CrouchMultiplier =
        bCrouched ? CrouchRecoilMultiplier : 1.0f;

    const float AimMultiplier =
        bAiming ? ADSRecoilMultiplier : 1.0f;

    const float StanceMultiplier =
        CrouchMultiplier * AimMultiplier;

    // 카메라 반동에 자세 배율을 적용한 뒤 누적한다.
    CameraTarget.X += FMath::FRandRange(
                          Stats.PitchKick.X,
                          Stats.PitchKick.Y) *
                      StanceMultiplier;

    CameraTarget.Y += FMath::FRandRange(
                          Stats.YawKick.X,
                          Stats.YawKick.Y) *
                      StanceMultiplier;

    // 총의 시각적 반동은 무기 고유 강도와 자세 배율을 함께 사용한다.
    const float VisualScale =
        Stats.VisualKickScale * StanceMultiplier;

    const auto RandomLocationKick = [](double A, double B) -> double
    {
        return FMath::FRandRange(
            static_cast<float>(FMath::Min(A, B)),
            static_cast<float>(FMath::Max(A, B)));
    };

    FVector LocationKick = FVector(
                               RandomLocationKick(Stats.LocationKickMin.X, Stats.LocationKickMax.X),
                               RandomLocationKick(Stats.LocationKickMin.Y, Stats.LocationKickMax.Y),
                               RandomLocationKick(Stats.LocationKickMin.Z, Stats.LocationKickMax.Z)) *
                           VisualScale;

    if (bAiming)
    {
        LocationKick.X *= FMath::Max(0.0f, Stats.ADSXLocationKickMultiplier);
    }
    VisualLocationTarget += LocationKick;

    // 기존 회전 범위를 유지하면서 앉았을 때 강도를 줄인다.
    // FRotator 생성자 순서는 Pitch, Yaw, Roll이다.
    const FRotator Kick(
        FMath::FRandRange(-3.0f, 3.0f) * VisualScale,
        FMath::FRandRange(
            -FMath::Max(0.0f, Stats.VisualYawKick),
            FMath::Max(0.0f, Stats.VisualYawKick)) *
            VisualScale,
        FMath::FRandRange(
            -FMath::Max(0.0f, Stats.VisualRollKick),
            FMath::Max(0.0f, Stats.VisualRollKick)) *
            VisualScale);

    VisualRotationTarget =
        (VisualRotationTarget.Quaternion() * Kick.Quaternion()).Rotator();

    // Select both pulse speeds at shot time so changing ADS mid-pulse cannot
    // change the current pulse's phase discontinuously.
    const float ShotInterval = 60.0f / FMath::Max(1.0f, Stats.RPM);
    const float PulseKickSpeed = FMath::Max(
        0.01f, bAiming && Stats.ADSVerticalPulseKickSpeed > 0.0f
                   ? Stats.ADSVerticalPulseKickSpeed
                   : Stats.VerticalPulseKickSpeed);
    const float PulseRecoverySpeed = FMath::Max(
        0.01f, bAiming && Stats.ADSVerticalPulseReturnSpeed > 0.0f
                   ? Stats.ADSVerticalPulseReturnSpeed
                   : Stats.VerticalPulseReturnSpeed);
    VerticalPulseStartTime = GetWorld()->GetTimeSeconds();
    VerticalPulseRiseDuration = ShotInterval * 0.25f / PulseKickSpeed;
    VerticalPulseDuration = VerticalPulseRiseDuration +
                            ShotInterval * 0.75f / PulseRecoverySpeed;

    // 전용 각도를 사용하고 기존 조준·앉기 감소만 적용한다.
    // VisualKickScale은 중복 적용하지 않는다.
    VerticalPulseAngle = Stats.ShotVerticalAngle * StanceMultiplier;
}

bool UWeaponCombatComponent::UpdateSprintFireLock()
{
    UWorld *World = GetWorld();
    APlayerController *PC = Shooter.IsValid()
                                ? Cast<APlayerController>(Shooter->GetController())
                                : nullptr;
    if (!bEquipped || !World || !PC || !PC->IsLocalController())
    {
        return false;
    }

    // Match the Left Shift check used by the weapon Blueprint.
    if (PC->IsInputKeyDown(EKeys::LeftShift))
    {
        if (!bSprintFireLocked)
        {
            StopFire();
        }
        bSprintFireLocked = true;
        bSprintKeyWasDown = true;
        return false;
    }

    const double Now = World->GetTimeSeconds();
    if (bSprintKeyWasDown)
    {
        bSprintKeyWasDown = false;
        SprintFireUnlockTime = Now + FMath::Max(0.0f, Stats.SprintToFireDelay);
    }

    // Also respect the existing shot interval and weapon swap delay.
    if (bSprintFireLocked && !bSwapping &&
        Now >= FMath::Max(SprintFireUnlockTime, NextFireTime))
    {
        bSprintFireLocked = false;
        return true;
    }
    return false;
}

void UWeaponCombatComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bEquipped)
    {
        if (UpdateSprintFireLock())
        {
            APlayerController *PC = Shooter.IsValid()
                                        ? Cast<APlayerController>(Shooter->GetController())
                                        : nullptr;
            if (PC && PC->IsInputKeyDown(EKeys::LeftMouseButton))
            {
                StartFire();
            }
        }
        UpdateRecoil(DeltaTime);
    }
}

void UWeaponCombatComponent::UpdateRecoil(float DeltaTime)
{
    const float CameraRecoverySpeed = FMath::Max(
        0.01f, bAiming && Stats.ADSCameraReturnSpeed > 0.0f
                   ? Stats.ADSCameraReturnSpeed
                   : Stats.CameraReturnSpeed);
    const float VisualRecoverySpeed = FMath::Max(
        0.01f, bAiming && Stats.ADSVisualReturnSpeed > 0.0f
                   ? Stats.ADSVisualReturnSpeed
                   : Stats.VisualReturnSpeed);

    const FVector2D PreviousCamera = CameraCurrent;

    CameraCurrent.X = FMath::FInterpTo(
        CameraCurrent.X, CameraTarget.X,
        DeltaTime, FMath::Max(0.01f, Stats.CameraKickSpeed));

    CameraCurrent.Y = FMath::FInterpTo(
        CameraCurrent.Y, CameraTarget.Y,
        DeltaTime, FMath::Max(0.01f, Stats.CameraKickSpeed));

    // 전체 반동값을 매 프레임 더하지 않고 변화량만 입력한다.
    // 따라서 같은 반동이 프레임마다 중복 적용되지 않는다.
    if (APawn *Pawn = Shooter.Get())
    {
        const FVector2D Delta = CameraCurrent - PreviousCamera;
        Pawn->AddControllerPitchInput(Delta.X);
        Pawn->AddControllerYawInput(Delta.Y);
    }

    CameraTarget.X = FMath::FInterpTo(
        CameraTarget.X, 0.0f,
        DeltaTime, CameraRecoverySpeed);

    CameraTarget.Y = FMath::FInterpTo(
        CameraTarget.Y, 0.0f,
        DeltaTime, CameraRecoverySpeed);

    VisualLocationCurrent = FMath::VInterpTo(
        VisualLocationCurrent, VisualLocationTarget,
        DeltaTime, FMath::Max(0.01f, Stats.VisualKickSpeed));

    VisualRotationCurrent = FMath::RInterpTo(
        VisualRotationCurrent, VisualRotationTarget,
        DeltaTime, FMath::Max(0.01f, Stats.VisualKickSpeed));

    float VerticalPulse = 0.0f;

    if (VerticalPulseStartTime >= 0.0 && GetWorld())
    {
        const double Elapsed =
            GetWorld()->GetTimeSeconds() - VerticalPulseStartTime;

        const float PulseTime = FMath::Max(0.0f, static_cast<float>(Elapsed));
        const float RiseDuration = FMath::Max(SMALL_NUMBER, VerticalPulseRiseDuration);
        const float ReturnDuration = FMath::Max(
            SMALL_NUMBER, VerticalPulseDuration - VerticalPulseRiseDuration);
        const float Phase = FMath::Clamp(
            PulseTime < VerticalPulseRiseDuration
                ? PulseTime / RiseDuration
                : 1.0f - (PulseTime - VerticalPulseRiseDuration) / ReturnDuration,
            0.0f, 1.0f);

        // 시작·정점·종료에서 부드럽게 연결되는 곡선.
        const float Shape = Phase * Phase * (3.0f - 2.0f * Phase);
        VerticalPulse = VerticalPulseAngle * Shape;

        if (PulseTime >= VerticalPulseDuration)
        {
            VerticalPulseStartTime = -1.0;
        }
    }

    if (USceneComponent *Visual = RecoilVisual.Get())
    {
        Visual->SetRelativeLocation(
            BaseVisualLocation + VisualLocationCurrent);

        // 기존 반동에 이번 발의 Z 회전만 추가한다.
        // 기존 누적값 자체에는 저장하지 않는다.
        FRotator FinalRotation = VisualRotationCurrent;
        FinalRotation.Roll += VerticalPulse;

        Visual->SetRelativeRotation(
            (BaseVisualRotation.Quaternion() * FinalRotation.Quaternion()).Rotator());
    }

    // 목표값 자체도 0으로 보간해서 원래 자세로 복귀시킨다.
    VisualLocationTarget = FMath::VInterpTo(
        VisualLocationTarget, FVector::ZeroVector,
        DeltaTime, VisualRecoverySpeed);

    VisualRotationTarget = FMath::RInterpTo(
        VisualRotationTarget, FRotator::ZeroRotator,
        DeltaTime, VisualRecoverySpeed);
}

void UWeaponCombatComponent::ResetRecoil()
{
    // 장착 해제 시 총 메시를 기본 자세로 복원한다.
    if (USceneComponent *Visual = RecoilVisual.Get())
    {
        Visual->SetRelativeLocation(BaseVisualLocation);
        Visual->SetRelativeRotation(BaseVisualRotation);
    }

    // 남아 있는 카메라 반동을 제거한다.
    if (APawn *Pawn = Shooter.Get())
    {
        Pawn->AddControllerPitchInput(-CameraCurrent.X);
        Pawn->AddControllerYawInput(-CameraCurrent.Y);
    }

    CameraTarget = FVector2D::ZeroVector;
    CameraCurrent = FVector2D::ZeroVector;

    VisualLocationTarget = FVector::ZeroVector;
    VisualLocationCurrent = FVector::ZeroVector;

    VisualRotationTarget = FRotator::ZeroRotator;
    VisualRotationCurrent = FRotator::ZeroRotator;
}

void UWeaponCombatComponent::SetAiming(bool bNewAiming)
{
    // 장착 중이고 재장전하지 않을 때만 조준할 수 있다.
    bAiming = bNewAiming && bEquipped && !bReloading;
}