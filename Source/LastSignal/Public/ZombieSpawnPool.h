#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawnPool.generated.h"

class UBoxComponent;

// 맵별 스폰 설정을 담는 데이터 테이블 구조체
USTRUCT(BlueprintType)
struct FZombieSpawnPoolData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 동작할 대상 맵 이름 (비워두면 모든 맵에서 동작)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FName TargetMapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TArray<TSubclassOf<AActor>> ZombieClass;

	// 최초 진입 시 스폰 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	int32 InitialSpawnCount = 5;

	// 주기적 추가 스폰 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	int32 PeriodicSpawnCount = 2;

	// 스폰 주기 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnInterval = 3.0f;

	// 필드 내 최대 유지 가능 좀비 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	int32 MaxZombieCount = 15;

	// UI 추가: 화면에 표시할 미션 목표 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FText MissionObjectiveText;
};

UCLASS()
class LASTSIGNAL_API AZombieSpawnPool : public AActor
{
	GENERATED_BODY()

public:
	AZombieSpawnPool();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultRootComponent;

	// [플레이어 진입 감지용] 트리거 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	// [좀비 실제 스폰 영역] 스폰 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Data")
	FDataTableRowHandle SpawnDataRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Data")
	TObjectPtr<UDataTable> SpawnDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	FName TargetMapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	TArray<TSubclassOf<AActor>> ZombieClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	int32 InitialSpawnCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	int32 PeriodicSpawnCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	int32 MaxZombieCount = 15;

	// UI 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Settings")
	FText MissionObjectiveText;

	// 디버그 메세지 화면 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Spawn|Debug")
	bool bShowDebugLog = true;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	FTimerHandle SpawnTimerHandle;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedZombies;

	bool bPlayerInside = false;
	bool bInitialSpawnDone = false;
	bool bIsActivePool = true;

	void LoadMapSpecificData();
	void StartSpawningProcess(APawn* PlayerPawn);
	void StopSpawningProcess();
	void SpawnZombieBatch(int32 Count);
	void SpawnPeriodicZombies();
	FVector GetRandomSpawnPoint() const;
	void CleanupDeadZombies();
	bool IsPlayerActor(AActor* Actor, APawn*& OutPlayerPawn) const;
	bool IsPlayerInTrigger() const;
	void PrintDebugMessage(const FString& Message, FColor Color = FColor::Green) const;

	// UI
	void UpdateMissionObjective() const;
};