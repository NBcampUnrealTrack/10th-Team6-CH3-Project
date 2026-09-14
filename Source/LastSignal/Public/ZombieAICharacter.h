#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieAICharacter.generated.h"

UENUM(BlueprintType)
enum class EZombieType : uint8
{
    HumanZombie UMETA(DisplayName = "HumanZombie"),
    MonsterZombie UMETA(DisplayName = "MonsterZombie"),
};

UCLASS()
class LASTSIGNAL_API AZombieAICharacter : public ACharacter
{
    GENERATED_BODY()

  public:
    AZombieAICharacter();
    void SetMovementSpeed(float NewSpeed);

    // -----------------------------------------------------------
    // [공격 관련 함수 추가]
    // -----------------------------------------------------------
    // 1. BTTask나 외부에서 호출할 공격 실행 함수 (몽타주 재생)
    void Attack();

    // 2. Anim Notify에서 호출할 실제 데미지 판정 함수
    void OnAttackHitCheck();

    // 현재 공격 중인지 확인하는 getter
    bool IsAttacking() const { return bIsAttacking; }

    UPROPERTY(EditAnywhere, Category = "AI")
    float WalkSpeed = 300.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float RunSpeed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    EZombieType ZombieType;

    UPROPERTY(EditAnywhere, Category = "AI")
    TMap<EZombieType, USkeletalMesh *> ZombieMeshes;

  protected:
    virtual void BeginPlay() override;

    // 공격 몽타주가 끝났을 때 bIsAttacking을 해제하기 위한 콜백
    UFUNCTION()
    void OnAttackMontageEnded(UAnimMontage *Montage, bool bInterrupted);

    // -----------------------------------------------------------
    // [공격 관련 변수 추가]
    // -----------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    UAnimMontage *AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackRange = 150.0f; // 판정 사거리

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackRadius = 50.0f; // 판정 구체 반지름

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    float AttackDamage = 20.0f; // 데미지 양

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bIsAttacking = false;

  public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
};