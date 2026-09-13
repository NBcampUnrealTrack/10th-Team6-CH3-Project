#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;            // 전방선언 부분입니다.
class UInputMappingContext;
class UInputAction;
class APrimaryWeapon;

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



protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Camera") //1인칭 카메라 시점
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input") // 인풋 요소입니다
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;
    
    void Move(const FInputActionValue &Value);   // Look이랑 Move 이쪽입니다.
    void Look(const FInputActionValue &Value);


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


};
