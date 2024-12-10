

#ifndef WEAPON_PISTOL_H
#define WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "CNPCBaseWeapon.h"


class CNPCWeapon_Pistol : public CNPCBaseWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_Pistol, CNPCBaseWeapon );

	CNPCWeapon_Pistol();
	void Spawn();
	void Precache();
	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void OnNPCEquip(CCombatCharacter *owner);
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	int		GetMinBurst( void );
	int		GetMaxBurst( void );
	float	GetFireRate( void );
	const Vector& GetBulletSpread( void );
	int		Weapon_CapabilitiesGet( void );

private:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;

	static acttable_t m_acttable[];

};

class CNPCWeapon_AlyxGun : public CNPCWeapon_Pistol
{
public:
	DECLARE_CLASS( CNPCWeapon_AlyxGun, CNPCWeapon_Pistol );

	void Spawn()
	{
		BaseClass::Spawn();
		m_iWeaponModel = PrecacheModel("models/weapons/W_Alyx_Gun.mdl");
	}

	const char *NPCWeaponGetWorldModel() { return "models/weapons/W_Alyx_Gun.mdl"; }
};


#endif