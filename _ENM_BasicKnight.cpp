//----------------------------------------------------------------------------------
//
// ENM_BasicKnight.cpp
//
// Copyright 2018-2020 Silveryte Studios
//
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include "ENM_BasicKnight.h"

#include "AI/Navigation/NavigationSystem.h"
#include "ANI/ANI_CharacterAnimationInstance.h"
#include "ASS_Assets.h"
#include "ASS_Colors.h"
#include "CHR/CHR_Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ENM/ENM_BaseController.h"
#include "ENM/Support/ENM_GroundAttack.h"
#include "ENM/Support/ENM_LinkPathComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "UTL/UTL_Aiming.h"

//----------------------------------------------------------------------------------
// Preprocessor
//----------------------------------------------------------------------------------
#define BASIC_KNIGHT_HEALTH         ( 300.0f )
#define BASIC_KNIGHT_LUNGE_SPEED    ( 7000.0f )
#define KNIGHT_KNOCKBACK_RADIUS     ( 400.0f )
#define BASIC_KNIGHT_BURST          ( 3 )
#define BASIC_KNIGHT_BURST_INTERVAL ( 0.2f )
#define BASIC_KNIGHT_LUNGE_DISTANCE ( 1800.0f )
#define BASIC_KNIGHT_SHOT_DISTANCE  ( 3000.0f )

// Physics
#define GRAVITY_DFLT                ( 4.0f )

#define KNIGHT_DEBUG                true
#if KNIGHT_DEBUG
#include "Engine.h"
#include "DrawDebugHelpers.h"
#endif

//----------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------

// TYPE A

// TYPE B

// TYPE C

//----------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------
// stuff and things that are globals

// more stuff and things that are globals

//----------------------------------------------------------------------------------
// Methods
//----------------------------------------------------------------------------------

/**-------------------------------------------------------------
* @brief Default constructor
*
* @return 
*/
AENM_BasicKnight::AENM_BasicKnight()
{
    // Tick ON
	PrimaryActorTick.bCanEverTick = true;

    GetCharacterMovement()->PathFollowingComp = CreateDefaultSubobject<UENM_LinkPathComponent>(TEXT("LinkPath"));
    GetCharacterMovement()->MaxWalkSpeed = 1200.0f;

    // Set controller
    AIControllerClass = AENM_BaseController::StaticClass();
    AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Setup character mesh
    GetMesh()->SetSkeletalMesh(ASS_LoadAsset<USkeletalMesh>(SKE_CHARACTER));
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    GetMesh()->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));
    GetMesh()->SetCollisionProfileName(FName("Enemy"));

    // Setup character mesh animation
    //GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    //GetMesh()->SetAnimInstanceClass(ASS_LoadAsset<UClass>(ANI_ENM_PAWN_BP));

    // Setup head
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetStaticMesh(ASS_LoadAsset<UStaticMesh>(STA_HEAD4));
    HeadMesh->SetCollisionProfileName(FName("Enemy"));

    // Setup materials
    CharacterMaterial = ASS_LoadAsset<UMaterialInstance>(MAT_BASIC_PAWN);
    GetMesh()->SetMaterialByName(FName("BaseSlot"), CharacterMaterial);
    HeadMesh->SetMaterialByName(FName("BaseSlot"), CharacterMaterial);

    LightMaterial = ASS_LoadAsset<UMaterialInstance>(MAT_ORANGE_LIGHT);
    GetMesh()->SetMaterialByName(FName("LightSlot"), LightMaterial);
    HeadMesh->SetMaterialByName(FName("LightSlot"), LightMaterial);

    // Setup Sound
    WeaponSound = ASS_LoadAsset<USoundCue>(SND_ENEMY_FIRE);

    // Setup character movement and physics
    JumpAllowed = true;
    GetCapsuleComponent()->SetEnableGravity(false);
    GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);
    GetCapsuleComponent()->SetCapsuleRadius(42.0f);
    GetCapsuleComponent()->SetCollisionProfileName(FName("Enemy"));
    GetCharacterMovement()->GravityScale = GRAVITY_DFLT;

    // Class values
    MaxHealth          = BASIC_KNIGHT_HEALTH;
    Health             = MaxHealth;
    CurrentTetherFlags = TetherFlagType::TETHER_NO_FLY;

} // AENM_BasicKnight


