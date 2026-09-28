#include "PlayerCharacter.h"
#include "LastSignalGameInstance.h"
#include "AdrenalineSkill.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InteractableTarget.h"
#include "PrimaryWeapon.h"
#include "SkillComponent.h"

// UI 추가
#include "LastSignalPlayerController.h"
#include "LastSignalPlayerHUDComponent.h"

APlayerCharacter::APlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false; // 1인칭이라 캐릭터 몸통 회전 x 카메라만 회전시키려고 만들었숩니다.
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-11.765135f, 11.176215f, 13.473909f)); // 눈높이 임의 설정
    FirstPersonCameraComponent->bUsePawnControlRotation = true;                                    // 마우스로 카메라 상하좌우 회전

    GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // 웅크리시 설정
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed; // 웅크리기시 속도감소

    // 스킬 컴포넌트 생성
    SkillComponent = CreateDefaultSubobject<UAdrenalineSkill>(TEXT("SkillComponent"));
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 설정된 카메라 높이를 웅크리기 보정의 기준으로 저장한다.
    DefaultCameraRelativeZ =
        FirstPersonCameraComponent->GetRelativeLocation().Z;

    if (APlayerController *PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: DefaultMappingContext successfully added."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter: DefaultMappingContext is missing!"));
            }
        }
    }

    // 무기 슬롯 2개 초기화
    WeaponSlots.Init(nullptr, 2);

    // 시작 시 보조무기 스폰 및 장착
    if (WeaponClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = this;

        APrimaryWeapon *SpawnedSecondary = GetWorld()->SpawnActor<APrimaryWeapon>(WeaponClass, SpawnParams);
        if (SpawnedSecondary)
        {
            WeaponSlots[1] = SpawnedSecondary;
            SwitchWeapon(1); // 시작 시 보조무기 장착
            UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Secondary weapon [%s] spawned and equipped in slot 1."), *SpawnedSecondary->GetName());
        }
    }

    else
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: No WeaponClass specified in Blueprint defaults."));
    }

    if (ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(GetGameInstance()))
    {
        if (GI->bSavedIsSwapUnlocked && GI->SavedPrimaryWeaponClass)
        {
            EquipPrimaryWeapon(GI->SavedPrimaryWeaponClass);

            if (WeaponSlots.IsValidIndex(GI->SavedEquippedSlotIndex) && WeaponSlots[GI->SavedEquippedSlotIndex])
            {
                SwitchWeapon(GI->SavedEquippedSlotIndex);
            }
        }
        else
        {
            bIsSwapUnlocked = false;
        }
    }
    else
    {
        bIsSwapUnlocked = false;
    }

    RestoreStateFromGameInstance();
}
	
