#include "AnimNotify_ZombieHitCheck.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieAICharacter.h" // 추가

void UAnimNotify_ZombieHitCheck::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || !MeshComp->GetOwner())
        return;

    AActor *OwnerActor = MeshComp->GetOwner();
    UWorld *World = OwnerActor->GetWorld();
    if (!World)
        return;

    FVector Start = OwnerActor->GetActorLocation();
    FVector End = Start + (OwnerActor->GetActorForwardVector() * AttackRange);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);

    bool bHit = World->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(AttackRadius),
        Params);

#if WITH_EDITOR
    FColor DrawColor = bHit ? FColor::Red : FColor::Green;
    DrawDebugSphere(World, End, AttackRadius, 12, DrawColor, false, 1.0f);
#endif

    if (bHit && HitResult.GetActor())
    {
        // 좀비끼리는 무시
        if (HitResult.GetActor()->IsA<AZombieAICharacter>())
        {
            return;
        }

        UGameplayStatics::ApplyDamage(
            HitResult.GetActor(),
            DamageAmount,
            OwnerActor->GetInstigatorController(),
            OwnerActor,
            UDamageType::StaticClass());
    }
}