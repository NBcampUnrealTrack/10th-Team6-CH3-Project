#include "ZombieAICharacter.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieAIController.h"

AZombieAICharacter::AZombieAICharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AZombieAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    UCharacterMovementComponent *Movement = GetCharacterMovement();
    if (Movement)
    {
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->bOrientRotationToMovement = true;
        Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    }
}

void AZombieAICharacter::BeginPlay()
{
    Super::BeginPlay();

    // 타입에 맞춰 이동 속도 초기화
    SetMovementSpeed(WalkSpeed);
}

void AZombieAICharacter::SetMovementSpeed(float NewSpeed)
{
    if (UCharacterMovementComponent *Movement = GetCharacterMovement())
    {
        float TargetSpeed = NewSpeed;

        if (ZombieType == EZombieType::HumanZombie)
        {
            TargetSpeed = NewSpeed;
        }
        else if (ZombieType == EZombieType::MonsterZombie)
        {
            TargetSpeed = NewSpeed * 2.0f;
        }

        Movement->MaxWalkSpeed = TargetSpeed;
    }
}

void AZombieAICharacter::Attack()
{
    if (bIsAttacking)
        return;

    UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && AttackMontage)
    {
        bIsAttacking = true;

        // 몽타주 재생
        AnimInstance->Montage_Play(AttackMontage);

        // 몽타주 종료 델리게이트 바인딩
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AZombieAICharacter::OnAttackMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);
    }
}

void AZombieAICharacter::OnAttackHitCheck()
{
    FHitResult HitResult;
    FCollisionQueryParams Params(NAME_None, false, this);

    FVector Start = GetActorLocation();
    FVector End = Start + (GetActorForwardVector() * AttackRange);

    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(AttackRadius),
        Params);

#if WITH_EDITOR
    FColor DrawColor = bHit ? FColor::Red : FColor::Green;
    DrawDebugSphere(GetWorld(), End, AttackRadius, 12, DrawColor, false, 1.0f);
#endif

    if (bHit && HitResult.GetActor())
    {
        // 타격 대상에게 TakeDamage 전달
        UGameplayStatics::ApplyDamage(
            HitResult.GetActor(),
            AttackDamage,
            GetController(),
            this,
            UDamageType::StaticClass());

        UE_LOG(LogTemp, Warning, TEXT("[Zombie] Hit Target: %s / Damage: %f"), *HitResult.GetActor()->GetName(), AttackDamage);
    }
}

void AZombieAICharacter::OnAttackMontageEnded(UAnimMontage *Montage, bool bInterrupted)
{
    bIsAttacking = false;
}

void AZombieAICharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AZombieAICharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}