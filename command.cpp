
#include "CAI_NPC.h"
#include "eventqueue.h"
#include "CCombatWeapon.h"
#include "sdk/smsdk_config.h"

ConVar *sv_gravity = NULL;
ConVar *phys_pushscale = NULL;
ConVar *npc_height_adjust = NULL;
ConVar *ai_path_adjust_speed_on_immediate_turns = NULL;
ConVar *ai_path_insert_pause_at_obstruction = NULL;
ConVar *ai_path_insert_pause_at_est_end = NULL;
ConVar *scene_flatturn = NULL;
ConVar *ai_use_clipped_paths = NULL;
ConVar *ai_moveprobe_usetracelist = NULL;
ConVar *ai_use_visibility_cache = NULL;
ConVar *ai_strong_optimizations = NULL;

ConVar *violence_hblood = NULL;
ConVar *violence_ablood = NULL;
ConVar *violence_hgibs = NULL;
ConVar *violence_agibs = NULL;

ConVar *sv_suppress_viewpunch = NULL;
ConVar *ai_navigator_generate_spikes = NULL;
ConVar *ai_navigator_generate_spikes_strength = NULL;

ConVar *ai_no_node_cache = NULL;
ConVar *sv_stepsize = NULL;
ConVar *hl2_episodic = NULL;
ConVar *ai_follow_use_points = NULL;
ConVar *ai_LOS_mode = NULL;
ConVar *ai_follow_use_points_when_moving = NULL;

ConVar *sk_autoaim_mode = NULL;

ConVar *sv_strict_notarget = NULL;

ConVar *ai_shot_bias_min = NULL;
ConVar *ai_shot_bias_max = NULL;
ConVar *ai_shot_bias = NULL;
ConVar *ai_spread_pattern_focus_time = NULL;

ConVar *ai_lead_time = NULL;

ConVar *scene_clamplookat = NULL;
ConVar *scene_showfaceto = NULL;
ConVar *flex_maxawaytime = NULL;
ConVar *flex_minawaytime = NULL;
ConVar *flex_maxplayertime = NULL;
ConVar *flex_minplayertime = NULL;

ConVar *ai_find_lateral_los = NULL;
ConVar *npc_sentences = NULL;

ConVar *rr_debugresponses = NULL;
ConVar *sk_ally_regen_time = NULL;
ConVar *sv_npc_talker_maxdist = NULL;
ConVar *ai_no_talk_delay = NULL;
ConVar *rr_debug_qa = NULL;
//ConVar *npc_ally_deathmessage = NULL;

ConVar *ai_no_local_paths = NULL;

ConVar *ai_use_think_optimizations = NULL;
ConVar *ai_use_efficiency = NULL;
ConVar *ai_efficiency_override = NULL;
ConVar *ai_frametime_limit = NULL;
ConVar *ai_default_efficient = NULL;
ConVar *ai_shot_stats = NULL;
ConVar *ai_shot_stats_term = NULL;

ConVar *ai_find_lateral_cover = NULL;
ConVar *think_limit = NULL;

ConVar *mat_dxlevel = NULL;
ConVar *npc_vphysics = NULL;

ConVar *ai_no_steer = NULL;
ConVar *ai_debug_directnavprobe = NULL;

ConVar *r_JeepViewZHeight = NULL;
ConVar *r_JeepViewDampenFreq = NULL;
ConVar *r_JeepViewDampenDamp = NULL;
ConVar *r_VehicleViewDampen = NULL;
ConVar *r_vehicleBrakeRate = NULL;

ConVar *xbox_throttlebias = NULL;
ConVar *xbox_throttlespoof = NULL;
ConVar *xbox_autothrottle = NULL;
ConVar *xbox_steering_deadzone = NULL;

ConVar *g_debug_vehicledriver = NULL;
ConVar *g_debug_npc_vehicle_roles = NULL;

ConVar npc_create_equipment("npc_create_equipment", "");

ConVar monster_version("monster_version", SMEXT_CONF_VERSION, FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_SPONLY|FCVAR_DONTRECORD);

