//#include "g_local.h"
#include "b_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include "../ghoul2/g2.h"

#define METROID_JUMP 1

//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void NPC_PlayConfusionSound( gentity_t *self );
extern void NPC_Jedi_PlayConfusionSound( gentity_t *self );
extern void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone );
//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void Jedi_Decloak( gentity_t *self );
//extern void G_AddMercBalance(gentity_t *defender, int amount);//add balance for having forcepower used on absorbing gunners
//[FatigueSys]
extern void G_AddMercBalance(gentity_t *self, int amount);//add balance to flamethrower
extern qboolean BG_SaberAttacking( playerState_t *ps );
extern qboolean BG_SaberInTransitionAny( int move );
extern qboolean /*GAME_INLINE*/ WalkCheck( gentity_t * self );
//[/FatigueSys]

extern vmCvar_t		g_saberRestrictForce;

extern qboolean BG_FullBodyTauntAnim( int anim );

extern bot_state_t *botstates[MAX_CLIENTS];
//extern void player_Freeze(gentity_t* self);
//extern void Player_CheckFreeze(gentity_t* self);
int speedLoopSound = 0;
 
int rageLoopSound = 0;
int extremeLoopSound = 0;

int protectLoopSound = 0;
int deathfieldLoopSound = 0;

int absorbLoopSound = 0;
int deathsightLoopSound = 0;

int seeLoopSound = 0;

int	ysalamiriLoopSound = 0;

#define FORCE_VELOCITY_DAMAGE 0
int ForceShootSevere( gentity_t *self );

int ForceShootDrain( gentity_t *self );

gentity_t *G_PreDefSound(vec3_t org, int pdSound)
{
	gentity_t	*te;

	te = G_TempEntity( org, EV_PREDEFSOUND );
	te->s.eventParm = pdSound;
	VectorCopy(org, te->s.origin);

	return te;
}

qboolean CheckPushItem( gentity_t *ent ) 
{
	if ( !ent->item )
	{
		return qfalse;
	}

	if ( ent->item->giType == IT_AMMO ||
		 ent->item->giType == IT_HEALTH ||
		 ent->item->giType == IT_ARMOR ||
		 ent->item->giType == IT_HOLDABLE )
	{
		return qtrue; // these don't have placeholders
	}

	return qfalse;
}

const int forcePowerMinRank[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] = //0 == neutral
{
	{
		999,//FP_HEAL,//instant
		999,//FP_LEVITATION,//hold/duration
		999,//FP_SPEED,//duration
		999,//FP_PUSH,//hold/duration
		999,//FP_PULL,//hold/duration
		999,//FP_TELEPATHY,//instant
		999,//FP_GRIP,//hold/duration
		999,//FP_LIGHTNING,//hold/duration
		999,//FP_RAGE,//duration
		999,//FP_PROTECT,//duration
		999,//FP_ABSORB,//duration
		999,//FP_TEAM_HEAL,//instant
		999,//FP_TEAM_FORCE,//instant
		999,//FP_DRAIN,//hold/duration
		999,//FP_SEE,//duration
		999,//FP_SABER_OFFENSE,
		999,//FP_SABER_DEFENSE,
		999//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		0,//FP_SABER_OFFENSE,
		0,//FP_SABER_DEFENSE,
		0//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		5,//FP_SABER_OFFENSE,
		5,//FP_SABER_DEFENSE,
		5//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		10,//FP_SABER_OFFENSE,
		10,//FP_SABER_DEFENSE,
		10//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

const int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	5000,
	10000,
	20000,
};

//[DodgeSys]
const int DodgeAbsorbBlockLevels[NUM_FORCE_POWER_LEVELS] = 
{
	9999,	//can't block force powers without absorb.
	DODGE_LIGHTLEVEL,
	DODGE_CRITICALLEVEL,
	0
};

#define SK_DP_FORFORCE		.5f	//determines the number of DP points players get for each skill point dedicated to Force Powers.
#define SK_DP_FORMERC		1/6.0f	//determines the number of DP points get for each skill point dedicated to gunner/merc skills.
void DetermineDodgeMax(gentity_t *ent)

											 
{//sets the maximum number of dodge points this player should have.  This is based on their skill point allociation.
	int i;
	int skillCount;
	float dodgeMax = 0;

	assert(ent && ent->client);

	if(ent->client->ps.isJediMaster)
	{//jedi masters have much more DP and don't actually have skills.
		ent->client->ps.stats[STAT_MAX_DODGE] = 250;
		return;
	}
	else if(ent->s.number < MAX_CLIENTS)
	{//players get a initial DP bonus.
		dodgeMax = 10;
	}

	//force powers
	for(i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if(ent->client->ps.fd.forcePowerLevel[i])
		{//has points in this skill
			for(skillCount = FORCE_LEVEL_1; skillCount <= ent->client->ps.fd.forcePowerLevel[i]; skillCount++)
			{
				dodgeMax += bgForcePowerCost[i][skillCount] * SK_DP_FORFORCE;
			}
		}
	}

	//additional skills
	for(i = 0; i < NUM_SKILLS; i++)
	{
		if(ent->client->skillLevel[i])
		{//has points in this skill
			for(skillCount = FORCE_LEVEL_1; skillCount <= ent->client->skillLevel[i]; skillCount++)
			{
				//[StanceSelection]
				if(i >= SK_BLUESTYLE && i <= SK_STAFFSTYLE)
				{//styles count as force powers
					dodgeMax += bgForcePowerCost[i+NUM_FORCE_POWERS][skillCount] * SK_DP_FORFORCE;
				}
				else
				{
					dodgeMax += bgForcePowerCost[i+NUM_FORCE_POWERS][skillCount] * SK_DP_FORMERC;
				}

				//dodgeMax += bgForcePowerCost[i][skillCount] * SK_DP_FORMERC;
				//[/StanceSelection]
			}
		}
	}

	ent->client->ps.stats[STAT_MAX_DODGE] = (int) dodgeMax;
   
}
//[/DodgeSys]


//[CoOp]
extern int SpawnForcePowerLevels[NUM_FORCE_POWERS];
extern qboolean UseSpawnForcePowers;
//[/CoOp]
void WP_InitForcePowers( gentity_t *ent )
{
	int i;
	int i_r;
	//[ExpSys]
	int maxRank = ent->client->sess.skillPoints;
	//int maxRank = g_maxForceRank.integer;
	//[/ExpSys]
	qboolean warnClient = qfalse;
	qboolean warnClientLimit = qfalse;
	char userinfo[MAX_INFO_STRING];
	char forcePowers[1024];
	char readBuf[1024];
	int lastFPKnown = -1;
	qboolean didEvent = qfalse;


	//[ExpSys]
	//skill points are experience based.
	/* basejka code
	if (!maxRank)
	{ //if server has no max rank, default to max (50)
		maxRank = FORCE_MASTERY_JEDI_MASTER;
	}
	else if (maxRank >= NUM_FORCE_MASTERY_LEVELS)
	{//ack, prevent user from being dumb
		maxRank = FORCE_MASTERY_JEDI_MASTER;
		trap_Cvar_Set( "g_maxForceRank", va("%i", maxRank) );
	}

	//[ForceSys]
	if(g_gametype.integer == GT_POWERDUEL && ent->client->sess.duelTeam == DUELTEAM_DOUBLE)
	{//double saberers have a lower force rank than the sole dueler while in power duel.
		maxRank--;
		if(maxRank < FORCE_MASTERY_UNINITIATED)
		{//don't have lower than the minimum force power
			maxRank = FORCE_MASTERY_UNINITIATED;
		}
	}
	//[/ForceSys]
	*/
	//[/ExpSys]

	/*
	if (g_forcePowerDisable.integer)
	{
		maxRank = FORCE_MASTERY_UNINITIATED;
	}
	*/
	//rww - don't do this

	if ( !ent || !ent->client )
	{
		return;
	}

	ent->client->ps.fd.saberAnimLevel = ent->client->sess.saberLevel;

	//[SaberSys]
	//racc - I'm not sure what this is for, but it's causing players to lose their stance if it's one of the hidden ones.
	/*
	if (ent->client->ps.fd.saberAnimLevel < FORCE_LEVEL_1 ||
		ent->client->ps.fd.saberAnimLevel > FORCE_LEVEL_3)
	{
		ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	*/
	//[/SaberSys]

	if (!speedLoopSound)
	{ //so that the client configstring is already modified with this when we need it
		speedLoopSound = G_SoundIndex("sound/weapons/force/speedloop.wav");
	}

	if (!rageLoopSound)
	{
		rageLoopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
	}
	
	if (!extremeLoopSound)
	{
		extremeLoopSound = G_SoundIndex("sound/weapons/force/extremeloop.wav");
	}
	
	if (!absorbLoopSound)
	{
		absorbLoopSound = G_SoundIndex("sound/weapons/force/absorbloop.wav");
	}
	
	if (!deathsightLoopSound)
	{
		deathsightLoopSound = G_SoundIndex("sound/weapons/force/deathsightloop.wav");
	}
	
	if (!protectLoopSound)
	{
		protectLoopSound = G_SoundIndex("sound/weapons/force/protectloop.wav");
	}

	if (!deathfieldLoopSound)
	{
		deathfieldLoopSound = G_SoundIndex("sound/weapons/force/deathfieldloop.wav");
	}
	
	if (!seeLoopSound)
	{
		seeLoopSound = G_SoundIndex("sound/weapons/force/seeloop.wav");
	}

	if (!ysalamiriLoopSound)
	{
		ysalamiriLoopSound = G_SoundIndex("sound/player/nullifyloop.wav");
	}

	if (ent->s.eType == ET_NPC)
	{ //just stop here then.
		return;
	}

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerLevel[i] = 0;
		ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		i++;
	}

	//[ExpSys]
	//racc - reset additional skills
	i = 0;
	while (i < NUM_SKILLS)
	{
		ent->client->skillLevel[i] = 0;
		i++;
	}
	//[/ExpSys]

	ent->client->ps.fd.forcePowerSelected = -1;

	ent->client->ps.fd.forceSide = 0;

	if (g_gametype.integer == GT_SIEGE &&
		ent->client->siegeClass != -1)
	{ //Then use the powers for this class, and skip all this nonsense.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
		
		i = 0;

		while (i < NUM_SKILLS)
		{
			ent->client->skillLevel[i] = bgSiegeClasses[ent->client->siegeClass].skillLevels[i];


			i++;
		}		
		

		if (!ent->client->sess.setForce)
		{
			//bring up the class selection menu
			trap_SendServerCommand(ent-g_entities, "scl");
		}
		ent->client->sess.setForce = qtrue;

		//[DodgeSys]
		DetermineDodgeMax(ent);
		//[/DodgeSys]
		return;
	}

	//[CoOp]
	//[ExpSys]
	/* the spawn forcepowers screw up the experience system.  Disabling them.
	//[/ExpSys]
	//use ICARUS overrides for our initial force powers.
	if(UseSpawnForcePowers)
	{
		int i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = SpawnForcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}

		ent->client->sess.setForce = qtrue;

		//[DodgeSys]
		DetermineDodgeMax(ent);
		//[/DodgeSys]
		return;
	}
	//[ExpSys]
	*/
	//[/ExpSys]
	//[/CoOp]

	//racc - actually all the NPC should have dumped out of here earlier than this.
	if (ent->s.eType == ET_NPC && ent->s.number >= MAX_CLIENTS)
	{ //rwwFIXMEFIXME: Temp
		strcpy(userinfo, "forcepowers\\7-1-333003000313003120");
	}
	else
	{
		trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
	}

	Q_strncpyz( forcePowers, Info_ValueForKey (userinfo, "forcepowers"), sizeof( forcePowers ) );

	if ( (ent->r.svFlags & SVF_BOT) && botstates[ent->s.number] )
	{ //if it's a bot just copy the info directly from its personality
		Com_sprintf(forcePowers, sizeof(forcePowers), "%s", botstates[ent->s.number]->forceinfo);
	}

	//rww - parse through the string manually and eat out all the appropriate data
	i = 0;

	if (g_forceBasedTeams.integer)
	{
		if (ent->client->sess.sessionTeam == TEAM_RED)
		{
			warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), FORCE_DARKSIDE, g_gametype.integer, g_forcePowerDisable.integer));
		}
		else if (ent->client->sess.sessionTeam == TEAM_BLUE)
		{
			warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), FORCE_LIGHTSIDE, g_gametype.integer, g_forcePowerDisable.integer));
		}
		else
		{
			warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), 0, g_gametype.integer, g_forcePowerDisable.integer));
		}
	}
	else
	{
		warnClient = !(BG_LegalizedForcePowers(forcePowers, maxRank, HasSetSaberOnly(), 0, g_gametype.integer, g_forcePowerDisable.integer));
	}

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-')
	{
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;
	//THE RANK
	ent->client->ps.fd.forceRank = atoi(readBuf);
	i++;

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-')
	{
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;		  
	//THE SIDE
	ent->client->ps.fd.forceSide = atoi(readBuf);
	i++;


	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '\n' &&
		i_r < NUM_FORCE_POWERS)
	{
		readBuf[0] = forcePowers[i];
		readBuf[1] = 0;

		ent->client->ps.fd.forcePowerLevel[i_r] = atoi(readBuf);
		if (ent->client->ps.fd.forcePowerLevel[i_r])
		{
			ent->client->ps.fd.forcePowersKnown |= (1 << i_r);
		}
		else
		{
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i_r);
		}
		i++;
		i_r++;
	}
	//THE POWERS

	//[ExpSys]
	//apply our additional force powers
	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '\n' &&
		i_r < NUM_SKILLS)
	{
		readBuf[0] = forcePowers[i];
		readBuf[1] = 0;

		ent->client->skillLevel[i_r] = atoi(readBuf);
		i++;
		i_r++;
	}
	//[/ExpSys]

	if (ent->s.eType != ET_NPC)
	{
		if (HasSetSaberOnly())
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FREE_SABER );
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FREE_SABER );
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}

		if (g_forcePowerDisable.integer)
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FORCE_DISABLE );
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else
		{
			gentity_t *te = G_TempEntity( vec3_origin, EV_SET_FORCE_DISABLE );
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}
	}

	//rww - It seems we currently want to always do this, even if the player isn't exceeding the max
	//rank, so..
//	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
//	{ //totally messes duel up to force someone into spec mode, and besides, each "round" is
	  //counted as a full restart
//		ent->client->sess.setForce = qtrue;
//	}

	//racc - Forces the player to look at their Profiles menu in some situations 
	//(like when the game starts or when the player's force power config string is bad)
	//This also lets the client know the server's maxForceRank is.
	//[BotTweaks]
	if (ent->s.eType == ET_NPC || ent->r.svFlags & SVF_BOT)
	//racc - bots care not about such things 
	//[/BotTweaks]
	{
		ent->client->sess.setForce = qtrue;
	}
	else if (g_gametype.integer == GT_SIEGE)
	{
		if (!ent->client->sess.setForce)
		{
			ent->client->sess.setForce = qtrue;
			//bring up the class selection menu
			trap_SendServerCommand(ent-g_entities, "scl");
		}
	}
	else
	{
		//[ExpSys]
		//since the possibility that a player's force configuration is more likely to be bad
		//with the new experience system, just go ahead and don't force warned clients into the
		//force powers screen.
		//if (!ent->client->sess.setForce)
		 if (warnClient || !ent->client->sess.setForce)
		//[/ExpSys]
		{ //the client's rank is too high for the server and has been autocapped, so tell them
			//temporary fix adding GT_DUEL to avoid error 
			if (g_gametype.integer != GT_HOLOCRON  && g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL)
			{
			//temporary fix
#ifdef EVENT_FORCE_RANK
				gentity_t *te = G_TempEntity( vec3_origin, EV_GIVE_NEW_RANK );

				te->r.svFlags |= SVF_BROADCAST;
				te->s.trickedentindex = ent->s.number;
				te->s.eventParm = maxRank;
				te->s.bolt1 = 0;
#endif
				didEvent = qtrue;

				//[BotTweaks]
				//racc - bots skip over this stuff now so this check isn't needed anymore.
				//if (!(ent->r.svFlags & SVF_BOT) && ent->s.eType != ET_NPC)
				//[/BotTweaks]
				{
					if (!g_teamAutoJoin.integer)
					{
						//Make them a spectator so they can set their powerups up without being bothered.
						ent->client->sess.sessionTeam = TEAM_SPECTATOR;
						ent->client->sess.spectatorState = SPECTATOR_FREE;
						ent->client->sess.spectatorClient = 0;

						ent->client->pers.teamState.state = TEAM_BEGIN;
						trap_SendServerCommand(ent-g_entities, "spc");	// Fire up the profile menu
					}
				}

#ifdef EVENT_FORCE_RANK
				te->s.bolt2 = ent->client->sess.sessionTeam;
#else
				//Event isn't very reliable, I made it a string. This way I can send it to just one
				//client also, as opposed to making a broadcast event.
				trap_SendServerCommand(ent->s.number, va("nfr %i %i %i", maxRank, 1, ent->client->sess.sessionTeam));
				//Arg1 is new max rank, arg2 is non-0 if force menu should be shown, arg3 is the current team
#endif
			}
			ent->client->sess.setForce = qtrue;
		}

		if (!didEvent )
		{
#ifdef EVENT_FORCE_RANK
			gentity_t *te = G_TempEntity( vec3_origin, EV_GIVE_NEW_RANK );

			te->r.svFlags |= SVF_BROADCAST;
			te->s.trickedentindex = ent->s.number;
			te->s.eventParm = maxRank;
			te->s.bolt1 = 1;
			te->s.bolt2 = ent->client->sess.sessionTeam;
#else
			trap_SendServerCommand(ent->s.number, va("nfr %i %i %i", maxRank, 0, ent->client->sess.sessionTeam));
#endif
		}

		if (warnClientLimit)
		{ //the server has one or more force powers disabled and the client is using them in his config
			trap_SendServerCommand(ent-g_entities, va("print \"The server has one or more force powers that you have chosen disabled.\nYou will not be able to use the disable force power(s) while playing on this server.\n\""));
		}
	}

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((ent->client->ps.fd.forcePowersKnown & (1 << i)) &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{ //err..
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}
		else
		{
			if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW)
			{
				lastFPKnown = i;
			}
		}

		i++;
	}

	if (ent->client->ps.fd.forcePowersKnown & ent->client->sess.selectedFP)
	{
		ent->client->ps.fd.forcePowerSelected = ent->client->sess.selectedFP;
	}

	if (!(ent->client->ps.fd.forcePowersKnown & (1 << ent->client->ps.fd.forcePowerSelected)))
	{
		if (lastFPKnown != -1)
		{
			ent->client->ps.fd.forcePowerSelected = lastFPKnown;
		}
		else
		{
			ent->client->ps.fd.forcePowerSelected = 0;
		}
	}

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerBaseLevel[i] = ent->client->ps.fd.forcePowerLevel[i];
		i++;
	}
	ent->client->ps.fd.forceUsingAdded = 0;


	//[DodgeSys]
	//determine the player's DP max.
	DetermineDodgeMax(ent);
	//[/DodgeSys]
}


void WP_SpawnInitForcePowers( gentity_t *ent )
{
	int i = 0;

	//[SaberSys]
	//new balancing system uses 7 as the default.
	//ent->client->ps.saberAttackChainCount = 7;
	ent->client->ps.saberAttackChainCount = 0;
	//[/SaberSys]

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}

		i++;
	}

	ent->client->ps.fd.forceDeactivateAll = 0;


	ent->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	ent->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
	


	ent->client->ps.fd.forceMindtrickTargetIndex = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex2 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex3 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex4 = 0;
		


	ent->client->ps.holocronBits = 0;

//	if(ent->client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_3)
	//{
	//	ent->client->ps.fd.forcePower+=15;
	//	ent->client->ps.fd.forcePowerMax+=15;
	//}

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.holocronsCarried[i] = 0;
		i++;
	}

	if (g_gametype.integer == GT_HOLOCRON)
	{
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			i++;
		}

		if (HasSetSaberOnly())
		{
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			}
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			}
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerDebounce[i] = 0;
		ent->client->ps.fd.forcePowerDuration[i] = 0;

		i++;
	}


	ent->client->ps.fd.forceJumpZStart = 0;
	ent->client->ps.fd.forceJumpCharge = 0;
	ent->client->ps.fd.forceJumpSound = 0;
	ent->client->ps.fd.forceGripDamageDebounceTime = 0;
	ent->client->ps.fd.forceGripBeingGripped = 0;
	ent->client->ps.fd.forceGripCripple = 0;
	ent->client->ps.fd.forceGripUseTime = 0;
	ent->client->ps.fd.forceGripSoundTime = 0;
	ent->client->ps.fd.forceGripStarted = 0;
	ent->client->ps.fd.forceHealTime = 0;
	ent->client->ps.fd.forceHealAmount = 0;
	ent->client->ps.fd.forceRageRecoveryTime = 0;
	ent->client->ps.fd.forceDrainEntNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceDrainTime = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((ent->client->ps.fd.forcePowersKnown & (1 << i)) &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{ //make sure all known powers are cleared if we have level 0 in them
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}

		i++;
	}

	if (g_gametype.integer == GT_SIEGE &&
		ent->client->siegeClass != -1)
	{ //Then use the powers for this class.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
		
		
		
		i = 0;

		while (i < NUM_SKILLS)
		{
			ent->client->skillLevel[i] = bgSiegeClasses[ent->client->siegeClass].skillLevels[i];

			i++;
		}		
		
		
	}

	//[CoOp]
	//use ICARUS overrides for our initial force powers.
	//[ExpSys]
	/* racc - disabled the force power spawn scripting since it screws up the experience system.
	//[/ExpSys]
	if(UseSpawnForcePowers && !ent->NPC)
	{
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = SpawnForcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}

	}
	//[ExpSys]
	*/
	//[/ExpSys]
	//[/CoOp]
}

extern qboolean BG_InKnockDown( int anim ); //bg_pmove.c

qboolean IsMerc(gentity_t*ent)
{
	if(!ent->client)
		return qfalse;

	if(ent->client->skillLevel[SK_JETPACK])
		return qtrue;
	else if(ent->client->skillLevel[SK_PISTOL])
		return qtrue;
	else if(ent->client->skillLevel[SK_BLASTER])
		return qtrue;
	else if(ent->client->skillLevel[SK_THERMAL])
		return qtrue;
	else if(ent->client->skillLevel[SK_ROCKET])
		return qtrue;
	else if(ent->client->skillLevel[SK_BACTA])
		return qtrue;
	else if(ent->client->skillLevel[SK_FLAMETHROWER])
		return qtrue;
	else if(ent->client->skillLevel[SK_BOWCASTER])
		return qtrue;
	else if(ent->client->skillLevel[SK_FORCEFIELD])
		return qtrue;
	else if(ent->client->skillLevel[SK_CLOAK])
		return qtrue;
	else if(ent->client->skillLevel[SK_SEEKER])
		return qtrue;
	else if(ent->client->skillLevel[SK_SENTRY])
		return qtrue;
	else if(ent->client->skillLevel[SK_DETPACK])
		return qtrue;
	else if(ent->client->skillLevel[SK_REPEATER])
		return qtrue;
	else if(ent->client->skillLevel[SK_DISRUPTOR])
		return qtrue;
	else if(ent->client->skillLevel[SK_TRIPMINE])
		return qtrue;
	else if(ent->client->skillLevel[SK_DEMP2])
		return qtrue;
	else if(ent->client->skillLevel[SK_FLECHETTE])
		return qtrue;
	else if(ent->client->skillLevel[SK_CONCUSSION])
		return qtrue;
	else if(ent->client->skillLevel[SK_OLD])
		return qtrue;
	else if(ent->client->skillLevel[SK_EWEB])
		return qtrue;
	else if(ent->client->skillLevel[SK_BINOCULARS])
		return qtrue;
	else if(ent->client->skillLevel[SK_WRIST])
		return qtrue;
	else if(ent->client->skillLevel[SK_ELECTROSHOCKER])
		return qtrue;	   
	else if(ent->client->skillLevel[SK_SPHERESHIELD])
		return qtrue;		
	else if(ent->client->skillLevel[SK_OVERLOAD])
		return qtrue;
	else if(ent->client->skillLevel[SK_SQUADTEAM])
		return qtrue;	
	else if(ent->client->skillLevel[SK_VEHICLEMOUNT])
		return qtrue;	
	else if(ent->client->skillLevel[SK_GRAPPLE])
		return qtrue;		
	return qfalse;
}

//[ForceSys]
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean BG_InSlowBounce(playerState_t *ps);
extern qboolean BG_InKnockDownOnGround( playerState_t *ps );
//[/ForceSys]
int ForcePowerUsableOn(gentity_t *attacker, gentity_t *other, forcePowers_t forcePower)
{
	if(other->client && other->client->ps.inAirAnim || other->client && other->client->ps.groundEntityNum == ENTITYNUM_NONE)
		return 1;

	if (other && other->client && BG_HasYsalamiri(g_gametype.integer, &other->client->ps))
	{
		return 0;
	}

	if (attacker && attacker->client && !BG_CanUseFPNow(g_gametype.integer, &attacker->client->ps, level.time, forcePower))
	{
		return 0;
	}
	
	if(IsMerc(other) && other->client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_1)
		return 1;

	//Dueling fighters cannot use force powers on others, with the exception of force push when locked with each other
	if (attacker && attacker->client && attacker->client->ps.duelInProgress)
	{
		return 0;
	}

	if (other && other->client && other->client->ps.duelInProgress)
	{
		return 0;
	}

	//[InAirChange]
	//if(other && other->client && other->client->ps.stats[STAT_DODGE] <= DODGE_CRITICALLEVEL)
	//	return 1;

	if(other->client && other->client->ps.fd.saberAnimLevel == SS_DESANN
		&& other->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY)
		return 0;

	if(forcePower == FP_TELEPATHY && other->client)
	{
		switch(other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_3:
			return 0;
			break;
		case FORCE_LEVEL_2:
			if(!WalkCheck(other) && PM_RunningAnim(other->client->ps.legsAnim) )
				return 0;
			break;

		case FORCE_LEVEL_1:
			if(!WalkCheck(other))
				return 0;
			break;
		}
	}
	else if(forcePower == FP_TEAM_HEAL && other->client)
	{
		switch(other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_3:
			return 0;
			break;
		case FORCE_LEVEL_2:
			if(!WalkCheck(other) && PM_RunningAnim(other->client->ps.legsAnim) )
				return 0;
			break;

		case FORCE_LEVEL_1:
			if(!WalkCheck(other))
				return 0;
			break;
		}
	}
	else if(forcePower == FP_GRIP && other->client)
	{
		switch(other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_1://Can only block if walking
			if(!WalkCheck(other))
				return 1;
			break;
		case FORCE_LEVEL_2://Can block if walking or running
			if(other->client->ps.inAirAnim || other->client->ps.groundEntityNum == ENTITYNUM_NONE)
				return 1;
			break;

		case FORCE_LEVEL_3:
			return 0;

		default:
			return 1;
		}
	}

	//[ForceSys]
	//handling the grip countering actions in OJP_CounterForce().
	/*
	if (forcePower == FP_GRIP)
	{
		if (other && other->client &&
			(other->client->ps.fd.forcePowersActive & (1<<FP_ABSORB)))
		{ //don't allow gripping to begin with if they are absorbing
			//play sound indicating that attack was absorbed
			if (other->client->forcePowerSoundDebounce < level.time)
			{
				gentity_t *abSound = G_PreDefSound(other->client->ps.origin, PDSOUND_ABSORBHIT);
				abSound->s.trickedentindex = other->s.number;
				other->client->forcePowerSoundDebounce = level.time + 400;
			}
			return 0;
		}
		else if (other && other->client &&
			other->client->ps.weapon == WP_SABER &&
			BG_SaberInSpecial(other->client->ps.saberMove))
		{ //don't grip person while they are in a special or some really bad things can happen.
			return 0;
		}
	}
	*/
	//[/ForceSys]

	if (other && other->client &&
		(forcePower == FP_PUSH ||
		forcePower == FP_PULL))
	{
		if (BG_InKnockDown(other->client->ps.legsAnim))
		{
			return 0;
		}
	}

	if (other && other->client && other->s.eType == ET_NPC &&
		other->s.NPC_class == CLASS_VEHICLE)
	{ //can't use the force on vehicles.. except lightning
		if (forcePower == FP_LIGHTNING || forcePower == FP_DRAIN || forcePower == FP_TEAM_FORCE || forcePower == FP_GRIP)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

//	if (other && other->client && other->s.eType == ET_NPC &&
//		g_gametype.integer == GT_SIEGE)
//	{ //can't use powers at all on npc's normally in siege... //racc - probably because they're objective based objects?
//		return 0;
//	}

	return 1;
}

qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	int	drain = overrideAmt ? overrideAmt :
				forcePowerNeeded[self->client->ps.fd.forcePowerLevel[forcePower]][forcePower];

	if (self->client->ps.fd.forcePowersActive & (1 << forcePower))
	{ //we're probably going to deactivate it..
		return qtrue;
	}
	if ( forcePower == FP_LEVITATION )
	{
		return qtrue;
	}
	if ( !drain )
	{
		return qtrue;
	}



	if ((forcePower == FP_DRAIN || forcePower == FP_LIGHTNING || forcePower == FP_TEAM_FORCE) &&
		self->client->ps.fd.forcePower >= 25)
	{ //it's ok then, drain/lightning are actually duration
		return qtrue;
	}


	if ( self->client->ps.fd.forcePower < drain )
	{
		return qfalse;
	}
	return qtrue;
}

qboolean WP_ForcePowerInUse( gentity_t *self, forcePowers_t forcePower )
{
	if ( (self->client->ps.fd.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		return qtrue;
	}

	return qfalse;
}

qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower )
{
	if (BG_HasYsalamiri(g_gametype.integer, &self->client->ps))
	{
		return qfalse;
	}

	if (self->health <= 0 || self->client->ps.stats[STAT_HEALTH] <= 0 ||
		(self->client->ps.eFlags & EF_DEAD))
	{
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //specs can't use powers through people
		return qfalse;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}
	if (self->client->tempSpectate >= level.time)
	{
		return qfalse;
	}

	if (!BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, forcePower))
	{
		return qfalse;
	}

	if ( !(self->client->ps.fd.forcePowersKnown & ( 1 << forcePower )) )
	{//don't know this power
		return qfalse;
	}
	
	if ( (self->client->ps.fd.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		if (forcePower != FP_LEVITATION && forcePower != FP_SPEED)
		{
			return qfalse;
		}
	}

	if (forcePower == FP_LEVITATION && self->client->fjDidJump)
	{
		return qfalse;
	}

	if (!self->client->ps.fd.forcePowerLevel[forcePower])
	{
		return qfalse;
	}

	if ( g_debugMelee.integer )
	{
		if ( (self->client->ps.pm_flags&PMF_STUCK_TO_WALL) )
		{//no offensive force powers when stuck to wall
			switch ( forcePower )
			{
			case FP_GRIP:
			case FP_LIGHTNING:
			case FP_DRAIN:
			case FP_TEAM_FORCE:				
			case FP_SABER_OFFENSE:
			case FP_SABER_DEFENSE:
			case FP_SABERTHROW:
				return qfalse;
				break;
			}
		}
	}

	if ( !self->client->ps.saberHolstered )
	{
		if ( (self->client->saber[0].saberFlags&SFL_TWO_HANDED) )
		{
			if ( g_saberRestrictForce.integer )
			{
				switch ( forcePower )
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
				case FP_TEAM_HEAL:
				case FP_TEAM_FORCE:				
					return qfalse;
					break;
				}
			}
		}

		if ( (self->client->saber[0].saberFlags&SFL_TWO_HANDED)
			|| ( self->client->saber[0].model[0]) )
		{//this saber requires the use of two hands OR our other hand is using an active saber too
			if ( (self->client->saber[0].forceRestrictions&(1<<forcePower)) )
			{//this power is verboten when using this saber
				return qfalse;
			}
		}

		if ( self->client->saber[0].model[0] )
		{//both sabers on
			if ( g_saberRestrictForce.integer )
			{
				switch ( forcePower )
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
				case FP_TEAM_HEAL:
				case FP_TEAM_FORCE:	
					return qfalse;
					break;
				}
			}
			if ( (self->client->saber[1].forceRestrictions&(1<<forcePower)) )
			{//this power is verboten when using this saber
				return qfalse;
			}
		}
	}
	return WP_ForcePowerAvailable( self, forcePower, 0 );	// OVERRIDEFIXME
}


