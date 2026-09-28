#include "WeaponSelectionBox.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PlayerCharacter.h"
#include "PrimaryWeapon.h"
#include "WeaponSelectWidget.h"
#include "GameFramework/PlayerController.h"

AWeaponSelectionBox::AWeaponSelectionBox()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BoxMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMeshComponent"));
    BoxMeshComponent->SetupAttachment(SceneRoot);
    BoxMeshComponent->SetMobility(EComponentMobility::Movable); // ★ 크기/변형 조절 가능하도록 허용
    BoxMeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    TriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerComponent"));
    TriggerComponent->SetupAttachment(SceneRoot);
    TriggerComponent->SetMobility(EComponentMobility::Movable);
    TriggerComponent->SetBoxExtent(TriggerBoxExtent);
    TriggerComponent->SetCollisionProfileName(TEXT("Trigger"));

    TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponSelectionBox::OnOverlapBegin);
    TriggerComponent->OnComponentEndOverlap.AddDynamic(this, &AWeaponSelectionBox::OnOverlapEnd);

    AvailablePrimaryWeapons.Init(nullptr, 3);
}

void AWeaponSelectionBox::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);

    if (TriggerComponent)
    {
        TriggerComponent->SetBoxExtent(TriggerBoxExtent, true);
        TriggerComponent->MarkRenderStateDirty();
    }
}

void AWeaponSelectionBox::BeginPlay()
{
    Super::BeginPlay();

    if (TriggerComponent)
    {
        TriggerComponent->SetBoxExtent(TriggerBoxExtent, true);
    }
}

void AWeaponSelectionBox::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        Player->NearbyInteractable = this;;
    }
}

void AWeaponSelectionBox::OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        if (Player->NearbyInteractable == this)
        {
            Player->NearbyInteractable = nullptr;
        }
    }
}

void AWeaponSelectionBox::Interact_Implementation(AActor *Interactor)
{
    // F → 무기 선택 화면을 띄운다 (카드를 고르면 화면이 SelectAndEquipWeapon을 호출)
    const APawn *InteractorPawn = Cast<APawn>(Interactor);
    APlayerController *PlayerController = InteractorPawn ? Cast<APlayerController>(InteractorPawn->GetController()) : nullptr;
    if (!PlayerController)
        return;

    if (UWeaponSelectWidget *SelectWidget = CreateWidget<UWeaponSelectWidget>(PlayerController, UWeaponSelectWidget::StaticClass()))
    {
        SelectWidget->SelectionBox = this;
        SelectWidget->AddToViewport(10); // HUD보다 위에 표시
    }
}

void AWeaponSelectionBox::SelectAndEquipWeapon(int32 WeaponIndex, AActor *Interactor)
{
    if (!Interactor || !AvailablePrimaryWeapons.IsValidIndex(WeaponIndex))
        return;

    TSubclassOf<APrimaryWeapon> SelectedWeaponClass = AvailablePrimaryWeapons[WeaponIndex];
    if (!SelectedWeaponClass)
    {
        return;
    }

    if (APlayerCharacter *Player = Cast<APlayerCharacter>(Interactor))
    {
        Player->EquipPrimaryWeapon(SelectedWeaponClass);
    }
}