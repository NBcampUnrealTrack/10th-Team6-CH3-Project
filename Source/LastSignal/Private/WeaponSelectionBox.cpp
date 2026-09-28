#include "WeaponSelectionBox.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PlayerCharacter.h"
#include "PrimaryWeapon.h"

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
        Player->NearbyInteractable = this;
        UE_LOG(LogTemp, Log, TEXT("[WeaponSelectionBox] 상자 범위 진입 -> F키 상호작용 가능"));
    }
}

void AWeaponSelectionBox::OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        if (Player->NearbyInteractable == this)
        {
            Player->NearbyInteractable = nullptr;
            UE_LOG(LogTemp, Log, TEXT("[WeaponSelectionBox] 상자 범위 이탈"));
        }
    }
}

void AWeaponSelectionBox::Interact_Implementation(AActor *Interactor)
{
    if (!Interactor)
        return;

    UE_LOG(LogTemp, Log, TEXT("[WeaponSelectionBox] F 키 상호작용 발생! 1번 주무기 지급"));
    SelectAndEquipWeapon(0, Interactor);
}

void AWeaponSelectionBox::SelectAndEquipWeapon(int32 WeaponIndex, AActor *Interactor)
{
    if (!Interactor || !AvailablePrimaryWeapons.IsValidIndex(WeaponIndex))
        return;

    TSubclassOf<APrimaryWeapon> SelectedWeaponClass = AvailablePrimaryWeapons[WeaponIndex];
    if (!SelectedWeaponClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WeaponSelectionBox] %d번 슬롯의 무기 클래스가 지정되지 않았습니다!"), WeaponIndex);
        return;
    }

    if (APlayerCharacter *Player = Cast<APlayerCharacter>(Interactor))
    {
        Player->EquipPrimaryWeapon(SelectedWeaponClass);
    }
}