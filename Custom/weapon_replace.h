
#ifndef NPCS_IN_CSS_WEAPON_REPLACE_H
#define NPCS_IN_CSS_WEAPON_REPLACE_H

class CCombatWeapon;

#define WEAPON_SMG1_REPLACE			tf_weapon_syringegun_medic
#define WEAPON_AR2_REPLACE			tf_weapon_charged_smg
#define WEAPON_SHOTGUN_REPLACE		tf_weapon_sentry_revenge
#define WEAPON_RPG_REPLACE			tf_weapon_rocketlauncher_fireball
#define WEAPON_STUNSTICK_REPLACE	tf_weapon_breakable_sign
#define WEAPON_ALYXGUN_REPLACE		tf_weapon_pep_brawler_blaster
#define WEAPON_PISTOL_REPLACE		tf_weapon_raygun

#define WEAPON_SMG1_REPLACE_NAME		"tf_weapon_syringegun_medic"
#define WEAPON_AR2_REPLACE_NAME			"tf_weapon_charged_smg"
#define WEAPON_SHOTGUN_REPLACE_NAME		"tf_weapon_sentry_revenge"
#define WEAPON_RPG_REPLACE_NAME			"tf_weapon_rocketlauncher_fireball"
#define WEAPON_STUNSTICK_REPLACE_NAME	"tf_weapon_breakable_sign"
#define WEAPON_ALYXGUN_REPLACE_NAME		"tf_weapon_pep_brawler_blaster"
#define WEAPON_PISTOL_REPLACE_NAME		"tf_weapon_raygun"

const char *GetWeaponReplaceName(const char *name);
const char *NPC_WeaponReplace(const char *classname);

void PreWeaponReplace(CCombatWeapon *pWeapon);
void PreWeaponReplace(const char *name);

void PostWeaponReplace();

#endif //NPCS_IN_CSS_WEAPON_REPLACE_H
