/**************************************************************
 
    Module: WPN_BaseProjectile.cpp
 
    Copyright 2018 Tyler Graveline
 
**************************************************************/

/**************************************************************
                    GENERAL INCLUDES
**************************************************************/
#include "WPN_BaseProjectile.h"

#include "ASS_Assets.h"
#include "ENM/ENM_BaseEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstance.h"
#include "TimerManager.h"
#include "UI/WIG/HUD/UI_DamageNumber.h"

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

/**************************************************************
                    VARIABLES
**************************************************************/

/**************************************************************
                    PROCEDURES
**************************************************************/

/**********************************************
AWPN_BaseProjectile

Sets default values
**********************************************/
AWPN_BaseProjectile::AWPN_BaseProjectile()
{
    // Locals
    UMaterialInstance *Material;

	// Tick OFF
    PrimaryActorTick.bCanEverTick = false;

    // Create mesh
    Mesh->SetStaticMesh(ASS_LoadAsset<UStaticMesh>(LP_ICO_SPHERE));

    // Create Material
    Material = ASS_LoadAsset<UMaterialInstance>(MAT_PROJECTILE_YELLOW);
    Mesh->SetMaterialByName(FName("BaseSlot"), Material);

    // Create trail
    TrailSystem->SetTemplate(ASS_LoadAsset<UParticleSystem>(SYS_TRAIL_YELLOW));
    TrailSystem->SetupAttachment(Mesh);

    // Create hit effects
    HitSystems[HIT_BLANK]  = ASS_LoadAsset<UParticleSystem>(SYS_BLANK_HIT);
    HitSystems[HIT_NORMAL] = ASS_LoadAsset<UParticleSystem>(SYS_NORMAL_HIT);
    HitSystems[HIT_CRIT]   = ASS_LoadAsset<UParticleSystem>(SYS_CRIT_HIT);

    // Setup Damage curve
    FallOffFloatCurve = ASS_LoadAsset<UCurveFloat>(BASE_DAMAGE_FLOAT_CURVE);

    // Enable collision
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionProfileName(FName("Projectile"));

    // Set the root component
    RootComponent = Mesh;

} /* AWPN_BaseProjectile() */


/**********************************************
GetObjectDamage


**********************************************/
float AWPN_BaseProjectile::GetObjectDamage()
{
    return (float)(UKismetMathLibrary::FFloor(FallOffFloatCurve->GetFloatValue(UKismetMathLibrary::VSize(GetActorLocation() - StartLocation))));

} /* GetObjectDamage */


/**********************************************
NotifyHit

Called when actor collides with another object
**********************************************/
void AWPN_BaseProjectile::NotifyHit
    (
        class UPrimitiveComponent* MyComp, 
        AActor* Other, 
        class UPrimitiveComponent* OtherComp, 
        bool bSelfMoved, 
        FVector HitLocation, 
        FVector HitNormal, 
        FVector NormalImpulse, 
        const FHitResult& Hit
    )
{
    // Apply point damage
    ApplyProjectileDamage(Other, OtherComp);

    // Spawn Particle
    SpawnHitParticle(Other, OtherComp, HitLocation);
    
    // Hide and disable mesh to give trail particle time to finish
    DisableProjectile();

} /* NotifyHit */


/**********************************************
BeginPlay

Called when the game starts or when spawned
**********************************************/
void AWPN_BaseProjectile::BeginPlay()
{
	Super::BeginPlay();
    StartLocation = GetActorLocation();
	
} /* BeginPlay */


/**********************************************
ApplyProjectileDamage


**********************************************/
float AWPN_BaseProjectile::ApplyProjectileDamage(AActor *Other, class UPrimitiveComponent* OtherComp)
{
    //Locals
    float AppliedDamage = 0.0f;
    float Damage;
    bool Critical;

    // Check if this was an enemy
    if (IsValid(Other) && Other->ActorHasTag(ENEMY_TAG))
    {
        // Lookup base damage
        Damage = GetObjectDamage();

        // Was this a critical hit
        Critical = ((AENM_BaseEnemy *)Other)->IsCriticalComponent(OtherComp);
        if (Critical)
        {
            Damage = Damage * 2.0f;
        }

        // Apply the damage
        AppliedDamage = UGameplayStatics::ApplyDamage
                            (
                            Other,
                            Damage,
                            NULL,
                            this,
                            NULL
                            );

        // Show Damage on Screen
        ShowDamageNumber(AppliedDamage, Critical);
    }

    return AppliedDamage;

} /* ApplyProjectileDamage */


/**********************************************
ApplyRadialProjectileDamage


**********************************************/
float AWPN_BaseProjectile::ApplyRadialProjectileDamage(AActor *Other, class UPrimitiveComponent* OtherComp)
{
    // TODO
    return 0.0f;
}


/**********************************************
ShowDamageNumber


**********************************************/
void AWPN_BaseProjectile::ShowDamageNumber(float AppliedDamage, bool Critical)
{
    // Locals
    UClass *ClassReference;
    APlayerController *Controller;
    int IntegerDamage;
    FVector2D ScreenLocation;
    FStringClassReference StringReference(UI_DAMAGE);

    // Display damage numbers on screen
    Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (IsValid(Controller))
    {
        // Add damage to screen
        ClassReference = StringReference.TryLoadClass<UUI_DamageNumber>();
        if (ClassReference != nullptr)
        {
            UUI_DamageNumber *Number = CreateWidget<UUI_DamageNumber>(Controller, ClassReference);
            if (IsValid(Number))
            {
                IntegerDamage = FMath::Clamp(int(AppliedDamage), 0, 999999);
                UGameplayStatics::ProjectWorldToScreen(Controller, GetActorLocation(), ScreenLocation);
                Number->InitDamageNumber(ScreenLocation, IntegerDamage, Critical);
                Number->AddToViewport(0);
                Number->SetVisibility(ESlateVisibility::Visible);
                Number->AnimateIn();
            }
        }
    }

} /* ShowDamageNumber */


/**********************************************
SpawnHitParticle


**********************************************/
void AWPN_BaseProjectile::SpawnHitParticle(AActor *Other, class UPrimitiveComponent* OtherComp, FVector Location)
{
    //Locals
    bool Critical;
    HitEffectsType Effect;
    FTransform SpawnTransform;

    // Check if this was an enemy
    if (IsValid(Other) && Other->ActorHasTag(ENEMY_TAG))
    {
        // Determine if the hit was a critical hit
        Critical = ((AENM_BaseEnemy *)Other)->IsCriticalComponent(OtherComp);
        if (Critical)
        {
            Effect = HIT_CRIT;
        }
        else
        {
            Effect = HIT_NORMAL;
        }
    }
    else
    {
        // Hit some random object
        Effect = HIT_BLANK;
    }

    // Spawn the particle
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FRotator::ZeroRotator.Quaternion()); 
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitSystems[Effect], SpawnTransform);

} /* SpawnHitParticle */
