/**************************************************************

    Module: ENM_BasicKnight.h

    Copyright 2018 Tyler Graveline
  
**************************************************************/

#pragma once

/**************************************************************
                    GENERAL INCLUDES
**************************************************************/
#include "CoreMinimal.h"
#include "ENM/ENM_BaseEnemy.h"
#include "ENM/Support/ENM_BasicWeapon.h"

#include "ENM_BasicKnight.generated.h"

/**************************************************************
                    LITERAL CONSTANTS
**************************************************************/

/**************************************************************
                    MEMORY CONSTANTS
**************************************************************/

/**************************************************************
                    MACROS
**************************************************************/

/**************************************************************
                    TYPE DEFINITIONS
**************************************************************/
class UANI_CharacterAnimationInstance;
class USoundCue;

/**************************************************************
                    CLASSES
**************************************************************/
UCLASS()
class INTELLIGENTSIAUE4_API AENM_BasicKnight : public AENM_BaseEnemy
{
GENERATED_BODY()
/**************************************************************
                    VARIABLES
**************************************************************/	
protected:
    // General
    float Pitch = 0.0f; // Pitch, used when aiming 

    // Mesh
    UStaticMeshComponent    *HeadMesh;   // Head mesh

    // Animation
    UANI_CharacterAnimationInstance *PrimaryAnimation;
    UENM_MoveComponent *MoveComponent;
    bool Stunned;

    const float RateOfFire = 2.0f;
    float NextTime = 0.0f;
    float AbilityNextTime = 0.0f;
    bool PerformMove = true;
    uint8 BurstProjectilesFired = 0;
    float EngagementDistance = 2000.0f;

    FVector WatchDogLocation = FVector::ZeroVector;

    // Sound
    USoundCue *WeaponSound;

    // Materials
    UMaterialInstance *LightMaterial;
    UMaterialInstance *CharacterMaterial;

    // Timer Handles
    FTimerHandle CleaveTimerHandle;
    FTimerHandle BurstFireTimerHandle;
    FTimerHandle WatchDogTimerHandle;

/**************************************************************
                    PROCEDURES
**************************************************************/
    // Constructor
public:
	AENM_BasicKnight();

    // Overrides
    virtual void Tick(float DeltaTime) override;
    virtual void InitializeAndRun(AENM_CombatScenarioController *NewCombatController, AENM_TetherPoint* NewTether) override;
    virtual bool Fire() override;
    virtual bool IsCriticalComponent(UPrimitiveComponent *Component) override;
    virtual void MoveCompleted(const FPathFollowingResult& Result) override;
protected:
	virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
  	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void Landed(const FHitResult& Hit) override;

    // Helpers
    float CalculateEnagementDistance();
    void CleanupAfterAction();

    // Mechanics
    void ProcessMovement();
    void ProcessRotation();
    void ProcessPromote();
    void ProcessFire();

    void StartFire();
    void StopFire();

    void PerformLunge();
    void StopLunge();
    void PerformGroundAttack();
    void PerformPowerShot();
    //void PerformCleave();
    //void PerformSkyBurst();

    void BurstFire();
    void Knockback();

    void HandleStuckEnemy();
    void KickWatchDog();

};
