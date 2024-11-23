
#include "CPlayer.h"
#include "CCombatWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "pickup.h"
#include "CPhysBox.h"
#include "CPhysicsProp.h"
#include "eventqueue.h"
#include "ladder.h"
#include "CViewModel.h"
#include "weapon_rpg_replace.h"
#include "combine_mine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "weapon_physcannon_replace.h"

#ifndef MAX_FOV
#define MAX_FOV	90
#endif

CE_LINK_ENTITY_TO_CLASS(CBasePlayer, CPlayer);

extern ConVar *sv_suppress_viewpunch;
extern ConVar sk_battery;
extern ConVar sk_battery_max;



SH_DECL_MANUALHOOK2(GiveNamedItem, 0, 0, 0, CBaseEntity *,const char *, int);
DECLARE_HOOK(GiveNamedItem, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, GiveNamedItem, CBaseEntity *, (const char *szName, int iSubType), (szName, iSubType));

SH_DECL_MANUALHOOK0_void(PreThink, 0, 0, 0);
DECLARE_HOOK(PreThink, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, PreThink,  (), ());

SH_DECL_MANUALHOOK0_void(PostThink, 0, 0, 0);
DECLARE_HOOK(PostThink, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, PostThink,  (), ());

SH_DECL_MANUALHOOK0(FindUseEntity, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(FindUseEntity, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, FindUseEntity, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK2(IsUseableEntity, 0, 0, 0, bool, CBaseEntity *, unsigned int );
DECLARE_HOOK(IsUseableEntity, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, IsUseableEntity, bool,  (CBaseEntity *pEntity, unsigned int requiredCaps), (pEntity, requiredCaps));

SH_DECL_MANUALHOOK1(BumpWeapon, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(BumpWeapon, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, BumpWeapon, bool, (CBaseEntity *pWeapon), (pWeapon));

SH_DECL_MANUALHOOK2_void(PickupObject, 0, 0, 0, CBaseEntity *, bool);
DECLARE_HOOK(PickupObject, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, PickupObject, (CBaseEntity *pObject, bool bLimitMassAndSize), (pObject, bLimitMassAndSize));

SH_DECL_MANUALHOOK1(GetAutoaimVector_Float, 0, 0, 0, Vector, float);
DECLARE_HOOK(GetAutoaimVector_Float, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_SPECIAL(Hooked_CPlayer, GetAutoaimVector_Float, Vector,  (float flScale), (flScale), vec3_origin);

SH_DECL_MANUALHOOK1_void(ForceDropOfCarriedPhysObjects, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(ForceDropOfCarriedPhysObjects, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, ForceDropOfCarriedPhysObjects, (CBaseEntity *pOnlyIfHoldingThis), (pOnlyIfHoldingThis));

SH_DECL_MANUALHOOK1_void(ModifyOrAppendPlayerCriteria, 0, 0, 0, AI_CriteriaSet&);
DECLARE_HOOK(ModifyOrAppendPlayerCriteria, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, ModifyOrAppendPlayerCriteria, (AI_CriteriaSet& set), (set));

SH_DECL_MANUALHOOK0_void(CreateRagdollEntity, 0, 0, 0);
DECLARE_HOOK(CreateRagdollEntity, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, CreateRagdollEntity,  (), ());

SH_DECL_MANUALHOOK0(EntSelectSpawnPoint, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(EntSelectSpawnPoint, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, EntSelectSpawnPoint, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK2_void(LeaveVehicle, 0, 0, 0, const Vector &, const QAngle &);
DECLARE_HOOK(LeaveVehicle, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, LeaveVehicle, (const Vector &vecExitPoint, const QAngle &vecExitAngles), (vecExitPoint, vecExitAngles));

SH_DECL_MANUALHOOK1_void(SetAnimation, 0, 0, 0, PLAYER_ANIM);
DECLARE_HOOK(SetAnimation, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER_void(Hooked_CPlayer, SetAnimation, (PLAYER_ANIM anim), (anim));

SH_DECL_MANUALHOOK2(GetInVehicle, 0, 0, 0, bool, IServerVehicle *, int );
DECLARE_HOOK(GetInVehicle, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, GetInVehicle, bool, (IServerVehicle *pVehicle, int nRole), (pVehicle, nRole));

SH_DECL_MANUALHOOK0(FlashlightIsOn, 0, 0, 0, int);
DECLARE_HOOK(FlashlightIsOn, Hooked_CPlayer);
DECLARE_DEFAULTHANDLER(Hooked_CPlayer, FlashlightIsOn, int, (), ());

//Sendprops
DEFINE_PROP(m_vecPunchAngle, CPlayer);
DEFINE_PROP(m_bWearingSuit, CPlayer);
DEFINE_PROP(m_DmgSave, CPlayer);
DEFINE_PROP(m_hUseEntity, CPlayer);
DEFINE_PROP(m_nOldButtons, CPlayer);
DEFINE_PROP(m_nPoisonDmg, CPlayer);
DEFINE_PROP(m_flFallVelocity, CPlayer);
DEFINE_PROP(m_iObserverMode, CPlayer);
DEFINE_PROP(m_hObserverTarget, CPlayer);
DEFINE_PROP(m_hVehicle, CPlayer);
DEFINE_PROP(m_iDefaultFOV, CPlayer);
DEFINE_PROP(m_iFOV, CPlayer);
DEFINE_PROP(m_flFOVTime, CPlayer);
DEFINE_PROP(m_iFOVStart, CPlayer);

DEFINE_PROP(m_iHideHUD, CPlayer);
DEFINE_PROP(m_flFOVRate, CPlayer);

//Datamaps
DEFINE_PROP(pl, CPlayer);
DEFINE_PROP(m_nButtons, CPlayer);
DEFINE_PROP(m_afButtonPressed, CPlayer);
DEFINE_PROP(m_afButtonReleased, CPlayer);
DEFINE_PROP(m_afPhysicsFlags, CPlayer);
DEFINE_PROP(m_iTrain, CPlayer);
DEFINE_PROP(m_tbdPrev, CPlayer);
DEFINE_PROP(m_iVehicleAnalogBias, CPlayer);


BEGIN_DATADESC( CPlayer )

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDropPhysObjects", InputForceDropPhysObjects ),

END_DATADESC()

void CPlayer::Spawn()
{
	m_bOnLadder = false;
	m_bHaveRPG = false;
	m_nVehicleViewSavedFrame = 0;
	BaseClass::Spawn();

	GiveNamedItem("item_kevlar");
	GiveNamedItem("item_assaultsuit");
	
	SetArmorValue(0);
	SetMuzzleFlashTime(0);
}

void CPlayer::FixHL2Ladder()
{
	m_bOnLadder = false;
	CFuncLadder *ladder = (CFuncLadder *)g_helpfunc.FindEntityByClassname((CBaseEntity *)NULL, "func_useableladder");
	while(ladder) {
		if(ladder->IsPlayerOnLadder(this))
		{
			m_bOnLadder = true;
			break;
		}
		ladder = (CFuncLadder*)g_helpfunc.FindEntityByClassname(ladder, "func_useableladder");
	}
}

void CPlayer::PreThink()
{
	FixHL2Ladder();
	BaseClass::PreThink();
}

void CPlayer::PostThink()
{
	BaseClass::PostThink();
}

void CPlayer::ViewPunchReset( float tolerance )
{
	if ( tolerance != 0 )
	{
		tolerance *= tolerance;	// square
		float check = m_vecPunchAngleVel->LengthSqr() + m_vecPunchAngle->LengthSqr();
		if ( check > tolerance )
			return;
	}
	m_vecPunchAngle = vec3_angle;
	m_vecPunchAngleVel = vec3_angle;
}

void CPlayer::ViewPunch( const QAngle &angleOffset )
{
	//See if we're suppressing the view punching
	if ( sv_suppress_viewpunch->GetBool() )
		return;

	// We don't allow view kicks in the vehicle
	if ( IsInAVehicle() )
		return;

	QAngle *angle = m_vecPunchAngle.ptr;
	angle->x += (angleOffset.x * 2);
	angle->y += (angleOffset.y * 2);
	angle->z += (angleOffset.z * 2);
}

void CPlayer::VelocityPunch( const Vector &vecForce )
{
	// Clear onground and add velocity.
	SetGroundEntity( NULL );
	ApplyAbsVelocityImpulse(vecForce );
}


void CPlayer::EyeVectors( Vector *pForward, Vector *pRight, Vector *pUp )
{
	if ( GetVehicle() != NULL )
	{
		// Cache or retrieve our calculated position in the vehicle
		CacheVehicleView();
		AngleVectors( m_vecVehicleViewAngles, pForward, pRight, pUp );
	}
	else
	{
		AngleVectors( EyeAngles(), pForward, pRight, pUp );
	}
}

void CPlayer::CacheVehicleView( void )
{
	// If we've calculated the view this frame, then there's no need to recalculate it
	if ( m_nVehicleViewSavedFrame == gpGlobals->framecount )
		return;

	IServerVehicle *pVehicle = GetVehicle();

	if ( pVehicle != NULL )
	{		
		int nRole = pVehicle->GetPassengerRole( BaseEntity() );

		// Get our view for this frame
		pVehicle->GetVehicleViewPosition( nRole, &m_vecVehicleViewOrigin, &m_vecVehicleViewAngles, &m_flVehicleViewFOV );
		m_nVehicleViewSavedFrame = gpGlobals->framecount;
	}
}

void CPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
{
	bool ret = g_helpfunc.GameRules_FPlayerCanTakeDamage(BaseEntity(), info.GetAttacker(), info);
	if(!ret)
		return;

	BaseClass::TraceAttack(info, vecDir, ptr, pAccumulator);
}

int CPlayer::OnTakeDamage(const CTakeDamageInfo& info)
{
	CTakeDamageInfo newinfo = info;
	CEntity *cent_attacker = CEntity::Instance(info.GetAttacker());
	if(cent_attacker)
	{
		CAI_NPC *npc = cent_attacker->MyNPCPointer();
	}

	if(info.GetDamageType() & DMG_FALL)
	{
		if(info.GetDamage() > 10)
		{
			newinfo.SetDamage(10.0f);
		}
	}

	int result = BaseClass::OnTakeDamage(newinfo);
	if (result)
	{
		// DMG_POISON is not handled because CBasePlayer::OnTakeDamage is never called.
		// We handle it ourselves because some NPCs deal poison damage.
		if (newinfo.GetDamageType() & DMG_POISON)
		{
			m_nPoisonDmg += (int) newinfo.GetDamage();
			m_tbdPrev = gpGlobals->curtime;
		}
	}
	return result;
}

/*Vector CPlayer::GetSmoothedVelocity( void )
{
	if (IsInAVehicle())
	{
		CEntity *vehicle = m_hVehicle;
		if (stricmp(vehicle->GetClassname(), "prop_vehicle_prisoner_pod") == 0)
		{
			return vehicle->GetVelocity();
		}
	}
	return BaseClass ::GetSmoothedVelocity();
}*/

void CPlayer::Weapon_Drop( CBaseEntity *pWeapon, const Vector *pvecTarget , const Vector *pVelocity )
{
	BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
	CCombatWeapon *cent_pWeapon = (CCombatWeapon *)CEntity::Instance(pWeapon);
	if(cent_pWeapon)
	{
		cent_pWeapon->OnWeaponDrop(this);
	}
}

void CPlayer::Weapon_Equip( CBaseEntity *pWeapon )
{
	BaseClass::Weapon_Equip(pWeapon);

	CCombatWeapon *cent_pWeapon = (CCombatWeapon *)CEntity::Instance(pWeapon);
	if(cent_pWeapon)
	{
		cent_pWeapon->OnWeaponEquip(this);
	}
}

bool CPlayer::ApplyBattery( float powerMultiplier )
{
	if(!IsSuitEquipped())
		m_bWearingSuit = true;

	if ((ArmorValue() < sk_battery_max.GetInt()))
	{
		TakeHealth( (int)(sk_battery.GetFloat() * powerMultiplier), DMG_GENERIC );

		CPASAttenuationFilter filter( this, "ItemBattery.Touch" );
		EmitSound( filter, entindex(), "ItemBattery.Touch" );
	
		return true;		
	}
	return false;
}

void CPlayer::IncrementArmorValue( int nCount, int nMaxValue )
{ 
	TakeHealth(nCount, DMG_GENERIC);
}

bool CPlayer::HasAnyAmmoOfType( int nAmmoIndex )
{
	if ( nAmmoIndex < 0 )
		return false;

	// If we have some in reserve, we're already done
	if ( GetAmmoCount( nAmmoIndex ) )
		return true;


	CCombatWeapon *pWeapon;

	// Check all held weapons
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );

		if ( !pWeapon )
			continue;

		if ( pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
		{
			return true;
		}
	}	

	// We're completely without this type of ammo
	return false;
}

int CPlayer::GiveAmmo(int nCount, int nAmmoIndex, bool bSuppressSound)
{
	// Don't try to give the player invalid ammo indices.
	if (nAmmoIndex < 0)
		return 0;

	bool bCheckAutoSwitch = false;
	if (!HasAnyAmmoOfType(nAmmoIndex))
	{
		bCheckAutoSwitch = true;
	}

	int nAdd = BaseClass::GiveAmmo(nCount, nAmmoIndex, bSuppressSound);

	if ( nCount > 0 && nAdd == 0 )
	{
		// we've been denied the pickup, display a hud icon to show that
		CSingleUserRecipientFilter user( this );
		//user.MakeReliable();

		int msgid = usermsgs->GetMessageIndex("AmmoDenied");
		if (msgid != -1) {
			cell_t players[1] = { entindex() };
			bf_write *bf = usermsgs->StartBitBufMessage(msgid, players, 1, 0);
			// Assert(bf);
			bf->WriteShort(nAmmoIndex);
			usermsgs->EndMessage();
		}
	}

	//
	// If I was dry on ammo for my best weapon and just picked up ammo for it,
	// autoswitch to my best weapon now.
	//
	if (bCheckAutoSwitch)
	{
		CEntity *cent = GetActiveWeapon();
		CCombatWeapon *pWeapon = g_helpfunc.GameRules_GetNextBestWeapon(BaseEntity(), (cent)?cent->BaseEntity():NULL);

		if ( pWeapon && pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
		{
			SwitchToNextBestWeapon(GetActiveWeapon());
		}
	}

	return nAdd;
}

bool CPlayer::IsHoldingEntity( CEntity *pEntity )
{
	return PlayerPickupControllerIsHoldingEntity( m_hUseEntity, pEntity );
}

float IntervalDistance( float x, float x0, float x1 )
{
	// swap so x0 < x1
	if ( x0 > x1 )
	{
		float tmp = x0;
		x0 = x1;
		x1 = tmp;
	}

	if ( x < x0 )
		return x0-x;
	else if ( x > x1 )
		return x - x1;
	return 0;
}

CBaseEntity *CPlayer::FindUseEntity()
{
	CBaseEntity *ret = BaseClass::FindUseEntity();
	if(ret == NULL)
		ret = FindUseEntity_Fix();
	return ret;
}

CBaseEntity *CPlayer::FindUseEntity_Fix()
{
	Vector forward, up;
	EyeVectors( &forward, NULL, &up );

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchCenter = EyePosition();

	// NOTE: Some debris objects are useable too, so hit those as well
	// A button, etc. can be made out of clip brushes, make sure it's +useable via a traceline, too.
	int useableContents = MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_PLAYERCLIP;

	CEntity *pFoundByTrace = NULL;

	// UNDONE: Might be faster to just fold this range into the sphere query
	CEntity *pObject = NULL;

	float nearestDist = FLT_MAX;
	// try the hit entity if there is one, or the ground entity if there isn't.
	CEntity *pNearest = NULL;

	const int NUM_TANGENTS = 8;
	// trace a box at successive angles down
	//							forward, 45 deg, 30 deg, 20 deg, 15 deg, 10 deg, -10, -15
	const float tangents[NUM_TANGENTS] = { 0, 1, 0.57735026919f, 0.3639702342f, 0.267949192431f, 0.1763269807f, -0.1763269807f, -0.267949192431f };
	for ( int i = 0; i < NUM_TANGENTS; i++ )
	{
		if ( i == 0 )
		{
			UTIL_TraceLine( searchCenter, searchCenter + forward * 1024, useableContents, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		}
		else
		{
			Vector down = forward - tangents[i]*up;
			VectorNormalize(down);
			UTIL_TraceHull( searchCenter, searchCenter + down * 72, -Vector(16,16,16), Vector(16,16,16), useableContents, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		}
		pObject = CEntity::Instance(tr.m_pEnt);

		pFoundByTrace = pObject;

		bool bUsable = IsUseableEntity(tr.m_pEnt, 0);
		while ( pObject && !bUsable && pObject->GetMoveParent() )
		{
			pObject = pObject->GetMoveParent();
			bUsable = IsUseableEntity((pObject)?pObject->BaseEntity():NULL, 0);
		}

		if ( bUsable )
		{
			Vector delta = tr.endpos - tr.startpos;
			float centerZ = WorldSpaceCenter().z;
			delta.z = IntervalDistance( tr.endpos.z, centerZ + CollisionProp_Actual()->OBBMins().z, centerZ + CollisionProp_Actual()->OBBMaxs().z );
			float dist = delta.Length();
			if ( dist < PLAYER_USE_RADIUS )
			{
				if ( pObject->MyNPCPointer() && pObject->MyNPCPointer()->IsPlayerAlly( BaseEntity() ) )
				{
					// If about to select an NPC, do a more thorough check to ensure
					// that we're selecting the right one from a group.
					pObject = DoubleCheckUseNPC( pObject, searchCenter, forward );
				}

				pNearest = pObject;
				
				// if this is directly under the cursor just return it now
				if ( i == 0 )
					return (pObject)?pObject->BaseEntity():NULL;
			}
		}
	}

	// check ground entity first
	// if you've got a useable ground entity, then shrink the cone of this search to 45 degrees
	// otherwise, search out in a 90 degree cone (hemisphere)
	if ( GetGroundEntity() && IsUseableEntity(GetGroundEntity()->BaseEntity(), FCAP_USE_ONGROUND) )
	{
		pNearest = GetGroundEntity();
	}
	if ( pNearest )
	{
		// estimate nearest object by distance from the view vector
		Vector point;
		pNearest->CollisionProp()->CalcNearestPoint( searchCenter, &point );
		nearestDist = CalcDistanceToLine( point, searchCenter, forward );
	}

	for ( CEntitySphereQuery sphere( searchCenter, PLAYER_USE_RADIUS ); ( pObject = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pObject )
			continue;

		if ( !IsUseableEntity( pObject->BaseEntity(), FCAP_USE_IN_RADIUS ) )
			continue;

		// see if it's more roughly in front of the player than previous guess
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );

		Vector dir = point - searchCenter;
		VectorNormalize(dir);
		float dot = DotProduct( dir, forward );

		// Need to be looking at the object more or less
		if ( dot < 0.8 )
			continue;

		float dist = CalcDistanceToLine( point, searchCenter, forward );

		if ( dist < nearestDist )
		{
			// Since this has purely been a radius search to this point, we now
			// make sure the object isn't behind glass or a grate.
			trace_t trCheckOccluded;
			UTIL_TraceLine( searchCenter, point, useableContents, BaseEntity(), COLLISION_GROUP_NONE, &trCheckOccluded );

			if ( trCheckOccluded.fraction == 1.0 || trCheckOccluded.m_pEnt == pObject->BaseEntity() )
			{
				pNearest = pObject;
				nearestDist = dist;
			}
		}
	}

	if ( !pNearest )
	{
		// Haven't found anything near the player to use, nor any NPC's at distance.
		// Check to see if the player is trying to select an NPC through a rail, fence, or other 'see-though' volume.
		trace_t trAllies;
		UTIL_TraceLine( searchCenter, searchCenter + forward * PLAYER_USE_RADIUS, MASK_OPAQUE_AND_NPCS, BaseEntity(), COLLISION_GROUP_NONE, &trAllies );

		CEntity *cent = CEntity::Instance(trAllies.m_pEnt);
		CAI_NPC *npc = (cent)?cent->MyNPCPointer():NULL;
		if ( npc && IsUseableEntity( trAllies.m_pEnt, 0 ) && npc->IsPlayerAlly( BaseEntity() ) )
		{
			// This is an NPC, take it!
			pNearest = cent;
		}
	}

	if ( pNearest && pNearest->MyNPCPointer() && pNearest->MyNPCPointer()->IsPlayerAlly( BaseEntity() ) )
	{
		pNearest = DoubleCheckUseNPC( pNearest, searchCenter, forward );
	}

	return (pNearest)?pNearest->BaseEntity():NULL;
}

CEntity *CPlayer::DoubleCheckUseNPC( CEntity *pNPC, const Vector &vecSrc, const Vector &vecDir )
{
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );
	
	CEntity *cent = CEntity::Instance(tr.m_pEnt);
	CAI_NPC *npc = (cent)?cent->MyNPCPointer():NULL;
	if( npc != NULL && npc != pNPC )
	{
		// Player is selecting a different NPC through some negative space
		// in the first NPC's hitboxes (between legs, over shoulder, etc).
		return npc;
	}

	return pNPC;
}

CCombatWeapon *CPlayer::GetRPGWeapon()
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CWeaponRPG *weapon = ToCWeaponRPG(GetWeapon(i));
		if ( weapon )
		{
			return weapon;
		}
	}
	return NULL;
}

CBaseEntity	*CPlayer::GiveNamedItem( const char *szName, int iSubType)
{
	const char *new_name = GetWeaponReplaceName(szName);
	if(new_name == NULL)
	{
		return BaseClass::GiveNamedItem(szName);
	} else {
		PreWeaponReplace(szName);
		CBaseEntity *pEntity =  BaseClass::GiveNamedItem(new_name);
		PostWeaponReplace();
		return pEntity;
	}
}

bool CPlayer::Weapon_CanSwitchTo(CBaseEntity *pWeapon)
{
	CWeaponRPG *rpg = ToCWeaponRPG(pWeapon);
	if(rpg)
	{
		if(rpg->GetMissile() != NULL)
			return false;
	}
	return BaseClass::Weapon_CanSwitchTo(pWeapon);
}

bool CPlayer::Weapon_CanUse(CBaseEntity *pWeapon)
{
	CWeaponPhysCannon *physcannon = ToCWeaponPhysCannon(pWeapon);
	if(physcannon)
	{
		return true;
	}

	return BaseClass::Weapon_CanUse(pWeapon);
}

int CPlayer::FlashlightIsOn()
{
	return IsEffectActive( EF_DIMLIGHT );
}

void CPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	CEntity *cent = CEntity::Instance(pObject);
	// can't pick up what you're standing on
	if ( GetGroundEntity() == cent )
		return;
	
	if ( bLimitMassAndSize == true )
	{
		if ( CPlayer::CanPickupObject( cent, 35, 128 ) == false )
			 return;
	}

	// Can't be picked up if NPCs are on me
	if ( cent->HasNPCsOnIt() )
		return;

	PlayerPickupObject( this, cent );
}


bool CPlayer::CanPickupObject( CEntity *pObject, float massLimit, float sizeLimit )
{
	// UNDONE: Make this virtual and move to HL2 player

	//Must be valid
	if ( pObject == NULL )
		return false;

	//Must move with physics
	if ( pObject->GetMoveType() != MOVETYPE_VPHYSICS )
		return false;

	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pObject->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );

	//Must have a physics object
	if (!count)
		return false;

	float objectMass = 0;
	bool checkEnable = false;
	for ( int i = 0; i < count; i++ )
	{
		objectMass += pList[i]->GetMass();
		if ( !pList[i]->IsMoveable() )
		{
			checkEnable = true;
		}
		if ( pList[i]->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP )
			return false;
		if ( pList[i]->IsHinged() )
			return false;
	}


	//Msg( "Target mass: %f\n", pPhys->GetMass() );

	//Must be under our threshold weight
	if ( massLimit > 0 && objectMass > massLimit )
		return false;

	if ( checkEnable )
	{
		CBounceBomb *pBomb = dynamic_cast<CBounceBomb*>(pObject);
		if( pBomb )
			return true;

		// Allow pickup of phys props that are motion enabled on player pickup
		CE_CPhysicsProp *pProp = dynamic_cast<CE_CPhysicsProp*>(pObject);
		CE_CPhysBox *pBox = dynamic_cast<CE_CPhysBox*>(pObject);
		if ( !pProp && !pBox )
			return false;

		if ( pProp && !(pProp->HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON )) )
			return false;

		if ( pBox && !(pBox->HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON )) )
			return false;
	}

	if ( sizeLimit > 0 )
	{
		const Vector &size = pObject->CollisionProp_Actual()->OBBSize();
		if ( size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit )
			return false;
	}

	return true;
}

void CPlayer::InputForceDropPhysObjects( inputdata_t &data )
{
	ForceDropOfCarriedPhysObjects( data.pActivator );
}

void CPlayer::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis)
{
	ForceDropOfCarriedPhysObjects(CEntity::Instance(pOnlyIfHoldingThis));
}


void CPlayer::RumbleEffect( unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags )
{
	g_helpfunc.CBasePlayer_RumbleEffect(BaseEntity(), index, rumbleData, rumbleFlags);
}


void CPlayer::ShowCrosshair( bool bShow )
{
	if ( bShow )
	{
		m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}
	else
	{
		m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}
}

bool CPlayer::CanEnterVehicle( IServerVehicle *pVehicle, int nRole )
{
	// Must not have a passenger there already
	if ( pVehicle->GetPassenger( nRole ) )
		return false;

	// Must be able to holster our current weapon (ie. grav gun!)
	if (!pVehicle->IsPassengerUsingStandardWeapons(nRole))
	{
		//Must be able to stow our weapon
		CCombatWeapon *pWeapon = GetActiveWeapon();
		if (pWeapon && !pWeapon->CanHolster())
			return false;
	}

	// Must be alive
	if (!IsAlive())
		return false;

	// Can't be pulled by a barnacle
	if ( IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
		return false;

	return true;
}

int	CPlayer::GetDefaultFOV( void )
{
	int iFOV = ( m_iDefaultFOV == 0 ) ? 90 : m_iDefaultFOV;
	if ( iFOV > MAX_FOV )
		iFOV = MAX_FOV;
	return iFOV;
}

int CPlayer::GetFOV( void )
{
	int nDefaultFOV;

	// The vehicle's FOV wins if we're asking for a default value
	if ( GetVehicle() )
	{
		CacheVehicleView();
		nDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : (int) m_flVehicleViewFOV;
	}
	else
	{
		nDefaultFOV = GetDefaultFOV();
	}

	int fFOV = ( m_iFOV == 0 ) ? nDefaultFOV : m_iFOV;

	// If it's immediate, just do it
	if ( m_flFOVRate == 0.0f )
		return fFOV;

	float deltaTime = (float)( gpGlobals->curtime - m_flFOVTime ) / m_flFOVRate;

	if ( deltaTime >= 1.0f )
	{
		//If we're past the zoom time, just take the new value and stop lerping
		m_iFOVStart = fFOV;
	}
	else
	{
		fFOV = (int)SimpleSplineRemapValClamped( deltaTime, 0.0f, 1.0f, m_iFOVStart, fFOV );
	}

	return fFOV;
}

void CPlayer::ForceDropOfCarriedPhysObjects( CEntity *pOnlyIfHoldingThis)
{
	if(!pOnlyIfHoldingThis || pOnlyIfHoldingThis != m_hHoldEntity)
		return;

	if ( PhysIsInCallback() )
	{
		variant_t value;
		g_CEventQueue->AddEvent( BaseEntity(), "ForceDropPhysObjects", value, 0.01f, pOnlyIfHoldingThis->BaseEntity(), BaseEntity() );
		return;
	}

	ClearUseEntity();
}

bool CPlayer::ClearUseEntity()
{
	if ( m_hUseEntity != NULL )
	{
		// Stop controlling the train/object
		// TODO: Send HUD Update
		m_hUseEntity->Use( BaseEntity(), BaseEntity(), USE_OFF, 0 );
		m_hUseEntity.ptr->Set(NULL);
		return true;
	}

	return false;
}

void CPlayer::EyePositionAndVectors( Vector *pPosition, Vector *pForward,
									 Vector *pRight, Vector *pUp )
{
	// Handle the view in the vehicle
	if ( GetVehicle() != NULL )
	{
		CacheVehicleView();
		AngleVectors( m_vecVehicleViewAngles, pForward, pRight, pUp );

		if ( pPosition != NULL )
		{
			*pPosition = m_vecVehicleViewOrigin;
		}
	}
	else
	{
		VectorCopy( BaseClass::EyePosition(), *pPosition );
		AngleVectors( EyeAngles(), pForward, pRight, pUp );
	}
}

void CPlayer::ExitLadder()
{
	if ( MOVETYPE_LADDER != GetMoveType() )
		return;

	SetMoveType( MOVETYPE_WALK );
	SetMoveCollide( MOVECOLLIDE_DEFAULT );
}

void CPlayer::PlayUseDenySound()
{
	EmitSound("HL2Player.UseDeny");
}

CViewModel *CPlayer::GetViewModel( int index )
{
	//assert(index >= 0);
	CBaseEntity *pBaseVM = g_helpfunc.CBasePlayer_GetViewModel(BaseEntity(), index);
	if(!pBaseVM)
		return NULL;

	return static_cast<CViewModel*>(CEntity::Instance(pBaseVM));
}