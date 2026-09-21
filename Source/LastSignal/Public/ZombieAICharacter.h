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

    // --- 체력 및 사망 관련 Getter ---
    UFUNCTION(BlueprintCallable, Category = "Stat")
    float GetMaxHP() const { return MaxHP; }

    UFUNCTION(BlueprintCallable, Category = "Stat")
    float GetCurrentHP() const { return CurrentHP; }

    // AnimBP State Machine 트랜지션용 (0.0f ~ 1.0f)
    UFUNCTION(BlueprintCallable, Category = "Stat")
    float GetHPRatio() const { return (MaxHP > 0.0f) ? (CurrentHP / MaxHP) : 0.0f; }

    UFUNCTION(BlueprintCallable, Category = "Stat")
    bool GetIsDead() const { return bIsDead; }

    UPROPERTY(EditAnywhere, Category = "AI")
    float WalkSpeed = 300.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float RunSpeed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    EZombieType ZombieType;

    UPROPERTY(EditAnywhere, Category = "AI")
    TMap<EZombieType, USkeletalMesh *> ZombieMeshes;

    // 헤드샷
    UPROPERTY(VisibleAnywhere, Category = "Combat")
    class USphereComponent *HeadHitbox;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float HeadshotMultiplier = 2.0f;

    FORCEINLINE USphereComponent *GetHeadHitbox() const { return HeadHitbox; }

  protected:
    virtual void BeginPlay() override;

    // 언리얼 데미지 수신 오버라이드 함수
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const &DamageEvent, class AController *EventInstigator, AActor *DamageCauser) override;

    // --- 체력 및 스탯 변수 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    float MaxHP = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
    float CurrentHP = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
    bool bIsDead = false;

    // --- 전투 설정 변수 ---
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