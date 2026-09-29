#pragma once

#include "CoreMinimal.h"
#include "WeaponImpactEffects.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class USoundBase;
class USoundAttenuation;

USTRUCT(BlueprintType)
struct LASTSIGNAL_API FWeaponImpactEffects
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact",
              meta = (DisplayName = "파티클 (캐스케이드)", ToolTip = "P_ 계열 파티클. 나이아가라가 지정되어 있으면 나이아가라를 우선 사용합니다."))
    TObjectPtr<UParticleSystem> ParticleEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact",
              meta = (DisplayName = "나이아가라", ToolTip = "NS_ 계열 효과. 파티클과 둘 다 지정하면 이 효과만 생성합니다."))
    TObjectPtr<UNiagaraSystem> NiagaraEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (DisplayName = "탄착 사운드"))
    TObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact",
              meta = (DisplayName = "사운드 감쇠", ToolTip = "거리별 소리 감쇠. 비어 있으면 사운드 에셋에 지정된 설정을 사용합니다."))
    TObjectPtr<USoundAttenuation> SoundAttenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (DisplayName = "효과 크기"))
    FVector Scale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact",
              meta = (DisplayName = "효과 회전 보정", ToolTip = "기본적으로 효과의 +Z축을 표면 바깥 방향으로 맞춥니다. 에셋 방향이 다르면 여기서 보정합니다."))
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact",
              meta = (DisplayName = "표면 간격", ClampMin = "0.0", Units = "cm"))
    float SurfaceOffset = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact|Smoke",
              meta = (DisplayName = "연기 파티클 (캐스케이드)", ToolTip = "기존 탄착 효과와 함께 명중 지점에 생성합니다. 연기 나이아가라가 있으면 나이아가라를 우선 사용합니다."))
    TObjectPtr<UParticleSystem> SmokeParticleEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact|Smoke",
              meta = (DisplayName = "연기 나이아가라", ToolTip = "기존 탄착 효과와 별도로 생성하는 연기입니다. 비워 두면 생성하지 않습니다."))
    TObjectPtr<UNiagaraSystem> SmokeNiagaraEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact|Smoke", meta = (DisplayName = "연기 크기"))
    FVector SmokeScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact|Smoke",
              meta = (DisplayName = "연기 방향을 표면에 맞춤", ToolTip = "켜면 연기의 +Z축을 표면 바깥으로 맞춥니다. 끄면 월드 위쪽을 기준으로 생성합니다. 이후 입자의 움직임은 연기 에셋 설정을 따릅니다."))
    bool bAlignSmokeToSurface = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact|Smoke", meta = (DisplayName = "연기 회전 보정"))
    FRotator SmokeRotationOffset = FRotator::ZeroRotator;

};
