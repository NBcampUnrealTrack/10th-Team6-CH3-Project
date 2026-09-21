#include "PrimaryWeapon.h"

// UI 추가
#include "LastSignalPlayerController.h"       
#include "LastSignalPlayerHUDComponent.h"   

#include "WeaponCombatComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
 

APrimaryWeapon::APrimaryWeapon()
{
    // 레티클 목표점을 매 프레임 갱신한다.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // 일반적인 애니메이션·반동 갱신 이후에 머즐 위치를 읽는다.
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(
        TEXT("WeaponRoot"));
    SetRootComponent(WeaponRoot);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(
        TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(WeaponRoot);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(false);

    Combat = CreateDefaultSubobject<UWeaponCombatComponent>(
        TEXT("Combat"));

    WeaponInput = CreateDefaultSubobject<UEnhancedInputComponent>(
        TEXT("WeaponInput"));

    // 이동 등 다른 입력 전체를 막지는 않는다.
    WeaponInput->bBlockInput = false;
    WeaponInput->Priority = 1;

    FireKey = EKeys::LeftMouseButton;
    ReloadKey = EKeys::R;
    AimKey = EKeys::RightMouseButton;
}

void APrimaryWeapon::BeginPlay()
{
    Super::BeginPlay();

    // 별도 입력 에셋 없이 사용할 수 있도록 런타임에 생성한다.
    FireAction = NewObject<UInputAction>(this);
    FireAction->ValueType = EInputActionValueType::Boolean;

    ReloadAction = NewObject<UInputAction>(this);
    ReloadAction->ValueType = EInputActionValueType::Boolean;
    AimAction = NewObject<UInputAction>(this);
    AimAction->ValueType = EInputActionValueType::Boolean;

    WeaponMapping = NewObject<UInputMappingContext>(this);
    WeaponMapping->MapKey(FireAction, FireKey);
    WeaponMapping->MapKey(ReloadAction, ReloadKey);
    WeaponMapping->MapKey(AimAction, AimKey);

    // Started는 누르는 순간, Completed는 놓는 순간이다.
    WeaponInput->BindAction(
        FireAction, ETriggerEvent::Started,
        this, &APrimaryWeapon::FirePressed);

    WeaponInput->BindAction(
        FireAction, ETriggerEvent::Completed,
        this, &APrimaryWeapon::FireReleased);

    WeaponInput->BindAction(
        FireAction, ETriggerEvent::Canceled,
        this, &APrimaryWeapon::FireReleased);

    WeaponInput->BindAction(
        ReloadAction, ETriggerEvent::Started,
        this, &APrimaryWeapon::ReloadPressed);

    // 우클릭을 누르면 조준하고, 놓거나 입력이 취소되면 해제한다.
    WeaponInput->BindAction(
        AimAction, ETriggerEvent::Started,
        this, &APrimaryWeapon::AimPressed);

    WeaponInput->BindAction(
        AimAction, ETriggerEvent::Completed,
        this, &APrimaryWeapon::AimReleased);

    WeaponInput->BindAction(
        AimAction, ETriggerEvent::Canceled,
        this, &APrimaryWeapon::AimReleased);

    Combat->OnShot.AddDynamic(
        this, &APrimaryWeapon::HandleShot);

    Combat->OnReloadChanged.AddDynamic(
        this, &APrimaryWeapon::HandleReload);

    // UI 추가
    Combat->OnAmmoChanged.AddDynamic(              
        this, &APrimaryWeapon::HandleAmmoChanged); 

    // 장착되기 전에는 보이지 않고 입력도 받지 않는다.
    SetActorHiddenInGame(true);
}

void APrimaryWeapon::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    Unequip();

    Combat->OnShot.RemoveDynamic(
        this, &APrimaryWeapon::HandleShot);

    Combat->OnReloadChanged.RemoveDynamic(
        this, &APrimaryWeapon::HandleReload);

    // UI 추가
    Combat->OnAmmoChanged.RemoveDynamic(           
        this, &APrimaryWeapon::HandleAmmoChanged); 

    Super::EndPlay(EndPlayReason);
}

bool APrimaryWeapon::Equip(
    APawn *NewOwner,
    USceneComponent *AttachParent,
    FName SocketName)
{
    // 일반 SpawnActor가 끝난 이후에 장착해야 한다.
    if (!HasActorBegunPlay() ||
        !IsValid(NewOwner) || !IsValid(AttachParent))
    {
        return false;
    }

    APlayerController *PC =
        Cast<APlayerController>(NewOwner->GetController());

    if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer())
    {
        return false;
    }

    UEnhancedInputLocalPlayerSubsystem *Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
            PC->GetLocalPlayer());

    if (!Subsystem)
    {
        return false;
    }

    Unequip();

    const bool bAttached = AttachToComponent(
        AttachParent,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        SocketName);

    if (!bAttached)
    {
        return false;
    }

    SetOwner(NewOwner);
    SetInstigator(NewOwner);

    EquippedController = PC;
    InputSubsystem = Subsystem;
    bEquipped = true;

    SetActorHiddenInGame(false);

    // 무기 고유 매핑이므로 해제 시 다른 입력 매핑을 건드리지 않는다.
    Subsystem->AddMappingContext(WeaponMapping, 1);
    PC->PushInputComponent(WeaponInput);

    // 총 메시의 현재 상대 위치·회전을 반동 기준 자세로 저장한다.
    Combat->ActivateWeapon(NewOwner, WeaponMesh);

    return true;
}

