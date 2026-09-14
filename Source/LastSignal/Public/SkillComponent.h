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

    // 초 마다 스킬게이지 자동으로 올라가며 (Charging 상태일때 동작한다)
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void OnZombieKilled(); // 킬 카운트가 올라갈때 호출한다 (Charging 상태일때 동작한다)

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool CanActivateSkill() const; // 플레이어가 스킬 키를 눌렀을 떄 발동조건이 (Ready 상태) 라면 동작하며 (Charging, Active 상태) 라면 동작하지 않는다.

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void ActivateSkill(); // 스킬 발동 키 입력 시 게이지를 0으로 초기화하고, 상태를 Active로 전환 되며 SkillDuration 타미머 작동

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void DeactivateSkill(); // SkillDuration 타이머 만료 시 자동 호출되는데 스킬을 종료 플레이어 사망, 무기 교체, 컷씬 재생 등 스킬을 도중에 강제 취소할 때 사용된다.

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