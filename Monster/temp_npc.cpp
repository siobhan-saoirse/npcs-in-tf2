
#include "CEntity.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"
#include "fmtstr.h"
#include "CAI_utils.h"
#include "CCombatWeapon.h"
#include "CPropDoor.h"
#include "temp_npc.h"
#include "sign_func.h"
#include "ammodef.h"
#include "CE_recipientfilter.h"
#include "CAI_Navigator.h"

#define SC_PLFEAR	"SC_PLFEAR"
#define SC_FEAR		"SC_FEAR"
#define SC_HEAL		"SC_HEAL"
#define SC_SCREAM	"SC_SCREAM"
#define SC_POK		"SC_POK"

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


//-----------------------------------------------------------------------------
void CE_Temp_NPC::AimGun()
{
	if (GetEnemy())
	{
		Vector vecShootOrigin;

		vecShootOrigin = Weapon_ShootPosition();
		Vector vecShootDir = GetShootEnemyDir( vecShootOrigin, false );

		SetAim( vecShootDir );
	}
	else
	{
		RelaxAim( );
	}
	Activity NewActivity = NPC_TranslateActivity(GetActivity());

	//Don't set the ideal activity to an activity that might not be there.
	if ( SelectWeightedSequence( NewActivity ) == ACT_INVALID )
		return;

	if (NewActivity != GetActivity() )
	{
		SetIdealActivity( NewActivity );
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define PC_LARGER_BURST_RANGE	(12.0f * 10.0f) // If an enemy is this close, player companions fire larger continuous bursts.
void CE_Temp_NPC::OnUpdateShotRegulator()
{
	BaseClass::OnUpdateShotRegulator();

	if( GetEnemy() && HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		/*if( GetAbsOrigin().DistTo( GetEnemy()->GetAbsOrigin() ) <= PC_LARGER_BURST_RANGE )
		{
			// Longer burst
			int longBurst = enginerandom->RandomInt( 10, 15 );
			GetShotRegulator()->SetBurstShotsRemaining( longBurst );
			GetShotRegulator()->SetRestInterval( 0.1, 0.2 );
		} else {*/
			GetShotRegulator()->SetBurstShotCountRange(GetActiveWeapon()->GetMinBurst(), GetActiveWeapon()->GetMaxBurst() );
		//}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CE_Temp_NPC::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( GetEnemy() && FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		// If we're fighting a gunship, try again
		if ( GetEnemy() && FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		break;

	case SCHED_RANGE_ATTACK1:
		if ( GetShotRegulator()->IsInRestInterval() )
			return SCHED_STANDOFF;

		if( !OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return SCHED_STANDOFF;
		break;

	case SCHED_RUN_FROM_ENEMY_FALLBACK:
		{
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				return SCHED_RANGE_ATTACK1;
			}
			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

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
		SetBloodColor(BLOOD_COLOR_MECH);
	}
	void Precache()
	{
		PrecacheModel("models/dog.mdl");
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
	

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY;
	}


	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void DeathSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
	
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void PainSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
};

class NPC_CitizenRefugee : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_CitizenRefugee, CE_Temp_NPC);

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
			
		SetModelName( AllocPooledString( CFmtStr( "models/Humans/Group02/%s", pszModelName ) ) );
		BaseClass::Precache();
	}
	

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY;
	}


	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void DeathSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
	
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void PainSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
};

class NPC_CitizenRebel : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_CitizenRebel, CE_Temp_NPC);

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
			
		SetModelName( AllocPooledString( CFmtStr( "models/Humans/Group03/%s", pszModelName ) ) );
		BaseClass::Precache();
	}
	

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY;
	}


	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void DeathSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
	
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void PainSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "NPC_Citizen.Die" );
	}
};

class NPC_Odessa : public NPC_Citizen
{
public:
	CE_DECLARE_CLASS(NPC_Odessa, NPC_Citizen);

