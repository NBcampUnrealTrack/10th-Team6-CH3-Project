#pragma once

#include "CoreMinimal.h"
#include "SkillComponent.h"
#include "AdrenalineSkill.generated.h"

class UNiagaraComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LASTSIGNAL_API UAdrenalineSkill : public USkillComponent
{
	GENERATED_BODY()

public:
	UAdrenalineSkill();

	virtual void ActivateSkill() override;
	virtual void DeactivateSkill() override;

protected:
	UPROPERTY()
	UNiagaraComponent* ActiveNiagaraEffect;

	float OriginalVerticalRecoil;
	float OriginalHorizonRecoil;
	float OriginalRecoilCameraShake;
	float OriginalWeight;
	int32 OriginalRPM;
	float OriginalAutoireSpread;
	float OriginalReloadTime;
	float OriginalTacReloadTime;
};
