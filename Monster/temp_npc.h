
#ifndef TEMP_NPC_H
#define TEMP_NPC_H
#ifdef _WIN32
#pragma once
#endif

abstract_class CE_Temp_NPC : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS(CE_Temp_NPC, CE_Cycler_Fix);

	virtual void Spawn()
	{
		Precache();
		BaseClass::Spawn();
		m_iHealth = 175;
		SetModel(STRING(GetModelName()));


		// JAY: Disabled jump for now - hard to compare to HL1
		CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB );
		CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
		CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT );
		CapabilitiesAdd( bits_CAP_DUCK | bits_CAP_DOORS_GROUP );
		CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );

		CapabilitiesAdd( bits_CAP_AIM_GUN );
		CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER | bits_CAP_NO_HIT_SQUADMATES );

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
	//-----------------------------------------------------------------------------
	// Purpose: degrees to turn in 0.1 seconds
	//-----------------------------------------------------------------------------
	float MaxYawSpeed( void )
	{
		switch( GetActivity() )
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 45;
			break;
		case ACT_RUN:
		case ACT_RUN_HURT:
			return 15;
			break;
		case ACT_WALK:
		case ACT_WALK_CROUCH:
			return 25;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 35;
		default:
			return 35;
			break;
		}
	}
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info )
	{
		CTakeDamageInfo subInfo = info;
		// Vital allies never take more than 25% of their health in a single hit (except for physics damage)
		// Don't do damage reduction for DMG_GENERIC. This allows SetHealth inputs to still do full damage.
		if ( subInfo.GetDamageType() != DMG_GENERIC )
		{
			float flDamage = subInfo.GetDamage();
			if ( Classify() == CLASS_PLAYER_ALLY_VITAL || Classify() == CLASS_PLAYER_ALLY ) {
				CEntity *attacker = CEntity::Instance(info.GetAttacker());
				if (attacker->GetFlags() & FL_CLIENT && attacker->GetTeamNumber() == 2) {
					flDamage = 0;	
					subInfo.SetDamage( flDamage );
				}
			}
			if ( Classify() == CLASS_PLAYER_ALLY_VITAL && !(subInfo.GetDamageType() & DMG_CRUSH) )
			{
				if ( flDamage > ( GetMaxHealth() * 0.25 ) )
				{
					flDamage = ( GetMaxHealth() * 0.25 );
					subInfo.SetDamage( flDamage );
				}
			}
		}

		return BaseClass::OnTakeDamage_Alive( subInfo );
	}
	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}

	bool HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt);

	// Set up the shot regulator based on the equipped weapon
	virtual void OnUpdateShotRegulator( );
	virtual int TranslateSchedule( int scheduleType );

	// Tasks
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual void BuildScheduleTestBits( );
	virtual void		AimGun();
 
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

#endif