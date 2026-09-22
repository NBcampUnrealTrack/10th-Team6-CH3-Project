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

// UI 관련 헤더
#include "LastSignalPlayerController.h"      
#include "LastSignalPlayerHUDComponent.h" 

AZombieSpawnPool::AZombieSpawnPool()
{
    PrimaryActorTick.bCanEverTick = false;

    // 공통 기준점 루트 컴포넌트 생성
    DefaultRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRootComponent"));
    SetRootComponent(DefaultRootComponent);

    // 트리거 박스 (오직 플레이어 진입/퇴장 오버랩 감지용)
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    TriggerBox->SetGenerateOverlapEvents(true);

    // 스폰 박스 (오직 좀비 스폰 위치 영역 계산용 - 콜리전 차단)
    SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
    SpawnBox->SetupAttachment(RootComponent);
    SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnBox->SetGenerateOverlapEvents(false);
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

    // 트리거 박스 오버랩 이벤트 바인딩
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
        // UI 추가
        MissionObjectiveText = FoundData->MissionObjectiveText;

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

    if (Pawn->IsPlayerControlled())
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
        if (!IsPlayerInTrigger())
        {
            StopSpawningProcess();
        }
    }
}

void AZombieSpawnPool::StartSpawningProcess(APawn *PlayerPawn)
{
    if (!IsPlayerInTrigger())
    {
        StopSpawningProcess();
        return;
    }

    if (bPlayerInside)
        return;

    bPlayerInside = true;
    UpdateMissionObjective(); // ← UI 추가


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

bool AZombieSpawnPool::IsPlayerInTrigger() const
{
    if (!TriggerBox)
        return false;

    TArray<AActor *> OverlappingActors;
    TriggerBox->GetOverlappingActors(OverlappingActors);

    for (AActor *Actor : OverlappingActors)
    {
        APawn *OutPawn = nullptr;
        if (IsPlayerActor(Actor, OutPawn))
        {
            return true;
        }
    }

    return false;
}

void AZombieSpawnPool::SpawnPeriodicZombies()
{
    if (!bPlayerInside || !IsPlayerInTrigger())
    {
        StopSpawningProcess();
        return;
    }

    SpawnZombieBatch(PeriodicSpawnCount);
}

void AZombieSpawnPool::SpawnZombieBatch(int32 Count)
{
    if (!IsPlayerInTrigger())
    {
        StopSpawningProcess();
        return;
    }

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
        if (!IsPlayerInTrigger())
        {
            StopSpawningProcess();
            break;
        }

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
    if (!SpawnBox || !GetWorld())
    {
        return GetActorLocation();
    }

    const FVector Center = SpawnBox->GetComponentLocation();
    const FVector Extents = SpawnBox->GetScaledBoxExtent();

    FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(Center, Extents);

    FVector TraceStart = FVector(RandomPoint.X, RandomPoint.Y, Center.Z + Extents.Z);
    FVector TraceEnd = FVector(RandomPoint.X, RandomPoint.Y, Center.Z - Extents.Z - 500.0f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        const float CharacterHalfHeightOffset = 95.0f;
        return HitResult.Location + FVector(0.0f, 0.0f, CharacterHalfHeightOffset);
    }

    return RandomPoint;
}

void AZombieSpawnPool::PrintDebugMessage(const FString &Message, FColor Color) const
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    if (bShowDebugLog && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, Color, Message);
    }
}

// UI 관련 함수
void AZombieSpawnPool::UpdateMissionObjective() const
{
    UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] UpdateMissionObjective called. Text=%s"), *MissionObjectiveText.ToString());

    if (MissionObjectiveText.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] MissionObjectiveText is EMPTY!"));
        return;
    }

    APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] PlayerController is null!"));
        return;
    }

    ALastSignalPlayerController *LastSignalPC = Cast<ALastSignalPlayerController>(PC);
    if (!LastSignalPC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] Cast to LastSignalPlayerController FAILED!"));
        return;
    }

    ULastSignalPlayerHUDComponent *HUD = LastSignalPC->GetHUDComponent();
    if (!HUD)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] HUDComponent is null!"));
        return;
    }

    HUD->SetMissionObjective(MissionObjectiveText);
    UE_LOG(LogTemp, Warning, TEXT("[ZombieSpawnPool] SetMissionObjective called successfully!"));
}

// {
//    if (MissionObjectiveText.IsEmpty())
//    {
//        return;
//    }
//
//    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
//    {
//        if (ALastSignalPlayerController *LastSignalPC = Cast<ALastSignalPlayerController>(PC))
//        {
//            if (ULastSignalPlayerHUDComponent *HUD = LastSignalPC->GetHUDComponent())
//            {
//                HUD->SetMissionObjective(MissionObjectiveText);
//            }
//        }
//    }
//}