	void Precache()
	{
		BaseClass::Precache();
		PrecacheScriptSound( "NPC_Citizen.FootstepLeft" );
		PrecacheScriptSound( "NPC_Citizen.FootstepRight" );
		PrecacheScriptSound( "NPC_Citizen.Die" );

		PrecacheModel("models/odessa.mdl");
		SetModelName( AllocPooledString( "models/odessa.mdl" ) );
	}
}

LINK_ENTITY_TO_CUSTOM_CLASS( npc_citizen, cycler_actor, NPC_Citizen);
LINK_ENTITY_TO_CUSTOM_CLASS( npc_citizen_refugee, cycler_actor, NPC_CitizenRefugee);
LINK_ENTITY_TO_CUSTOM_CLASS( npc_citizen_rebel, cycler_actor, NPC_CitizenRebel);
LINK_ENTITY_TO_CUSTOM_CLASS( npc_odessa, cycler_actor, NPC_Odessa);


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

class NPC_Mossman : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(NPC_Mossman, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/mossman.mdl");
		SetModelName(AllocPooledString("models/mossman.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_mossman, cycler_actor, NPC_Mossman);

class NPC_Breen : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(NPC_Breen, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/breen.mdl");
		SetModelName(AllocPooledString("models/breen.mdl"));
		BaseClass::Precache();
	}
	
	virtual Class_T	Classify ( void )
	{
		return CLASS_COMBINE;
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_breen, cycler_actor, NPC_Breen);

#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

class CNPC_Scientist : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(CNPC_Scientist, CE_Temp_NPC);

	void Spawn()
	{
		BaseClass::Spawn();
		
		CapabilitiesClear();
		CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE );
		CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );

		//Select the body first if it's going to be random cause we set his voice pitch in Precache.
		m_nBody = enginerandom->RandomInt( 0, NUM_SCIENTIST_HEADS-1 );// pick a head, any head

		// White hands
		m_nSkin = 0;

		
		// Luther is black, make his hands black
		if ( m_nBody == HEAD_LUTHER )
			m_nSkin = 1;
	}
	void Precache()
	{
		PrecacheModel("models/scientist.mdl");
		SetModelName(AllocPooledString("models/scientist.mdl"));
		PrecacheScriptSound( "Scientist.Pain" );
		BaseClass::Precache();
	}
	int		SelectSchedule( void );
	// Tasks
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	enum
	{
		SCHED_SCI_HEAL = BaseClass::NEXT_SCHEDULE,
		SCHED_SCI_FOLLOWTARGET,
		SCHED_SCI_STOPFOLLOWING,
		SCHED_SCI_FACETARGET,
		SCHED_SCI_COVER,
		SCHED_SCI_HIDE,
		SCHED_SCI_IDLESTAND,
		SCHED_SCI_PANIC,
		SCHED_SCI_FOLLOWSCARED,
		SCHED_SCI_FACETARGETSCARED,
		SCHED_SCI_FEAR,
		SCHED_SCI_STARTLE,
	};

	enum
	{
		TASK_SAY_HEAL = BaseClass::NEXT_TASK,
		TASK_HEAL,
		TASK_SAY_FEAR,
		TASK_RUN_PATH_SCARED,
		TASK_SCREAM,
		TASK_RANDOM_SCREAM,
		TASK_MOVE_TO_TARGET_RANGE_SCARED,
	};


	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void DeathSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "Scientist.Pain" );
	}
	
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void PainSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "Scientist.Pain" );
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY;
	}

	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CUSTOM_CLASS( monster_scientist, monster_generic, CNPC_Scientist);

int ACT_EXCITED;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_scientist, CNPC_Scientist )

	DECLARE_TASK( TASK_SAY_HEAL )
	DECLARE_TASK( TASK_HEAL )
	DECLARE_TASK( TASK_SAY_FEAR )
	DECLARE_TASK( TASK_RUN_PATH_SCARED )
	DECLARE_TASK( TASK_SCREAM )
	DECLARE_TASK( TASK_RANDOM_SCREAM )
	DECLARE_TASK( TASK_MOVE_TO_TARGET_RANGE_SCARED )
	
	DECLARE_ACTIVITY( ACT_EXCITED )

	//=========================================================
	// > SCHED_SCI_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_HEAL,

		"	Tasks"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			50"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCI_FOLLOWTARGET"
		"		TASK_FACE_IDEAL						0"
		"		TASK_SAY_HEAL						0"
		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
		"		TASK_HEAL							0"
		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_SCI_FOLLOWTARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_FOLLOWTARGET,

		"	Tasks"
