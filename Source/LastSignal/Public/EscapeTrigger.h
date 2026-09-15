#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeTrigger.generated.h"

class UBoxComponent;

UCLASS()

class LASTSIGNAL_API AEscapeTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	AEscapeTrigger();

protected:
	
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Escape") // 헬기존 감지용 충돌 박스, GoalTrigger/Radio랑 같은 용도
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(VisibleAnywhere, Category = "Escape") // 이미 한 번 발동했으면 다시 발동 안 되게 막는 플래그 (왔다갔다해도 3분 타이머 안 리셋되게)
    bool bEscapeStarted = false;

    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult &SweepResult);
};
