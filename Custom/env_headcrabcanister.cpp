
#include "CSode_Fix.h"
#include "CSpriteTrail.h"
#include "CSmoke_trail.h"
#include "env_headcrabcanister_shared.h"
#include "CSkyCamera.h"
#include "CE_recipientfilter.h"
#include "CPlayer.h"
#include "npc_headcrab.h"
#include "CEnvExplosion.h"


//-----------------------------------------------------------------------------
// Models!
//-----------------------------------------------------------------------------
#define ENV_HEADCRABCANISTER_MODEL	"models/props_combine/headcrabcannister01a.mdl"
#define ENV_HEADCRABCANISTER_BROKEN_MODEL	"models/props_combine/headcrabcannister01b.mdl"
#define ENV_HEADCRABCANISTER_SKYBOX_MODEL	"models/props_combine/headcrabcannister01a_skybox.mdl"
#define ENV_HEADCRABCANISTER_INCOMING_SOUND_TIME	1.0f

ConVar sk_env_headcrabcanister_shake_amplitude( "sk_env_headcrabcanister_shake_amplitude", "50" );
ConVar sk_env_headcrabcanister_shake_radius( "sk_env_headcrabcanister_shake_radius", "1024" );
ConVar sk_env_headcrabcanister_shake_radius_vehicle( "sk_env_headcrabcanister_shake_radius_vehicle", "2500" );

#define ENV_HEADCRABCANISTER_TRAIL_TIME	3.0f

//-----------------------------------------------------------------------------
// Spawn flags
//-----------------------------------------------------------------------------
enum
{
	SF_NO_IMPACT_SOUND = 0x1,
	SF_NO_LAUNCH_SOUND = 0x2,
	SF_START_IMPACTED = 0x1000,
	SF_LAND_AT_INITIAL_POSITION = 0x2000,
	SF_WAIT_FOR_INPUT_TO_OPEN = 0x4000,
	SF_WAIT_FOR_INPUT_TO_SPAWN_HEADCRABS = 0x8000,
	SF_NO_SMOKE	= 0x10000,
	SF_NO_SHAKE = 0x20000,
	SF_REMOVE_ON_IMPACT = 0x40000,
	SF_NO_IMPACT_EFFECTS = 0x80000,
};


//-----------------------------------------------------------------------------
// Headcrab types
//-----------------------------------------------------------------------------
static const char *s_pHeadcrabClass[] = 
{
	"npc_headcrab",
	"npc_headcrab_fast",
	"npc_headcrab_poison",
};


//-----------------------------------------------------------------------------
// Context think
//-----------------------------------------------------------------------------
static const char *s_pOpenThinkContext = "OpenThink";
static const char *s_pHeadcrabThinkContext = "HeadcrabThink";


//-----------------------------------------------------------------------------
// HeadcrabCanister Class
//-----------------------------------------------------------------------------
class CEnvHeadcrabCanister : public CSode_Fix
{
	CE_DECLARE_CLASS( CEnvHeadcrabCanister, CSode_Fix );
	DECLARE_DATADESC();

public:

	// Initialization
	CEnvHeadcrabCanister();

	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual void		UpdateOnRemove();

	virtual void		SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

private:
	void				InputFireCanister( inputdata_t &inputdata );
	void				InputOpenCanister( inputdata_t &inputdata );
	void				InputSpawnHeadcrabs( inputdata_t &inputdata );
	void				InputStopSmoke( inputdata_t &inputdata );

	// Think(s)
	void				HeadcrabCanisterSkyboxThink( void );
	void				HeadcrabCanisterWorldThink( void );
	void				HeadcrabCanisterSpawnHeadcrabThink();
	void				HeadcrabCanisterSkyboxOnlyThink( void );
	void				HeadcrabCanisterSkyboxRestartThink( void );
	void				WaitForOpenSequenceThink();


	// Place the canister in the world
	CE_CSkyCamera*			PlaceCanisterInWorld();

	// Check for impacts
	void				TestForCollisionsAgainstEntities( const Vector &vecEndPosition );
	void				TestForCollisionsAgainstWorld( const Vector &vecEndPosition );

	// Figure out where we enter the world
	void				ComputeWorldEntryPoint( Vector *pStartPosition, QAngle *pStartAngles, Vector *pStartDirection );

	// Blows up!
	void				Detonate( void );

	// Landed!
	void				SetLanded( void );
	void				Landed( void );

	// Open!
	void				OpenCanister( void );
	void				CanisterFinishedOpening();

	// Set up the world model
	void				SetupWorldModel();

	// Start spawning headcrabs
	void				StartSpawningHeadcrabs( float flDelay );

private:
	bool m_bLanded;