/**-------------------------------------------------------------
* @brief Called after spawning actor but before play
*
* @param Transform 
*/
void AENM_BasicKnight::OnConstruction(const FTransform& Transform)
{
    // Locals
    FAttachmentTransformRules AttachmentRules(FAttachmentTransformRules::KeepWorldTransform);

    // Attach Head
    AttachmentRules.bWeldSimulatedBodies = true;
    AttachmentRules.LocationRule = EAttachmentRule::SnapToTarget;
    AttachmentRules.RotationRule = EAttachmentRule::KeepRelative;
    AttachmentRules.ScaleRule    = EAttachmentRule::KeepRelative;
    HeadMesh->AttachToComponent( GetMesh(), AttachmentRules, FName(HEAD_SOCKET) );
    HeadMesh->SetRelativeLocation(FVector(-0.05f, 0.6f, 0.05f));

} // OnConstruction


/**-------------------------------------------------------------
* @brief Called once actor has been spawned into world
*/
void AENM_BasicKnight::BeginPlay()
{
    // Locals
	Super::BeginPlay();
    KickWatchDog();

    // Get animation instance
    PrimaryAnimation = Cast<UANI_CharacterAnimationInstance>(GetMesh()->GetAnimInstance());

} // BeginPlay


/**-------------------------------------------------------------
* @brief Called when actor is being removed from world
*
* @param EndPlayReason 
*/
void AENM_BasicKnight::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ASS_DynamicLoadAsset<UParticleSystem>(SYS_DEATH), GetActorTransform());
    UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ASS_DynamicLoadAsset<USoundCue>(SND_DEATH), GetActorLocation(), FRotator::ZeroRotator, 100.0f);

} // EndPlay


/**-------------------------------------------------------------
* @brief Called every frame
*
* @param DeltaTime 
*/
void AENM_BasicKnight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    ProcessRotation();
    ProcessMovement();

    //{
    //    ACharacter *Character = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    //    UNavigationPath* Path;
    //    Path = UNavigationSystem::FindPathToLocationSynchronously(this, GetActorLocation(), Character->GetActorLocation(), this);

    //    if (Path && Path->IsValid() && !Path->IsPartial())
    //    {
    //        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Path Found") );
    //    }
    //    else
    //    {
    //        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Path Not Found") );
    //    }
    //}

} // Tick


/**-------------------------------------------------------------
* @brief 
*
* @param NewCombatController 
* @param NewTether 
*/
void AENM_BasicKnight::InitializeAndRun(AENM_CombatScenarioController *NewCombatController, AENM_TetherPoint* NewTether)
{
    // Assign combat controller
    CombatController = NewCombatController;

    // Update the tether
    UpdateTether(NewTether);

    // Setup Processing
    //GetWorldTimerManager().SetTimer(PromoteTimerHandle, this, &AENM_BasicKnight::ProcessPromote, PROMOTE_TIMER, true, PROMOTE_TIMER);
    StartFire();

    Stunned = false;

    MoveComponent = NewObject<UENM_MoveComponent>(this, UENM_MoveComponent::StaticClass());
    MoveComponent->RegisterComponent();
    MoveComponent->InitMoveComponent(this);

} // InitializeAndRun


