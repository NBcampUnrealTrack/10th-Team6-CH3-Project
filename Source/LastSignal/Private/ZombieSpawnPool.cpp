#include "ZombieSpawnPool.h"
#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

AZombieSpawnPool::AZombieSpawnPool()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;

    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    TriggerBox->SetGenerateOverlapEvents(true);
}

void AZombieSpawnPool::BeginPlay()
{
    Super::BeginPlay();

    // 맵 이름 기반 데이터 테이블 설정 로드
    LoadMapSpecificData();

    // 비활성화된 맵이면 작동 중단
    if (!bIsActivePool)
    {
        return;
    }

    // 오버랩 이벤트
    if (TriggerBox)
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AZombieSpawnPool::OnOverlapBegin);
        TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AZombieSpawnPool::OnOverlapEnd);

        // 플레이어 스폰 지연을 고려한 초기 오버랩 검사
        FTimerHandle InitialCheckHandle;
        GetWorld()->GetTimerManager().SetTimer(
            InitialCheckHandle, [this]()
            {
                if (!IsValid(this) || !TriggerBox) return;

                TArray<AActor*> OverlappingActors;
                TriggerBox->GetOverlappingActors(OverlappingActors);

                for (AActor* Actor : OverlappingActors)
                {
                    APawn* PlayerPawn = nullptr;
                    if (IsPlayerActor(Actor, PlayerPawn))
                    {
                        StartSpawningProcess(PlayerPawn);
                        break;
                    }
                } },
            0.2f, false);
    }
}

void AZombieSpawnPool::LoadMapSpecificData()
{
    if (!GetWorld())
        return;

    FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(this, true).TrimStartAndEnd();

    const UDataTable *TargetTable = SpawnDataTable;
    if (!TargetTable && !SpawnDataRow.IsNull())
    {
        TargetTable = SpawnDataRow.DataTable;
    }

    if (!TargetTable)
    {
        bIsActivePool = false;
        return;
    }

    const FZombieSpawnPoolData *FoundData = nullptr;

    if (!SpawnDataRow.IsNull() && SpawnDataRow.RowName != NAME_None)
    {
        FoundData = TargetTable->FindRow<FZombieSpawnPoolData>(SpawnDataRow.RowName, TEXT("ZombieSpawnPoolContext"));
    }

    if (!FoundData)
    {
        TArray<FName> RowNames = TargetTable->GetRowNames();
        for (const FName &RowName : RowNames)
        {
            const FZombieSpawnPoolData *RowData = TargetTable->FindRow<FZombieSpawnPoolData>(RowName, TEXT("ZombieSpawnPoolContext"));
            if (!RowData)
                continue;

            FString RowNameStr = RowName.ToString().TrimStartAndEnd();
            FString TargetMapStr = RowData->TargetMapName.ToString().TrimStartAndEnd();

            if (RowNameStr.Equals(CurrentMapName, ESearchCase::IgnoreCase) ||
                TargetMapStr.Equals(CurrentMapName, ESearchCase::IgnoreCase))
            {
                FoundData = RowData;
                break;
            }
        }
    }

    if (FoundData)
    {
        TargetMapName = FoundData->TargetMapName;
        ZombieClass = FoundData->ZombieClass;
        InitialSpawnCount = FoundData->InitialSpawnCount;
        PeriodicSpawnCount = FoundData->PeriodicSpawnCount;
        SpawnInterval = FoundData->SpawnInterval;
        MaxZombieCount = FoundData->MaxZombieCount;

        bIsActivePool = true;
    }
    else
    {
        bIsActivePool = false;
    }
}