	CEnvHeadcrabCanisterShared  m_Shared;
	CEFakeHandle<CE_CSpriteTrail> m_hTrail;
	CEFakeHandle<CSmokeTrail>	m_hSmokeTrail;
	int m_nHeadcrabType;
	int m_nHeadcrabCount;
	Vector m_vecImpactPosition;
	float m_flDamageRadius;
	float m_flDamage;
	bool m_bIncomingSoundStarted;
	bool m_bHasDetonated;
	bool m_bLaunched;
	bool m_bOpened;
	float m_flSmokeLifetime;
	string_t m_iszLaunchPositionName;

	COutputEHANDLE m_OnLaunched;
	COutputEvent m_OnImpacted;
	COutputEvent m_OnOpened;

	// Only for skybox only cannisters.
	float m_flMinRefireTime;
	float m_flMaxRefireTime;
	int m_nSkyboxCannisterCount;
};


//=============================================================================
//
// HeadcrabCanister Functions
//

LINK_ENTITY_TO_CUSTOM_CLASS( env_headcrabcanister, item_sodacan, CEnvHeadcrabCanister );

BEGIN_DATADESC( CEnvHeadcrabCanister )

	DEFINE_FIELD( m_bLanded,							FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_Shared ),
	DEFINE_FIELD( m_hTrail,								FIELD_EHANDLE ),
	DEFINE_FIELD( m_hSmokeTrail,						FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_nHeadcrabType,					FIELD_INTEGER,	"HeadcrabType" ),
	DEFINE_KEYFIELD( m_nHeadcrabCount,					FIELD_INTEGER,	"HeadcrabCount" ),
	DEFINE_KEYFIELD( m_flSmokeLifetime,					FIELD_FLOAT, "SmokeLifetime" ),
	DEFINE_KEYFIELD( m_iszLaunchPositionName,			FIELD_STRING, "LaunchPositionName" ),
	DEFINE_FIELD( m_vecImpactPosition,					FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bIncomingSoundStarted,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHasDetonated,						FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLaunched,							FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bOpened,							FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flMinRefireTime,					FIELD_FLOAT,	"MinSkyboxRefireTime" ),
	DEFINE_KEYFIELD( m_flMaxRefireTime,					FIELD_FLOAT,	"MaxSkyboxRefireTime" ),
	DEFINE_KEYFIELD( m_nSkyboxCannisterCount,			FIELD_INTEGER,	"SkyboxCannisterCount" ),
	DEFINE_KEYFIELD( m_flDamageRadius,					FIELD_FLOAT,	"DamageRadius" ),
	DEFINE_KEYFIELD( m_flDamage,						FIELD_FLOAT,	"Damage" ),

	// Function Pointers.
	/*DEFINE_FUNCTION( HeadcrabCanisterSkyboxThink ),
	DEFINE_FUNCTION( HeadcrabCanisterWorldThink ),
	DEFINE_FUNCTION( HeadcrabCanisterSpawnHeadcrabThink ),
	DEFINE_FUNCTION( WaitForOpenSequenceThink ),
	DEFINE_FUNCTION( HeadcrabCanisterSkyboxOnlyThink ),
	DEFINE_FUNCTION( HeadcrabCanisterSkyboxRestartThink ),
	*/

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "FireCanister", InputFireCanister ),
	DEFINE_INPUTFUNC( FIELD_VOID, "OpenCanister", InputOpenCanister ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnHeadcrabs", InputSpawnHeadcrabs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopSmoke", InputStopSmoke ),

	// Outputs
	DEFINE_OUTPUT( m_OnLaunched, "OnLaunched" ),
	DEFINE_OUTPUT( m_OnImpacted, "OnImpacted" ),
	DEFINE_OUTPUT( m_OnOpened, "OnOpened" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEnvHeadcrabCanister::CEnvHeadcrabCanister()
{
	m_flMinRefireTime = -1.0f;
	m_flMaxRefireTime = -1.0f;

	m_bLanded = false;
	m_nHeadcrabType = 0;
	m_nHeadcrabCount = 0;
	m_vecImpactPosition.Init();

	m_flDamageRadius = 0.0f;
	m_flDamage = 0.0f;
	m_bIncomingSoundStarted = false;
	m_bHasDetonated = false;
	m_bLaunched = false;
	m_bOpened = false;
	m_flSmokeLifetime = 0.0f;
	m_iszLaunchPositionName = NULL_STRING;
	m_nSkyboxCannisterCount = 0;
}


//-----------------------------------------------------------------------------
// Precache!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( ENV_HEADCRABCANISTER_MODEL );
	PrecacheModel( ENV_HEADCRABCANISTER_BROKEN_MODEL );
	PrecacheModel( ENV_HEADCRABCANISTER_SKYBOX_MODEL );
	PrecacheModel("sprites/smoke.vmt");

	PrecacheScriptSound( "HeadcrabCanister.LaunchSound" );
	PrecacheScriptSound( "HeadcrabCanister.AfterLanding" );
	PrecacheScriptSound( "HeadcrabCanister.Explosion" );
	PrecacheScriptSound( "HeadcrabCanister.IncomingSound" );
	PrecacheScriptSound( "HeadcrabCanister.SkyboxExplosion" );
	PrecacheScriptSound( "HeadcrabCanister.Open" );

	//UTIL_PrecacheOther( s_pHeadcrabClass[m_nHeadcrabType] );
}


