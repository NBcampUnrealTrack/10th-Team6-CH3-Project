#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableTarget.h"
#include "WeaponSelectionBox.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USceneComponent;
class APrimaryWeapon;

UCLASS()
class LASTSIGNAL_API AWeaponSelectionBox : public AActor, public IInteractableTarget
{
    GENERATED_BODY()

  public:
    AWeaponSelectionBox();

    virtual void OnConstruction(const FTransform &Transform) override;

    virtual void Interact_Implementation(AActor *Interactor) override;
    virtual FText GetInteractPromptText_Implementation() const override { return FText::FromString(TEXT("무기 선택")); }

    // 주무기 지급 및 장착 함수
    UFUNCTION(BlueprintCallable, Category = "Weapon Selection")
    void SelectAndEquipWeapon(int32 WeaponIndex, AActor *Interactor);

    UFUNCTION(BlueprintPure, Category = "Weapon Selection")
    TArray<TSubclassOf<APrimaryWeapon>> GetAvailablePrimaryWeapons() const { return AvailablePrimaryWeapons; }

  protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BoxMeshComponent;

    // F키 상호작용 감지 범위
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerComponent;

    // 트리거 크기 조절
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box Settings")
    FVector TriggerBoxExtent = FVector(120.f, 120.f, 80.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Selection")
    TArray<TSubclassOf<APrimaryWeapon>> AvailablePrimaryWeapons;
};