//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the bullsquid
//
// $NoKeywords: $
//=============================================================================//

#include "npc_bullsquid.h"
#include "CE_recipientfilter.h"
#include "cutil.h"
#include "CAI_tacticalservices.h"
#include "CAI_Hint.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"
#include "fmtstr.h"
#include "CAI_utils.h"
#include "CCombatWeapon.h"
#include "CPropDoor.h"
#include "temp_npc.h"

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

ConVar sk_bullsquid_health( "sk_bullsquid_health", "0" );
ConVar sk_bullsquid_dmg_bite( "sk_bullsquid_dmg_bite", "0" );
ConVar sk_bullsquid_dmg_whip( "sk_bullsquid_dmg_whip", "0" );

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SQUID_HURTHOP = LAST_SHARED_SCHEDULE + 1,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SQUID_HOPTURN = LAST_SHARED_TASK + 1,
	TASK_SQUID_EAT,
};

//-----------------------------------------------------------------------------
// Squid Conditions
//-----------------------------------------------------------------------------
enum
{
	COND_SQUID_SMELL_FOOD	= LAST_SHARED_CONDITION + 1,
};


//=========================================================
// Interactions
//=========================================================
int	g_interactionBullsquidThrow		= 0;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSQUID_AE_SPIT		( 1 )
#define		BSQUID_AE_BITE		( 2 )
#define		BSQUID_AE_BLINK		( 3 )
#define		BSQUID_AE_ROAR		( 4 )
#define		BSQUID_AE_HOP		( 5 )
#define		BSQUID_AE_THROW		( 6 )
#define		BSQUID_AE_WHIP_SND	( 7 )
#define		BSQUID_AE_TAILWHIP	( 8 )

LINK_ENTITY_TO_CUSTOM_CLASS( monster_bullchicken, monster_generic, CNPC_Bullsquid );

int ACT_SQUID_EXCITED;
int ACT_SQUID_EAT;
int ACT_SQUID_DETECT_SCENT;
int ACT_SQUID_INSPECT_FLOOR;


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Bullsquid )

	DEFINE_FIELD( m_fCanThreatDisplay,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLastHurtTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flNextSpitTime,		FIELD_TIME ),
//	DEFINE_FIELD( m_nSquidSpitSprite,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flHungryTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nextSquidSoundTime,	FIELD_TIME ),

END_DATADESC()