//-----------------------------------------------------------------------------
// Spawn!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// Do we have a position to launch from?
	if ( m_iszLaunchPositionName != NULL_STRING )
	{
		// It doesn't have any real presence at first.
		SetSolid( SOLID_NONE );

		m_vecImpactPosition = GetAbsOrigin();
		m_bIncomingSoundStarted = false;
		m_bLanded = false;
		m_bHasDetonated = false;
		m_bOpened = false;
	}
	else if ( !HasSpawnFlags( SF_START_IMPACTED ) )
	{
		// It doesn't have any real presence at first.
		SetSolid( SOLID_NONE );

		if ( !HasSpawnFlags( SF_LAND_AT_INITIAL_POSITION ) )
		{
			Vector vecForward;
			GetVectors( &vecForward, NULL, NULL );
			vecForward *= -1.0f;

			trace_t trace;
			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecForward * 10000, MASK_NPCWORLDSTATIC, 
				BaseEntity(), COLLISION_GROUP_NONE, &trace );

			m_vecImpactPosition = trace.endpos;
		}
		else
		{
			m_vecImpactPosition = GetAbsOrigin();
		}

		m_bIncomingSoundStarted = false;
		m_bLanded = false;
		m_bHasDetonated = false;
		m_bOpened = false;
	}
	else
	{
		m_bHasDetonated = true;
		m_bIncomingSoundStarted = true;
		m_bOpened = false;
		m_vecImpactPosition = GetAbsOrigin();
		Landed();
	}
}


//-----------------------------------------------------------------------------
// On remove!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
	StopSound( "HeadcrabCanister.AfterLanding" );
	if ( m_hTrail )
	{
		UTIL_Remove( m_hTrail );
		m_hTrail.Set(NULL);
	}
	if ( m_hSmokeTrail )
	{
		UTIL_Remove( m_hSmokeTrail );
		m_hSmokeTrail.Set(NULL);
	}
}


//-----------------------------------------------------------------------------
// Set up the world model
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::SetupWorldModel()
{
	SetModel( ENV_HEADCRABCANISTER_MODEL );
	SetSolid( SOLID_BBOX );

	float flRadius = CollisionProp()->BoundingRadius();
	Vector vecMins( -flRadius, -flRadius, -flRadius );
	Vector vecMaxs( flRadius, flRadius, flRadius );
	UTIL_SetSize( BaseEntity(), vecMins, vecMaxs );

}


//-----------------------------------------------------------------------------
// Figure out where we enter the world
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::ComputeWorldEntryPoint( Vector *pStartPosition, QAngle *pStartAngles, Vector *pStartDirection )
{
	SetupWorldModel();

	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	// Raycast up to the place where we should start from (start raycast slightly off the ground,
	// since it'll be buried in the ground oftentimes)
	trace_t tr;
	CTraceFilterWorldOnly filter;
	UTIL_TraceLine( GetAbsOrigin() + vecForward * 100, GetAbsOrigin() + vecForward * 10000,
		CONTENTS_SOLID, &filter, &tr );

	*pStartPosition = tr.endpos;
	*pStartAngles = GetAbsAngles();
	VectorMultiply( vecForward, -1.0f, *pStartDirection );
}


