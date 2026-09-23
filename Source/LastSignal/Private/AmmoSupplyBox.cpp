#include "AmmoSupplyBox.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"

AAmmoSupplyBox::AAmmoSupplyBox()
{
    PrimaryActorTick.bCanEverTick = false;

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    SetRootComponent(BoxMesh);

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(BoxMesh);
    TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AAmmoSupplyBox::BeginPlay()
{
    Super::BeginPlay();

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AAmmoSupplyBox::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AAmmoSupplyBox::OnOverlapEnd);
}

void AAmmoSupplyBox::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (!bIsAvailable)
        return;

    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        Player->NearbyInteractable = this;
    }
}

void AAmmoSupplyBox::OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        if (Player->NearbyInteractable == this)
        {
            Player->NearbyInteractable = nullptr;
        }
    }
}

void AAmmoSupplyBox::Interact_Implementation(AActor *InteractingActor)
{
    if (!bIsAvailable)
        return;

    APlayerCharacter *Player = Cast<APlayerCharacter>(InteractingActor);
    if (!Player)
        return;

    if (!CanInteractSupply(Player))
        return;

    OnSupplyGranted(Player);

    if (SupplySound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SupplySound, GetActorLocation());
    }
    PlaySupplyEffects(Player);

    if (bOneTimeUse)
    {
        bIsAvailable = false;
        if (Player->NearbyInteractable == this)
        {
            Player->NearbyInteractable = nullptr;
        }

        if (bDestroyOnUse)
        {
            Destroy();
        }
    }
}

bool AAmmoSupplyBox::CanInteractSupply_Implementation(APlayerCharacter *Player)
{
    return true;
}

void AAmmoSupplyBox::OnSupplyGranted_Implementation(APlayerCharacter *Player)
{
    if (Player)
    {
        Player->RefillAllAmmo();
    }
}