//=========================================================
// Spawn
//=========================================================
void CNPC_Bullsquid::Spawn()
{
	Precache( );

	SetModel( "models/bullsquid.mdl");
	SetHullType(HULL_WIDE_SHORT);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	
	SetRenderColor( 255, 255, 255, 255 );
	
	m_iHealth			= sk_bullsquid_health.GetFloat();
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 );
	
	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->curtime;

	NPCInit();

	m_flDistTooFar		= 784;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Bullsquid::Precache()
{
	PrecacheModel( "models/bullsquid.mdl" );

	g_helpfunc.UTIL_PrecacheOther( "grenade_spit" );

	PrecacheScriptSound( "NPC_Bullsquid.Idle" );
	PrecacheScriptSound( "NPC_Bullsquid.Pain" );
	PrecacheScriptSound( "NPC_Bullsquid.Alert" );
	PrecacheScriptSound( "NPC_Bullsquid.Death" );
	PrecacheScriptSound( "NPC_Bullsquid.Attack1" );
	PrecacheScriptSound( "NPC_Bullsquid.Growl" );
	PrecacheScriptSound( "NPC_Bullsquid.TailWhip");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Bullsquid::Classify( void )
{
	return CLASS_BULLSQUID; 
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CNPC_Bullsquid::IdleSound( void )
{
	EmitSound( "NPC_Bullsquid.Idle" );
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_Bullsquid::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Bullsquid.Pain" );
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_Bullsquid::AlertSound( void )
{
	EmitSound( "NPC_Bullsquid.Alert" );
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_Bullsquid::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Bullsquid.Death" );
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_Bullsquid::AttackSound( void )
{
	EmitSound( "NPC_Bullsquid.Attack1" );
}

//=========================================================
// GrowlSound
//=========================================================
void CNPC_Bullsquid::GrowlSound( void )
{
	if (gpGlobals->curtime >= m_nextSquidSoundTime)
	{
		EmitSound( "NPC_Bullsquid.Growl" );
		m_nextSquidSoundTime	= gpGlobals->curtime + enginerandom->RandomInt(1.5,3.0);
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Bullsquid::MaxYawSpeed( void )
{
	float flYS = 0;

	switch ( GetActivity() )
	{
	case	ACT_WALK:			flYS = 90;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	case	ACT_RANGE_ATTACK1:	flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a throw velocity from start to end position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_Bullsquid::CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize) 
{
	// Get the height I have to throw to get to the target
	float stepHeight = endPos.z - startPos.z;
	float throwHeight = 0;

	// -----------------------------------------------------------------
	// Now calcluate the distance to a point halfway between our current
	// and target position.  (the apex of our throwing arc)
	// -----------------------------------------------------------------
	Vector targetDir2D = endPos - startPos;
	targetDir2D.z = 0;

	float distance = VectorNormalize(targetDir2D);


	// If jumping up we want to throw a bit higher than the height diff	
	if (stepHeight > 0) 
	{
		throwHeight = stepHeight + fArcSize;	
	}	
	else
	{
		throwHeight = fArcSize;
	}
	// Make sure that I at least catch some air
	if (throwHeight < fArcSize)
	{
		throwHeight = fArcSize;
	}


	// -------------------------------------------------------------
	// calculate the vertical and horizontal launch velocities
	// -------------------------------------------------------------
	float velVert = (float)sqrt(2.0f*fGravity*throwHeight);

	float divisor = velVert;
	divisor += (float)sqrt((2.0f*(-fGravity)*(stepHeight-throwHeight)));

	float velHorz = (distance * fGravity)/divisor;

	// -----------------------------------------------------------
	// Make the horizontal throw vector and add vertical component
	// -----------------------------------------------------------
	Vector throwVel = targetDir2D * velHorz;
	throwVel.z = velVert;

	return throwVel;
}

float CNPC_Bullsquid::ThrowLimit(	const Vector &vecStart,
								const Vector &vecEnd,
								float		fGravity,
								float		fArcSize,
								const Vector &mins,
								const Vector &maxs,
								CEntity *pTarget,
								Vector		*jumpVel,
								CEntity **pBlocker)
{
	// Get my jump velocity
	Vector rawJumpVel	= CalcThrowVelocity(vecStart, vecEnd, fGravity, fArcSize);
	*jumpVel			= rawJumpVel;
	Vector vecFrom		= vecStart;

	// Calculate the total time of the jump minus a tiny fraction
	float jumpTime		= (vecStart - vecEnd).Length2D()/rawJumpVel.Length2D();
	float timeStep		= jumpTime / 10.0;

	Vector gravity = Vector(0,0,fGravity);

	// this loop takes single steps to the goal.
	for (float flTime = 0 ; flTime < jumpTime-0.1 ; flTime += timeStep )
	{
		// Calculate my position after the time step (average velocity over this time step)
		Vector nextPos = vecFrom + (rawJumpVel - 0.5 * gravity * timeStep) * timeStep;

		// If last time step make next position the target position
		if ((flTime + timeStep) > jumpTime)
		{
			nextPos = vecEnd;
		}

		trace_t tr;
		UTIL_TraceHull( vecFrom, nextPos, mins, maxs, MASK_SOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );

		if (tr.startsolid || tr.fraction < 1.0)
		{
			CEntity *pEntity = CEntity::Instance(tr.m_pEnt);

			// If we hit the target we are good to go!
			if (pEntity == pTarget)
			{
				return 0;
			}

#ifdef _THROWDEBUG
			NDebugOverlay::Line( vecFrom, nextPos, 255, 0, 0, true, 1.0 );
#endif
			// ----------------------------------------------------------
			// If blocked by an npc remember
			// ----------------------------------------------------------
			*pBlocker = pEntity;

			// Return distance sucessfully traveled before block encountered
			return ((tr.endpos - vecStart).Length());
		}
#ifdef _THROWDEBUG
		else
		{
			NDebugOverlay::Line( vecFrom, nextPos, 255, 255, 255, true, 1.0 );
		}
#endif


		rawJumpVel  = rawJumpVel - gravity * timeStep;
		vecFrom		= nextPos;
	}
	return 0;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Bullsquid::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BSQUID_AE_SPIT:
		{
			if ( GetEnemy() )
			{
				Vector vSpitPos = GetAbsOrigin() + Vector(0,0,30);
				
				Vector			vTarget = GetEnemy()->GetAbsOrigin();
				Vector			vToss;
				CEntity*	pBlocker;
				float flGravity  = SPIT_GRAVITY;
				ThrowLimit(vSpitPos, vTarget, flGravity, 3, Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker);

				CGrenadeSpit *pGrenade = (CGrenadeSpit*)CreateNoSpawn( "grenade_spit", vSpitPos, vec3_angle, this->BaseEntity() );
				//pGrenade->KeyValue( "velocity", vToss );
				pGrenade->Spawn( );
				pGrenade->SetThrower( this->BaseEntity() );
				pGrenade->SetOwnerEntity( this->BaseEntity() );
				pGrenade->SetSpitSize( 2 );
				pGrenade->SetAbsVelocity( vToss );

				// Tumble through the air
				pGrenade->SetLocalAngularVelocity(
					QAngle(
						enginerandom->RandomFloat( -100, -500 ),
						enginerandom->RandomFloat( -100, -500 ),
						enginerandom->RandomFloat( -100, -500 )
					)
				);
						
				AttackSound();
			}
		}
		break;

		case BSQUID_AE_BITE:
		{
		// SOUND HERE!
			CEntity *pHurt = (CEntity *)CEntity::Instance( CheckTraceHullAttack_Float( 70, Vector(-16,-16,-16), Vector(16,16,16), sk_bullsquid_dmg_bite.GetFloat(), DMG_SLASH ) );
			if ( pHurt )
			{
				Vector forward, up;
				AngleVectors( GetAbsAngles(), &forward, NULL, &up );
				pHurt->ApplyAbsVelocityImpulse( 100 * (up-forward) );
				pHurt->SetGroundEntity( NULL );
				EmitSound( "NPC_AntlionGuard.Shove" );
			}
		}
		break;

		case BSQUID_AE_WHIP_SND:
		{
			EmitSound( "NPC_Bullsquid.TailWhip" );
			break;
		}


		case BSQUID_AE_TAILWHIP:
		{
			CEntity *pHurt = (CEntity *)CEntity::Instance( CheckTraceHullAttack_Float( 70, Vector(-16,-16,-16), Vector(16,16,16), sk_bullsquid_dmg_whip.GetFloat(), DMG_SLASH | DMG_ALWAYSGIB ) );
			if ( pHurt ) 
			{
				Vector right, up;
				AngleVectors( GetAbsAngles(), NULL, &right, &up );

				if ( pHurt->GetFlags() & ( FL_CLIENT ) )
					 pHurt->ViewPunch( QAngle( 20, 0, -20 ) );
			
				pHurt->ApplyAbsVelocityImpulse( 100 * (up+2*right) );
				EmitSound( "NPC_Vortigaunt.Claw" );
			}
		}
		break;


		case BSQUID_AE_BLINK:
		{
			// close eye. 
			m_nSkin = 1;
		}
		break;

		case BSQUID_AE_HOP:
		{
			float flGravity = GetCurrentGravity();

			// throw the squid up into the air on this frame.
			if ( GetFlags() & FL_ONGROUND )
			{
				SetGroundEntity( NULL );
			}

			// jump 40 inches into the air
			Vector vecVel = GetAbsVelocity();
			vecVel.z += sqrt( flGravity * 2.0 * 40 );
			SetAbsVelocity( vecVel );
		}
		break;

		case BSQUID_AE_THROW:
			{
				// squid throws its prey IF the prey is a client. 
				CEntity *pHurt = (CEntity *)CEntity::Instance(CheckTraceHullAttack_Float( 70, Vector(-16,-16,-16), Vector(16,16,16), 0, 0 ));


				if ( pHurt )
				{
					if (pHurt->GetFlags() & FL_CLIENT)
					{
						pHurt->ViewPunch( QAngle(20,0,-20) );
					}
							
					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->GetAbsOrigin(), 25.0, 1.5, 0.7, 2, SHAKE_START );

					// If the player, throw him around
					if (pHurt->GetFlags() & FL_CLIENT)
					{
						Vector forward, up;
						AngleVectors( GetLocalAngles(), &forward, NULL, &up );
						pHurt->ApplyAbsVelocityImpulse( forward * 300 + up * 300 );
					}
					// If not the player see if has bullsquid throw interatcion
					else
					{
						CCombatCharacter *pVictim = ToBaseCombatCharacter( pHurt );
						if (pVictim)
						{
							if ( pVictim->DispatchInteraction( g_interactionBullsquidThrow, NULL, BaseEntity() ) )
							{
								Vector forward, up;
								AngleVectors( GetLocalAngles(), &forward, NULL, &up );
								pVictim->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );
							}
						}
					}
				}
			}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
	}
}

int CNPC_Bullsquid::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return ( COND_NONE );
	}

	if ( flDist > 85 && flDist <= 784 && flDot >= 0.5 && gpGlobals->curtime >= m_flNextSpitTime )
	{
		if ( GetEnemy() != NULL )
		{
			if ( fabs( GetAbsOrigin().z - GetEnemy()->GetAbsOrigin().z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return( COND_NONE );
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->curtime + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->curtime + 0.5;
		}

		return( COND_CAN_RANGE_ATTACK1 );
	}

	return( COND_NONE );
}

//=========================================================
// MeleeAttack2Conditions - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
int CNPC_Bullsquid::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy()->m_iHealth <= sk_bullsquid_dmg_whip.GetFloat() && flDist <= 85 && flDot >= 0.7 )
	{
		return ( COND_CAN_MELEE_ATTACK1 );
	}
	
	return( COND_NONE );
}

//=========================================================
// MeleeAttack2Conditions - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
int CNPC_Bullsquid::MeleeAttack2Conditions( float flDot, float flDist )
{
	if ( flDist <= 85 && flDot >= 0.7 && !HasCondition( COND_CAN_MELEE_ATTACK1 ) )		// The player & bullsquid can be as much as their bboxes 
		 return ( COND_CAN_MELEE_ATTACK2 );
	
	return( COND_NONE );
}

bool CNPC_Bullsquid::FValidateHintType( CAI_Hint *pHint )
{
	DevMsg( "Couldn't validate hint type" );

	return false;
}

void CNPC_Bullsquid::RemoveIgnoredConditions( void )
{
	if ( m_flHungryTime > gpGlobals->curtime )
		 ClearCondition( COND_SQUID_SMELL_FOOD );

	if ( gpGlobals->curtime - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		ClearCondition( COND_SMELL );
	}
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CNPC_Bullsquid::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{

#if 0 //Fix later.

	float flDist;
	Vector vecApex, vOffset;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if ( GetEnemy() != NULL && IsMoving() && pevAttacker == GetEnemy() && gpGlobals->curtime - m_flLastHurtTime > 3 )
	{
		flDist = ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).Length2D();
		
		if ( flDist > SQUID_SPRINT_DIST )
		{
			AI_Waypoint_t*	pRoute = GetNavigator()->GetPath()->Route();

			if ( pRoute )
			{
				flDist = ( GetAbsOrigin() - pRoute[ pRoute->iNodeID ].vecLocation ).Length2D();// reusing flDist. 

				if ( GetNavigator()->GetPath()->BuildTriangulationRoute( GetAbsOrigin(), pRoute[ pRoute->iNodeID ].vecLocation, flDist * 0.5, GetEnemy(), &vecApex, &vOffset, NAV_GROUND ) )
				{
					GetNavigator()->PrependWaypoint( vecApex, bits_WP_TO_DETOUR | bits_WP_DONT_SIMPLIFY );
				}
			}
		}
	}
#endif

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}

//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CNPC_Bullsquid::GetSoundInterests( void )
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
		    SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_PLAYER;
}

//=========================================================
// OnListened - monsters dig through the active sound list for
// any sounds that may interest them. (smells, too!)
//=========================================================
void CNPC_Bullsquid::OnListened( void )
{
	AISoundIter_t iter;
	
	CSound *pCurrentSound;

	static int conditionsToClear[] = 
	{
		COND_SQUID_SMELL_FOOD,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );
	/*
	pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );
	
	while ( pCurrentSound )
	{
		// the npc cares about this sound, and it's close enough to hear.
		int condition = COND_NONE;
		
		if ( !pCurrentSound->FIsSound() )
		{
			// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
			if ( pCurrentSound->m_iType & ( SOUND_MEAT | SOUND_CARCASS ) )
			{
				// the detected scent is a food item
				condition = COND_SQUID_SMELL_FOOD;
			}
		}
		
		if ( condition != COND_NONE )
			SetCondition( condition );

		pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
	}
	*/

	BaseClass::OnListened();
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CNPC_Bullsquid::RunAI( void )
{
	// first, do base class stuff
	BaseClass::RunAI();

	if ( m_nSkin != 0 )
	{
		// close eye if it was open.
		m_nSkin = 0; 
	}

	if ( enginerandom->RandomInt( 0,39 ) == 0 )
	{
		m_nSkin = 1;
	}

	if ( GetEnemy() != NULL && GetActivity() == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D() < SQUID_SPRINT_DIST )
		{
			m_flPlaybackRate = 1.25;
		}
	}

}

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_Bullsquid::SelectSchedule( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_ALERT:
		{
			if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
			{
				return SCHED_SQUID_HURTHOP;
			}

			if ( HasCondition( COND_SQUID_SMELL_FOOD ) )
			{
				CSound		*pSound;

				pSound = GetBestScent();
				
				if ( pSound && (!FInViewCone( pSound->GetSoundOrigin() ) || !CBaseEntity_FVisible( pSound->GetSoundOrigin() )) )
				{
					// scent is behind or occluded
					return SCHED_SQUID_SNIFF_AND_EAT;
				}

				// food is right out in the open. Just go get it.
				return SCHED_SQUID_EAT;
			}

			if ( HasCondition( COND_SMELL ) )
			{
				// there's something stinky. 
				CSound		*pSound;

				pSound = GetBestScent();
				if ( pSound )
					return SCHED_SQUID_WALLOW;
			}

			break;
		}
	case NPC_STATE_COMBAT:
		{
// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if ( HasCondition( COND_NEW_ENEMY ) )
			{
				return SCHED_WAKE_ANGRY;
			}

			if ( HasCondition( COND_SQUID_SMELL_FOOD ) )
			{
				CSound		*pSound;

				pSound = GetBestScent();
				
				if ( pSound && (!FInViewCone( pSound->GetSoundOrigin() ) || !CBaseEntity_FVisible( pSound->GetSoundOrigin() )) )
				{
					// scent is behind or occluded
					return SCHED_SQUID_SNIFF_AND_EAT;
				}

				// food is right out in the open. Just go get it.
				return SCHED_SQUID_EAT;
			}

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				return SCHED_RANGE_ATTACK1;
			}

			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
			{
				return SCHED_MELEE_ATTACK1;
			}

			if ( HasCondition( COND_CAN_MELEE_ATTACK2 ) )
			{
				return SCHED_MELEE_ATTACK2;
			}
			
			return SCHED_CHASE_ENEMY;

			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CNPC_Bullsquid::FInViewCone( Vector pOrigin )
{
	Vector los = ( pOrigin - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = EyeDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > m_flFieldOfView )
		return true;

	return false;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CNPC_Bullsquid::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			if (GetEnemy())
			{
				GrowlSound();

				m_flLastAttackTime = gpGlobals->curtime;

				BaseClass::StartTask( pTask );
			}
			break;
		}
	case TASK_SQUID_HOPTURN:
		{
			SetActivity( ACT_HOP );
			
			if ( GetEnemy() )
			{
				Vector	vecFacing = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
				VectorNormalize( vecFacing );

				GetMotor()->SetIdealYaw( vecFacing );
			}

			break;
		}
	case TASK_SQUID_EAT:
		{
			m_flHungryTime = gpGlobals->curtime + pTask->flTaskData;
			TaskComplete();
			break;
		}
	default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_Bullsquid::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SQUID_HOPTURN:
		{
			if ( GetEnemy() )
			{
				Vector	vecFacing = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
				VectorNormalize( vecFacing );
				GetMotor()->SetIdealYaw( vecFacing );
			}

			if ( IsSequenceFinished() )
			{
				TaskComplete(); 
			}
			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
NPC_STATE CNPC_Bullsquid::SelectIdealState( void )
{
	return BaseClass::SelectIdealState();
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( monster_bullchicken, CNPC_Bullsquid )

	DECLARE_TASK( TASK_SQUID_HOPTURN )
	DECLARE_TASK( TASK_SQUID_EAT )

	DECLARE_CONDITION( COND_SQUID_SMELL_FOOD )

	DECLARE_ACTIVITY( ACT_SQUID_EXCITED )
	DECLARE_ACTIVITY( ACT_SQUID_EAT )
	DECLARE_ACTIVITY( ACT_SQUID_DETECT_SCENT )
	DECLARE_ACTIVITY( ACT_SQUID_INSPECT_FLOOR )

	DECLARE_INTERACTION( g_interactionBullsquidThrow )

	//=========================================================
	// > SCHED_SQUID_HURTHOP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SQUID_HURTHOP,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_WAKE				0"
		"		TASK_SQUID_HOPTURN			0"
		"		TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_SQUID_SEECRAB
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SQUID_SEECRAB,
	
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_WAKE				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_SQUID_EXCITED"
		"		TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)
	
	//=========================================================
	// > SCHED_SQUID_EAT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SQUID_EAT,
	
		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SQUID_EAT						10"
		"		TASK_STORE_LASTPOSITION				0"
		"		TASK_GET_PATH_TO_BESTSCENT			0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_SQUID_EAT						50"
		"		TASK_GET_PATH_TO_LASTPOSITION		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CLEAR_LASTPOSITION				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_SMELL"
	)
	
	//=========================================================
	// > SCHED_SQUID_SNIFF_AND_EAT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SQUID_SNIFF_AND_EAT,
	
		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SQUID_EAT						10"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_DETECT_SCENT"
		"		TASK_STORE_LASTPOSITION				0"
		"		TASK_GET_PATH_TO_BESTSCENT			0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SQUID_EAT"
		"		TASK_SQUID_EAT						50"
		"		TASK_GET_PATH_TO_LASTPOSITION		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CLEAR_LASTPOSITION				0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_SMELL"
	)
	
	//=========================================================
	// > SCHED_SQUID_WALLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SQUID_WALLOW,
	
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SQUID_EAT					10"
		"		TASK_STORE_LASTPOSITION			0"
		"		TASK_GET_PATH_TO_BESTSCENT		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_SQUID_INSPECT_FLOOR"
		"		TASK_SQUID_EAT					50"
		"		TASK_GET_PATH_TO_LASTPOSITION	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_CLEAR_LASTPOSITION			0"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
	)
	
AI_END_CUSTOM_NPC()
