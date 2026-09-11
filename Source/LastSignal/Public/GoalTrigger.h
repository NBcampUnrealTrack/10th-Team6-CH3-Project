#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoalTrigger.generated.h"


class UBoxComponent;

UCLASS()
class LASTSIGNAL_API AGoalTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	AGoalTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Goal")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnyWhere, Category = "Goal") //  이 트리거를 밟았을 때 오픈되는 레벨 이름. 맵마다 에디터에서 다르게 지정 클리어 시, 레벨 넘기는 로직을 위한 매개 변수입니다, 
        FName NextLevelName;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
	

};
