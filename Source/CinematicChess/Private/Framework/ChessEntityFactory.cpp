#include "Framework/ChessEntityFactory.h"
#include "Framework/CinematicChessEntity.h"
#include "Framework/ChessEntityTypes.h"
#include "Engine/World.h"

ACinematicChessEntity* UCinematicChessEntityFactory::SpawnFromDNA(
    UObject* WorldContextObject,
    TSubclassOf<ACinematicChessEntity> EntityClass,
    const FCharacterDNA& DNA,
    const FTransform& SpawnTransform)
{
    if (!WorldContextObject || !*EntityClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[CinematicChess] SpawnFromDNA: bad world or entity class."));
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACinematicChessEntity* Entity = World->SpawnActor<ACinematicChessEntity>(
        EntityClass, SpawnTransform, Params);
    if (!Entity)
    {
        UE_LOG(LogTemp, Error, TEXT("[CinematicChess] SpawnFromDNA: spawn failed for %s."),
            *DNA.CharacterID);
        return nullptr;
    }

    Entity->InitializeFromDNA(DNA);
    return Entity;
}

TArray<ACinematicChessEntity*> UCinematicChessEntityFactory::SpawnFullSet(
    UObject* WorldContextObject,
    TSubclassOf<ACinematicChessEntity> EntityClass,
    const TArray<FCharacterDNA>& RosterDNA,
    float SquareSizeCm)
{
    TArray<ACinematicChessEntity*> Spawned;

    FString Error;
    if (!ValidateRoster(RosterDNA, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[CinematicChess] SpawnFullSet: roster invalid: %s"), *Error);
        return Spawned;
    }

    // Count piece occurrences per faction so each DNA block lands on the
    // correct regulation square (e.g. the two knights -> b1/g1).
    // (int32 keys: enum-class keys need per-enum GetTypeHash specializations.)
    TMap<int32, int32> WhiteSeen;
    TMap<int32, int32> BlackSeen;

    for (const FCharacterDNA& DNA : RosterDNA)
    {
        TMap<int32, int32>& Seen =
            (DNA.Faction == EChessFaction::White) ? WhiteSeen : BlackSeen;
        const int32 PieceKey = static_cast<int32>(DNA.PieceType);
        const int32 PieceIndex = Seen.FindOrAdd(PieceKey);

        const FString Square = StartingSquareFor(DNA.Faction, DNA.PieceType, PieceIndex);
        const FVector Location = SquareToWorld(Square, SquareSizeCm);
        const FRotator Rotation = (DNA.Faction == EChessFaction::White)
            ? FRotator::ZeroRotator
            : FRotator(0.0f, 180.0f, 0.0f);

        ACinematicChessEntity* Entity = SpawnFromDNA(
            WorldContextObject, EntityClass, DNA, FTransform(Rotation, Location));
        if (Entity)
        {
            Entity->SetBoardSquare(Square);
            Spawned.Add(Entity);
        }

        Seen[PieceKey] = PieceIndex + 1;
    }

    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] Spawned full set: %d entities."), Spawned.Num());
    return Spawned;
}

bool UCinematicChessEntityFactory::ValidateRoster(const TArray<FCharacterDNA>& RosterDNA, FString& OutError)
{
    if (RosterDNA.Num() != 32)
    {
        OutError = FString::Printf(TEXT("Roster must hold 32 DNA blocks, found %d."), RosterDNA.Num());
        return false;
    }

    TSet<FString> SeenIDs;
    TMap<int32, TMap<int32, int32>> Counts;

    for (const FCharacterDNA& DNA : RosterDNA)
    {
        if (DNA.CharacterID.IsEmpty() || SeenIDs.Contains(DNA.CharacterID))
        {
            OutError = FString::Printf(TEXT("Duplicate or empty CharacterID '%s'."), *DNA.CharacterID);
            return false;
        }
        SeenIDs.Add(DNA.CharacterID);
        Counts.FindOrAdd(static_cast<int32>(DNA.Faction)).FindOrAdd(static_cast<int32>(DNA.PieceType))++;
    }

    // Regulation army composition per faction: 1K 1Q 2B 2N 2R 8P.
    const TPair<EChessPieceType, int32> Expected[] = {
        {EChessPieceType::King, 1}, {EChessPieceType::Queen, 1},
        {EChessPieceType::Bishop, 2}, {EChessPieceType::Knight, 2},
        {EChessPieceType::Rook, 2}, {EChessPieceType::Pawn, 8},
    };

    for (EChessFaction Faction : {EChessFaction::White, EChessFaction::Black})
    {
        const TMap<int32, int32>& Got = Counts.FindOrAdd(static_cast<int32>(Faction));
        for (const TPair<EChessPieceType, int32>& Pair : Expected)
        {
            const int32 Have = Got.FindRef(static_cast<int32>(Pair.Key));
            if (Have != Pair.Value)
            {
                OutError = FString::Printf(TEXT("Faction %d: expected %d of piece %d, found %d."),
                    static_cast<int32>(Faction), Pair.Value,
                    static_cast<int32>(Pair.Key), Have);
                return false;
            }
        }
    }

    OutError = TEXT("");
    return true;
}

FString UCinematicChessEntityFactory::StartingSquareFor(
    EChessFaction Faction, EChessPieceType PieceType, int32 PieceIndex)
{
    const bool bWhite = (Faction == EChessFaction::White);
    const int32 BackRank = bWhite ? 1 : 8;
    const int32 PawnRank = bWhite ? 2 : 7;

    switch (PieceType)
    {
    case EChessPieceType::King:   return FString::Printf(TEXT("e%d"), BackRank);
    case EChessPieceType::Queen:  return FString::Printf(TEXT("d%d"), BackRank);
    case EChessPieceType::Bishop: return FString::Printf(TEXT("%c%d"), PieceIndex == 0 ? TEXT('c') : TEXT('f'), BackRank);
    case EChessPieceType::Knight: return FString::Printf(TEXT("%c%d"), PieceIndex == 0 ? TEXT('b') : TEXT('g'), BackRank);
    case EChessPieceType::Rook:   return FString::Printf(TEXT("%c%d"), PieceIndex == 0 ? TEXT('a') : TEXT('h'), BackRank);
    case EChessPieceType::Pawn:
    {
        const char File = static_cast<char>(TEXT('a') + PieceIndex);
        return FString::Printf(TEXT("%c%d"), File, PawnRank);
    }
    default:
        return TEXT("a1");
    }
}

FVector UCinematicChessEntityFactory::SquareToWorld(const FString& Algebraic, float SquareSizeCm)
{
    FChessBoardCoordinate Coord;
    if (!FChessBoardCoordinate::FromAlgebraic(Algebraic, Coord))
    {
        return FVector::ZeroVector;
    }
    // Board origin at a1 corner; X = files, Y = ranks.
    return FVector(Coord.Col * SquareSizeCm, Coord.Row * SquareSizeCm, 0.0f);
}
