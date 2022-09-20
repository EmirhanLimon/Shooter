// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"

#include "Ammo.h"
#include "Item.h"
#include "DrawDebugHelpers.h"
#include "Weapon.h"
#include "Sound/SoundCue.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter() :
BaseTurnRate(45.f),
BaseLookUpRate(45.f),
bAiming(false),
CameraDefaultFOV(0.f),
CameraZoomedFOV(25.f),
CameraCurrentFOV(0.f),
ZoomInterpSpeed(20.f),
HipTurnRate(90.f),
HipLookUpRate(90.f),
AimingTurnRate(20.f),
AimingLookUpRate(20.f),
MouseHipTurnRate(0.6f),
MouseHipLookUpRate(0.6f),
MouseAimingTurnRate(0.2f),
MouseAimingLookUpRate(0.2f),
CrossHairSpreadMultiplier(0.f),
CrossHairVelocityFactor(0.f),
CrossHairInAirFactor(0.f),
CrossHairAimFactor(0.f),
CrossHairShootingFactor(0.f),
ShootTimeDuration(0.05f),
bFiringBullet(false),
bShouldFire(true),
bFireButtonPressed(false),
bShouldTraceForItems(false),
CameraInterpDistance(250.f),
CameraInterpElevation(65.f),
Starting9mmAmmo(85),
StartingARAmmo(120),
CombatState(ECombatState::ECS_Unoccupied),
bCrouching(false),
BaseMovementSpeed(650),
CrouchMovementSpeed(300),
StandingCapsuleHalfHeight(88),
CrouchingCapsuleHalfHeight(44),
BaseGroundFriction(2.f),
CrouchingGroundFriction(100.f),
bAimingButtonPressed(false),
bShouldPlayPickUpSound(true),
bShouldPlayEquipSound(true),
PickUpSoundResetTime(0.2f),
EquipSoundResetTime(0.2f),
HighlightedSlot(-1)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(GetFollowCamera());

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(GetFollowCamera());

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(GetFollowCamera());

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(GetFollowCamera());

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(GetFollowCamera());

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

	

}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}	
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();
	EquippedWeapon->SetCharacter(this);
	InitializeAmmoMap();
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	InitializeInterpLocations();

}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CameraInterpZoom(DeltaTime);
	SetLookRates();
	CalculateCrossHairSpread(DeltaTime);
	TraceForItems();
	InterpCapsuleHalfHeight(DeltaTime);
	
}
// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("EKey", IE_Pressed, this, &AShooterCharacter::EKeyPressed);
	PlayerInputComponent->BindAction("1Key", IE_Pressed, this, &AShooterCharacter::OneKeypressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this, &AShooterCharacter::TwoKeypressed);
	PlayerInputComponent->BindAction("3Key", IE_Pressed, this, &AShooterCharacter::ThreeKeypressed);
	PlayerInputComponent->BindAction("4Key", IE_Pressed, this, &AShooterCharacter::FourKeypressed);
	PlayerInputComponent->BindAction("5Key", IE_Pressed, this, &AShooterCharacter::FiveKeypressed);
}

void AShooterCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickUpSound = true;
}

void AShooterCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}

