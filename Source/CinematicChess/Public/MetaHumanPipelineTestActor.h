#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessEntityTypes.h"
#include "MetaHumanPipelineTestActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnMetaHumanTestStageComplete, EMetaHumanTestStage, Stage, bool, bPassed);

/**
 * Phase 0 MetaHuman pipeline validation actor.
 *
 * Walks the EMetaHumanTestStage machine in order against one imported
 * MetaHuman test character: import -> face -> materials -> hair ->
 * armor attach -> lighting -> Tumbo approval.
 *
 * A failed stage blocks every later stage. No face-bearing asset ships to
 * the public build without reaching Approved — and Approved requires
 * Tumbo's explicit sign-off, recorded via RecordTumboApproval().
 * This is the enforcement point for his rule, not a suggestion.
 */
UCLASS()
class CINEMATICCHESS_API AMetaHumanPipelineTestActor : public AActor
{
    GENERATED_BODY()

public:
    AMetaHumanPipelineTestActor();

    /** MetaHuman asset to validate (imported test character). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
    FString TestCharacterAssetID;

    /** Face profile under test. Empty = face checks are skipped as failed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
    FString FaceProfileID;

    /** Current stage. Read-only at runtime; advance via RunNextStage(). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MetaHuman Test")
    EMetaHumanTestStage CurrentStage = EMetaHumanTestStage::NotStarted;

    UPROPERTY(BlueprintAssignable, Category = "MetaHuman Test")
    FOnMetaHumanTestStageComplete OnStageComplete;

    /** Runs the next stage in the machine. Returns false when blocked/finished. */
    UFUNCTION(BlueprintCallable, Category = "MetaHuman Test")
    bool RunNextStage();

    /** Runs the whole machine from the start. Returns true only if Approved. */
    UFUNCTION(BlueprintCallable, Category = "MetaHuman Test")
    bool RunFullPipelineTest();

    /** Records Tumbo's approval. The ONLY path to Approved. */
    UFUNCTION(BlueprintCallable, Category = "MetaHuman Test")
    bool RecordTumboApproval(const FString& ApproverNote);

    /** Human-readable report of every stage result. */
    UFUNCTION(BlueprintCallable, Category = "MetaHuman Test")
    FString GetTestReport() const;

protected:
    virtual void BeginPlay() override;

private:
    TMap<EMetaHumanTestStage, bool> StageResults;
    FString ApprovalNote;

    bool RunStage(EMetaHumanTestStage Stage);
    bool CheckImport();
    bool CheckFace();
    bool CheckMaterials();
    bool CheckHair();
    bool CheckArmorAttach();
    bool CheckLighting();

    static EMetaHumanTestStage NextStageAfter(EMetaHumanTestStage Stage);
    static FString StageDisplayName(EMetaHumanTestStage Stage);
};
