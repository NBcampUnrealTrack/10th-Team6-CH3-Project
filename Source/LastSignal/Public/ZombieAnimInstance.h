#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "ZombieAnimInstance.generated.h"

UCLASS()
class LASTSIGNAL_API UZombieAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

  protected:

    // 애니메이션 몽타주 노티파이 이름이 "HitCheck"일 때 자동으로 호출됩니다.
    UFUNCTION()
    void AnimNotify_HitCheck();
};