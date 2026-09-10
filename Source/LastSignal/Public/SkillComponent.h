#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

UENUM(BlueprintType)
enum class ESkillState : uint8
{
    Charging,
    Ready,
    Active
};

// UI 연동용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillValueChangedDelegate, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillStateChangedDelegate, ESkillState, NewState);

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LASTSIGNAL_API USkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void OnZombieKilled(); //킬 카운트 임의로 적어둠

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool CanActivateSkill() const;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void ActivateSkill();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void DeactivateSkill();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillValueChangedDelegate OnSkillValueChanged; // UI 게이지 변경 확인 (현재값, 최대값)

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillStateChangedDelegate OnSkillStateChanged; // UI 스킬 상태 변경 확인 (Charging, Ready, Active)

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float CurrentSkillValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float MaxSkillValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float ChargeRatePerSecond;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float KillBonusValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float SkillDuration;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    ESkillState CurrentState;

    FTimerHandle SkillDurationTimerHandle;
};