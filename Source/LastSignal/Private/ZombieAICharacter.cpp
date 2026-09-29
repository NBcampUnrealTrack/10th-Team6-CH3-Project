#include "ZombieAICharacter.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LastSignalGameMode.h"
#include "ZombieAIController.h"
// UI 추가
#include "LastSignalPlayerController.h"
#include "LastSignalPlayerHUDComponent.h"

AZombieAICharacter::AZombieAICharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AZombieAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // 캡슐이 사격 트레이스(Visibility)를 막지 않게 해서 머리 히트박스/메시까지 도달하게 한다.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    // 몸통 피격은 메시가 받는다.
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    UCharacterMovementComponent *Movement = GetCharacterMovement();
    if (Movement)
    {
        Movement->MaxWalkSpeed = WalkSpeed;
        Movement->bOrientRotationToMovement = true;
        Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    }

    HeadHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHitbox"));
    HeadHitbox->SetupAttachment(GetMesh(), TEXT("head")); // 실제 소켓/본 이름으로 교체
    HeadHitbox->SetSphereRadius(12.f);
    HeadHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadHitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 트레이스가 ECC_Visibility라서 이거 필수
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

float AZombieAICharacter::TakeDamage(float DamageAmount, FDamageEvent const &DamageEvent, AController *EventInstigator, AActor *DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 이미 죽었거나 데미지가 0 이하인 경우 무시
    if (bIsDead || ActualDamage <= 0.0f)
        return 0.0f;

    CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.0f, MaxHP);

    UE_LOG(LogTemp, Warning, TEXT("[Zombie] Took Damage: %f / Remaining HP: %f (%.1f%%)"),
           ActualDamage, CurrentHP, GetHPRatio() * 100.0f);

    // 체력이 50% 이하이고, 아직 살아있는 경우 이동 속도를 절반으로 감속
    if (GetHPRatio() <= 0.5f && GetHPRatio() > 0.0f)
    {
        SetMovementSpeed(WalkSpeed * 0.5f);
    }

    // 체력이 0 이하가 되면 사망 처리
    if (CurrentHP <= 0.0f)
    {
        bIsDead = true;
        bIsAttacking = false;

        // 죽는 소리 재생 (액터가 정리돼도 소리는 끝까지 재생됨)
        if (DeathSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
        }

        // 진행 중인 공격 몽타주를 즉시 중단 (블렌드 아웃 0초)
        if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->StopAllMontages(0.0f);
        }

        if (ALastSignalGameMode *GameMode = Cast<ALastSignalGameMode>(UGameplayStatics::GetGameMode(this)))
            GameMode->OnZombieKilled(); // 킬카운트 증가 (게임모드로 보냄)

        // UI 추가
        // 플레이어가 이 좀비를 처치한 경우 HUD에 킬 전달
        if (EventInstigator)
        {
            if (ALastSignalPlayerController *PlayerController =
                    Cast<ALastSignalPlayerController>(EventInstigator))
            {
                if (ULastSignalPlayerHUDComponent *HUDComponent =
                        PlayerController->GetHUDComponent())
                {
                    HUDComponent->RegisterKill(
                        FText::FromString(TEXT("Zombie")));
                }
            }
        }
        // 여기까지

        UE_LOG(LogTemp, Error, TEXT("[Zombie] Dead!"));

        // 1. AI 동작 중단 및 빙의 해제
        if (AAIController *AICon = Cast<AAIController>(GetController()))
        {
            if (UBrainComponent *Brain = AICon->GetBrainComponent())
            {
                Brain->StopLogic(TEXT("Dead")); // 진행 중인 BT 태스크 중단
            }
            AICon->StopMovement();
            AICon->UnPossess();
        }

        // 2. 플레이어/다른 AI와의 충돌 제거 (시체 통과 가능 처리)
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HeadHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 시체 머리가 총알을 막는 문제 방지

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