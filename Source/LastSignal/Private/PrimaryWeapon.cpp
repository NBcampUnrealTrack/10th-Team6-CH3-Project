#include "PrimaryWeapon.h"

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
    PrimaryActorTick.bCanEverTick = false;

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
}

void APrimaryWeapon::BeginPlay()
{
    Super::BeginPlay();

    // 별도 입력 에셋 없이 사용할 수 있도록 런타임에 생성한다.
    FireAction = NewObject<UInputAction>(this);
    FireAction->ValueType = EInputActionValueType::Boolean;

    ReloadAction = NewObject<UInputAction>(this);
    ReloadAction->ValueType = EInputActionValueType::Boolean;

    WeaponMapping = NewObject<UInputMappingContext>(this);
    WeaponMapping->MapKey(FireAction, FireKey);
    WeaponMapping->MapKey(ReloadAction, ReloadKey);

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

    Combat->OnShot.AddDynamic(
        this, &APrimaryWeapon::HandleShot);

    Combat->OnReloadChanged.AddDynamic(
        this, &APrimaryWeapon::HandleReload);

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
    // 먼저 입력 상태를 차단한다.
    bEquipped = false;

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
}

void APrimaryWeapon::HandleReload(bool bReloading)
{
    PlayReloadEffects(bReloading);
}