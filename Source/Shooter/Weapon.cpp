// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "ShooterCharacter.h"

AWeapon::AWeapon():
ThrowWeaponTime(0.7f),
bFalling(false),
Ammo(30),
MagazineCapacity(30),
WeaponType(EWeaponType::EWT_SubmachineGun),
AmmoType(EAmmoType::EAT_9mm),
ReloadMontageSection(FName(TEXT("Reload SMG"))),
ClipBoneName(FName(TEXT("smg_clip")))
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation { 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if(BoneToHide != FName(""))
	{
		GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
	}
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation { 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward { GetItemMesh()->GetForwardVector() };
	const FVector MeshRight { GetItemMesh()->GetRightVector() };
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	float RandomRotation { 30.f };
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseDirection *= 10'000.f;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);
	EnableGlowMaterial();
	GetWorldTimerManager().SetTimer(ScaleChangeTimer, this, &AWeapon::ScaleChange, 0.8f);
}

void AWeapon::DecrementAmmo()
{
	if(Ammo - 1 <= 0)
	{
		Ammo = 0;
	}
	else
	{
		--Ammo;
	}
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attemped to reload with more than magazine capacity!"));
	Ammo += Amount;
}

bool AWeapon::ClipIsFull()
{
	return Ammo >= MagazineCapacity;
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_PickUp);
	StartPulseTimer();
}

void AWeapon::ScaleChange()
{
	SetActorScale3D(FVector(2.f));
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const FString WeaponTablePath { TEXT("DataTable'/Game/_Game/DataTable/WeaponDataTable.WeaponDataTable'") };
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));

	if(WeaponTableObject)
	{
		FWeaponDataTable* WeaponDataRow = nullptr;
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EWT_AssaultRifle:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
			break;
		case EWeaponType::EWT_Pistol:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
			break;
		}
		if(WeaponDataRow)
		{
			AmmoType = WeaponDataRow->AmmoType;
			Ammo = WeaponDataRow->WeaponAmmo;
			MagazineCapacity = WeaponDataRow->MagazingCapacity;
			SetPickUpSound(WeaponDataRow->PickUpSound);
			SetEquipSound(WeaponDataRow->EquipSound);
			GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
			SetItemName(WeaponDataRow->ItemName);
			SetIconItem(WeaponDataRow->InventoryIcon);
			SetAmmoIcon(WeaponDataRow->AmmoIcon);
			SetMaterialInstance(WeaponDataRow->MaterialInstance);
			PreviousMaterialIndex = GetMaterialIndex();
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
			SetMaterialIndex(WeaponDataRow->MaterialIndex);
			SetClipBoneName(WeaponDataRow->ClipBoneName);
			SetReloadMontageSection(WeaponDataRow->ReloadMontageSection);
			GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);
			CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
			CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
			CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
			CrosshairsRight = WeaponDataRow->CrosshairsRight;
			CrosshairsTop = WeaponDataRow->CrosshairsTop;
			AutoFireRate = WeaponDataRow->AutoFireRate;
			MuzzleFlash = WeaponDataRow->MuzzleFlash;
			FireSound = WeaponDataRow->FireSound;
			BoneToHide = WeaponDataRow->BoneToHide;
		}
		if(GetMaterialInstance())
		{
			SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
			GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
			GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
			EnableGlowMaterial();
		}
	}
}
