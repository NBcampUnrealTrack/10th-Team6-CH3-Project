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

    CachedPlayer = Player;
    bIsInteracting = true;

    // 플레이어 이동 및 사격 잠금
    //Player->SetPlayerControlLocked(true);

    // 2. 상호작용 시작 시 사운드 및 몽타주 재생
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

    // 플레이어 조작 잠금 해제
    //CachedPlayer->SetPlayerControlLocked(false);

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
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("탄약을 모두 사용했습니다."));
        }

        if (DepletedSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, DepletedSound, GetActorLocation());
        }
        if (DepletedMontage)
        {
            CachedPlayer->PlayAnimMontage(DepletedMontage);
        }

        OnSupplyDepleted(CachedPlayer);
    }

    bIsInteracting = false;
    CachedPlayer = nullptr;
}