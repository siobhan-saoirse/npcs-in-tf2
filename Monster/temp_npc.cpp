
#include "CEntity.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"
#include "fmtstr.h"
#include "CAI_utils.h"
#include "CCombatWeapon.h"
#include "CPropDoor.h"


static const char *g_ppszRandomHeads[] = 
{
	"male_01.mdl",
	"male_02.mdl",
	"female_01.mdl",
	"male_03.mdl",
	"female_02.mdl",
	"male_04.mdl",
	"female_03.mdl",
	"male_05.mdl",
	"female_04.mdl",
	"male_06.mdl",
	"female_06.mdl",
	"male_07.mdl",
	"female_07.mdl",
	"male_08.mdl",
	"male_09.mdl",
};

static const char *g_ppszModelLocs[] =
{
	"Group01",
	"Group01",
	"Group02",
	"Group03%s",
};

#define IsExcludedHead( type, bMedic, iHead) false // see XBox codeline for an implementation

abstract_class CE_Temp_NPC : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS(CE_Temp_NPC, CE_Cycler_Fix);

	virtual void Spawn()
	{
		Precache();
		BaseClass::Spawn();
		m_iHealth = 100;
		SetModel(STRING(GetModelName()));


		//	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB);
		// JAY: Disabled jump for now - hard to compare to HL1
		CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND );
		CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
		CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT );
		CapabilitiesAdd( bits_CAP_DUCK | bits_CAP_DOORS_GROUP );
		CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );

		CapabilitiesAdd( bits_CAP_AIM_GUN );
		CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE );

		// Innate range attack for grenade
		// CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK2 );

		// Innate range attack for kicking
		//CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1 );

		// Can be in a squad
		CapabilitiesAdd( bits_CAP_SQUAD);
		CapabilitiesAdd( bits_CAP_USE_WEAPONS );

		CapabilitiesAdd( bits_CAP_DUCK );				// In reloading and cover

		CapabilitiesAdd( bits_CAP_NO_HIT_SQUADMATES );

		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_STEP );
		SetBloodColor( BLOOD_COLOR_RED );

		SetImpactEnergyScale( 0.0f ); 
		
		NPCInit();
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}

	bool HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt);

	// Tasks
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual void BuildScheduleTestBits( );
 
	// Navigation
	bool OnMoveBlocked( AIMoveResult_t *pResult );

	// Damage
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	// Various start tasks
	virtual	void StartTaskRangeAttack1( const Task_t *pTask );

	// Various run tasks
	virtual void RunTaskRangeAttack1( const Task_t *pTask );

	// Purpose: check ammo
	virtual void CheckAmmo( void );
};



void CE_Temp_NPC::CheckAmmo( void )
{
	BaseClass::CheckAmmo();
	
	if (!GetActiveWeapon())
		return;

	// Don't do this while holstering / unholstering
	if ( IsWeaponStateChanging() )
		return;

	if (GetActiveWeapon()->UsesPrimaryAmmo())
	{
		if (!GetActiveWeapon()->HasPrimaryAmmo() )
		{
			SetCondition(COND_NO_PRIMARY_AMMO);
		}
		else if (GetActiveWeapon()->UsesClipsForAmmo1() && GetActiveWeapon()->Clip1() < (GetActiveWeapon()->GetMaxClip1() / 4 + 1))
		{
			// don't check for low ammo if you're near the max range of the weapon
			SetCondition(COND_LOW_PRIMARY_AMMO);
		}
	}

	if (!GetActiveWeapon()->HasSecondaryAmmo() )
	{
		if ( GetActiveWeapon()->UsesClipsForAmmo2() )
		{
			SetCondition(COND_NO_SECONDARY_AMMO);
		}
	}
}


//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1 / TASK_RANGE_ATTACK2 / etc.
//-----------------------------------------------------------------------------
void CE_Temp_NPC::RunTaskRangeAttack1( const Task_t *pTask )
{
	if ( ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR ) == 0 )
	{
		BaseClass::RunTask( pTask );
		return;
	}

	AutoMovement( );

	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if( vecEnemyLKP != vec3_origin )
	{
		if ( ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RELOAD ) && 
			 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
			 FInAimCone_Vector( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	if ( IsActivityFinished() )
	{
		if ( !GetEnemy() || !GetEnemy()->IsAlive() )
		{
			TaskComplete();
			return;
		}

		if ( !GetShotRegulator()->IsInRestInterval() )
		{
			if ( GetShotRegulator()->ShouldShoot() )
			{
				OnRangeAttack1();
				ResetIdealActivity( ACT_RANGE_ATTACK1 );
			}
			return;
		}
		TaskComplete();
	}
}


//-----------------------------------------------------------------------------
// Running Tasks
//-----------------------------------------------------------------------------
void CE_Temp_NPC::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		RunTaskRangeAttack1( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}

//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1
//-----------------------------------------------------------------------------
void CE_Temp_NPC::StartTaskRangeAttack1( const Task_t *pTask )
{
	if ( ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR ) == 0 )
	{
		BaseClass::StartTask( pTask );
		return;
	}

	// Can't shoot if we're in the rest interval; fail the schedule
	if ( GetShotRegulator()->IsInRestInterval() )
	{
		TaskFail( "Shot regulator in rest interval" );
		return;
	}

	if ( GetShotRegulator()->ShouldShoot() )
	{
		OnRangeAttack1();
		ResetIdealActivity( ACT_RANGE_ATTACK1 );
	}
	else
	{
		// This can happen if we start while in the middle of a burst
		// which shouldn't happen, but given the chaotic nature of our AI system,
		// does occasionally happen.
		ResetIdealActivity( ACT_IDLE_ANGRY );
	}
}

