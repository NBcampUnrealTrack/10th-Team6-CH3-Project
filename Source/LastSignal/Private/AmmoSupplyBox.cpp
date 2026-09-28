#include "AmmoSupplyBox.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "TimerManager.h"

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
    CurrentCharges = MaxCharges;

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AAmmoSupplyBox::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AAmmoSupplyBox::OnOverlapEnd);
}

void AAmmoSupplyBox::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
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
    if (bIsInteracting)
        return;

    APlayerCharacter *Player = Cast<APlayerCharacter>(InteractingActor);
    if (!Player)
        return;

    if (CurrentCharges <= 0)
    {
        if (DepletedSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, DepletedSound, GetActorLocation());
        }
        if (DepletedMontage)
        {
            Player->PlayAnimMontage(DepletedMontage);
        }

        OnSupplyDepleted(Player);
        return;
    }

    CachedPlayer = Player;
    bIsInteracting = true;

    Player->SetPlayerControlLocked(true);

    if (ChannelingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ChannelingSound, GetActorLocation());
    }
    if (ChannelingMontage)
    {
        Player->PlayAnimMontage(ChannelingMontage);
    }

    OnInteractionStarted(Player);

    GetWorldTimerManager().SetTimer(
        InteractionTimerHandle,
        this,
        &AAmmoSupplyBox::CompleteInteraction,
        InteractionDuration,
        false);
}

void AAmmoSupplyBox::CompleteInteraction()
{
    if (!CachedPlayer)
    {
        bIsInteracting = false;
        return;
    }

    CachedPlayer->SetPlayerControlLocked(false);

    if (CurrentCharges > 0)
    {
        CurrentCharges--;

        CachedPlayer->RefillAllAmmo();

        if (SuccessSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, SuccessSound, GetActorLocation());
        }
        if (SuccessMontage)
        {
            CachedPlayer->PlayAnimMontage(SuccessMontage);
        }

        OnSupplySuccess(CachedPlayer);

        if (CurrentCharges <= 0 && bDestroyOnDepleted)
        {
            if (CachedPlayer->NearbyInteractable == this)
            {
                CachedPlayer->NearbyInteractable = nullptr;
            }
            Destroy();
        }
    }

    bIsInteracting = false;
    CachedPlayer = nullptr;
}