/**-------------------------------------------------------------
* @brief Enemy faces the player 
*/
void AENM_BasicKnight::ProcessRotation()
{
    // Locals
    FRotator TargetRotation;

    Pitch = 0.0f;
    if (Engaged)
    {
        ACharacter *Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        if (IsValid(Character))
        {
            TargetRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Character->GetActorLocation());
            SetActorRotation(FRotator(0, TargetRotation.Yaw, TargetRotation.Roll));
            Pitch = TargetRotation.Pitch;
        }
    }

} // ProcessRotation


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::ProcessMovement()
{
    if (!Stunned)
    {
        if (Engaged)
        {
            if (PerformMove)
            {
                if (PositionStale)
                {
                    //EngagementDistance = CalculateEnagementDistance();
                    //PositionStale = !MoveComponent->FindRadial(this, EngagementDistance);
                    PositionStale = !MoveComponent->FindSector(this, 100.0f);
                }
                else
                {
                    KickWatchDog();
                    MoveComponent->MoveToSector(this);
                }
                return;
            }
            return;
        }
        else // !Engaged
        {
            // @todo Patrol()
        }
    }
    else // Stun
    {
    }

} // ProcessMovement


/**-------------------------------------------------------------
* @brief 
*
* @param Result 
*/
void AENM_BasicKnight::MoveCompleted(const FPathFollowingResult& Result)
{
    // Locals
    ACharacter *Character;
    float Distance;
    float Random;

    Super::MoveCompleted(Result);

    if (Result.HasFlag(FPathFollowingResultFlags::Success))
    {
        if (GetWorld()->GetTimeSeconds() > AbilityNextTime)
        {
            AbilityNextTime = GetWorld()->GetTimeSeconds() + UKismetMathLibrary::RandomFloatInRange(4.0f, 6.0f);

            // Occasionally do nothing and continue on our merry way
            Random = UKismetMathLibrary::RandomFloat();
            if (Random < 0.8f)
            {
                Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
                if (IsValid(Character) && UUTL_Aiming::TraceToPlayer(this, GetActorLocation()))
                {
                    Distance = UKismetMathLibrary::VSize(Character->GetActorLocation() - GetActorLocation());
                    PerformMove = false;
                    PositionStale = true;
                    StopFire();

                    // @todo Character flying

                    if (Distance < BASIC_KNIGHT_LUNGE_DISTANCE)
                    {
                        PerformGroundAttack();
                    }
                    else if (Distance >= BASIC_KNIGHT_LUNGE_DISTANCE && Distance < BASIC_KNIGHT_SHOT_DISTANCE)
                    {
                        PerformLunge();
                    }
                    else
                    {
                        PerformPowerShot();
                    }

                    // Success
                    return;
                }
            }
        }

        PerformMove = true;
        PositionStale = true;
    }

} // MoveCompleted


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::HandleStuckEnemy()
{
    // Locals
    FVector Point;
    FVector Out;
    
    Point = GetActorLocation();
    Out = GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(this, Point);

    // @todo this check can probably go away with patrol logic
    if (Engaged && !Stunned)
    {
        // If point cannot be projected to NavMesh or Location is the same after WatchDog
        // has already triggered once, assume we are stuck
        if (Point == Out || Point == WatchDogLocation)
        {
            // Each enemy is assigned a tether when initalized, ask for the tether
            // and warp there, as it will be a known safe location
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Stuck"));
            SetActorLocation(CurrentTether->GetActorLocation());
            PositionStale = true;
            PerformMove = true;
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Not Stuck"));
        }

        WatchDogLocation = Point;
    }

} // HandleStuckEnemy


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::KickWatchDog()
{
    GetWorldTimerManager().SetTimer(WatchDogTimerHandle, this, &AENM_BasicKnight::HandleStuckEnemy, 3.0f, true, 3.0f);

} // KickWatchDog


