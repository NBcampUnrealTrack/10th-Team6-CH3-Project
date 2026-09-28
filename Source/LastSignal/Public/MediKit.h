#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTarget.h"
#include "MediKit.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;
class APlayerCharacter;

UCLASS()
class LASTSIGNAL_API AMediKit : public AActor, public IInteractableTarget
{
    GENERATED_BODY()

public:
    AMediKit();

    virtual void Interact_Implementation(AActor* InteractingActor) override;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BoxMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats", meta = (ClampMin = "1", UIMin = "1"))
    int32 MaxCharges = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats", meta = (ClampMin = "0", UIMin = "0"))
    int32 CurrentCharges = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float HealAmount = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats")
    bool bFullHeal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float InteractionDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Stats")
    bool bDestroyOnDepleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Sound")
    TObjectPtr<USoundBase> InteractSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Sound")
    TObjectPtr<USoundBase> UseSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Sound")
    TObjectPtr<USoundBase> DepletedSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Sound")
    TObjectPtr<USoundBase> FullHealthSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MediKit|Sound")
    bool bPlaySound2D = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "MediKit|Events")
    void OnInteractionStarted(APlayerCharacter* Player);

    UFUNCTION(BlueprintImplementableEvent, Category = "MediKit|Events")
    void OnHealSuccess(APlayerCharacter* Player);

    UFUNCTION(BlueprintImplementableEvent, Category = "MediKit|Events")
    void OnHealDepleted(APlayerCharacter* Player);

private:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void CompleteInteraction();
    void PlayMediKitSound(USoundBase* SoundToPlay);

    FTimerHandle InteractionTimerHandle;
    bool bIsInteracting = false;

    UPROPERTY()
    TObjectPtr<APlayerCharacter> CachedPlayer;
};