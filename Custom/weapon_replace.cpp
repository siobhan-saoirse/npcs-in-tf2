#include "weapon_replace.h"
#include "CCombatWeapon.h"
#include "weapon_rpg_replace.h"
#include "weapon_physcannon_replace.h"
#include "sm_stringhashmap.h"

class WeaponReplaceSystem : public CBaseGameSystem
{
public:
	WeaponReplaceSystem(const char *name): CBaseGameSystem(name)
	{
	}
	const char *GetReplaceName(const char *name)
	{
		auto result = m_pWeaponReplaceData.find(name);
		if (result.found())
			return result->value;
		return nullptr;
	}

	void LevelInitPreEntity()
	{
		AddReplace("weapon_smg1","tf_weapon_syringegun_medic");
		AddReplace("weapon_annabelle","tf_weapon_shotgun_hwg");
		AddReplace("weapon_shotgun","tf_weapon_sentry_revenge");
		AddReplace("weapon_ar2","tf_weapon_charged_smg");
		AddReplace("weapon_357", "tf_weapon_revolver"); 
		AddReplace("weapon_crossbow","tf_weapon_jar_gas");
		AddReplace("weapon_rpg","tf_weapon_rocketlauncher_fireball");
		AddReplace("weapon_slam","tf_weapon_stickbomb");
		AddReplace("weapon_frag","tf_weapon_jar");
		AddReplace("weapon_crowbar","tf_weapon_cleaver");
		AddReplace("weapon_pistol","tf_weapon_raygun");
		AddReplace("weapon_physcannon","tf_weapon_mechanical_arm");
		AddReplace("weapon_alyxgun","tf_weapon_pep_brawler_blaster");
	}

	void LevelShutdownPostEntity()
	{
		m_pWeaponReplaceData.clear();
	}

	void SDKInitPost()
	{
		LevelInitPreEntity();
	}

	void SDKShutdown()
	{
		m_pWeaponReplaceData.clear();
	}

private:
	void AddReplace(const char *name_to_replace, const char *replace_name)
	{
		m_pWeaponReplaceData.replace(name_to_replace, replace_name);
	}

private:
	StringHashMap<const char*> m_pWeaponReplaceData;
};

WeaponReplaceSystem g_WeaponReplaceSystem("WeaponReplaceSystem");

const char *GetWeaponReplaceName(const char *name)
{
	return g_WeaponReplaceSystem.GetReplaceName(name);
}

const char *NPC_WeaponReplace(const char *szValue)
{
	const char *newszValue = szValue;
	if(FStrEq(szValue, "weapon_ar2"))
		newszValue = WEAPON_AR2_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_shotgun"))
		newszValue = WEAPON_SHOTGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_annabelle"))
		newszValue = WEAPON_ANNABELLE_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_smg1"))
		newszValue = WEAPON_SMG1_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_stunstick"))
		newszValue = WEAPON_STUNSTICK_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_alyxgun"))
		newszValue = WEAPON_ALYXGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_pistol"))
		newszValue = WEAPON_PISTOL_REPLACE_NAME;
	else
		META_CONPRINTF("%s: invalid classname to replace '%s'\n", __func__, szValue);
	//newszValue = WEAPON_SMG1_REPLACE_NAME;
	return newszValue;
}

void PreWeaponReplace(CCombatWeapon *pWeapon)
{
	CWeaponRPG *rpg = ToCWeaponRPG(pWeapon);
	if(rpg)
	{
		CWeaponRPG::IS_REPLACE_SPAWN = true;
		return;
	}
	CWeaponPhysCannon *physcannon = ToCWeaponPhysCannon(pWeapon);
	if(physcannon)
	{
		CWeaponPhysCannon::IS_REPLACE_SPAWN = true;
		return;
	}
}

void PreWeaponReplace(const char *name)
{
	if (!Q_stricmp( "weapon_rpg", name) )
	{
		CWeaponRPG::IS_REPLACE_SPAWN = true;
	} else if (!Q_stricmp( "weapon_physcannon", name) ) {
		CWeaponPhysCannon::IS_REPLACE_SPAWN = true;
	}
}

void PostWeaponReplace()
{
	CWeaponRPG::IS_REPLACE_SPAWN = false;
	CWeaponPhysCannon::IS_REPLACE_SPAWN = false;
}