//[ForceSys]

int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent)
{//racc - performs force absorb check, returns power difference between the attacker's power level and the defender's power level.
	//returns -1 if absorb didn't happen.  This function also handles the actual force absorb energy gain for the defender.
	int getLevel = 0;
	int addTot = 0;
	gentity_t *abSound;

	if (atPower != FP_LIGHTNING &&
		atPower != FP_DRAIN &&
		atPower != FP_TEAM_FORCE &&
		atPower != FP_GRIP &&
		atPower != FP_TELEPATHY &&
		atPower != FP_TEAM_HEAL &&
		atPower != FP_PROTECT &&
		atPower != FP_ABSORB &&
		atPower != FP_PUSH &&
		atPower != FP_PULL)
	{ //Only these powers can be absorbed
		return -1;
	}

	if (!atdAbsLevel)
	{ //looks like attacker doesn't have any absorb power
		return -1;
	}

	if (!(attacked->client && attacked->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !attacked->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	getLevel = atPowerLevel;
	getLevel -= atdAbsLevel;

	if (getLevel < 0)
	{
		getLevel = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
//	addTot = (atForceSpent/3)*attacked->client->ps.fd.forcePowerLevel[FP_ABSORB];
	addTot = atForceSpent;
	if (addTot < 1)
	{
		addTot = 1;
	}
	attacked->client->ps.fd.forcePower += addTot;
	if (attacked->client->ps.fd.forcePower > 250)
	{
		attacked->client->ps.fd.forcePower = 250;
	}

	//play sound indicating that attack was absorbed
	if (attacked->client->forcePowerSoundDebounce < level.time)
	{

		abSound = G_PreDefSound(attacked->client->ps.origin, PDSOUND_ABSORBHIT);
		
		abSound->s.trickedentindex = attacked->s.number;

		attacked->client->forcePowerSoundDebounce = level.time + 400;
	}

	return getLevel;
}

//[/ForceSys]


void WP_ForcePowerRegenerate( gentity_t *self, int overrideAmt )
{ //called on a regular interval to regenerate force power.
	if ( !self->client )
	{
		return;
	}

	if ( overrideAmt )
	{ //custom regen amount
		self->client->ps.fd.forcePower += overrideAmt;
	}
	else
	{ //otherwise, just 1
		self->client->ps.fd.forcePower++;
	}

	if ( self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax )
	{ //cap it off at the max (default 100)
		self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
	}
}

void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{ //activate the given force power
	int	duration = 0;
	qboolean hearable = qfalse;
	float hearDist = 0;

	if (!WP_ForcePowerAvailable( self, forcePower, overrideAmt ))
	{
		return;
	}

	if ( BG_FullBodyTauntAnim( self->client->ps.legsAnim ) )
	{//stop taunt
		self->client->ps.legsTimer = 0;
	}
	if ( BG_FullBodyTauntAnim( self->client->ps.torsoAnim ) )
	{//stop taunt
		self->client->ps.torsoTimer = 0;
	}
	//hearable and hearDist are merely for the benefit of bots, and not related to if a sound is actually played.
	//If duration is set, the force power will assume to be timer-based.
	switch( (int)forcePower )
	{
	case FP_HEAL:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_LEVITATION:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_SPEED:
		hearable = qtrue;
		hearDist = 256;
		//[ForceSys]
	//[ForceSys]
				 
		self->client->forceSpeedStartTime = level.time;

		if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}

		if (overrideAmt)
		{
			duration = overrideAmt;
		}

		//[/ForceSys]

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_PUSH:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PUSH2);
		}
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_PUSH];
		break;
	case FP_PULL:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PULL2);
		}
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_PULL];
		break;
	case FP_TELEPATHY:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_GRIP:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 60000;
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_GRIP];
		break;
	case FP_LIGHTNING:
		hearable = qtrue;
		hearDist = 512;
		if(self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
		{
			duration = 1000;
		}
		else
		{
			duration = overrideAmt;
		}
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
		break;
	case FP_RAGE:
		hearable = qtrue;
		hearDist = 256;
		
		if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}
	
	
	
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_PROTECT:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_ABSORB:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_TEAM_HEAL:
		hearable = qtrue;
		hearDist = 256;	
		if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_TEAM_FORCE:
		hearable = qtrue;
		hearDist = 256;
		duration = 1;		  
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE];

		//duration = overrideAmt;
		//overrideAmt = 0;
		break;
	case FP_DRAIN:
		hearable = qtrue;
		hearDist = 512;
		if(self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1)
		{
			duration = 1000;
		}
		else	
		{
			duration = overrideAmt;
		}
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN];
		break;
	case FP_SEE:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	if ( duration )
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = 0;
	}

	if (hearable)
	{
		self->client->ps.otherSoundLen = hearDist;
		self->client->ps.otherSoundTime = level.time + 100;
	}
	
	self->client->ps.fd.forcePowerDebounce[forcePower] = 0;

	if ((int)forcePower == FP_SPEED && overrideAmt)
	{
		BG_ForcePowerDrain( &self->client->ps, forcePower, overrideAmt*0.025 );
	}
	else
	{ 
		BG_ForcePowerDrain( &self->client->ps, forcePower, overrideAmt );
	}
}
void ForceRegeneration( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_HEAL ) )
	{
		return;
	}

	if ( self->client->ps.fd.forcePower >= self->client->ps.fd.forcePowerMax && self->client->ps.stats[STAT_DODGE] >= self->client->ps.stats[STAT_MAX_DODGE])
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_3)
	{
				if (self->client->ps.fd.forcePower < self->client->ps.fd.forcePowerMax )
				{
					self->client->ps.fd.forcePower += 100;
					if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
					{
						self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				if (self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE] )
				{
					self->client->ps.stats[STAT_DODGE] += 100;
					if (self->client->ps.stats[STAT_DODGE] > self->client->ps.stats[STAT_MAX_DODGE])
					{
						self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2)
	{
				if (self->client->ps.fd.forcePower < self->client->ps.fd.forcePowerMax )
				{
					self->client->ps.fd.forcePower += 50;
					if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
					{
						self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				if (self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE] )
				{
					self->client->ps.stats[STAT_DODGE] += 50;
					if (self->client->ps.stats[STAT_DODGE] > self->client->ps.stats[STAT_MAX_DODGE])
					{
						self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
	}
	else
	{
				if (self->client->ps.fd.forcePower < self->client->ps.fd.forcePowerMax )
				{
					self->client->ps.fd.forcePower += 25;
					if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
					{
						self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				if (self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE] )
				{
					self->client->ps.stats[STAT_DODGE] += 25;
					if (self->client->ps.stats[STAT_DODGE] > self->client->ps.stats[STAT_MAX_DODGE])
					{
						self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
					}
					//BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}

	}
	/*
	else
	{
		WP_ForcePowerStart( self, FP_HEAL, 0 );
	}
	*/
	//NOTE: Decided to make all levels instant.

	G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/regeneration.wav") );

//[Bolted effect]
	G_PlayBoltedEffect( G_EffectIndex( "force/heal3.efx" ), self, "thoracic" );
	gentity_t* tent;
	tent = G_TempEntity(self->r.currentOrigin, EV_FORCE_REGENERATED);
	tent->s.owner = self->s.number;
//[/Bolted effect]
}
void ForceHeal( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_HEAL ) )
	{
		return;
	}

	if ( self->health >= self->client->ps.stats[STAT_MAX_HEALTH] && self->client->ps.stats[STAT_ARMOR] >= self->client->ps.stats[STAT_MAX_ARMOR])
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_3)
	{
				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->health += 100;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				else if (self->client->ps.stats[STAT_HEALTH] == self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->client->ps.stats[STAT_ARMOR] += 100;
					if (self->client->ps.stats[STAT_ARMOR] > self->client->ps.stats[STAT_MAX_ARMOR])
					{
						self->client->ps.stats[STAT_ARMOR] = self->client->ps.stats[STAT_MAX_ARMOR];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2)
	{
				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->health += 50;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				else if (self->client->ps.stats[STAT_HEALTH] == self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->client->ps.stats[STAT_ARMOR] += 50;
					if (self->client->ps.stats[STAT_ARMOR] > self->client->ps.stats[STAT_MAX_ARMOR])
					{
						self->client->ps.stats[STAT_ARMOR] = self->client->ps.stats[STAT_MAX_ARMOR];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
	}
	else
	{
				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->health += 25;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}
				else if (self->client->ps.stats[STAT_HEALTH] == self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					self->client->ps.stats[STAT_ARMOR] += 25;
					if (self->client->ps.stats[STAT_ARMOR] > self->client->ps.stats[STAT_MAX_ARMOR])
					{
						self->client->ps.stats[STAT_ARMOR] = self->client->ps.stats[STAT_MAX_ARMOR];
					}
					BG_ForcePowerDrain( &self->client->ps, FP_HEAL, 0 );
				}

	}
	/*
	else
	{
		WP_ForcePowerStart( self, FP_HEAL, 0 );
	}
	*/
	//NOTE: Decided to make all levels instant.

	G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav") );

//[Bolted effect]
	G_PlayBoltedEffect( G_EffectIndex( "force/heal2.efx" ), self, "thoracic" );
	gentity_t	*tent;
	tent = G_TempEntity(self->r.currentOrigin, EV_FORCE_HEALED);
	tent->s.owner = self->s.number;
//[/Bolted effect]
}

void WP_AddToClientBitflags(gentity_t *ent, int entNum)
{
	if (!ent)
	{
		return;
	}

	if (entNum > 47)
	{
		ent->s.trickedentindex4 |= (1 << (entNum-48));
	}
	else if (entNum > 31)
	{
		ent->s.trickedentindex3 |= (1 << (entNum-32));
	}
	else if (entNum > 15)
	{
		ent->s.trickedentindex2 |= (1 << (entNum-16));
	}
	else
	{
		ent->s.trickedentindex |= (1 << entNum);
	}
}






/*
void ForceTeamForceReplenish( gentity_t *self )
{
	float radius = 256;
	int i = 0;
	gentity_t *ent;
	vec3_t a;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int poweradd = 0;
	gentity_t *te = NULL;

	if ( self->health <= 0 )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TEAM_FORCE ) )
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] >= level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
		radius *= 1.5;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
		radius *= 2;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && self != ent && OnSameTeam(self, ent) && ent->client->ps.fd.forcePower < 100 && ForcePowerUsableOn(self, ent, FP_TEAM_FORCE) &&
			trap_InPVS(self->client->ps.origin, ent->client->ps.origin))
		{
			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

			if (VectorLength(a) <= radius)
			{
				pl[numpl] = i;
				numpl++;
			}
		}

		i++;
	}

	if (numpl < 1)
	{
		return;
	}

	if (numpl == 1)
	{
		poweradd = 50;
	}
	else if (numpl == 2)
	{
		poweradd = 33;
	}
	else
	{
		poweradd = 25;
	}
	self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 2000;

	BG_ForcePowerDrain( &self->client->ps, FP_TEAM_FORCE, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE] );

	i = 0;

	while (i < numpl)
	{
		g_entities[pl[i]].client->ps.fd.forcePower += poweradd;
		if (g_entities[pl[i]].client->ps.fd.forcePower > 250)
		{
			g_entities[pl[i]].client->ps.fd.forcePower = 250;
		}

		//At this point we know we got one, so add him into the collective event client bitflag
		if (!te)
		{
			te = G_TempEntity( self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 2; //eventParm 1 is heal, eventParm 2 is force regen
		}

		WP_AddToClientBitflags(te, pl[i]);
		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.
		
		i++;
	}
}

*/
//[ForceSys]
extern qboolean PM_SaberInParry( int move );
qboolean OJP_CounterForce(gentity_t *attacker, gentity_t *defender, int attackPower);
//[/ForceSys]
extern void BG_ReduceMishapLevel(playerState_t *ps);
void ForceGrasp( gentity_t *self )
{
	trace_t tr;
	vec3_t tfrom, tto, fwd;

	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING) || self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN) || self->client->ps.fd.forcePowersActive & (1 << FP_TEAM_FORCE) ))
	{
		return;
	}
	

	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}

	if (self->client->ps.fd.forceGripUseTime > level.time)
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_GRIP ) )
	{
		return;
	}

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*MAX_GRIP_DISTANCE;
	tto[1] = tfrom[1] + fwd[1]*MAX_GRIP_DISTANCE;
	tto[2] = tfrom[2] + fwd[2]*MAX_GRIP_DISTANCE;
	
	trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 &&
		tr.entityNum != ENTITYNUM_NONE &&
		g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&  //racc - not currently under the effects of gripcripple.
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time && //racc - not being gripped
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		//[ForceSys]
		!OJP_CounterForce(self, &g_entities[tr.entityNum], FP_GRIP) &&
		//[/ForceSys]
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum])) ) //don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS && g_entities[tr.entityNum].client->ps.m_iVehicleNum)
		{ //a player on a vehicle
			gentity_t *vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
			{
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{ //push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)&g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;
		BG_ReduceMishapLevel(&g_entities[tr.entityNum].client->ps);
		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 20000;
	}/*
	else if(self && Q_stricmp(g_entities[tr.entityNum].classname,"body") == 0)
	{
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		self->client->ps.fd.forceGripDamageDebounceTime = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;
	}*/
	else
	{
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
		return;
	}
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/telekinesis") );
}

void ForceGrip( gentity_t *self )
{
	trace_t tr;
	vec3_t tfrom, tto, fwd;

	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING) || self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN) || self->client->ps.fd.forcePowersActive & (1 << FP_TEAM_FORCE) ))
	{
		return;
	}
	

	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}

	if (self->client->ps.fd.forceGripUseTime > level.time)
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_GRIP ) )
	{
		return;
	}

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*MAX_GRIP_DISTANCE;
	tto[1] = tfrom[1] + fwd[1]*MAX_GRIP_DISTANCE;
	tto[2] = tfrom[2] + fwd[2]*MAX_GRIP_DISTANCE;
	
	trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 &&
		tr.entityNum != ENTITYNUM_NONE &&
		g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&  //racc - not currently under the effects of gripcripple.
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time && //racc - not being gripped
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		//[ForceSys]
		!OJP_CounterForce(self, &g_entities[tr.entityNum], FP_GRIP) &&
		//[/ForceSys]
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum])) ) //don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS && g_entities[tr.entityNum].client->ps.m_iVehicleNum)
		{ //a player on a vehicle
			gentity_t *vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
			{
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{ //push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)&g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;
		BG_ReduceMishapLevel(&g_entities[tr.entityNum].client->ps);
		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 20000;
	}/*
	else if(self && Q_stricmp(g_entities[tr.entityNum].classname,"body") == 0)
	{
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		self->client->ps.fd.forceGripDamageDebounceTime = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;
	}*/
	else
	{
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
		return;
	}
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/grip") );
}

void ForceSpeed( gentity_t *self, int forceDuration )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//[ForceSys]
	/*
	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)) )
	{
		WP_ForcePowerStop( self, FP_SPEED );
		return;
	}
	*/
	//[/ForceSys]

	if ( !WP_ForcePowerUsable( self, FP_SPEED ) )
	{
		return;
	}

	if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
		&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD )
	{//holding Siege item
		if ( g_entities[self->client->holdingObjectiveItem].genericValue15 )
		{//disables force powers
			return;
		}
	}

	//[ForceSys]
	if(self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{//it's already turned on.  just keep it going.
		self->client->ps.fd.forcePowerDuration[FP_SPEED] = level.time + 500;
		return;
	}
	//[/ForceSys]

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_SPEED, forceDuration );
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
	G_Sound( self, TRACK_CHANNEL_2, speedLoopSound );
}

void ForceSeeing( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)) )
	{
		WP_ForcePowerStop( self, FP_SEE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_SEE ) )
	{
		return;
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_SEE, 0 );

	G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/see.wav") );
	G_Sound( self, TRACK_CHANNEL_5, seeLoopSound );
}

void ForceDeathfield( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_PROTECT ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Absorb.
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	//{
	//	WP_ForcePowerStop( self, FP_RAGE );
	//}
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
	//{
	//	WP_ForcePowerStop( self, FP_ABSORB );
	//}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_PROTECT, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_DEATHFIELD);
	G_Sound( self, TRACK_CHANNEL_3, deathfieldLoopSound );
}


void ForceProtect( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_PROTECT ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Absorb.
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	//{
	//	WP_ForcePowerStop( self, FP_RAGE );
	//}
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
	//{
	//	WP_ForcePowerStop( self, FP_ABSORB );
	//}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_PROTECT, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_PROTECT);
	G_Sound( self, TRACK_CHANNEL_3, protectLoopSound );
}
void ForceDeathsight( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_ABSORB ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	//{
	//	WP_ForcePowerStop( self, FP_RAGE );
	//}
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
	//{
	//	WP_ForcePowerStop( self, FP_PROTECT );
	//}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_ABSORB, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_DEATHSIGHT);
	G_Sound( self, TRACK_CHANNEL_3, deathsightLoopSound );
}
void ForceAbsorb( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_ABSORB ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE) )
	//{
	//	WP_ForcePowerStop( self, FP_RAGE );
	//}
	//if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
	//{
	//	WP_ForcePowerStop( self, FP_PROTECT );
	//}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_ABSORB, 0 );
	G_PreDefSound(self->client->ps.origin, PDSOUND_ABSORB);
	G_Sound( self, TRACK_CHANNEL_3, absorbLoopSound );
}



void ForceValor( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_RAGE ) )
	{
		return;
	}

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
//	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
//	{
//		WP_ForcePowerStop( self, FP_PROTECT );
//	}
//	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
//	{
//		WP_ForcePowerStop( self, FP_ABSORB );
//	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_RAGE, 0 );

	G_Sound( self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/extreme.wav") );
	G_Sound( self, TRACK_CHANNEL_3, extremeLoopSound );
}



void ForceRage( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_RAGE ) )
	{
		return;
	}

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
//	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
//	{
//		WP_ForcePowerStop( self, FP_PROTECT );
//	}
//	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
//	{
//		WP_ForcePowerStop( self, FP_ABSORB );
//	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart( self, FP_RAGE, 0 );

	G_Sound( self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/rage.wav") );
	G_Sound( self, TRACK_CHANNEL_3, rageLoopSound );
}
qboolean ForceLightningCheckattack(gentity_t* self)
{	
if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3 || self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3  || self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PUSH] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] == FORCE_LEVEL_3)
					{
	if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING)
	{
		return qtrue;
	}
	else
		{
	return qfalse;
	}
}
else
{
	return qfalse;
}
}

void ForceJudgement( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//[ForceSys]
	if( !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//[/ForceSys]
	{//racc - can't use this power while low on force or can't use this power now.
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) || self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN) ))
	{
		return;
	}
	
	
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
																								 
																		 
		 
  

	//Shoot judgement from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/judgement") );
	
	if(self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
		WP_ForcePowerStart( self, FP_LIGHTNING, 0 );
	else
		WP_ForcePowerStart( self, FP_LIGHTNING, 0 );
	//[ForceSys]
	
	

	
	
	//WP_ForcePowerStart( self, FP_LIGHTNING, 500 );
	//[ForceSys]
}

void ForceLightning( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//[ForceSys]
	if( !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//[/ForceSys]
	{//racc - can't use this power while low on force or can't use this power now.
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) || self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN) ))
	{
		return;
	}
	
	
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
																								 
																		 
		 
  

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/lightning") );
	
	if(self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
		WP_ForcePowerStart( self, FP_LIGHTNING, 0 );
	else
		WP_ForcePowerStart( self, FP_LIGHTNING, 0 );
	//[ForceSys]
	
	

	
	
	//WP_ForcePowerStart( self, FP_LIGHTNING, 500 );
	//[ForceSys]
}

qboolean IsHybrid(gentity_t *ent)
{
	qboolean jedi = qfalse,merc = qfalse;

	if(ent->client->ps.fd.forcePowersKnown & (1 << FP_SEE))
	jedi = qtrue;

	if(ent->client->skillLevel[SK_JETPACK])
	merc = qtrue;
	else if(ent->client->skillLevel[SK_PISTOL])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_BLASTER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_THERMAL])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_ROCKET])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_BACTA])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_FLAMETHROWER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_BOWCASTER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_FORCEFIELD])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_CLOAK])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_SEEKER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_SENTRY])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_DETPACK])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_REPEATER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_DISRUPTOR])
	merc = qtrue;
	else if(ent->client->skillLevel[SK_TRIPMINE])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_DEMP2])
		merc = qtrue;	
	else if(ent->client->skillLevel[SK_FLECHETTE])
		merc = qtrue;	
	else if(ent->client->skillLevel[SK_CONCUSSION])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_OLD])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_EWEB])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_BINOCULARS])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_WRIST])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_ELECTROSHOCKER])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_SPHERESHIELD])
		merc = qtrue;   
	else if(ent->client->skillLevel[SK_OVERLOAD])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_SQUADTEAM])
		merc = qtrue;
	else if(ent->client->skillLevel[SK_VEHICLEMOUNT])
		merc = qtrue;	
	else if(ent->client->skillLevel[SK_GRAPPLE])
		merc = qtrue;
	if(jedi && merc)
		return qtrue;			 
return qfalse;
}


qboolean OJP_CounterForce(gentity_t *attacker, gentity_t *defender, int attackPower)
{//generically checks to see if the defender is able to block an attack from this attacker 
	int abilityDef;		//the difference in skill between the defender's defend power and the attacker's attack power.

//	if(BG_IsUsingHeavyWeap(&defender->client->ps))
//	{//can't block force powers while using heavy weapons
//		return qfalse;
//	}

	if(IsMerc(defender) && defender->client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_1)
		return qfalse;

	if( !(defender->client->ps.fd.forcePowersKnown & (1 << attackPower)) 
		&& !(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1) )
	{//doesn't have absorb or same power as the attack power.
		return qfalse;
	}
	//determine ability difference
	abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[attackPower];

	if(abilityDef > attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[FP_ABSORB])
	{//defender's absorb ability is stronger than their attackPower ability, use that instead.
		abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[FP_ABSORB];
	}
	
	if(abilityDef >= 2)
	{//defender is largely weaker than the attacker (2 levels)
		if(!WalkCheck(defender) || defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{//can't block much stronger Force power while running or in mid-air
			return qfalse;
		}
	}
	else if(abilityDef >= 1)
	{//defender is slightly weaker than their attacker
		if(defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			return qfalse;
		}
	}

 	if(PM_SaberInBrokenParry(defender->client->ps.saberMove))
	{//can't block while stunned
		return qfalse;
	}

	if(BG_InSlowBounce(&defender->client->ps) && defender->client->ps.userInt3 & (1 << FLAG_OLDSLOWBOUNCE))
	{//can't block lightning while in the heavier slow bounces.
		return qfalse;
	}

	if( defender->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY )
	{//can't block if we're too off balance.
		return qfalse;
	}
	if( defender->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT
		&& attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
	{//can't block if we're too off balance and they are using Juyo's perk
		return qfalse;
	}

	//Don't allow force blocking at below certain DP levels based on your absorb force power level.
//	if(defender->client->ps.stats[STAT_DODGE] <= DodgeAbsorbBlockLevels[defender->client->ps.fd.forcePowerLevel[FP_ABSORB]])
//	{
//		return qfalse;
//	}

	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{//can block force while using forceHandExtend. This may be a temporary bug fix.

		return qtrue;
	}

	if(IsHybrid(defender))
	{
		defender->client->ps.userInt3 |= (1<<FLAG_BLOCKING);
		defender->client->blockTime = level.time + 1000;
	}
	
	return qtrue;
}


//[ForceSys]
#define DODGE_SABERBLOCKLIGHTNING	1 //the DP cost to block lightning
qboolean OJP_BlockEnergy(gentity_t *attacker, gentity_t *defender, int dpBlockCost, forcePowers_t forcePower, qboolean forcePowerVariation)
{//defender is attempting to block lightning.  Try to do it.

	qboolean saberLightBlock = qtrue;
	if(!defender || !defender->client || !attacker || !attacker->client)
	{//bad infor state
		return qfalse;
	}
	
	if ((defender->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !defender->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return qfalse;
	}
	
	if (forcePower == FP_LIGHTNING)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_LIGHTNING))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_LIGHTNING] >= attacker->client->ps.fd.forcePowerLevel[FP_LIGHTNING] && attacker->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_LIGHTNING] >= attacker->client->ps.fd.forcePowerLevel[FP_LIGHTNING] && attacker->client->skillLevel[SK_LIGHTNINGA] <= FORCE_LEVEL_1 && defender->client->skillLevel[SK_LIGHTNINGA] <= FORCE_LEVEL_1 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		}
	}
	else if (forcePower == FP_DRAIN)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_DRAIN))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_DRAIN] >= attacker->client->ps.fd.forcePowerLevel[FP_DRAIN] && attacker->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_DRAIN] >= attacker->client->ps.fd.forcePowerLevel[FP_DRAIN] && attacker->client->skillLevel[SK_DRAINA] <= FORCE_LEVEL_1 && defender->client->skillLevel[SK_DRAINA] <= FORCE_LEVEL_1 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		}				
	}

	if(defender->client->ps.weapon != WP_SABER  //not using saber
		|| defender->client->ps.saberHolstered == 2 //sabers off
		|| defender->client->ps.saberInFlight)  //saber not here
	{//saber not currently in use or available, attempt to use our hands instead.
		saberLightBlock = qfalse;
	}

	if(!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, 0.0f)
		&& (defender->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL //too low on DP
			|| !saberLightBlock) ) //can't block behind us while hand blocking.
	{//not facing the lightning attacker and low on DP!
		return qfalse;
	}

	//determine the cost to block the lightning


	//check to see if we have enough DP
	if(defender->client->ps.stats[STAT_DODGE] < dpBlockCost)
	{
		return qfalse;
	}

	if(saberLightBlock)
	{
		//ok, we can do it.  Hold up the saber to block it.
		defender->client->ps.saberBlocked = BLOCKED_TOP;
	}
	else
	{//use our hand to block the lightning
		defender->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		defender->client->ps.forceHandExtendTime = level.time + 500;
	}

	//charge us some DP as well.
	//[ExpSys]
	G_DodgeDrain(defender, attacker, dpBlockCost);
	//defender->client->ps.stats[STAT_DODGE] -= dpBlockCost;
	//[/ExpSys]

	return qtrue;

}

qboolean OJP_BlockInfluence(gentity_t *attacker, gentity_t *defender, int dpBlockCost, forcePowers_t forcePower, qboolean forcePowerVariation)
{
	//[ForceSys]
	/*
	int powerUse = 0;

	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (defender->client->ps.weaponTime > 0)
	{
		return 0;
	}
	*/
	//[/ForceSys]

	if ( defender->health <= 0 )
	{
		return qfalse;
	}
/*
	if ( defender->client->ps.powerups[PW_DISINT_4] > level.time )took this out to see if its causing a bug, and it was!
	{//racc in the process of getting pushed/pulled already.
		return 0;
	}
*/
	//[ForceSys]
	/*
	if (defender->client->ps.weaponstate == WEAPON_CHARGING ||
		defender->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}
	*/
	if ((defender->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !defender->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return qfalse;
	}
	
	if (forcePower == FP_TELEPATHY)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_TELEPATHY))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TELEPATHY] >= attacker->client->ps.fd.forcePowerLevel[FP_TELEPATHY] && attacker->client->skillLevel[SK_TELEPATHYA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_TELEPATHYA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TELEPATHY] >= attacker->client->ps.fd.forcePowerLevel[FP_TELEPATHY] && attacker->client->skillLevel[SK_TELEPATHYA] <= FORCE_LEVEL_1 && defender->client->skillLevel[SK_TELEPATHYA] <= FORCE_LEVEL_1 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		}			
	}
	else if (forcePower == FP_TEAM_HEAL)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_TEAM_HEAL))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] >= attacker->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] && attacker->client->skillLevel[SK_STASISA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_STASISA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] >= attacker->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] && attacker->client->skillLevel[SK_STASISA] <= FORCE_LEVEL_1 && defender->client->skillLevel[SK_STASISA] <= FORCE_LEVEL_1 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}		
		}
	}

	
	

		
	if(!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, 0.0f) 
		&& defender->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
	{//not facing the attacker and low on DP!
		return qfalse;
	}
	
	//check to see if we have enough DP
	if(defender->client->ps.stats[STAT_DODGE] < dpBlockCost)
	{
		return qfalse;
	}

	//use our hand to block the lightning
	defender->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	defender->client->ps.forceHandExtendTime = level.time + 500;
	

	//charge us some DP as well.
	//[ExpSys]
	G_DodgeDrain(defender, attacker, dpBlockCost);
	//defender->client->ps.stats[STAT_DODGE] -= dpBlockCost;
	//[/ExpSys]

	return qtrue;
}




qboolean OJP_BlockStatus(gentity_t *attacker, gentity_t *defender, int dpBlockCost, forcePowers_t forcePower, qboolean forcePowerVariation)
{
	//[ForceSys]
	/*
	int powerUse = 0;

	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (defender->client->ps.weaponTime > 0)
	{
		return 0;
	}
	*/
	//[/ForceSys]

	if ( defender->health <= 0 )
	{
		return qfalse;
	}
/*
	if ( defender->client->ps.powerups[PW_DISINT_4] > level.time )took this out to see if its causing a bug, and it was!
	{//racc in the process of getting pushed/pulled already.
		return 0;
	}
*/
	//[ForceSys]
	/*
	if (defender->client->ps.weaponstate == WEAPON_CHARGING ||
		defender->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}
	*/
	if ((defender->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !defender->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return qfalse;
	}
	
	if (forcePower == FP_PROTECT)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_PROTECT))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_PROTECT] >= attacker->client->ps.fd.forcePowerLevel[FP_PROTECT] && attacker->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}		
		}			
	}
	else if (forcePower == FP_ABSORB)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_ABSORB))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_ABSORB] >= attacker->client->ps.fd.forcePowerLevel[FP_ABSORB] && attacker->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		}			
	}

	
	

		
	if(!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, 0.0f) 
		&& defender->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
	{//not facing the attacker and low on DP!
		return qfalse;
	}
	
	//check to see if we have enough DP
	if(defender->client->ps.stats[STAT_DODGE] < dpBlockCost)
	{
		return qfalse;
	}

	//use our hand to block the lightning
	defender->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	defender->client->ps.forceHandExtendTime = level.time + 500;
	

	//charge us some DP as well.
	//[ExpSys]
	G_DodgeDrain(defender, attacker, dpBlockCost);
	//defender->client->ps.stats[STAT_DODGE] -= dpBlockCost;
	//[/ExpSys]
	
	return qtrue;
}