//		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_SCI_STOPFOLLOWING"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		128"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE"	
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
	)

	//=========================================================
	// > SCHED_SCI_STOPFOLLOWING
	//=========================================================
//	DEFINE_SCHEDULE
//	(
//		SCHED_SCI_STOPFOLLOWING,
//
//		"	Tasks"
//		"		TASK_TALKER_CANT_FOLLOW			0"
//		"	"
//		"	Interrupts"
//	)

	//=========================================================
	// > SCHED_SCI_FACETARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_FACETARGET,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_SCI_FOLLOWTARGET"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_SCI_COVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_COVER,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_SCI_PANIC"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_RUN_PATH_SCARED			0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCI_HIDE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_HIDE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_SCI_PANIC"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_CROUCHIDLE"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_CROUCHIDLE"
		"		TASK_WAIT_RANDOM			10"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_HATE"
		"		COND_SEE_FEAR"
		"		COND_SEE_DISLIKE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_SCI_IDLESTAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_IDLESTAND,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					2"
		"		TASK_TALKER_HEADRESET		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_SCI_PANIC
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_PANIC,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_SCREAM							0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_EXCITED"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_SCI_FOLLOWSCARED
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_FOLLOWSCARED,

		"	Tasks"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCI_FOLLOWTARGET"	
		"		TASK_MOVE_TO_TARGET_RANGE_SCARED	128"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_SCI_FACETARGETSCARED
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_FACETARGETSCARED,

		"	Tasks"
		"	TASK_FACE_TARGET				0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_CROUCHIDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_SCI_FOLLOWSCARED"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_FEAR
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_FEAR,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_SAY_FEAR				0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCI_STARTLE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCI_STARTLE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCI_PANIC"
		"		TASK_RANDOM_SCREAM					0.3"
		"		TASK_STOP_MOVING					0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_CROUCH"
		"		TASK_RANDOM_SCREAM					0.1"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_CROUCHIDLE"
		"		TASK_WAIT_RANDOM					1"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_HATE"
		"		COND_SEE_FEAR"
		"		COND_SEE_DISLIKE"
	)

AI_END_CUSTOM_NPC()

int CNPC_Scientist::SelectSchedule( void )
{
	if( m_NPCState == NPC_STATE_PRONE )
	{
		// Immediately call up to the talker code. Barnacle death is priority schedule.
		return BaseClass::SelectSchedule();
	}

	
	
	if ( HasCondition( COND_HEAR_DANGER ) && m_NPCState != NPC_STATE_PRONE )
	{
		CSound *pSound;
		pSound = GetBestSound();

		if ( pSound && pSound->IsSoundType(SOUND_DANGER) )
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	}

	switch( m_NPCState )
	{

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:

		// Cower when you hear something scary
		if ( HasCondition( COND_HEAR_DANGER ) || HasCondition( COND_HEAR_COMBAT ) ) 
		{
			CSound *pSound;
			pSound = GetBestSound();
		
			if ( pSound )
			{
				if ( pSound->IsSoundType(SOUND_DANGER | SOUND_COMBAT) )
				{
				}
			}
		}
		break;


	case NPC_STATE_COMBAT:

		if ( HasCondition( COND_NEW_ENEMY ) )
			return SCHED_SCI_FEAR;					// Point and scream!
		if ( HasCondition( COND_SEE_ENEMY ) )
			return SCHED_SCI_COVER;		// Take Cover
		
		if ( HasCondition( COND_HEAR_COMBAT ) || HasCondition( COND_HEAR_DANGER ) )
			 return SCHED_TAKE_COVER_FROM_BEST_SOUND;	// Cower and panic from the scary sound!
	
		return SCHED_SCI_COVER;			// Run & Cower
		break;
	}

	return BaseClass::SelectSchedule();
}

