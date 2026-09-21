#include "WeaponCombatComponent.h"

#include "ZombieAICharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"

UWeaponCombatComponent::UWeaponCombatComponent()
{
    // 사격은 타이머로 처리하고, 반동 보간만 Tick에서 처리한다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
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
}

void UWeaponCombatComponent::DeactivateWeapon()
{
    bEquipped = false;
    bTriggerHeld = false;
    CurrentSpreadHeat = 0.0f; // 누적 탄퍼짐 초기화

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimer);
        World->GetTimerManager().ClearTimer(ReloadTimer);
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
    if (!bEquipped || bTriggerHeld)
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

    const float MaxHeat = FMath::Max(0.0f, Stats.MaxSpreadAngle - Stats.BaseSpreadAngle);
    CurrentSpreadHeat = FMath::Clamp(CurrentSpreadHeat + Stats.SpreadIncreasePerShot, 0.0f, MaxHeat);

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
    APawn *Pawn = Shooter.Get();
    AController *Controller = Pawn ? Pawn->GetController() : nullptr;

    if (!Controller || !GetWorld())
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

    // 총구(Muzzle) 소켓을 먼저 조회 후 없을시 플레이어 위치를 기준으로 발사한다.
    FVector TraceStart = ViewLocation;
    if (USceneComponent *Visual = RecoilVisual.Get())
    {
        if (Visual->DoesSocketExist(TEXT("Muzzle")))
        {
            TraceStart = Visual->GetSocketLocation(TEXT("Muzzle"));
        }
    }

    // 현재 탄퍼짐을 연산한다.
    const float TotalSpread = FMath::Clamp(
        Stats.BaseSpreadAngle + CurrentSpreadHeat,
        0.0f,
        Stats.MaxSpreadAngle);

    const float HalfConeAngleRad = FMath::DegreesToRadians(TotalSpread * 0.5f);

    // 조준선기준 탄퍼짐 계산 및 최종 위치 설정한다.
    const FVector FireDirection = FMath::VRandCone(ViewRotation.Vector(), HalfConeAngleRad);
    const FVector End = TraceStart + (FireDirection * FMath::Max(1.0f, Stats.Range));

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Pawn);

    bOutHit = GetWorld()->LineTraceSingleByChannel(
        OutHit,
        TraceStart,
        End,
        ECC_Visibility,
        Params);

    // 명중하지 않아도 총알 한 발은 정상적으로 발사한 것이다.
    return true;
}