void CC_NPC_Create( const CCommand &args )
{
	int index = g_Monster.GetCommandClient();
	CPlayer* pPlayer = UTIL_PlayerByIndex(index);
	if(pPlayer == NULL)
		return;

	gamehelpers->TextMsg(index, TEXTMSG_DEST_CHAT, "[MONSTER] Creating NPC...");

	// Try to create entity
	CAI_NPC *baseNPC = dynamic_cast< CAI_NPC * >( CreateEntityByName(args[1]) );
	if (baseNPC)
	{
		baseNPC->DispatchKeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();

		if ( args.ArgC() == 3 )
		{
			baseNPC->SetName(args[2]);
		}

		DispatchSpawn(baseNPC->BaseEntity());
		// Now attempt to drop into the world		
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer->BaseEntity(), COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			UTIL_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
			}
		}

		baseNPC->Activate();
	}
}



void cmd1_CommandCallback(const CCommand &command)
{
	int client = g_Monster.GetCommandClient();
	if(client)
	{
		gamehelpers->TextMsg(client, TEXTMSG_DEST_CHAT, "[MONSTER] You don't have access to this command!");
	} else {
		Vector vec(129.54f, 34.93f, 207.84f);//(118.25f, -490.82f, 90.03f);

		// test
		for ( int i = 1; i <= gpGlobals->maxClients; i++ ) {
			CPlayer *pPlayer = UTIL_PlayerByIndex(i);
			if (pPlayer) {
				vec = pPlayer->GetAbsOrigin();
				break;
			}
		}


		//vec.Init(6220.0f, 2813.0f, 1090.0f);

		//vec.Init(73.18,-54.81,-60.0);

		//vec.Init(952.65466,61.566082,-58.339985);


		//CEntity *cent = CreateEntityByName("npc_barnacle");
		//CEntity *cent = CreateEntityByName("npc_headcrab_fast");
		//CEntity *cent = CreateEntityByName("npc_headcrab_black");

		//CEntity *cent = CreateEntityByName("npc_fastzombie");
		//CEntity cent = CreateEntityByName("npc_fastzombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie");
		//CEntity *cent = CreateEntityByName("npc_poisonzombie");
		
		//CEntity *cent = CreateEntityByName("npc_manhack");
		//CEntity *cent = CreateEntityByName("npc_antlionguard");

		//CEntity *cent = CreateEntityByName("npc_stalker");

		//CEntity *cent = CreateEntityByName("npc_antlion");

		//CEntity *cent = CreateEntityByName("npc_vortigaunt");

		//CEntity *cent = CreateEntityByName("npc_rollermine");
		
		//CEntity *cent = CreateEntityByName("npc_test");
		
		/*CEntity *cent = CreateEntityByName("env_headcrabcanister");
	
		cent->CustomDispatchKeyValue("HeadcrabType", "0");
		cent->CustomDispatchKeyValue("HeadcrabCount", "10");
		
		cent->CustomDispatchKeyValue("SmokeLifetime","60");
		cent->CustomDispatchKeyValue("SkyboxCannisterCount","1");
		cent->CustomDispatchKeyValue("DamageRadius","0");
		cent->CustomDispatchKeyValue("Damage","100");*/

		//CEntity *cent = CreateEntityByName("npc_turret_floor");

		//CEntity *cent = CreateEntityByName("npc_combine");
			
		//CEntity *cent = CreateEntityByName("npc_combine_s");

		//cent->DispatchKeyValue("additionalequipment","weapon_ak47");
		//cent->DispatchKeyValue("tacticalvariant","1");

		/*CBaseEntity *cbase = cent->BaseEntity();

		CAI_NPC *hc = dynamic_cast<CAI_NPC *>(cent);
		
		//hc->AddSpawnFlags(( 1 << 18 ));

		cent->Teleport(&vec, NULL,NULL);

		//cent->AddSpawnFlags(4096);

		cent->Spawn();
		cent->Activate();*/


		//hc->GetSequenceKeyValues( 0 );
		

		//g_CEventQueue->AddEvent( cbase, "SelfDestruct", 0.5f, cbase,cbase );

		//hc->Dissolve(NULL, gpGlobals->curtime, false, 0 );

#if 0
		//CEntity *cent = CreateEntityByName("prop_vehicle_jeep");
		CEntity *cent = CreateEntityByName("prop_vehicle_prisoner_pod");
		cent->CustomDispatchKeyValue("vehiclescript", "scripts/vehicles/prisoner_pod.txt");
		//cent->CustomDispatchKeyValue("model", "models/buggy.mdl");
		cent->CustomDispatchKeyValue("model", "models/vehicles/prisoner_pod.mdl");
		cent->CustomDispatchKeyValue("solid","6");
		cent->CustomDispatchKeyValue("skin","0");
		cent->CustomDispatchKeyValue("actionScale","1");
		cent->CustomDispatchKeyValue("EnableGun","1");
		cent->CustomDispatchKeyValue("ignorenormals","0");
		cent->CustomDispatchKeyValue("fadescale","1");
		cent->CustomDispatchKeyValue("fademindist","-1");
		cent->CustomDispatchKeyValue("VehicleLocked","0");
		cent->CustomDispatchKeyValue("screenspacefade","0");
#endif

		CEntity *cent = CreateEntityByName("npc_strider");

		cent->Teleport(&vec, NULL,NULL);
		cent->Spawn();
		cent->Activate();


		META_CONPRINTF("%p %d %d\n",cent->BaseEntity(), cent->entindex_non_network(), cent->entindex());
	}
}