void APlayerCharacter::EquipPrimaryWeapon(TSubclassOf<APrimaryWeapon> NewWeaponClass)
{
    if (!NewWeaponClass)
        return;

    // 기존 슬롯 0 무기 파괴 (재교체 기능 지원)
    if (WeaponSlots[0])
    {
        WeaponSlots[0]->Unequip();
        WeaponSlots[0]->Destroy();
        WeaponSlots[0] = nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;

    APrimaryWeapon *NewPrimary = GetWorld()->SpawnActor<APrimaryWeapon>(NewWeaponClass, SpawnParams);
    if (NewPrimary)
    {
        WeaponSlots[0] = NewPrimary;
        bIsSwapUnlocked = true; // 스왑 기능 해금
        SwitchWeapon(0); // 주무기 장착
        UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Primary weapon [%s] spawned and equipped in slot 0."), *NewPrimary->GetName());
       
        if (ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(GetGameInstance()))
        {
            GI->bSavedIsSwapUnlocked = true;
            GI->SavedPrimaryWeaponClass = NewWeaponClass;
        }
    }
}

void APlayerCharacter::OnWeaponSlot1()
{
    if (!bIsSwapUnlocked || !WeaponSlots[0])
        return;

    SwitchWeapon(0);
}

void APlayerCharacter::OnWeaponSlot2()
{
    if (!bIsSwapUnlocked || !WeaponSlots[1])
        return;

    SwitchWeapon(1);
}

void APlayerCharacter::SwitchWeapon(int32 SlotIndex)
{
    if (!WeaponSlots.IsValidIndex(SlotIndex))
        return;

    APrimaryWeapon *TargetWeapon = WeaponSlots[SlotIndex];
    if (!TargetWeapon)
        return;

    if (CurrentWeaponIndex == SlotIndex && CurrentWeapon == TargetWeapon)
        return;

    if (EquippedWeapon)
    {
        EquippedWeapon->Unequip();
    }

    CurrentWeaponIndex = SlotIndex;
    EquippedWeapon = TargetWeapon;
    CurrentWeapon = TargetWeapon;

    EquippedWeapon->Equip(this, FirstPersonCameraComponent, NAME_None);
    UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Switched to weapon [%s] in slot %d."), *EquippedWeapon->GetName(), SlotIndex);
}


void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 앉기 처리가 먼저 기본 카메라 높이를 설정한다.
    UpdateCameraCrouchInterp(DeltaTime);

    // 기본 높이 위에 작은 보행 흔들림을 더한다.
    UpdateMovementBob(DeltaTime);
}

void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) // 웅크리기 시작 카메라
{
    Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    CameraCrouchOffsetZ += HalfHeightAdjust; // 원래의 반
}

void APlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) // 웅크리기 끝 카메라
{
    Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
    CameraCrouchOffsetZ -= HalfHeightAdjust; // 원래의 반
}

void APlayerCharacter::UpdateCameraCrouchInterp(float DeltaTime) // 웅크리기 시 카메라 높이 연결
{
    if (!FMath::IsNearlyZero(CameraCrouchOffsetZ, 0.1f)) // 웅크리기 시작하면 카메라 높이 보정값이 생기고, 그 값이 0에 가까워질 때까지 보정
    {
        const float CurrentInterpSpeed = bIsCrouched ? CrouchDownInterpSpeed : StandUpInterpSpeed; // 웅크리기 시작할 때와 끝날 때 속도 다르게
        CameraCrouchOffsetZ = FMath::FInterpConstantTo(CameraCrouchOffsetZ, 0.0f, DeltaTime, CurrentInterpSpeed);
    }
    else
    {
        CameraCrouchOffsetZ = 0.0f;
    }

    const FVector CurrentRelLoc = FirstPersonCameraComponent->GetRelativeLocation();
    FirstPersonCameraComponent->SetRelativeLocation(FVector(CurrentRelLoc.X, CurrentRelLoc.Y, DefaultCameraRelativeZ + CameraCrouchOffsetZ));
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent *EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        if (SprintAction) // 달리기: 누르고 있으면 달리기, 떼거거나 cancleed 되면 일단 걷기로 복귀
        {
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopSprint);
        }

        if (CrouchAction) // 웅크리기: 누르고 있으면 웅크리기, 떼거나 canceled 되면 일단 일반 높이로 복귀
        {
            EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);
            EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
            EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &APlayerCharacter::StopCrouch);
        }

        if (SkillAction) // 스킬 입력 바인딩
        {
            EnhancedInput->BindAction(SkillAction, ETriggerEvent::Started, this, &APlayerCharacter::UseSkill);
        }

        if (InteractAction) // 상호작용 입력 바인딩
        {
            EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::TryInteract);
        }
        if (WeaponSlot1Action)
        {
            EnhancedInput->BindAction(WeaponSlot1Action, ETriggerEvent::Started, this, &APlayerCharacter::OnWeaponSlot1);
        }

        if (WeaponSlot2Action)
        {
            EnhancedInput->BindAction(WeaponSlot2Action, ETriggerEvent::Started, this, &APlayerCharacter::OnWeaponSlot2);
        }

        if (PauseAction) // 일시정지 메뉴 입력 바인딩
        {
            EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerCharacter::TogglePauseMenu);
        }

    }
}

