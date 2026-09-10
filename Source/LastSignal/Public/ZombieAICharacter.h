#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieAICharacter.generated.h"

UENUM(BlueprintType)
enum class EZombieType : uint8
{
    HumanZombie UMETA(DisplayName = "HumanZombie"),
    MosterZombie UMETA(DisplayName = "MonsterZombie")
};

UCLASS()
class LASTSIGNAL_API AZombieAICharacter : public ACharacter
{
    GENERATED_BODY()

  public:
    AZombieAICharacter();
    void SetMovementSpeed(float NewSpeed);

    UPROPERTY(EditAnywhere, Category = "AI")
    float WalkSpeed = 300.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float RunSpeed = 600.0f;

    UPROPERTY(EditAnywhere)
    EZombieType ZombieType;

    UPROPERTY(EditAnywhere)
    TMap<EZombieType, USkeletalMesh *> ZombieMeshes;

  protected:
    virtual void BeginPlay() override;

  public:
    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
};