qboolean OJP_BlockFocus(gentity_t *attacker, gentity_t *defender, int dpBlockCost, forcePowers_t forcePower, qboolean forcePowerVariation)
{
	//[ForceSys]
	/*
	int powerUse = 0;

	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (defender->client->ps.weaponTime > 0)
	{
		return 0;
	}
	*/
	//[/ForceSys]

	if ( defender->health <= 0 )
	{
		return qfalse;
	}
/*
	if ( defender->client->ps.powerups[PW_DISINT_4] > level.time )took this out to see if its causing a bug, and it was!
	{//racc in the process of getting pushed/pulled already.
		return 0;
	}
*/
	//[ForceSys]
	/*
	if (defender->client->ps.weaponstate == WEAPON_CHARGING ||
		defender->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}
	*/
	
	if ((defender->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !defender->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return qfalse;
	}
	
	if (forcePower == FP_TEAM_FORCE)
	{//not facing the attacker and low on DP!
		if(!OJP_CounterForce(attacker, defender, FP_TEAM_FORCE))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(defender->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && defender->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] >= attacker->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] && attacker->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2 && defender->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(defender->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] >= attacker->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] && attacker->client->skillLevel[SK_DESTRUCTIONA] <= FORCE_LEVEL_1 && defender->client->skillLevel[SK_DESTRUCTIONA] <= FORCE_LEVEL_1 ))
		{//not facing the attacker and low on DP!
			return qfalse;
		}	
		}	
		}			
	}

		
	if(!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, 0.0f) 
		&& defender->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
	{//not facing the attacker and low on DP!
		return qfalse;
	}
	
	//check to see if we have enough DP
	if(defender->client->ps.stats[STAT_DODGE] < dpBlockCost)
	{
		return qfalse;
	}

	//use our hand to block the lightning
	defender->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	defender->client->ps.forceHandExtendTime = level.time + 500;
	

	//charge us some DP as well.
	//[ExpSys]
	G_DodgeDrain(defender, attacker, dpBlockCost);
	//defender->client->ps.stats[STAT_DODGE] -= dpBlockCost;
	//[/ExpSys]
	
	return qtrue;
}





//[/ForceSys]
//[ForceSys]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//[/ForceSys]
void ForceJudgementDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	//[ForceSys]
	//the target saber blocked the judgement
	qboolean saberBlocked = qfalse;
	//[/ForceSys]
	int JUDGEMENT_TIME = 2500;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if ( traceEnt->client )
		{//an enemy or object
			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > 250)
				{
					traceEnt->client->ps.fd.forcePower = 250;
				}
				return;
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_LIGHTNING))
			{
				//[ForceSys]
				int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				
				// removed the absorb code stuff.
	int modPowerLevel = -1;
				
	
				
				//[/ForceSys]

	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
					{
						dmg = 1;
						}			
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_2)
					{
						dmg = 3;
						}
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_3)
					{
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					dmg = 7;
				}
				else
				{
					dmg =5;
				}
					}
				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_LIGHTNING, self->client->ps.fd.forcePowerLevel[FP_LIGHTNING], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LIGHTNING]][FP_LIGHTNING]);
				}

				if (modPowerLevel != -1)
				{

						dmg = 0;
						traceEnt->client->noLightningTime = level.time + 400;

				}
				if ( traceEnt->client )
				{
				if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					dmg = 0;
					traceEnt->client->noLightningTime = level.time + 400;
				}
				}
//				//[ForceSys]
				if (dmg)
				{
				saberBlocked = OJP_BlockEnergy(self, traceEnt, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LIGHTNING]][FP_LIGHTNING], FP_LIGHTNING,qtrue);
				}
				if (dmg && !saberBlocked)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					//G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );


				if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
					{
						traceEnt->client->disablingTime = level.time + JUDGEMENT_TIME;		
					}			
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_2)
					{
						traceEnt->client->disablingTime = level.time + 2*JUDGEMENT_TIME;
					}
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_3)
					{
					if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
					{//jackin' 'em up, Palpatine-style
						traceEnt->client->disablingTime = level.time + 4*JUDGEMENT_TIME;
					}
					else
					{
						traceEnt->client->disablingTime = level.time + 3*JUDGEMENT_TIME;
					}
					}
					//[ForceSys]
					//judgement also blasts the target back.
					G_Throw(traceEnt, dir, 100);
					if(!WalkCheck(traceEnt) 
					|| (WalkCheck(traceEnt) && traceEnt->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY) 
					|| BG_IsUsingHeavyWeap(&traceEnt->client->ps)
					|| PM_SaberInBrokenParry(traceEnt->client->ps.saberMove)
					|| traceEnt->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
					{
						G_Knockdown(traceEnt, self, dir, 300, qtrue);
					}
					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
						G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block judgement
						int rSaberNum = 0;
						int rBladeNum = 0;						
						traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
						if ( saberBlocked
							&& traceEnt->client
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight )
						{
							vec3_t	end2;
							vec3_t ang = { 0, 0, 0};
							ang[0] = flrand(0,360);
							ang[1] = flrand(0,360);
							ang[2] = flrand(0,360);
							VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
							G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}

					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if (traceEnt->client->judgementTime < (level.time + JUDGEMENT_TIME/2) && (dmg && !saberBlocked))
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						gentity_t	*tent;
						tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FORCE_JUDGEMENT);
						tent->s.eventParm = DirToByte(dir);
						tent->s.owner = traceEnt->s.number;
						traceEnt->client->judgementTime = level.time + JUDGEMENT_TIME;							
					}
					if ( traceEnt->client->ps.powerups[PW_CLOAKED] )
					{//disable cloak temporarily
						Jedi_Decloak( traceEnt );
						traceEnt->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
					{//disable cloak temporarily
						Sphereshield_Off( traceEnt );
						traceEnt->client->sphereshieldToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if ( traceEnt->client->ps.powerups[PW_OVERLOADED] )
					{//disable cloak temporarily
						Overload_Off( traceEnt );
						traceEnt->client->overloadToggleTime = level.time + Q_irand( 3000, 10000 );
					}	
					if ( traceEnt->client->jetPackOn )
					{//disable cloak temporarily
						Jetpack_Off(traceEnt);
						traceEnt->client->jetPackToggleTime = level.time + Q_irand( 3000, 10000 );
					}
				}

			}
		}
	}
}




void ForceLightningDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	//[ForceSys]
	//the target saber blocked the lightning
	qboolean saberBlocked = qfalse;
	//[/ForceSys]
	int LIGHTNING_TIME = 2500;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if ( traceEnt->client )
		{//an enemy or object
			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > 250)
				{
					traceEnt->client->ps.fd.forcePower = 250;
				}
				return;
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_LIGHTNING))
			{
				//[ForceSys]
				int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				
				// removed the absorb code stuff.
	int modPowerLevel = -1;
				
	
				
				//[/ForceSys]

	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_1)
					{
						dmg = 1;
						}			
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_2)
					{
						dmg = 3;
						}
				else if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] == FORCE_LEVEL_3)
					{
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					dmg = 7;
				}
				else
				{
					dmg =5;
				}
					}
				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_LIGHTNING, self->client->ps.fd.forcePowerLevel[FP_LIGHTNING], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LIGHTNING]][FP_LIGHTNING]);
				}

				if (modPowerLevel != -1)
				{

						dmg = 0;
						traceEnt->client->noLightningTime = level.time + 400;

				}
				if ( traceEnt->client )
				{
				if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					dmg = 0;
					traceEnt->client->noLightningTime = level.time + 400;
				}
				}
				//[ForceSys]
				if (dmg)
				{
				saberBlocked = OJP_BlockEnergy(self, traceEnt, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LIGHTNING]][FP_LIGHTNING], FP_LIGHTNING,qfalse);
				}
				if (dmg && !saberBlocked)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );
					
					//[ForceSys]
					//lightning also blasts the target back.
					G_Throw(traceEnt, dir, 100);
					if(!WalkCheck(traceEnt) 
					|| (WalkCheck(traceEnt) && traceEnt->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY) 
					|| BG_IsUsingHeavyWeap(&traceEnt->client->ps)
					|| PM_SaberInBrokenParry(traceEnt->client->ps.saberMove)
					|| traceEnt->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
					{
						G_Knockdown(traceEnt, self, dir, 300, qtrue);
					}
					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
						G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
						int rSaberNum = 0;
						int rBladeNum = 0;						
						traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
						if ( saberBlocked
							&& traceEnt->client
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight )
						{
							vec3_t	end2;
							vec3_t ang = { 0, 0, 0};
							ang[0] = flrand(0,360);
							ang[1] = flrand(0,360);
							ang[2] = flrand(0,360);
							VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
							G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if (traceEnt->client->lightningTime < (level.time + LIGHTNING_TIME/2) && (dmg && !saberBlocked))
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						gentity_t	*tent;
						tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FORCE_LIGHTNING);
						tent->s.eventParm = DirToByte(dir);
						tent->s.owner = traceEnt->s.number;
						traceEnt->client->lightningTime = level.time + LIGHTNING_TIME;
					}
					if ( traceEnt->client->ps.powerups[PW_CLOAKED] )
					{//disable cloak temporarily
						Jedi_Decloak( traceEnt );
						traceEnt->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
					{//disable cloak temporarily
						Sphereshield_Off( traceEnt );
						traceEnt->client->sphereshieldToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if ( traceEnt->client->ps.powerups[PW_OVERLOADED] )
					{//disable cloak temporarily
						Overload_Off( traceEnt );
						traceEnt->client->overloadToggleTime = level.time + Q_irand( 3000, 10000 );
					}	
					if ( traceEnt->client->jetPackOn )
					{//disable cloak temporarily
						Jetpack_Off(traceEnt);
						traceEnt->client->jetPackToggleTime = level.time + Q_irand( 3000, 10000 );
					}
				}

			}
		}
	}
}

void ForceShootJudgement( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	if ( self->health <= 0 )
	{
		return;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = FORCE_LIGHTNING_RADIUS, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}
		
			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceJudgementDamage( self, traceEnt, dir, ent_org );
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );
		
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			return;
		}
		
		traceEnt = &g_entities[tr.entityNum];
		ForceJudgementDamage( self, traceEnt, forward, tr.endpos );
	}
}

void ForceShootLightning( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	if ( self->health <= 0 )
	{
		return;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = FORCE_LIGHTNING_RADIUS, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}
		
			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceLightningDamage( self, traceEnt, dir, ent_org );
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );
		
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			return;
		}
		
		traceEnt = &g_entities[tr.entityNum];
		ForceLightningDamage( self, traceEnt, forward, tr.endpos );
	}
}



extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//[/ForceSys]

extern qboolean G_BoxInBounds( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs );

//-----------------------------------------------------------------------------
static void FPSetStart( gentity_t *ent, vec3_t start, vec3_t mins, vec3_t maxs )
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t	tr;
	vec3_t	entMins;
	vec3_t	entMaxs;

	VectorAdd( ent->r.currentOrigin, ent->r.mins, entMins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, entMaxs );

	if ( G_BoxInBounds( start, mins, maxs, entMins, entMaxs ) )
	{
		return;
	}

	if ( !ent->client )
	{
		return;
	}

	trap_Trace( &tr, ent->client->ps.origin, mins, maxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP );

	if ( tr.startsolid || tr.allsolid )
	{
		return;
	}

	if ( tr.fraction < 1.0f )
	{
		VectorCopy( tr.endpos, start );
	}
}

void ForceBlinding( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//[ForceSys]
	if( !WP_ForcePowerUsable( self, FP_TEAM_FORCE ) )
	//if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//[/ForceSys]
	{//racc - can't use this power while low on force or can't use this power now.
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) ))
	{
		return;
	}
	
	
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
																								 
																		 
		 
  

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 1;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/blinding") );
	
	if(self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_1)
		WP_ForcePowerStart( self, FP_TEAM_FORCE, 0 );
	else
		WP_ForcePowerStart( self, FP_TEAM_FORCE, 0 );
	//[ForceSys]
	
	//WP_ForcePowerStart( self, FP_LIGHTNING, 500 );
	//[ForceSys]
}

void ForceDestruction( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//[ForceSys]
	if( !WP_ForcePowerUsable( self, FP_TEAM_FORCE ) )
	//if ( self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING ) )
	//[/ForceSys]
	{//racc - can't use this power while low on force or can't use this power now.
		return;
	}
	if ( self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) ))
	{
		return;
	}
	
	
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
																								 
																		 
		 
  

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 1;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/destruction") );
	
	if(self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_1)
		WP_ForcePowerStart( self, FP_TEAM_FORCE, 0 );
	else
		WP_ForcePowerStart( self, FP_TEAM_FORCE, 0 );
	//[ForceSys]
	
	//WP_ForcePowerStart( self, FP_LIGHTNING, 500 );
	//[ForceSys]
}


void ForceBlindingDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	//[ForceSys]
	//the target saber blocked the lightning
	//[/ForceSys]
	qboolean forceBlocked = qfalse;
	int BLINDING_TIME = 2500;
	int poweradd = 0;
	int modPowerLevel = -1;
	int dmg = 1;
	gentity_t *te = NULL;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;


	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_1)
	{
	BLINDING_TIME *= 1.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
	BLINDING_TIME *= 2.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
	BLINDING_TIME *= 4.0;
	}
	
	
	if ( traceEnt && traceEnt->client)
	{


	if (ForcePowerUsableOn(self, traceEnt, FP_TEAM_FORCE))
	{
				//[ForceSys]
	int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				
				// removed the absorb code stuff.
	int modPowerLevel = -1;
				
	
				
				//[/ForceSys]

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_1)
	{
		dmg *= 1;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
		dmg *= 2;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
		dmg *= 3;
	}
	
	if (traceEnt->client)
	{
		modPowerLevel = WP_AbsorbConversion(traceEnt->client, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_TEAM_FORCE, self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE]);
	}

	if (modPowerLevel != -1)
	{

			dmg = 0;

	}
	
	if ( traceEnt->client )
	{
	if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
	{
		dmg = 0;
	}
	}	
	
	forceBlocked = OJP_BlockFocus(self, traceEnt,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE], FP_TEAM_FORCE,qfalse);		
		//At this point we know we got one, so add him into the collective event client bitflag

		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

	if (traceEnt->client->blindingTime < (level.time + BLINDING_TIME/2) && (dmg && !forceBlocked))
	{

	gentity_t	*tent;
	tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FORCE_BLINDED);
	tent->s.eventParm = DirToByte(dir);
	tent->s.owner = traceEnt->s.number;
	traceEnt->client->blindingTime = level.time + BLINDING_TIME;
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_1)
	{
	tent->s.otherEntityNum2 = 1;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
	tent->s.otherEntityNum2 = 2;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
	tent->s.otherEntityNum2 = 4;
	}	
	}	
	
	}	
	}		
}



#define FORCE_DESTRUCTION_SIZE			4
static void ForceDestructionDamage (gentity_t* ent)
{
	vec3_t	start;
	vec3_t	dir;
	int		damage	= 50;
	float	vel = 2000.0;
	gentity_t *missile;

	//hold us still for a bit
	//ent->client->ps.pm_time = 300;
	//ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	//add viewkick
//	if ( ent->s.number < MAX_CLIENTS//player only
//		&& !cg.renderingThirdPerson )//gives an advantage to being in 3rd person, but would look silly otherwise
//	{//kick the view back
//		cg.kick_angles[PITCH] = Q_flrand( -10, -15 );
//		cg.kick_time = level.time;
//	}
	//mm..yeah..this needs some reworking for mp

	if (ent->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
		damage = 100;
	}
	else if (ent->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
		if (ent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
			|| ent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
			|| ent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
			|| ent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
		{//jackin' 'em up, Palpatine-style
			damage = 400;
		}
		else
		{
			damage = 200;
		}
	}

	AngleVectors(ent->client->ps.viewangles, dir, NULL, NULL);
	VectorNormalize(dir);

	VectorCopy(ent->client->renderInfo.eyePoint, start);
	FPSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	missile = CreateMissile( start, dir, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_STUN_BATON;
	missile->mass = 10;
	// Make it easier to hit things
	VectorSet( missile->r.maxs, FORCE_DESTRUCTION_SIZE, FORCE_DESTRUCTION_SIZE, FORCE_DESTRUCTION_SIZE);
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_FORCE_DESTRUCTION;
	missile->splashMethodOfDeath = MOD_FORCE_DESTRUCTION;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = damage/2;
	missile->splashRadius = FORCE_DESTRUCTION_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}
void ForceShootBlinding(gentity_t *self)
{
	float radius = MAX_BLINDING_DISTANCE;
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	
	
	if ( self->health <= 0 )
	{
		return;
	}


	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );


		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
		ForceBlindingDamage( self, traceEnt, dir, ent_org );
		}
}

void ForceShootDestruction( gentity_t *self )
{



	if ( self->health <= 0 )
	{
		return;
	}


	{//trace-line


	
		ForceDestructionDamage( self);
	}
}
void ForceSever( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}


	if ( !WP_ForcePowerUsable( self, FP_DRAIN ) )
	{
		return;
	}
	
	if ( self->client->ps.fd.forcePowerDebounce[FP_DRAIN] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay	  
  
		return;
	}
	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) || self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING) ))
	{
																						   
			  
																			   
		return;
	}
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
						 
				   
   
  

//	self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
//	self->client->ps.forceHandExtendTime = level.time + 1000;
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/sever.wav") );
	
																	   
	WP_ForcePowerStart( self, FP_DRAIN, 0 );
	 
											  
			 
 
												 
			 
}

void ForceDrain( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}


	if ( !WP_ForcePowerUsable( self, FP_DRAIN ) )
	{
		return;
	}
	
	if ( self->client->ps.fd.forcePowerDebounce[FP_DRAIN] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay	  
  
		return;
	}
	if ( self->client->ps.forceHandExtend != HANDEXTEND_NONE && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP) || self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING) ))
	{
																						   
			  
																			   
		return;
	}
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	
		   
	

									 
	{
		return;
	}

	//[ForceSys]
	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)) ))
	//if (self->client->ps.weaponTime > 0)
	//[/ForceSys]
	{
		return;
	}
						 
				   
   
  

//	self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
//	self->client->ps.forceHandExtendTime = level.time + 1000;
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/drain.wav") );
	
																	   
	WP_ForcePowerStart( self, FP_DRAIN, 0 );
	 
											  
			 
 
												 
			 
}


//[ForceSys]
void ForceSeverDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	qboolean saberBlocked = qfalse;
	
	gentity_t *tent;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if ( traceEnt->client )
		{//an enemy or object
			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > 250)
				{
					traceEnt->client->ps.fd.forcePower = 250;
				}
				return;
			}

			if (ForcePowerUsableOn(self, traceEnt, FP_DRAIN))
		{
				//[ForceSys]
				int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				

				int modPowerLevel = -1;
	
				//[/ForceSys]
				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1 )
					{
						dmg = 1;
						}			
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_2 )
					{
						dmg = 3;
						}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_3 )
					{
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					dmg = 7;
				}
				else
				{
					dmg =5;
				}
					}

				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.fd.forcePowerLevel[FP_DRAIN], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN]);
				}

				if (modPowerLevel != -1)
				{

						dmg = 0;


				}
				if ( traceEnt->client )
				{
				if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					dmg = 0;
				}
				}
				if (dmg)
				{				
				saberBlocked = OJP_BlockEnergy(self, traceEnt, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN], FP_DRAIN,qtrue);
				}
				//G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );
				if (dmg && !saberBlocked)
				{
					//traceEnt->client->ps.fd.forcePower -= (dmg);
					
					
					//rww - Shields can now absorb lightning too.
					
					//[ForceSys]
					//lightning also blasts the target back.
				if (traceEnt->client->ps.fd.forcePower > 0)
				{
					traceEnt->client->ps.fd.forcePower -= (dmg);
				if (traceEnt->client->ps.fd.forcePower < 0)
				{
					traceEnt->client->ps.fd.forcePower = 0;
				}		
				}
				if (traceEnt->client->ps.stats[STAT_DODGE] > 0 )
				{
					traceEnt->client->ps.stats[STAT_DODGE] -= (dmg);
				if (traceEnt->client->ps.stats[STAT_DODGE] < 0)
				{
					traceEnt->client->ps.stats[STAT_DODGE] = 0;
				}
				}						
					
	 
			
	

				

				if (self->client->ps.fd.forcePower < self->client->ps.fd.forcePowerMax &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->client->ps.fd.forcePower += dmg;
					if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
					{
						self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
					}
				}
				if (self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->client->ps.stats[STAT_DODGE] += dmg;
					if (self->client->ps.stats[STAT_DODGE] > self->client->ps.stats[STAT_MAX_DODGE])
					{
						self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
					}
				}


				}				
	  
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
						G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/drained", Q_irand(1, 3) )) );
					}
	 				
					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
						int rSaberNum = 0;
						int rBladeNum = 0;						
						traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
						if ( saberBlocked
							&& traceEnt->client
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight )
						{
							vec3_t	end2;
							vec3_t ang = { 0, 0, 0};
							ang[0] = flrand(0,360);
							ang[1] = flrand(0,360);
							ang[2] = flrand(0,360);
							VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
							G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
			if (traceEnt->client->forcePowerSoundDebounce < level.time  && (dmg && !saberBlocked)) 
				{
					tent = G_TempEntity( impactPoint, EV_FORCE_SEVERED);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;

					traceEnt->client->forcePowerSoundDebounce = level.time + 400;
				}
	
				
				}
			



			}
		}
	}
}

void ForceDrainDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	qboolean saberBlocked = qfalse;
	
	gentity_t *tent;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->takedamage )
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if ( traceEnt->client )
		{//an enemy or object
			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > 250)
				{
					traceEnt->client->ps.fd.forcePower = 250;
				}
				return;
			}

			if (ForcePowerUsableOn(self, traceEnt, FP_DRAIN))
		{
				//[ForceSys]
				int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				

				int modPowerLevel = -1;
	
				//[/ForceSys]
				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1 )
					{
						dmg = 1;
						}			
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_2 )
					{
						dmg = 3;
						}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_3 )
					{
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					dmg = 7;
				}
				else
				{
					dmg =5;
				}
					}

				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.fd.forcePowerLevel[FP_DRAIN], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN]);
				}

				if (modPowerLevel != -1)
				{

						dmg = 0;


				}
				if ( traceEnt->client )
				{
				if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					dmg = 0;
				}
				}
				if (dmg)
				{				
				saberBlocked = OJP_BlockEnergy(self, traceEnt, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN], FP_DRAIN,qfalse);
				}
				//G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );

				if (dmg && !saberBlocked)
				{
					//traceEnt->client->ps.fd.forcePower -= (dmg);
					
					
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_DARK );
					
					//[ForceSys]
					//lightning also blasts the target back.
				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += dmg;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					self->client->ps.stats[STAT_HEALTH] = self->health;
				}
				else if (self->client->ps.stats[STAT_HEALTH] == self->client->ps.stats[STAT_MAX_HEALTH] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0 )
				{
					self->client->ps.stats[STAT_ARMOR] += dmg;
					if (self->client->ps.stats[STAT_ARMOR] > self->client->ps.stats[STAT_MAX_ARMOR])
					{
						self->client->ps.stats[STAT_ARMOR] = self->client->ps.stats[STAT_MAX_ARMOR];
					}


				}
				}



				

	  
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
						G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/drained", Q_irand(1, 3) )) );
					}
	 				
					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
						int rSaberNum = 0;
						int rBladeNum = 0;						
						traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
						if ( saberBlocked
							&& traceEnt->client
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight )
						{
							vec3_t	end2;
							vec3_t ang = { 0, 0, 0};
							ang[0] = flrand(0,360);
							ang[1] = flrand(0,360);
							ang[2] = flrand(0,360);
							VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
							G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
			if (traceEnt->client->forcePowerSoundDebounce < level.time  && (dmg && !saberBlocked)) 
				{
					tent = G_TempEntity( impactPoint, EV_FORCE_DRAINED);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;

					traceEnt->client->forcePowerSoundDebounce = level.time + 400;
				}
				
				}
			



			}
		}
	}
}

int ForceShootSevere( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	int			gotOneOrMore = 0;

	if ( self->health <= 0 )
	{
		return 0;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = MAX_DRAIN_DISTANCE, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
															 						   			 
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
	
			if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}
		
			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
					trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
		ForceSeverDamage( self, traceEnt, forward, tr.endpos );
		gotOneOrMore = 1;
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );
		
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT);
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid || !g_entities[tr.entityNum].client  )
		{
			return 0;
		}
		
		traceEnt = &g_entities[tr.entityNum];
		ForceSeverDamage( self, traceEnt, forward, tr.endpos );
		gotOneOrMore = 1;
	}
	

	
	//BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, 1 ); //used to be 1, but this did, too, anger the God of Balance.

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return gotOneOrMore;
}

int ForceShootDrain( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	int			gotOneOrMore = 0;

	if ( self->health <= 0 )
	{
		return 0;
	}
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = MAX_DRAIN_DISTANCE, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
															 						   			 
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
	
			if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}
		
			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
					trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
		ForceDrainDamage( self, traceEnt, forward, tr.endpos );
		gotOneOrMore = 1;
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );
		
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT);
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid || !g_entities[tr.entityNum].client  )
		{
			return 0;
		}
		
		traceEnt = &g_entities[tr.entityNum];
		ForceDrainDamage( self, traceEnt, forward, tr.endpos );
		gotOneOrMore = 1;
	}
	

	
	//BG_ForcePowerDrain( &self->client->ps, FP_DRAIN, 1 ); //used to be 1, but this did, too, anger the God of Balance.

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return gotOneOrMore;
}
//[/ForceSys]


void ForceJumpCharge( gentity_t *self, usercmd_t *ucmd )
{ //I guess this is unused now. Was used for the "charge" jump type.
	float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	if ( self->health <= 0 )
	{
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION])
	{
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.fd.forceJumpAddTime = 0;
	}

	if (self->client->ps.fd.forceJumpAddTime >= level.time)
	{
		return;
	}

	//need to play sound
	if ( !self->client->ps.fd.forceJumpCharge )
	{
		G_Sound( self, TRACK_CHANNEL_1, G_SoundIndex("sound/weapons/force/jumpbuild.wav") );
	}

	//Increment
	if (self->client->ps.fd.forceJumpAddTime < level.time)
	{
		self->client->ps.fd.forceJumpCharge += forceJumpChargeInterval*50;
		self->client->ps.fd.forceJumpAddTime = level.time + 500;
	}

	//clamp to max strength for current level
	if ( self->client->ps.fd.forceJumpCharge > forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]] )
	{
		self->client->ps.fd.forceJumpCharge = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]];
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
	}

	//clamp to max available force power
	if ( self->client->ps.fd.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION] > self->client->ps.fd.forcePower )
	{//can't use more than you have
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
		self->client->ps.fd.forceJumpCharge = self->client->ps.fd.forcePower*forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);
	}
	
	//G_Printf("%f\n", self->client->ps.fd.forceJumpCharge);
}

int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd )
{
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy( self->client->ps.viewangles, view );
	view[0] = 0;
	AngleVectors( view, forward, right, NULL );
	if ( ucmd->forwardmove && ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if ( ucmd->rightmove > 0 )
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if ( ucmd->forwardmove || ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 100;
		}
		else if ( ucmd->forwardmove < 0 )
		{
			pushFwd = -100;
		}
		else if ( ucmd->rightmove > 0 )
		{
			pushRt = 100;
		}
		else if ( ucmd->rightmove < 0 )
		{
			pushRt = -100;
		}
	}

	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);

	G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);

	if (self->client->ps.fd.forceJumpCharge < JUMP_VELOCITY+40)
	{ //give him at least a tiny boost from just a tap
		self->client->ps.fd.forceJumpCharge = JUMP_VELOCITY+400;
	}

	if (self->client->ps.velocity[2] < -30)
	{ //so that we can get a good boost when force jumping in a fall
		self->client->ps.velocity[2] = -30;
	}

	VectorMA( self->client->ps.velocity, pushFwd, forward, jumpVel );
	VectorMA( self->client->ps.velocity, pushRt, right, jumpVel );
	jumpVel[2] += self->client->ps.fd.forceJumpCharge;
	if ( pushFwd > 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_FORWARD;
	}
	else if ( pushFwd < 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_BACKWARD;
	}
	else if ( pushRt > 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_RIGHT;
	}
	else if ( pushRt < 0 && self->client->ps.fd.forceJumpCharge > 200 )
	{
		return FJ_LEFT;
	}
	else
	{
		return FJ_UP;
	}
}

void ForceJump( gentity_t *self, usercmd_t *ucmd )
{
	float forceJumpChargeInterval;
	vec3_t	jumpVel;

	if ( self->client->ps.fd.forcePowerDuration[FP_LEVITATION] > level.time )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_LEVITATION ) )
	{
		return;
	}
	if ( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}

	self->client->fjDidJump = qtrue;

	forceJumpChargeInterval = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]]/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	WP_GetVelocityForForceJump( self, jumpVel, ucmd );

	//FIXME: sound effect
	self->client->ps.fd.forceJumpZStart = self->client->ps.origin[2];//remember this for when we land
	VectorCopy( jumpVel, self->client->ps.velocity );
	//wasn't allowing them to attack when jumping, but that was annoying
	//self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

	WP_ForcePowerStart( self, FP_LEVITATION, self->client->ps.fd.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION] );
	//self->client->ps.fd.forcePowerDuration[FP_LEVITATION] = level.time + self->client->ps.weaponTime;
	self->client->ps.fd.forceJumpCharge = 0;
	self->client->ps.forceJumpFlip = qtrue;
}

void WP_AddAsMindtricked(forcedata_t *fd, int entNum)
{
	if (!fd)
	{
		return;
	}

	if (entNum > 47)
	{
		fd->forceMindtrickTargetIndex4 |= (1 << (entNum-48));
	}
	else if (entNum > 31)
	{
		fd->forceMindtrickTargetIndex3 |= (1 << (entNum-32));
	}
	else if (entNum > 15)
	{
		fd->forceMindtrickTargetIndex2 |= (1 << (entNum-16));
	}
	else
	{
		fd->forceMindtrickTargetIndex |= (1 << entNum);
	}
}


qboolean ForceTelepathyCheckDirectNPCTarget( gentity_t *self, trace_t *tr, qboolean *tookPower )
{
	gentity_t	*traceEnt;
	qboolean	targetLive = qfalse, mindTrickDone = qfalse;
	vec3_t		tfrom, tto, fwd;
	float		radius = MAX_TRICK_DISTANCE;

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0]*radius;
	tto[1] = tfrom[1] + fwd[1]*radius;
	tto[2] = tfrom[2] + fwd[2]*radius;

	trap_Trace( tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID );
	
	if ( tr->entityNum == ENTITYNUM_NONE 
		|| tr->fraction == 1.0f
		|| tr->allsolid 
		|| tr->startsolid )
	{
		return qfalse;
	}
	
	traceEnt = &g_entities[tr->entityNum];
	
	if( traceEnt->NPC 
		&& traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
	{
		return qfalse;
	}

	if ( traceEnt && traceEnt->client  )
	{
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
		case CLASS_ATST://much too big to grip!
		//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_BOBAFETT:
		case CLASS_RANCOR:
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}

	if ( traceEnt->s.number < MAX_CLIENTS )
	{//a regular client
		return qfalse;
	}

	if ( targetLive && traceEnt->NPC )
	{//hit an organic non-player
		vec3_t	eyeDir;
		if ( G_ActivateBehavior( traceEnt, BSET_MINDTRICK ) )
		{//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		else if ( (self->NPC && traceEnt->client->playerTeam != self->client->playerTeam)
			|| (!self->NPC && traceEnt->client->playerTeam != self->client->playerTeam/* self->client->sess.sessionTeam */) )
		{//an enemy
			int override = 0;
			if ( (traceEnt->NPC->scriptFlags&SCF_NO_MIND_TRICK) )
			{
			}
			else if ( traceEnt->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_1 )
			{//haha!  Jedi aren't easily confused!
				
				{//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds
					NPC_PlayConfusionSound( traceEnt );
					if ( traceEnt->enemy )
					{
						G_ClearEnemy( traceEnt );
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound( traceEnt );
			}
			WP_ForcePowerStart( self, FP_TELEPATHY, override );
		}
		else if ( traceEnt->client->playerTeam == self->client->playerTeam )
		{//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if ( traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC!=NULL && !(traceEnt->NPC->scriptFlags&SCF_NO_RESPONSE) )
			{
				NPC_UseResponse( traceEnt, self, qfalse );
				WP_ForcePowerStart( self, FP_TELEPATHY, 1 );
			}
		}//NOTE: no effect on TEAM_NEUTRAL?
		AngleVectors( traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL );
		VectorNormalize( eyeDir );
		G_PlayEffectID( G_EffectIndex( "force/force_touch" ), traceEnt->client->renderInfo.eyePoint, eyeDir );

		//make sure this plays and that you cannot press fire for about 1 second after this
		//FIXME: BOTH_FORCEMINDTRICK or BOTH_FORCEDISTRACT
		//NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
		//FIXME: build-up or delay this until in proper part of anim
		mindTrickDone = qtrue;
	}
	else 
	{
		if ( self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && tr->fraction * 2048 > 64 )
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffectID( G_EffectIndex( "force/force_touch" ), tr->endpos, tr->plane.normal );
			//FIXME: these events don't seem to always be picked up...?
			//[CoOp]
			//Added sound/sight event for the sake of the NPCs.
			AddSoundEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue, qtrue );
			//AddSoundEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue );//, qtrue );
			//[/CoOp]
			AddSightEvent( self, tr->endpos, 512, AEL_SUSPICIOUS, 50 );
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			*tookPower = qtrue;
		}
		//NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
	}
	//self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	/*
	if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
	*/
	return qtrue;
}

//extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength );
void ForceCorrupt(gentity_t *self)
{
												
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_TRICK_DISTANCE;
	qboolean	tookPower = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}
	
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

//	if (self->client->ps.powerups[PW_REDFLAG] ||
//		self->client->ps.powerups[PW_BLUEFLAG])
//	{ //can't mindtrick while carrying the flag
//		return;
//	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) )
	{
		WP_ForcePowerStop( self, FP_TELEPATHY );
  
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TELEPATHY ) )
	{
		return;
	}

//	if(self->client->ps.weapon == WP_SABER)
//			WP_DeactivateSaber(self,qfalse);
/*
	if ( ForceTelepathyCheckDirectNPCTarget( self, &tr, &tookPower ) )
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}
*/
	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
	{
		visionArc = 15;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		//radius = MAX_TRICK_DISTANCE*2.0f;
	}



	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	
   
	}


	{
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) && ForcePowerUsableOn(self, ent, FP_TELEPATHY))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (OnSameTeam(self, ent))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
					else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
				}
			}
		
			ent = &g_entities[entityList[e]];
			if (ent && ent != self && ent->client)
			{
				if (WP_AbsorbConversion(ent->client, ent->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_TELEPATHY, self->client->ps.fd.forcePowerLevel[FP_TELEPATHY], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY])==-1)
				{	
				if (!OJP_BlockInfluence(self,ent,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY],FP_TELEPATHY,qtrue))
				{
				gotatleastone = qtrue;
						if ( ent->NPC )
						{//NPC
							NPC_PlayConfusionSound( ent );					
							int newPlayerTeam, newEnemyTeam;
							if ( ent->enemy )
							{
							G_ClearEnemy( ent );
							}
							ent->client->leader = self;
							
							if ( ent->client->playerTeam == NPCTEAM_ENEMY )
							{
							newPlayerTeam = NPCTEAM_PLAYER;
							newEnemyTeam = NPCTEAM_ENEMY;
							}
							else if ( ent->client->playerTeam == NPCTEAM_PLAYER )
							{
							newPlayerTeam = NPCTEAM_ENEMY;
							newEnemyTeam = NPCTEAM_PLAYER;
							}
							else
							{//neutral - wan't attack anyone
								newPlayerTeam = NPCTEAM_NEUTRAL;
								newEnemyTeam = NPCTEAM_NEUTRAL;
							}
						//store these for retrieval later
						ent->genericValue1 = ent->client->playerTeam;
						ent->genericValue2 = ent->client->enemyTeam;
						ent->genericValue3 = ent->s.teamowner;
						//set the new values
						ent->client->playerTeam = newPlayerTeam;
						ent->client->enemyTeam = newEnemyTeam;
						ent->s.teamowner = newPlayerTeam;
						//FIXME: need a *charmed* timer on this...?  Or do TEAM_PLAYERS assume that "confusion" means they should switch to team_enemy when done?
						ent->client->leader = self;
						ent->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];	



						ent->corruptionactivator = self;							
						}
						else
						{
						WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);
						ent->client->corruptedTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];



						ent->corruptionactivator = self;	
						}						
				
				}
				}
			}
			e++;
		}

  
		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/corrupt.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
  
  
	}

}
void ForceTelepathy(gentity_t *self)
{
												
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_TRICK_DISTANCE;
	qboolean	tookPower = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}
	
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

//	if (self->client->ps.powerups[PW_REDFLAG] ||
//		self->client->ps.powerups[PW_BLUEFLAG])
//	{ //can't mindtrick while carrying the flag
//		return;
//	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) )
	{
		WP_ForcePowerStop( self, FP_TELEPATHY );
  
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TELEPATHY ) )
	{
		return;
	}

//	if(self->client->ps.weapon == WP_SABER)
//			WP_DeactivateSaber(self,qfalse);
/*
	if ( ForceTelepathyCheckDirectNPCTarget( self, &tr, &tookPower ) )
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}
*/
	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
	{
		visionArc = 15;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		//radius = MAX_TRICK_DISTANCE*2.0f;
	}



	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	
   
	}


	{
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
		
		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) && ForcePowerUsableOn(self, ent, FP_TELEPATHY))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (OnSameTeam(self, ent))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
					else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
				}
			}
				
			ent = &g_entities[entityList[e]];
			if (ent && ent != self && ent->client)
			{
				if (WP_AbsorbConversion(ent->client, ent->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_TELEPATHY, self->client->ps.fd.forcePowerLevel[FP_TELEPATHY], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY])==-1)
				{	
				if (!OJP_BlockInfluence(self,ent,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY],FP_TELEPATHY,qfalse))
				{
				gotatleastone = qtrue;
				if(ent->NPC)
				{
					ent->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds	

					ent->confusionactivator = self;					
					NPC_PlayConfusionSound( ent );		
					if ( ent->enemy )
					{
						G_ClearEnemy( ent );
					}					
				}
				else
				{
				WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);



				ent->confusionactivator = self;		
				}					
				
				}
				}
			}
			e++;
		}

  
		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
  
  
	}

}


void ForceInsanity(gentity_t *self)
{
	int modPowerLevel = -1;
	int INSANITY_TIME = 5000;
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_STASIS_DISTANCE;
	qboolean	tookPower = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}
	
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

//	if (self->client->ps.powerups[PW_REDFLAG] ||
//		self->client->ps.powerups[PW_BLUEFLAG])
//	{ //can't mindtrick while carrying the flag
//		return;
//	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_TEAM_HEAL)) )
	{
		WP_ForcePowerStop( self, FP_TEAM_HEAL );
		
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TEAM_HEAL ) )
	{
		return;
	}

//	if(self->client->ps.weapon == WP_SABER)
//			WP_DeactivateSaber(self,qfalse);
/*
	if ( ForceStasisCheckDirectNPCTarget( self, &tr, &tookPower ) )
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}*/
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
	{
		visionArc = 15;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		//radius = MAX_STASIS_DISTANCE*2.0f;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
	{
	INSANITY_TIME *= 1.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
	INSANITY_TIME *= 2.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
	INSANITY_TIME *= 4.0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
		  
   
	}

	{
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) && ForcePowerUsableOn(self, ent, FP_TEAM_HEAL))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (OnSameTeam(self, ent))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
					else if (!ForcePowerUsableOn(self, ent, FP_TEAM_HEAL))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
				}
			}
				ent = &g_entities[entityList[e]];



			
			
			
			if (ent && ent != self && ent->client)
			{
				if (WP_AbsorbConversion(ent->client, ent->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_TEAM_HEAL, self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])==-1)
				{
				if (!OJP_BlockInfluence(self,ent,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL], FP_TEAM_HEAL,qtrue))
				{
				gotatleastone = qtrue;
					ent->client->insanityTime = level.time + INSANITY_TIME;
					ent->insanityactivator = self;		
					ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + INSANITY_TIME;


				//	G_AddEvent(ent, EV_STASIS, DirToByte(dir));
					G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/insanity.wav"));
				//	player_Freeze(ent);

					gentity_t	*tent;
					tent = G_TempEntity(ent->r.currentOrigin, EV_FORCE_INSANITY);
					tent->s.owner = ent->s.number;

				//	player_Freeze(ent);
					if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
					{
					tent->s.otherEntityNum2 = 1;
					}
					else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
					{
					tent->s.otherEntityNum2 = 2;
					}
					else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
					{
					tent->s.otherEntityNum2 = 4;
					}					
				
				}
				}
			}
			e++;
		}

		
		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TEAM_HEAL, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/insanity.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
		
		
	}

}


void ForceStasis(gentity_t *self)
{
	int modPowerLevel = -1;
	int STASIS_TIME = 5000;
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_STASIS_DISTANCE;
	qboolean	tookPower = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}
	
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

//	if (self->client->ps.powerups[PW_REDFLAG] ||
//		self->client->ps.powerups[PW_BLUEFLAG])
//	{ //can't mindtrick while carrying the flag
//		return;
//	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_TEAM_HEAL)) )
	{
		WP_ForcePowerStop( self, FP_TEAM_HEAL );
		
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_TEAM_HEAL ) )
	{
		return;
	}

//	if(self->client->ps.weapon == WP_SABER)
//			WP_DeactivateSaber(self,qfalse);
/*
	if ( ForceStasisCheckDirectNPCTarget( self, &tr, &tookPower ) )
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav") );
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}*/
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
	{
		visionArc = 15;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		//radius = MAX_STASIS_DISTANCE*2.0f;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
	{
	STASIS_TIME *= 1.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
	STASIS_TIME *= 2.0;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
	STASIS_TIME *= 4.0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
		  
   
	}

	{
		gentity_t *ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;

		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) && ForcePowerUsableOn(self, ent, FP_TEAM_HEAL))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (OnSameTeam(self, ent))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
					else if (!ForcePowerUsableOn(self, ent, FP_TEAM_HEAL))
					{
					entityList[e] = ENTITYNUM_NONE;
					}
				}
			}
				ent = &g_entities[entityList[e]];


			
			
			
			if (ent && ent != self && ent->client)
			{
				if (WP_AbsorbConversion(ent->client, ent->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_TEAM_HEAL, self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])==-1)
				{
				if (!OJP_BlockInfluence(self,ent,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL],FP_TEAM_HEAL,qfalse))
				{
				gotatleastone = qtrue;
					ent->client->stasisTime = level.time + STASIS_TIME;

					ent->stasisactivator = self;		
					ent->client->ps.userInt1 |= LOCK_MOVERIGHT;
					ent->client->ps.userInt1 |= LOCK_MOVELEFT;
					ent->client->ps.userInt1 |= LOCK_MOVEFORWARD;
					ent->client->ps.userInt1 |= LOCK_MOVEBACK;
					ent->client->ps.userInt1 |= LOCK_MOVEUP;
					ent->client->ps.userInt1 |= LOCK_MOVEDOWN;
					ent->client->ps.userInt1 |= LOCK_UP;
					ent->client->ps.userInt1 |= LOCK_DOWN;
					ent->client->ps.userInt1 |= LOCK_RIGHT;
					ent->client->ps.userInt1 |= LOCK_LEFT;	
					ent->client->viewLockTime = level.time + STASIS_TIME;
					ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + STASIS_TIME;
					if (ent->client->ps.eFlags & EF_WP_OPTION_2)
					{
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim3[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, STASIS_TIME);
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_3)
					{
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim5[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, STASIS_TIME);
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_4)
					{
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim7[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, STASIS_TIME);
					}
					else
					{
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, STASIS_TIME);
					}	

				//	G_AddEvent(ent, EV_STASIS, DirToByte(dir));
					G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav"));
					ent->client->ps.saberMove = LS_READY;//don't finish whatever saber anim you may have been in
					ent->client->ps.saberBlocked = BLOCKED_NONE;
				//	player_Freeze(ent);

					gentity_t	*tent;
					tent = G_TempEntity(ent->r.currentOrigin, EV_FORCE_STASIS);
					tent->s.owner = ent->s.number;
					if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_1)
					{
					tent->s.otherEntityNum2 = 1;
					}
					else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
					{
					tent->s.otherEntityNum2 = 2;
					}
					else if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
					{
					tent->s.otherEntityNum2 = 4;
					}								
				
				}
				}
				
			}
			e++;
		}

		
		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if ( !tookPower )
			{
				WP_ForcePowerStart( self, FP_TEAM_HEAL, 0 );
			}

			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav") );

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
		
		
	}

}


















void GEntity_UseFunc( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	GlobalUse(self, other, activator);
}


qboolean CanCounterThrow(gentity_t *self, gentity_t *thrower, int dpBlockCost, qboolean pull, qboolean forcePowerVariation)
{
	//[ForceSys]
	/*
	int powerUse = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return 0;
	}
	*/
	//[/ForceSys]

	if ( self->health <= 0 )
	{
		return qfalse;
	}
/*
	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )took this out to see if its causing a bug, and it was!
	{//racc in the process of getting pushed/pulled already.
		return 0;
	}
*/
	//[ForceSys]
	/*
	if (self->client->ps.weaponstate == WEAPON_CHARGING ||
		self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}
	*/
	
	if ((self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) && !self->client->ps.userInt3 & (1 << FLAG_ABSORB2)))
	{ //absorb is not active
		return qfalse;
	}
	if (pull)
	{//not facing the thrower and low on DP!
		if(!OJP_CounterForce(thrower, self, FP_PULL))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(self>client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && self->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(self->client->ps.fd.forcePowerLevel[FP_PULL] >= thrower->client->ps.fd.forcePowerLevel[FP_PULL] && thrower->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2 && self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2 ))
		{//not facing the thrower and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(self->client->ps.fd.forcePowerLevel[FP_PULL] >= thrower->client->ps.fd.forcePowerLevel[FP_PULL] && thrower->client->skillLevel[SK_PULLA] <= FORCE_LEVEL_1 && self->client->skillLevel[SK_PULLA] <= FORCE_LEVEL_1 ))
		{//not facing the thrower and low on DP!
			return qfalse;
		}	
		}		
		}		
	}
	else
	{//not facing the thrower and low on DP!
		if(!OJP_CounterForce(thrower, self, FP_PUSH))
		{//wasn't able to counter due to generic counter issue
			return qfalse;
		}
		if(!(self->client->ps.fd.forcePowersKnown & (1 << FP_ABSORB) && self->client->skillLevel[SK_ABSORBA] <= FORCE_LEVEL_1))
		{
		if(forcePowerVariation == qtrue)
		{//wasn't able to counter due to generic counter issue
		if (!(self->client->ps.fd.forcePowerLevel[FP_PUSH] >= thrower->client->ps.fd.forcePowerLevel[FP_PUSH] && thrower->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2 && self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2 ))
		{//not facing the thrower and low on DP!
			return qfalse;
		}	
		}	
		else
		{//wasn't able to counter due to generic counter issue
		if (!(self->client->ps.fd.forcePowerLevel[FP_PUSH] >= thrower->client->ps.fd.forcePowerLevel[FP_PUSH] && thrower->client->skillLevel[SK_PUSHA] <= FORCE_LEVEL_1 && self->client->skillLevel[SK_PUSHA] <= FORCE_LEVEL_1 ))
		{//not facing the thrower and low on DP!
			return qfalse;
		}	
		}		
		}	
	}

	
	

		
	if(!InFront(thrower->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.0f) 
		&& self->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
	{//not facing the thrower and low on DP!
		return qfalse;
	}
	
	//check to see if we have enough DP
	if(self->client->ps.stats[STAT_DODGE] < dpBlockCost)
	{
		return qfalse;
	}

	//use our hand to block the lightning
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 500;
	

	//charge us some DP as well.
	//[ExpSys]
	G_DodgeDrain(self, thrower, dpBlockCost);
	//self->client->ps.stats[STAT_DODGE] -= dpBlockCost;
	//[/ExpSys]

	return qtrue;
}

qboolean G_InGetUpAnim(playerState_t *ps)
{
	switch( (ps->legsAnim) )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	switch( (ps->torsoAnim) )
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	return qfalse;
}

void G_LetGoOfWall( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	if ( BG_InReboundJump( ent->client->ps.legsAnim ) 
		|| BG_InReboundHold( ent->client->ps.legsAnim ) )
	{
		ent->client->ps.legsTimer = 0;
	}
	if ( BG_InReboundJump( ent->client->ps.torsoAnim ) 
		|| BG_InReboundHold( ent->client->ps.torsoAnim ) )
	{
		ent->client->ps.torsoTimer = 0;
	}
}

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	256,//256,
	384,//384,
	512
};
//rwwFIXMEFIXME: incorporate this into the below function? Currently it's only being used by jedi AI

extern void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace );
//[ForceSys]
void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) ;
//[/ForceSys]
void ForceExplode( gentity_t *self, qboolean pull )
{
	//shove things in front of you away
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius = 512; //since it's view-based now. //350;
	int			powerLevel;
	int			visionArc;
	int			pushPower;
	int			pushPowerMod;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	float		dot1;
	trace_t		tr;
	int			x;
	vec3_t		pushDir;
	vec3_t		thispush_org;
	vec3_t		tfrom, tto, fwd, a;
	//[ForceSys]
	//NUAM in the basejka code?
	//float		knockback = pull?0:200;
	//[/ForceSys]
	int			powerUse = 0;
    //[SentryTurnoff]
	gentity_t *aimingAt; 
	//[/SentryTurnOff]
	//[GripPush]
	qboolean iGrip=qfalse;
	//[/GripPush]

	visionArc = 360;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && (self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN || !G_InGetUpAnim(&self->client->ps)) && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP)   ) )
	{
		return;
	}

	//[ForceSys]
	/*
	if (!g_useWhileThrowing.integer && self->client->ps.saberInFlight)
	{
		return;
	}
	*/
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK) )
	{
		return;
	}

	//allow push/pull during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || (!(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)))))
	//if (self->client->ps.weaponTime > 0)
	//[ForceSys]
	{
		return;
	}

	if ( self->health <= 0 )
	{
		return;
	}
	//[ForceSys]
	/*
	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
	{//racc - in the process of getting pushed/pulled.
		return;
	}
	*/
	//[/ForceSys]



	
	
	if (pull)
	{
		powerUse = FP_PULL;
	}
	else
	{
		powerUse = FP_PUSH;
	}

	if ( !WP_ForcePowerUsable( self, powerUse ) )
	{
		return;
	}

	if(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
	{
		WP_ForcePowerStop(self,FP_GRIP);
		iGrip=qtrue;
	}

	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame)
	{//RACC - used force push in a saber lock.
		//[SaberLockSys]
		/* can't use push in saberlocks anymore.
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/repulse.wav" ) );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;

		self->client->ps.saberLockHits += self->client->ps.fd.forcePowerLevel[FP_PUSH]*2;

		WP_ForcePowerStart( self, FP_PUSH, 0 );
		*/
		//[/SaberLockSys]
		return;
	}


	if (self->client->repulseTime <= level.time)
	{	
	vec3_t	pt, dirself;
	pt[0] = self->r.currentOrigin[0];
	pt[1] = self->r.currentOrigin[1];
	pt[2] = self->r.currentOrigin[2] + 500;
	VectorSubtract( pt, self->r.currentOrigin, dirself );
	VectorMA(self->client->ps.velocity, 0.8f, dirself, self->client->ps.velocity);	
	self->client->repulseTime = level.time + 600;	
	}
	else if (self->client->repulseTime > level.time)
	{	
	return;
	}	


	
	WP_ForcePowerStart( self, powerUse, 0 );

	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( pull )
	{//RACC - force pull
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/impulse.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
//			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			//[ForceSys]
//			self->client->ps.forceHandExtendTime = level.time + 600;
			/*
			if ( g_gametype.integer == GT_SIEGE && self->client->ps.weapon == WP_SABER )
			{//hold less so can attack right after a pull
				self->client->ps.forceHandExtendTime = level.time + 200;
			}
			else
			{
				self->client->ps.forceHandExtendTime = level.time + 400;
			}
			*/
			//[/ForceSys]
		}
		self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else
	{//RACC - Force Push
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/repulse.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
//			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			//[ForceSys]
//			self->client->ps.forceHandExtendTime = level.time + 650;
			//self->client->ps.forceHandExtendTime = level.time + 1000;
			//[/ForceSys]
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps))
		{//RACC - Force Pushed while in get up animation.
			if (self->client->ps.forceDodgeAnim > 4)
			{
				self->client->ps.forceDodgeAnim -= 8;
			}
			self->client->ps.forceDodgeAnim += 8; //special case, play push on upper torso, but keep playing current knockdown anim on legs
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}


	if (pull)
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PULL];
		pushPower = 512*self->client->ps.fd.forcePowerLevel[FP_PULL];
	}
	else
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PUSH];
		pushPower = 512*self->client->ps.fd.forcePowerLevel[FP_PUSH];
	}

	if (!powerLevel)
	{ //Shouldn't have made it here..
		return;
	}

	if (powerLevel == FORCE_LEVEL_2)
	{
		visionArc = 360;
	}
	else if (powerLevel == FORCE_LEVEL_3)
	{
		visionArc = 360;
	}

	
		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		e = 0;

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (!ent->client && ent->s.eType == ET_NPC)
			{ //g2animent
				if (ent->s.genericenemyindex < level.time)
				{
					ent->s.genericenemyindex = level.time + 2000;
				}
			}
