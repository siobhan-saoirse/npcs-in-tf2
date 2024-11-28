//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_BULLSQUID_H
#define NPC_BULLSQUID_H

#include "CAI_NPC.h"
#include "CCombatCharacter.h"
#include "CCycler_Fix.h"
#include "CEntity.h"
#include "grenade_spit.h"
#include "sign_func.h"

class CNPC_Bullsquid : public CE_Cycler_Fix
{
	CE_DECLARE_CLASS( CNPC_Bullsquid, CE_Cycler_Fix );

public:
	void Spawn( void );
	void Precache( void );
	Class_T	Classify( void );
	
	void IdleSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void DeathSound( const CTakeDamageInfo &info );
	void AttackSound( void );
	void GrowlSound( void );
	float   ThrowLimit( const Vector &vecStart, const Vector &vecEnd, float fGravity, float fArcSize, const Vector &mins, const Vector &maxs, CEntity *pTarget, Vector *jumpVel, CEntity **pBlocker);
	static Vector CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize);

	float MaxYawSpeed ( void );

	void HandleAnimEvent( animevent_t *pEvent );

	int RangeAttack1Conditions( float flDot, float flDist );
	int MeleeAttack1Conditions( float flDot, float flDist );
	int MeleeAttack2Conditions( float flDot, float flDist );

	bool FValidateHintType ( CAI_Hint *pHint );
	void RemoveIgnoredConditions( void );
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	int GetSoundInterests ( void );
	void RunAI ( void );
	virtual void OnListened ( void );

	int SelectSchedule( void );
	bool FInViewCone ( Vector pOrigin );

	void StartTask ( const Task_t *pTask );
	void RunTask ( const Task_t *pTask );

	NPC_STATE SelectIdealState ( void );

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

private:
	
	bool  m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 
	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
	int   m_nSquidSpitSprite;
	float m_flHungryTime;// set this is a future time to stop the monster from eating for a while. 

	float m_nextSquidSoundTime;
};
#endif // NPC_BULLSQUID_H