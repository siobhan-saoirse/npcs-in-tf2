
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