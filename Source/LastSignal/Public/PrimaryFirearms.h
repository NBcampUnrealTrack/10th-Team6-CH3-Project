#pragma once

#include "CoreMinimal.h"
#include "PrimaryWeapon.h"
#include "PrimaryFirearms.generated.h"

// M4A1: 자동 사격, 일반탄, 탄창 교체.
UCLASS()
class LASTSIGNAL_API AM4A1 : public APrimaryWeapon
{
    GENERATED_BODY()

  public:
    AM4A1();
};

// SR25: 반자동 사격, 일반탄, 탄창 교체.
UCLASS()
class LASTSIGNAL_API ASR25 : public APrimaryWeapon
{
    GENERATED_BODY()

  public:
    ASR25();
};

// MP153: 반자동 사격, 산탄, 개별 장전.
UCLASS()
class LASTSIGNAL_API AMP153 : public APrimaryWeapon
{
    GENERATED_BODY()

  public:
    AMP153();
};