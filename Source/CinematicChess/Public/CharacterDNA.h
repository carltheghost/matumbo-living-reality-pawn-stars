#pragma once

#include "CoreMinimal.h"
#include "CharacterDNA.generated.h"

UENUM(BlueprintType)
enum class EChessFaction : uint8
{
    White,
    Black
};

UENUM(BlueprintType)
enum class EChessPieceType : uint8
{
    King,
    Queen,
    Bishop,
    Knight,
    Rook,
    Pawn
};

USTRUCT(BlueprintType)
struct FCharacterDNA
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CharacterID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessFaction Faction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChessPieceType PieceType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString FaceProfileID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArmorSetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MaterialSetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString WeaponAttachmentID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsesMetaHumanFace = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsCentaurArchitecture = false;
};