void monster_spawn_jeep(const CCommand &command)
{
	int client = g_Monster.GetCommandClient();
	if(client)
	{
		gamehelpers->TextMsg(client, TEXTMSG_DEST_CHAT, "[MONSTER] You don't have access to this command!");
	} else {
		if (command.ArgC() < 2) {
			META_CONPRINTF("monster_spawnjeep <userid>\n");
			return;
		}

		int userid = atoi(command.Arg(1));
		client = playerhelpers->GetClientOfUserId(userid);

		CPlayer *pPlayer = UTIL_PlayerByIndex(client);
		if (pPlayer && pPlayer->IsAlive()) {
			Vector vec = pPlayer->GetAbsOrigin();
			vec.z += 10.0f;

			CEntity *cent = CreateEntityByName("prop_vehicle_jeep");
			cent->CustomDispatchKeyValue("vehiclescript", "scripts/vehicles/jeep_test.txt");
			cent->CustomDispatchKeyValue("model", "models/buggy.mdl");
			cent->CustomDispatchKeyValue("solid","6");
			cent->CustomDispatchKeyValue("skin","0");
			cent->CustomDispatchKeyValue("actionScale","1");
			cent->CustomDispatchKeyValue("ignorenormals","0");
			cent->CustomDispatchKeyValue("fadescale","1");
			cent->CustomDispatchKeyValue("fademindist","-1");
			cent->CustomDispatchKeyValue("VehicleLocked","0");
			cent->CustomDispatchKeyValue("screenspacefade","0");

			cent->Teleport(&vec, NULL,NULL);
			cent->Spawn();
			cent->Activate();
		} else {
			META_CONPRINTF("monster_spawnjeep: invalid userid (not a living player)\n");
		}
	}
}

void monster_dump_CommandCallback(const CCommand &command)
{
	if (g_Monster.GetCommandClient() == 0)
	{
		GetEntityManager()->PrintDump();
	}
}