/**-------------------------------------------------------------
* @brief ProcessTemporary 
*/
void AENM_BasicKnight::ProcessPromote()
{
    // Locals
    ACharacter *Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    float Distance;

    if (IsValid(Character))
    {
        Distance = UKismetMathLibrary::VSize(Character->GetActorLocation() - GetActorLocation());
        Promoted = Distance < 750.0f;

        if (!Promoted)
        {
            // Invalidate temporary so if temporary is set, the
            // TemporaryPoint is reset
            MoveComponent->ClearTemporaryPoint();
        }
        else
        {
        }
    }

} // ProcessTemporary


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::ProcessFire()
{
    // Locals
    //if (Active || Promoted)
    {
        Fire();
    }
    //else
    //{
    //    FireRequest();
    //}

} // ProcessFire


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::StartFire()
{
    GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AENM_BasicKnight::ProcessFire, FIRE_TIMER, true, FIRE_TIMER);

} // StartFire


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::StopFire()
{
    GetWorldTimerManager().ClearTimer(FireTimerHandle);
    GetWorldTimerManager().ClearTimer(BurstFireTimerHandle);

} // StopFire


/**-------------------------------------------------------------
* @brief Fire weapon 
*
* @return 
*/
bool AENM_BasicKnight::Fire()
{
    // Locals
    FScriptDelegate Delegate;
    FVector StartPoint;
    bool Success;

    // Rate of fire
    if (GetWorld()->GetTimeSeconds() > NextTime)
    {
        StartPoint = GetActorLocation();
        StartPoint.Z += 200.0f;

        // Check that enemy can see player
        if (UUTL_Aiming::TraceToPlayer(GetWorld(), StartPoint))
        {
            // Delegate.BindUFunction(this, FName("SpawnProjectile"));
            // EnemyWeapon->GetParryChargeSystem()->OnSystemFinished.AddUnique(Delegate);
            // EnemyWeapon->GetParryChargeSystem()->ActivateSystem();
            
            NextTime = GetWorld()->GetTimeSeconds() + 5.0f;
            BurstProjectilesFired = 0;
            GetWorldTimerManager().SetTimer(BurstFireTimerHandle, this, &AENM_BasicKnight::BurstFire, BASIC_KNIGHT_BURST_INTERVAL, true);
            Success = true;
        }
        else
        {
            Success = false;
        }
    }
    else
    {
        Success = false;
    }

    return Success;

} // Fire


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::BurstFire()
{
    // Locals
    FVector Start;

    if (BurstProjectilesFired < BASIC_KNIGHT_BURST)
    {

        Start = GetActorLocation();

        // Muzzle Flash
        // SpawnTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
        // SpawnTransform.SetScale3D(FVector::OneVector * 1.0f);
        //UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FlashSystem, SpawnTransform);
    
        // Spawn Projectile
        SpawnProjectile(AENM_BaseProjectile::StaticClass(), Start, AENM_BaseProjectile::GetProjectileSpeed());

        // Sound
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), WeaponSound, Start);

        BurstProjectilesFired++;
    }
    else
    {
        GetWorldTimerManager().ClearTimer(BurstFireTimerHandle);
    }
}


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::PerformGroundAttack()
{
    // Locals
    AENM_GroundAttack *Attack;
    ACharacter *Character;
    FVector Direction;

    Character = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (Character && IsValid(Character))
    {
        Direction = Character->GetActorLocation() - GetActorLocation();
        Attack = GetWorld()->SpawnActor<AENM_GroundAttack>(AENM_GroundAttack::StaticClass(), GetActorLocation(), FRotator::ZeroRotator);
        Attack->InitGroundAttack(GetActorLocation(), Direction);
    }
        
    CleanupAfterAction();

} // PerformGroundAttack