bool UWeaponCombatComponent::FireShotgun(
    FHitResult &OutHit,
    bool &bOutHit)
{
    APawn *Pawn = Shooter.Get();
    AController *Controller = Pawn ? Pawn->GetController() : nullptr;

    if (!Controller || !GetWorld())
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

    FVector TraceStart = ViewLocation;
    if (USceneComponent *Visual = RecoilVisual.Get())
    {
        if (Visual->DoesSocketExist(TEXT("Muzzle")))
        {
            TraceStart = Visual->GetSocketLocation(TEXT("Muzzle"));
        }
    }

    const float TotalSpread = FMath::Clamp(
        Stats.BaseSpreadAngle + CurrentSpreadHeat,
        0.0f,
        Stats.MaxSpreadAngle);

    const float HalfConeAngleRad = FMath::DegreesToRadians(TotalSpread * 0.5f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(Pawn);

    const int32 NumPellets = FMath::Max(1, Stats.PelletCount);
    bOutHit = false;

    // 설정된 PelletCount (예: 9발) 수만큼 확산 발사
    for (int32 i = 0; i < NumPellets; ++i)
    {
        const FVector PelletDir = FMath::VRandCone(ViewRotation.Vector(), HalfConeAngleRad);
        const FVector End = TraceStart + (PelletDir * FMath::Max(1.0f, Stats.Range));

        FHitResult PelletHit;
        const bool bPelletHit = GetWorld()->LineTraceSingleByChannel(
            PelletHit,
            TraceStart,
            End,
            ECC_Visibility,
            Params);

        if (bPelletHit)
        {
            // 대표 이펙트 전달용 (첫 번째 명중 피격점 저장)
            if (!bOutHit)
            {
                OutHit = PelletHit;
                bOutHit = true;
            }

            // 맞은 펠릿당 데미지 개별 적용
            ApplyDamage(PelletHit);
        }
    }

    return true;
}

void UWeaponCombatComponent::StartReload()
{
    // 약실에 탄약이 남아있는 경우 전술 재장전상태으로 간주한다.
    const bool bIsTactical = (CurrentAmmo > 0);
    const int32 MaxCapacity = bIsTactical ? (Stats.MagazineSize + 1) : Stats.MagazineSize;

    if (!bEquipped || bReloading ||
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

void UWeaponCombatComponent::ReloadStep()
{
    if (!bEquipped || !bReloading)
    {
        return;
    }

    // 전술 재장전일 경우 탄창 용량 + 약실 1발 장전
    const bool bIsTactical = (CurrentAmmo > 0);
    const int32 MaxCapacity = bIsTactical ? (Stats.MagazineSize + 1) : Stats.MagazineSize;

    const int32 Needed = FMath::Max(0, MaxCapacity - CurrentAmmo);

    int32 AmmoToLoad = FMath::Min(Needed, ReserveAmmo);

    // 개별 장전은 이번 단계에서 한 발만 삽입한다.
    if (Stats.ReloadType == EWeaponReloadType::PerShell)
    {
        AmmoToLoad = FMath::Min(AmmoToLoad, 1);
    }

    CurrentAmmo += AmmoToLoad;
    ReserveAmmo -= AmmoToLoad;

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
            FMath::Max(0.01f, Stats.ReloadTime),
            false);
    }
    else
    {
        EndReload();
    }

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
    // 한 발의 카메라 반동을 현재 목표에 누적한다.
    CameraTarget.X += FMath::FRandRange(
        Stats.PitchKick.X, Stats.PitchKick.Y);

    CameraTarget.Y += FMath::FRandRange(
        Stats.YawKick.X, Stats.YawKick.Y);

    // 블루프린트의 팔 위치 반동 범위를 옮겼다.
    // 새 총 메시의 로컬 축에 맞게 수치는 조정할 수 있다.
    VisualLocationTarget += FVector(
                                FMath::FRandRange(-10.0f, -5.0f),
                                FMath::FRandRange(1.0f, 4.0f),
                                FMath::FRandRange(-0.6f, 0.6f)) *
                            Stats.VisualKickScale;

    // FRotator 생성자 순서는 Pitch, Yaw, Roll이다.
    const FRotator Kick(
        FMath::FRandRange(-3.0f, 3.0f) * Stats.VisualKickScale,
        FMath::FRandRange(3.0f, 6.0f) * Stats.VisualKickScale,
        FMath::FRandRange(-2.0f, 2.0f) * Stats.VisualKickScale);

    VisualRotationTarget =
        (VisualRotationTarget.Quaternion() * Kick.Quaternion()).Rotator();
}

void UWeaponCombatComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bEquipped)
    {
        UpdateRecoil(DeltaTime);

        // 사격 중단 시 누적된 탄퍼짐이 줄어든다.
        if (CurrentSpreadHeat > 0.0f)
        {
            CurrentSpreadHeat = FMath::FInterpTo(CurrentSpreadHeat, 0.0f, DeltaTime, Stats.SpreadRecoverySpeed);
        }
    }
}

void UWeaponCombatComponent::UpdateRecoil(float DeltaTime)
{
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
        DeltaTime, FMath::Max(0.01f, Stats.CameraReturnSpeed));

    CameraTarget.Y = FMath::FInterpTo(
        CameraTarget.Y, 0.0f,
        DeltaTime, FMath::Max(0.01f, Stats.CameraReturnSpeed));

    VisualLocationCurrent = FMath::VInterpTo(
        VisualLocationCurrent, VisualLocationTarget,
        DeltaTime, FMath::Max(0.01f, Stats.VisualKickSpeed));

    VisualRotationCurrent = FMath::RInterpTo(
        VisualRotationCurrent, VisualRotationTarget,
        DeltaTime, FMath::Max(0.01f, Stats.VisualKickSpeed));

    if (USceneComponent *Visual = RecoilVisual.Get())
    {
        Visual->SetRelativeLocation(
            BaseVisualLocation + VisualLocationCurrent);

        Visual->SetRelativeRotation(
            (BaseVisualRotation.Quaternion() *
             VisualRotationCurrent.Quaternion())
                .Rotator());
    }

    // 목표값 자체도 0으로 보간해서 원래 자세로 복귀시킨다.
    VisualLocationTarget = FMath::VInterpTo(
        VisualLocationTarget, FVector::ZeroVector,
        DeltaTime, FMath::Max(0.01f, Stats.VisualReturnSpeed));

    VisualRotationTarget = FMath::RInterpTo(
        VisualRotationTarget, FRotator::ZeroRotator,
        DeltaTime, FMath::Max(0.01f, Stats.VisualReturnSpeed));
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

    CurrentSpreadHeat = 0.0f;
    CameraTarget = FVector2D::ZeroVector;
    CameraCurrent = FVector2D::ZeroVector;

    VisualLocationTarget = FVector::ZeroVector;
    VisualLocationCurrent = FVector::ZeroVector;

    VisualRotationTarget = FRotator::ZeroRotator;
    VisualRotationCurrent = FRotator::ZeroRotator;
}