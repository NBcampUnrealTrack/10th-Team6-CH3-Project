#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/EngineTypes.h"
#include "PrimaryWeapon.h"
#include "SkillComponent.h"
#include "AdrenalineSkill.h"
#include "Engine/World.h"
#include "InteractableTarget.h"

// UI 추가
#include "LastSignalPlayerController.h"      
#include "LastSignalPlayerHUDComponent.h"    


APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;  // 1인칭이라 캐릭터 몸통 회전 x 카메라만 회전시키려고 만들었숩니다.
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
        FirstPersonCameraComponent->SetRelativeLocation(FVector(-11.765135f, 11.176215f, 13.473909f)); // 눈높이 임의 설정
    FirstPersonCameraComponent->bUsePawnControlRotation = true;                 // 마우스로 카메라 상하좌우 회전

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

        if (WeaponClass) // 무기 클래스가 지정돼 있으면 스폰해서 바로 장착
        {
            EquippedWeapon = GetWorld()->SpawnActor<APrimaryWeapon>(WeaponClass);

            if (EquippedWeapon)
            {
                EquippedWeapon->Equip(this, FirstPersonCameraComponent, NAME_None); // 일단 팔 메시 없어서 소켓 없이 카메라 기준으로 바로 부착
                UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: Weapon [%s] successfully equipped."), *EquippedWeapon->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("PlayerCharacter: No WeaponClass specified in Blueprint defaults."));
        }
    
}
	

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateCameraCrouchInterp(DeltaTime); // 웅크리기 시 카메라 높이 연결

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


void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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

    }
}

void APlayerCharacter::TryInteract()
{
    if (NearbyInteractable && NearbyInteractable->Implements<UInteractableTarget>())
        IInteractableTarget::Execute_Interact(NearbyInteractable, this);
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
    if (bIsCrouched)
    {
        UnCrouch(); // 웅크린 상태에서 달리기를 누르면 일어남
    }
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopSprint()
{
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