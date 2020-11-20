//----------------------------------------------------------------------------------
//
// ENM_BasicKnight.h
//
// Copyright 2018-2020 Silveryte Studios
//
//----------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include "CoreMinimal.h"
#include "ENM/ENM_BaseEnemy.h"
#include "ENM/Support/ENM_BasicWeapon.h"

#include "ENM_BasicKnight.generated.h"

//----------------------------------------------------------------------------------
// Preprocessor
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------
class UANI_CharacterAnimationInstance;
class USoundCue;

//----------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Class Definition
//----------------------------------------------------------------------------------
UCLASS()
class INTELLIGENTSIAUE4_API AENM_BasicKnight : public AENM_BaseEnemy
{
GENERATED_BODY()
//----------------------------------------------------------------------------------
// Members
//----------------------------------------------------------------------------------
protected:
    // General
    /// Pitch, used when aiming 
    float Pitch = 0.0f;

    // Mesh
    /// Head mesh
    UPROPERTY() UStaticMeshComponent *HeadMesh;

    // Animation
    /// 
    UPROPERTY() UANI_CharacterAnimationInstance *PrimaryAnimation;

    /// 
    UPROPERTY() UENM_MoveComponent *MoveComponent;

    /// 
    bool Stunned;

    /// 
    const float RateOfFire = 2.0f;

    /// 
    float NextTime = 0.0f;

    /// 
    float AbilityNextTime = 0.0f;

    /// 
    bool PerformMove = true;

    /// 
    uint8 BurstProjectilesFired = 0;

    /// 
    float EngagementDistance = 2000.0f;

    FVector WatchDogLocation = FVector::ZeroVector;

    // Sound
    /// 
    UPROPERTY() USoundCue *WeaponSound;

    // Materials
    /// 
    UPROPERTY() UMaterialInstance *LightMaterial;

    /// 
    UPROPERTY() UMaterialInstance *CharacterMaterial;

    // Timer Handles
    /// 
    FTimerHandle CleaveTimerHandle;

    /// 
    FTimerHandle BurstFireTimerHandle;

    /// 
    FTimerHandle WatchDogTimerHandle;

//----------------------------------------------------------------------------------
// Methods
//----------------------------------------------------------------------------------
    // Constructors
    // Overrides
    // Accessors
    // Mutators
    // Helpers
    // Mechanics
    // Wrappers
    // Constructor
public:
	AENM_BasicKnight();

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

    float CalculateEnagementDistance();
    void CleanupAfterAction();

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
