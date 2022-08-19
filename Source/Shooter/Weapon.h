

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Weapon.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AWeapon : public AItem
{
	GENERATED_BODY()
public:
	AWeapon();

	virtual void Tick(float DeltaSeconds) override;
protected:
	void StopFalling();

private:
	FTimerHandle ThrowWeaponTimer;

	float ThrowWeaponTime;

	bool bFalling;

public:
	void ThrowWeapon();
};
