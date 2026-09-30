#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ZombieAICharacter.h"
#include "ZombieAIController.h"
#include "ZombieSpawnPool.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
struct FZombieTestWorld
{
    UWorld *World;
    FZombieTestWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
            .CreateAISystem(true).ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
                                   ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
        World->SetGameMode(FURL());
        World->InitializeActorsForPlay(FURL());
        World->GetWorldSettings()->NotifyBeginPlay();
        World->BeginPlay();
    }
    ~FZombieTestWorld()
    {
        World->EndPlay(EEndPlayReason::Quit);

        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
    void Advance(float Seconds)
    {
        // TimerManager intentionally runs once per engine frame, including in isolated test worlds.
        for (int32 i = 0; i < FMath::CeilToInt(Seconds / 0.1f); ++i)
        {
            ++GFrameCounter;
            World->GetTimerManager().Tick(0.1f);
        }
    }
};

int32 CountZombies(UWorld *World, bool bLivingOnly)
{
    int32 Count = 0;
    for (TActorIterator<AZombieAICharacter> It(World); It; ++It)
        if (IsValid(*It) && (!bLivingOnly || !It->GetIsDead()))
            ++Count;
    return Count;
}
int32 CountZombieControllers(UWorld *World)
{
    int32 Count = 0;
    for (TActorIterator<AZombieAIController> It(World); It; ++It)
        if (IsValid(*It))
            ++Count;
    return Count;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZombieDeathCleanupTest, "LastSignal.Zombies.DeathCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FZombieDeathCleanupTest::RunTest(const FString &Parameters)
{
    FZombieTestWorld TestWorld;
    for (int32 i = 0; i < 32; ++i)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AZombieAICharacter *Zombie = TestWorld.World->SpawnActor<AZombieAICharacter>(
            FVector(i * 200.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Params);
        if (!TestNotNull(TEXT("Zombie spawned"), Zombie)) return false;
        TestNotNull(TEXT("Living zombie has controller"), Zombie->GetController());
        UGameplayStatics::ApplyDamage(Zombie, Zombie->GetMaxHP() + 1.0f, nullptr, nullptr, nullptr);
        TestTrue(TEXT("Dead state recorded"), Zombie->GetIsDead());
        TestNull(TEXT("Dead zombie detached from controller"), Zombie->GetController());
        TestFalse(TEXT("Dead actor tick disabled"), Zombie->IsActorTickEnabled());
        TestFalse(TEXT("Dead movement tick disabled"), Zombie->GetCharacterMovement()->IsComponentTickEnabled());
        TestFalse(TEXT("Corpse no longer blocks"), Zombie->GetActorEnableCollision());
        TestTrue(TEXT("Death animation can still update"), Zombie->GetMesh()->IsComponentTickEnabled());
        TestTrue(TEXT("Corpse has finite lifetime"), Zombie->GetLifeSpan() > 0.0f && Zombie->GetLifeSpan() <= 8.0f);
        TestEqual(TEXT("Damage after death ignored"), UGameplayStatics::ApplyDamage(Zombie, 10.0f, nullptr, nullptr, nullptr), 0.0f);
    }
    TestEqual(TEXT("No orphan zombie controllers after 32 deaths"), CountZombieControllers(TestWorld.World), 0);
    TestEqual(TEXT("Corpses initially preserved"), CountZombies(TestWorld.World, false), 32);
    TestWorld.Advance(9.0f);
    TestEqual(TEXT("All corpses removed after lifetime"), CountZombies(TestWorld.World, false), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZombieSpawnCleanupTest, "LastSignal.Zombies.SpawnSlotsAfterDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FZombieSpawnCleanupTest::RunTest(const FString &Parameters)
{
    FZombieTestWorld TestWorld;
    UDataTable *Table = NewObject<UDataTable>(TestWorld.World);
    Table->RowStruct = FZombieSpawnPoolData::StaticStruct();
    FZombieSpawnPoolData Row;
    Row.ZombieClass.Add(AZombieAICharacter::StaticClass());
    Row.InitialSpawnCount = 3;
    Row.PeriodicSpawnCount = 3;
    Row.MaxZombieCount = 3;
    Row.SpawnInterval = 0.5f;
    Table->AddRow(TEXT("Test"), Row);
    AZombieSpawnPool *Pool = TestWorld.World->SpawnActorDeferred<AZombieSpawnPool>(
        AZombieSpawnPool::StaticClass(), FTransform::Identity);
    Pool->SpawnDataRow.DataTable = Table;
    Pool->SpawnDataRow.RowName = TEXT("Test");
    Pool->FinishSpawning(FTransform::Identity);
    APlayerController *Player = TestWorld.World->SpawnActor<APlayerController>();
    APawn *Pawn = TestWorld.World->SpawnActor<APawn>();
    Player->Possess(Pawn);
    Pool->OnOverlapBegin(nullptr, Pawn, nullptr, 0, false, FHitResult());
    TestEqual(TEXT("Initial spawn respects cap"), CountZombies(TestWorld.World, true), 3);
    for (int32 Wave = 0; Wave < 6; ++Wave)
    {
        for (TActorIterator<AZombieAICharacter> It(TestWorld.World); It; ++It)
            if (!It->GetIsDead())
                UGameplayStatics::ApplyDamage(*It, 1000.0f, nullptr, nullptr, nullptr);
        TestWorld.Advance(0.7f);
        TestEqual(TEXT("Dead bodies do not occupy living spawn slots"), CountZombies(TestWorld.World, true), 3);
        TestEqual(TEXT("Only living zombies retain AI controllers"), CountZombieControllers(TestWorld.World), 3);
    }
    TestTrue(TEXT("Test includes retained corpses"), CountZombies(TestWorld.World, false) > 3);
    Pool->Destroy();
    for (TActorIterator<AZombieAICharacter> It(TestWorld.World); It; ++It)
        if (!It->GetIsDead())
            UGameplayStatics::ApplyDamage(*It, 1000.0f, nullptr, nullptr, nullptr);
    TestWorld.Advance(9.0f);
    TestEqual(TEXT("No respawn after pool destruction; all corpses expire"), CountZombies(TestWorld.World, false), 0);
    TestEqual(TEXT("No AI controllers remain"), CountZombieControllers(TestWorld.World), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZombieRagdollTest, "LastSignal.Zombies.RagdollAndAnimation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FZombieRagdollTest::RunTest(const FString &Parameters)
{
    FZombieTestWorld TestWorld;
    const TCHAR *MeshPaths[] = {
        TEXT("/Game/CombetLogicAsset/Zombie/zombie-character-v2-50k-rig"),
        TEXT("/Game/AI/Zombie3/SK_Zombie03_AllAnimations"),
        TEXT("/Game/AI/Zombie4/SK_Zombie04_AllAnimations"),
        TEXT("/Game/AI/Zombie5/SK_Zombie05_AllAnimations"),
        TEXT("/Game/AI/Zombie6/SK_Zombie06_AllAnimations"),
        TEXT("/Game/AI/Zombie8/SK_Zombie08_AllAnimations"),
        TEXT("/Game/AI/Zombie9/SK_Zombie09_AllAnimations")
    };
    const TCHAR *AnimPaths[] = {
        TEXT("/Game/AI/Zombie2/ABP_Zombie_V2.ABP_Zombie_V2_C"),
        TEXT("/Game/AI/Zombie3/ABP_Zombie3.ABP_Zombie3_C"),
        TEXT("/Game/AI/Zombie4/ABP_Zombie4.ABP_Zombie4_C"),
        TEXT("/Game/AI/Zombie5/ABP_Zombie5.ABP_Zombie5_C"),
        TEXT("/Game/AI/Zombie6/ABP_Zombie6.ABP_Zombie6_C"),
        TEXT("/Game/AI/Zombie8/ABP_Zombie8.ABP_Zombie8_C"),
        TEXT("/Game/AI/Zombie9/ABP_Zombie9.ABP_Zombie9_C")
    };
    int32 MeshIndex = 0;
    for (const TCHAR *Path : MeshPaths)
    {
        USkeletalMesh *Asset = LoadObject<USkeletalMesh>(nullptr, Path);
        if (!TestNotNull(Path, Asset)) return false;
        UPhysicsAsset *Physics = Asset->GetPhysicsAsset();
        if (!TestNotNull(TEXT("Zombie mesh has ragdoll asset"), Physics)) return false;
        TestTrue(TEXT("Ragdoll has multiple bodies and joints"),
            Physics->SkeletalBodySetups.Num() > 4 && Physics->ConstraintSetup.Num() > 3);
        for (float Chance : {0.0f, 1.0f})
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AZombieAICharacter *Zombie = TestWorld.World->SpawnActor<AZombieAICharacter>(
                FVector(0.0f, 0.0f, 300.0f), FRotator::ZeroRotator, SpawnParams);
            Zombie->GetMesh()->SetSkeletalMesh(Asset);
            UClass *AnimClass = LoadClass<UAnimInstance>(nullptr, AnimPaths[MeshIndex]);
            if (!TestNotNull(TEXT("Death AnimBP exists"), AnimClass)) return false;
            Zombie->GetMesh()->SetAnimInstanceClass(AnimClass);
            Zombie->RagdollDeathChance = Chance;
            UGameplayStatics::ApplyDamage(Zombie, 10000.0f, nullptr, nullptr, nullptr);
            const bool bRagdoll = Chance == 1.0f;
            TestEqual(TEXT("Probability endpoints choose correct death mode"),
                Zombie->GetMesh()->IsAnySimulatingPhysics(), bRagdoll);
            TestEqual(TEXT("Only ragdoll pauses animation"), bool(Zombie->GetMesh()->bPauseAnims), bRagdoll);
            TestNull(TEXT("Both death modes remove AI controller"), Zombie->GetController());
            TestTrue(TEXT("Both death modes expire"), Zombie->GetLifeSpan() > 0.0f);
            if (!bRagdoll)
            {
                for (int32 Step = 0; Step < 15; ++Step)
                {
                    Zombie->GetMesh()->TickAnimation(0.1f, false);
                    Zombie->GetMesh()->RefreshBoneTransforms();
                }
                TestEqual(FString::Printf(TEXT("%s enters existing Death animation"), Path),
                    Zombie->GetMesh()->GetAnimInstance()->GetCurrentStateName(Zombie->GetMesh()->GetAnimInstance()->GetStateMachineIndex(TEXT("New State Machine"))), FName(TEXT("Death")));
            }
            if (bRagdoll)
            {
                TestEqual(TEXT("Ragdoll uses physics-only collision"),
                    Zombie->GetMesh()->GetCollisionEnabled(), ECollisionEnabled::PhysicsOnly);
                TestEqual(TEXT("Ragdoll contacts floor"),
                    Zombie->GetMesh()->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
                TestEqual(TEXT("Ragdoll ignores players"),
                    Zombie->GetMesh()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
                TestEqual(TEXT("Ragdoll ignores shots"),
                    Zombie->GetMesh()->GetCollisionResponseToChannel(ECC_Visibility), ECR_Ignore);
            }
        }
        ++MeshIndex;
    }
    TestWorld.Advance(9.0f);
    TestEqual(TEXT("All animated and ragdoll corpses removed"), CountZombies(TestWorld.World, false), 0);
    return true;
}

#endif