#define SNEAK_ATTACK_DIST	360.0f // 30 feet
void CE_Temp_NPC::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	bool bSneakAttacked = false;

	if( ptr->hitgroup == HITGROUP_HEAD )
	{
		CEntity *attacker = CEntity::Instance(info.GetAttacker());
		if ( attacker && attacker->IsPlayer() && attacker != GetEnemy() && !IsInAScript() )
		{
			// Shot in the head by a player I've never seen. In this case the player 
			// has gotten the drop on this enemy and such an attack is always lethal (at close range)
			bSneakAttacked = true;

			AIEnemiesIter_t	iter;
			for( AI_EnemyInfo_t *pMemory = GetEnemies()->GetFirst(&iter); pMemory != NULL; pMemory = GetEnemies()->GetNext(&iter) )
			{
				if ( pMemory->hEnemy == info.GetAttacker() )
				{
					bSneakAttacked = false;
					break;
				}
			}

			float flDist;

			flDist = (attacker->GetAbsOrigin() - GetAbsOrigin()).Length();

			if( flDist > SNEAK_ATTACK_DIST )
			{
				bSneakAttacked = false;
			}
		}
	}

	if( bSneakAttacked )
	{
		CTakeDamageInfo newInfo = info;

		newInfo.SetDamage( GetHealth() );
		BaseClass::TraceAttack( newInfo, vecDir, ptr, pAccumulator );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}


void CE_Temp_NPC::BuildScheduleTestBits( )
{
	BaseClass::BuildScheduleTestBits();

	if ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR )
	{
		if ( GetShotRegulator()->IsInRestInterval() )
		{
			ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		}
	}
} 

//-----------------------------------------------------------------------------
// Starting Tasks
//-----------------------------------------------------------------------------
void CE_Temp_NPC::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		StartTaskRangeAttack1( pTask );
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CE_Temp_NPC::HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt)
{
#ifdef HL2_DLL
	// Annoying to ifdef this out. Copy it into all the HL2 specific humanoid NPC's instead?
	if ( interactionType == g_interactionBarnacleVictimDangle )
	{
		// Force choosing of a new schedule
		ClearSchedule( "Grabbed by a barnacle" );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		// Destroy the entity, the barnacle is going to use the ragdoll that it is releasing
		// as the corpse.
		UTIL_Remove( this );
		return true;
	}
#endif
	return BaseClass::HandleInteraction( interactionType, data, sourceEnt);
}


static bool IsSmall( CEntity *pBlocker )
{
	CCollisionProperty *pCollisionProp = pBlocker->CollisionProp_Actual();
	int  nSmaller = 0;
	Vector vecSize = pCollisionProp->OBBMaxs() - pCollisionProp->OBBMins();
	for ( int i = 0; i < 3; i++ )
	{
		if ( vecSize[i] >= 42 )
			return false;

		if ( vecSize[i] <= 30 )
		{
			nSmaller++;
		}
	}

	return ( nSmaller >= 2 );
}

bool CE_Temp_NPC::OnMoveBlocked( AIMoveResult_t *pResult )
{
	if ( *pResult != AIMR_BLOCKED_NPC && GetNavigator()->GetBlockingEntity() && !GetNavigator()->GetBlockingEntity()->IsNPC() )
	{
		CEntity *pBlocker = GetNavigator()->GetBlockingEntity();

		float massBonus = ( IsNavigationUrgent() ) ? 40.0 : 0;

		if ( pBlocker->GetMoveType() == MOVETYPE_VPHYSICS && 
			 pBlocker != GetGroundEntity() && 
			 !pBlocker->IsNavIgnored() &&
			 !dynamic_cast<CPropDoor *>(pBlocker) &&
			 pBlocker->VPhysicsGetObject() && 
			 pBlocker->VPhysicsGetObject()->IsMoveable() && 
			 ( pBlocker->VPhysicsGetObject()->GetMass() <= 35.0 + massBonus + 0.1 || 
			   ( pBlocker->VPhysicsGetObject()->GetMass() <= 50.0 + massBonus + 0.1 && IsSmall( pBlocker ) ) ) )
		{
			pBlocker->SetNavIgnore( 2.5 );
		}
#if 0
		else
		{
			CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>( pBlocker );
			if ( pProp && pProp->GetHealth() && pProp->GetExplosiveDamage() == 0.0 && GetActiveWeapon() && !GetActiveWeapon()->ClassMatches( "weapon_rpg" ) )
			{
				Msg( "!\n" );
				// Destroy!
			}
		}
#endif
	}

	return BaseClass::OnMoveBlocked( pResult );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct HeadCandidate_t
{
	int iHead;
	int nHeads;

	static int __cdecl Sort( const HeadCandidate_t *pLeft, const HeadCandidate_t *pRight )
	{
		return ( pLeft->nHeads - pRight->nHeads );
	}
};

class NPC_Gman : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Gman, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/gman.mdl");
		SetModelName(AllocPooledString("models/gman.mdl"));
		SetBloodColor( BLOOD_COLOR_MECH );
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_gman, cycler_actor, NPC_Gman);


