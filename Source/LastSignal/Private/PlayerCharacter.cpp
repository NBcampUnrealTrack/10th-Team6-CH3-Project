#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "Engine/EngineTypes.h"
#include "PrimaryWeapon.h"
#include "Engine/World.h"


APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;  // 1인칭이라 캐릭터 몸통 회전 x 카메라만 회전시키려고 만들었숩니다.
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // 눈높이 임의 설정
    FirstPersonCameraComponent->bUsePawnControlRotation = true;                 // 마우스로 카메라 상하좌우 회전


}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController *PlayerController = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }

    if (WeaponClass) // 무기 클래스가 지정돼 있으면 스폰해서 바로 장착
        {
            EquippedWeapon = GetWorld()->SpawnActor<APrimaryWeapon>(WeaponClass);

            if (EquippedWeapon)
            {
                EquippedWeapon->Equip(this, FirstPersonCameraComponent, NAME_None); // 일단 팔 메시 없어서 소켓 없이 카메라 기준으로 바로 부착
            }
        }
}
	

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

void APlayerCharacter::Look(const FInputActionValue &Value) // Look 함수
{
    const FVector2D LookInput = Value.Get<FVector2D>();

    if (Controller)
    {
        AddControllerYawInput(LookInput.X);
        AddControllerPitchInput(LookInput.Y);
    }
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const &DamageEvent, AController *EventInstigator, AActor *DamageCauser) // 데미지 받을떄 자동 호출 함수 구현
{
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser); // 부모(ACharacter)가 데미지 계산을 먼저 해주고, 실제 적용된 데미지량을 돌려줌 -> 기본 로직에 데미지량이 바뀔 수 있어서 최종 데미지를 부모가 계산하고 그 값을 우리가 이용함

    if (ActualDamage <= 0.0f || CurrentHealth <= 0.0f) // 데미지가 0이거나, 이미 죽어있으면 더 처리할 필요 없음
    {
        return ActualDamage;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth); // 체력 깎기. Clamp로 0~MaxHealth 범위를 벗어나지 않게 고정

    if (CurrentHealth <= 0.0f) // 체력 0 이하가 되면 죽음 처리
    {
        Die();
    }

    return ActualDamage;
}

void APlayerCharacter::Die() // 죽음 처리 (지금은 테스트 위해서 최소한만 구현했어요 — 나중에 애니메이션/입력 차단 같은거 만들 예정)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PLAYER DIED"));
    }

    
    OnDied.Broadcast(); // 구독해둔 다른 클래스들(나중에 GameMode 등)에게 "죽었다"고 방송 (델리게이트라서 있는거에요)
}