/*			
//[SEEKERSENTRY]
			if(Q_stricmp(ent->classname,"sentryGun") == 0 )//|| ent->s.NPC_class == CLASS_SEEKER)
			{
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0]*radius/2;
		tto[1] = tfrom[1] + fwd[1]*radius/2;
		tto[2] = tfrom[2] + fwd[2]*radius/2;

		//[CoOp]
		//moved up a bit for the sake of the brush based force stuff.
		//numListedEntities = 0;
		//[/CoOp]

		trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
			{ //g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}

			//[CoOp]
			//moved up a bit for the sake of the brush based force stuff.
			//numListedEntities = 0;
			//[/CoOp]
			aimingAt = &g_entities[tr.entityNum];
			if(ent == aimingAt)//Looking at the sentry...so deactivate for 10 seconds
				ent->nextthink = level.time + 5000;
		}
			}
			//[/SEEKERSENTRY]
*/
			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !trap_InPVS(self->client->ps.origin, ent->client->ps.origin) &&
					ForcePowerUsableOn(self, ent, powerUse))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (pull)
					{
						if (!ForcePowerUsableOn(self, ent, FP_PULL))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
					else
					{
						if (!ForcePowerUsableOn(self, ent, FP_PUSH))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
				}
			}
			e++;
		}
	

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		if (entityList[e] != ENTITYNUM_NONE &&
			entityList[e] >= 0 &&
			entityList[e] < MAX_GENTITIES)
		{
			ent = &g_entities[entityList[e]];
		}
		else
		{
			ent = NULL;
		}

		if (!ent)
			continue;
		if (ent == self)
			continue;
		if (ent->client && OnSameTeam(ent, self))
		{
			continue;
		}
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( ent->s.eType != ET_ITEM )
			{
				//FIXME: need pushable objects
				if ( Q_stricmp( "func_button", ent->classname ) == 0 )
				{//we might push it
					if ( pull || !(ent->spawnflags&SPF_BUTTON_FPUSHABLE) )
					{//not force-pushable, never pullable
						continue;
					}
				}
				else
				{
					if ( ent->s.eFlags & EF_NODRAW )
					{
						continue;
					}
					if ( !ent->client )
					{
						if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
						{//not a lightsaber 
							if ( Q_stricmp( "func_door", ent->classname ) != 0 || !(ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
							{//not a force-usable door
								if ( Q_stricmp( "func_static", ent->classname ) != 0 || (!(ent->spawnflags&1/*F_PUSH*/)&&!(ent->spawnflags&2/*F_PULL*/)) )
								{//not a force-usable func_static
									if ( Q_stricmp( "limb", ent->classname ) )
									{//not a limb
										continue;
									}
								}
							}
							else if ( ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2 )
							{//not at rest
								continue;
							}
						}
					}
					else if ( ent->client->NPC_class == CLASS_GALAKMECH 
						|| ent->client->NPC_class == CLASS_ATST
						|| ent->client->NPC_class == CLASS_RANCOR )
					{//can't push ATST or Galak or Rancor
						continue;
					}
				}
			}
		}
		else
		{//RACC - Missiles
			if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
			{//can't force-push/pull stuck missiles (detpacks, tripmines)
				continue;
			}
			if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
			{//only thermal detonators can be pushed once stopped
				continue;
			}
		}

		//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( center[i] < ent->r.absmin[i] ) 
			{
				v[i] = ent->r.absmin[i] - center[i];
			} else if ( center[i] > ent->r.absmax[i] ) 
			{
				v[i] = center[i] - ent->r.absmax[i];
			} else 
			{
				v[i] = 0;
			}
		}

		VectorSubtract( ent->r.absmax, ent->r.absmin, size );
		VectorMA( ent->r.absmin, 0.5, size, ent_org );

		VectorSubtract( ent_org, center, dir );
		VectorNormalize( dir );


		dist = VectorLength( v );

		//Now check and see if we can actually deflect it
		//method1
		//if within a certain range, deflect it
		if ( dist >= radius ) 
		{
			continue;
		}
	


		// ok, we are within the radius, add us to the incoming list
		push_list[ent_count] = ent;
		ent_count++;
	}

	if ( ent_count )
	{
		//method1:
		for ( x = 0; x < ent_count; x++ )
		{
			int modPowerLevel = powerLevel;

			//[ForceSys]

			if (push_list[x]->client)
			{
				modPowerLevel = WP_AbsorbConversion(push_list[x], push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB], self, powerUse, powerLevel, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[powerUse]][powerUse]);
				if (modPowerLevel == -1)
				{
					modPowerLevel = powerLevel;
				}

			}
			
			//[/ForceSys]

			pushPower = 512*modPowerLevel;

			if (push_list[x]->client)
			{
				VectorCopy(push_list[x]->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(push_list[x]->s.origin, thispush_org);
			}

			if ( push_list[x]->client)
			{//FIXME: make enemy jedi able to hunker down and resist this?
				//[ForceSys]
				qboolean wasWallGrabbing = qfalse;
				//int otherPushPower = push_list[x]->client->ps.fd.forcePowerLevel[powerUse];
				//[/ForceSys]
				qboolean canPullWeapon = qtrue;
				float dirLen = 0;

				if ( g_debugMelee.integer )
				{
					if ( (push_list[x]->client->ps.pm_flags&PMF_STUCK_TO_WALL) )
					{//no resistance if stuck to wall
						//push/pull them off the wall
						//[ForceSys]
						wasWallGrabbing = qtrue;
						//otherPushPower = 0;
						//[/ForceSys]
						G_LetGoOfWall( push_list[x] );
					}
				}

				//[ForceSys]
				//knockback = pull?0:200;
				//[/ForceSys]

				pushPowerMod = pushPower;

				//[ForceSys]
				/* racc - let the counter throw function handle this
				if (push_list[x]->client->pers.cmd.forwardmove ||
					push_list[x]->client->pers.cmd.rightmove)
				{ //if you are moving, you get one less level of defense
					otherPushPower--;

					if (otherPushPower < 0)
					{
						otherPushPower = 0;
					}
				}
				*/
				int forceNeeded;
				if (pull)
				{
					forceNeeded = forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PULL]][FP_PULL];
				}
				else
				{
					forceNeeded = forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH];
				}				
				//switched to more logical wasWallGrabbing toggle.
				if (!wasWallGrabbing && CanCounterThrow(push_list[x], self, forceNeeded, pull, qtrue)
					&& push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
				//if (otherPushPower && CanCounterThrow(push_list[x], self, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH], pull, qtrue))
				//[/ForceSys]
				{//racc - player blocked the throw.
					if ( pull )
					{
						G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/impulse.wav" ) );
						//push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
						//[ForceSys]
						push_list[x]->client->ps.forceHandExtendTime = level.time + 600;
						//push_list[x]->client->ps.forceHandExtendTime = level.time + 400;
						//[/ForceSys]
					}
					else
					{
						G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/repulse.wav" ) );
						//push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
						//[ForceSys]
						push_list[x]->client->ps.forceHandExtendTime = level.time + 650;
						//push_list[x]->client->ps.forceHandExtendTime = level.time + 1000;
						//[/ForceSys]
					}
					//racc - add the force push glow to the defender
					push_list[x]->client->ps.powerups[PW_DISINT_4] = push_list[x]->client->ps.forceHandExtendTime + 200;

					if (pull)
					{
						push_list[x]->client->ps.powerups[PW_PULL] = push_list[x]->client->ps.powerups[PW_DISINT_4];
					}
					else
					{
						push_list[x]->client->ps.powerups[PW_PULL] = 0;
					}

					//Make a counter-throw effect

					//[ForceSys]
					//racc - countering a throw now automatically prevents push back.
					continue;

					/* basejka method
					if (otherPushPower >= modPowerLevel)
					{
						pushPowerMod = 0;
						canPullWeapon = qfalse;
					}
					else
					{
						int powerDif = (modPowerLevel - otherPushPower);

						if (powerDif >= 3)
						{
							pushPowerMod -= pushPowerMod*0.2;
						}
						else if (powerDif == 2)
						{
							pushPowerMod -= pushPowerMod*0.4;
						}
						else if (powerDif == 1)
						{
							pushPowerMod -= pushPowerMod*0.8;
						}

						if (pushPowerMod < 0)
						{
							pushPowerMod = 0;
						}
					}
					*/
					//[/ForceSys]
				}

				//shove them
				if ( pull )
				{
					VectorSubtract( self->client->ps.origin, thispush_org, pushDir );

					if (push_list[x]->client && VectorLength(pushDir) <= 256)
					{
						//[ForceSys]

						/* don't randomize the weapon pull thingy
						int randfact = 0;

						if (modPowerLevel == FORCE_LEVEL_1)
						{
							randfact = 3;
						}
						else if (modPowerLevel == FORCE_LEVEL_2)
						{
							randfact = 7;
						}
						else if (modPowerLevel == FORCE_LEVEL_3)
						{
							randfact = 10;
						}
						*/
						if(!OnSameTeam(self, push_list[x]) && push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
						{
							canPullWeapon = qfalse;
						}
						if (!OnSameTeam(self, push_list[x]) && canPullWeapon)
						//if (!OnSameTeam(self, push_list[x]) && Q_irand(1, 10) <= randfact && canPullWeapon)
						//[/ForceSys]
						{//racc - pull the weapon out of the player's hand.
							vec3_t uorg, vecnorm;
							//[ForceSys]
							VectorCopy(self->client->ps.origin, tfrom);
							tfrom[2] += self->client->ps.viewheight;
							AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
							tto[0] = tfrom[0] + fwd[0]*radius/2;
							tto[1] = tfrom[1] + fwd[1]*radius/2;
							tto[2] = tfrom[2] + fwd[2]*radius/2;

							trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

							if (tr.fraction != 1.0 
								&& tr.entityNum == push_list[x]->s.number)
							{
								VectorCopy(self->client->ps.origin, uorg);
								uorg[2] += 64;

								VectorSubtract(uorg, thispush_org, vecnorm);
								VectorNormalize(vecnorm);

								TossClientWeapon(push_list[x], vecnorm, 500);
							}

							/* basejka code
							VectorCopy(self->client->ps.origin, uorg);
							uorg[2] += 64;

							VectorSubtract(uorg, thispush_org, vecnorm);
							VectorNormalize(vecnorm);

							TossClientWeapon(push_list[x], vecnorm, 500);
							*/
							//[/ForceSys]
						}
					}
				}
				else
				{
					VectorSubtract( thispush_org, self->client->ps.origin, pushDir );
				}

				//[ForceSys]
				/* racc - converted this stuff to be push strength based
				if ((modPowerLevel > otherPushPower || push_list[x]->client->ps.m_iVehicleNum) && push_list[x]->client)
				{

					if (modPowerLevel == FORCE_LEVEL_3 &&
						push_list[x]->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{
						dirLen = VectorLength(pushDir);

						if (BG_KnockDownable(&push_list[x]->client->ps) &&
							dirLen <= (64*((modPowerLevel - otherPushPower)-1)))
						{ //can only do a knockdown if fairly close
							push_list[x]->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							push_list[x]->client->ps.forceHandExtendTime = level.time + 700;
							push_list[x]->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
							push_list[x]->client->ps.quickerGetup = qtrue;
						}
						else if (push_list[x]->s.number < MAX_CLIENTS && push_list[x]->client->ps.m_iVehicleNum &&
							dirLen <= 128.0f )
						{ //a player on a vehicle
							gentity_t *vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];
							if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{ //push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)push_list[x], qfalse);
								}
							}
						}
					}
				}
				*/
				//[/ForceSys]

				if (!dirLen)
				{
					dirLen = VectorLength(pushDir);
				}

				VectorNormalize(pushDir);

				if (push_list[x]->client)
				{
					//escape a force grip if we're in one
					if (self->client->ps.fd.forceGripBeingGripped > level.time)
					{ //force the enemy to stop gripping me if I managed to push him
						if (push_list[x]->client->ps.fd.forceGripEntityNum == self->s.number)
						{
							//[ForceSys]
							//always allow a push to disable a grip move.
							//if (modPowerLevel >= push_list[x]->client->ps.fd.forcePowerLevel[FP_GRIP])
							//[/ForceSys]
							{ //only break the grip if our push/pull level is >= their grip level
								WP_ForcePowerStop(push_list[x], FP_GRIP);
								self->client->ps.fd.forceGripBeingGripped = 0;
								push_list[x]->client->ps.fd.forceGripUseTime = level.time + 1000; //since we just broke out of it..
							}
						}
					}

					push_list[x]->client->ps.otherKiller = self->s.number;
					push_list[x]->client->ps.otherKillerTime = level.time + 5000;
					push_list[x]->client->ps.otherKillerDebounceTime = level.time + 100;
					//[Asteroids]
					push_list[x]->client->otherKillerMOD = MOD_UNKNOWN;
					push_list[x]->client->otherKillerVehWeapon = 0;
					push_list[x]->client->otherKillerWeaponType = WP_NONE;
					//[/Asteroids]

					pushPowerMod -= (dirLen*0.7);
					if (pushPowerMod < 16)
					{
						pushPowerMod = 16;
					}

					//[ForceSys]
					if (pushPowerMod > 250)
					{//got pushed hard, get knockdowned or knocked off a animals or speeders if riding one.
						if (push_list[x]->client->ps.m_iVehicleNum)
						{ //a player on a vehicle
							gentity_t *vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];
							if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{ //push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)push_list[x], qfalse);
								}
							}
						}
/*
                       if((push_list[x]->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY)
					   || BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   || PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   || push_list[x]->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
						{
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						*/
					//[/ForceSys]
					}
					//fullbody push effect
					push_list[x]->client->pushEffectTime = level.time + 600;

					if(!pull)
					{
					if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL)
						|| (BG_InRoll(&push_list[x]->client->ps,push_list[x]->client->ps.legsAnim )
						&& !pull))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && (BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && WalkCheck(push_list[x]))
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && (push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL
					   && InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)))
						&& !pull)
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;//Push

						if(!BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && !WalkCheck(push_list[x]) && (push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0))
						{//Using a light weapon,Running,Don't have absorb
							if(!InFront(push_list[x]->r.currentOrigin,self->r.currentOrigin,self->client->ps.viewangles,0.3f))
								G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						else
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
					}
					}
					else if(pull)
					{
						if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL)
						|| BG_InRoll(&push_list[x]->client->ps,push_list[x]->client->ps.legsAnim))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else if(((WalkCheck(push_list[x]) && BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
						|| (!WalkCheck(push_list[x]) && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps))))
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && (push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL
					   && InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
						if(!BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && !WalkCheck(push_list[x]) && (push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0))
						{//Using a light weapon,Running,Don't have absorb
							if(!InFront(push_list[x]->r.currentOrigin,self->r.currentOrigin,self->client->ps.viewangles,0.3f))
								G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						else
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
					}
					}

					if(iGrip)
						pushPowerMod *=2;

					push_list[x]->client->ps.velocity[0] = pushDir[0]*pushPowerMod;
					push_list[x]->client->ps.velocity[1] = pushDir[1]*pushPowerMod;

					if ((int)push_list[x]->client->ps.velocity[2] == 0)
					{ //if not going anywhere vertically, boost them up a bit
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod +200;

						if (push_list[x]->client->ps.velocity[2] < 128)
						{
							push_list[x]->client->ps.velocity[2] = 128;
						}
					}
					else
					{
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod;
					}
					
					
					int dmg = 50;

					if (self->client->ps.fd.forcePowerLevel[FP_PUSH] == FORCE_LEVEL_2 || self->client->ps.fd.forcePowerLevel[FP_PULL] == FORCE_LEVEL_2)
					{
						dmg == 100;
					}	
					else if (self->client->ps.fd.forcePowerLevel[FP_PUSH] == FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] == FORCE_LEVEL_3)
					{
						dmg == 150;
					}
					G_Damage(push_list[x], self, self, NULL, NULL, dmg, 0, MOD_FORCE_DARK);
					
					
				}
			}
			else if ( push_list[x]->s.eType == ET_MISSILE && push_list[x]->s.pos.trType != TR_STATIONARY && (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				if ( pull )
				{//deflect rather than reflect?
				}
				else 
				{
					//[ForceSys]
					G_DeflectMissile( self, push_list[x], forward );
					//G_ReflectMissile( self, push_list[x], forward );
					//[/ForceSys]
				}
			}
			else if ( !Q_stricmp( "func_static", push_list[x]->classname ) )
			{//force-usable func_static
				if ( !pull && (push_list[x]->spawnflags&1/*F_PUSH*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
				else if ( pull && (push_list[x]->spawnflags&2/*F_PULL*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
			}
			else if ( !Q_stricmp( "func_door", push_list[x]->classname ) && (push_list[x]->spawnflags&2) )
			{//push/pull the door
				vec3_t	pos1, pos2;
				vec3_t	trFrom;

				VectorCopy(self->client->ps.origin, trFrom);
				trFrom[2] += self->client->ps.viewheight;

				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( trFrom, radius, forward, end );
				trap_Trace( &tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}

				if ( VectorCompare( vec3_origin, push_list[x]->s.origin ) )
				{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract( push_list[x]->r.absmax, push_list[x]->r.absmin, size );
					VectorMA( push_list[x]->r.absmin, 0.5, size, center );
					if ( (push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS1 )
					{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos1, center );
					}
					else if ( !(push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS2 )
					{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos2, center );
					}
					VectorAdd( center, push_list[x]->pos1, pos1 );
					VectorAdd( center, push_list[x]->pos2, pos2 );
				}
				else
				{//actually has an origin, pos1 and pos2 are absolute
					VectorCopy( push_list[x]->r.currentOrigin, center );
					VectorCopy( push_list[x]->pos1, pos1 );
					VectorCopy( push_list[x]->pos2, pos2 );
				}

				if ( Distance( pos1, trFrom ) < Distance( pos2, trFrom ) )
				{//pos1 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{//pos2 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GEntity_UseFunc( push_list[x], self, self );
			}
			else if ( Q_stricmp( "func_button", push_list[x]->classname ) == 0 )
			{//pretend you pushed it
				Touch_Button( push_list[x], self, NULL );
				continue;
			}
		}
	}

	//attempt to break any leftover grips
	//if we're still in a current grip that wasn't broken by the push, it will still remain
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}
}
void ForceThrow( gentity_t *self, qboolean pull )
{
	//shove things in front of you away
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius = 512; //since it's view-based now. //350;
	int			powerLevel;
	int			visionArc;
	int			pushPower;
	int			pushPowerMod;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	float		dot1;
	trace_t		tr;
	int			x;
	vec3_t		pushDir;
	vec3_t		thispush_org;
	vec3_t		tfrom, tto, fwd, a;
	//[ForceSys]
	//NUAM in the basejka code?
	//float		knockback = pull?0:200;
	//[/ForceSys]
	int			powerUse = 0;
    //[SentryTurnoff]
	gentity_t *aimingAt; 
	//[/SentryTurnOff]
	//[GripPush]
	qboolean iGrip=qfalse;
	//[/GripPush]

	visionArc = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && (self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN || !G_InGetUpAnim(&self->client->ps)) && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP)   ) )
	{
		return;
	}

	//[ForceSys]
	/*
	if (!g_useWhileThrowing.integer && self->client->ps.saberInFlight)
	{
		return;
	}
	*/
	if(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK) )
	{
		return;
	}

	//allow push/pull during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || (!(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK)))))
	//if (self->client->ps.weaponTime > 0)
	//[ForceSys]
	{
		return;
	}

	if ( self->health <= 0 )
	{
		return;
	}
	//[ForceSys]
	/*
	if ( self->client->ps.powerups[PW_DISINT_4] > level.time )
	{//racc - in the process of getting pushed/pulled.
		return;
	}
	*/
	//[/ForceSys]
	if (pull)
	{
		powerUse = FP_PULL;
	}
	else
	{
		powerUse = FP_PUSH;
	}

	if ( !WP_ForcePowerUsable( self, powerUse ) )
	{
		return;
	}

	if(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
	{
		WP_ForcePowerStop(self,FP_GRIP);
		iGrip=qtrue;
	}

	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame)
	{//RACC - used force push in a saber lock.
		//[SaberLockSys]
		/* can't use push in saberlocks anymore.
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;

		self->client->ps.saberLockHits += self->client->ps.fd.forcePowerLevel[FP_PUSH]*2;

		WP_ForcePowerStart( self, FP_PUSH, 0 );
		*/
		//[/SaberLockSys]
		return;
	}
	
	WP_ForcePowerStart( self, powerUse, 0 );

	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( pull )
	{//RACC - force pull
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			//[ForceSys]
			self->client->ps.forceHandExtendTime = level.time + 600;
			/*
			if ( g_gametype.integer == GT_SIEGE && self->client->ps.weapon == WP_SABER )
			{//hold less so can attack right after a pull
				self->client->ps.forceHandExtendTime = level.time + 200;
			}
			else
			{
				self->client->ps.forceHandExtendTime = level.time + 400;
			}
			*/
			//[/ForceSys]
		}
		self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else
	{//RACC - Force Push
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			//[ForceSys]
			self->client->ps.forceHandExtendTime = level.time + 650;
			//self->client->ps.forceHandExtendTime = level.time + 1000;
			//[/ForceSys]
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps))
		{//RACC - Force Pushed while in get up animation.
			if (self->client->ps.forceDodgeAnim > 4)
			{
				self->client->ps.forceDodgeAnim -= 8;
			}
			self->client->ps.forceDodgeAnim += 8; //special case, play push on upper torso, but keep playing current knockdown anim on legs
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->client->ps.origin, center );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}


	if (pull)
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PULL];
		pushPower = 256*self->client->ps.fd.forcePowerLevel[FP_PULL];
	}
	else
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PUSH];
		pushPower = 256*self->client->ps.fd.forcePowerLevel[FP_PUSH];
	}

	if (!powerLevel)
	{ //Shouldn't have made it here..
		return;
	}

	if (powerLevel == FORCE_LEVEL_2)
	{
		visionArc = 60;
	}
	else if (powerLevel == FORCE_LEVEL_3)
	{
		visionArc = 180;
	}


	if (powerLevel == FORCE_LEVEL_1)
	{ //can only push/pull targeted things at level 1
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0]*radius/2;
		tto[1] = tfrom[1] + fwd[1]*radius/2;
		tto[2] = tfrom[2] + fwd[2]*radius/2;
		/*
		//[CoOp]
		//moved up a bit for the sake of the brush based force stuff.
		numListedEntities = 0;
		//[/CoOp]
		*/
		trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
			{ //g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}

			//[CoOp]
			//moved up a bit for the sake of the brush based force stuff.
			numListedEntities = 0;
			//[/CoOp]
			entityList[numListedEntities] = tr.entityNum;

	if (pull)
			{
			
																												
									  
																	 
	 
						 
	 
	  
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PULL))
				{
					return;
				}
	  
			 
			}
			else
			{
		  
																		 
									
																	 
	 
						 
	 
	  
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PUSH))
				{
					return;
				}
	  
	
						 
   

													 
   
		 
			   
							   
							 

																	   

							 
		   

							   
	
									   
															 
												   
													   
						 
	 
			}
			numListedEntities++;
		}

	
		else
		{
			//didn't get anything, so just
			return;
		}
	
		   
	}
	else
	{
		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		e = 0;

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (!ent->client && ent->s.eType == ET_NPC)
			{ //g2animent
				if (ent->s.genericenemyindex < level.time)
				{
					ent->s.genericenemyindex = level.time + 2000;
				}
			}
/*			
//[SEEKERSENTRY]
			if(Q_stricmp(ent->classname,"sentryGun") == 0 )//|| ent->s.NPC_class == CLASS_SEEKER)
			{
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0]*radius/2;
		tto[1] = tfrom[1] + fwd[1]*radius/2;
		tto[2] = tfrom[2] + fwd[2]*radius/2;

		//[CoOp]
		//moved up a bit for the sake of the brush based force stuff.
		//numListedEntities = 0;
		//[/CoOp]

		trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client && g_entities[tr.entityNum].s.eType == ET_NPC)
			{ //g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}

			//[CoOp]
			//moved up a bit for the sake of the brush based force stuff.
			//numListedEntities = 0;
			//[/CoOp]
			aimingAt = &g_entities[tr.entityNum];
			if(ent == aimingAt)//Looking at the sentry...so deactivate for 10 seconds
				ent->nextthink = level.time + 5000;
		}
			}
			//[/SEEKERSENTRY]
*/
			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) &&
					ForcePowerUsableOn(self, ent, powerUse))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (pull)
					{
						if (!ForcePowerUsableOn(self, ent, FP_PULL))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
					else
					{
						if (!ForcePowerUsableOn(self, ent, FP_PUSH))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
				}
			}
			e++;
		}
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		if (entityList[e] != ENTITYNUM_NONE &&
			entityList[e] >= 0 &&
			entityList[e] < MAX_GENTITIES)
		{
			ent = &g_entities[entityList[e]];
		}
		else
		{
			ent = NULL;
		}

		if (!ent)
			continue;
		if (ent == self)
			continue;
		if (ent->client && OnSameTeam(ent, self))
		{
			continue;
		}
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( ent->s.eType != ET_ITEM )
			{
				//FIXME: need pushable objects
				if ( Q_stricmp( "func_button", ent->classname ) == 0 )
				{//we might push it
					if ( pull || !(ent->spawnflags&SPF_BUTTON_FPUSHABLE) )
					{//not force-pushable, never pullable
						continue;
					}
				}
				else
				{
					if ( ent->s.eFlags & EF_NODRAW )
					{
						continue;
					}
					if ( !ent->client )
					{
						if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
						{//not a lightsaber 
							if ( Q_stricmp( "func_door", ent->classname ) != 0 || !(ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
							{//not a force-usable door
								if ( Q_stricmp( "func_static", ent->classname ) != 0 || (!(ent->spawnflags&1/*F_PUSH*/)&&!(ent->spawnflags&2/*F_PULL*/)) )
								{//not a force-usable func_static
									if ( Q_stricmp( "limb", ent->classname ) )
									{//not a limb
										continue;
									}
								}
							}
							else if ( ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2 )
							{//not at rest
								continue;
							}
						}
					}
					else if ( ent->client->NPC_class == CLASS_GALAKMECH 
						|| ent->client->NPC_class == CLASS_ATST
						|| ent->client->NPC_class == CLASS_RANCOR )
					{//can't push ATST or Galak or Rancor
						continue;
					}
				}
			}
		}
		else
		{//RACC - Missiles
			if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
			{//can't force-push/pull stuck missiles (detpacks, tripmines)
				continue;
			}
			if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
			{//only thermal detonators can be pushed once stopped
				continue;
			}
		}

		//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( center[i] < ent->r.absmin[i] ) 
			{
				v[i] = ent->r.absmin[i] - center[i];
			} else if ( center[i] > ent->r.absmax[i] ) 
			{
				v[i] = center[i] - ent->r.absmax[i];
			} else 
			{
				v[i] = 0;
			}
		}

		VectorSubtract( ent->r.absmax, ent->r.absmin, size );
		VectorMA( ent->r.absmin, 0.5, size, ent_org );

		VectorSubtract( ent_org, center, dir );
		VectorNormalize( dir );
		if ( (dot1 = DotProduct( dir, forward )) < 0.6 )
			continue;

		dist = VectorLength( v );

		//Now check and see if we can actually deflect it
		//method1
		//if within a certain range, deflect it
		if ( dist >= radius ) 
		{
			continue;
		}
	
		//in PVS?
		if ( !ent->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
		{//must be in PVS
			continue;
		}

		//really should have a clear LOS to this thing...
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
		if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
		{//must have clear LOS
			//try from eyes too before you give up
			vec3_t eyePoint;
			VectorCopy(self->client->ps.origin, eyePoint);
			eyePoint[2] += self->client->ps.viewheight;
			trap_Trace( &tr, eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );

			if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
			{
				continue;
			}
		}

		// ok, we are within the radius, add us to the incoming list
		push_list[ent_count] = ent;
		ent_count++;
	}

	if ( ent_count )
	{
		//method1:
		for ( x = 0; x < ent_count; x++ )
		{
			int modPowerLevel = powerLevel;

			//[ForceSys]

			if (push_list[x]->client)
			{
				modPowerLevel = WP_AbsorbConversion(push_list[x], push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB], self, powerUse, powerLevel, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[powerUse]][powerUse]);
				if (modPowerLevel == -1)
				{
					modPowerLevel = powerLevel;
				}

			}
			
			//[/ForceSys]

			pushPower = 256*modPowerLevel;

			if (push_list[x]->client)
			{
				VectorCopy(push_list[x]->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(push_list[x]->s.origin, thispush_org);
			}

			if ( push_list[x]->client)
			{//FIXME: make enemy jedi able to hunker down and resist this?
				//[ForceSys]
				qboolean wasWallGrabbing = qfalse;
				//int otherPushPower = push_list[x]->client->ps.fd.forcePowerLevel[powerUse];
				//[/ForceSys]
				qboolean canPullWeapon = qtrue;
				float dirLen = 0;

				if ( g_debugMelee.integer )
				{
					if ( (push_list[x]->client->ps.pm_flags&PMF_STUCK_TO_WALL) )
					{//no resistance if stuck to wall
						//push/pull them off the wall
						//[ForceSys]
						wasWallGrabbing = qtrue;
						//otherPushPower = 0;
						//[/ForceSys]
						G_LetGoOfWall( push_list[x] );
					}
				}

				//[ForceSys]
				//knockback = pull?0:200;
				//[/ForceSys]

				pushPowerMod = pushPower;

				//[ForceSys]
				/* racc - let the counter throw function handle this
				if (push_list[x]->client->pers.cmd.forwardmove ||
					push_list[x]->client->pers.cmd.rightmove)
				{ //if you are moving, you get one less level of defense
					otherPushPower--;

					if (otherPushPower < 0)
					{
						otherPushPower = 0;
					}
				}
				*/

				//switched to more logical wasWallGrabbing toggle.
				int forceNeeded;
				if (pull)
				{
					forceNeeded = forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PULL]][FP_PULL];
				}
				else
				{
					forceNeeded = forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH];
				}				
				//switched to more logical wasWallGrabbing toggle.
				if (!wasWallGrabbing && CanCounterThrow(push_list[x], self, forceNeeded, pull, qfalse)
					&& push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
				//if (otherPushPower && CanCounterThrow(push_list[x], self, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH], pull, qtrue))
				//[/ForceSys]
				{//racc - player blocked the throw.
					if ( pull )
					{
						G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
						//push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
						//[ForceSys]
						push_list[x]->client->ps.forceHandExtendTime = level.time + 600;
						//push_list[x]->client->ps.forceHandExtendTime = level.time + 400;
						//[/ForceSys]
					}
					else
					{
						G_Sound( push_list[x], CHAN_BODY, G_SoundIndex( "sound/weapons/force/push.wav" ) );
						//push_list[x]->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
						//[ForceSys]
						push_list[x]->client->ps.forceHandExtendTime = level.time + 650;
						//push_list[x]->client->ps.forceHandExtendTime = level.time + 1000;
						//[/ForceSys]
					}
					//racc - add the force push glow to the defender
					push_list[x]->client->ps.powerups[PW_DISINT_4] = push_list[x]->client->ps.forceHandExtendTime + 200;

					if (pull)
					{
						push_list[x]->client->ps.powerups[PW_PULL] = push_list[x]->client->ps.powerups[PW_DISINT_4];
					}
					else
					{
						push_list[x]->client->ps.powerups[PW_PULL] = 0;
					}

					//Make a counter-throw effect

					//[ForceSys]
					//racc - countering a throw now automatically prevents push back.
					continue;

					/* basejka method
					if (otherPushPower >= modPowerLevel)
					{
						pushPowerMod = 0;
						canPullWeapon = qfalse;
					}
					else
					{
						int powerDif = (modPowerLevel - otherPushPower);

						if (powerDif >= 3)
						{
							pushPowerMod -= pushPowerMod*0.2;
						}
						else if (powerDif == 2)
						{
							pushPowerMod -= pushPowerMod*0.4;
						}
						else if (powerDif == 1)
						{
							pushPowerMod -= pushPowerMod*0.8;
						}

						if (pushPowerMod < 0)
						{
							pushPowerMod = 0;
						}
					}
					*/
					//[/ForceSys]
				}

				//shove them
				if ( pull )
				{
					VectorSubtract( self->client->ps.origin, thispush_org, pushDir );

					if (push_list[x]->client && VectorLength(pushDir) <= 256)
					{
						//[ForceSys]

						/* don't randomize the weapon pull thingy
						int randfact = 0;

						if (modPowerLevel == FORCE_LEVEL_1)
						{
							randfact = 3;
						}
						else if (modPowerLevel == FORCE_LEVEL_2)
						{
							randfact = 7;
						}
						else if (modPowerLevel == FORCE_LEVEL_3)
						{
							randfact = 10;
						}
						*/
						if(!OnSameTeam(self, push_list[x]) && push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
						{
							canPullWeapon = qfalse;
						}
						if (!OnSameTeam(self, push_list[x]) && canPullWeapon)
						//if (!OnSameTeam(self, push_list[x]) && Q_irand(1, 10) <= randfact && canPullWeapon)
						//[/ForceSys]
						{//racc - pull the weapon out of the player's hand.
							vec3_t uorg, vecnorm;
							//[ForceSys]
							VectorCopy(self->client->ps.origin, tfrom);
							tfrom[2] += self->client->ps.viewheight;
							AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
							tto[0] = tfrom[0] + fwd[0]*radius/2;
							tto[1] = tfrom[1] + fwd[1]*radius/2;
							tto[2] = tfrom[2] + fwd[2]*radius/2;

							trap_Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID);

							if (tr.fraction != 1.0 
								&& tr.entityNum == push_list[x]->s.number)
							{
								VectorCopy(self->client->ps.origin, uorg);
								uorg[2] += 64;

								VectorSubtract(uorg, thispush_org, vecnorm);
								VectorNormalize(vecnorm);

								TossClientWeapon(push_list[x], vecnorm, 500);
							}

							/* basejka code
							VectorCopy(self->client->ps.origin, uorg);
							uorg[2] += 64;

							VectorSubtract(uorg, thispush_org, vecnorm);
							VectorNormalize(vecnorm);

							TossClientWeapon(push_list[x], vecnorm, 500);
							*/
							//[/ForceSys]
						}
					}
				}
				else
				{
					VectorSubtract( thispush_org, self->client->ps.origin, pushDir );
				}

				//[ForceSys]
				/* racc - converted this stuff to be push strength based
				if ((modPowerLevel > otherPushPower || push_list[x]->client->ps.m_iVehicleNum) && push_list[x]->client)
				{

					if (modPowerLevel == FORCE_LEVEL_3 &&
						push_list[x]->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{
						dirLen = VectorLength(pushDir);

						if (BG_KnockDownable(&push_list[x]->client->ps) &&
							dirLen <= (64*((modPowerLevel - otherPushPower)-1)))
						{ //can only do a knockdown if fairly close
							push_list[x]->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							push_list[x]->client->ps.forceHandExtendTime = level.time + 700;
							push_list[x]->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
							push_list[x]->client->ps.quickerGetup = qtrue;
						}
						else if (push_list[x]->s.number < MAX_CLIENTS && push_list[x]->client->ps.m_iVehicleNum &&
							dirLen <= 128.0f )
						{ //a player on a vehicle
							gentity_t *vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];
							if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{ //push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)push_list[x], qfalse);
								}
							}
						}
					}
				}
				*/
				//[/ForceSys]

				if (!dirLen)
				{
					dirLen = VectorLength(pushDir);
				}

				VectorNormalize(pushDir);

				if (push_list[x]->client)
				{
					//escape a force grip if we're in one
					if (self->client->ps.fd.forceGripBeingGripped > level.time)
					{ //force the enemy to stop gripping me if I managed to push him
						if (push_list[x]->client->ps.fd.forceGripEntityNum == self->s.number)
						{
							//[ForceSys]
							//always allow a push to disable a grip move.
							//if (modPowerLevel >= push_list[x]->client->ps.fd.forcePowerLevel[FP_GRIP])
							//[/ForceSys]
							{ //only break the grip if our push/pull level is >= their grip level
								WP_ForcePowerStop(push_list[x], FP_GRIP);
								self->client->ps.fd.forceGripBeingGripped = 0;
								push_list[x]->client->ps.fd.forceGripUseTime = level.time + 1000; //since we just broke out of it..
							}
						}
					}

					push_list[x]->client->ps.otherKiller = self->s.number;
					push_list[x]->client->ps.otherKillerTime = level.time + 5000;
					push_list[x]->client->ps.otherKillerDebounceTime = level.time + 100;
					//[Asteroids]
					push_list[x]->client->otherKillerMOD = MOD_UNKNOWN;
					push_list[x]->client->otherKillerVehWeapon = 0;
					push_list[x]->client->otherKillerWeaponType = WP_NONE;
					//[/Asteroids]

					pushPowerMod -= (dirLen*0.7);
					if (pushPowerMod < 16)
					{
						pushPowerMod = 16;
					}

					//[ForceSys]
					if (pushPowerMod > 250)
					{//got pushed hard, get knockdowned or knocked off a animals or speeders if riding one.
						if (push_list[x]->client->ps.m_iVehicleNum)
						{ //a player on a vehicle
							gentity_t *vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];
							if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{ //push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t *)push_list[x], qfalse);
								}
							}
						}
/*
                       if((push_list[x]->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY)
					   || BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   || PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   || push_list[x]->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
						{
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						*/
					//[/ForceSys]
					}
					//fullbody push effect
					push_list[x]->client->pushEffectTime = level.time + 600;

					if(!pull)
					{
					if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL)
						|| (BG_InRoll(&push_list[x]->client->ps,push_list[x]->client->ps.legsAnim )
						&& !pull))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod /= 2;
					}
					else if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && (BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && WalkCheck(push_list[x]))
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && (push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL
					   && InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)))
						&& !pull)
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PUSH] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;//Push

						if(!BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && !WalkCheck(push_list[x]) && (push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0))
						{//Using a light weapon,Running,Don't have absorb
							if(!InFront(push_list[x]->r.currentOrigin,self->r.currentOrigin,self->client->ps.viewangles,0.3f))
								G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						else
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
					}
					}
					else if(pull)
					{
						if((WalkCheck(push_list[x])
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL)
						|| BG_InRoll(&push_list[x]->client->ps,push_list[x]->client->ps.legsAnim))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod /= 2;
					}
					else if(((WalkCheck(push_list[x]) && BG_IsUsingHeavyWeap(&push_list[x]->client->ps)
						|| (!WalkCheck(push_list[x]) && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps))))
						&& (push_list[x]->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_HEAVY)
					   && !PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove)
					   && (push_list[x]->client->ps.stats[STAT_DODGE] > DODGE_CRITICALLEVEL
					   && InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)))
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
					}
					else
					{
						if((push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0)
							|| push_list[x]->client->ps.fd.forcePowerLevel[FP_PULL] < self->client->ps.fd.forcePowerLevel[FP_PUSH])
						pushPowerMod *= 1;
						if(!BG_IsUsingHeavyWeap(&push_list[x]->client->ps) && !WalkCheck(push_list[x]) && (push_list[x]->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_0 && push_list[x]->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_0))
						{//Using a light weapon,Running,Don't have absorb
							if(!InFront(push_list[x]->r.currentOrigin,self->r.currentOrigin,self->client->ps.viewangles,0.3f))
								G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						else
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
					}
					}

					if(iGrip)
						pushPowerMod *=2;

					push_list[x]->client->ps.velocity[0] = pushDir[0]*pushPowerMod;
					push_list[x]->client->ps.velocity[1] = pushDir[1]*pushPowerMod;

					if ((int)push_list[x]->client->ps.velocity[2] == 0)
					{ //if not going anywhere vertically, boost them up a bit
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod +200;

						if (push_list[x]->client->ps.velocity[2] < 128)
						{
							push_list[x]->client->ps.velocity[2] = 128;
						}
					}
					else
					{
						push_list[x]->client->ps.velocity[2] = pushDir[2]*pushPowerMod;
					}
				}
			}
			else if ( push_list[x]->s.eType == ET_MISSILE && push_list[x]->s.pos.trType != TR_STATIONARY && (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				if ( pull )
				{//deflect rather than reflect?
				}
				else 
				{
					//[ForceSys]
					G_DeflectMissile( self, push_list[x], forward );
					//G_ReflectMissile( self, push_list[x], forward );
					//[/ForceSys]
				}
			}
			else if ( !Q_stricmp( "func_static", push_list[x]->classname ) )
			{//force-usable func_static
				if ( !pull && (push_list[x]->spawnflags&1/*F_PUSH*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
				else if ( pull && (push_list[x]->spawnflags&2/*F_PULL*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
			}
			else if ( !Q_stricmp( "func_door", push_list[x]->classname ) && (push_list[x]->spawnflags&2) )
			{//push/pull the door
				vec3_t	pos1, pos2;
				vec3_t	trFrom;

				VectorCopy(self->client->ps.origin, trFrom);
				trFrom[2] += self->client->ps.viewheight;

				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( trFrom, radius, forward, end );
				trap_Trace( &tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}

				if ( VectorCompare( vec3_origin, push_list[x]->s.origin ) )
				{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract( push_list[x]->r.absmax, push_list[x]->r.absmin, size );
					VectorMA( push_list[x]->r.absmin, 0.5, size, center );
					if ( (push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS1 )
					{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos1, center );
					}
					else if ( !(push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS2 )
					{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos2, center );
					}
					VectorAdd( center, push_list[x]->pos1, pos1 );
					VectorAdd( center, push_list[x]->pos2, pos2 );
				}
				else
				{//actually has an origin, pos1 and pos2 are absolute
					VectorCopy( push_list[x]->r.currentOrigin, center );
					VectorCopy( push_list[x]->pos1, pos1 );
					VectorCopy( push_list[x]->pos2, pos2 );
				}

				if ( Distance( pos1, trFrom ) < Distance( pos2, trFrom ) )
				{//pos1 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{//pos2 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GEntity_UseFunc( push_list[x], self, self );
			}
			else if ( Q_stricmp( "func_button", push_list[x]->classname ) == 0 )
			{//pretend you pushed it
				Touch_Button( push_list[x], self, NULL );
				continue;
			}
		}
	}

	//attempt to break any leftover grips
	//if we're still in a current grip that wasn't broken by the push, it will still remain
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}
}

extern void WP_ActivateSaber( gentity_t *self );

void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower )
{
	int wasActive = self->client->ps.fd.forcePowersActive;

	self->client->ps.fd.forcePowersActive &= ~( 1 << forcePower );

	switch( (int)forcePower )
	{
	case FP_HEAL:
		self->client->ps.fd.forceHealAmount = 0;
		self->client->ps.fd.forceHealTime = 0;
		break;
	case FP_LEVITATION:
		break;
	case FP_SPEED:
		if (wasActive & (1 << FP_SPEED))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_2-50], CHAN_VOICE);
		}
		if(self->client->forceSpeedStartTime+2000 > level.time && self->client->ps.fd.forcePowerLevel[FP_SPEED] >= FORCE_LEVEL_3)
			self->client->ps.fd.forcePower -= 20;
		break;
	case FP_PUSH:
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}
		self->client->ps.activeForcePass = 0;
		break;
	case FP_PULL:
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}
		self->client->ps.activeForcePass = 0;
		break;
	case FP_TELEPATHY:
		if (wasActive & (1 << FP_TELEPATHY))
		{
//			G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav") );
		}			
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
		break;
	case FP_SEE:
		if (wasActive & (1 << FP_SEE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_5-50], CHAN_VOICE);
		}
		break;
	case FP_TEAM_HEAL:
		if (wasActive & (1 << FP_TEAM_HEAL))
		{
		//	G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav") );
		}		
		break;
	case FP_GRIP:
		self->client->ps.fd.forceGripUseTime = level.time + 3000;
		if (self->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].client &&
			g_entities[self->client->ps.fd.forceGripEntityNum].health > 0 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse &&
			(level.time - g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.fd.forceGripStarted) > 500)
		{ //if we had our throat crushed in for more than half a second, gasp for air when we're let go
			if (wasActive & (1 << FP_GRIP))
			{
				G_EntitySound( &g_entities[self->client->ps.fd.forceGripEntityNum], CHAN_VOICE, G_SoundIndex("*gasp.wav") );
			}
		}

		if (g_entities[self->client->ps.fd.forceGripEntityNum].client &&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse)
		{
			
			g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.forceGripChangeMovetype = PM_NORMAL;
			//[ForceSys]
			g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.fd.forceGripCripple = 0;
			//[/ForceSys]
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0;
		}

		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		self->client->ps.powerups[PW_DISINT_4] = 0;
		break;
	case FP_TEAM_FORCE:
		if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] < FORCE_LEVEL_2)
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 1500;
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
		break;
	case FP_LIGHTNING:
		if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 1500;
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
		break;
	case FP_RAGE:
		self->client->ps.fd.forceRageRecoveryTime = level.time + 10000;
		if (wasActive & (1 << FP_RAGE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
		break;
	case FP_ABSORB:
		if (wasActive & (1 << FP_ABSORB))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
		break;
	case FP_PROTECT:
		if (wasActive & (1 << FP_PROTECT))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3-50], CHAN_VOICE);
		}
		break;
	case FP_DRAIN:
		if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum...
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 1500;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
	default:
		break;
	}
}



qboolean ValidGripEnt(gentity_t*self,gentity_t*ent)
{
	if(!ent)
		return qfalse;

	if(Q_stricmp(ent->classname,"body")==0)
		return qtrue;

	if(!ent->client || !ent->inuse || ent->health < 1 || !ForcePowerUsableOn(self, ent, FP_GRIP))
		return qfalse;

	return qtrue;
}
void DoGraspAction(gentity_t *self, forcePowers_t forcePower)
{//racc - have someone in our grasp, deal with them.
	gentity_t *gripEnt;
	int gripLevel = 0;
	trace_t tr;
	vec3_t a;
	vec3_t fwd, fwd_o, start_o, nvel;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gripEnt = &g_entities[self->client->ps.fd.forceGripEntityNum];

	if (!ValidGripEnt(self,gripEnt))
	{
		WP_ForcePowerStop(self,FP_GRIP);
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		if (gripEnt && gripEnt->client && gripEnt->inuse)
		{
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}
		return;
	}

	if(gripEnt->client)
		VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);
	else
		VectorSubtract(gripEnt->s.pos.trBase,self->client->ps.origin,a);

	if(gripEnt->client)
		trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID);
	else
		trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->s.pos.trBase, self->s.number, MASK_PLAYERSOLID);

	//[ForceSys]
	gripLevel = WP_AbsorbConversion(gripEnt, gripEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.fd.forcePowerLevel[FP_GRIP], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP]);
																													 
	 
																												

	if (gripLevel == -1)
	{
		gripLevel = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	}

	 //racc - absorb no longer has an active power


	if (!gripLevel)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}
	
	//[/ForceSys]

	if (VectorLength(a) > MAX_GRIP_DISTANCE)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if(gripEnt->client)
	{
		if ( !InFront( gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) &&
			gripLevel < FORCE_LEVEL_3)
		{
			WP_ForcePowerStop(self, forcePower);
			return;
		}
	}
	else
	{
		if ( !InFront( gripEnt->s.pos.trBase, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) &&
			gripLevel < FORCE_LEVEL_3)
		{
			WP_ForcePowerStop(self, forcePower);
			return;
		}
	}

	/*
	if (tr.fraction != 1.0f &&
		tr.entityNum != gripEnt->s.number *//*&&
		gripLevel < FORCE_LEVEL_3*//*)
	{
		WP_ForcePowerStop(self, forcePower);
		continue;
	}
*/
	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time)
	{ //no damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;

	}

	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.

	if (gripLevel == FORCE_LEVEL_1)
	{
		if(gripEnt->client)
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if(gripEnt->client)
		{
			G_SetAnim(gripEnt, &gripEnt->client->pers.cmd, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);			
		}

		if(gripEnt->client)
			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
	}

	if (gripLevel == FORCE_LEVEL_2)
	{
		if(gripEnt->client)
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if(gripEnt->client)
		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			gripEnt->client->ps.velocity[2] = 5;

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		if(gripEnt->client)
		{
			gripEnt->client->ps.otherKiller = self->s.number;
			gripEnt->client->ps.otherKillerTime = level.time + 10000;
			gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
			//[Asteroids]
			gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
			gripEnt->client->otherKillerVehWeapon = 0;
			gripEnt->client->otherKillerWeaponType = WP_NONE;
		//[/Asteroids]

			gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;
			G_SetAnim(gripEnt, &gripEnt->client->pers.cmd, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}

		if(gripEnt->client)
		{

			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000 && !self->client->ps.fd.forceGripDamageDebounceTime)
			{ //if we managed to lift him into the air for 2 seconds, give him a crack
				self->client->ps.fd.forceGripDamageDebounceTime = 1;


				//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
				//G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );



				if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
				{ //choking, so don't let him keep gripping himself
					WP_ForcePowerStop(gripEnt, FP_GRIP);
				}
			}
			else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 10000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
		}
	}

	if (gripLevel == FORCE_LEVEL_3)
	{
		if(gripEnt->client)
		{
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

			gripEnt->client->ps.otherKiller = self->s.number;
			gripEnt->client->ps.otherKillerTime = level.time + 20000;
			gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
			//[Asteroids]
			gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
			gripEnt->client->otherKillerVehWeapon = 0;
			gripEnt->client->otherKillerWeaponType = WP_NONE;
			//[/Asteroids]

			gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;
			G_SetAnim(gripEnt, &gripEnt->client->pers.cmd, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		}
		if(gripEnt->client)
		{
			if (gripEnt->client->ps.forceGripMoveInterval < level.time)
			{
				float nvLen = 0;

				VectorCopy(gripEnt->client->ps.origin, start_o);
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
				fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
				fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
				fwd_o[2] += 16;
				VectorSubtract(fwd_o, start_o, nvel);

				nvLen = VectorLength(nvel);

				if (nvLen < 32)
				{ //within x units of desired spot
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*64;
					gripEnt->client->ps.velocity[1] = nvel[1]*64;
					gripEnt->client->ps.velocity[2] = nvel[2]*64;
				}
				else if (nvLen < 128)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*256;
					gripEnt->client->ps.velocity[1] = nvel[1]*256;
					gripEnt->client->ps.velocity[2] = nvel[2]*256;
				}
				else if (nvLen < 256)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*512;
					gripEnt->client->ps.velocity[1] = nvel[1]*512;
					gripEnt->client->ps.velocity[2] = nvel[2]*512;
				}
				else if (nvLen < 400)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*1024;
					gripEnt->client->ps.velocity[1] = nvel[1]*1024;
					gripEnt->client->ps.velocity[2] = nvel[2]*1024;
				}
				else
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*1024;
					gripEnt->client->ps.velocity[1] = nvel[1]*1024;
					gripEnt->client->ps.velocity[2] = nvel[2]*1024;
				}

				gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
			}

			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 10000 && !self->client->ps.fd.forceGripDamageDebounceTime)
			{ //if we managed to lift him into the air for 2 seconds, give him a crack
				self->client->ps.fd.forceGripDamageDebounceTime = 1;


				//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
				//G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

				

				if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
				{ //choking, so don't let him keep gripping himself
					WP_ForcePowerStop(gripEnt, FP_GRIP);
				}
			}
			else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 20000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
		}//end FORCE_LEVEL_3
		else
			{
float nvLen = 0;

				VectorCopy(gripEnt->s.pos.trBase, start_o);
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
				fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
				fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
				fwd_o[2] += 16;
				VectorSubtract(fwd_o, start_o, nvel);

				nvLen = VectorLength(nvel);

				if (nvLen < 32)
				{ //within x units of desired spot
					VectorNormalize(nvel);
					gripEnt->s.pos.trBase[0] = nvel[0]*64;
					gripEnt->s.pos.trBase[1] = nvel[1]*64;
					gripEnt->s.pos.trBase[2] = nvel[2]*64;
				}
				else if (nvLen < 128)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trBase[0] = nvel[0]*256;
					gripEnt->s.pos.trBase[1] = nvel[1]*256;
					gripEnt->s.pos.trBase[2] = nvel[2]*256;
				}
				else if (nvLen < 256)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*512;
					gripEnt->s.pos.trDelta[1] = nvel[1]*512;
					gripEnt->s.pos.trDelta[2] = nvel[2]*512;
				}
				else if (nvLen < 400)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*1024;
					gripEnt->s.pos.trDelta[1] = nvel[1]*1024;
					gripEnt->s.pos.trDelta[2] = nvel[2]*1024;
				}
				else
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*1024;
					gripEnt->s.pos.trDelta[1] = nvel[1]*1024;
					gripEnt->s.pos.trDelta[2] = nvel[2]*1024;
				}

				//gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
			}
	}
}

