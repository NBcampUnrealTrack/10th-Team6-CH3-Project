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

    // BTTask에서 호출할 공격 시작 함수
    void Attack();

    // Anim Notify에서 호출할 실제 타격/데미지 처리 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnAttackHitCheck();

    // 몽타주 종료 시 호출될 델리게이트 콜백 함수
    void OnAttackMontageEnded(class UAnimMontage *Montage, bool bInterrupted);

    // 공격 중 여부 Getter
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool GetIsAttacking() const { return bIsAttacking; }

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

    // 전투 설정 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    class UAnimMontage *AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 20.0f;

    // 공격 중 상태 플래그
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bIsAttacking = false;

  public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
};