void APlayerCharacter::TryInteract() // 인터랙트
{
    if (NearbyInteractable && NearbyInteractable->Implements<UInteractableTarget>())
        IInteractableTarget::Execute_Interact(NearbyInteractable, this);
}

void APlayerCharacter::TogglePauseMenu() // 일시정지
{
   
    if (ALastSignalPlayerController *LastSignalPC = Cast<ALastSignalPlayerController>(GetController()))
    {
        LastSignalPC->TogglePauseMenu();
    }
}


void APlayerCharacter::Move(const FInputActionValue &Value) // Move 함수
{
    const FVector2D MoveInput = Value.Get<FVector2D>();

    if (Controller)
    {
        const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
        const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(Forward, MoveInput.Y);
        AddMovementInput(Right, MoveInput.X);
    }
}

void APlayerCharacter::StartSprint()
{
    // 달리기 입력 상태를 흔들림에도 전달한다.
    bSprintBobRequested = true;
    if (bIsCrouched)
    {
        UnCrouch(); // 웅크린 상태에서 달리기를 누르면 일어남
    }
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopSprint()
{
    bSprintBobRequested = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::StartCrouch()
{
    StopSprint(); // 달리던 중 웅크리면 달리기 해제
    Crouch();     // ACharacter 내장 웅크리기
}

void APlayerCharacter::StopCrouch()
{
    UnCrouch(); // ACharacter 내장 일어서기
}

void APlayerCharacter::Look(const FInputActionValue &Value) // Look 함수
{
    const FVector2D LookInput = Value.Get<FVector2D>();

    if (Controller)
    {
        AddControllerYawInput(LookInput.X);
        AddControllerPitchInput(LookInput.Y);
    }
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const &DamageEvent, AController *EventInstigator, AActor *DamageCauser) // 데미지 받을떄 자동 호출 함수 구현22222
{
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser); // 부모(ACharacter)가 데미지 계산을 먼저 해주고, 실제 적용된 데미지량을 돌려줌 -> 기본 로직에 데미지량이 바뀔 수 있어서 최종 데미지를 부모가 계산하고 그 값을 우리가 이용함

    if (ActualDamage <= 0.0f || CurrentHealth <= 0.0f) // 데미지가 0이거나, 이미 죽어있으면 더 처리할 필요 없음
    {
        return ActualDamage;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth); // 체력 깎기. Clamp로 0~MaxHealth 범위를 벗어나지 않게 고정

    // UI 추가
    UE_LOG(LogTemp, Log, TEXT("Player took %.0f damage, Health: %.0f/%.0f"), ActualDamage, CurrentHealth, MaxHealth); // 로그로 데미지량과 체력 상태 확인
    if (APlayerController *PC = Cast<APlayerController>(GetController()))
    {
        if (ALastSignalPlayerController *LastSignalPC = Cast<ALastSignalPlayerController>(PC))
        {
            if (ULastSignalPlayerHUDComponent *HUD = LastSignalPC->GetHUDComponent())
            {
                HUD->ApplyDamage(ActualDamage);
            }
        }
    }
    // 여기까지

    if (CurrentHealth <= 0.0f) // 체력 0 이하가 되면 죽음 처리
    {
        Die();
    }

    return ActualDamage;
}

void APlayerCharacter::Die() // 죽음 처리 (지금은 테스트 위해서 최소한만 구현했어요 — 나중에 애니메이션/입력 차단 같은거 만들 예정)
{

    UE_LOG(LogTemp, Warning, TEXT("Player Character DIED! Broadcasting OnDied event.")); // 로그로 죽음 처리 확인

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PLAYER DIED"));
    }

    OnDied.Broadcast(); // 구독해둔 다른 클래스들(나중에 GameMode 등)에게 "죽었다"고 방송 (델리게이트라서 있는거에요)
}