void DoGripAction(gentity_t *self, forcePowers_t forcePower)
{//racc - have someone in our grip, deal with them.
	gentity_t *gripEnt;
	int gripLevel = 0;
	trace_t tr;
	vec3_t a;
	vec3_t fwd, fwd_o, start_o, nvel;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gripEnt = &g_entities[self->client->ps.fd.forceGripEntityNum];

	if (!ValidGripEnt(self,gripEnt))
	{
		WP_ForcePowerStop(self,FP_GRIP);
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		if (gripEnt && gripEnt->client && gripEnt->inuse)
		{
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}
		return;
	}

	if(gripEnt->client)
		VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);
	else
		VectorSubtract(gripEnt->s.pos.trBase,self->client->ps.origin,a);

	if(gripEnt->client)
		trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID);
	else
		trap_Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->s.pos.trBase, self->s.number, MASK_PLAYERSOLID);

	//[ForceSys]
	gripLevel = WP_AbsorbConversion(gripEnt, gripEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.fd.forcePowerLevel[FP_GRIP], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP]);
																													 
	 
																												

	if (gripLevel == -1)
	{
		gripLevel = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	}

	 //racc - absorb no longer has an active power


	if (!gripLevel)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}
	
	//[/ForceSys]

	if (VectorLength(a) > MAX_GRIP_DISTANCE)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if(gripEnt->client)
	{
		if ( !InFront( gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) &&
			gripLevel < FORCE_LEVEL_3)
		{
			WP_ForcePowerStop(self, forcePower);
			return;
		}
	}
	else
	{
		if ( !InFront( gripEnt->s.pos.trBase, self->client->ps.origin, self->client->ps.viewangles, 0.9f ) &&
			gripLevel < FORCE_LEVEL_3)
		{
			WP_ForcePowerStop(self, forcePower);
			return;
		}
	}

	/*
	if (tr.fraction != 1.0f &&
		tr.entityNum != gripEnt->s.number *//*&&
		gripLevel < FORCE_LEVEL_3*//*)
	{
		WP_ForcePowerStop(self, forcePower);
		continue;
	}
*/
	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time)
	{ //2 damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_3)
		{
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
				G_Damage(gripEnt, self, self, NULL, NULL, 7, 0, MOD_FORCE_DARK);
				}
				else
				{
				G_Damage(gripEnt, self, self, NULL, NULL, 5, 0, MOD_FORCE_DARK);
				}

		}
		else if (self->client->ps.fd.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2)
		{
		G_Damage(gripEnt, self, self, NULL, NULL, 3, 0, MOD_FORCE_DARK);
		}
		else 
		{
		G_Damage(gripEnt, self, self, NULL, NULL, 1, 0, MOD_FORCE_DARK);
		}
	}

	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.

	if (gripLevel == FORCE_LEVEL_1)
	{
		if(gripEnt->client)
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if(gripEnt->client)
		{
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );
			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 500;		
		}
		
		if(gripEnt->client)
			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
	}

	if (gripLevel == FORCE_LEVEL_2)
	{
		if(gripEnt->client)
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if(gripEnt->client)
		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			gripEnt->client->ps.velocity[2] = 5;

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		if(gripEnt->client)
		{
			gripEnt->client->ps.otherKiller = self->s.number;
			gripEnt->client->ps.otherKillerTime = level.time + 10000;
			gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
			//[Asteroids]
			gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
			gripEnt->client->otherKillerVehWeapon = 0;
			gripEnt->client->otherKillerWeaponType = WP_NONE;
		//[/Asteroids]

			gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;
			
			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 1000;
		}

		if(gripEnt->client)
		{
			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000 && !self->client->ps.fd.forceGripDamageDebounceTime)
			{ //if we managed to lift him into the air for 2 seconds, give him a crack
				self->client->ps.fd.forceGripDamageDebounceTime = 1;
				G_Damage(gripEnt, self, self, NULL, NULL, 50, 0, MOD_FORCE_DARK);



				if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
				{ //choking, so don't let him keep gripping himself
					WP_ForcePowerStop(gripEnt, FP_GRIP);
				}
			}
			else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 10000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
		}
	}

	if (gripLevel == FORCE_LEVEL_3)
	{
		if(gripEnt->client)
		{
			gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

			gripEnt->client->ps.otherKiller = self->s.number;
			gripEnt->client->ps.otherKillerTime = level.time + 20000;
			gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
			//[Asteroids]
			gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
			gripEnt->client->otherKillerVehWeapon = 0;
			gripEnt->client->otherKillerWeaponType = WP_NONE;
			//[/Asteroids]

			gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;
			
			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound( gripEnt, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;
		}
		if(gripEnt->client)
		{
			if (gripEnt->client->ps.forceGripMoveInterval < level.time)
			{
				float nvLen = 0;

				VectorCopy(gripEnt->client->ps.origin, start_o);
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
				fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
				fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
				fwd_o[2] += 16;
				VectorSubtract(fwd_o, start_o, nvel);

				nvLen = VectorLength(nvel);

				if (nvLen < 32)
				{ //within x units of desired spot
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*64;
					gripEnt->client->ps.velocity[1] = nvel[1]*64;
					gripEnt->client->ps.velocity[2] = nvel[2]*64;
				}
				else if (nvLen < 128)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*256;
					gripEnt->client->ps.velocity[1] = nvel[1]*256;
					gripEnt->client->ps.velocity[2] = nvel[2]*256;
				}
				else if (nvLen < 256)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*512;
					gripEnt->client->ps.velocity[1] = nvel[1]*512;
					gripEnt->client->ps.velocity[2] = nvel[2]*512;
				}
				else if (nvLen < 400)
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*1024;
					gripEnt->client->ps.velocity[1] = nvel[1]*1024;
					gripEnt->client->ps.velocity[2] = nvel[2]*1024;
				}
				else
				{
					VectorNormalize(nvel);
					gripEnt->client->ps.velocity[0] = nvel[0]*1024;
					gripEnt->client->ps.velocity[1] = nvel[1]*1024;
					gripEnt->client->ps.velocity[2] = nvel[2]*1024;
				}

				gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
			}

			if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 10000 && !self->client->ps.fd.forceGripDamageDebounceTime)
			{ //if we managed to lift him into the air for 2 seconds, give him a crack
				self->client->ps.fd.forceGripDamageDebounceTime = 1;
				G_Damage(gripEnt, self, self, NULL, NULL, 100, 0, MOD_FORCE_DARK);



				if (gripEnt->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
				{ //choking, so don't let him keep gripping himself
					WP_ForcePowerStop(gripEnt, FP_GRIP);
				}
			}
			else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 20000)
			{
				WP_ForcePowerStop(self, forcePower);
			}
		}//end FORCE_LEVEL_3
		else
			{
float nvLen = 0;

				VectorCopy(gripEnt->s.pos.trBase, start_o);
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				fwd_o[0] = self->client->ps.origin[0] + fwd[0]*128;
				fwd_o[1] = self->client->ps.origin[1] + fwd[1]*128;
				fwd_o[2] = self->client->ps.origin[2] + fwd[2]*128;
				fwd_o[2] += 16;
				VectorSubtract(fwd_o, start_o, nvel);

				nvLen = VectorLength(nvel);

				if (nvLen < 32)
				{ //within x units of desired spot
					VectorNormalize(nvel);
					gripEnt->s.pos.trBase[0] = nvel[0]*64;
					gripEnt->s.pos.trBase[1] = nvel[1]*64;
					gripEnt->s.pos.trBase[2] = nvel[2]*64;
				}
				else if (nvLen < 128)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trBase[0] = nvel[0]*256;
					gripEnt->s.pos.trBase[1] = nvel[1]*256;
					gripEnt->s.pos.trBase[2] = nvel[2]*256;
				}
				else if (nvLen < 256)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*512;
					gripEnt->s.pos.trDelta[1] = nvel[1]*512;
					gripEnt->s.pos.trDelta[2] = nvel[2]*512;
				}
				else if (nvLen < 400)
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*1024;
					gripEnt->s.pos.trDelta[1] = nvel[1]*1024;
					gripEnt->s.pos.trDelta[2] = nvel[2]*1024;
				}
				else
				{
					VectorNormalize(nvel);
					gripEnt->s.pos.trDelta[0] = nvel[0]*1024;
					gripEnt->s.pos.trDelta[1] = nvel[1]*1024;
					gripEnt->s.pos.trDelta[2] = nvel[2]*1024;
				}

				//gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
			}
	}
}


qboolean G_IsMindTricked(forcedata_t *fd, int client)
{
	int checkIn;
	int trickIndex1, trickIndex2, trickIndex3, trickIndex4;
	int sub = 0;

	if (!fd)
	{
		return qfalse;
	}

	trickIndex1 = fd->forceMindtrickTargetIndex;
	trickIndex2 = fd->forceMindtrickTargetIndex2;
	trickIndex3 = fd->forceMindtrickTargetIndex3;
	trickIndex4 = fd->forceMindtrickTargetIndex4;

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn & (1 << (client-sub)))
	{
		return qtrue;
	}
	
	return qfalse;
}



static void RemoveTrickedEnt(forcedata_t *fd, int client)
{
	if (!fd)
	{
		return;
	}

	if (client > 47)
	{
		fd->forceMindtrickTargetIndex4 &= ~(1 << (client-48));
	}
	else if (client > 31)
	{
		fd->forceMindtrickTargetIndex3 &= ~(1 << (client-32));
	}
	else if (client > 15)
	{
		fd->forceMindtrickTargetIndex2 &= ~(1 << (client-16));
	}
	else
	{
		fd->forceMindtrickTargetIndex &= ~(1 << client);
	}
}

extern int g_LastFrameTime;
extern int g_TimeSinceLastFrame;





static void WP_UpdateMindtrickEnts(gentity_t *self)
{

	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (G_IsMindTricked(&self->client->ps.fd, i))
		{
			gentity_t *ent = &g_entities[i];

			if ( !ent || !ent->client || !ent->inuse || ent->health < 1 )
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);


		
			}

//			else if ((level.time - self->client->dangerTime) < g_TimeSinceLastFrame*4)
//			{ //Untrick this entity if the tricker (self) fires while in his fov
//				if (trap_InPVS(ent->client->ps.origin, self->client->ps.origin) &&
//					OrgVisible(ent->client->ps.origin, self->client->ps.origin, ent->s.number) )
//				{
//					RemoveTrickedEnt(&self->client->ps.fd, i);
//				}
//			}

			else if (BG_HasYsalamiri(g_gametype.integer, &ent->client->ps))
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);	


			}
			
		}


		i++;
	}
		
//	if (!self->client->ps.fd.forceMindtrickTargetIndex &&
//		!self->client->ps.fd.forceMindtrickTargetIndex2 &&
//		!self->client->ps.fd.forceMindtrickTargetIndex3 &&
//		!self->client->ps.fd.forceMindtrickTargetIndex4)
//	{ //everyone who we had tricked is no longer tricked, so stop the power
//		WP_ForcePowerStop(self, FP_TELEPATHY);
//		//WP_ForcePowerStop(self, FP_TEAM_HEAL);
//	}
//	else if (self->client->ps.powerups[PW_REDFLAG] ||
//		self->client->ps.powerups[PW_BLUEFLAG])
//	{
//		WP_ForcePowerStop(self, FP_TELEPATHY);
//		WP_ForcePowerStop(self, FP_TEAM_HEAL);
//	}
}


//[BugFix27]
//Debouncer information for the lightning
static int LightningDebounceTime = 0;
//sets the time between lightning hit shots on the server so that we can alter the sv_fps without issues.  
#define LIGHTNINGDEBOUNCE		50 
//[/BugFix27]
static int SpeedDebounceTime = 0;
//sets the time between force speed FP drains.  
static int ProtectDebounceTime=0;
//same as above except for Protect
#define SPEEDDEBOUNCE		200 
#define PROTECTDEBOUNCE		200
static void WP_ForcePowerRun( gentity_t *self, forcePowers_t forcePower, usercmd_t *cmd )
{
	extern usercmd_t	ucmd;

	switch( (int)forcePower )
	{
	case FP_HEAL:
		if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1)
		{
			if (self->client->ps.velocity[0] || self->client->ps.velocity[1] || self->client->ps.velocity[2])
			{
				WP_ForcePowerStop( self, forcePower );
				break;
			}
		}

		if (self->health < 1 || self->client->ps.stats[STAT_HEALTH] < 1)
		{
			WP_ForcePowerStop( self, forcePower );
			break;
		}

		if (self->client->ps.fd.forceHealTime > level.time)
		{
			break;
		}
				
		if (self->client->skillLevel[SK_HEALA] == FORCE_LEVEL_2)
		{
		if ( self->client->ps.fd.forcePower >= self->client->ps.fd.forcePowerMax && self->client->ps.stats[STAT_DODGE] >= self->client->ps.stats[STAT_MAX_DODGE])
		{ //rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
			WP_ForcePowerStop( self, forcePower );
			break;
		}
		self->client->ps.fd.forceHealTime = level.time + 1000;
		self->client->ps.fd.forcePower++;
		self->client->ps.stats[STAT_DODGE]++;
		if ( self->client->ps.fd.forcePower >= self->client->ps.fd.forcePowerMax && self->client->ps.stats[STAT_DODGE] >= self->client->ps.stats[STAT_MAX_DODGE])	// Past max health
		{
			self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
			self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
			WP_ForcePowerStop( self, forcePower );
		}

		if ( (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount > 25) ||
			(self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount > 50))
		{
			WP_ForcePowerStop( self, forcePower );
		}		
		}
		else
		{
		if ( self->health >= self->client->ps.stats[STAT_MAX_HEALTH] && self->client->ps.stats[STAT_ARMOR] >= self->client->ps.stats[STAT_MAX_ARMOR])
		{ //rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
			WP_ForcePowerStop( self, forcePower );
			break;
		}
		self->client->ps.fd.forceHealTime = level.time + 1000;
		self->health++;
		self->client->ps.stats[STAT_ARMOR]++;
		self->client->ps.fd.forceHealAmount++;

		if ( self->health >= self->client->ps.stats[STAT_MAX_HEALTH] && self->client->ps.stats[STAT_ARMOR] >= self->client->ps.stats[STAT_MAX_ARMOR])	// Past max health
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
			self->client->ps.stats[STAT_ARMOR] = self->client->ps.stats[STAT_MAX_ARMOR];
			WP_ForcePowerStop( self, forcePower );
		}

		if ( (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount > 25) ||
			(self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount > 50))
		{
			WP_ForcePowerStop( self, forcePower );
		}			
		}
		break;
	case FP_SPEED:
		//This is handled in PM_WalkMove and PM_StepSlideMove
		if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD )
		{
			if ( g_entities[self->client->holdingObjectiveItem].genericValue15 )
			{//disables force powers
				WP_ForcePowerStop( self, forcePower );
			}
		}

		if ( (ucmd.buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_SPEED )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_SPEED] = level.time + 500;
		}

		
		if ( self->client->ps.fd.forcePower < forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED] 
			|| self->client->ps.fd.forcePowerDuration[FP_SPEED] < level.time )
		{
			WP_ForcePowerStop( self, forcePower );
		}
		else if( SpeedDebounceTime == level.time //someone already advanced the timer this frame
			|| (level.time - SpeedDebounceTime >= SPEEDDEBOUNCE) )
		{
			BG_ForcePowerDrain( &self->client->ps, forcePower, 0 );
			SpeedDebounceTime = level.time;
			
		}
		/*
		if ( self->client->ps.powerups[PW_REDFLAG]
			|| self->client->ps.powerups[PW_BLUEFLAG]
			|| self->client->ps.powerups[PW_NEUTRALFLAG] )
		{//no force speed when carrying flag
			WP_ForcePowerStop( self, forcePower );
		}
		*/
		break;
	case FP_PROTECT:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			BG_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}
		if (self->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PROTECT2);
		}
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 300;
		}
		break;
	case FP_ABSORB:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			BG_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}
			
		if (self->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_ABSORB2);
		}
		
			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 300;
		}
		break;
	case FP_TEAM_HEAL:
		if ( (ucmd.buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_TEAM_HEAL )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_TEAM_HEAL] = level.time + 500;
		 
		}	


	if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD )
		{
			if ( g_entities[self->client->holdingObjectiveItem].genericValue15 )
			{//disables force powers
				WP_ForcePowerStop( self, forcePower );
			}
		}


		
		break;		
	case FP_GRIP:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		if (self->client->ps.fd.forcePowerDebounce[FP_PULL] < level.time)
		{ //This is sort of not ideal. Using the debounce value reserved for pull for this because pull doesn't need it.
			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 );
			self->client->ps.fd.forcePowerDebounce[FP_PULL] = level.time + 500;
		}
		if (self->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}
		if (self->client->skillLevel[SK_GRIPA] == FORCE_LEVEL_2)
		{
		DoGraspAction(self, forcePower);
		}
		else
		{
		DoGripAction(self, forcePower);
		}
		break;
	case FP_LEVITATION:
		if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.fd.forceJumpZStart )
		{//done with jump
			WP_ForcePowerStop( self, forcePower );
		}
		break;
	case FP_RAGE:
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime;

			if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 128;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 256;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 512;
			}
			else 
			{
				addTime = 100;
			}
			
		if (self->client->skillLevel[SK_RAGEA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_RAGE2);
		}
		
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}
		
		break;
	case FP_DRAIN:
if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if ( self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( (cmd->buttons & BUTTON_FORCE_DRAIN) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_DRAIN) )
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
			}
		}
		// OVERRIDEFIXME
	 
		if (self->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_DRAIN2);
		}
									
						
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) || self->client->ps.fd.forcePowerDuration[FP_DRAIN] < level.time ||
			self->client->ps.fd.forcePower < 1)
 
										 
			   
		{
		  
	  
				
			WP_ForcePowerStop( self, forcePower );

		}
	 
																							  
								 
			  
																									   
		else
																 
		
			   
		{
		if (self->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2)
		{
			ForceShootSevere( self );
		}
		else
		{
			ForceShootDrain( self );			
		}

			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 ); //used to be 1, but this did, too, anger the God of Balance.	     
														  
												
				

									
																		   
		}
	
		 
		break;
	case FP_LIGHTNING:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{ //Animation for hand extend doesn't end with hand out, so we have to limit lightning intervals by animation intervals (once hand starts to go in in animation, lightning should stop)
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if ( self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( (cmd->buttons & BUTTON_FORCE_LIGHTNING) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_LIGHTNING) )
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
			}
		}

		// OVERRIDEFIXME
		//[ForceSys]
		if (self->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_LIGHTNING2);
		}
	
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) || self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] < level.time 
			 || self->client->ps.fd.forcePower < 1 )
		//if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) || self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] < level.time ||
		//	self->client->ps.fd.forcePower < 25 || self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		//[/ForceSys]
		{
			WP_ForcePowerStop( self, forcePower );
		}
		//[BugFix27]
		//added server debouncer to make the lightning damage consistant even with different sv_fps settings.
		else if( LightningDebounceTime == level.time //someone already advanced the timer this frame
			|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
		//else
		//[/BugFix27]
		{

		if (self->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2)
		{
			ForceShootJudgement( self );
		}	
		else
		{
			ForceShootLightning( self );			
		}

			//[ForceSys]
			
			BG_ForcePowerDrain( &self->client->ps, forcePower, 1 ); //holding FP cost
															  
																	   
																	  
																		  
									  
			//BG_ForcePowerDrain( &self->client->ps, forcePower, 0 );
			//[/ForceSys]
														 

			//[BugFix27]
			//update the lightning shot debouncer
			LightningDebounceTime = level.time;
			//[/BugFix27]						  
		}
		break;
	case FP_TEAM_FORCE:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

	if ( (ucmd.buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_TEAM_FORCE )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_TEAM_FORCE] = level.time + 1;
		}
		// OVERRIDEFIXME

		if (self->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_DESTRUCTION2);
		}

		if (!WP_ForcePowerAvailable(self, forcePower, 0) || self->client->ps.fd.forcePowerDuration[FP_TEAM_FORCE] < level.time ||
			self->client->ps.fd.forcePower < 1)



		{



			WP_ForcePowerStop(self, forcePower);

		}





		else



		{
		if (self->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2)
		{
			ForceShootBlinding(self);			
		}
		else
		{
			ForceShootDestruction(self);			
		}






//			BG_ForcePowerDrain( &self->client->ps, forcePower, 20 ); //used to be 1, but this did, too, anger the God of Balance.

		}

	
		break;
	case FP_TELEPATHY:
	if ( (ucmd.buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_TELEPATHY )
		{//holding it keeps it going
			self->client->ps.fd.forcePowerDuration[FP_TELEPATHY] = level.time + 500;
		}

		if ( self->client->holdingObjectiveItem >= MAX_CLIENTS  
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD
			&& g_entities[self->client->holdingObjectiveItem].genericValue15 )
		{ //if force hindered can't mindtrick whilst carrying a siege item
			WP_ForcePowerStop( self, FP_TELEPATHY );
		}
		else
		{
			WP_UpdateMindtrickEnts(self);
		}			

		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	case FP_PUSH:		
		if (self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PUSH2);
		}		

		break;
	case FP_PULL:		
		if (self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PULL2);
		}		

		break;
	default:
		break;
	}
}

