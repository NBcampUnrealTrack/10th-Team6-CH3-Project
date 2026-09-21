#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent; // 전방선언 부분입니다.
class UInputMappingContext;
class UInputAction;
class APrimaryWeapon;
class USkillComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiedDelegate); // 캐릭터가 죽을때 다른 클래스로 알려주는 이벤트 타입 만드는 부분 (델리게이트 기반)

UCLASS()
class LASTSIGNAL_API APlayerCharacter : public ACharacter
{
    GENERATED_BODY()

  public:
    APlayerCharacter();

    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const &DamageEvent, AController *EventInstigator, AActor *DamageCauser) override; // 데미지 받는 함수 ( 캐릭터가 데미지 받으면 자동 호출해요)

    UPROPERTY(BlueprintAssignable, Category = "Health|Events") // 캐릭터 죽을 때 이벤트 방송 로직
    FOnDiedDelegate OnDied;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    APrimaryWeapon *GetCurrentWeapon() const { return CurrentWeapon; }

    float GetCurrentHealth() const { return CurrentHealth; } // GameMode가 세이브/로드 때 C++에서만 쓰는 접근자 (블루프린트 노출 필요해지면 그때 UFUNCTION 추가)
    void SetCurrentHealth(float NewHealth) { CurrentHealth = NewHealth; }

    AActor *NearbyInteractable = nullptr; // 상호작용 가능한 근처 오브젝트 (IInteractableTarget 구현체), Radio 등이 오버랩으로 직접 세팅/해제

  protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Camera") // 1인칭 카메라 시점
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
    float CrouchDownInterpSpeed = 200.0f; // 내려갈 때 속도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
    float StandUpInterpSpeed = 240.0f; // 올라올 때 속도

    float DefaultCameraRelativeZ = 60.0f; // 기본 카메라 상대 높이
    float CameraCrouchOffsetZ = 0.0f;

    virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override; // 웅크리기 시작
    virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;   // 웅크리기 끝
    void UpdateCameraCrouchInterp(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input") // 인풋 요소입니다
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    // 캐릭터 행동관련

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CrouchAction;

    // 캐릭터 이동속도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float CrouchSpeed = 250.0f;

    // 스킬 입력 액션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SkillAction;

    // 인터랙션 입력 액션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    // 스킬 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    TObjectPtr<USkillComponent> SkillComponent;

    void Move(const FInputActionValue &Value); // Look이랑 Move 이쪽입니다.
    void Look(const FInputActionValue &Value);

    void StartSprint(); // sprint
    void StopSprint();
    void StartCrouch(); // crouch
    void StopCrouch();
    void UseSkill();
    void TryInteract();

    // 여기부터 캐릭터 체력 관련 UPROPERTY랑 함수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health") // 최대체력
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Health") // 현재 체력
    float CurrentHealth = 100.0f;

    void Die(); // 체력 0되면 호출처리하는 함수

    UPROPERTY(EditDefaultsOnly, Category = "Weapon") // 어떤 무기를 장착할지 (BP_PlayerCharacter Class Defaults에서 지정)
    TSubclassOf<APrimaryWeapon> WeaponClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon") // 실제로 스폰돼서 장착된 무기 인스턴스
    TObjectPtr<APrimaryWeapon> EquippedWeapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    APrimaryWeapon *CurrentWeapon;

    // 기본 걷기와 달리기의 흔들림은 실제 지상 이동 속도로 조절한다.
    void UpdateMovementBob(float DeltaTime);
    virtual void OnJumped_Implementation() override;
    virtual void Landed(const FHitResult &Hit) override;

    // 기존 변수 이름을 유지하여 BP에 저장된 걷기 설정을 보존한다.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Walk",
              meta = (ClampMin = "0.0", DisplayName = "Walk Weapon Scale (걷기 팔 배율)"))
    float WeaponBobScale = 1.0f;

    // 무기 액터의 로컬 축: X 전후, Y 좌우, Z 상하. 단위는 cm.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Walk",
              meta = (DisplayName = "Walk Weapon Amplitude (걷기 팔 이동폭)"))
    FVector WalkWeaponBobAmplitude = FVector(0.0f, 1.2f, 0.7f);

    // 로컬 cpp에서 사용자가 맞춘 0.8 주기를 그대로 기본값으로 사용한다.
    // 주파수 단위는 Hz이며, 팔 상하 움직임은 좌우 주기의 두 배다.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Walk",
              meta = (ClampMin = "0.01", DisplayName = "Walk Weapon Frequency (걷기 팔 주파수)"))
    float WalkWeaponBobFrequency = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Walk",
              meta = (ClampMin = "0.0", DisplayName = "Walk Camera Height (걷기 카메라 높이)"))
    float CameraBobHeight = 0.15f;

    // 기존 카메라는 0.8 주기의 두 배로 상하 이동하므로 기본값은 1.6Hz다.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Walk",
              meta = (ClampMin = "0.01", DisplayName = "Walk Camera Frequency (걷기 카메라 주파수)"))
    float WalkCameraBobFrequency = 1.6f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Sprint",
              meta = (ClampMin = "0.0", DisplayName = "Sprint Weapon Scale (달리기 팔 배율)"))
    float SprintWeaponBobScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Sprint",
              meta = (DisplayName = "Sprint Weapon Amplitude (달리기 팔 이동폭)"))
    FVector SprintWeaponBobAmplitude = FVector(0.0f, 1.8f, 1.05f);

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Sprint",
              meta = (ClampMin = "0.01", DisplayName = "Sprint Weapon Frequency (달리기 팔 주파수)"))
    float SprintWeaponBobFrequency = 1.2f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Sprint",
              meta = (ClampMin = "0.0", DisplayName = "Sprint Camera Height (달리기 카메라 높이)"))
    float SprintCameraBobHeight = 0.225f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Sprint",
              meta = (ClampMin = "0.01", DisplayName = "Sprint Camera Frequency (달리기 카메라 주파수)"))
    float SprintCameraBobFrequency = 2.4f;

    // 점프 성공 시 잠깐 내려갔다가 원위치로 돌아오는 움직임.
    // Offset은 최대 이동량(cm), Duration은 전체 복귀 시간(초)이다.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Jump",
              meta = (DisplayName = "Jump Weapon Offset (점프 팔 이동량)"))
    FVector JumpWeaponOffset = FVector(-0.5f, 0.0f, -1.5f);

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Jump",
              meta = (ClampMin = "0.01", DisplayName = "Jump Weapon Duration (점프 팔 지속시간)"))
    float JumpWeaponDuration = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Jump",
              meta = (DisplayName = "Jump Camera Offset Z (점프 카메라 이동량)"))
    float JumpCameraOffsetZ = -0.15f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Jump",
              meta = (ClampMin = "0.01", DisplayName = "Jump Camera Duration (점프 카메라 지속시간)"))
    float JumpCameraDuration = 0.25f;

    // 착지 충격은 낙하 속도에 비례하며 최대 배율로 제한한다.
    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (DisplayName = "Land Weapon Offset (착지 팔 이동량)"))
    FVector LandWeaponOffset = FVector(-0.8f, 0.0f, -2.5f);

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (ClampMin = "0.01", DisplayName = "Land Weapon Duration (착지 팔 지속시간)"))
    float LandWeaponDuration = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (DisplayName = "Land Camera Offset Z (착지 카메라 이동량)"))
    float LandCameraOffsetZ = -0.3f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (ClampMin = "0.01", DisplayName = "Land Camera Duration (착지 카메라 지속시간)"))
    float LandCameraDuration = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (ClampMin = "1.0", DisplayName = "Land Reference Speed (착지 기준 낙하속도)"))
    float LandReferenceSpeed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Land",
              meta = (ClampMin = "0.0", DisplayName = "Land Max Scale (착지 최대 배율)"))
    float LandMaxScale = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Common",
              meta = (ClampMin = "0.01", DisplayName = "Bob Blend Speed (흔들림 전환 속도)"))
    float BobBlendSpeed = 8.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Common",
              meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Aim Bob Scale (조준 흔들림 배율)"))
    float AimBobScale = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Camera|MovementBob|Common",
              meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Crouch Bob Scale (앉기 흔들림 배율)"))
    float CrouchBobScale = 0.6f;

    // 실행 중 상태는 설정값과 분리한다. 아래 값들은 에디터 편집 대상이 아니다.
    bool bSprintBobRequested = false;
    float SprintBobBlend = 0.0f;
    float MovementBobPhase = 0.0f;
    float CameraBobPhase = 0.0f;
    float MovementBobWeight = 0.0f;
    float SmoothedBobStance = 1.0f;
    float JumpBobElapsed = -1.0f;
    float LandBobElapsed = -1.0f;
    float LandBobStrength = 0.0f;

    TWeakObjectPtr<APrimaryWeapon> BobWeapon;
    FVector BobWeaponBaseLocation = FVector::ZeroVector;
};
