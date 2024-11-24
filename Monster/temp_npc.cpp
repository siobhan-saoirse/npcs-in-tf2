
#include "CEntity.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"


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

		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_STEP );
		SetBloodColor( BLOOD_COLOR_RED );

		SetImpactEnergyScale( 0.0f ); 

		CapabilitiesClear();
		
		CapabilitiesAdd( bits_CAP_USE_WEAPONS );
		CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND );
		CapabilitiesAdd( bits_CAP_AIM_GUN );
		
		NPCInit();
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY_VITAL;
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

