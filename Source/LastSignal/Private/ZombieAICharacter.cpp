#include "ZombieAICharacter.h" // 헤더 파일 최상단 포함
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

void AZombieAICharacter::SetMovementSpeed(float NewSpeed)
{
    if (UCharacterMovementComponent *Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = NewSpeed;
        if (ZombieType == EZombieType::HumanZombie)
        {
            Movement->MaxWalkSpeed = WalkSpeed * 1;
        }
        else if (ZombieType == EZombieType::MonsterZombie)
        {
            Movement->MaxWalkSpeed = WalkSpeed * 2;
        }
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
        APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (HitResult.GetActor() == PlayerPawn)
        {
            UGameplayStatics::ApplyDamage(
                HitResult.GetActor(),
                AttackDamage,
                GetController(),
                this,
                UDamageType::StaticClass());

            UE_LOG(LogTemp, Warning, TEXT("[Zombie] Hit Player! Damage: %f"), AttackDamage);
        }
    }
}

void AZombieAICharacter::OnAttackMontageEnded(UAnimMontage *Montage, bool bInterrupted)
{
    bIsAttacking = false;
}

void AZombieAICharacter::BeginPlay()
{
    Super::BeginPlay();
    SetMovementSpeed(100);
}

void AZombieAICharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AZombieAICharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}