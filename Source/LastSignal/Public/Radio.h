#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTarget.h"
#include "Radio.generated.h"


class UBoxComponent;

UCLASS()
class LASTSIGNAL_API ARadio : public AActor, public IInteractableTarget
{
	GENERATED_BODY()
	
public:	
	
	ARadio();

	virtual void Interact_Implementation(AActor *Interactor) override;

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

    UPROPERTY(EditAnywhere, Category = "Radio")
    FText ObjectiveAfterActivation; // 무전기 사용 후 바뀔 목표 문구
};

