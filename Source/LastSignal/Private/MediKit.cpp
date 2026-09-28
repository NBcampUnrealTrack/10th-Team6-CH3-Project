#include "MediKit.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "TimerManager.h"

AMediKit::AMediKit()
{
    PrimaryActorTick.bCanEverTick = false;

    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    SetRootComponent(BoxMesh);

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(BoxMesh);
    TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

    bDestroyOnDepleted = false;
    bPlaySound2D = true;
}

void AMediKit::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);

    CurrentCharges = FMath::Clamp(CurrentCharges, 0, MaxCharges);
}

void AMediKit::BeginPlay()
{
    Super::BeginPlay();
    CurrentCharges = FMath::Clamp(CurrentCharges, 0, MaxCharges);

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMediKit::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AMediKit::OnOverlapEnd);
}

void AMediKit::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        Player->NearbyInteractable = this;
    }
}

void AMediKit::OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (APlayerCharacter *Player = Cast<APlayerCharacter>(OtherActor))
    {
        if (Player->NearbyInteractable == this)
        {
            Player->NearbyInteractable = nullptr;
        }
    }
}

void AMediKit::PlayMediKitSound(USoundBase *SoundToPlay)
{
    if (!SoundToPlay)
    {
        return;
    }

    if (bPlaySound2D)
    {
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }
    else
    {
        UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
    }
}

void AMediKit::Interact_Implementation(AActor *InteractingActor)
{
    if (bIsInteracting)
        return;

    APlayerCharacter *Player = Cast<APlayerCharacter>(InteractingActor);
    if (!Player)
        return;

    if (CurrentCharges <= 0)
    {
        PlayMediKitSound(DepletedSound);
        OnHealDepleted(Player);
        return;
    }

    if (Player->GetCurrentHealth() >= Player->GetMaxHealth())
    {
        PlayMediKitSound(FullHealthSound ? FullHealthSound : InteractSound);
        return;
    }

    CachedPlayer = Player;
    bIsInteracting = true;

    Player->SetPlayerControlLocked(true);
    PlayMediKitSound(InteractSound);
    OnInteractionStarted(Player);

    GetWorldTimerManager().SetTimer(
        InteractionTimerHandle,
        this,
        &AMediKit::CompleteInteraction,
        InteractionDuration,
        false);
}

void AMediKit::CompleteInteraction()
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

        if (bFullHeal)
        {
            CachedPlayer->RestoreFullHealth();
        }
        else
        {
            CachedPlayer->Heal(HealAmount);
        }

        PlayMediKitSound(UseSound);
        OnHealSuccess(CachedPlayer);

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