// 스킬 실행 함수
void APlayerCharacter::UseSkill()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] UseSkill() 입력 호출됨"));

    if (SkillComponent)
    {
        SkillComponent->ActivateSkill();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerCharacter] SkillComponent가 유효하지 않습니다(nullptr)!"));
    }
}

// 점프 키를 눌렀을 때가 아니라 실제 점프가 성공했을 때 실행한다.
void APlayerCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();

    if (IsLocallyControlled() && CurrentHealth > 0.0f)
    {
        JumpBobElapsed = 0.0f;
        LandBobElapsed = -1.0f;
    }
}

void APlayerCharacter::Landed(const FHitResult &Hit)
{
    // Landed에서는 아직 착지 전 수직 속도를 읽을 수 있다.
    const float ImpactSpeed = FMath::Max(0.0f, -GetVelocity().Z);
    Super::Landed(Hit);

    if (IsLocallyControlled() && CurrentHealth > 0.0f)
    {
        // 작은 턱에서 생기는 아주 약한 낙하는 무시한다.
        LandBobStrength = ImpactSpeed >= 100.0f
                              ? FMath::Clamp(ImpactSpeed / FMath::Max(LandReferenceSpeed, 1.0f),
                                             0.0f, FMath::Max(LandMaxScale, 0.0f))
                              : 0.0f;

        LandBobElapsed = LandBobStrength > 0.0f ? 0.0f : -1.0f;
        JumpBobElapsed = -1.0f;
    }
}

