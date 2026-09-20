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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiedDelegate);  // 캐릭터가 죽을때 다른 클래스로 알려주는 이벤트 타입 만드는 부분 (델리게이트 기반)

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

	UPROPERTY(VisibleAnywhere, Category = "Camera") //1인칭 카메라 시점
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

    //인터랙션 입력 액션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    // 일시정지 메뉴 입력 액션
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> PauseAction;

    // 스킬 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    TObjectPtr<USkillComponent> SkillComponent;
    

    void Move(const FInputActionValue &Value);   // Look이랑 Move 이쪽입니다.
    void Look(const FInputActionValue &Value);

    void StartSprint(); // sprint
    void StopSprint();
    void StartCrouch(); // crouch
    void StopCrouch(); 
    void UseSkill();
    void TryInteract();
    void TogglePauseMenu();

    // 여기부터 캐릭터 체력 관련 UPROPERTY랑 함수

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health") // 최대체력
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Health") // 현재 체력
    float CurrentHealth = 100.0f;

    void Die();  // 체력 0되면 호출처리하는 함수

    UPROPERTY(EditDefaultsOnly, Category = "Weapon") // 어떤 무기를 장착할지 (BP_PlayerCharacter Class Defaults에서 지정)
    TSubclassOf<APrimaryWeapon> WeaponClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon") // 실제로 스폰돼서 장착된 무기 인스턴스
    TObjectPtr<APrimaryWeapon> EquippedWeapon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    APrimaryWeapon *CurrentWeapon;

};
