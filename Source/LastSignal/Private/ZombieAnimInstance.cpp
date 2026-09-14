#include "ZombieAnimInstance.h"
#include "ZombieAICharacter.h"

void UZombieAnimInstance::AnimNotify_HitCheck()
{
    AZombieAICharacter *Zombie = Cast<AZombieAICharacter>(TryGetPawnOwner());
    if (Zombie)
    {
        Zombie->OnAttackHitCheck();
    }
}