// 보행은 반복 파형, 점프와 착지는 한 번 이동한 뒤 복귀하는 파형으로 합성한다.
void APlayerCharacter::UpdateMovementBob(float DeltaTime)
{
    if (!FirstPersonCameraComponent || !IsLocallyControlled())
    {
        return;
    }

    APrimaryWeapon *Weapon = EquippedWeapon.Get();
    const bool bAttachedWeapon = IsValid(Weapon) && Weapon->GetRootComponent() && Weapon->GetRootComponent()->GetAttachParent() == FirstPersonCameraComponent;

    // 무기 메시 반동과 겹치지 않도록 장착 액터의 상대 위치만 변경한다.
    APrimaryWeapon *ActiveBobWeapon = bAttachedWeapon ? Weapon : nullptr;
    if (BobWeapon.Get() != ActiveBobWeapon)
    {
        if (APrimaryWeapon *PreviousWeapon = BobWeapon.Get())
        {
            if (PreviousWeapon->GetRootComponent() && PreviousWeapon->GetRootComponent()->GetAttachParent() == FirstPersonCameraComponent)
            {
                PreviousWeapon->SetActorRelativeLocation(BobWeaponBaseLocation);
            }
        }

        BobWeapon = ActiveBobWeapon;
        if (ActiveBobWeapon)
        {
            BobWeaponBaseLocation =
                ActiveBobWeapon->GetRootComponent()->GetRelativeLocation();
        }
    }

    const float Speed = GetVelocity().Size2D();
    const bool bAlive = CurrentHealth > 0.0f;
    const bool bMovingOnGround = bAlive && GetCharacterMovement()->IsMovingOnGround() && Speed > 5.0f;
    const bool bSprint = bSprintBobRequested && !bIsCrouched && bAlive;
    const float InterpSpeed = FMath::Max(BobBlendSpeed, 0.01f);

    // 걷기/달리기 전환 시 강도와 주파수를 함께 부드럽게 변경한다.
    SprintBobBlend = FMath::FInterpTo(SprintBobBlend,
                                      bSprint ? 1.0f : 0.0f, DeltaTime, InterpSpeed);

    const float ReferenceSpeed = FMath::Max(
        FMath::Lerp(WalkSpeed, SprintSpeed, SprintBobBlend), 1.0f);
    const float SpeedRatio = FMath::Clamp(Speed / ReferenceSpeed, 0.0f, 1.0f);

    MovementBobWeight = FMath::FInterpTo(MovementBobWeight,
                                         bMovingOnGround ? SpeedRatio : 0.0f, DeltaTime, InterpSpeed);

    const bool bAiming = IsValid(Weapon) && Weapon->IsAiming();
    const float StanceTarget = (bAiming ? AimBobScale : 1.0f) * (bIsCrouched ? CrouchBobScale : 1.0f);
    SmoothedBobStance = FMath::FInterpTo(SmoothedBobStance,
                                         StanceTarget, DeltaTime, InterpSpeed);

    const float WeaponFrequency = FMath::Lerp(
        WalkWeaponBobFrequency, SprintWeaponBobFrequency, SprintBobBlend);
    const float CameraFrequency = FMath::Lerp(
        WalkCameraBobFrequency, SprintCameraBobFrequency, SprintBobBlend);

    if (bMovingOnGround)
    {
        // 입력만 유지하고 벽에 막힌 상태에서는 주기가 진행되지 않는다.
        const float PhaseStep = DeltaTime * 2.0f * PI * FMath::Clamp(SpeedRatio, 0.5f, 1.0f);
        MovementBobPhase = FMath::Fmod(MovementBobPhase + PhaseStep * FMath::Max(WeaponFrequency, 0.01f), 2.0f * PI);
        CameraBobPhase = FMath::Fmod(CameraBobPhase + PhaseStep * FMath::Max(CameraFrequency, 0.01f), 2.0f * PI);
    }

    const FVector WeaponAmplitude = FMath::Lerp(
        WalkWeaponBobAmplitude * FMath::Max(WeaponBobScale, 0.0f),
        SprintWeaponBobAmplitude * FMath::Max(SprintWeaponBobScale, 0.0f),
        SprintBobBlend);
    const float CameraHeight = FMath::Lerp(
        CameraBobHeight, SprintCameraBobHeight, SprintBobBlend);

    const float SideWave = FMath::Sin(MovementBobPhase);
    const float UpWave = FMath::Sin(MovementBobPhase * 2.0f);
    FVector WeaponOffset = FVector(
                               UpWave * WeaponAmplitude.X,
                               SideWave * WeaponAmplitude.Y,
                               UpWave * WeaponAmplitude.Z) *
                           MovementBobWeight;
    float CameraOffsetZ = FMath::Sin(CameraBobPhase) * CameraHeight * MovementBobWeight;

    if (!bAlive)
    {
        JumpBobElapsed = -1.0f;
        LandBobElapsed = -1.0f;
    }

    // 카메라와 팔의 지속시간이 달라도 각각 원위치까지 복귀시킨다.
    const auto AdvancePulse = [DeltaTime](float &Elapsed, float MaxDuration)
    {
        if (Elapsed >= 0.0f)
        {
            Elapsed += DeltaTime;
            if (Elapsed >= FMath::Max(MaxDuration, 0.01f))
            {
                Elapsed = -1.0f;
            }
        }
    };

    // 시작과 끝에서 속도가 0이 되는 부드러운 일회성 눌림을 만든다.
    const auto Pulse = [](float Elapsed, float Duration)
    {
        const float SafeDuration = FMath::Max(Duration, 0.01f);
        if (Elapsed < 0.0f || Elapsed >= SafeDuration)
        {
            return 0.0f;
        }
        const float Wave = FMath::Sin(PI * Elapsed / SafeDuration);
        return Wave * Wave;
    };

    AdvancePulse(JumpBobElapsed, FMath::Max(JumpWeaponDuration, JumpCameraDuration));
    AdvancePulse(LandBobElapsed, FMath::Max(LandWeaponDuration, LandCameraDuration));

    WeaponOffset += JumpWeaponOffset * Pulse(JumpBobElapsed, JumpWeaponDuration);
    WeaponOffset += LandWeaponOffset * Pulse(LandBobElapsed, LandWeaponDuration) * LandBobStrength;
    CameraOffsetZ += JumpCameraOffsetZ * Pulse(JumpBobElapsed, JumpCameraDuration);
    CameraOffsetZ += LandCameraOffsetZ * Pulse(LandBobElapsed, LandCameraDuration) * LandBobStrength;

    if (ActiveBobWeapon)
    {
        ActiveBobWeapon->SetActorRelativeLocation(
            BobWeaponBaseLocation + WeaponOffset * SmoothedBobStance);
    }

    // 매 프레임 기준 높이부터 계산하므로 흔들림이 누적되지 않는다.
    // 컨트롤러 회전을 건드리지 않아 카메라 반동과 별도로 동작한다.
    FVector CameraLocation = FirstPersonCameraComponent->GetRelativeLocation();
    CameraLocation.Z = DefaultCameraRelativeZ + CameraCrouchOffsetZ + CameraOffsetZ * SmoothedBobStance;
    FirstPersonCameraComponent->SetRelativeLocation(CameraLocation);
}