void CNPC_Scientist::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:

		//GetExpresser()->BlockSpeechUntil( gpGlobals->curtime + 2 );
		//SetSpeechTarget( GetTarget() );
		//Speak( SC_HEAL );

		TaskComplete();
		break;
	 
	case TASK_SCREAM:
		//Scream();
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		//if ( random->RandomFloat( 0, 1 ) < pTask->flTaskData )
			//Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		/*
		if ( IsOkToSpeak() )
		{
			GetExpresser()->BlockSpeechUntil( gpGlobals->curtime + 2 );
			SetSpeechTarget( GetEnemy() );
			if ( GetEnemy() && GetEnemy()->IsPlayer() )
				Speak( SC_PLFEAR );
			else
				Speak( SC_FEAR );
		}
		*/
		TaskComplete();
		break;

	case TASK_HEAL:
		SetIdealActivity( ACT_MELEE_ATTACK1 );
		break;

	case TASK_RUN_PATH_SCARED:
		GetNavigator()->SetMovementActivity( ACT_RUN_SCARED );
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (GetTarget()->GetAbsOrigin() - GetAbsOrigin()).Length() < 1 )
			{
				TaskComplete();
			}
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

void CNPC_Scientist::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( !IsMoving() )
			TaskComplete();
		//if ( random->RandomInt(0,31) < 8 )
			//Scream();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			float distance;

			if ( GetTarget() == NULL )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				/*
				distance = ( GetNavigator()->GetPath()->ActualGoalPosition() - GetAbsOrigin() ).Length2D();
				
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flTaskData) || (GetNavigator()->GetPath()->ActualGoalPosition() - GetTarget()->GetAbsOrigin()).Length() > pTask->flTaskData * 0.5 )
				{
					GetNavigator()->GetPath()->ResetGoalPosition(GetTarget()->GetAbsOrigin());
					distance = ( GetNavigator()->GetPath()->ActualGoalPosition() - GetAbsOrigin() ).Length2D();
//					GetNavigator()->GetPath()->Find();
					GetNavigator()->SetGoal( GOALTYPE_TARGETENT );
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility
				if ( distance < pTask->flTaskData )
				{
					TaskComplete();
					GetNavigator()->GetPath()->Clear();		// Stop moving
				}
				else
				{
					if ( distance < 190 && GetNavigator()->GetMovementActivity() != ACT_WALK_SCARED )
						GetNavigator()->SetMovementActivity( ACT_WALK_SCARED );
					else if ( distance >= 270 && GetNavigator()->GetMovementActivity() != ACT_RUN_SCARED )
						GetNavigator()->SetMovementActivity( ACT_RUN_SCARED );
				}*/
				if (GetNavigator()->GetMovementActivity() != ACT_RUN_SCARED )
					GetNavigator()->SetMovementActivity( ACT_RUN_SCARED );
			}
		}
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define		BARNEY_BODY_GUNHOLSTERED	0
#define		BARNEY_BODY_GUNDRAWN		1
#define		BARNEY_BODY_GUNGONE			2

class CNPC_Barney : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(CNPC_Barney, CE_Temp_NPC);	
	DECLARE_DATADESC();


	void Spawn()
	{
		BaseClass::Spawn();
		
		CapabilitiesClear();
		CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE | bits_CAP_DOORS_GROUP);
		CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
		
		SetBodygroup( 1, 1 );
	}
	void Precache()
	{
		PrecacheModel("models/hl1bar.mdl");
		SetModelName(AllocPooledString("models/hl1bar.mdl"));
		PrecacheScriptSound( "Barney.FirePistol" );
		PrecacheScriptSound( "Barney.Pain" );
		PrecacheScriptSound( "Barney.Die" );
		BaseClass::Precache();
	}

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void DeathSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "Barney.Die" );
	}
	
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void PainSound( const CTakeDamageInfo &info )
	{
		// Sentences don't play on dead NPCs
		SentenceStop();

		EmitSound( "Barney.Pain" );
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY;
	}

	void    BarneyFirePistol ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		TranslateSchedule( int scheduleType );
	bool    CheckRangeAttack1 ( float flDot, float flDist );
	int		RangeAttack1Conditions( float flDot, float flDist );
	float	m_flCheckAttackTime;
	bool	m_fLastAttackCheck;

	enum
	{
		SCHED_BARNEY_FOLLOW = BaseClass::NEXT_SCHEDULE,
		SCHED_BARNEY_ENEMY_DRAW,
		SCHED_BARNEY_FACE_TARGET,
		SCHED_BARNEY_IDLE_STAND,
		SCHED_BARNEY_STOP_FOLLOWING,
	};

	DEFINE_CUSTOM_AI;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Barney )
	DEFINE_FIELD( m_flCheckAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_fLastAttackCheck, FIELD_BOOLEAN ),
