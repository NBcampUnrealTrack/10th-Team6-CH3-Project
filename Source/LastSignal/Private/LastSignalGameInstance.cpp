#include "LastSignalGameInstance.h"

void ULastSignalGameInstance::Init()
{
	Super::Init();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("GameInstance Init"));
}