void APlayerCharacter::SaveStateToGameInstance()
{
    ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(GetGameInstance());
    if (!GI)
        return;

    GI->SavedHP = CurrentHealth;
    GI->SavedEquippedSlotIndex = CurrentWeaponIndex;

    if (WeaponSlots.IsValidIndex(0) && WeaponSlots[0] && WeaponSlots[0]->GetCombatComponent())
    {
        GI->SavedPrimaryCurrentAmmo = WeaponSlots[0]->GetCombatComponent()->GetCurrentAmmo();
        GI->SavedPrimaryReserveAmmo = WeaponSlots[0]->GetCombatComponent()->GetReserveAmmo();
    }

    if (WeaponSlots.IsValidIndex(1) && WeaponSlots[1] && WeaponSlots[1]->GetCombatComponent())
    {
        GI->SavedSecondaryCurrentAmmo = WeaponSlots[1]->GetCombatComponent()->GetCurrentAmmo();
        GI->SavedSecondaryReserveAmmo = WeaponSlots[1]->GetCombatComponent()->GetReserveAmmo();
    }
}

void APlayerCharacter::RestoreStateFromGameInstance()
{
    ULastSignalGameInstance *GI = Cast<ULastSignalGameInstance>(GetGameInstance());
    if (!GI)
        return;

    if (GI->SavedPrimaryCurrentAmmo >= 0 && WeaponSlots.IsValidIndex(0) && WeaponSlots[0] && WeaponSlots[0]->GetCombatComponent())
    {
        WeaponSlots[0]->GetCombatComponent()->SetAmmo(GI->SavedPrimaryCurrentAmmo, GI->SavedPrimaryReserveAmmo);
    }

    if (GI->SavedSecondaryCurrentAmmo >= 0 && WeaponSlots.IsValidIndex(1) && WeaponSlots[1] && WeaponSlots[1]->GetCombatComponent())
    {
        WeaponSlots[1]->GetCombatComponent()->SetAmmo(GI->SavedSecondaryCurrentAmmo, GI->SavedSecondaryReserveAmmo);
    }

    if (EquippedWeapon)
    {
        EquippedWeapon->UpdateHUD();
    }
}

void APlayerCharacter::RefillAllAmmo()
{
    for (APrimaryWeapon *Weapon : WeaponSlots)
    {
        if (Weapon && Weapon->GetCombatComponent())
        {
            UWeaponCombatComponent *Combat = Weapon->GetCombatComponent();

            const int32 MaxMag = Combat->Stats.MagazineSize;
            const int32 MaxReserve = Combat->Stats.InitialReserveAmmo;

            Combat->SetAmmo(MaxMag, MaxReserve);
        }
    }

    if (EquippedWeapon)
    {
        EquippedWeapon->UpdateHUD();
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] 모든 무기의 탄약이 가득 찼습니다."));
}