END_DATADESC()

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_barney, CNPC_Barney )

	//=========================================================
	// > SCHED_BARNEY_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_FOLLOW,
	
		"	Tasks"
//		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_BARNEY_STOP_FOLLOWING"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE"
		"	"
		"	Interrupts"
		"			COND_NEW_ENEMY"
		"			COND_LIGHT_DAMAGE"
		"			COND_HEAVY_DAMAGE"
		"			COND_HEAR_DANGER"
		"			COND_PROVOKED"
	)
	
	//=========================================================
	// > SCHED_BARNEY_ENEMY_DRAW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_ENEMY_DRAW,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_BARNEY_FACE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_FACE_TARGET,
	
		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_TARGET			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_BARNEY_FOLLOW"
		"	"
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_DANGER"
	)
	
	//=========================================================
	// > SCHED_BARNEY_IDLE_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_IDLE_STAND,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					2"
		"		TASK_TALKER_HEADRESET		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_SMELL"
	)
		
AI_END_CUSTOM_NPC()


//------------------------------------------------------------------------------
// Purpose : For innate range attack
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Barney::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}
	
	else if ( flDist > 1024 )
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.5 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	if ( CheckRangeAttack1 ( flDot, flDist ) )
		return( COND_CAN_RANGE_ATTACK1 );

	return COND_NONE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CNPC_Barney::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->curtime > m_flCheckAttackTime )
	{
		m_flCheckAttackTime = gpGlobals->curtime + 1;
		m_fLastAttackCheck = TRUE;
		m_flCheckAttackTime = gpGlobals->curtime + 1.5;
	}
	
	return m_fLastAttackCheck;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CNPC_Barney::BarneyFirePistol( void )
{
	Vector vecShootOrigin;
	
	vecShootOrigin = GetAbsOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	
	VectorAngles( vecShootDir, angDir );
//	SetBlending( 0, angDir.x );
	DoMuzzleFlash();

	CAI_NPC::FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, GetAmmoDef()->Index("AR2") );
	
	int pitchShift = enginerandom->RandomInt( 0, 20 );
	
	EmitSound( "Barney.FirePistol" );

	g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT, GetAbsOrigin(), 384, 0.3 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Barney::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		SetBodygroup( 1, BARNEY_BODY_GUNDRAWN);
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup( 1, BARNEY_BODY_GUNHOLSTERED);
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//=======================================================
// AI Schedules Specific to this monster
//=========================================================

int CNPC_Barney::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ARM_WEAPON:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_BARNEY_ENEMY_DRAW;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_BARNEY_FACE_TARGET;
			else
				return baseType;
		}
		break;

	case SCHED_TARGET_CHASE:
	{
		return SCHED_BARNEY_FOLLOW;
		break;
	}

	case SCHED_IDLE_STAND:
		{
			int	baseType;

			// call base class default so that scientist will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule( scheduleType );
			
			if ( baseType == SCHED_IDLE_STAND )
				return SCHED_BARNEY_IDLE_STAND;
			else
				return baseType;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_CHASE_ENEMY:
		{
			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				 return SCHED_TAKE_COVER_FROM_ENEMY;

			// No need to take cover since I can see him
			// SHOOT!
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
				 return SCHED_RANGE_ATTACK1;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


LINK_ENTITY_TO_CUSTOM_CLASS( monster_barney, monster_generic, CNPC_Barney);
