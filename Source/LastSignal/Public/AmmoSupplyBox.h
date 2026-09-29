#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTarget.h"

#include "AmmoSupplyBox.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;
class UAnimMontage;
class APlayerCharacter;

UCLASS(Blueprintable, BlueprintType)
class LASTSIGNAL_API AAmmoSupplyBox : public AActor, public IInteractableTarget
{
    GENERATED_BODY()

  public:
    AAmmoSupplyBox();

    virtual void Interact_Implementation(AActor *InteractingActor) override;
    virtual FText GetInteractPromptText_Implementation() const override;

  protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BoxMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Settings")
    float InteractionDuration = 1.5f; // 보급 시간 (철컥 소리 0.8초 기준, 5초는 너무 길어서 줄임)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Settings")
    int32 MaxCharges = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AmmoBox|State")
    int32 CurrentCharges = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Settings")
    bool bDestroyOnDepleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Channeling")
    TObjectPtr<USoundBase> ChannelingSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Channeling")
    TObjectPtr<UAnimMontage> ChannelingMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Success")
    TObjectPtr<USoundBase> SuccessSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Success")
    TObjectPtr<UAnimMontage> SuccessMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Depleted")
    TObjectPtr<USoundBase> DepletedSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox|Effects|Depleted")
    TObjectPtr<UAnimMontage> DepletedMontage;

    UFUNCTION(BlueprintImplementableEvent, Category = "AmmoBox|Events")
    void OnInteractionStarted(APlayerCharacter *Player);

    UFUNCTION(BlueprintImplementableEvent, Category = "AmmoBox|Events")
    void OnSupplySuccess(APlayerCharacter *Player);

    UFUNCTION(BlueprintImplementableEvent, Category = "AmmoBox|Events")
    void OnSupplyDepleted(APlayerCharacter *Player);

  private:
    FTimerHandle InteractionTimerHandle;

    UPROPERTY()
    TObjectPtr<APlayerCharacter> CachedPlayer;

    bool bIsInteracting = false;

    void CompleteInteraction();

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex);
};