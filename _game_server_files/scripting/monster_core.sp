#include <sourcemod>
#include <sdktools>
#include <monster>

ConVar g_Cvar_Respawn;

public Plugin myinfo =
{
	name = "Monster Core",
	description = "Manages file precaches/downloads of monster stuff and adds a little bit of logic for coop maps",
	author = "ZombieX2 and donrevan",
	version = "1.2",
	url = "https://forums.alliedmods.net/showthread.php?p=2583637"
};

public void OnPluginStart()
{
	g_Cvar_Respawn = CreateConVar("monster_respawn_on_death", "2.0", "Specifies the delay that killed players should respawn after death (0 to disable respawn)", _, true, 0.0, true);
}

public void OnMapStart()
{
	PrintToServer("[MONSTER] Precaching models..");
	PrecacheModel("models/headcrabclassic.mdl",true);
	PrecacheModel("models/headcrab.mdl",true);
	PrecacheModel("models/headcrabblack.mdl",true);

	PrecacheModel("models/zombie/classic.mdl",true);
	PrecacheModel("models/zombie/classic_torso.mdl",true);
	PrecacheModel("models/zombie/classic_legs.mdl",true);
	PrecacheModel("models/zombie/fast.mdl",true);
	PrecacheModel("models/zombie/Fast_torso.mdl",true);
	PrecacheModel("models/gibs/fast_zombie_torso.mdl",true);
	PrecacheModel("models/gibs/fast_zombie_legs.mdl",true);
	PrecacheModel("models/zombie/poison.mdl",true);
	
	PrecacheModel("models/manhack.mdl",true);
	
	PrecacheModel("models/antlion.mdl",true);
	PrecacheModel("models/antlion_worker.mdl",true);
	PrecacheModel("models/antlion_guard.mdl",true);
	
	PrecacheModel("models/stalker.mdl",true);
	
	PrecacheModel("models/vortigaunt.mdl",true);

	PrecacheModel("models/roller.mdl",true);
	PrecacheModel("models/roller_spikes.mdl",true);

	PrecacheModel("sprites/grubflare1.vmt",true);
	PrecacheModel("sprites/glow1.vmt",true);
	PrecacheModel("sprites/laser.vmt",true);
	PrecacheModel("sprites/redglow1.vmt",true);
	PrecacheModel("sprites/orangeglow1.vmt",true);
	PrecacheModel("sprites/yellowglow1.vmt",true);
	PrecacheModel("sprites/lgtning.vmt",true);
	PrecacheModel("sprites/vortring1.vmt",true);
	
	PrecacheModel("sprites/bluelight1.vmt",true);
	PrecacheModel("sprites/rollermine_shock.vmt",true);
	PrecacheModel("sprites/rollermine_shock_yellow.vmt",true);
	
	PrecacheModel("models/props_combine/headcrabcannister01a.mdl",true);
	PrecacheModel("models/props_combine/headcrabcannister01b.mdl",true);
	PrecacheModel("models/props_combine/headcrabcannister01a_skybox.mdl",true);

	PrintToServer("[MONSTER] monster_core has been loaded successfully!");
}