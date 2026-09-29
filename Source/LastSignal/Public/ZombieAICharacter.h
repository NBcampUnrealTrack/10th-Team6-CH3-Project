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
    float WalkSpeed = 200.0f;

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

    // 좀비 목소리: 평소엔 신음, 쫓아올 땐 추격 소리, 공격/피격/사망 소리 (거리 감쇠 + 동시에 우는 좀비 수 제한)
    float SecondsUntilNextGroan = 0.0f;
    float SecondsSinceLastPain = 10.0f;
    float VoicePitch = 0.0f; // 좀비마다 다른 목소리 높이 (처음 울 때 정함)

    UPROPERTY(Transient)
    TObjectPtr<class UAudioComponent> CurrentVoice; // 지금 내고 있는 소리 (평소/추격 소리는 이게 끝나야 다음 걸 냄)

    // bInterrupt: 공격/피격/사망처럼 바로 내야 하는 소리면 하던 소리를 끊고 냄
    void PlayVoice(class USoundBase *Sound, float Volume, bool bInterrupt);

    UPROPERTY(Transient)
    TObjectPtr<class USoundAttenuation> GroanAttenuation;

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

        // --- 사운드 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase *DeathSound;

  public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
};