bool AZombieSpawnPool::IsPlayerActor(AActor *Actor, APawn *&OutPlayerPawn) const
{
    if (!Actor)
        return false;

    APawn *Pawn = Cast<APawn>(Actor);
    if (!Pawn)
        return false;

    if (Pawn->IsPlayerControlled() || Pawn->IsLocallyControlled())
    {
        OutPlayerPawn = Pawn;
        return true;
    }

    APawn *LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (LocalPlayerPawn && LocalPlayerPawn == Pawn)
    {
        OutPlayerPawn = Pawn;
        return true;
    }

    return false;
}

void AZombieSpawnPool::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (!bIsActivePool)
        return;

    APawn *PlayerPawn = nullptr;
    if (IsPlayerActor(OtherActor, PlayerPawn))
    {
        StartSpawningProcess(PlayerPawn);
    }
}

void AZombieSpawnPool::OnOverlapEnd(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (!bIsActivePool)
        return;

    APawn *PlayerPawn = nullptr;
    if (IsPlayerActor(OtherActor, PlayerPawn))
    {
        TArray<AActor *> OverlappingActors;
        TriggerBox->GetOverlappingActors(OverlappingActors);

        bool bAnyPlayerLeft = false;
        for (AActor *Actor : OverlappingActors)
        {
            APawn *DummyPawn = nullptr;
            if (IsPlayerActor(Actor, DummyPawn))
            {
                bAnyPlayerLeft = true;
                break;
            }
        }

        if (!bAnyPlayerLeft)
        {
            StopSpawningProcess();
        }
    }
}

void AZombieSpawnPool::StartSpawningProcess(APawn *PlayerPawn)
{
    if (bPlayerInside)
        return;
    bPlayerInside = true;

    // 최초 수량 스폰
    if (!bInitialSpawnDone)
    {
        SpawnZombieBatch(InitialSpawnCount);
        bInitialSpawnDone = true;
    }

    // 주기적으로 스폰 타이머 작동
    if (GetWorld() && SpawnInterval > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            SpawnTimerHandle,
            this,
            &AZombieSpawnPool::SpawnPeriodicZombies,
            SpawnInterval,
            true);
    }
}

void AZombieSpawnPool::StopSpawningProcess()
{
    bPlayerInside = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
    }
}

void AZombieSpawnPool::SpawnPeriodicZombies()
{
    SpawnZombieBatch(PeriodicSpawnCount);
}

void AZombieSpawnPool::SpawnZombieBatch(int32 Count)
{
    if (!ZombieClass || !GetWorld())
    {
        return;
    }

    CleanupDeadZombies();

    const int32 CurrentCount = SpawnedZombies.Num();
    const int32 AvailableSlots = MaxZombieCount - CurrentCount;

    if (AvailableSlots <= 0)
    {
        return;
    }

    const int32 ActualSpawnCount = FMath::Min(Count, AvailableSlots);

    for (int32 i = 0; i < ActualSpawnCount; ++i)
    {
        FVector SpawnLocation = GetRandomSpawnPoint();
        FRotator SpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor *NewZombie = GetWorld()->SpawnActor<AActor>(ZombieClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (NewZombie)
        {
            SpawnedZombies.Add(NewZombie);

            if (APawn *ZombiePawn = Cast<APawn>(NewZombie))
            {
                if (!ZombiePawn->GetController())
                {
                    ZombiePawn->SpawnDefaultController();
                }
            }
        }
    }
}

void AZombieSpawnPool::CleanupDeadZombies()
{
    SpawnedZombies.RemoveAll([](AActor *Zombie)
                             { return !IsValid(Zombie); });
}

FVector AZombieSpawnPool::GetRandomSpawnPoint() const
{
    if (TriggerBox)
    {
        FVector Center = TriggerBox->GetComponentLocation();
        FVector Extents = TriggerBox->GetScaledBoxExtent();
        return UKismetMathLibrary::RandomPointInBoundingBox(Center, Extents);
    }
    return GetActorLocation();
}

void AZombieSpawnPool::PrintDebugMessage(const FString &Message, FColor Color) const
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    if (bShowDebugLog && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, Color, Message);
    }
}