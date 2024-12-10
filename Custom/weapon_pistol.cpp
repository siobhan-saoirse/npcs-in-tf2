#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "weapon_pistol.h"

#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

ConVar	pistol_use_new_accuracy( "pistol_use_new_accuracy", "1" );


static CEntityFactory_CE<CNPCWeapon_Pistol> WEAPON_PISTOL_REPLACE(WEAPON_PISTOL_REPLACE_NAME);
static CEntityFactory_CE<CNPCWeapon_AlyxGun> WEAPON_ALYXGUN_REPLACE(WEAPON_ALYXGUN_REPLACE_NAME);

acttable_t	CNPCWeapon_Pistol::m_acttable[] =
		{
			{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
			{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
			{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
			{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
			{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
			{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
			{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			true },
			{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		true },
			{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
			{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			true },
			{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	true },
			{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		true },

			// Readiness activities (not aiming)
			{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
			{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
			{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
			{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

			{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
			{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
			{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
			{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

			{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
			{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
			{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
			{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

			// Readiness activities (aiming)
			{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
			{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
			{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
			{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

			{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
			{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
			{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
			{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

			{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
			{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
			{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
			{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
			//End readiness activities

			// Crouch activities
			{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
			{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
			{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

			// Readiness translations
			{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
			{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
			{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
			{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },


		//	{ ACT_ARM,				ACT_ARM_PISTOL,					true },
		//	{ ACT_DISARM,			ACT_DISARM_PISTOL,				true },
		};


CNPCWeapon_Pistol::CNPCWeapon_Pistol()
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;
}

void CNPCWeapon_Pistol::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/w_pistol.mdl");
	BaseClass::Spawn();
}

void CNPCWeapon_Pistol::Precache()
{
	PrecacheScriptSound("Weapon_Pistol.Reload");
	PrecacheScriptSound("Weapon_Pistol.NPC_Reload");
	PrecacheScriptSound("Weapon_Pistol.Empty");
	PrecacheScriptSound("Weapon_Pistol.Single");
	PrecacheScriptSound("Weapon_Pistol.NPC_Single");
	PrecacheScriptSound("Weapon_Pistol.Special1");
	PrecacheScriptSound("Weapon_Pistol.Special2");
	PrecacheScriptSound("Weapon_Pistol.Burst");

	BaseClass::Precache();
}

const char *CNPCWeapon_Pistol::NPCWeaponGetWorldModel() const
{
	return "models/weapons/w_pistol.mdl";
}

acttable_t*	CNPCWeapon_Pistol::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_Pistol::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

void CNPCWeapon_Pistol::OnNPCEquip(CCombatCharacter *owner)
{
	m_fMinRange1 = 24;
	m_fMaxRange1 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;
}

void CNPCWeapon_Pistol::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOperator);

			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = owner->Weapon_ShootPosition();

			CAI_NPC *npc = owner->MyNPCPointer();
			// Assert( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, owner->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, owner->CB_GetEnemy() );

			CustomWeaponSound(owner->entindex(), vecShootOrigin, "Weapon_Pistol.NPC_Single");

			owner->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0, 12.0f );
			owner->DoMuzzleFlash();
			m_iClip1 = *(m_iClip1) - 1;
		}
			break;
		default:
			CCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

void CNPCWeapon_Pistol::NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{

}

const WeaponProficiencyInfo_t *CNPCWeapon_Pistol::NPCWeaponGetProficiencyValues()
{
	return NULL;
}

int CNPCWeapon_Pistol::GetMinBurst( void )
{
	if(IsNPCUsing())
	{
		return 1;
	} else
		return BaseClass::GetMinBurst();
}

int CNPCWeapon_Pistol::GetMaxBurst( void )
{
	if(IsNPCUsing())
	{
		return 3;
	} else
		return BaseClass::GetMaxBurst();
}

float CNPCWeapon_Pistol::GetFireRate( void )
{
	if(IsNPCUsing())
	{
		return 0.05f;
	} else
		return BaseClass::GetFireRate();
}

const Vector& CNPCWeapon_Pistol::GetBulletSpread( void )
{
	if(!IsNPCUsing())
		return BaseClass::GetBulletSpread();

	static Vector npcCone = VECTOR_CONE_5DEGREES;
	if ( GetOwner() && GetOwner()->IsNPC() )
		return npcCone;

	static Vector cone;

	if ( pistol_use_new_accuracy.GetBool() )
	{
		float ramp = RemapValClamped(	m_flAccuracyPenalty,
										 0.0f,
										 PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME,
										 0.0f,
										 1.0f );

		// We lerp from very accurate to inaccurate over time
		VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );
	}
	else
	{
		// Old value
		cone = VECTOR_CONE_4DEGREES;
	}

	return cone;
}

int	CNPCWeapon_Pistol::Weapon_CapabilitiesGet( void )
{
	if(IsNPCUsing())
	{
		return bits_CAP_WEAPON_RANGE_ATTACK1;
	} else
		return BaseClass::Weapon_CapabilitiesGet();
}
