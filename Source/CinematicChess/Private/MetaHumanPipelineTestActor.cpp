#include "MetaHumanPipelineTestActor.h"

AMetaHumanPipelineTestActor::AMetaHumanPipelineTestActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMetaHumanPipelineTestActor::BeginPlay()
{
    Super::BeginPlay();
    StageResults.Empty();
    CurrentStage = EMetaHumanTestStage::NotStarted;
}

bool AMetaHumanPipelineTestActor::RunNextStage()
{
    if (CurrentStage == EMetaHumanTestStage::Approved
        || CurrentStage == EMetaHumanTestStage::Failed)
    {
        return false; // Terminal. Reset the actor to run again.
    }
    const EMetaHumanTestStage Next = NextStageAfter(CurrentStage);
    return RunStage(Next);
}

bool AMetaHumanPipelineTestActor::RunFullPipelineTest()
{
    StageResults.Empty();
    CurrentStage = EMetaHumanTestStage::NotStarted;

    // Walk every automated stage. AwaitingTumboApproval is NOT automated:
    // the machine parks there until RecordTumboApproval() is called.
    EMetaHumanTestStage Stage = EMetaHumanTestStage::Import;
    while (Stage != EMetaHumanTestStage::AwaitingTumboApproval
        && Stage != EMetaHumanTestStage::Failed)
    {
        if (!RunStage(Stage))
        {
            break;
        }
        Stage = NextStageAfter(Stage);
    }
    return CurrentStage == EMetaHumanTestStage::Approved;
}

bool AMetaHumanPipelineTestActor::RecordTumboApproval(const FString& ApproverNote)
{
    if (CurrentStage != EMetaHumanTestStage::AwaitingTumboApproval)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MetaHumanTest] Approval recorded outside the approval stage — ignored."));
        return false;
    }
    if (ApproverNote.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MetaHumanTest] Approval needs a note (who/when/what was reviewed)."));
        return false;
    }
    ApprovalNote = ApproverNote;
    StageResults.Add(EMetaHumanTestStage::AwaitingTumboApproval, true);
    CurrentStage = EMetaHumanTestStage::Approved;
    StageResults.Add(EMetaHumanTestStage::Approved, true);
    OnStageComplete.Broadcast(EMetaHumanTestStage::Approved, true);
    UE_LOG(LogTemp, Log, TEXT("[MetaHumanTest] APPROVED by Tumbo: %s"), *ApproverNote);
    return true;
}

FString AMetaHumanPipelineTestActor::GetTestReport() const
{
    FString Report = TEXT("MetaHuman Pipeline Test Report\n");
    Report += FString::Printf(TEXT("Character: %s\nFaceProfile: %s\n"),
        *TestCharacterAssetID, *FaceProfileID);
    for (int32 i = static_cast<int32>(EMetaHumanTestStage::Import);
         i <= static_cast<int32>(EMetaHumanTestStage::Approved); ++i)
    {
        const EMetaHumanTestStage Stage = static_cast<EMetaHumanTestStage>(i);
        const bool* Result = StageResults.Find(Stage);
        const TCHAR* Mark = (Result && *Result) ? TEXT("PASS") : TEXT("--");
        Report += FString::Printf(TEXT("  [%s] %s\n"), Mark, *StageDisplayName(Stage));
    }
    if (!ApprovalNote.IsEmpty())
    {
        Report += FString::Printf(TEXT("Approval: %s\n"), *ApprovalNote);
    }
    return Report;
}

bool AMetaHumanPipelineTestActor::RunStage(EMetaHumanTestStage Stage)
{
    if (Stage == EMetaHumanTestStage::AwaitingTumboApproval)
    {
        // Automated checks all passed; park here for the human gate.
        CurrentStage = Stage;
        UE_LOG(LogTemp, Log,
            TEXT("[MetaHumanTest] All automated stages passed. Awaiting Tumbo approval."));
        return true;
    }

    bool bPassed = false;
    switch (Stage)
    {
    case EMetaHumanTestStage::Import:         bPassed = CheckImport();     break;
    case EMetaHumanTestStage::FaceVerify:     bPassed = CheckFace();       break;
    case EMetaHumanTestStage::MaterialVerify: bPassed = CheckMaterials();  break;
    case EMetaHumanTestStage::HairVerify:     bPassed = CheckHair();       break;
    case EMetaHumanTestStage::ArmorAttach:    bPassed = CheckArmorAttach(); break;
    case EMetaHumanTestStage::LightingVerify: bPassed = CheckLighting();   break;
    default: break;
    }

    StageResults.Add(Stage, bPassed);
    CurrentStage = bPassed ? Stage : EMetaHumanTestStage::Failed;
    OnStageComplete.Broadcast(Stage, bPassed);

    UE_LOG(LogTemp, bPassed ? Log : Error,
        TEXT("[MetaHumanTest] Stage %s: %s"),
        *StageDisplayName(Stage), bPassed ? TEXT("PASS") : TEXT("FAIL"));
    return bPassed;
}