void APrimaryWeapon::Unequip()
{
    // 장착 해제 시 조준 입력과 실제 조준 상태를 함께 정리한다.
    bEquipped = false;
    bAimHeld = false;
    RefreshAiming();

    // 아래에는 기존 입력 해제 및 무기 비활성화 코드 유지.
    if (APlayerController *PC = EquippedController.Get())
    {
        PC->PopInputComponent(WeaponInput);
    }

    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            InputSubsystem.Get())
    {
        if (WeaponMapping)
        {
            Subsystem->RemoveMappingContext(WeaponMapping);
        }
    }

    if (Combat)
    {
        Combat->DeactivateWeapon();
    }

    EquippedController.Reset();
    InputSubsystem.Reset();

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorHiddenInGame(true);
    SetOwner(nullptr);
    SetInstigator(nullptr);
}

void APrimaryWeapon::FirePressed()
{
    if (bEquipped)
    {
        Combat->StartFire();
    }
}

void APrimaryWeapon::FireReleased()
{
    Combat->StopFire();
}

void APrimaryWeapon::ReloadPressed()
{
    if (bEquipped)
    {
        Combat->StartReload();
    }
}

void APrimaryWeapon::HandleShot(
    bool bHit,
    const FHitResult &HitResult)
{
    UE_LOG(LogTemp, Log, TEXT("Shot fired. Hit=%s Actor=%s"),
           bHit ? TEXT("true") : TEXT("false"),
           *GetNameSafe(HitResult.GetActor())); // 테스트용: 맞았는지/뭘 맞았는지 바로 확인하려고 넣었어요 (꼭 확인 필요)
    
    
    PlayShotEffects(bHit, HitResult);

    // UI 추가: 맞았을 때만 히트마커를 요청한다.

        if (bHit && EquippedController.IsValid())
    {
        if (ALastSignalPlayerController *LastSignalPC =
                Cast<ALastSignalPlayerController>(EquippedController.Get()))
        {
            if (ULastSignalPlayerHUDComponent *HUD = LastSignalPC->GetHUDComponent())
            {
                HUD->RequestHitMarker();
            }
        }
    }
}



void APrimaryWeapon::HandleReload(bool bReloading)
{
    // 재장전 연출을 먼저 갱신한다.
    // 재장전 종료 시에는 해당 몽타주를 정리한 뒤 조준을 복구한다.
    PlayReloadEffects(bReloading);

    // 재장전 시작 시 조준 해제.
    // 완료 또는 취소 시 우클릭을 유지하고 있으면 다시 조준.
    RefreshAiming();
}

// UI 추가
void APrimaryWeapon::HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo)
{
    APlayerController *PC = EquippedController.Get();
    if (!PC)
    {
        return;
    }

    ALastSignalPlayerController *LastSignalPC =
        Cast<ALastSignalPlayerController>(PC);

    if (!LastSignalPC)
    {
        return;
    }

    if (ULastSignalPlayerHUDComponent *HUD = LastSignalPC->GetHUDComponent())
    {
        FLastSignalWeaponHUDData WeaponData;
        WeaponData.WeaponName = FText::FromString(GetClass()->GetName());
        WeaponData.CurrentAmmo = CurrentAmmo;
        WeaponData.MagazineSize = Combat->Stats.MagazineSize;

        HUD->SetWeapon(WeaponData);
    }
}

bool APrimaryWeapon::IsAiming() const
{
    return Combat && Combat->IsAiming();
}

void APrimaryWeapon::AimPressed()
{
    if (!bEquipped)
    {
        return;
    }

    bAimHeld = true;
    RefreshAiming();
}

void APrimaryWeapon::AimReleased()
{
    bAimHeld = false;
    RefreshAiming();
}

void APrimaryWeapon::RefreshAiming()
{
    if (!Combat)
    {
        return;
    }

    const bool bWasAiming = Combat->IsAiming();

    // 재장전 중 조준 차단은 전투 컴포넌트에서 판단한다.
    Combat->SetAiming(bEquipped && bAimHeld);

    const bool bNowAiming = Combat->IsAiming();

    // 같은 상태에서 전환 애니메이션이 반복 재생되지 않게 한다.
    if (bWasAiming != bNowAiming)
    {
        PlayAimEffects(bNowAiming);
    }
}