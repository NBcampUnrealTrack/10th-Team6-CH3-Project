#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Radio.generated.h"


class UBoxComponent;

UCLASS()
class LASTSIGNAL_API ARadio : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ARadio();

	void Interact();  // 플레이어가 상호작용하면 캐릭쪽에서 호출

protected:
	
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Radio")
    TObjectPtr<UBoxComponent> TriggerBox;

    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,             // 오버랩 했을때만 상호작용 가능하게 만듬
                         UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                         const FHitResult &SweepResult);

    UFUNCTION()
    void OnBoxEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                       UPrimitiveComponent *OtherComp, int32 OtherBodyIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Radio")
    void OnRadioActivated(); // 상호작용 성공 시점, 연출은 나중에 Blueprint에서 구현
};

