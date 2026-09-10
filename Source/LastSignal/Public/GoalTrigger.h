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

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
	

};