/**-------------------------------------------------------------
* @brief @todo actually make powerful 
*/
void AENM_BasicKnight::PerformPowerShot()
{
    // Locals
    FVector Start;

    Start = GetActorLocation();

    // Muzzle Flash
    // SpawnTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
    // SpawnTransform.SetScale3D(FVector::OneVector * 1.0f);
    //UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FlashSystem, SpawnTransform);
    
    // Spawn Projectile
    SpawnProjectile(AENM_BaseProjectile::StaticClass(), Start, AENM_BaseProjectile::GetProjectileSpeed());

    // Sound
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), WeaponSound, Start);

    CleanupAfterAction();

} // PerformPowerShot


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::PerformLunge()
{
    // Locals
    ACharacter *Character;
    FVector Distance;
    FVector Lunge;
    float ZComponent;

    Character = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (Character && IsValid(Character))
    {
        Distance = Character->GetActorLocation() - GetActorLocation();

        // Remove Friction
        GetCharacterMovement()->GroundFriction = 0.0f;
        GetCharacterMovement()->FallingLateralFriction = 0.0f;
        GetCharacterMovement()->GravityScale = (BASIC_KNIGHT_LUNGE_SPEED / UKismetMathLibrary::Abs(GetCharacterMovement()->GetGravityZ())) * 1.5f;

        // Determine Lunge Vector
        ZComponent = ((UKismetMathLibrary::VSize(Distance) / BASIC_KNIGHT_LUNGE_SPEED) / 2.0f) * GetCharacterMovement()->GetGravityZ();
        ZComponent = UKismetMathLibrary::Abs(ZComponent);
        Lunge = UKismetMathLibrary::Normal(Distance);
        Lunge.X *= BASIC_KNIGHT_LUNGE_SPEED;
        Lunge.Y *= BASIC_KNIGHT_LUNGE_SPEED;
        Lunge.Z = ZComponent;

        // Launch
        LaunchCharacter(Lunge, true, true);
    }
    else
    {
        CleanupAfterAction();
    }

} // PerformLunge


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::StopLunge()
{
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->FallingLateralFriction = 0.0f;
    GetCharacterMovement()->GravityScale = 1.0f;
    GetCharacterMovement()->Velocity = FVector::ZeroVector;

} // StopLunge


/**-------------------------------------------------------------
* @brief OnLanded 
*
* @param Hit 
*/
void AENM_BasicKnight::Landed(const FHitResult& Hit)
{
    StopLunge();
    Knockback();
    CleanupAfterAction();

} // OnLanded


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::Knockback()
{
        // Locals
    ACHR_Character *Character;
    float Distance;
    FVector Impact;

    Character = Cast<ACHR_Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (Character && IsValid(Character))
    {
        Impact = Character->GetActorLocation() - GetActorLocation();
        Distance = UKismetMathLibrary::VSize(Impact);
        Impact = UKismetMathLibrary::Normal(Impact);
        Impact.Z = 0.5f;

        if (Distance <= KNIGHT_KNOCKBACK_RADIUS)
        {
            Character->AddKnockback(Impact, 2000.0f);
        }
    }

#if KNIGHT_DEBUG
DrawDebugSphere
    (
    GetWorld(), 
    GetActorLocation(),
    KNIGHT_KNOCKBACK_RADIUS, 
    8, 
    FColor::Red, 
    false, 
    5.0f,
    '\000', 
    3.0f
    );
#endif

} // Knockback


/**-------------------------------------------------------------
* @brief 
*/
void AENM_BasicKnight::CleanupAfterAction()
{
    PerformMove = true;
    StartFire();
}


/**-------------------------------------------------------------
* @brief 
*
* @return 
*/
float AENM_BasicKnight::CalculateEnagementDistance()
{
    float Random;

    Random = UKismetMathLibrary::RandomFloat();
    if (Random < 0.4f)
    {
        // Shot
        return BASIC_KNIGHT_SHOT_DISTANCE + 500.0f;
    }
    else if (Random >= 0.4f && Random < 0.8f)
    {
        // Ground attack
        return UKismetMathLibrary::RandomFloatInRange(1000.0f, BASIC_KNIGHT_LUNGE_DISTANCE);
    }
    else
    {
        // Lunge
        return UKismetMathLibrary::RandomFloatInRange(BASIC_KNIGHT_LUNGE_DISTANCE, BASIC_KNIGHT_SHOT_DISTANCE);
    }

} // CalculateEnagementDistance


/**-------------------------------------------------------------
* @brief Checks to see if component hit was critical 
*
* @param Component 
*
* @return 
*/
bool AENM_BasicKnight::IsCriticalComponent(UPrimitiveComponent *Component)
{
    return (Component == HeadMesh);

} // IsCriticalComponent


