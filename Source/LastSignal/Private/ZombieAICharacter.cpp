#include "ZombieAICharacter.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
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

    // 시작 시 체력 초기화
    CurrentHP = MaxHP;

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

// 외부(플레이어 사격 등)에서 ApplyDamage 호출 시 자동 실행
float AZombieAICharacter::TakeDamage(float DamageAmount, FDamageEvent const &DamageEvent, AController *EventInstigator, AActor *DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 이미 죽었거나 데미지가 0 이하인 경우 무시
    if (bIsDead || ActualDamage <= 0.0f)
        return 0.0f;

    CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);

    UE_LOG(LogTemp, Warning, TEXT("[Zombie] Took Damage: %f / Remaining HP: %f (%.1f%%)"),
           ActualDamage, CurrentHP, GetHPRatio() * 100.0f);

    //  [추가] 체력이 50% 이하이고, 아직 살아있는 경우 이동 속도를 절반으로 감속
    if (GetHPRatio() <= 0.5f && GetHPRatio() > 0.0f)
    {
        SetMovementSpeed(WalkSpeed * 0.5f);
    }

    // 체력이 0 이하가 되면 사망 처리
    if (CurrentHP <= 0.0f)
    {
        bIsDead = true;

        UE_LOG(LogTemp, Error, TEXT("[Zombie] Dead!"));

        // 1. AI 동작 중단 및 빙의 해제
        if (AAIController *AICon = Cast<AAIController>(GetController()))
        {
            AICon->StopMovement();
            AICon->UnPossess();
        }

        // 2. 플레이어/다른 AI와의 충돌 제거 (시체 통과 가능 처리)
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // 3. 이동 컴포넌트 비활성화
        if (UCharacterMovementComponent *Movement = GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            Movement->DisableMovement();
        }
    }

    return ActualDamage;
}

void AZombieAICharacter::Attack()
{
    // 사망 상태에서는 공격 불가
    if (bIsDead)
        return;

    UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && AttackMontage)
    {
        if (!AnimInstance->Montage_IsPlaying(AttackMontage))
        {
            AnimInstance->Montage_Play(AttackMontage);
        }
    }
}

void AZombieAICharacter::OnAttackHitCheck()
{
    if (bIsDead)
        return;

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