class NPC_Dog : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Dog, CE_Temp_NPC);

	virtual void Spawn()
	{
		BaseClass::Spawn();
		m_iHealth = 1000;
	}
	void Precache()
	{
		PrecacheModel("models/dpg.mdl");
		SetModelName(AllocPooledString("models/dog.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_dog, cycler_actor, NPC_Dog);

class NPC_Kleiner : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Kleiner, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/kleiner.mdl");
		SetModelName(AllocPooledString("models/kleiner.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_kleiner, cycler_actor, NPC_Kleiner);

class NPC_Eli : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Eli, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/eli.mdl");
		SetModelName(AllocPooledString("models/eli.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_eli, cycler_actor, NPC_Eli);

class NPC_Citizen : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Citizen, CE_Temp_NPC);

	void Precache()
	{
		PrecacheScriptSound( "NPC_Citizen.FootstepLeft" );
		PrecacheScriptSound( "NPC_Citizen.FootstepRight" );
		PrecacheScriptSound( "NPC_Citizen.Die" );

		int nHeads = ARRAYSIZE( g_ppszRandomHeads );
		int i;
		for ( i = 0; i < nHeads; ++i )
		{
			if ( !IsExcludedHead( type, false, i ) )
			{
				PrecacheModel( CFmtStr( "models/Humans/Group01/%s", g_ppszRandomHeads[i] ) );
				PrecacheModel( CFmtStr( "models/Humans/Group02/%s", g_ppszRandomHeads[i] ) );
				PrecacheModel( CFmtStr( "models/Humans/Group03/%s", g_ppszRandomHeads[i] ) );
				PrecacheModel( CFmtStr( "models/Humans/Group03m/%s", g_ppszRandomHeads[i] ) );
			}
		}
		const char *pszModelName = NULL;

			int headCounts[ARRAYSIZE(g_ppszRandomHeads)] = { 0 };
			// Find all candidates
			CUtlVectorFixed<HeadCandidate_t, ARRAYSIZE(g_ppszRandomHeads)> candidates;

			for ( i = 0; i < ARRAYSIZE(g_ppszRandomHeads); i++ )
			{
						HeadCandidate_t candidate = { i, headCounts[i] };
						candidates.AddToTail( candidate );
			}

			Assert( candidates.Count() );
			candidates.Sort( &HeadCandidate_t::Sort );

			int iSmallestCount = candidates[0].nHeads;
			int iLimit;

			for ( iLimit = 0; iLimit < candidates.Count(); iLimit++ )
			{
				if ( candidates[iLimit].nHeads > iSmallestCount )
					break;
			}

			int m_iHead = candidates[enginerandom->RandomInt( 0, iLimit - 1 )].iHead;
			pszModelName = g_ppszRandomHeads[m_iHead];
			
		SetModelName( AllocPooledString( CFmtStr( "models/Humans/Group01/%s", pszModelName ) ) );
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_citizen, cycler_actor, NPC_Citizen);


class NPC_Alyx : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Alyx, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/alyx.mdl");
		SetModelName(AllocPooledString("models/alyx.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_alyx, cycler_actor, NPC_Alyx);

class NPC_Barney : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Barney, CE_Temp_NPC);
	DECLARE_DATADESC();

	void Precache()
	{
		PrecacheModel("models/barney.mdl");
		SetModelName(AllocPooledString("models/barney.mdl"));
		BaseClass::Precache();
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		BaseClass::Use(pActivator, pCaller, useType, value);
		m_OnPlayerUse.FireOutput( pActivator, pCaller );
	}

	COutputEvent				m_OnPlayerUse;
};

BEGIN_DATADESC( NPC_Barney )
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
END_DATADESC()

LINK_ENTITY_TO_CUSTOM_CLASS( npc_barney, cycler_actor, NPC_Barney);

class NPC_Monk : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(NPC_Monk, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/monk.mdl");
		SetModelName(AllocPooledString("models/monk.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_monk, cycler_actor, NPC_Monk);