int WP_DoSpecificPower( gentity_t *self, usercmd_t *ucmd, forcePowers_t forcepower)
{
	int powerSucceeded;

	powerSucceeded = 1;

	// OVERRIDEFIXME
	if ( !WP_ForcePowerAvailable( self, forcepower, 0 ) )
	{
		return 0;
	}

	switch(forcepower)
	{
	case FP_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_HEALA] == FORCE_LEVEL_2)
		{
		ForceRegeneration( self );
		}
		else
		{
		ForceHeal(self);	
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_LEVITATION:
		//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.
		
		if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			self->client->ps.fd.forceJumpCharge = 0;
			G_MuteSound( self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE );
			//This only happens if the groundEntityNum == ENTITYNUM_NONE when the button is actually released
		}
		else
		{//still on ground, so jump
			ForceJump( self, ucmd );
		}
		break;
	case FP_SPEED:
		//[ForceSys]
		/*
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		*/
		//[/ForceSys]
		ForceSpeed(self, 0);
		//[ForceSys]
		//self->client->ps.fd.forceButtonNeedRelease = 1;
		//[/ForceSys]
		break;
	case FP_GRIP:
		if (self->client->ps.fd.forceGripEntityNum == ENTITYNUM_NONE)
		{

			
		if (self->client->skillLevel[SK_GRIPA] == FORCE_LEVEL_2)
		{
			ForceGrasp( self );
		}
		else
		{
			ForceGrip( self );
		}
		}

		if (self->client->ps.fd.forceGripEntityNum != ENTITYNUM_NONE)
		{
			if (!(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP)))
			{
				WP_ForcePowerStart( self, FP_GRIP, 0 );
			}
		}
		else
		{
			powerSucceeded = 0;
		}
		break;
	case FP_LIGHTNING:
		if (self->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2)
		{
		ForceJudgement(self);
		}	
		else
		{
		ForceLightning(self);
		}
		break;
	case FP_PUSH:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease && !(self->r.svFlags & SVF_BOT))
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PUSH2);
		}
		if (self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2)
		{

		//G_SetAnim(self, &self->client->pers.cmd, SETANIM_LEGS, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		ForceExplode(self, qfalse);
		}
		else
		{
		ForceThrow(self, qfalse);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PULL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2)
		{
		self->client->ps.userInt3 |= (1 << FLAG_PULL2);
		}
		if (self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2)
		{

		//G_SetAnim(self, &self->client->pers.cmd, SETANIM_LEGS, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		ForceExplode(self, qtrue);
		}
		else
		{
		ForceThrow(self, qtrue);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TELEPATHY:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_TELEPATHYA] == FORCE_LEVEL_2)
		{
		ForceCorrupt(self);
		}	
		else
		{
		ForceTelepathy(self);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_RAGE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_RAGEA] == FORCE_LEVEL_2)
		{
		ForceValor(self);
		}	
		else
		{
		ForceRage(self);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PROTECT:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2)
		{
		ForceDeathfield(self);
		}	
		else
		{
		ForceProtect(self);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_ABSORB:
		//[ForceSys]
									 
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2)
		{
		ForceDeathsight(self);
		}	
		else
		{
		ForceAbsorb(self);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
	
		//[/ForceSys]
		break;
	case FP_TEAM_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_STASISA] == FORCE_LEVEL_2)
		{
		ForceInsanity(self);
		}	
		else
		{
		ForceStasis(self);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_FORCE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (self->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2)
		{
		ForceBlinding(self);
		}	
		else
		{
		ForceDestruction(self);
		}

		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_DRAIN:
		if (self->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2)
		{
		ForceSever(self);
		}	
		else
		{
		ForceDrain(self);
		}
		break;
	case FP_SEE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceSeeing(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	return powerSucceeded;
}

void FindGenericEnemyIndex(gentity_t *self)
{ //Find another client that would be considered a threat.
	int i = 0;
	float tlen;
	gentity_t *ent;
	gentity_t *besten = NULL;
	float blen = 99999999.9f;
	vec3_t a;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->s.number != self->s.number && ent->health > 0 && !OnSameTeam(self, ent) && ent->client->ps.pm_type != PM_INTERMISSION && ent->client->ps.pm_type != PM_SPECTATOR)
		{
			VectorSubtract(ent->client->ps.origin, self->client->ps.origin, a);
			tlen = VectorLength(a);

			if (tlen < blen &&
				InFront(ent->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f ) &&
				OrgVisible(self->client->ps.origin, ent->client->ps.origin, self->s.number))
			{
				blen = tlen;
				besten = ent;
			}
		}

		i++;
	}

	if (!besten)
	{
		return;
	}

	self->client->ps.genericEnemyIndex = besten->s.number;
}

void DeathfieldBubbleDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	//[ForceSys]
	//the target saber blocked the lightning
	//[/ForceSys]
	qboolean forceBlocked = qfalse;
	int DEATHFIELD_TIME = 2500;
	int dmg = 1;
	int poweradd = 0;
	int modPowerLevel = -1;
	gentity_t *te = NULL;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->client)
	{


	if (ForcePowerUsableOn(self, traceEnt, FP_PROTECT))
	{
				//[ForceSys]
	int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				
				// removed the absorb code stuff.
	int modPowerLevel = -1;
				
	
				
				//[/ForceSys]

	if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_1)
	{
		dmg *= 1;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_2)
	{
		dmg *= 2;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_3)
	{
		dmg *= 3;
	}
	
	if (traceEnt->client)
	{
		modPowerLevel = WP_AbsorbConversion(traceEnt->client, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_PROTECT, self->client->ps.fd.forcePowerLevel[FP_PROTECT], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT]);
	}

	if (modPowerLevel != -1)
	{

			dmg = 0;

	}
	
	if ( traceEnt->client )
	{
	if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
	{
		dmg = 0;
	}
	}	
	
	forceBlocked = OJP_BlockStatus(self, traceEnt,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT], FP_PROTECT,qtrue);	
	
	if (dmg && !forceBlocked)
	{
	G_Damage(traceEnt, self, self, NULL, NULL, dmg, 0, MOD_FORCE_DARK);	
	}
		//At this point we know we got one, so add him into the collective event client bitflag

		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

	if (traceEnt->client->deathfieldTime < (level.time + DEATHFIELD_TIME/2) && (dmg && !forceBlocked))
	{

	gentity_t	*tent;
	tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FORCE_DEATHFIELDED);
	tent->s.eventParm = DirToByte(dir);
	tent->s.owner = traceEnt->s.number;
	traceEnt->client->deathfieldTime = level.time + DEATHFIELD_TIME;
	}	
	
	}	
	}		
}


void DeathfieldBubble(gentity_t *self)
{
	float radius = 128;
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	
	
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->deathfieldbubbledamageTime < level.time)
	{
		self->client->deathfieldbubbledamageTime = level.time + 30;	
	}
	else
	{
		return;		
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );


		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );


			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
		DeathfieldBubbleDamage( self, traceEnt, dir, ent_org );
		}
}

void DeathsightBubbleDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	//[ForceSys]
	//the target saber blocked the lightning
	//[/ForceSys]
	qboolean forceBlocked = qfalse;
	int DEATHSIGHT_TIME = 2500;
	int dmg = 1;
	int poweradd = 0;
	int modPowerLevel = -1;
	gentity_t *te = NULL;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if ( traceEnt && traceEnt->client)
	{


	if (ForcePowerUsableOn(self, traceEnt, FP_ABSORB))
	{
				//[ForceSys]
	int	dmg = 1;
				//int	dmg = Q_irand(1,2); //Q_irand( 1, 3 );
				
				// removed the absorb code stuff.
	int modPowerLevel = -1;
				
	
				
				//[/ForceSys]

	if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_1)
	{
		dmg *= 1;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_2)
	{
		dmg *= 2;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] == FORCE_LEVEL_3)
	{
		dmg *= 3;
	}
	
	if (traceEnt->client)
	{
		modPowerLevel = WP_AbsorbConversion(traceEnt->client, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_ABSORB, self->client->ps.fd.forcePowerLevel[FP_ABSORB], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB]);
	}

	if (modPowerLevel != -1)
	{

			dmg = 0;

	}
	
	if ( traceEnt->client )
	{
	if (traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
	{
		dmg = 0;
	}
	}	
	
	forceBlocked = OJP_BlockStatus(self, traceEnt,forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB], FP_ABSORB,qtrue);
	
	if (dmg && !forceBlocked)
	{
	G_Damage(traceEnt, self, self, NULL, NULL, dmg, 0, MOD_FORCE_DARK);	

	
	if ( traceEnt->client->ps.stats[STAT_HEALTH]+ traceEnt->client->ps.stats[STAT_ARMOR]-1 < 1 )
	{//electrocution effect
		traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
	}
	}
		//At this point we know we got one, so add him into the collective event client bitflag

		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

	if (traceEnt->client->deathsightTime < (level.time + DEATHSIGHT_TIME/2) && (dmg && !forceBlocked))
	{

	gentity_t	*tent;
	tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FORCE_DEATHSIGHTED);
	tent->s.eventParm = DirToByte(dir);
	tent->s.owner = traceEnt->s.number;
	traceEnt->client->deathsightTime = level.time + DEATHSIGHT_TIME;
	}	
	
	}	
	}		
}


void DeathsightBubble(gentity_t *self)
{
	float radius = 128;
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	
	
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->deathsightbubbledamageTime < level.time)
	{
		self->client->deathsightbubbledamageTime = level.time + 30;	
	}
	else
	{
		return;		
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	if ( self->client->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->client->ps.origin, center );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap_EntitiesInBox( mins, maxs, iEntityList, MAX_GENTITIES );

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			if ( !g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ ) 
			{
				if ( center[i] < traceEnt->r.absmin[i] ) 
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				} else if ( center[i] > traceEnt->r.absmax[i] ) 
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				} else 
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->r.absmax, traceEnt->r.absmin, size );
			VectorMA( traceEnt->r.absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius ) 
			{
				continue;
			}
		
			//in PVS?
			if ( !traceEnt->r.bmodel && !trap_InPVS( ent_org, self->client->ps.origin ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
		DeathsightBubbleDamage( self, traceEnt, dir, ent_org );
		}
	}
	else
	{//trace-line
		VectorMA( self->client->ps.origin, 2048, forward, end );
		
		trap_Trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );
		if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			return;
		}
		
		traceEnt = &g_entities[tr.entityNum];
		DeathsightBubbleDamage( self, traceEnt, forward, tr.endpos );
	}

}



void SeekerDroneUpdate(gentity_t *self)
{
	vec3_t org, elevated, dir, a, endir;
	gentity_t *en;
	float angle;
	float prefig = 0;
	trace_t tr;

	if (!(self->client->ps.eFlags & EF_SEEKERDRONE))
	{
		self->client->ps.genericEnemyIndex = -1;
		return;
	}

	if (self->health < 1)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags -= EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.droneExistTime >= level.time && 
		self->client->ps.droneExistTime < (level.time+5000))
	{
		self->client->ps.genericEnemyIndex = 1024+self->client->ps.droneExistTime;
		if (self->client->ps.droneFireTime < level.time)
		{
			G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/laser_trap/warning.wav") );
			self->client->ps.droneFireTime = level.time + 100;
		}
		return;
	}
	else if (self->client->ps.droneExistTime < level.time)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		prefig = (self->client->ps.droneExistTime-level.time)/80;

		if (prefig > 55)
		{
			prefig = 55;
		}
		else if (prefig < 1)
		{
			prefig = 1;
		}

		elevated[2] -= 55-prefig;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags -= EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.genericEnemyIndex == -1)
	{
		self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		if (!en || !en->client)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->s.number == self->s.number)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->health < 1)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (OnSameTeam(self, en))
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else
		{
			if (!InFront(en->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f ))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
			else if (!OrgVisible(self->client->ps.origin, en->client->ps.origin, self->s.number))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
		}
	}

	if (self->client->ps.genericEnemyIndex == ENTITYNUM_NONE || self->client->ps.genericEnemyIndex == -1)
	{
		FindGenericEnemyIndex(self);
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		//org is now where the thing should be client-side because it uses the same time-based offset
		if (self->client->ps.droneFireTime < level.time)
		{
			trap_Trace(&tr, org, NULL, NULL, en->client->ps.origin, -1, MASK_SOLID);

			if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
			{
				VectorSubtract(en->client->ps.origin, org, endir);
				VectorNormalize(endir);

				WP_FireGenericBlasterMissile(self, org, endir, 0, 15, 2000, MOD_BLASTER);
				G_SoundAtLoc( org, CHAN_WEAPON, G_SoundIndex("sound/weapons/bryar/fire.wav") );

				self->client->ps.droneFireTime = level.time + Q_irand(400, 700);
			}
		}
	}
}

void HolocronUpdate(gentity_t *self)
{ //keep holocron status updated in holocron mode
	int i = 0;
	int noHRank = 0;

	if (noHRank < FORCE_LEVEL_0)
	{
		noHRank = FORCE_LEVEL_0;
	}
	if (noHRank > FORCE_LEVEL_3)
	{
		noHRank = FORCE_LEVEL_3;
	}

	trap_Cvar_Update(&g_MaxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.holocronsCarried[i])
		{ //carrying it, make sure we have the power
			self->client->ps.holocronBits |= (1 << i);
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
		}
		else
		{ //otherwise, make sure the power is cleared from us
			self->client->ps.fd.forcePowerLevel[i] = 0;
			if (self->client->ps.holocronBits & (1 << i))
			{
				self->client->ps.holocronBits -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE)
			{
				self->client->ps.fd.forcePowersKnown -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}
			}
			else if (i == FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown |= (1 << i);

				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}


			}
			else if (i == FP_SABER_DEFENSE)
			{
				self->client->ps.fd.forcePowersKnown |= (1 << i);

				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}


			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}


	if (self->client->skillLevel[SK_BLUESTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_BLUESTYLE] |= FORCE_LEVEL_1;
	}	
	if (self->client->skillLevel[SK_REDSTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_REDSTYLE] |= FORCE_LEVEL_1;
	}	
	if (self->client->skillLevel[SK_PURPLESTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_PURPLESTYLE] |= FORCE_LEVEL_1;
	}
	if (self->client->skillLevel[SK_GREENSTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_GREENSTYLE] |= FORCE_LEVEL_1;
	}		
	if (self->client->skillLevel[SK_DUALSTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_DUALSTYLE] |= FORCE_LEVEL_1;
	}	
	if (self->client->skillLevel[SK_STAFFSTYLE] < FORCE_LEVEL_1)
	{
	self->client->skillLevel[SK_STAFFSTYLE] |= FORCE_LEVEL_1;
	}


	if (HasSetSaberOnly())
	{ //if saberonly, we get these powers no matter what (still need the holocrons for level 3)
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
		}
	}
}

void JediMasterUpdate(gentity_t *self)
{ //keep jedi master status updated for JM gametype
	int i = 0;

	trap_Cvar_Update(&g_MaxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.isJediMaster)
		{
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
/*
			if (i == FP_TEAM_HEAL || i == FP_TEAM_FORCE 
				)
			{ //team powers are useless in JM, absorb is too because no one else has powers to absorb. Drain is just
			  //relatively useless in comparison, because its main intent is not to heal, but rather to cripple others
			  //by draining their force at the same time. And no one needs force in JM except the JM himself.
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
				self->client->ps.fd.forcePowerLevel[i] = 0;
			}
*/
/*
			if (i == FP_TELEPATHY)
			{ //this decision was made because level 3 mindtrick allows the JM to just hide too much, and no one else has force
			  //sight to counteract it. Since the JM himself is the focus of gameplay in this mode, having him hidden for large
			  //durations is indeed a bad thing.
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_2;
			}
*/
		}
		else
		{
			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) )
			{
				self->client->ps.fd.forcePowersKnown -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive & (1 << i)) )
			{
				WP_ForcePowerStop(self, i);
			}

			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}
}