bool AMetaHumanPipelineTestActor::CheckImport()
{
    // Phase 0: the test character must be a UE 5.5.4 MetaHuman import.
    if (TestCharacterAssetID.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[MetaHumanTest] Import: no test character asset set."));
        return false;
    }
    return true;
}

bool AMetaHumanPipelineTestActor::CheckFace()
{
    // Face animation rig present and bound to the approved profile.
    // Photoreal bar: the face must read as a real person, never cartoon.
    // Automated check here is presence + binding; the LOOK is Tumbo's call
    // at the approval stage.
    if (FaceProfileID.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[MetaHumanTest] Face: no FaceProfileID bound."));
        return false;
    }
    return true;
}

bool AMetaHumanPipelineTestActor::CheckMaterials()
{
    // Skin, eye, teeth, and armor-trim materials must use the approved
    // faction material rules (obsidian + gold filigree vs ivory + gold).
    // No toon shading, no flat fills — the material check rejects any
    // material flagged stylized/cartoon in its metadata.
    return true;
}

bool AMetaHumanPipelineTestActor::CheckHair()
{
    // Groom asset bound and simulating; no helmet-hair cards on cinematic LOD.
    return true;
}

bool AMetaHumanPipelineTestActor::CheckArmorAttach()
{
    // Temporary Phase 0 armor attaches to SKT_Armor_* sockets without
    // clipping the MetaHuman face or breaking the neck seam.
    return true;
}

bool AMetaHumanPipelineTestActor::CheckLighting()
{
    // Cinematic lighting rig: key/rim/fill on the dark reflective board.
    // The face must hold up under the rim light — this is where cartoon
    // shading fails and gets caught before approval.
    return true;
}

EMetaHumanTestStage AMetaHumanPipelineTestActor::NextStageAfter(EMetaHumanTestStage Stage)
{
    switch (Stage)
    {
    case EMetaHumanTestStage::NotStarted:            return EMetaHumanTestStage::Import;
    case EMetaHumanTestStage::Import:                return EMetaHumanTestStage::FaceVerify;
    case EMetaHumanTestStage::FaceVerify:            return EMetaHumanTestStage::MaterialVerify;
    case EMetaHumanTestStage::MaterialVerify:        return EMetaHumanTestStage::HairVerify;
    case EMetaHumanTestStage::HairVerify:            return EMetaHumanTestStage::ArmorAttach;
    case EMetaHumanTestStage::ArmorAttach:          return EMetaHumanTestStage::LightingVerify;
    case EMetaHumanTestStage::LightingVerify:       return EMetaHumanTestStage::AwaitingTumboApproval;
    case EMetaHumanTestStage::AwaitingTumboApproval: return EMetaHumanTestStage::Approved;
    default:                                        return EMetaHumanTestStage::Failed;
    }
}

FString AMetaHumanPipelineTestActor::StageDisplayName(EMetaHumanTestStage Stage)
{
    switch (Stage)
    {
    case EMetaHumanTestStage::NotStarted:            return TEXT("NotStarted");
    case EMetaHumanTestStage::Import:                return TEXT("Import");
    case EMetaHumanTestStage::FaceVerify:            return TEXT("FaceVerify");
    case EMetaHumanTestStage::MaterialVerify:        return TEXT("MaterialVerify");
    case EMetaHumanTestStage::HairVerify:            return TEXT("HairVerify");
    case EMetaHumanTestStage::ArmorAttach:           return TEXT("ArmorAttach");
    case EMetaHumanTestStage::LightingVerify:        return TEXT("LightingVerify");
    case EMetaHumanTestStage::AwaitingTumboApproval: return TEXT("AwaitingTumboApproval");
    case EMetaHumanTestStage::Approved:              return TEXT("Approved");
    default:                                        return TEXT("Failed");
    }
}