//-----------------------------------------------------------------------------
// Place the canister in the world
//-----------------------------------------------------------------------------
CE_CSkyCamera *CEnvHeadcrabCanister::PlaceCanisterInWorld()
{
	CE_CSkyCamera *pCamera = NULL;

	// Are we launching from a point? If so, use that point.
	if ( m_iszLaunchPositionName != NULL_STRING )
	{
		// Get the launch position entity
		CEntity *pLaunchPos = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszLaunchPositionName );
		if ( !pLaunchPos )
		{
			Warning("%s (%s) could not find an entity matching LaunchPositionName of '%s'\n", GetEntityName(), GetDebugName(), STRING(m_iszLaunchPositionName) );
			SUB_Remove();
		}
		else
		{
			SetupWorldModel();

			Vector vecForward, vecImpactDirection;
			GetVectors( &vecForward, NULL, NULL );
			VectorMultiply( vecForward, -1.0f, vecImpactDirection );

			m_Shared.InitInWorld( gpGlobals->curtime, pLaunchPos->GetAbsOrigin(), GetAbsAngles(), 
				vecImpactDirection, m_vecImpactPosition, true );
			SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterWorldThink );
			SetNextThink( gpGlobals->curtime );
		}
	}
	else if ( DetectInSkybox() )
	{
		pCamera = GetEntitySkybox();

		SetModel( ENV_HEADCRABCANISTER_SKYBOX_MODEL );
		SetSolid( SOLID_NONE );

		Vector vecForward;
		GetVectors( &vecForward, NULL, NULL );
		vecForward *= -1.0f;

		m_Shared.InitInSkybox( gpGlobals->curtime, m_vecImpactPosition, GetAbsAngles(), vecForward, 
			m_vecImpactPosition, pCamera->m_skyboxData->origin, pCamera->m_skyboxData->scale );
		AddEFlags( EFL_IN_SKYBOX );
		SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterSkyboxOnlyThink );
		SetNextThink( gpGlobals->curtime + m_Shared.GetEnterWorldTime() + TICK_INTERVAL );
	}
	else
	{
		Vector vecStartPosition, vecDirection;
		QAngle vecStartAngles;
		ComputeWorldEntryPoint( &vecStartPosition, &vecStartAngles, &vecDirection ); 

		// Figure out which skybox to place the entity in.
		pCamera = GetCurrentSkyCamera();
		if ( pCamera )
		{
			m_Shared.InitInSkybox( gpGlobals->curtime, vecStartPosition, vecStartAngles, vecDirection, 
				m_vecImpactPosition, pCamera->m_skyboxData->origin, pCamera->m_skyboxData->scale );

			if ( m_Shared.IsInSkybox() )
			{
				SetModel( ENV_HEADCRABCANISTER_SKYBOX_MODEL );
				SetSolid( SOLID_NONE );
				AddEFlags( EFL_IN_SKYBOX );
				SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterSkyboxThink );
				SetNextThink( gpGlobals->curtime + m_Shared.GetEnterWorldTime() );
			}
			else
			{
				SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterWorldThink );
				SetNextThink( gpGlobals->curtime );
			}
		}
		else
		{
			m_Shared.InitInWorld( gpGlobals->curtime, vecStartPosition, vecStartAngles, 
				vecDirection, m_vecImpactPosition );
			SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterWorldThink );
			SetNextThink( gpGlobals->curtime );
		}
	}

	Vector vecEndPosition;
	QAngle vecEndAngles;
	m_Shared.GetPositionAtTime( gpGlobals->curtime, vecEndPosition, vecEndAngles );
	SetAbsOrigin( vecEndPosition );
	SetAbsAngles( vecEndAngles );

	return pCamera;
}

	
//-----------------------------------------------------------------------------
// Fires the canister!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::InputFireCanister( inputdata_t &inputdata )
{
	if (m_bLaunched)
		return;

	m_bLaunched = true;

	if ( HasSpawnFlags( SF_START_IMPACTED ) )
	{
		StartSpawningHeadcrabs( 0.01f );
		return;
	}

	// Play a firing sound
	CPASAttenuationFilter filter( this, ATTN_NONE );

	if ( !HasSpawnFlags( SF_NO_LAUNCH_SOUND ) )
	{
		EmitSound( filter, entindex(), "HeadcrabCanister.LaunchSound" );
	}

	// Place the canister
	CE_CSkyCamera *pCamera = PlaceCanisterInWorld();

	// Hook up a smoke trail
	CE_CSpriteTrail *trail = CE_CSpriteTrail::SpriteTrailCreate( "sprites/smoke.vmt", GetAbsOrigin(), true );
	trail->SetTransparency( kRenderTransAdd, 224, 224, 255, 255, kRenderFxNone );
	trail->SetAttachment( BaseEntity(), 0 );
	trail->SetStartWidth( 32.0 );
	trail->SetEndWidth( 200.0 );
	trail->SetStartWidthVariance( 15.0f );
	trail->SetTextureResolution( 0.002 );
	trail->SetLifeTime( ENV_HEADCRABCANISTER_TRAIL_TIME );
	trail->SetMinFadeLength( 1000.0f );
	m_hTrail.Set(trail->BaseEntity());

	if ( pCamera && m_Shared.IsInSkybox() )
	{
		m_hTrail->SetSkybox( pCamera->m_skyboxData->origin, pCamera->m_skyboxData->scale );
	}

	// Fire that output!
	m_OnLaunched.Set( BaseEntity(), BaseEntity(), BaseEntity() );
}


//-----------------------------------------------------------------------------
// Opens the canister!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::InputOpenCanister( inputdata_t &inputdata )
{
	if ( m_bLanded && !m_bOpened && HasSpawnFlags( SF_WAIT_FOR_INPUT_TO_OPEN ) )
	{
		OpenCanister();
	}
}