#define GET_CONVAR(name) \
	name = g_pCVar->FindVar(#name); \
	if(name == NULL) { \
		META_CONPRINTF("[%s] %s - CVar not found\n",g_Monster.GetLogTag(), #name); \
		return false; \
	}

bool CommandInitialize()
{
	new ConCommand("npc1",cmd1_CommandCallback, "", 0);
	new ConCommand("monster_dump",monster_dump_CommandCallback, "", 0);
	new ConCommand("npc_create", CC_NPC_Create, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT|FCVAR_GAMEDLL);

	new ConCommand("monster_spawnjeep", monster_spawn_jeep, "", 0);

	GET_CONVAR(sv_gravity);
	GET_CONVAR(phys_pushscale);
	GET_CONVAR(npc_height_adjust);

	GET_CONVAR(ai_path_adjust_speed_on_immediate_turns);
	GET_CONVAR(ai_path_insert_pause_at_obstruction);
	GET_CONVAR(ai_path_insert_pause_at_est_end);
	GET_CONVAR(ai_use_clipped_paths);
	GET_CONVAR(ai_moveprobe_usetracelist);
	GET_CONVAR(scene_flatturn);

	GET_CONVAR(violence_hblood);
	GET_CONVAR(violence_ablood);
	GET_CONVAR(violence_hgibs);
	GET_CONVAR(violence_agibs);

	GET_CONVAR(sv_suppress_viewpunch);
	GET_CONVAR(ai_use_visibility_cache);
	GET_CONVAR(ai_strong_optimizations);
	GET_CONVAR(ai_navigator_generate_spikes);
	GET_CONVAR(ai_navigator_generate_spikes_strength);
	GET_CONVAR(ai_no_node_cache);

	GET_CONVAR(sv_stepsize);
	GET_CONVAR(hl2_episodic);
	GET_CONVAR(ai_follow_use_points);
	GET_CONVAR(ai_LOS_mode);
	GET_CONVAR(ai_follow_use_points_when_moving);

	GET_CONVAR(sk_autoaim_mode);

	GET_CONVAR(sv_strict_notarget);

	GET_CONVAR(ai_shot_bias_min);
	GET_CONVAR(ai_shot_bias_max);
	GET_CONVAR(ai_shot_bias);
	GET_CONVAR(ai_spread_pattern_focus_time);

	GET_CONVAR(ai_lead_time);

	GET_CONVAR(scene_clamplookat);
	GET_CONVAR(scene_showfaceto);
	GET_CONVAR(flex_maxawaytime);
	GET_CONVAR(flex_minawaytime);
	GET_CONVAR(flex_maxplayertime);
	GET_CONVAR(flex_minplayertime);

	GET_CONVAR(ai_find_lateral_los);
	GET_CONVAR(npc_sentences);

	GET_CONVAR(ai_find_lateral_cover);
	GET_CONVAR(think_limit);

	GET_CONVAR(rr_debugresponses);
	GET_CONVAR(sk_ally_regen_time);
	GET_CONVAR(sv_npc_talker_maxdist);
	GET_CONVAR(ai_no_talk_delay);
	GET_CONVAR(rr_debug_qa);
	//GET_CONVAR(npc_ally_deathmessage);

	GET_CONVAR(ai_no_local_paths);

	GET_CONVAR(ai_use_think_optimizations);
	GET_CONVAR(ai_use_efficiency);
	GET_CONVAR(ai_efficiency_override);
	GET_CONVAR(ai_frametime_limit);
	GET_CONVAR(ai_default_efficient);
	GET_CONVAR(ai_shot_stats);
	GET_CONVAR(ai_shot_stats_term);

	GET_CONVAR(mat_dxlevel);
	GET_CONVAR(npc_vphysics);

	GET_CONVAR(ai_no_steer);
	GET_CONVAR(ai_debug_directnavprobe);

	GET_CONVAR(r_JeepViewZHeight);
	GET_CONVAR(r_JeepViewDampenFreq);
	GET_CONVAR(r_JeepViewDampenDamp);
	GET_CONVAR(r_VehicleViewDampen);
	GET_CONVAR(r_vehicleBrakeRate);

	GET_CONVAR(xbox_throttlebias);
	GET_CONVAR(xbox_throttlespoof);
	GET_CONVAR(xbox_autothrottle);
	GET_CONVAR(xbox_steering_deadzone);

	GET_CONVAR(g_debug_vehicledriver);
	GET_CONVAR(g_debug_npc_vehicle_roles);

	return true;
}