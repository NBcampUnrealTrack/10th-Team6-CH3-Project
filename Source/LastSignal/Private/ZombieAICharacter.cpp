#include "ZombieAICharacter.h"
#include "ZombieAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

AZombieAICharacter::AZombieAICharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AZombieAIController::StaticClass();
    // 캐릭터를 기본적으로 조종할 AIController 클래스를 지정
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    // 레벨 배치 또는 Spawn시 AIController가 자동으로 이 캐릭터를 Possess하도록 설정
    UCharacterMovementComponent *Movement = GetCharacterMovement();
    
    if (ZombieType == EZombieType::HumanZombie)
    {
        Movement->MaxWalkSpeed = WalkSpeed * 1; 
    }

     if (ZombieType == EZombieType::MosterZombie)
    {
        Movement->MaxWalkSpeed = WalkSpeed * 2;
    }

    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->bOrientRotationToMovement = true; 
    //캐릭터가 이동하는 방향을 바라보도록 회전설정
    Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    //캐릭터가 이동방향으로 회전하는 속도를 설정
}

void AZombieAICharacter::SetMovementSpeed(float NewSpeed)
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = NewSpeed;
    }
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