//-----------------------------------------------------------------------------
// Spawns headcrabs
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::InputSpawnHeadcrabs( inputdata_t &inputdata )
{
	if ( m_bLanded && m_bOpened && HasSpawnFlags( SF_WAIT_FOR_INPUT_TO_SPAWN_HEADCRABS ) )
	{
		StartSpawningHeadcrabs( 0.01f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::InputStopSmoke( inputdata_t &inputdata )
{
	if ( m_hSmokeTrail != NULL )
	{
		UTIL_Remove( m_hSmokeTrail );
		m_hSmokeTrail.Set(NULL);
	}
}

//=============================================================================
//
// Enumerator for swept bbox collision.
//
class CCollideList : public IEntityEnumerator
{
public:
	CCollideList( Ray_t *pRay, CBaseEntity* pIgnoreEntity, int nContentsMask ) : 
		m_Entities( 0, 32 ), m_pIgnoreEntity( pIgnoreEntity ),
		m_nContentsMask( nContentsMask ), m_pRay(pRay) {}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		// Don't bother with the ignore entity.
		if ( pHandleEntity == m_pIgnoreEntity )
			return true;

		// Assert( pHandleEntity );

		trace_t tr;
		enginetrace->ClipRayToEntity( *m_pRay, m_nContentsMask, pHandleEntity, &tr );
		if (( tr.fraction < 1.0f ) || (tr.startsolid) || (tr.allsolid))
		{
			CEntity *pEntity = CEntity::Instance( pHandleEntity->GetRefEHandle() );
			m_Entities.AddToTail( pEntity );
		}

		return true;
	}

	CUtlVector<CEntity*>	m_Entities;

private:
	CBaseEntity		*m_pIgnoreEntity;
	int				m_nContentsMask;
	Ray_t			*m_pRay;
};


//-----------------------------------------------------------------------------
// Test for impact!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::TestForCollisionsAgainstEntities( const Vector &vecEndPosition )
{
	// Debugging!!
//	NDebugOverlay::Box( GetAbsOrigin(), m_vecMin * 0.5f, m_vecMax * 0.5f, 255, 255, 0, 0, 5 );
//	NDebugOverlay::Box( vecEndPosition, m_vecMin, m_vecMax, 255, 0, 0, 0, 5 );

	float flRadius = CollisionProp()->BoundingRadius();
	Vector vecMins( -flRadius, -flRadius, -flRadius );
	Vector vecMaxs( flRadius, flRadius, flRadius );

	Ray_t ray;
	ray.Init( GetAbsOrigin(), vecEndPosition, vecMins, vecMaxs );

	CCollideList collideList( &ray, BaseEntity(), MASK_SOLID );
	enginetrace->EnumerateEntities( ray, false, &collideList );

	float flDamage = m_flDamage;

	// Now get each entity and react accordinly!
	for( int iEntity = collideList.m_Entities.Count(); --iEntity >= 0; )
	{
		CEntity *pEntity = collideList.m_Entities[iEntity];
		Vector vecForceDir = m_Shared.m_vecDirection;

		// Check for a physics object and apply force!
		IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
		if ( pPhysObject )
		{
			float flMass = PhysGetEntityMass( pEntity );
			vecForceDir *= flMass * 750;
			pPhysObject->ApplyForceCenter( vecForceDir );
		}

		if ( pEntity->m_takedamage && ( m_flDamage != 0.0f ) )
		{
			CTakeDamageInfo info( BaseEntity(), BaseEntity(), flDamage, DMG_BLAST );
			CalculateExplosiveDamageForce( &info, vecForceDir, pEntity->GetAbsOrigin() );
			pEntity->TakeDamage( info );
		}
	}
}


//-----------------------------------------------------------------------------
// Test for impact!
//-----------------------------------------------------------------------------
#define INNER_RADIUS_FRACTION 0.25f

void CEnvHeadcrabCanister::TestForCollisionsAgainstWorld( const Vector &vecEndPosition )
{
	// Splash damage!
	// Iterate on all entities in the vicinity.
	float flDamageRadius = m_flDamageRadius;
	float flDamage = m_flDamage;

	CEntity *pEntity;
	for ( CEntitySphereQuery sphere( vecEndPosition, flDamageRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity == this )
			continue;

		if ( !pEntity->IsSolid() )
			continue;

		// Get distance to object and use it as a scale value.
		Vector vecSegment;
		VectorSubtract( pEntity->GetAbsOrigin(), vecEndPosition, vecSegment ); 
		float flDistance = VectorNormalize( vecSegment );

		float flFactor = 1.0f / ( flDamageRadius * (INNER_RADIUS_FRACTION - 1) );
		flFactor *= flFactor;
		float flScale = flDistance - flDamageRadius;
		flScale *= flScale * flFactor;
		if ( flScale > 1.0f ) 
		{ 
			flScale = 1.0f; 
		}
		
		// Check for a physics object and apply force!
		Vector vecForceDir = vecSegment;
		IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
		if ( pPhysObject )
		{
			// Send it flying!!!
			float flMass = PhysGetEntityMass( pEntity );
			vecForceDir *= flMass * 750 * flScale;
			pPhysObject->ApplyForceCenter( vecForceDir );
		}

		if ( pEntity->m_takedamage && ( m_flDamage != 0.0f ) )
		{
			CTakeDamageInfo info( BaseEntity(), BaseEntity(), flDamage * flScale, DMG_BLAST );
			CalculateExplosiveDamageForce( &info, vecSegment, pEntity->GetAbsOrigin() );
			pEntity->TakeDamage( info );
		}

		if ( pEntity->IsPlayer() && !(static_cast<CPlayer*>(pEntity)->IsInAVehicle()) )
		{
			if (vecSegment.z < 0.1f)
			{
				vecSegment.z = 0.1f;
				VectorNormalize( vecSegment );					
			}
			float flAmount = SimpleSplineRemapVal( flScale, 0.0f, 1.0f, 250.0f, 1000.0f );
			pEntity->ApplyAbsVelocityImpulse( vecSegment * flAmount );
		}
	}
}

//-----------------------------------------------------------------------------
// Headcrab creation
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::HeadcrabCanisterSpawnHeadcrabThink()
{
	Vector vecSpawnPosition;
	QAngle vecSpawnAngles;

	--m_nHeadcrabCount;

	int nHeadCrabAttachment = LookupAttachment( "headcrab" );
	if ( GetAttachment( nHeadCrabAttachment, vecSpawnPosition, vecSpawnAngles ) )
	{
		CEntity *pEnt = CreateEntityByName( s_pHeadcrabClass[m_nHeadcrabType] );
		CBaseHeadcrab *pHeadCrab = assert_cast<CBaseHeadcrab*>(pEnt);

		// Necessary to get it to eject properly (don't allow the NPC
		// to override the spawn position specified).
		pHeadCrab->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

		// So we don't collide with the canister
		// NOTE: Hierarchical attachment is necessary here to get the animations to work
		pHeadCrab->SetOwnerEntity( BaseEntity() );
		DispatchSpawn( pHeadCrab->BaseEntity() );
		pHeadCrab->SetParent( BaseEntity(), nHeadCrabAttachment );
		pHeadCrab->SetLocalOrigin( vec3_origin );
		pHeadCrab->SetLocalAngles( vec3_angle );
		pHeadCrab->CrawlFromCanister();
	}

	if ( m_nHeadcrabCount != 0 )
	{
		float flWaitTime = enginerandom->RandomFloat( 1.0f, 2.0f );
		SetContextThink( &CEnvHeadcrabCanister::HeadcrabCanisterSpawnHeadcrabThink, gpGlobals->curtime + flWaitTime, s_pHeadcrabThinkContext );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pHeadcrabThinkContext );
	}
}


//-----------------------------------------------------------------------------
// Start spawning headcrabs
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::StartSpawningHeadcrabs( float flDelay )
{
	if ( !m_bLanded || !m_bOpened || m_nHeadcrabCount == 0 )
		return;

	if ( m_nHeadcrabCount != 0 )
	{
		SetContextThink( &CEnvHeadcrabCanister::HeadcrabCanisterSpawnHeadcrabThink, gpGlobals->curtime + flDelay, s_pHeadcrabThinkContext );
	}
}


//-----------------------------------------------------------------------------
// Canister finished opening
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::CanisterFinishedOpening( void )
{
	ResetSequence( LookupSequence( "idle_open" ) );
	m_OnOpened.FireOutput( this, this, 0 );
	m_bOpened = true;
	SetContextThink( NULL, gpGlobals->curtime, s_pOpenThinkContext );

	if ( !HasSpawnFlags( SF_START_IMPACTED ) )
	{
		if ( !HasSpawnFlags( SF_WAIT_FOR_INPUT_TO_SPAWN_HEADCRABS ) )
		{
			StartSpawningHeadcrabs( 3.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Finish the opening sequence
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::WaitForOpenSequenceThink()
{
	StudioFrameAdvance();
	if ( ( GetSequence() == LookupSequence( "open" ) ) && IsSequenceFinished() )
	{
		CanisterFinishedOpening();
	}
	else
	{
		SetContextThink( &CEnvHeadcrabCanister::WaitForOpenSequenceThink, gpGlobals->curtime + 0.01f, s_pOpenThinkContext );
	}
}


//-----------------------------------------------------------------------------
// Open the canister!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::OpenCanister( void )
{
	if ( m_bOpened )
		return;

	int nOpenSequence = LookupSequence( "open" );
	if ( nOpenSequence != ACT_INVALID )
	{
		EmitSound( "HeadcrabCanister.Open" );

		ResetSequence( nOpenSequence );
		SetContextThink( &CEnvHeadcrabCanister::WaitForOpenSequenceThink, gpGlobals->curtime + 0.01f, s_pOpenThinkContext );
	}
	else
	{
		CanisterFinishedOpening();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::SetLanded( void )
{
	SetAbsOrigin( m_vecImpactPosition );
	SetModel( ENV_HEADCRABCANISTER_BROKEN_MODEL );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	VPhysicsInitStatic();
	
	AddEffects( EF_NOINTERP );
	m_bLanded = true;
}

//-----------------------------------------------------------------------------
// Landed!
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::Landed( void )
{
	EmitSound( "HeadcrabCanister.AfterLanding" );

	// Lock us now that we've stopped
	SetLanded();

	// Hook the follow trail to the lead of the canister (which should be buried)
	// to hide problems with the edge of the follow trail
	if (m_hTrail)
	{
		m_hTrail->SetAttachment( BaseEntity(), LookupAttachment("trail") );
	}

	// Start smoke, unless we don't want it
	if ( !HasSpawnFlags( SF_NO_SMOKE ) )
	{
		// Create the smoke trail to obscure the headcrabs
		CSmokeTrail *trail = CSmokeTrail::CreateSmokeTrail();
		trail->FollowEntity( this, "smoke" );

		trail->m_SpawnRate			= 8;
		trail->m_ParticleLifetime	= 2.0f;

		trail->m_StartColor->Init( 0.7f, 0.7f, 0.7f );
		trail->m_EndColor->Init( 0.6, 0.6, 0.6 );

		trail->m_StartSize	= 32;
		trail->m_EndSize	= 64;
		trail->m_SpawnRadius= 8;
		trail->m_MinSpeed	= 0;
		trail->m_MaxSpeed	= 8;
		trail->m_MinDirectedSpeed	= 32;
		trail->m_MaxDirectedSpeed	= 64;
		trail->m_Opacity	= 0.35f;

		trail->SetLifetime( m_flSmokeLifetime );

		m_hSmokeTrail.Set(trail->BaseEntity());
	}

	SetThink( NULL );

	if ( !HasSpawnFlags( SF_WAIT_FOR_INPUT_TO_OPEN ) )
	{
		if ( HasSpawnFlags( SF_START_IMPACTED ) )
		{
			CanisterFinishedOpening( );
		}
		else
		{
			OpenCanister();
		}
	}
}


//-----------------------------------------------------------------------------
// Creates the explosion effect
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::Detonate( )
{
	// Send the impact output
	m_OnImpacted.FireOutput( this, this, 0 );

	if ( !HasSpawnFlags( SF_NO_IMPACT_SOUND ) )
	{
		StopSound( "HeadcrabCanister.IncomingSound" );
		EmitSound( "HeadcrabCanister.Explosion" );
	}

	// If we're supposed to be removed, do that now
	if ( HasSpawnFlags( SF_REMOVE_ON_IMPACT ) )
	{
		SetAbsOrigin( m_vecImpactPosition );
		SetModel( ENV_HEADCRABCANISTER_BROKEN_MODEL );
		SetMoveType( MOVETYPE_NONE );
		AddEffects( EF_NOINTERP );
		m_bLanded = true;
		
		// Become invisible so our trail can finish up
		AddEffects( EF_NODRAW );
		SetSolidFlags( FSOLID_NOT_SOLID );

		SetThink( &CEnvHeadcrabCanister::SUB_Remove );
		SetNextThink( gpGlobals->curtime + ENV_HEADCRABCANISTER_TRAIL_TIME );

		return;
	}

	// Test for damaging things
	TestForCollisionsAgainstWorld( m_vecImpactPosition );

	// Shake the screen unless flagged otherwise
	if ( !HasSpawnFlags( SF_NO_SHAKE ) )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( 1 );

		// If the player is on foot, then do a more limited shake
		float shakeRadius = ( pPlayer && pPlayer->IsInAVehicle() ) ? sk_env_headcrabcanister_shake_radius_vehicle.GetFloat() : sk_env_headcrabcanister_shake_radius.GetFloat();

		UTIL_ScreenShake( m_vecImpactPosition, sk_env_headcrabcanister_shake_amplitude.GetFloat(), 150.0, 1.0, shakeRadius, SHAKE_START );
	}

	// Do explosion effects
	if ( !HasSpawnFlags( SF_NO_IMPACT_EFFECTS ) )
	{
		// Normal explosion
		ExplosionCreate( m_vecImpactPosition, GetAbsAngles(), BaseEntity(), 50, 500, 
			SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSOUND, 1300.0f );
			
		// Dust explosion
		//CE_TODO
		/*AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( m_vecImpactPosition );
		
		if( pExplosion )
		{
			pExplosion->SetLifetime(10);
		}*/
	}
}


//-----------------------------------------------------------------------------
// Purpose: This think function simulates (moves/collides) the HeadcrabCanister while in
//          the world.
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::HeadcrabCanisterWorldThink( void )
{
	// Get the current time.
	float flTime = gpGlobals->curtime;

	Vector vecStartPosition = GetAbsOrigin();

	// Update HeadcrabCanister position for swept collision test.
	Vector vecEndPosition;
	QAngle vecEndAngles;
	m_Shared.GetPositionAtTime( flTime, vecEndPosition, vecEndAngles );

	if ( !m_bIncomingSoundStarted && !HasSpawnFlags( SF_NO_IMPACT_SOUND ) )
	{
		float flDistSq = ENV_HEADCRABCANISTER_INCOMING_SOUND_TIME * m_Shared.m_flFlightSpeed;
		flDistSq *= flDistSq;
		if ( vecEndPosition.DistToSqr(m_vecImpactPosition) <= flDistSq )
		{
			// Figure out if we're close enough to play the incoming sound
			EmitSound( "HeadcrabCanister.IncomingSound" );
			m_bIncomingSoundStarted = true;
		}
	}

	TestForCollisionsAgainstEntities( vecEndPosition );
	if ( m_Shared.DidImpact( flTime ) )
	{
		if ( !m_bHasDetonated )
		{
			Detonate();
			m_bHasDetonated = true;
		}
		
		if ( !HasSpawnFlags( SF_REMOVE_ON_IMPACT ) )
		{
			Landed();
		}

		return;
	}
		   
	// Always move full movement.
	SetAbsOrigin( vecEndPosition );

	// Touch triggers along the way
	PhysicsTouchTriggers( &vecStartPosition );

	SetNextThink( gpGlobals->curtime + 0.2f );
	SetAbsAngles( vecEndAngles );

	if ( !m_bHasDetonated )
	{
		if ( vecEndPosition.DistToSqr( m_vecImpactPosition ) < BoundingRadius() * BoundingRadius() )
		{
			Detonate();
			m_bHasDetonated = true;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: This think function should be called at the time when the HeadcrabCanister 
//          will be leaving the skybox and entering the world.
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::HeadcrabCanisterSkyboxThink( void )
{
	// Use different position computation
	m_Shared.ConvertFromSkyboxToWorld();

	Vector vecEndPosition;
	QAngle vecEndAngles;
	m_Shared.GetPositionAtTime( gpGlobals->curtime, vecEndPosition, vecEndAngles );
	UTIL_SetOrigin( this, vecEndPosition );
	SetAbsAngles( vecEndAngles );
	RemoveEFlags( EFL_IN_SKYBOX );

	// Switch to the actual-scale model
	SetupWorldModel();

	// Futz with the smoke trail to get it working across the boundary
	m_hTrail->SetSkybox( vec3_origin, 1.0f );

	// Now we start looking for collisions
	SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterWorldThink );
	SetNextThink( gpGlobals->curtime + 0.01f );
}


//-----------------------------------------------------------------------------
// Purpose: This stops its motion in the skybox
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::HeadcrabCanisterSkyboxOnlyThink( void )
{
	Vector vecEndPosition;
	QAngle vecEndAngles;
	m_Shared.GetPositionAtTime( gpGlobals->curtime, vecEndPosition, vecEndAngles );
	UTIL_SetOrigin( this, vecEndPosition );
	SetAbsAngles( vecEndAngles );

	if ( !HasSpawnFlags( SF_NO_IMPACT_SOUND ) )
	{	
		CPASAttenuationFilter filter( this, ATTN_NONE );
		EmitSound( filter, entindex(), "HeadcrabCanister.SkyboxExplosion" );
	}

	if ( m_nSkyboxCannisterCount != 0 )
	{
		if ( --m_nSkyboxCannisterCount <= 0 )
		{
			SetThink( NULL );
			return;
		}
	}

	float flRefireTime = enginerandom->RandomFloat( m_flMinRefireTime, m_flMaxRefireTime ) + ENV_HEADCRABCANISTER_TRAIL_TIME;
	SetThink( &CEnvHeadcrabCanister::HeadcrabCanisterSkyboxRestartThink );
	SetNextThink( gpGlobals->curtime + flRefireTime );
}


//-----------------------------------------------------------------------------
// This will re-fire the headcrab cannister
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::HeadcrabCanisterSkyboxRestartThink( void )
{
	if ( m_hTrail )
	{
		UTIL_Remove( m_hTrail );
		m_hTrail.Set(NULL);
	}

	m_bLaunched = false;

	inputdata_t data;
	InputFireCanister( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInfo - 
//			bAlways - 
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanister::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Make our smoke trail always come with us
	if ( m_hSmokeTrail )
	{
		m_hSmokeTrail->SetTransmit( pInfo, bAlways );
	}
}
