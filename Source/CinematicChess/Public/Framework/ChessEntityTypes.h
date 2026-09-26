#pragma once

#include "CoreMinimal.h"
#include "ChessEntityTypes.generated.h"

UENUM(BlueprintType)
enum class EChessFaction : uint8 { White, Black };

UENUM(BlueprintType)
enum class EChessRank : uint8 { King, Queen, Bishop, Knight, Rook, Pawn };