qboolean WP_HasForcePowers( const playerState_t *ps )
{
	int i;
	if ( ps )
	{
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( i == FP_LEVITATION )
			{
				if ( ps->fd.forcePowerLevel[i] > FORCE_LEVEL_1 )
				{
					return qtrue;
				}
			}
			else if ( ps->fd.forcePowerLevel[i] > FORCE_LEVEL_0 )
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

//try a special roll getup move
qboolean G_SpecialRollGetup(gentity_t *self)
{ //fixme: currently no knockdown will actually land you on your front... so froll's are pretty useless at the moment.
	qboolean rolled = qfalse;

	/*
	if (self->client->ps.weapon != WP_SABER &&
		self->client->ps.weapon != WP_MELEE)
	{ //can't do acrobatics without saber selected
		return qfalse;
	}
	*/

	if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove > 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_R, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove < 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_L, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove > 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_F, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (self->client->pers.cmd.upmove)
	{
		G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		self->client->ps.forceDodgeAnim = 2;
		self->client->ps.forceHandExtendTime = level.time + 500;

		//self->client->ps.velocity[2] = 300;
	}

	if (rolled)
	{
		G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
	}

	return rolled;
}

//[CloakingVehicles]
extern qboolean DrivingCloakableVehicle(gentity_t *self);
extern void G_ToggleVehicleCloak(playerState_t *ps);
//[/CloakingVehicles]


//[FatigueSys]
GAME_INLINE qboolean MeditateCheck( gentity_t * self )
{		
	int	anim;

	if ( self->client->saber[0].meditateAnim != -1 )
	{
		anim = self->client->saber[0].meditateAnim;
	}
	else if ( self->client->saber[1].model[0]
			&& self->client->saber[1].meditateAnim != -1 )
	{
		anim = self->client->saber[1].meditateAnim;
	}
	else
	{
		anim = BOTH_MEDITATE; 
	}

	//we don't use a HANDEXTEND_TAUNT check anymore since 
	//it's not always consistantly on during a meditate taunt.
	if( self->client->ps.torsoAnim == anim && self->client->ps.torsoTimer <= 100)
	{
		return qtrue;
	}
	
	return qfalse;
}
//[/FatigueSys]


//[FatigueSys]
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern void UpdateFatigueFlags( playerState_t *ps );
//[/FatigueSys]


//[Flamethrower]
void Flamethrower_Fire( gentity_t *self );
//[/Flamethrower]
//[Dioxisthrower]
void Dioxisthrower_Fire( gentity_t *self );
//[/Dioxisthrower]
//[Icethrower]
void Icethrower_Fire( gentity_t *self );
//[/Icethrower]
//[DodgeSys]


//extern void UpdateDodgeFlags( playerState_t *ps);								 
//[/DodgeSys]

//[Electroshocker]
void Electroshocker_Fire( gentity_t *self );
//[/Electroshocker]
//[Lasersupport]
void Lasersupport_Fire( gentity_t *self );
//[/Lasersupport]
//[Orbitalstrike]
void Orbitalstrike_Fire( gentity_t *self );
//[/Orbitalstrike]


void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd )
{
	qboolean	usingForce = qfalse;
	int			i, holo, holoregen;
	int			prepower = 0;

	//[FatigueSys]
	int			FatigueTime;
	int			AgilityTime;
	//[/FatigueSys]

	//see if any force powers are running
	if ( !self )
	{
		return;
	}

	if ( !self->client )
	{
		return;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //not a "real" game client, it's a spectator following someone
		return;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	/*
	if (self->client->ps.fd.saberAnimLevel > self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{
		self->client->ps.fd.saberAnimLevel = self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	else if (!self->client->ps.fd.saberAnimLevel)
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	*/
	//The stance in relation to power level is no longer applicable with the crazy new akimbo/staff stances.
	if (!self->client->ps.fd.saberAnimLevel)
	{//racc - don't ever have SS_NONE?
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}

	//[CoOp]
	//don't want this since this overrules ICARUS script settings
	/*
	if (g_gametype.integer != GT_SIEGE)
	{
		if (!(self->client->ps.fd.forcePowersKnown & (1 << FP_LEVITATION)))
		{
			self->client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		}

		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
		}
	}
	*/
	//[/CoOp]

	if (self->client->ps.fd.forcePowerSelected < 0)
	{ //bad
		self->client->ps.fd.forcePowerSelected = 0;
	}

	UpdateFatigueFlags(&self->client->ps);
	//[/FatigueSys]


	if ( ((self->client->sess.selectedFP != self->client->ps.fd.forcePowerSelected) ||
		(self->client->sess.saberLevel != self->client->ps.fd.saberAnimLevel)) &&
		!(self->r.svFlags & SVF_BOT) )
	{
		if (self->client->sess.updateUITime < level.time)
		{ //a bit hackish, but we don't want the client to flood with userinfo updates if they rapidly cycle
		  //through their force powers or saber attack levels

			self->client->sess.selectedFP = self->client->ps.fd.forcePowerSelected;
			self->client->sess.saberLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (!g_LastFrameTime)
	{
		g_LastFrameTime = level.time;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		self->client->ps.zoomFov = 0;
		self->client->ps.zoomMode = 0;
		self->client->ps.zoomLocked = qfalse;
		self->client->ps.zoomTime = 0;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
		self->client->ps.forceHandExtendTime >= level.time)
	{
		self->client->ps.saberMove = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.saberBlocked = 0;
		self->client->ps.weaponTime = 0;
		self->client->ps.weaponstate = WEAPON_READY;
	}
	else if (self->client->ps.forceHandExtend != HANDEXTEND_NONE &&
		self->client->ps.forceHandExtendTime < level.time)
	{
		if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
			!self->client->ps.forceDodgeAnim)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (G_SpecialRollGetup(self))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else
			{ //hmm.. ok.. no more getting up on your own, you've gotta push something, unless..
				if ((level.time-self->client->ps.forceHandExtendTime) > 4000)
				{ //4 seconds elapsed, I guess they're too dumb to push something to get up!
					if (self->client->pers.cmd.upmove &&
						self->client->ps.fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
					{ //force getup
						G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
						self->client->ps.forceDodgeAnim = 2;
						self->client->ps.forceHandExtendTime = level.time + 500;

						//self->client->ps.velocity[2] = 400;
					}
					else if (self->client->ps.quickerGetup)
					{
						G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
						self->client->ps.forceDodgeAnim = 3;
						self->client->ps.forceHandExtendTime = level.time + 500;
						self->client->ps.velocity[2] = 300;
					}
					else
					{
						self->client->ps.forceDodgeAnim = 1;
						self->client->ps.forceHandExtendTime = level.time + 1000;
					}
				}
			}
			self->client->ps.quickerGetup = qfalse;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_POSTTHROWN)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceDodgeAnim = 1;
				self->client->ps.forceHandExtendTime = level.time + 1000;
				G_EntitySound( self, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
				self->client->ps.velocity[2] = 100;
			}
			else if (!self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceHandExtendTime = level.time + 100;
			}
			else
			{
				self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
			}
		}
		//[DodgeSys]
		else if(self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
		{//don't do the HANDEXTEND_WEAPONREADY since it screws up our saber block code.
			self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		}
		//[/DodgeSys]
		else
		{
			self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
		}
	}

	if (g_gametype.integer == GT_HOLOCRON)
	{
		HolocronUpdate(self);
	}
	if (g_gametype.integer == GT_JEDIMASTER)
	{
		JediMasterUpdate(self);
	}

	SeekerDroneUpdate(self);

	if (self->client->ps.powerups[PW_FORCE_BOON])
	{
		prepower = self->client->ps.fd.forcePower;
	}

	if (self && self->client && (BG_HasYsalamiri(g_gametype.integer, &self->client->ps) ||
		self->client->ps.fd.forceDeactivateAll || self->client->tempSpectate >= level.time))
	{ //has ysalamiri.. or we want to forcefully stop all his active powers
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}

		if (self->client->tempSpectate >= level.time)
		{				
			//self->client->ps.fd.forcePower = 10;
			self->client->ps.fd.forceRageRecoveryTime = 0;
		}

		self->client->ps.fd.forceDeactivateAll = 0;

		if (self->client->ps.fd.forceJumpCharge)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}
	else
	{ //otherwise just do a check through them all to see if they need to be stopped for any reason.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION &&
				!BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, i))
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}
	}

	i = 0;

	if (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] || self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK])
	{ //enlightenment
		if (!self->client->ps.fd.forceUsingAdded)
		{
			i = 0;

			while (i < NUM_FORCE_POWERS)
			{
				self->client->ps.fd.forcePowerBaseLevel[i] = self->client->ps.fd.forcePowerLevel[i];
				
/*				//[ExpSys]
				if(
					i == FP_TEAM_HEAL
					|| i == FP_TEAM_FORCE
					)
				{//don't boost the level of Enhanced's disabled force powers.
					i++;
					continue;
				}
*/	
				if(!forcePowerDarkLight[i] 
					|| (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] && forcePowerDarkLight[i] == FORCE_LIGHTSIDE)
					|| (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] && forcePowerDarkLight[i] == FORCE_DARKSIDE))
				//if (!forcePowerDarkLight[i] ||
				//	self->client->ps.fd.forceSide == forcePowerDarkLight[i])
				//[/ExpSys]
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
					self->client->ps.fd.forcePowersKnown |= (1 << i);
				}

				i++;
			}

			self->client->ps.fd.forceUsingAdded = 1;
		}
	}
	else if (self->client->ps.fd.forceUsingAdded)
	{ //we don't have enlightenment but we're still using enlightened powers, so clear them back to how they should be.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			self->client->ps.fd.forcePowerLevel[i] = self->client->ps.fd.forcePowerBaseLevel[i];
			if (!self->client->ps.fd.forcePowerLevel[i])
			{
				if (self->client->ps.fd.forcePowersActive & (1 << i))
				{
					WP_ForcePowerStop(self, i);
				}
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}

			i++;
		}

		self->client->ps.fd.forceUsingAdded = 0;
	}

	i = 0;

	if (!(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)))
	{ //clear the mindtrick index values


		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
	}
	


	if (self->health < 1)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripCripple = 1;
		//keep the saber off during this period
		if (self->client->ps.weapon == WP_SABER && !self->client->ps.saberHolstered)
		{
			Cmd_ToggleSaber_f(self);
		}
	}
	else
	{
		self->client->ps.fd.forceGripCripple = 0;
	}

	if (self->client->ps.fd.forceJumpSound)
	{
		G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		self->client->ps.fd.forceJumpSound = 0;
	}

	if (self->client->ps.fd.forceGripCripple)
	{
		if (self->client->ps.fd.forceGripSoundTime < level.time  )
		{
		if (self->enemy && self->enemy->client && self->enemy->client->skillLevel[SK_GRIPA] == FORCE_LEVEL_2)
		{
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEGRASP);			
		}
		else
		{
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEGRIP);			
		}
			self->client->ps.fd.forceGripSoundTime = level.time + 1000;
		
			
		}

	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{
		self->client->ps.powerups[PW_SPEED] = level.time + 100;
	}

	if ( self->health <= 0 )
	{//if dead, deactivate any active force powers
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( self->client->ps.fd.forcePowerDuration[i] || (self->client->ps.fd.forcePowersActive&( 1 << i )) )
			{
				WP_ForcePowerStop( self, (forcePowers_t)i );
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		goto powersetcheck;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		self->client->fjDidJump = qfalse;
	}

	if (self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->fjDidJump)
	{ //this was for the "charge" jump method... I guess
		if (ucmd->upmove < 10 && (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LEVITATION))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1-50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}

#ifndef METROID_JUMP
	else if ( (ucmd->upmove > 10) && (self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.groundTime && (level.time - self->client->ps.groundTime) > 150 && !BG_HasYsalamiri(g_gametype.integer, &self->client->ps) && BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, FP_LEVITATION) )
	{//just charging up
		ForceJumpCharge( self, ucmd );
		usingForce = qtrue;
	}
	else if (ucmd->upmove < 10 && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.pm_flags &= ~(PMF_JUMP_HELD);
	}
#endif

	if (!(self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.fd.forceJumpCharge)
	{
		if (!(ucmd->buttons & BUTTON_FORCEPOWER) ||
			self->client->ps.fd.forcePowerSelected != FP_LEVITATION)
		{
			if (WP_DoSpecificPower( self, ucmd, FP_LEVITATION ))
			{
				usingForce = qtrue;
			}
		}
	}


	//[Icethrower]
	if(self->client->iceTime > level.time)
	{//icethrower is active, flip active icethrower flag

		if (self->client->ps.jetpackFuel < FLAMETHROWER_FUELCOST)
		{//not enough gas, turn it off.
			self->client->iceTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_THROWER);
			self->client->ps.userInt3 &= ~(1 << FLAG_THROWER2);
		}
		else
		{//fire icethrower
			self->client->ps.userInt3 |= (1 << FLAG_THROWER);
			self->client->ps.userInt3 |= (1 << FLAG_THROWER2);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/iceburst") );
				Icethrower_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.jetpackFuel -= FLAMETHROWER_FUELCOST;
				if (self->client->skillLevel[SK_FLAMETHROWER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_FLAMETHROWER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER];
				}	
			}
		}
	}

	//[/Icethrower]

	//[Dioxisthrower]
	else if(self->client->dioxisTime > level.time)
	{//dioxisthrower is active, flip active dioxisthrower flag

		if (self->client->ps.jetpackFuel < FLAMETHROWER_FUELCOST)
		{//not enough gas, turn it off.
			self->client->dioxisTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_THROWER2);
		}
		else
		{//fire dioxisthrower
			self->client->ps.userInt3 |= (1 << FLAG_THROWER2);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/dioxisburst") );
				Dioxisthrower_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.jetpackFuel -= FLAMETHROWER_FUELCOST;
				if (self->client->skillLevel[SK_FLAMETHROWER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_FLAMETHROWER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER];
				}	
			}
		}
	}


	//[/Dioxisthrower]


	//[Flamethrower]
	else if(self->client->flameTime > level.time)
	{//flamethrower is active, flip active flamethrower flag

		if (self->client->ps.jetpackFuel < FLAMETHROWER_FUELCOST)
		{//not enough gas, turn it off.
			self->client->flameTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_THROWER);
		}
		else
		{//fire flamethrower
			self->client->ps.userInt3 |= (1 << FLAG_THROWER);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/fireburst") );
				Flamethrower_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.jetpackFuel -= FLAMETHROWER_FUELCOST;
				if (self->client->skillLevel[SK_FLAMETHROWER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_FLAMETHROWER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_FLAMETHROWER];
				}	
			}
		}
	}
	else
	{
		//G_Printf("%i: %i: Not using flamethrower\n", level.time, self->s.number);
		self->client->ps.userInt3 &= ~(1 << FLAG_THROWER);
		self->client->ps.userInt3 &= ~(1 << FLAG_THROWER2);
	}
	//[/Flamethrower]


		//[Orbitalstrike]
	if(self->client->orbitalstrikeTime > level.time)
	{//orbitalstrike is active, flip active orbitalstrike flag

		if (self->client->ps.cloakFuel < ELECTROSHOCKER_FUELCOST)
		{//not enough battery, turn it off.
			self->client->orbitalstrikeTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER);
			self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER2);
		}
		else
		{//fire lasersupport
			self->client->ps.userInt3 |= (1 << FLAG_ADVANCEDTHROWER);
			self->client->ps.userInt3 |= (1 << FLAG_ADVANCEDTHROWER2);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/orbitalstrike") );
				Orbitalstrike_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.cloakFuel -= ELECTROSHOCKER_FUELCOST;
				
				if (self->client->skillLevel[SK_ELECTROSHOCKER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_ELECTROSHOCKER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER];
				}	
				
				
				
			}
		}
	}
	
		//[Lasersupport]
	else if(self->client->lasersupportTime > level.time)
	{//Lasersupport is active, flip active Lasersupport flag

		if (self->client->ps.cloakFuel < ELECTROSHOCKER_FUELCOST)
		{//not enough battery, turn it off.
			self->client->lasersupportTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER2);
		}
		else
		{//fire lasersupport
			self->client->ps.userInt3 |= (1 << FLAG_ADVANCEDTHROWER2);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/lasersupport") );
				Lasersupport_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.cloakFuel -= ELECTROSHOCKER_FUELCOST;
				
				if (self->client->skillLevel[SK_ELECTROSHOCKER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_ELECTROSHOCKER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER];
				}	
				
				
				
			}
		}
	}
	
		//[Electroshocker]
	else if(self->client->electroshockerTime > level.time)
	{//Electroshocker is active, flip active Electroshocker flag

		if (self->client->ps.cloakFuel < ELECTROSHOCKER_FUELCOST)
		{//not enough battery, turn it off.
			self->client->electroshockerTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER);
		}
		else
		{//fire electroshocker
			self->client->ps.userInt3 |= (1 << FLAG_ADVANCEDTHROWER);
			self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if( LightningDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - LightningDebounceTime >= LIGHTNINGDEBOUNCE) )
			{
				G_Sound( self, CHAN_WEAPON, G_SoundIndex("sound/effects/Electroshock") );
				Electroshocker_Fire(self);
				LightningDebounceTime = level.time;
				if(!Q_irand(0, 1))
				{
				   G_AddMercBalance(self, 1);
				}
				self->client->ps.cloakFuel -= ELECTROSHOCKER_FUELCOST;
				
				if (self->client->skillLevel[SK_ELECTROSHOCKER] > FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER] + FORCE_LEVEL_3;
				}
				else if (self->client->skillLevel[SK_ELECTROSHOCKER] <= FORCE_LEVEL_2)
				{	
				self->client->ps.activeForcePass = self->client->skillLevel[SK_ELECTROSHOCKER];
				}	
				
				
				
			}
		}
	}
	
	else
	{
		//G_Printf("%i: %i: Not using electroshocker\n", level.time, self->s.number);
		self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER);
		self->client->ps.userInt3 &= ~(1 << FLAG_ADVANCEDTHROWER2);
	}
	
	
	
	if (self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT) )
	{
	if (self->client->ps.userInt3 & (1 << FLAG_PROTECT2))
	{
		DeathfieldBubble(self);			
	}
	else
	{

	}
	}
	
	
	if (self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB) )
	{
	if (self->client->ps.userInt3 & (1 << FLAG_ABSORB2))
	{	
		DeathsightBubble(self);	
	}
	else
	{

	}
	}
	
	if (self->NPC && self->NPC->charmedTime > level.time && self->corruptionactivator && ((!(self->corruptionactivator->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) && self->NPC->charmedTime <= level.time || self->corruptionactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
		{
		npcteam_t	savTeam = self->client->enemyTeam;
		self->client->enemyTeam = self->client->playerTeam;
		self->client->playerTeam = savTeam;
		self->client->leader = NULL;
		self->NPC->charmedTime = 0;
		self->corruptionactivator=NULL;	
		if(self->client->NPC_class == CLASS_SQUADTEAM || self->client->NPC_class == CLASS_SEEKER)
		{
		self->client->leader=self->originalactivator;
		}

		}
	else if (self->client && self->client->corruptedTime > level.time && self->corruptionactivator && ((!(self->corruptionactivator->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) || self->client->corruptedTime <= level.time || self->corruptionactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
		{
		self->client->corruptedTime = 0;
		self->corruptionactivator=NULL;			
		
		}
	

	
	if (self->NPC && self->NPC->confusionTime > level.time && self->confusionactivator && ((!(self->confusionactivator->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) || self->NPC->confusionTime <= level.time || self->confusionactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
		{
		self->NPC->confusionTime = 0;	
		self->confusionactivator=NULL;	
	
		}		
	else if (self->client && self->confusionactivator && ((!(self->confusionactivator->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)) || self->confusionactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
		{
		self->confusionactivator=NULL;				
		}		
	

	

	if(self->client && self->client->insanityTime > level.time && self->insanityactivator  && ((!(self->insanityactivator->client->ps.fd.forcePowersActive & (1 << FP_TEAM_HEAL)) || self->client->insanityTime <= level.time || self->insanityactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
	{
		gentity_t	*tent;
		tent = G_TempEntity(self->r.currentOrigin, EV_FORCE_INSANITY);
		tent->s.owner = self->s.number;
		tent->s.otherEntityNum2 = 1/5000;		
		self->client->insanityTime = 0;
		self->insanityactivator=NULL;		
	
		self->client->ps.legsTimer = self->client->ps.torsoTimer = 0;
	}


	
	if(self->client && self->client->stasisTime > level.time && self->stasisactivator  && ((!(self->stasisactivator->client->ps.fd.forcePowersActive & (1 << FP_TEAM_HEAL)) || self->client->stasisTime <= level.time || self->stasisactivator->client->ps.stats[STAT_HEALTH] <= 0 ) || BG_HasYsalamiri(g_gametype.integer, &self->client->ps)))
	{
		gentity_t	*tent;
		tent = G_TempEntity(self->r.currentOrigin, EV_FORCE_STASIS);
		tent->s.owner = self->s.number;
		tent->s.otherEntityNum2 = 1/5000;				
		self->client->stasisTime = 0;	
		self->stasisactivator=NULL;	

		self->client->ps.userInt1 &= ~LOCK_MOVERIGHT;
		self->client->ps.userInt1 &= ~LOCK_MOVELEFT;
		self->client->ps.userInt1 &= ~LOCK_MOVEFORWARD;
		self->client->ps.userInt1 &= ~LOCK_MOVEBACK;
		self->client->ps.userInt1 &= ~LOCK_MOVEUP;
		self->client->ps.userInt1 &= ~LOCK_MOVEDOWN;
		self->client->ps.userInt1 &= ~LOCK_UP;
		self->client->ps.userInt1 &= ~LOCK_DOWN;
		self->client->ps.userInt1 &= ~LOCK_RIGHT;
		self->client->ps.userInt1 &= ~LOCK_LEFT;	
		self->client->viewLockTime = 0;
		self->client->ps.legsTimer = self->client->ps.torsoTimer = 0;		
	}







	if(self->client->insanityTime > level.time )
	{//stasis is active, flip active frozen flag
		{//fire electroshocker
		G_Damage(self, self, self, NULL, NULL, 1, 0, MOD_FORCE_DARK);
		if( self->s.NPC_class != CLASS_VEHICLE && self->localAnimIndex <= 1 )
		{
		G_SetAnim(self, NULL, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL, self->client->insanityTime);
		G_SetAnim(self, NULL, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, self->client->insanityTime);	
		}
		}
	}
	
	if(self->client->blindingTime > level.time )
	{//stasis is active, flip active frozen flag
		{//fire electroshocker
		if( self->s.NPC_class != CLASS_VEHICLE && self->localAnimIndex <= 1 )
		{
		G_SetAnim(self, NULL, SETANIM_LEGS, BOTH_WIND, SETANIM_FLAG_NORMAL, self->client->blindingTime);
		G_SetAnim(self, NULL, SETANIM_TORSO, BOTH_WIND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, self->client->blindingTime);	
		}
		}
	} 
	
		//[/Make it Jump]
	if (self->client->repulseTime > level.time)
	{	
	G_SetAnim(self, NULL, SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_NORMAL, self->client->repulseTime);
	G_SetAnim(self, NULL, SETANIM_TORSO, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, self->client->repulseTime);		 
	}
	
		if (self->client)
		{
			if(self->client->SquadTeam3 && self->client->SquadTeam3->client && self->client->SquadTeam3->inuse && self->client->SquadTeam3->health > 0 )
			{
			if( self->client->SquadTeam3->NPC->goalEntity && self->client->SquadTeam3->originalactivator  && self->client->SquadTeam3->NPC->goalEntity == self->client->SquadTeam3->originalactivator)
			{
			vec3_t	pt, dir;
			pt[0] = self->client->SquadTeam3->originalactivator->r.currentOrigin[0] + cos( 3.1415/6 ) * 50;
			pt[1] = self->client->SquadTeam3->originalactivator->r.currentOrigin[1] + sin( 3.1415/6 ) * 50;
			pt[2] = self->client->SquadTeam3->originalactivator->r.currentOrigin[2];
			VectorSubtract( pt, self->client->SquadTeam3->r.currentOrigin, dir );
			VectorMA(self->client->SquadTeam3->client->ps.velocity, 0.8f, dir, self->client->SquadTeam3->client->ps.velocity);
			}
			}
			if(self->client->SquadTeam2 && self->client->SquadTeam2->client && self->client->SquadTeam2->inuse && self->client->SquadTeam2->health > 0 )		
			{
			if( self->client->SquadTeam2->NPC->goalEntity && self->client->SquadTeam2->originalactivator && self->client->SquadTeam2->NPC->goalEntity == self->client->SquadTeam2->originalactivator)
			{
			vec3_t	pt, dir;
			pt[0] = self->client->SquadTeam2->originalactivator->r.currentOrigin[0] + cos( 5*3.1415/6 ) * 50;
			pt[1] = self->client->SquadTeam2->originalactivator->r.currentOrigin[1] + sin( 5*3.1415/6 ) * 50;
			pt[2] = self->client->SquadTeam2->originalactivator->r.currentOrigin[2];
			VectorSubtract( pt, self->client->SquadTeam2->r.currentOrigin, dir );
			VectorMA(self->client->SquadTeam2->client->ps.velocity, 0.8f, dir, self->client->SquadTeam2->client->ps.velocity);
			}
			}
			if(self->client->SquadTeam && self->client->SquadTeam->client && self->client->SquadTeam->inuse && self->client->SquadTeam->health > 0 )		
			{
			if (self->client->SquadTeam->NPC->goalEntity && self->client->SquadTeam->originalactivator && self->client->SquadTeam->NPC->goalEntity == self->client->SquadTeam->originalactivator)
			{
			vec3_t	pt, dir;
			pt[0] = self->client->SquadTeam->originalactivator->r.currentOrigin[0] + cos( 9*3.1415/6 ) * 50;
			pt[1] = self->client->SquadTeam->originalactivator->r.currentOrigin[1] + sin( 9*3.1415/6 ) * 50;
			pt[2] = self->client->SquadTeam->originalactivator->r.currentOrigin[2];
			VectorSubtract( pt, self->client->SquadTeam->r.currentOrigin, dir );
			VectorMA(self->client->SquadTeam->client->ps.velocity, 0.8f, dir, self->client->SquadTeam->client->ps.velocity);
			}
			}
		}
	extern void RocketDie(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int mod);
	//[SnapThrow]
	if (BG_CrouchAnim( self->client->ps.legsAnim) == qtrue)
	{
	if (self->client->skillLevel[SK_BACKPACKROCKET] >= FORCE_LEVEL_1 && ucmd->buttons & BUTTON_USE  )
	{
	if( !(self->client->backpackrocketTime > level.time))
		{
	{//player wants to snap throw a rocket
	vec3_t	start;
	vec3_t	dir;
	vec3_t	reposition;
	int 	BACKPACK_ROCKET_DAMAGE=300;
	int		BACKPACK_ROCKET_SIZE = 3;
	int		BACKPACK_ROCKET_SPLASH_DAMAGE = 150;
	int		BACKPACK_ROCKET_SPLASH_RADIUS = 512;
	int 	ROCKET_DELAY = 6000;
	float	vel = 2000.0;

	G_SetAnim(self, &self->client->pers.cmd, SETANIM_LEGS, BOTH_FLIP_F, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
	if (self->client->skillLevel[SK_BACKPACKROCKET] >= FORCE_LEVEL_3)
	{
		BACKPACK_ROCKET_SPLASH_DAMAGE = BACKPACK_ROCKET_SPLASH_DAMAGE * 3;
		BACKPACK_ROCKET_DAMAGE = BACKPACK_ROCKET_DAMAGE * 3;
	}
	else if (self->client->skillLevel[SK_BACKPACKROCKET] == FORCE_LEVEL_2)
	{
		BACKPACK_ROCKET_SPLASH_DAMAGE = BACKPACK_ROCKET_SPLASH_DAMAGE * 2;
		BACKPACK_ROCKET_DAMAGE = BACKPACK_ROCKET_DAMAGE * 2;
	}
	AngleVectors(self->client->ps.viewangles, dir, NULL, NULL);
	VectorNormalize(dir);
	reposition[0] = 0.0;
	reposition[1] = 0.0;
	reposition[2] = 15.0;
	reposition[0] = self->client->renderInfo.eyePoint[0] + reposition[0];
	reposition[1] = self->client->renderInfo.eyePoint[1] + reposition[1];
	reposition[2] = self->client->renderInfo.eyePoint[2] + reposition[2];
	VectorCopy(reposition, start);

	gentity_t	*missile; 
	missile= CreateMissile( start, dir, vel, 10000, self, qfalse );
	
	


	if (self->client->skillLevel[SK_BACKPACKROCKETA] == FORCE_LEVEL_1)
	{
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_THERMAL;
	}
	else if (self->client->skillLevel[SK_BACKPACKROCKETA] == FORCE_LEVEL_2)
	{
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;
	}
	else if (self->client->skillLevel[SK_BACKPACKROCKETA] == FORCE_LEVEL_3)
	{
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_CONCUSSION;
	}	
	else
	{
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_THERMAL;
	}	
	
	

	VectorSet( missile->r.maxs, BACKPACK_ROCKET_SIZE, BACKPACK_ROCKET_SIZE, BACKPACK_ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );


	missile->damage = BACKPACK_ROCKET_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	missile->methodOfDeath = MOD_ROCKET;
	missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	

	missile->clipmask = MASK_SHOT;
//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
	missile->splashDamage = BACKPACK_ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = BACKPACK_ROCKET_SPLASH_RADIUS;
	// we don't want it to bounce forever
	missile->bounceCount = 0;
	self->client->backpackrocketTime = level.time + ROCKET_DELAY;//delay the activation
	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/rocket/fire") );

	}
	}
	}
	}




	if (self->client->skillLevel[SK_SPECIALCHARACTER] >= FORCE_LEVEL_1 )
	{
	Q3_SetInvisible( self->s.number, qtrue );	
	G_MuteSound(self->s.number, CHAN_VOICE);
	if ( !(G_IsRidingVehicle(self)) && self->health > 0)
	{
				if (self->client->specialcharacterSpawn == 0 )
				{
					if (self->client->skillLevel[SK_SPECIALCHARACTER] == FORCE_LEVEL_2 )
					{
					gentity_t* SpecialCharacter;
					SpecialCharacter = NPC_SpawnType(self, "droideka", va("player%iSpecialCharacter", self->s.number), qtrue);
					self->client->specialcharacterSpawn = 1;
					}
					else if (self->client->skillLevel[SK_SPECIALCHARACTER] == FORCE_LEVEL_1 )
					{
					gentity_t* SpecialCharacter;
					SpecialCharacter = NPC_SpawnType(self, "Mark1_Vehicle", va("player%iSpecialCharacter", self->s.number), qtrue);
					self->client->specialcharacterSpawn = 1;
					}
				}
				gentity_t* target;
				trace_t		trace;
				vec3_t		src, dest, vf;
				vec3_t		viewspot;
				VectorCopy(self->client->ps.origin, viewspot);
				viewspot[2] += self->client->ps.viewheight;
				VectorCopy( viewspot, src );
				AngleVectors( self->client->ps.viewangles, vf, NULL, vf );
				VectorMA( src, 128, vf, dest );
				trap_Trace( &trace, src, vec3_origin, vec3_origin, dest, self->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE );
				target = &g_entities[trace.entityNum];				
				if (target && target->m_pVehicle && target->client &&
					target->s.NPC_class == CLASS_VEHICLE )
				{ //if SpecialCharacter is a vehicle then perform appropriate checks
					Vehicle_t* pVeh = target->m_pVehicle;

					if (pVeh->m_pVehicleInfo)
					{
						{ //not belonging to a team, or client is on same team
							pVeh->m_pVehicleInfo->Board(pVeh, (bgEntity_t*)self);
						}						
						//clear the damn button!
						self->client->pers.cmd.buttons &= ~BUTTON_USE;


					}

				}



	}
	

	}
			
	
	self->client->ps.eFlags &= ~EF_FP_OPTION_2;
	self->client->ps.eFlags &= ~EF_HI_OPTION_2;	
	self->client->ps.eFlags &= ~EF_HI_OPTION_3;	
	
	if (self->client->ps.fd.forcePowerSelected == FP_PUSH && self->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_PULL && self->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_HEAL && self->client->skillLevel[SK_HEALA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_HEAL] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_PROTECT && self->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_PROTECT] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_ABSORB && self->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_ABSORB] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_TELEPATHY && self->client->skillLevel[SK_TELEPATHYA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_TEAM_HEAL && self->client->skillLevel[SK_STASISA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_GRIP && self->client->skillLevel[SK_GRIPA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_GRIP] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_LIGHTNING && self->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_DRAIN && self->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_DRAIN] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_RAGE && self->client->skillLevel[SK_RAGEA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_RAGE] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	else if (self->client->ps.fd.forcePowerSelected == FP_TEAM_FORCE && self->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2 && self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}





	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLAMETHROWER && self->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_2 && self->client->skillLevel[SK_FLAMETHROWER] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_2;	
	}
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLAMETHROWER && self->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_3 && self->client->skillLevel[SK_FLAMETHROWER] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_3;	
	}	
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_ELECTROSHOCKER && self->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_2 && self->client->skillLevel[SK_ELECTROSHOCKER] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_2;	
	}
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_ELECTROSHOCKER && self->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_3 && self->client->skillLevel[SK_ELECTROSHOCKER] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_3;	
	}		
	
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_SQUADTEAM && self->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_2 && self->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_2;	
	}	
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_SQUADTEAM && self->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_3 && self->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_3;	
	}		
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_SQUADTEAM && self->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_1 && self->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_2;	
	self->client->ps.eFlags |= EF_HI_OPTION_3;
	}	
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_SQUADTEAM && self->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_2 && self->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_2;	
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}	
	else if(bg_itemlist[self->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_SQUADTEAM && self->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_3 && self->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	self->client->ps.eFlags |= EF_HI_OPTION_3;	
	self->client->ps.eFlags |= EF_FP_OPTION_2;
	}	 
	else
	{
	self->client->ps.eFlags &= ~EF_FP_OPTION_2;
	self->client->ps.eFlags &= ~EF_HI_OPTION_2;	
	self->client->ps.eFlags &= ~EF_HI_OPTION_3;			
	}
	//if(self->client->ps.powerups[PW_SPHERESHIELDED])
	//{
	//	G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/ambience/cairn/cairn_assembly.wav"));
	//}
	
	//if(self->client->ps.powerups[PW_OVERLOADED])
	//{
	//	G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/ambience/cairn/cairn_zap.wav"));
	//}	
	if ( ucmd->buttons & BUTTON_FORCEGRIP )
	{ //grip is one of the powers with its own button.. if it's held, call the specific grip power function.
		if (WP_DoSpecificPower( self, ucmd, FP_GRIP ))
		{
			usingForce = qtrue;
		}
		else
		{ //don't let recharge even if the grip misses if the player still has the button down
			usingForce = qtrue;
		}
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_GRIP)
			{
				WP_ForcePowerStop(self, FP_GRIP);
			}
		}
	}

	if ( ucmd->buttons & BUTTON_FORCE_LIGHTNING )
	{ //lightning
		//[Flamethrower]
		//ItemUse_FlameThrower(self);
		//[/Flamethrower]
		WP_DoSpecificPower(self, ucmd, FP_LIGHTNING);
		usingForce = qtrue;
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LIGHTNING)
			{
				WP_ForcePowerStop(self, FP_LIGHTNING);
			}
		}
	}



	if ( ucmd->buttons & BUTTON_FORCE_DRAIN )
	{ //drain
		WP_DoSpecificPower(self, ucmd, FP_DRAIN);
		usingForce = qtrue;
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_DRAIN)
			{
				WP_ForcePowerStop(self, FP_DRAIN);
			}
		}
	}

	
		if (self->client->ps.fd.forcePowersActive & (1 << FP_TEAM_FORCE))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_TEAM_FORCE)
			{
				WP_ForcePowerStop(self, FP_TEAM_FORCE);
			}
		}



	//[CloakingVehicles]
	if((ucmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.m_iVehicleNum && DrivingCloakableVehicle(self))
	{//player is on a vehicle that can cloak.
		if(!self->client->ps.fd.forceButtonNeedRelease)
		{
			G_ToggleVehicleCloak(&g_entities[self->client->ps.m_iVehicleNum].client->ps);
			self->client->ps.fd.forceButtonNeedRelease = 1;
		}
	}
	else if( (ucmd->buttons & BUTTON_FORCEPOWER) &&
	//if ( (ucmd->buttons & BUTTON_FORCEPOWER) &&
	//[/CloakingVehicles]
		BG_CanUseFPNow(g_gametype.integer, &self->client->ps, level.time, self->client->ps.fd.forcePowerSelected))
	{
		if (self->client->ps.fd.forcePowerSelected == FP_LEVITATION)
		{
			ForceJumpCharge( self, ucmd );
			usingForce = qtrue;
		}
		else if (WP_DoSpecificPower( self, ucmd, self->client->ps.fd.forcePowerSelected ))
		{
			usingForce = qtrue;
		}
		else if (self->client->ps.fd.forcePowerSelected == FP_GRIP)
		{
			usingForce = qtrue;
		}
	}
	else
	{
		self->client->ps.fd.forceButtonNeedRelease = 0;
	}

	for ( i = 0; i < NUM_FORCE_POWERS; i++ )
	{
		if ( self->client->ps.fd.forcePowerDuration[i] )
		{
			if ( self->client->ps.fd.forcePowerDuration[i] < level.time )
			{
				if ( (self->client->ps.fd.forcePowersActive&( 1 << i )) )
				{//turn it off
					WP_ForcePowerStop( self, (forcePowers_t)i );
				}
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		if ( (self->client->ps.fd.forcePowersActive&( 1 << i )) )
		{
			usingForce = qtrue;
			WP_ForcePowerRun( self, (forcePowers_t)i, ucmd );
		}
	}
	if ( self->client->ps.saberInFlight && self->client->ps.saberEntityNum )
	{//don't regen force power while throwing saber
		if ( self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( &g_entities[self->client->ps.saberEntityNum] != NULL && g_entities[self->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR )
			{//fell to the ground and we're trying to pull it back
				usingForce = qtrue;
			}
		}
	}
		   
		   
	if ( !self->client->ps.fd.forcePowersActive || self->client->ps.fd.forcePowersActive == (1 << FP_DRAIN) )
	{//when not using the force, regenerate at 1 point per half second
		//[SaberThrowSys]
		//Saber is going to be gone alot more, better be able to regen without it.
		if (self->client->ps.fd.forcePowerRegenDebounceTime < level.time &&
																							  
   
		//if ( !self->client->ps.saberInFlight && self->client->ps.fd.forcePowerRegenDebounceTime < level.time &&
		//[/SaberThrowSys]
			//[FatigueSys]
			//Don't regen force while attacking with the saber.
			(self->client->ps.weapon != WP_SABER || !BG_SaberInSpecial(self->client->ps.saberMove)) &&
			!BG_SaberAttacking(&self->client->ps) && !BG_SaberInTransitionAny(self->client->ps.saberMove)
			//Don't regen while running
//			&& WalkCheck(self)
			&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)  //can't regen while in the air.
			//(self->client->ps.weapon != WP_SABER || !BG_SaberInSpecial(self->client->ps.saberMove)) )
			//[/FatigueSys]
		{
		while ( self->client->ps.fd.forcePowerRegenDebounceTime < level.time )
		{
			if (g_gametype.integer != GT_HOLOCRON || g_MaxHolocronCarry.value)
			{
				//if (!g_trueJedi.integer || self->client->ps.weapon == WP_SABER)
				//let non-jedi force regen since we're doing a more strict jedi/non-jedi thing... this gives dark jedi something to drain
				
					if (self->client->ps.powerups[PW_FORCE_BOON])
					{
						WP_ForcePowerRegenerate( self, 6 );
					}
					else if (self->client->ps.isJediMaster && g_gametype.integer == GT_JEDIMASTER)
					{
						WP_ForcePowerRegenerate( self, 4 ); //jedi master regenerates 4 times as fast
					}
					else
					{
						WP_ForcePowerRegenerate( self, 1 );
					}
				
				/*
				else if (g_trueJedi.integer && self->client->ps.weapon != WP_SABER)
				{
					self->client->ps.fd.forcePower = 0;
				}
				*/
			}
			else
			{ //regenerate based on the number of holocrons carried
				holoregen = 0;
				holo = 0;
				while (holo < NUM_FORCE_POWERS)
				{
					if (self->client->ps.holocronsCarried[holo])
					{
						holoregen++;
					}
					holo++;
				}

				WP_ForcePowerRegenerate(self, holoregen);
			}

			//[FatigueSys]
			//we're disabled siege specific force regen code since it screws up FP balancing in Enhanced.
			/*
			if (g_gametype.integer == GT_SIEGE)
			{
				//[FatigueSys]
				//removing the siege regen restriction for players carrying an objective since FP is now used for more 
				//than just Force Powers.
				*//*
				if (self->client->holdingObjectiveItem &&
					g_entities[self->client->holdingObjectiveItem].inuse &&
					g_entities[self->client->holdingObjectiveItem].genericValue15)
				{ //1 point per 7 seconds.. super slow
					self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 7000;
				}
				*//*

				if (self->client->siegeClass != -1 &&
				//else if (self->client->siegeClass != -1 &&
				//[/FatigueSys]
					(bgSiegeClasses[self->client->siegeClass].classflags & (1<<CFL_FASTFORCEREGEN)))
				{ //if this is siege and our player class has the fast force regen ability, then recharge with 1/5th the usual delay
					self->client->ps.fd.forcePowerRegenDebounceTime = level.time + (g_forceRegenTime.integer*0.2);
				}
				else
				{
						self->client->ps.fd.forcePowerRegenDebounceTime = level.time + g_forceRegenTime.integer;
				}
			}
			else
			*/
			//[/FatigueSys]
			
			
			/*	if ( g_gametype.integer == GT_POWERDUEL && self->client->sess.duelTeam == DUELTEAM_LONE )
				{
					if ( g_duel_fraglimit.integer )
					{
						//[FatigueSys]
						FatigueTime = (g_forceRegenTime.integer*
							(0.6 + (.3 * (float)self->client->sess.wins / (float)g_duel_fraglimit.integer)));

						//self->client->ps.fd.forcePowerRegenDebounceTime = level.time + (g_forceRegenTime.integer*
						//	(0.6 + (.3 * (float)self->client->sess.wins / (float)g_duel_fraglimit.integer)));
						//[/FatigueSys]
					}
					else
					{
						//[FatigueSys]
						FatigueTime = (g_forceRegenTime.integer*0.7);

						//self->client->ps.fd.forcePowerRegenDebounceTime = level.time + (g_forceRegenTime.integer*0.7);
						//[/FatigueSys]
					}
				}
				else*/
				
					//[FatigueSys]
					FatigueTime = g_forceRegenTime.integer;

					//[/FatigueSys]
				

				//[FatigueSys]
				if(MeditateCheck(self))
				{//more regen rate while meditating
					self->client->ps.fd.forcePowerRegenDebounceTime = level.time + (FatigueTime/5);

					//[Test]
					//simple debugging messages for determining if the meditation regen is working.
					if(d_test.integer != -1 && self->client->ps.clientNum == d_test.integer)
					{
						trap_SendServerCommand( d_test.integer, va("print \"%i: Meditation FP Regen. Next RegenTime: %i\n\"", d_test.integer, FatigueTime/5 ) );
					}
					//[/Test]
				}	
				else	
				{//standard regen
					self->client->ps.fd.forcePowerRegenDebounceTime = level.time + FatigueTime;
					//[Test]
					//simple debugging messages for determining if the meditation regen is working.
					if(d_test.integer != -1 && self->client->ps.clientNum == d_test.integer)
					{
						trap_SendServerCommand( d_test.integer, va("print \"%i: Normal FP Regen. Next RegenTime: %i\n\"", d_test.integer, FatigueTime ) );
					}
					//[/Test]
				}
				
				//[/FatigueSys]
			}
			//[FatigueSys]
			UpdateFatigueFlags(&self->client->ps);
			//[/FatigueSys]
		}
  }
		//[FatigueSys]
		else
		{//add a debounce to force regen if you're doing something that blocks FP regen.
			self->client->ps.fd.forcePowerRegenDebounceTime = level.time + FATIGUE_REGEN_DEBOUNCE;
		}
		//[/FatigueSys]
	
	
	//[DodgeSys]
	if(self->client->DodgeDebounce < level.time  
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saberMove)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time	//not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE //can't regen while in the air.
//		&& WalkCheck(self)
		)
	{
		AgilityTime= g_dodgeRegenTime.integer;	
		if((self->client->ps.fd.forcePower > FATIGUELEVEL_HEAVY)
			&& self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE])
		{//you have enough fatigue to transfer to Dodge
			if(self->client->ps.stats[STAT_MAX_DODGE] - self->client->ps.stats[STAT_DODGE] < DODGE_FATIGUE)
			{
				self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
			}
			else
			{
				self->client->ps.stats[STAT_DODGE] += DODGE_FATIGUE;
			}
			
			//UpdateDodgeFlags(&self->client->ps);

			//self->client->ps.fd.forcePower--;
		}

		self->client->DodgeDebounce = level.time + AgilityTime;
	}
	//[/DodgeSys]

	//[SaberSys]
	/* MP was merged with FP.
	if(self->client->MishapDebounce < level.time  
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saberMove)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time	//not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)  //can't regen while in the air.
	{
		if(self->client->ps.saberAttackChainCount > 0)
		{
			self->client->ps.saberAttackChainCount--;
		}
		
		if(self->client->ps.weapon == WP_SABER)
		{//saberer regens slower since they use MP differently
			if(self->client->ps.fd.saberAnimLevel == SS_MEDIUM)
			{//yellow style is more "centered" and recovers MP faster.
				//self->client->MishapDebounce = level.time + (g_mishapRegenTime.integer*.75);
				//self->client->MishapDebounce/=100*10+self->client->MishapDebounce;
				self->client->MishapDebounce = level.time + (g_mishapRegenTime.integer*.75);
			}
			else
			{
				self->client->MishapDebounce = level.time + g_mishapRegenTime.integer;
			}
		}
		else
		{//gunner regen faster
				self->client->MishapDebounce = level.time + g_mishapRegenTime.integer/5;
		}
	}
	*/
	//[/SaberSys]
	
powersetcheck:

	if (prepower && self->client->ps.fd.forcePower < prepower)
	{
		int dif = ((prepower - self->client->ps.fd.forcePower)/2);
		if (dif < 1)
		{
			dif = 1;
		}

		self->client->ps.fd.forcePower = (prepower-dif);
	}
}

qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )
{
	int	dodgeAnim = -1;

	if ( !self || !self->client || self->health <= 0 )
	{
		return qfalse;
	}
	
	if ( self->client->stasisTime > level.time || self->client->freezeTime > level.time)
	{
		return qfalse;		
	}
	
	if (!g_forceDodge.integer)
	{
		return qfalse;
	}

	if (g_forceDodge.integer != 2)
	{
		if (!(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
		{
			return qfalse;
		}
	}

	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//can't dodge in mid-air
		return qfalse;
	}

	if ( self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE )
	{//in some effect that stops me from moving on my own
		return qfalse;
	}


	if (g_forceDodge.integer == 2)
	{
		if (self->client->ps.fd.forcePowersActive)
		{ //for now just don't let us dodge if we're using a force power at all
			return qfalse;
		}
	}

	if (g_forceDodge.integer == 2)
	{
		if ( !WP_ForcePowerUsable( self, FP_SPEED ) )
		{//make sure we have it and have enough force power
			return qfalse;
		}
	}

	if (g_forceDodge.integer == 2)
	{
		if ( Q_irand( 1, 7 ) > self->client->ps.fd.forcePowerLevel[FP_SPEED] )
		{//more likely to fail on lower force speed level
			return qfalse;
		}
	}
	else
	{
		//We now dodge all the time, but only on level 3
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{//more likely to fail on lower force sight level
			return qfalse;
		}
	}

	switch( hitLoc )
	{
	case HL_NONE:
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
		return qfalse;

	case HL_BACK_RT:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
	case HL_CHEST:
	case HL_WAIST:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodgeAnim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodgeAnim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	default:
		return qfalse;
	}

	if ( dodgeAnim != -1 )
	{
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodgeAnim;
		self->client->ps.forceHandExtendTime = level.time + 300;

		self->client->ps.powerups[PW_SPEEDBURST] = level.time + 100;

		if (g_forceDodge.integer == 2)
		{
			ForceSpeed( self, 500 );
		}
		else
		{
			G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
		}
		return qtrue;
	}
	return qfalse;
}
