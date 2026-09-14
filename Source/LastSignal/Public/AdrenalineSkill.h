#pragma once

#include "CoreMinimal.h"
#include "SkillComponent.h"
#include "WeaponCombatComponent.h"
#include "AdrenalineSkill.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class APrimaryWeapon;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LASTSIGNAL_API UAdrenalineSkill : public USkillComponent
{
    GENERATED_BODY()

  public:
    UAdrenalineSkill();

    virtual void ActivateSkill() override;
    virtual void DeactivateSkill() override;

    // 무기 교체 시 캐릭터 클래스 등에서 호출
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnWeaponSwapped(APrimaryWeapon *NewWeapon);

  protected:
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem *AdrenalineVFX;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase *ActivationSound;

    UPROPERTY()
    UNiagaraComponent *ActiveNiagaraEffect;

    // 현재 적용 중인 무기의 원본 스탯 백업
    FWeaponStats OriginalWeaponStats;

    // 현재 버프가 적용되어 있는 무기 참조
    TWeakObjectPtr<APrimaryWeapon> CurrentBuffedWeapon;

  private:
    void ApplyBuffToWeapon(APrimaryWeapon *Weapon);
    void RestoreWeaponStats();
};