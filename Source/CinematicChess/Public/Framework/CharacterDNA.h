#pragma once

#include "CoreMinimal.h"
#include "CharacterDNA.generated.h"

USTRUCT(BlueprintType)
struct FCharacterDNA
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString EntityID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;
};
