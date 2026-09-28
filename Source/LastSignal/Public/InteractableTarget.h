#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableTarget.generated.h"


UINTERFACE(MinimalAPI)
class UInteractableTarget : public UInterface
{
	GENERATED_BODY()
};

class LASTSIGNAL_API IInteractableTarget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "Interact")
    void Interact(AActor *Interactor);

	// 가까이 가면 뜨는 [F] 안내에 쓸 행동 이름 (예: "무전기 사용"). 빈 텍스트면 안내를 숨긴다
	UFUNCTION(BlueprintNativeEvent, Category = "Interact")
	FText GetInteractPromptText() const;
	virtual FText GetInteractPromptText_Implementation() const { return FText::FromString(TEXT("상호작용")); }
};
