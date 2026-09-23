#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTarget.h"

#include "AmmoSupplyBox.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;
class APlayerCharacter;

UCLASS(Blueprintable, BlueprintType)
class LASTSIGNAL_API AAmmoSupplyBox : public AActor, public IInteractableTarget
{
    GENERATED_BODY()

  public:
    AAmmoSupplyBox();

    virtual void Interact_Implementation(AActor *InteractingActor) override;

  protected:
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BoxMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Settings")
    bool bOneTimeUse = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Settings", meta = (EditCondition = "bOneTimeUse"))
    bool bDestroyOnUse = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|State")
    bool bIsAvailable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects")
    TObjectPtr<USoundBase> SupplySound;

    UFUNCTION(BlueprintNativeEvent, Category = "AmmoBox|Logic")
    bool CanInteractSupply(APlayerCharacter *Player);
    virtual bool CanInteractSupply_Implementation(APlayerCharacter *Player);

    UFUNCTION(BlueprintNativeEvent, Category = "AmmoBox|Logic")
    void OnSupplyGranted(APlayerCharacter *Player);
    virtual void OnSupplyGranted_Implementation(APlayerCharacter *Player);

    UFUNCTION(BlueprintImplementableEvent, Category = "AmmoBox|Effects")
    void PlaySupplyEffects(APlayerCharacter *Player);

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex);
};
