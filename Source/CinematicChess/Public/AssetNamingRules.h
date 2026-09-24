#pragma once

#include "CoreMinimal.h"

namespace CinematicChessAssetRules
{
    static const FString CharacterPrefix = TEXT("CHR_");
    static const FString ArmorPrefix = TEXT("ARM_");
    static const FString WeaponPrefix = TEXT("WPN_");
    static const FString MaterialPrefix = TEXT("MAT_");

    inline bool ValidateCharacterName(const FString& Name)
    {
        return Name.StartsWith(CharacterPrefix);
    }

    inline bool ValidateArmorName(const FString& Name)
    {
        return Name.StartsWith(ArmorPrefix);
    }

    inline bool ValidateWeaponName(const FString& Name)
    {
        return Name.StartsWith(WeaponPrefix);
    }

    inline bool ValidateMaterialName(const FString& Name)
    {
        return Name.StartsWith(MaterialPrefix);
    }
}