float AShooterCharacter::GetCrossHairSpreadMultiplier() const
{
	return CrossHairSpreadMultiplier;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if(OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AShooterCharacter::MoveForward(float Value)
{
	if(Controller != nullptr && Value != 0.0f)
	{
		const FRotator Rotation { Controller->GetControlRotation() };
		const FRotator YawRotation { 0, Rotation.Yaw, 0};
		const FVector Direction { FRotationMatrix { YawRotation }.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);	
	}
	
}

void AShooterCharacter::MoveRight(float Value)
{
	if(Controller != nullptr && Value != 0.0f)
	{
		const FRotator Rotation { Controller->GetControlRotation() };
		const FRotator YawRotation { 0, Rotation.Yaw, 0};
		const FVector Direction { FRotationMatrix { YawRotation }.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);	
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate* BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::FireWeapon()
{
	if(EquippedWeapon == nullptr) return;
	if(CombatState != ECombatState::ECS_Unoccupied) return;	
	if(WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunfireMontage();
		EquippedWeapon->DecrementAmmo();
		StartFireTimer();
	}
	StartCrossHairBulletFire();
	
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	FHitResult CrossHairHitResult;
	bool bCrossHairHit = TraceUnderCrossHair(CrossHairHitResult, OutBeamLocation);
	if(bCrossHairHit)
	{
		OutBeamLocation = CrossHairHitResult.Location;
	}
	else
	{
		
	}
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart { MuzzleSocketLocation };
	const FVector WeaponTraceEnd { OutBeamLocation };
	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);
			
	if(WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.Location;
		return true;
	}
	return false;
	
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if(CombatState != ECombatState::ECS_Reloading)
	{
		Aim();
	}
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	if(bAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{
	if(bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor{};
	if(bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value* TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if(bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value* LookUpScaleFactor);
}

void AShooterCharacter::CalculateCrossHairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange { 0.f, 600.f };
	FVector2D VelocityMultiplierRange { 0.f, 1.f };
	FVector Velocity { GetVelocity() };
	Velocity.Z = 0.f;

	if(GetCharacterMovement()->IsFalling())
	{
		CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if(bAiming)
	{
		CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.6f, DeltaTime, 30.f);
	}
	else
	{
		CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.f, DeltaTime, 30.f);
	}

	if(bFiringBullet)
	{
		CrossHairShootingFactor = FMath::FInterpTo(CrossHairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrossHairShootingFactor = FMath::FInterpTo(CrossHairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrossHairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
	CrossHairSpreadMultiplier = 0.5f + CrossHairVelocityFactor + CrossHairInAirFactor - CrossHairAimFactor + CrossHairShootingFactor;
}

void AShooterCharacter::StartCrossHairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrossHairShootTimer, this, &AShooterCharacter::FinishCrossHairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
	
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	if(EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, EquippedWeapon->GetAutoFireRate());
	
}

void AShooterCharacter::AutoFireReset()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if(WeaponHasAmmo())
	{
		if(bFireButtonPressed)
		{
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

bool AShooterCharacter::TraceUnderCrossHair(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrossHairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
			CrossHairLocation,CrossHairWorldPosition, CrossHairWorldDirection);

	if(bScreenToWorld)
	{
		const FVector Start { CrossHairWorldPosition };
		const FVector End { Start + CrossHairWorldDirection * 50'000.f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if(OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}
	
	return false;
}

void AShooterCharacter::TraceForItems()
{
	if(bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrossHair(ItemTraceResult, HitLocation);
		if(ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.Actor);
			const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if(TraceHitWeapon)
			{
				if(HighlightedSlot == -1)
				{
					HighlightInventorySlot();
				}
			}
			else
			{
				if(HighlightedSlot != -1)
				{
					UnHighlightInventorySlot();
				}
			}
			
			if(TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}
			if(TraceHitItem && TraceHitItem->GetPickUpWidget())
			{
				TraceHitItem->GetPickUpWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();
				if(Inventory.Num() >= INVENTORY_CAPACITY)
				{
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetCharacterInventoryFull(false);
				}
			}
			if(TraceHitItemLastFrame)
			{
				if(TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickUpWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if(TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickUpWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if(DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	if(WeaponToEquip)
	{
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if(HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		if(EquippedWeapon == nullptr)
		{
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}
		EquippedWeapon = WeaponToEquip;
        EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon()
{
	if(EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::SelectButtonPressed()
{
	if(CombatState != ECombatState::ECS_Unoccupied) return;
	if(TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
		TraceHitItem = nullptr;
	}
	
}

void AShooterCharacter::SelectButtonReleased()
{
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if(Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	DropWeapon();
	EquipWeapon(WeaponToSwap, true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if(EquippedWeapon == nullptr) return false;

	return EquippedWeapon->GetAmmo() > 0;
}

void AShooterCharacter::PlayFireSound()
{
	if(EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AShooterCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	
	if(BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		
		if(EquippedWeapon->GetMuzzleFlash())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}
		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);
		if(bBeamEnd)
		{
			if(ImpactParticle)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, BeamEnd);
			}
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticle, SocketTransform);
			if(Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}	
}

void AShooterCharacter::PlayGunfireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	
	if(CombatState != ECombatState::ECS_Unoccupied) return;
	if(EquippedWeapon == nullptr) return;
	if(CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if(bAiming)
		{
			StopAiming();
		}
		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ReloadMontage)
		{
			GEngine->AddOnScreenDebugMessage(-1,1.f,FColor::Red,TEXT("sad"));
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

void AShooterCharacter::FinishReloading()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if(bAimingButtonPressed)
	{
		Aim();
	}

	if(EquippedWeapon == nullptr) return;

	const auto AmmoType { EquippedWeapon->GetAmmoType() };

	if(AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];

		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if(MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AShooterCharacter::FinishEquipping()
{
	CombatState = ECombatState::ECS_Unoccupied;
}

bool AShooterCharacter::CarryingAmmo()
{
	if(EquippedWeapon == nullptr) return false;

	auto AmmoType = EquippedWeapon->GetAmmoType();
	if(AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AShooterCharacter::GrabClip()
{
	if(EquippedWeapon == nullptr) return;
	if(HandSceneComponent == nullptr) return;
	int32 ClipBoneIndex { EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);
	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

void AShooterCharacter::CrouchButtonPressed()
{
	if(!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}
	if(bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::Jump()
{
	if(bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		ACharacter::Jump();
	}
}

void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight {};
	if(bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}
	const float InterpHalfHeight { FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHalfHeight, DeltaTime, 20.f)};
	const float DeltaCapsuleHalfHeight { InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()};
	const FVector MeshOffset { 0.f, 0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AShooterCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;
	if(!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AShooterCharacter::PickUpAmmo(AAmmo* Ammo)
{
	if(AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount { AmmoMap[Ammo->GetAmmoType()] };
        AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if(EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if(EquippedWeapon->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}
	Ammo->Destroy();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation { WeaponInterpComp, 0};
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1 { InterpComp1, 0};
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2 { InterpComp2, 0};
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3 { InterpComp3, 0};
	InterpLocations.Add(InterpLoc3);

	FInterpLocation InterpLoc4 { InterpComp4, 0};
	InterpLocations.Add(InterpLoc4);

	FInterpLocation InterpLoc5 { InterpComp5, 0};
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6 { InterpComp6, 0};
	InterpLocations.Add(InterpLoc6);
}

void AShooterCharacter::EKeyPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeypressed()
{
	if(EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeypressed()
{
	if(EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeypressed()
{
	if(EquippedWeapon->GetSlotIndex() == 3) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeypressed()
{
	if(EquippedWeapon->GetSlotIndex() == 4) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeypressed()
{
	if(EquippedWeapon->GetSlotIndex() == 5) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
	const bool bCanExchangeItems = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
			(CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Equipping);
	if(bCanExchangeItems)
	{
		auto OldEquippedWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);
	
		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);

		CombatState = ECombatState::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection(FName("Equip"));
		}
		NewWeapon->PlayEquipSound(true);
	}
	
}

int32 AShooterCharacter::GetEmptyInventorySlot()
{
	for(int32 i = 0; i < Inventory.Num(); i++)
	{
		if(Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if(Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}
	return -1;
}

void AShooterCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot { GetEmptyInventorySlot() };
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

void AShooterCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for(int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if(InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if(Amount < -1 || Amount > 1) return;

	if(InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::StartPickUpSoundTimer()
{
	bShouldPlayPickUpSound = false;
	GetWorldTimerManager().SetTimer(PickUpSoundTimer,this, &AShooterCharacter::ResetPickupSoundTimer, PickUpSoundResetTime);
}

void AShooterCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(EquipSoundTimer,this, &AShooterCharacter::ResetEquipSoundTimer, EquipSoundResetTime);
}

void AShooterCharacter::FinishCrossHairBulletFire()
{
	bFiringBullet = false;
}

/*FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation { FollowCamera->GetComponentLocation() };
	const FVector CameraForward { FollowCamera->GetForwardVector() };

	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);
}*/

void AShooterCharacter::GetPickUpItem(AItem* Item)
{
	Item->PlayEquipSound();
	auto Weapon = Cast<AWeapon>(Item);
	if(Weapon)
	{
		if(Inventory.Num() < INVENTORY_CAPACITY)
		{
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else
		{
			SwapWeapon(Weapon);
		}
		
	}
	auto Ammo = Cast<AAmmo>(Item);
	if(Ammo)
	{
		PickUpAmmo(Ammo);
	}
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
	if(Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

// Called every frame




