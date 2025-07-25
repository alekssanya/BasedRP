#include "g_local.h"
#include "bg_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include "../ghoul2/g2.h"
//[SaberSys]
#include "g_saberbeh.h"
//[/SaberSys]
#define SABER_BOX_SIZE 16.0f
extern bot_state_t *botstates[MAX_CLIENTS];
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold );
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

//[SaberSys]
extern float VectorDistance(vec3_t v1, vec3_t v2);
qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
//[/SaberSys]

//[Melee]
qboolean G_HeavyMelee( gentity_t *attacker );
//[/Melee]

//[DodgeSys]
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
extern int G_GetHitLocation(gentity_t *target, vec3_t ppoint);
//[/DodgeSys]

extern vmCvar_t		g_saberRealisticCombat;
extern vmCvar_t		d_saberSPStyleDamage;
extern vmCvar_t		g_debugSaberLocks;

//[SaberSys]
#ifndef FINAL_BUILD
extern vmCvar_t		g_debugsabercombat;
#endif
//[/SaberSys]

//[DodgeSys]
extern vmCvar_t		g_debugdodge;
//[/DodgeSys]

// nmckenzie: SABER_DAMAGE_WALLS
extern vmCvar_t		g_saberWallDamageScale;

int saberSpinSound = 0;

//would be cleaner if these were renamed to BG_ and proto'd in a header.
qboolean PM_SaberInTransition( int move );
qboolean PM_SaberInDeflect( int move );
qboolean PM_SaberInBrokenParry( int move );
qboolean PM_SaberInBounce( int move );
qboolean BG_SaberInReturn( int move );
qboolean BG_InKnockDownOnGround( playerState_t *ps );
qboolean BG_StabDownAnim( int anim );
qboolean BG_SabersOff( playerState_t *ps );
qboolean BG_SaberInTransitionAny( int move );
qboolean BG_SaberInAttackPure( int move );
qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );
qboolean WP_SaberBladeDoTransitionDamage( saberInfo_t *saber, int bladeNum );

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void WP_SaberRemoveG2Model( gentity_t *saberent );

//[BugFix4]
#ifdef __linux__
#define RAND_MAX 2147483647
#endif
//[/BugFix4]

//[SaberSys]
qboolean ButterFingers(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other, trace_t *tr);


//All the new fancy Saber defines that control saber behavior

//[SaberDefines]
//defines how much bounce a blade has when it is knocked from a player's hands from an impact.
//Only applies when the target is a non-client entity.
#define SABER_ELASTIC_RATIO .5

//Default damage for a thrown saber
#define DAMAGE_THROWN			100

//[SaberSys]
//Default damage for an attack animation
#define DAMAGE_ATTACK			200
//Default damage for an transition animation
#define DAMAGE_TRANSITION		125

#define DODGE_SABERBLOCK		10  //standard dodge cost for blocking a saber.

//#define DODGE_REPEATERBLOCK		5	//the cost of blocking repeater shots is lower since the repeater shoots much faster. 
//EDIT: Slowed way down and blob is way overused so I'm taking this out for now

//This is the amount of DP that a player gains from making a successful parry while low on DP
#define DODGE_LOWDPBOOST		10

//This is the amount of FP that a player gains from making a successful parry while low on FP
#define DODGE_LOWFPBOOST		5

//Saber Behavior
//remember that this is based on a 0-1000 scale

//This is the parry rate for bots (in addition to the normal check) since
//bots don't block intelligently.
//This is multipled by the bot's skill level (which can be 1-5)
#define BOT_PARRYRATE			50

//scaler multipler for mishaps while running.
#define SABBEH_RUN_MODIFIER		2
//[/SaberDefines]
//[/SaberSys]

//[Melee]
//[MeleeDefines]
//Damage for left punch
#define MELEE_SWING1_DAMAGE			10

//Damage for right punch
#define MELEE_SWING2_DAMAGE			10
//[/MeleeDefines]
//[/Melee]


//[DodgeSys]
//[DodgeDefines]

/*  Turned off PreCog for now.
//The Cost of a PreCog (before the fact) Dodging each type of damage.
//-1 = can't dodge.
//Always use the main MOD for a weapon that has splash.
int PreCogDodgeCosts[MOD_MAX] = 
{
	-1,		//MOD_UNKNOWN,
	-1,		//MOD_STUN_BATON,
	-1,		//MOD_MELEE,
	15,		//MOD_SABER,
	-1,		//MOD_BRYAR_PISTOL,
	-1,		//MOD_BRYAR_PISTOL_ALT,
	-1,		//MOD_BLASTER,
	-1,		//MOD_TURBLAST,
	-1,		//MOD_DISRUPTOR,
	-1,		//MOD_DISRUPTOR_SPLASH,
	-1,		//MOD_DISRUPTOR_SNIPER,
	-1,		//MOD_BOWCASTER,
	-1,		//MOD_REPEATER,
	-1,		//MOD_REPEATER_ALT,
	-1,		//MOD_REPEATER_ALT_SPLASH,
	-1,		//MOD_DEMP2,
	-1,		//MOD_DEMP2_ALT,
	-1,		//MOD_FLECHETTE,
	-1,		//MOD_FLECHETTE_ALT_SPLASH,
	10,		//MOD_ROCKET,
	-1,		//MOD_ROCKET_SPLASH,
	20,		//MOD_ROCKET_HOMING,
	-1,		//MOD_ROCKET_HOMING_SPLASH,
	10,		//MOD_THERMAL,
	-1,		//MOD_THERMAL_SPLASH,
	-1,		//MOD_TRIP_MINE_SPLASH,
	-1,		//MOD_TIMED_MINE_SPLASH,
	-1,		//MOD_DET_PACK_SPLASH,
	-1,		//MOD_VEHICLE,
	-1,		//MOD_CONC,
	-1,		//MOD_CONC_ALT,
	-1,		//MOD_FORCE_DARK,
	-1,		//MOD_SENTRY,
	-1,		//MOD_WATER,
	-1,		//MOD_SLIME,
	-1,		//MOD_LAVA,
	-1,		//MOD_CRUSH,
	-1,		//MOD_TELEFRAG,
	-1,		//MOD_FALLING,
	-1,		//MOD_SUICIDE,
	-1,		//MOD_TARGET_LASER,
	-1,		//MOD_TRIGGER_HURT,
	-1		//MOD_TEAM_CHANGE,
	//MOD_MAX
};
*/

//The Cost of Body/Roll Dodging each type of damage.
//-1 = can't dodge.
int BasicDodgeCosts[MOD_MAX] = 
{
	-1,		//MOD_UNKNOWN,
	-1,		//MOD_STUN_BATON,
	-1,		//MOD_MELEE,
	40,		//MOD_SABER,
	10,		//MOD_BRYAR_PISTOL,
	-1,		//MOD_BRYAR_PISTOL_ALT,
	10,		//MOD_BLASTER,
	50,		//MOD_TURBLAST,
	50,		//MOD_DISRUPTOR,
	50,		//MOD_DISRUPTOR_SPLASH,
	50,		//MOD_DISRUPTOR_SNIPER,
	30,		//MOD_BOWCASTER,
	5,		//MOD_REPEATER,
	50,		//MOD_REPEATER_ALT,
	50,		//MOD_REPEATER_ALT_SPLASH,
	10,		//MOD_DEMP2,
	30,		//MOD_DEMP2_ALT,
	5,		//MOD_FLECHETTE,
	30,		//MOD_FLECHETTE_ALT_SPLASH,
	50,		//MOD_ROCKET,
	50,		//MOD_ROCKET_SPLASH,
	50,		//MOD_ROCKET_HOMING,
	50,		//MOD_ROCKET_HOMING_SPLASH,
	50,		//MOD_THERMAL,
	50,		//MOD_THERMAL_SPLASH,
	50,		//MOD_TRIP_MINE_SPLASH,
	50,		//MOD_TIMED_MINE_SPLASH,
	50,		//MOD_DET_PACK_SPLASH,
	-1,		//MOD_VEHICLE,
	50,		//MOD_CONC,
	50,		//MOD_CONC_ALT,
	-1,		//MOD_FORCE_DARK,
	10,		//MOD_SENTRY,
	-1,		//MOD_WATER,
	-1,		//MOD_SLIME,
	-1,		//MOD_LAVA,
	-1,		//MOD_CRUSH,
	-1,		//MOD_TELEFRAG,
	-1,		//MOD_FALLING,
	-1,		//MOD_SUICIDE,
	-1,		//MOD_TARGET_LASER,
	-1,		//MOD_TRIGGER_HURT,
	-1,		//MOD_TEAM_CHANGE,
	//[Asteroids]
	-1,		//MOD_COLLISION,
	-1,		//MOD_VEH_EXPLOSION,
	//[/Asteroids]
	//[SeekerItemNPC]
	10,		//MOD_SEEKER,	//death by player's seeker droid.
	10,		//MOD_INCINERATOR,	//death by player's seeker droid.	
	10,		//MOD_DIOXIS,	//death by player's seeker droid.
	10,		//MOD_FREEZER,	//death by player's seeker droid.
	50,		//MOD_INCINERATOR_EXPLOSION,	//death by player's seeker droid.	
	50,		//MOD_INCINERATOR_EXPLOSION_SPLASH,	//death by player's seeker droid.
	50,		//MOD_DIOXIS_EXPLOSION,	//death by player's seeker droid.	
	50,		//MOD_DIOXIS_EXPLOSION_SPLASH,	//death by player's seeker droid.
	50,		//MOD_FREEZER_EXPLOSION,	//death by player's seeker droid.	
	50,		//MOD_FREEZER_EXPLOSION_SPLASH,	//death by player's seeker droid.
	50,		//MOD_ION_EXPLOSION,	//death by player's seeker droid.	
	50,		//MOD_ION_EXPLOSION_SPLASH,	//death by player's seeker droid.
	-1,		//MOD_FORCE_DESTRUCTION,	//death by player's seeker droid.	
	//[/SeekerItemNPC]
	//MOD_MAX
};

#define DODGE_KICKCOST	10  //The cost nessicary to convert a recieved kick into a backflip
//[/DodgeSys]
//[/DodgeDefines]


float RandFloat(float min, float max) {
//[BugFix4]
//Fixes problem with linux compiles.
	return ((rand() * (max - min)) / (float)RAND_MAX) + min;
//	return ((rand() * (max - min)) / 32768.0F) + min; 
//[/BugFix4]
}

#ifdef DEBUG_SABER_BOX
void	G_DebugBoxLines(vec3_t mins, vec3_t maxs, int duration)
{
	vec3_t start;
	vec3_t end;

	float x = maxs[0] - mins[0];
	float y = maxs[1] - mins[1];

	// top of box
	VectorCopy(maxs, start);
	VectorCopy(maxs, end);
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] -= y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, maxs, 0x00000ff, duration);
	// bottom of box
	VectorCopy(mins, start);
	VectorCopy(mins, end);
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] += y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, mins, 0x00000ff, duration);
}
#endif

//general check for performing certain attacks against others
qboolean G_CanBeEnemy(gentity_t *self, gentity_t *enemy)
{
	if (!self->inuse || !enemy->inuse || !self->client || !enemy->client)
	{
		return qfalse;
	}

	if (self->client->ps.duelInProgress && self->client->ps.duelIndex != enemy->s.number)
	{ //dueling but not with this person
		return qfalse;
	}

	if (enemy->client->ps.duelInProgress && enemy->client->ps.duelIndex != self->s.number)
	{ //other guy dueling but not with me
		return qfalse;
	}

	if (g_gametype.integer < GT_TEAM)
	{ //ok, sure
		return qtrue;
	}

	if (g_friendlyFire.integer)
	{ //if ff on then can inflict damage normally on teammates
		return qtrue;
	}

	if (OnSameTeam(self, enemy))
	{ //ff not on, don't hurt teammates
		return qfalse;
	}

	return qtrue;
}

//This function gets the attack power which is used to decide broken parries,
//knockaways, and numerous other things. It is not directly related to the
//actual amount of damage done, however. -rww
GAME_INLINE int G_SaberAttackPower(gentity_t *ent, qboolean attacking)
{
	int baseLevel;
	assert(ent && ent->client);

	baseLevel = ent->client->ps.fd.saberAnimLevel;

	//Give "medium" strength for the two special stances.
	if (baseLevel == SS_DUAL)
	{
		baseLevel = 2;
	}
	else if (baseLevel == SS_STAFF)
	{
		baseLevel = 2;
	}

	if (attacking)
	{ //the attacker gets a boost to help penetrate defense.
		//General boost up so the individual levels make a bigger difference.
		baseLevel *= 2;

		baseLevel++;

		//Get the "speed" of the swing, roughly, and add more power
		//to the attack based on it.
		if (ent->client->lastSaberStorageTime >= (level.time-50) &&
			ent->client->olderIsValid)
		{
			vec3_t vSub;
			int swingDist;
			int toleranceAmt;

			//We want different "tolerance" levels for adding in the distance of the last swing
			//to the base power level depending on which stance we are using. Otherwise fast
			//would have more advantage than it should since the animations are all much faster.
			switch (ent->client->ps.fd.saberAnimLevel)
			{
			case SS_STRONG:
				toleranceAmt = 8;
				break;
			case SS_MEDIUM:
				toleranceAmt = 16;
				break;
			case SS_FAST:
				toleranceAmt = 24;
				break;
			default: //dual, staff, etc.
				toleranceAmt = 16;
				break;
			}

            VectorSubtract(ent->client->lastSaberBase_Always, ent->client->olderSaberBase, vSub);
			swingDist = (int)VectorLength(vSub);

			while (swingDist > 0)
			{ //I would like to do something more clever. But I suppose this works, at least for now.
				baseLevel++;
				swingDist -= toleranceAmt;
			}
		}

#ifndef FINAL_BUILD
		if (g_saberDebugPrint.integer > 1)
		{
			Com_Printf("Client %i: ATT STR: %i\n", ent->s.number, baseLevel);
		}
#endif
	}

	if ((ent->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)) ||
		(ent->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
	{ //We're very weak when one of our arms is broken
		baseLevel *= 0.3;
	}

	//Cap at reasonable values now.
	if (baseLevel < 1)
	{
		baseLevel = 1;
	}
	else if (baseLevel > 16)
	{
		baseLevel = 16;
	}

	if (g_gametype.integer == GT_POWERDUEL &&
		ent->client->sess.duelTeam == DUELTEAM_LONE)
	{ //get more power then
		return baseLevel*2;
	}
	else if (attacking && g_gametype.integer == GT_SIEGE)
	{ //in siege, saber battles should be quicker and more biased toward the attacker
		return baseLevel*3;
	}
	
	return baseLevel;
}

void WP_DeactivateSaber( gentity_t *self, qboolean clearLength )
{
	if ( !self || !self->client )
	{
		return;
	}
	//keep my saber off!
	if ( !self->client->ps.saberHolstered )
	{
		self->client->ps.saberHolstered = 2;
		/*
		if ( clearLength )
		{
			self->client->ps.SetSaberLength( 0 );
		}
		*/
		//Doens't matter ATM
		if (self->client->saber[0].soundOff)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[0].soundOff);
		}

		if (self->client->saber[1].soundOff &&
			self->client->saber[1].model[0])
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[1].soundOff);
		}

	}
}

void WP_ActivateSaber( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return;
	}

	if (self->NPC &&
		self->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT &&
		(self->client->ps.forceHandExtendTime - level.time) > 200)
	{ //if we're an NPC and in the middle of a taunt then stop it
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = 0;
	}
	else if (self->client->ps.fd.forceGripCripple)
	{ //can't activate saber while being gripped
		return;
	}

	//[StanceSelection]
	//NPC code calls this all the time, only whip out saber if it's not already turned on.
	if ( self->client->ps.saberHolstered == 2)
	//if ( self->client->ps.saberHolstered )
	//[/StanceSelection]
	{
		self->client->ps.saberHolstered = 0;
		if (self->client->saber[0].soundOn)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[0].soundOn);
		}

		if (self->client->saber[1].soundOn)
		{
			G_Sound(self, CHAN_WEAPON, self->client->saber[1].soundOn);
		}
	}
}

#define PROPER_THROWN_VALUE 999 //Ah, well.. 

//[RACC] - This seems to be the think setting for telling the saber to resume it's normal position in the hand.
void SaberUpdateSelf(gentity_t *ent)
{
	if (ent->r.ownerNum == ENTITYNUM_NONE)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (!g_entities[ent->r.ownerNum].inuse ||
		!g_entities[ent->r.ownerNum].client/* ||
		g_entities[ent->r.ownerNum].client->sess.sessionTeam == TEAM_SPECTATOR*/)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (g_entities[ent->r.ownerNum].client->ps.saberInFlight && g_entities[ent->r.ownerNum].health > 0)
	{ //let The Master take care of us now (we'll get treated like a missile until we return)
		ent->nextthink = level.time;
		ent->genericValue5 = PROPER_THROWN_VALUE;
		return;
	}

	ent->genericValue5 = 0;

	if (g_entities[ent->r.ownerNum].client->ps.weapon != WP_SABER ||
		(g_entities[ent->r.ownerNum].client->ps.pm_flags & PMF_FOLLOW) ||
		//RWW ADDED 7-19-03 BEGIN
		g_entities[ent->r.ownerNum].client->sess.sessionTeam == TEAM_SPECTATOR ||
		g_entities[ent->r.ownerNum].client->tempSpectate >= level.time ||
		//RWW ADDED 7-19-03 END
		g_entities[ent->r.ownerNum].health < 1 ||
		BG_SabersOff( &g_entities[ent->r.ownerNum].client->ps ) ||
		(!g_entities[ent->r.ownerNum].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] && g_entities[ent->r.ownerNum].s.eType != ET_NPC))
	{ //owner is not using saber, spectating, dead, saber holstered, or has no attack level
		ent->r.contents = 0;
		ent->clipmask = 0;
	}
	else
	{ //Standard contents (saber is active)
#ifdef DEBUG_SABER_BOX
		if (g_saberDebugBox.integer == 1|| g_saberDebugBox.integer == 4)
		{
			vec3_t dbgMins;
			vec3_t dbgMaxs;

			VectorAdd( ent->r.currentOrigin, ent->r.mins, dbgMins );
			VectorAdd( ent->r.currentOrigin, ent->r.maxs, dbgMaxs );

			G_DebugBoxLines(dbgMins, dbgMaxs, (10.0f/(float)g_svfps.integer)*100);
		}
#endif
		if (ent->r.contents != CONTENTS_LIGHTSABER)
		{
			if ((level.time - g_entities[ent->r.ownerNum].client->lastSaberStorageTime) <= 200)
			{ //Only go back to solid once we're sure our owner has updated recently
				ent->r.contents = CONTENTS_LIGHTSABER;
				ent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
			}
		}
		else
		{
			ent->r.contents = CONTENTS_LIGHTSABER;
			ent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
		}
	}

	trap_LinkEntity(ent);

	ent->nextthink = level.time;
}

void SaberGotHit( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *own = &g_entities[self->r.ownerNum];

	if (!own || !own->client)
	{
		return;
	}

	//Do something here..? Was handling projectiles here, but instead they're now handled in their own functions.
}

qboolean BG_SuperBreakLoseAnim( int anim );

GAME_INLINE void SetSaberBoxSize(gentity_t *saberent)
{
	gentity_t *owner = NULL;
	vec3_t saberOrg, saberTip;
	int i;
	int j = 0;
	int k = 0;
	qboolean dualSabers = qfalse;
	qboolean alwaysBlock[MAX_SABERS][MAX_BLADES];
	qboolean forceBlock = qfalse;

	assert(saberent && saberent->inuse);

	if (saberent->r.ownerNum < MAX_CLIENTS && saberent->r.ownerNum >= 0)
	{
		owner = &g_entities[saberent->r.ownerNum];
	}
	else if (saberent->r.ownerNum >= 0 && saberent->r.ownerNum < ENTITYNUM_WORLD &&
		g_entities[saberent->r.ownerNum].s.eType == ET_NPC)
	{
		owner = &g_entities[saberent->r.ownerNum];
	}

	if (!owner || !owner->inuse || !owner->client)
	{
		assert(!"Saber with no owner?");
		return;
	}

	if (  owner->client->saber[1].model[0] )
	{
		dualSabers = qtrue;
	}

	if ( PM_SaberInBrokenParry(owner->client->ps.saberMove)
		|| BG_SuperBreakLoseAnim( owner->client->ps.torsoAnim ) )
	{ //let swings go right through when we're in this state
		for ( i = 0; i < MAX_SABERS; i++ )
		{
			if ( i > 0 && !dualSabers )
			{//not using a second saber, set it to not blocking
				for ( j = 0; j < MAX_BLADES; j++ )
				{
					alwaysBlock[i][j] = qfalse;
				}
			}
			else
			{
				if ( (owner->client->saber[i].saberFlags2&SFL2_ALWAYS_BLOCK) )
				{
					for ( j = 0; j < owner->client->saber[i].numBlades; j++ )
					{
						alwaysBlock[i][j] = qtrue;
						forceBlock = qtrue;
					}
				}
				if ( owner->client->saber[i].bladeStyle2Start > 0 )
				{
					for ( j = owner->client->saber[i].bladeStyle2Start; j < owner->client->saber[i].numBlades; j++ )
					{
						if ( (owner->client->saber[i].saberFlags2&SFL2_ALWAYS_BLOCK2) )
						{
							alwaysBlock[i][j] = qtrue;
							forceBlock = qtrue;
						}
						else
						{
							alwaysBlock[i][j] = qfalse;
						}
					}
				}
			}
		}
		if ( !forceBlock )
		{//no sabers/blades to FORCE to be on, so turn off blocking altogether
			VectorSet( saberent->r.mins, 0, 0, 0 );
			VectorSet( saberent->r.maxs, 0, 0, 0 );
#ifndef FINAL_BUILD
			if (g_saberDebugPrint.integer > 1)
			{
				Com_Printf("Client %i in broken parry, saber box 0\n", owner->s.number);
			}
#endif
			return;
		}
	}

	if ((level.time - owner->client->lastSaberStorageTime) > 200 ||
		(level.time - owner->client->saber[j].blade[k].storageTime) > 100)
	{ //it's been too long since we got a reliable point storage, so use the defaults and leave.
		VectorSet( saberent->r.mins, -SABER_BOX_SIZE, -SABER_BOX_SIZE, -SABER_BOX_SIZE );
		VectorSet( saberent->r.maxs, SABER_BOX_SIZE, SABER_BOX_SIZE, SABER_BOX_SIZE );
		return;
	}

	if ( dualSabers
		|| owner->client->saber[0].numBlades > 1 )
	{//dual sabers or multi-blade saber
		if ( owner->client->ps.saberHolstered > 1 )
		{//entirely off
			//no blocking at all
			VectorSet( saberent->r.mins, 0, 0, 0 );
			VectorSet( saberent->r.maxs, 0, 0, 0 );
			return;
		}
	}
	else
	{//single saber
		if ( owner->client->ps.saberHolstered )
		{//off
			//no blocking at all
			VectorSet( saberent->r.mins, 0, 0, 0 );
			VectorSet( saberent->r.maxs, 0, 0, 0 );
			return;
		}
	}
	//Start out at the saber origin, then go through all the blades and push out the extents
	//for each blade, then set the box relative to the origin.
	VectorCopy(saberent->r.currentOrigin, saberent->r.mins);
	VectorCopy(saberent->r.currentOrigin, saberent->r.maxs);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < MAX_SABERS; j++)
		{
			if (!owner->client->saber[j].model[0])
			{
				break;
			}
			if ( dualSabers
				&& owner->client->ps.saberHolstered == 1 
				&& j == 1 )
			{ //this mother is holstered, get outta here.
				j++;
				continue;
			}
			for (k = 0; k < owner->client->saber[j].numBlades; k++)
			{
				if ( k > 0 )
				{//not the first blade
					if ( !dualSabers )
					{//using a single saber
						if ( owner->client->saber[j].numBlades > 1 )
						{//with multiple blades
							if( owner->client->ps.saberHolstered == 1 )
							{//all blades after the first one are off
								break;
							}
						}
					}
				}			
				if ( forceBlock )
				{//only do blocking with blades that are marked to block
					if ( !alwaysBlock[j][k] )
					{//this blade shouldn't be blocking
						continue;
					}
				}
				//VectorMA(owner->client->saber[j].blade[k].muzzlePoint, owner->client->saber[j].blade[k].lengthMax*0.5f, owner->client->saber[j].blade[k].muzzleDir, saberOrg);
				VectorCopy(owner->client->saber[j].blade[k].muzzlePoint, saberOrg);
				VectorMA(owner->client->saber[j].blade[k].muzzlePoint, owner->client->saber[j].blade[k].lengthMax, owner->client->saber[j].blade[k].muzzleDir, saberTip);

				if (saberOrg[i] < saberent->r.mins[i])
				{
					saberent->r.mins[i] = saberOrg[i];
				}
				if (saberTip[i] < saberent->r.mins[i])
				{
					saberent->r.mins[i] = saberTip[i];
				}

				if (saberOrg[i] > saberent->r.maxs[i])
				{
					saberent->r.maxs[i] = saberOrg[i];
				}
				if (saberTip[i] > saberent->r.maxs[i])
				{
					saberent->r.maxs[i] = saberTip[i];
				}

				//G_TestLine(saberOrg, saberTip, 0x0000ff, 50);
			}
		}
	}

	VectorSubtract(saberent->r.mins, saberent->r.currentOrigin, saberent->r.mins);
	VectorSubtract(saberent->r.maxs, saberent->r.currentOrigin, saberent->r.maxs);
}

void WP_SaberInitBladeData( gentity_t *ent )
{
	gentity_t *saberent = NULL;
	gentity_t *checkEnt;
	int i = 0;
	
	//G_Printf("WP_SaberInitBladeData called\n");
	while (i < level.num_entities)
	{ //make sure there are no other saber entities floating around that think they belong to this client.
		checkEnt = &g_entities[i];

		if (checkEnt->inuse && checkEnt->neverFree &&
			checkEnt->r.ownerNum == ent->s.number &&
			checkEnt->classname && checkEnt->classname[0] &&
			!Q_stricmp(checkEnt->classname, "lightsaber"))
		{
			if (saberent)
			{ //already have one
				checkEnt->neverFree = qfalse;
				checkEnt->think = G_FreeEntity;
				checkEnt->nextthink = level.time;
			}
			else
			{ //hmm.. well then, take it as my own.
				//free the bitch but don't issue a kg2 to avoid overflowing clients.
				checkEnt->s.modelGhoul2 = 0;
				G_FreeEntity(checkEnt);

				//now init it manually and reuse this ent slot.
				G_InitGentity(checkEnt);
				saberent = checkEnt;
			}
		}

		i++;
	}

	//We do not want the client to have any real knowledge of the entity whatsoever. It will only
	//ever be used on the server.
	if (!saberent)
	{ //ok, make one then
		saberent = G_Spawn();
	}
	ent->client->ps.saberEntityNum = ent->client->saberStoredIndex = saberent->s.number;
	
	saberent->classname = "lightsaber";
	
	saberent->neverFree = qtrue; //the saber being removed would be a terrible thing.

	saberent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	saberent->r.ownerNum = ent->s.number;

	saberent->clipmask = MASK_PLAYERSOLID | CONTENTS_LIGHTSABER;
	saberent->r.contents = CONTENTS_LIGHTSABER;

	SetSaberBoxSize(saberent);

	saberent->mass = 10;

	saberent->s.eFlags |= EF_NODRAW;
	saberent->r.svFlags |= SVF_NOCLIENT;

	saberent->s.modelGhoul2 = 1;
	//should we happen to be removed (we belong to an NPC and he is removed) then
	//we want to attempt to remove our g2 instance on the client in case we had one.

	saberent->touch = SaberGotHit;

	saberent->think = SaberUpdateSelf;
	saberent->genericValue5 = 0;
	saberent->nextthink = level.time + 50;

	saberSpinSound = G_SoundIndex("sound/weapons/saber/saberspin.wav");
}

#define LOOK_DEFAULT_SPEED	0.15f
#define LOOK_TALKING_SPEED	0.15f	

GAME_INLINE qboolean G_CheckLookTarget( gentity_t *ent, vec3_t	lookAngles, float *lookingSpeed )
{
	//FIXME: also clamp the lookAngles based on the clamp + the existing difference between
	//		headAngles and torsoAngles?  But often the tag_torso is straight but the torso itself
	//		is deformed to not face straight... sigh...

	
	if (ent->s.eType == ET_NPC &&
		ent->s.m_iVehicleNum &&
		ent->s.NPC_class != CLASS_VEHICLE )
	{ //an NPC bolted to a vehicle should just look around randomly
		if ( TIMER_Done( ent, "lookAround" ) )
		{
			ent->NPC->shootAngles[YAW] = flrand(0,360);
			TIMER_Set( ent, "lookAround", Q_irand( 500, 3000 ) );
		}
		VectorSet( lookAngles, 0, ent->NPC->shootAngles[YAW], 0 );
		return qtrue;
	}
	//Now calc head angle to lookTarget, if any
	if ( ent->client->renderInfo.lookTarget >= 0 && ent->client->renderInfo.lookTarget < ENTITYNUM_WORLD )
	{
		vec3_t	lookDir, lookOrg, eyeOrg;
		int i;

		if ( ent->client->renderInfo.lookMode == LM_ENT )
		{
			gentity_t	*lookCent = &g_entities[ent->client->renderInfo.lookTarget];
			if ( lookCent )
			{
				if (lookCent->client && lookCent->client->ps.powerups[PW_CLOAKED] )
					return qfalse;  					 
				
				if ( lookCent != ent->enemy )
				{//We turn heads faster than headbob speed, but not as fast as if watching an enemy
					*lookingSpeed = LOOK_DEFAULT_SPEED;
				}

				//FIXME: Ignore small deltas from current angles so we don't bob our head in synch with theirs?

				/*
				if ( ent->client->renderInfo.lookTarget == 0 && !cg.renderingThirdPerson )//!cg_thirdPerson.integer )
				{//Special case- use cg.refdef.vieworg if looking at player and not in third person view
					VectorCopy( cg.refdef.vieworg, lookOrg );
				}
				*/ //No no no!
				if ( lookCent->client )
				{
					VectorCopy( lookCent->client->renderInfo.eyePoint, lookOrg );
				}
				else if ( lookCent->inuse && !VectorCompare( lookCent->r.currentOrigin, vec3_origin ) )
				{
					VectorCopy( lookCent->r.currentOrigin, lookOrg );
				}
				else
				{//at origin of world
					return qfalse;
				}
				//Look in dir of lookTarget
			}
		}
		else if ( ent->client->renderInfo.lookMode == LM_INTEREST && ent->client->renderInfo.lookTarget > -1 && ent->client->renderInfo.lookTarget < MAX_INTEREST_POINTS )
		{
			gentity_t	*lookCent = &g_entities[ent->client->renderInfo.lookTarget];
			//[OpenRP - Don't turn head when cloaked player is next to you]	  	
			if ( lookCent->client->ps.powerups[PW_CLOAKED] )
					return qfalse;
			//[/OpenRP - Don't turn head when cloaked player is next to you]
			VectorCopy( level.interestPoints[ent->client->renderInfo.lookTarget].origin, lookOrg );
		}
		else
		{
			return qfalse;
		}

		VectorCopy( ent->client->renderInfo.eyePoint, eyeOrg );

		VectorSubtract( lookOrg, eyeOrg, lookDir );

		vectoangles( lookDir, lookAngles );

		for ( i = 0; i < 3; i++ )
		{
			lookAngles[i] = AngleNormalize180( lookAngles[i] );
			ent->client->renderInfo.eyeAngles[i] = AngleNormalize180( ent->client->renderInfo.eyeAngles[i] );
		}
		AnglesSubtract( lookAngles, ent->client->renderInfo.eyeAngles, lookAngles );
		return qtrue;
	}

	return qfalse;
}

//rww - attempted "port" of the SP version which is completely client-side and
//uses illegal gentity access. I am trying to keep this from being too
//bandwidth-intensive.
//This is primarily droid stuff I guess, I'm going to try to handle all humanoid
//NPC stuff in with the actual player stuff if possible.
void NPC_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles);
GAME_INLINE void G_G2NPCAngles(gentity_t *ent, vec3_t legs[3], vec3_t angles)
{
	char *craniumBone = "cranium";
	char *thoracicBone = "thoracic"; //only used by atst so doesn't need a case
	qboolean looking = qfalse;
	vec3_t viewAngles;
	vec3_t lookAngles;

	if ( ent->client )
	{
		if ( (ent->client->NPC_class == CLASS_PROBE ) 
			|| (ent->client->NPC_class == CLASS_R2D2 ) 
			|| (ent->client->NPC_class == CLASS_R5D2) 
			|| (ent->client->NPC_class == CLASS_ATST) )
		{
			vec3_t	trailingLegsAngles;

			if (ent->s.eType == ET_NPC &&
				ent->s.m_iVehicleNum &&
				ent->s.NPC_class != CLASS_VEHICLE )
			{ //an NPC bolted to a vehicle should use the full angles
				VectorCopy(ent->r.currentAngles, angles);
			}
			else
			{
				VectorCopy( ent->client->ps.viewangles, angles );
				angles[PITCH] = 0;
			}

			//FIXME: use actual swing/clamp tolerances?
			/*
			if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//on the ground
				CG_PlayerLegsYawFromMovement( cent, ent->client->ps.velocity, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
			}
			else
			{//face legs to front
				CG_PlayerLegsYawFromMovement( cent, vec3_origin, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
			}
			*/

			VectorCopy( ent->client->ps.viewangles, viewAngles );
	//			viewAngles[YAW] = viewAngles[ROLL] = 0;
			viewAngles[PITCH] *= 0.5;
			VectorCopy( viewAngles, lookAngles );

			lookAngles[1] = 0;

			if ( ent->client->NPC_class == CLASS_ATST )
			{//body pitch
				NPC_SetBoneAngles(ent, thoracicBone, lookAngles);
				//BG_G2SetBoneAngles( cent, ent, ent->thoracicBone, lookAngles, BONE_ANGLES_POSTMULT,POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
			}

			VectorCopy( viewAngles, lookAngles );

			if ( ent && ent->client && ent->client->NPC_class == CLASS_ATST )
			{
				//CG_ATSTLegsYaw( cent, trailingLegsAngles );
				AnglesToAxis( trailingLegsAngles, legs );
			}
			else
			{
				//FIXME: this needs to properly set the legs.yawing field so we don't erroneously play the turning anim, but we do play it when turning in place
				/*
				if ( angles[YAW] == cent->pe.legs.yawAngle )
				{
					cent->pe.legs.yawing = qfalse;
				}
				else
				{
					cent->pe.legs.yawing = qtrue;
				}

				cent->pe.legs.yawAngle = angles[YAW];
				if ( ent->client )
				{
					ent->client->renderInfo.legsYaw = angles[YAW];
				}
				AnglesToAxis( angles, legs );
				*/
			}

	//			if ( ent && ent->client && ent->client->NPC_class == CLASS_ATST )
	//			{
	//				looking = qfalse;
	//			}
	//			else
			{	//look at lookTarget!
				//FIXME: snaps to side when lets go of lookTarget... ?
				float	lookingSpeed = 0.3f;
				looking = G_CheckLookTarget( ent, lookAngles, &lookingSpeed );
				lookAngles[PITCH] = lookAngles[ROLL] = 0;//droids can't pitch or roll their heads
				if ( looking )
				{//want to keep doing this lerp behavior for a full second after stopped looking (so don't snap)
					ent->client->renderInfo.lookingDebounceTime = level.time + 1000;
				}
			}
			if ( ent->client->renderInfo.lookingDebounceTime > level.time )
			{	//adjust for current body orientation
				vec3_t	oldLookAngles;

				lookAngles[YAW] -= 0;//ent->client->ps.viewangles[YAW];//cent->pe.torso.yawAngle;
				//lookAngles[YAW] -= cent->pe.legs.yawAngle;

				//normalize
				lookAngles[YAW] = AngleNormalize180( lookAngles[YAW] );

				//slowly lerp to this new value
				//Remember last headAngles
				VectorCopy( ent->client->renderInfo.lastHeadAngles, oldLookAngles );
				if( VectorCompare( oldLookAngles, lookAngles ) == qfalse )
				{
					//FIXME: This clamp goes off viewAngles,
					//but really should go off the tag_torso's axis[0] angles, no?
					lookAngles[YAW] = oldLookAngles[YAW]+(lookAngles[YAW]-oldLookAngles[YAW])*0.4f;
				}
				//Remember current lookAngles next time
				VectorCopy( lookAngles, ent->client->renderInfo.lastHeadAngles );
			}
			else
			{//Remember current lookAngles next time
				VectorCopy( lookAngles, ent->client->renderInfo.lastHeadAngles );
			}
			if ( ent->client->NPC_class == CLASS_ATST )
			{
				VectorCopy( ent->client->ps.viewangles, lookAngles );
				lookAngles[0] = lookAngles[2] = 0;
				lookAngles[YAW] -= trailingLegsAngles[YAW];
			}
			else
			{
				lookAngles[PITCH] = lookAngles[ROLL] = 0;
				lookAngles[YAW] -= ent->client->ps.viewangles[YAW];
			}

			NPC_SetBoneAngles(ent, craniumBone, lookAngles);
			//BG_G2SetBoneAngles( cent, ent, ent->craniumBone, lookAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw); 
			//return;
		}
		else//if ( (ent->client->NPC_class == CLASS_GONK ) || (ent->client->NPC_class == CLASS_INTERROGATOR) || (ent->client->NPC_class == CLASS_SENTRY) )
		{
		//	VectorCopy( ent->client->ps.viewangles, angles );
		//	AnglesToAxis( angles, legs );
			//return;
		}
	}
}

GAME_INLINE void G_G2PlayerAngles( gentity_t *ent, vec3_t legs[3], vec3_t legsAngles)
{
	qboolean tPitching = qfalse,
			 tYawing = qfalse,
			 lYawing = qfalse;
	float tYawAngle = ent->client->ps.viewangles[YAW],
		  tPitchAngle = 0,
		  lYawAngle = ent->client->ps.viewangles[YAW];

	int ciLegs = ent->client->ps.legsAnim;
	int ciTorso = ent->client->ps.torsoAnim;

	vec3_t turAngles;
	vec3_t lerpOrg, lerpAng;

	if (ent->s.eType == ET_NPC && ent->client)
	{ //sort of hacky, but it saves a pretty big load off the server
		int i = 0;
		gentity_t *clEnt;

		//If no real clients are in the same PVS then don't do any of this stuff, no one can see him anyway!
		while (i < MAX_CLIENTS)
		{
			clEnt = &g_entities[i];

			if (clEnt && clEnt->inuse && clEnt->client &&
				trap_InPVS(clEnt->client->ps.origin, ent->client->ps.origin))
			{ //this client can see him
				break;
			}

			i++;
		}

		if (i == MAX_CLIENTS)
		{ //no one can see him, just return
			return;
		}
	}

	VectorCopy(ent->client->ps.origin, lerpOrg);
	VectorCopy(ent->client->ps.viewangles, lerpAng);

	if (ent->localAnimIndex <= 1)
	{ //don't do these things on non-humanoids
		vec3_t lookAngles;
		entityState_t *emplaced = NULL;

		if (ent->client->ps.hasLookTarget)
		{
			VectorSubtract(g_entities[ent->client->ps.lookTarget].r.currentOrigin, ent->client->ps.origin, lookAngles);
			vectoangles(lookAngles, lookAngles);
			ent->client->lookTime = level.time + 1000;
		}
		else
		{
			VectorCopy(ent->client->ps.origin, lookAngles);
		}
		lookAngles[PITCH] = 0;

		if (ent->client->ps.emplacedIndex)
		{
			emplaced = &g_entities[ent->client->ps.emplacedIndex].s;
		}

		BG_G2PlayerAngles(ent->ghoul2, ent->client->renderInfo.motionBolt, &ent->s, level.time, lerpOrg, lerpAng, legs,
			legsAngles, &tYawing, &tPitching, &lYawing, &tYawAngle, &tPitchAngle, &lYawAngle, FRAMETIME, turAngles,
			ent->modelScale, ciLegs, ciTorso, &ent->client->corrTime, lookAngles, ent->client->lastHeadAngles,
			ent->client->lookTime, emplaced, NULL);

		if (ent->client->ps.heldByClient && ent->client->ps.heldByClient <= MAX_CLIENTS)
		{ //then put our arm in this client's hand
			//is index+1 because index 0 is valid.
			int heldByIndex = ent->client->ps.heldByClient-1;
			gentity_t *other = &g_entities[heldByIndex];
			int lHandBolt = 0;

			if (other && other->inuse && other->client && other->ghoul2)
			{
				lHandBolt = trap_G2API_AddBolt(other->ghoul2, 0, "*l_hand");
			}
			else
			{ //they left the game, perhaps?
				ent->client->ps.heldByClient = 0;
				return;
			}

			if (lHandBolt)
			{
				mdxaBone_t boltMatrix;
				vec3_t boltOrg;
				vec3_t tAngles;

				VectorCopy(other->client->ps.viewangles, tAngles);
				tAngles[PITCH] = tAngles[ROLL] = 0;

				trap_G2API_GetBoltMatrix(other->ghoul2, 0, lHandBolt, &boltMatrix, tAngles, other->client->ps.origin, level.time, 0, other->modelScale);
				boltOrg[0] = boltMatrix.matrix[0][3];
				boltOrg[1] = boltMatrix.matrix[1][3];
				boltOrg[2] = boltMatrix.matrix[2][3];

				BG_IK_MoveArm(ent->ghoul2, lHandBolt, level.time, &ent->s, ent->client->ps.torsoAnim/*BOTH_DEAD1*/, boltOrg, &ent->client->ikStatus,
					ent->client->ps.origin, ent->client->ps.viewangles, ent->modelScale, 500, qfalse);
			}
		}
		else if (ent->client->ikStatus)
		{ //make sure we aren't IKing if we don't have anyone to hold onto us.
			int lHandBolt = 0;

			if (ent && ent->inuse && ent->client && ent->ghoul2)
			{
				lHandBolt = trap_G2API_AddBolt(ent->ghoul2, 0, "*l_hand");
			}
			else
			{ //This shouldn't happen, but just in case it does, we'll have a failsafe.
				ent->client->ikStatus = qfalse;
			}

			if (lHandBolt)
			{
				BG_IK_MoveArm(ent->ghoul2, lHandBolt, level.time, &ent->s,
					ent->client->ps.torsoAnim/*BOTH_DEAD1*/, vec3_origin, &ent->client->ikStatus, ent->client->ps.origin, ent->client->ps.viewangles, ent->modelScale, 500, qtrue);
			}
		}
	}
	else if ( ent->m_pVehicle && ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
	{
		vec3_t lookAngles;

		VectorCopy(ent->client->ps.viewangles, legsAngles);
		legsAngles[PITCH] = 0;
		AnglesToAxis( legsAngles, legs );

		VectorCopy(ent->client->ps.viewangles, lookAngles);
		lookAngles[YAW] = lookAngles[ROLL] = 0;

		BG_G2ATSTAngles( ent->ghoul2, level.time, lookAngles );
	}
	else if (ent->NPC)
	{ //an NPC not using a humanoid skeleton, do special angle stuff.
		if (ent->s.eType == ET_NPC &&
			ent->s.NPC_class == CLASS_VEHICLE &&
			ent->m_pVehicle &&
			ent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{ //fighters actually want to take pitch and roll into account for the axial angles
			VectorCopy(ent->client->ps.viewangles, legsAngles);
			AnglesToAxis( legsAngles, legs );
		}
		else
		{
			G_G2NPCAngles(ent, legs, legsAngles);
		}
	}
}

GAME_INLINE qboolean SaberAttacking(gentity_t *self)
{
	if (PM_SaberInParry(self->client->ps.saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBrokenParry(self->client->ps.saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInDeflect(self->client->ps.saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBounce(self->client->ps.saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInKnockaway(self->client->ps.saberMove))
	{
		return qfalse;
	}

	if (BG_SaberInAttack(self->client->ps.saberMove))
	{
		if (self->client->ps.weaponstate == WEAPON_FIRING && self->client->ps.saberBlocked == BLOCKED_NONE)
		{ //if we're firing and not blocking, then we're attacking.
			return qtrue;
		}
	}

	if (BG_SaberInSpecial(self->client->ps.saberMove))
	{
		return qtrue;
	}

	return qfalse;
}

typedef enum
{
	LOCK_FIRST = 0,
	LOCK_TOP = LOCK_FIRST,
	LOCK_DIAG_TR,
	LOCK_DIAG_TL,
	LOCK_DIAG_BR,
	LOCK_DIAG_BL,
	LOCK_R,
	LOCK_L,
	LOCK_RANDOM
} sabersLockMode_t;

#define LOCK_IDEAL_DIST_TOP 32.0f
#define LOCK_IDEAL_DIST_CIRCLE 48.0f

#define SABER_HITDAMAGE 100
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );

int G_SaberLockAnim( int attackerSaberStyle, int defenderSaberStyle, int topOrSide, int lockOrBreakOrSuperBreak, int winOrLose )
{
	int baseAnim = -1;
	if ( lockOrBreakOrSuperBreak == SABERLOCK_LOCK )
	{//special case: if we're using the same style and locking
		if ( attackerSaberStyle == defenderSaberStyle  //racc - using same style
			|| (attackerSaberStyle>=SS_FAST&&attackerSaberStyle<=SS_TAVION&&defenderSaberStyle>=SS_FAST&&defenderSaberStyle<=SS_TAVION) ) //racc - or using single saber
		{//using same style 
			if ( winOrLose == SABERLOCK_LOSE )
			{//you want the defender's stance...
				switch ( defenderSaberStyle )
				{
				case SS_DUAL:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_DL_DL_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_DL_DL_S_L_2;
					}
					break;
				case SS_STAFF:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_ST_ST_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_ST_ST_S_L_2;
					}
					break;
				default:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_S_S_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_S_S_S_L_2;
					}
					break;
				}
			}
		}
	}
	if ( baseAnim == -1 )
	{
		switch ( attackerSaberStyle )
		{
		case SS_DUAL:
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_DL_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_DL_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_DL_S_S_B_1_L;
					break;
			}
			break;
		case SS_STAFF:
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_ST_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_ST_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_ST_S_S_B_1_L;
					break;
			}
			break;
		default://single
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_S_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_S_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_S_S_S_B_1_L;
					break;
			}
			break;
		}
		//side lock or top lock?
		if ( topOrSide == SABERLOCK_TOP )
		{
			baseAnim += 5;
		}
		//lock, break or superbreak?
		if ( lockOrBreakOrSuperBreak == SABERLOCK_LOCK )
		{
			baseAnim += 2;
		}
		else 
		{//a break or superbreak
			if ( lockOrBreakOrSuperBreak == SABERLOCK_SUPERBREAK )
			{
				baseAnim += 3;
			}
			//winner or loser?
			if ( winOrLose == SABERLOCK_WIN )
			{
				baseAnim += 1;
			}
		}
	}
	return baseAnim;
}

extern qboolean BG_CheckIncrementLockAnim( int anim, int winOrLose ); //bg_saber.c
#define LOCK_IDEAL_DIST_JKA 46.0f//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN

GAME_INLINE qboolean WP_SabersCheckLock2( gentity_t *attacker, gentity_t *defender, sabersLockMode_t lockMode )
{
	int		attAnim, defAnim = 0;
	float	attStart = 0.5f, defStart = 0.5f;
	float	idealDist = 48.0f;
	vec3_t	attAngles, defAngles, defDir;
	vec3_t	newOrg;
	vec3_t	attDir;
	float	diff = 0;
	trace_t trace;

	//MATCH ANIMS
	if ( lockMode == LOCK_RANDOM )
	{
		lockMode = (sabersLockMode_t)Q_irand( (int)LOCK_FIRST, (int)(LOCK_RANDOM)-1 );
	}
	if ( attacker->client->ps.fd.saberAnimLevel >= SS_FAST
		&& attacker->client->ps.fd.saberAnimLevel <= SS_TAVION
		&& defender->client->ps.fd.saberAnimLevel >= SS_FAST
		&& defender->client->ps.fd.saberAnimLevel <= SS_TAVION )
	{//2 single sabers?  Just do it the old way...
		switch ( lockMode )
		{
		case LOCK_TOP:
			attAnim = BOTH_BF2LOCK;
			defAnim = BOTH_BF1LOCK;
			attStart = defStart = 0.5f;
			idealDist = LOCK_IDEAL_DIST_TOP;
			break;
		case LOCK_DIAG_TR:
			attAnim = BOTH_CCWCIRCLELOCK;
			defAnim = BOTH_CWCIRCLELOCK;
			attStart = defStart = 0.5f;
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		case LOCK_DIAG_TL:
			attAnim = BOTH_CWCIRCLELOCK;
			defAnim = BOTH_CCWCIRCLELOCK;
			attStart = defStart = 0.5f;
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		case LOCK_DIAG_BR:
			//[SaberLockSys]
			//Saberlocking in this direction didn't look correct.
			attAnim = BOTH_CCWCIRCLELOCK;
			defAnim = BOTH_CWCIRCLELOCK;
			attStart = defStart = 0.15f;
			//attAnim = BOTH_CWCIRCLELOCK;
			//defAnim = BOTH_CCWCIRCLELOCK;
			//attStart = defStart = 0.85f;
			//[/SaberLockSys]
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		case LOCK_DIAG_BL:
			//[SaberLockSys]
			//Saberlocking in this direction didn't look correct.
			attAnim = BOTH_CWCIRCLELOCK;
			defAnim = BOTH_CCWCIRCLELOCK;
			attStart = defStart = 0.15f;
			//attAnim = BOTH_CCWCIRCLELOCK;
			//defAnim = BOTH_CWCIRCLELOCK;
			//attStart = defStart = 0.85f;
			//[/SaberLockSys]
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		case LOCK_R:
			attAnim = BOTH_CCWCIRCLELOCK;
			defAnim = BOTH_CWCIRCLELOCK;
			//[SaberLockSys]
			//the starting position wasn't correct for the lock direction.
			attStart = defStart = 0.25f;
			//attStart = defStart = 0.75f;
			//[/SaberLockSys]
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		case LOCK_L:
			attAnim = BOTH_CWCIRCLELOCK;
			defAnim = BOTH_CCWCIRCLELOCK;
			//[SaberLockSys]
			//the starting position wasn't correct for the lock direction.
			attStart = defStart = 0.25f;
			//attStart = defStart = 0.75f;
			//[/SaberLockSys]
			idealDist = LOCK_IDEAL_DIST_CIRCLE;
			break;
		default:
			return qfalse;
			break;
		}
	}
	else
	{//use the new system
		idealDist = LOCK_IDEAL_DIST_JKA;//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN
		if ( lockMode == LOCK_TOP )
		{//top lock
			attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_TOP, SABERLOCK_LOCK, SABERLOCK_WIN );
			defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_TOP, SABERLOCK_LOCK, SABERLOCK_LOSE );
			attStart = defStart = 0.5f;
		}
		else
		{//side lock
			switch ( lockMode )
			{
			case LOCK_DIAG_TR:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				attStart = defStart = 0.5f;
				break;
			case LOCK_DIAG_TL:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				attStart = defStart = 0.5f;
				break;
			case LOCK_DIAG_BR:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				if ( BG_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
				{
					attStart = 0.85f;//move to end of anim
				}
				else
				{
					attStart = 0.15f;//start at beginning of anim
				}
				if ( BG_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
				{
					defStart = 0.85f;//start at end of anim
				}
				else
				{
					defStart = 0.15f;//start at beginning of anim
				}
				break;
			case LOCK_DIAG_BL:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				if ( BG_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
				{
					attStart = 0.85f;//move to end of anim
				}
				else
				{
					attStart = 0.15f;//start at beginning of anim
				}
				if ( BG_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
				{
					defStart = 0.85f;//start at end of anim
				}
				else
				{
					defStart = 0.15f;//start at beginning of anim
				}
				break;
			case LOCK_R:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				if ( BG_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
				{
					attStart = 0.75f;//move to end of anim
				}
				else
				{
					attStart = 0.25f;//start at beginning of anim
				}
				if ( BG_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
				{
					defStart = 0.75f;//start at end of anim
				}
				else
				{
					defStart = 0.25f;//start at beginning of anim
				}
				break;
			case LOCK_L:
				attAnim = G_SaberLockAnim( attacker->client->ps.fd.saberAnimLevel, defender->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
				defAnim = G_SaberLockAnim( defender->client->ps.fd.saberAnimLevel, attacker->client->ps.fd.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
				//attacker starts with advantage
				if ( BG_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
				{
					attStart = 0.75f;//move to end of anim
				}
				else
				{
					attStart = 0.25f;//start at beginning of anim
				}
				if ( BG_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
				{
					defStart = 0.75f;//start at end of anim
				}
				else
				{
					defStart = 0.25f;//start at beginning of anim
				}
				break;
			default:
				return qfalse;
				break;
			}
		}
	}

	G_SetAnim(attacker, NULL, SETANIM_BOTH, attAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	attacker->client->ps.saberLockFrame = bgAllAnims[attacker->localAnimIndex].anims[attAnim].firstFrame+(bgAllAnims[attacker->localAnimIndex].anims[attAnim].numFrames*attStart);

	G_SetAnim(defender, NULL, SETANIM_BOTH, defAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	defender->client->ps.saberLockFrame = bgAllAnims[defender->localAnimIndex].anims[defAnim].firstFrame+(bgAllAnims[defender->localAnimIndex].anims[defAnim].numFrames*defStart);

	attacker->client->ps.saberLockHits = 0;
	defender->client->ps.saberLockHits = 0;

	attacker->client->ps.saberLockAdvance = qfalse;
	defender->client->ps.saberLockAdvance = qfalse;

	VectorClear( attacker->client->ps.velocity );
	VectorClear( defender->client->ps.velocity );
	attacker->client->ps.saberLockTime = defender->client->ps.saberLockTime = level.time + 10000;
	attacker->client->ps.saberLockEnemy = defender->s.number;
	defender->client->ps.saberLockEnemy = attacker->s.number;
	attacker->client->ps.weaponTime = defender->client->ps.weaponTime = Q_irand( 1000, 3000 );//delay 1 to 3 seconds before pushing

	//racc - FYI, the PM_lightsaber code sets .saberMoves to LS_NONE the next time it's run on these players.

	//[SaberLockSys]
	//remove the attack fake flag from both players since the saberlock code operates outside the standard saber animation control.
	//Without this, we were having trouble with cascading saberlocks when the saberlocks are cause directly by attack fakes.
	attacker->client->ps.userInt3 &= ~( 1 << FLAG_ATTACKFAKE );
	defender->client->ps.userInt3 &= ~( 1 << FLAG_ATTACKFAKE );
	//[/SaberLockSys]

	VectorSubtract( defender->r.currentOrigin, attacker->r.currentOrigin, defDir );
	VectorCopy( attacker->client->ps.viewangles, attAngles );
	attAngles[YAW] = vectoyaw( defDir );
	SetClientViewAngle( attacker, attAngles );
	defAngles[PITCH] = attAngles[PITCH]*-1;
	defAngles[YAW] = AngleNormalize180( attAngles[YAW] + 180);
	defAngles[ROLL] = 0;
	SetClientViewAngle( defender, defAngles );
	
	//MATCH POSITIONS
	diff = VectorNormalize( defDir ) - idealDist;//diff will be the total error in dist
	//try to move attacker half the diff towards the defender
	VectorMA( attacker->r.currentOrigin, diff*0.5f, defDir, newOrg );

	trap_Trace( &trace, attacker->r.currentOrigin, attacker->r.mins, attacker->r.maxs, newOrg, attacker->s.number, attacker->clipmask);
	if ( !trace.startsolid && !trace.allsolid )
	{
		G_SetOrigin( attacker, trace.endpos );
		if (attacker->client)
		{
			VectorCopy(trace.endpos, attacker->client->ps.origin);
		}
		trap_LinkEntity( attacker );
	}
	//now get the defender's dist and do it for him too
	VectorSubtract( attacker->r.currentOrigin, defender->r.currentOrigin, attDir );
	diff = VectorNormalize( attDir ) - idealDist;//diff will be the total error in dist
	//try to move defender all of the remaining diff towards the attacker
	VectorMA( defender->r.currentOrigin, diff, attDir, newOrg );
	trap_Trace( &trace, defender->r.currentOrigin, defender->r.mins, defender->r.maxs, newOrg, defender->s.number, defender->clipmask);
	if ( !trace.startsolid && !trace.allsolid )
	{
		if (defender->client)
		{
			VectorCopy(trace.endpos, defender->client->ps.origin);
		}
		G_SetOrigin( defender, trace.endpos );
		trap_LinkEntity( defender );
	}

	//DONE!
	return qtrue;
}


//[SaberLockSys]
//racc - redesigned most of WP_SabersCheckLock function.
extern qboolean BG_InSlowBounce(playerState_t *ps);
extern saberMoveData_t	saberMoveData[LS_MOVE_MAX];
qboolean WP_SabersCheckLock( gentity_t *ent1, gentity_t *ent2 )
{
	float dist;
	int	lockQuad;

	if ( g_debugSaberLocks.integer )
	{
		WP_SabersCheckLock2( ent1, ent2, LOCK_RANDOM );
		return qtrue;
	}

	if (!g_saberLocking.integer)
	{
		return qfalse;
	}

	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}

	if (ent1->s.eType == ET_NPC ||
		ent2->s.eType == ET_NPC)
	{ //if either ents is NPC, then never let an NPC lock with someone on the same playerTeam
		if (ent1->client->playerTeam == ent2->client->playerTeam)
		{
			return qfalse;
		}
	}

	if (!ent1->client->ps.saberEntityNum ||
		!ent2->client->ps.saberEntityNum ||
		ent1->client->ps.saberInFlight ||
		ent2->client->ps.saberInFlight)
	{ //can't get in lock if one of them has had the saber knocked out of his hand
		return qfalse;
	}

	if ( fabs( ent1->r.currentOrigin[2]-ent2->r.currentOrigin[2] ) > 16 )
	{
		return qfalse;
	}
	if ( ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}
	dist = DistanceSquared(ent1->r.currentOrigin,ent2->r.currentOrigin);
	if ( dist < 64 || dist > 6400 )
	{//between 8 and 80 from each other
		return qfalse;
	}

	if (BG_InSpecialJump(ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (BG_InSpecialJump(ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (BG_InRoll(&ent1->client->ps, ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (BG_InRoll(&ent2->client->ps, ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (ent1->client->ps.forceHandExtend != HANDEXTEND_NONE ||
		ent2->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return qfalse;
	}

	if ((ent1->client->ps.pm_flags & PMF_DUCKED) ||
		(ent2->client->ps.pm_flags & PMF_DUCKED))
	{
		return qfalse;
	}

	if ( (ent1->client->saber[0].saberFlags&SFL_NOT_LOCKABLE)
		|| (ent2->client->saber[0].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}
	if (  ent1->client->saber[1].model[0]
		&& !ent1->client->ps.saberHolstered
		&& (ent1->client->saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}
	if (  ent2->client->saber[1].model[0]
		&& !ent2->client->ps.saberHolstered
		&& (ent2->client->saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}

	//don't allow saberlocks while a player is in a slow bounce.  This was allowing players to spam attack fakes/saberlocks
	//and never let their opponent get out of a slow bounce.
	if(BG_InSlowBounce(&ent1->client->ps) || BG_InSlowBounce(&ent2->client->ps))
	{
		return qfalse;
	}

	if (!InFront( ent1->client->ps.origin, ent2->client->ps.origin, ent2->client->ps.viewangles, 0.4f ))
	{
		return qfalse;
	}
	if (!InFront( ent2->client->ps.origin, ent1->client->ps.origin, ent1->client->ps.viewangles, 0.4f ))
	{
		return qfalse;
	}

	if(PM_SaberInParry(ent1->client->ps.saberMove)  //parries should always use their end quad
		/*|| BG_GetTorsoAnimPoint(&ent1->client->ps, ent1->localAnimIndex) < .5*/)
	{//use the endquad of the move
		lockQuad = saberMoveData[ent1->client->ps.saberMove].endQuad;
	}
	else
	{//use the startquad of the move
		lockQuad = saberMoveData[ent1->client->ps.saberMove].startQuad;
	}

	switch(lockQuad)
	{
		case Q_BR:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
			break;
		case Q_R:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
			break;
		case Q_TR:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
			break;
		case Q_T:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_TOP );
			break;
		case Q_TL:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
			break;
		case Q_L:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
			break;
		case Q_BL:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
			break;
		case Q_B:
			return WP_SabersCheckLock2( ent1, ent2, LOCK_TOP );
			break;
		default:
			//this shouldn't happen.  just wing it
			return qfalse;
			break;
	};
}


/* original basejka code 
qboolean WP_SabersCheckLock( gentity_t *ent1, gentity_t *ent2 )
{
	float dist;
	qboolean	ent1BlockingPlayer = qfalse;
	qboolean	ent2BlockingPlayer = qfalse;

	if ( g_debugSaberLocks.integer )
	{
		WP_SabersCheckLock2( ent1, ent2, LOCK_RANDOM );
		return qtrue;
	}
	//for now.. it's not fair to the lone duelist.
	//we need dual saber lock animations.
	if (g_gametype.integer == GT_POWERDUEL)
	{
		return qfalse;
	}

	if (!g_saberLocking.integer)
	{
		return qfalse;
	}

	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}

	if (ent1->s.eType == ET_NPC ||
		ent2->s.eType == ET_NPC)
	{ //if either ents is NPC, then never let an NPC lock with someone on the same playerTeam
		if (ent1->client->playerTeam == ent2->client->playerTeam)
		{
			return qfalse;
		}
	}

	if (!ent1->client->ps.saberEntityNum ||
		!ent2->client->ps.saberEntityNum ||
		ent1->client->ps.saberInFlight ||
		ent2->client->ps.saberInFlight)
	{ //can't get in lock if one of them has had the saber knocked out of his hand
		return qfalse;
	}

	if (ent1->s.eType != ET_NPC && ent2->s.eType != ET_NPC)
	{ //can always get into locks with NPCs
		if (!ent1->client->ps.duelInProgress ||
			!ent2->client->ps.duelInProgress ||
			ent1->client->ps.duelIndex != ent2->s.number ||
			ent2->client->ps.duelIndex != ent1->s.number)
		{ //only allow saber locking if two players are dueling with each other directly
			if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL)
			{//racc - you're always dueling each other in the duel gametypes.
				return qfalse;
			}
		}
	}

	if ( fabs( ent1->r.currentOrigin[2]-ent2->r.currentOrigin[2] ) > 16 )
	{
		return qfalse;
	}
	if ( ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}
	dist = DistanceSquared(ent1->r.currentOrigin,ent2->r.currentOrigin);
	if ( dist < 64 || dist > 6400 )
	{//between 8 and 80 from each other
		return qfalse;
	}

	if (BG_InSpecialJump(ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (BG_InSpecialJump(ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (BG_InRoll(&ent1->client->ps, ent1->client->ps.legsAnim))
	{
		return qfalse;
	}
	if (BG_InRoll(&ent2->client->ps, ent2->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (ent1->client->ps.forceHandExtend != HANDEXTEND_NONE ||
		ent2->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return qfalse;
	}

	if ((ent1->client->ps.pm_flags & PMF_DUCKED) ||
		(ent2->client->ps.pm_flags & PMF_DUCKED))
	{
		return qfalse;
	}

	if ( (ent1->client->saber[0].saberFlags&SFL_NOT_LOCKABLE)
		|| (ent2->client->saber[0].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}
	if ( ent1->client->saber[1].model
		&& ent1->client->saber[1].model[0]
		&& !ent1->client->ps.saberHolstered
		&& (ent1->client->saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}
	if ( ent2->client->saber[1].model
		&& ent2->client->saber[1].model[0]
		&& !ent2->client->ps.saberHolstered
		&& (ent2->client->saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{
		return qfalse;
	}

	if (!InFront( ent1->client->ps.origin, ent2->client->ps.origin, ent2->client->ps.viewangles, 0.4f ))
	{
		return qfalse;
	}
	if (!InFront( ent2->client->ps.origin, ent1->client->ps.origin, ent1->client->ps.viewangles, 0.4f ))
	{
		return qfalse;
	}

	//T to B lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A7_T__B_)
	{//ent1 is attacking top-down
		return WP_SabersCheckLock2( ent1, ent2, LOCK_TOP );
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A7_T__B_)
	{//ent2 is attacking top-down
		return WP_SabersCheckLock2( ent2, ent1, LOCK_TOP );
	}

	if ( ent1->s.number == 0 &&
		ent1->client->ps.saberBlocking == BLK_WIDE && ent1->client->ps.weaponTime <= 0 )
	{
		ent1BlockingPlayer = qtrue;
	}
	if ( ent2->s.number == 0 &&
		ent2->client->ps.saberBlocking == BLK_WIDE && ent2->client->ps.weaponTime <= 0 )
	{
		ent2BlockingPlayer = qtrue;
	}

	//TR to BL lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A7_TR_BL)
	{//ent1 is attacking diagonally
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A6_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A7_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
		}
		return qfalse;
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A7_TR_BL)
	{//ent2 is attacking diagonally
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A6_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A7_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BL );
		}
		return qfalse;
	}

	//TL to BR lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A7_TL_BR)
	{//ent1 is attacking diagonally
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A6_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A7_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
		}
		return qfalse;
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A7_TL_BR)
	{//ent2 is attacking diagonally
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A6_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A7_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BR );
		}
		return qfalse;
	}
	//L to R lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A7__L__R)
	{//ent1 is attacking l to r
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		return qfalse;
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A7__L__R)
	{//ent2 is attacking l to r
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		return qfalse;
	}
	//R to L lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A7__R__L)
	{//ent1 is attacking r to l
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		return qfalse;
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A7__R__L)
	{//ent2 is attacking r to l
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		return qfalse;
	}
	if ( !Q_irand( 0, 10 ) )
	{
		return WP_SabersCheckLock2( ent1, ent2, LOCK_RANDOM );
	}
	return qfalse;
}
*/
//[/SaberLockSys]


GAME_INLINE int G_GetParryForBlock(int block)
{
	switch (block)
	{
		case BLOCKED_UPPER_RIGHT:
			return LS_PARRY_UR;
			break;
		case BLOCKED_UPPER_RIGHT_PROJ:
			return LS_REFLECT_UR;
			break;
		case BLOCKED_UPPER_LEFT:
			return LS_PARRY_UL;
			break;
		case BLOCKED_UPPER_LEFT_PROJ:
			return LS_REFLECT_UL;
			break;
		case BLOCKED_LOWER_RIGHT:
			return LS_PARRY_LR;
			break;
		case BLOCKED_LOWER_RIGHT_PROJ:
			return LS_REFLECT_LR;
			break;
		case BLOCKED_LOWER_LEFT:
			return LS_PARRY_LL;
			break;
		case BLOCKED_LOWER_LEFT_PROJ:
			return LS_REFLECT_LL;
			break;
		case BLOCKED_TOP:
			return LS_PARRY_UP;
			break;
		case BLOCKED_TOP_PROJ:
			return LS_REFLECT_UP;
			break;
		default:
			break;
	}

	return LS_NONE;
}

int PM_SaberBounceForAttack( int move );
int PM_SaberDeflectionForQuad( int quad );

extern stringID_table_t animTable[MAX_ANIMATIONS+1];
//[RACC] - Determines the deflection animation for an attacking player.  Returns true if it 
//find and set a deflection animation for the situation, returns qfalse for bad situations
//or situations where the saber was bounced instead of deflected.
GAME_INLINE qboolean WP_GetSaberDeflectionAngle( gentity_t *attacker, gentity_t *defender, float saberHitFraction )
{
	qboolean animBasedDeflection = qtrue;
	int attSaberLevel, defSaberLevel;

	if ( !attacker || !attacker->client || !attacker->ghoul2 )
	{
		return qfalse;
	}
	if ( !defender || !defender->client || !defender->ghoul2 )
	{
		return qfalse;
	}

	if ((level.time - attacker->client->lastSaberStorageTime) > 500)
	{ //last update was too long ago, something is happening to this client to prevent his saber from updating
		return qfalse;
	}
	if ((level.time - defender->client->lastSaberStorageTime) > 500)
	{ //ditto
		return qfalse;
	}

	attSaberLevel = G_SaberAttackPower(attacker, SaberAttacking(attacker));
	defSaberLevel = G_SaberAttackPower(defender, SaberAttacking(defender));

	if ( animBasedDeflection )
	{
		//Hmm, let's try just basing it off the anim
		int attQuadStart = saberMoveData[attacker->client->ps.saberMove].startQuad;
		int attQuadEnd = saberMoveData[attacker->client->ps.saberMove].endQuad;
		int defQuad = saberMoveData[defender->client->ps.saberMove].endQuad;
		int quadDiff = fabs((float)(defQuad-attQuadStart));

		if ( defender->client->ps.saberMove == LS_READY )
		{
			//FIXME: we should probably do SOMETHING here...
			//I have this return qfalse here in the hopes that
			//the defender will pick a parry and the attacker
			//will hit the defender's saber again.
			//But maybe this func call should come *after*
			//it's decided whether or not the defender is
			//going to parry.
			return qfalse;
		}

		//reverse the left/right of the defQuad because of the mirrored nature of facing each other in combat
		switch ( defQuad )
		{
		case Q_BR:
			defQuad = Q_BL;
			break;
		case Q_R:
			defQuad = Q_L;
			break;
		case Q_TR:
			defQuad = Q_TL;
			break;
		case Q_TL:
			defQuad = Q_TR;
			break;
		case Q_L:
			defQuad = Q_R;
			break;
		case Q_BL:
			defQuad = Q_BR;
			break;
		}

		if ( quadDiff > 4 )
		{//wrap around so diff is never greater than 180 (4 * 45)
			quadDiff = 4 - (quadDiff - 4);
		}
		//have the quads, find a good anim to use
		if ( (!quadDiff || (quadDiff == 1 && Q_irand(0,1))) //defender pretty much stopped the attack at a 90 degree angle
			&& (defSaberLevel == attSaberLevel || Q_irand( 0, defSaberLevel-attSaberLevel ) >= 0) )//and the defender's style is stronger
		{
			//bounce straight back
#ifndef FINAL_BUILD
			int attMove = attacker->client->ps.saberMove;
#endif
			attacker->client->ps.saberMove = PM_SaberBounceForAttack( attacker->client->ps.saberMove );
#ifndef FINAL_BUILD
			if (g_saberDebugPrint.integer)
			{
				Com_Printf( "attack %s vs. parry %s bounced to %s\n", 
					animTable[saberMoveData[attMove].animToUse].name, 
					animTable[saberMoveData[defender->client->ps.saberMove].animToUse].name,
					animTable[saberMoveData[attacker->client->ps.saberMove].animToUse].name );
			}
#endif
			attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			return qfalse;
		}
		else
		{//attack hit at an angle, figure out what angle it should bounce off att
			int newQuad;
			quadDiff = defQuad - attQuadEnd;
			//add half the diff of between the defense and attack end to the attack end
			if ( quadDiff > 4 )
			{
				quadDiff = 4 - (quadDiff - 4);
			}
			else if ( quadDiff < -4 )
			{
				quadDiff = -4 + (quadDiff + 4);
			}
			newQuad = attQuadEnd + ceil( ((float)quadDiff)/2.0f );
			if ( newQuad < Q_BR )
			{//less than zero wraps around
				newQuad = Q_B + newQuad;
			}
			if ( newQuad == attQuadStart )
			{//never come off at the same angle that we would have if the attack was not interrupted
				if ( Q_irand(0, 1) )
				{
					newQuad--;
				}
				else
				{
					newQuad++;
				}
				if ( newQuad < Q_BR )
				{
					newQuad = Q_B;
				}
				else if ( newQuad > Q_B )
				{
					newQuad = Q_BR;
				}
			}
			if ( newQuad == defQuad )
			{//bounce straight back
#ifndef FINAL_BUILD
				int attMove = attacker->client->ps.saberMove;
#endif
				attacker->client->ps.saberMove = PM_SaberBounceForAttack( attacker->client->ps.saberMove );
#ifndef FINAL_BUILD
				if (g_saberDebugPrint.integer)
				{
					Com_Printf( "attack %s vs. parry %s bounced to %s\n", 
						animTable[saberMoveData[attMove].animToUse].name, 
						animTable[saberMoveData[defender->client->ps.saberMove].animToUse].name,
						animTable[saberMoveData[attacker->client->ps.saberMove].animToUse].name );
				}
#endif
				attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
				return qfalse;
			}
			//else, pick a deflection
			else
			{
#ifndef FINAL_BUILD
				int attMove = attacker->client->ps.saberMove;
#endif
				attacker->client->ps.saberMove = PM_SaberDeflectionForQuad( newQuad );
#ifndef FINAL_BUILD
				if (g_saberDebugPrint.integer)
				{
					Com_Printf( "attack %s vs. parry %s deflected to %s\n", 
						animTable[saberMoveData[attMove].animToUse].name, 
						animTable[saberMoveData[defender->client->ps.saberMove].animToUse].name,
						animTable[saberMoveData[attacker->client->ps.saberMove].animToUse].name );
				}
#endif
				attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
				return qtrue;
			}
		}
	}
	else
	{ //old math-based method (probably broken)
		vec3_t	att_HitDir, def_BladeDir, temp;
		float	hitDot;

		VectorCopy(attacker->client->lastSaberBase_Always, temp);

		AngleVectors(attacker->client->lastSaberDir_Always, att_HitDir, 0, 0);

		AngleVectors(defender->client->lastSaberDir_Always, def_BladeDir, 0, 0);

		//now compare
		hitDot = DotProduct( att_HitDir, def_BladeDir );
		if ( hitDot < 0.25f && hitDot > -0.25f )
		{//hit pretty much perpendicular, pop straight back
			attacker->client->ps.saberMove = PM_SaberBounceForAttack( attacker->client->ps.saberMove );
			attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			return qfalse;
		}
		else 
		{//a deflection
			vec3_t	att_Right, att_Up, att_DeflectionDir;
			float	swingRDot, swingUDot;

			//get the direction of the deflection
			VectorScale( def_BladeDir, hitDot, att_DeflectionDir );
			//get our bounce straight back direction
			VectorScale( att_HitDir, -1.0f, temp );
			//add the bounce back and deflection
			VectorAdd( att_DeflectionDir, temp, att_DeflectionDir );
			//normalize the result to determine what direction our saber should bounce back toward
			VectorNormalize( att_DeflectionDir );

			//need to know the direction of the deflectoin relative to the attacker's facing
			VectorSet( temp, 0, attacker->client->ps.viewangles[YAW], 0 );//presumes no pitch!
			AngleVectors( temp, NULL, att_Right, att_Up );
			swingRDot = DotProduct( att_Right, att_DeflectionDir );
			swingUDot = DotProduct( att_Up, att_DeflectionDir );

			if ( swingRDot > 0.25f )
			{//deflect to right
				if ( swingUDot > 0.25f )
				{//deflect to top
					attacker->client->ps.saberMove = LS_D1_TR;
				}
				else if ( swingUDot < -0.25f )
				{//deflect to bottom
					attacker->client->ps.saberMove = LS_D1_BR;
				}
				else
				{//deflect horizontally
					attacker->client->ps.saberMove = LS_D1__R;
				}
			}
			else if ( swingRDot < -0.25f )
			{//deflect to left
				if ( swingUDot > 0.25f )
				{//deflect to top
					attacker->client->ps.saberMove = LS_D1_TL;
				}
				else if ( swingUDot < -0.25f )
				{//deflect to bottom
					attacker->client->ps.saberMove = LS_D1_BL;
				}
				else
				{//deflect horizontally
					attacker->client->ps.saberMove = LS_D1__L;
				}
			}
			else
			{//deflect in middle
				if ( swingUDot > 0.25f )
				{//deflect to top
					attacker->client->ps.saberMove = LS_D1_T_;
				}
				else if ( swingUDot < -0.25f )
				{//deflect to bottom
					attacker->client->ps.saberMove = LS_D1_B_;
				}
				else
				{//deflect horizontally?  Well, no such thing as straight back in my face, so use top
					if ( swingRDot > 0 )
					{
						attacker->client->ps.saberMove = LS_D1_TR;
					}
					else if ( swingRDot < 0 )
					{
						attacker->client->ps.saberMove = LS_D1_TL;
					}
					else
					{
						attacker->client->ps.saberMove = LS_D1_T_;
					}
				}
			}

			attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
			return qtrue;
		}
	}
}


//[RACC] - Uses saberMove defines for input
int G_KnockawayForParry( int move )
{
	//FIXME: need actual anims for this
	//FIXME: need to know which side of the saber was hit!  For now, we presume the saber gets knocked away from the center
	switch ( move )
	{
	case LS_PARRY_UP:
		return LS_K1_T_;//push up
		break;
	case LS_PARRY_UR:
	default://case LS_READY:
		return LS_K1_TR;//push up, slightly to right
		break;
	case LS_PARRY_UL:
		return LS_K1_TL;//push up and to left
		break;
	case LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
		break;
	case LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
		break;
	}
}

#define SABER_NONATTACK_DAMAGE 50

//For strong attacks, we ramp damage based on the point in the attack animation
GAME_INLINE int G_GetAttackDamage(gentity_t *self, int minDmg, int maxDmg, float multPoint)
{
	int peakDif = 0;
	int speedDif = 0;
	int totalDamage = maxDmg;
	float peakPoint = 0;
	float attackAnimLength = bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].numFrames * fabs((float)(bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].frameLerp));
	float currentPoint = 0;
	float damageFactor = 0;
	float animSpeedFactor = 1.0f;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	//[FatigueSys]
	BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, self->client->ps.torsoAnim, &animSpeedFactor, self->client->ps.brokenLimbs, self->client->ps.userInt3);
	//BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, self->client->ps.torsoAnim, &animSpeedFactor, self->client->ps.brokenLimbs);
	//[/FatigueSys]
	speedDif = attackAnimLength - (attackAnimLength * animSpeedFactor);
	attackAnimLength += speedDif;
	peakPoint = attackAnimLength;
	peakPoint -= attackAnimLength*multPoint;

	//we treat torsoTimer as the point in the animation (closer it is to attackAnimLength, closer it is to beginning)
	currentPoint = self->client->ps.torsoTimer;


	damageFactor = (float)((currentPoint/peakPoint));
	if (damageFactor > 1)
	{
		damageFactor = (2.0f - damageFactor);
	}

	totalDamage *= damageFactor;
	if (totalDamage < minDmg)
	{
		totalDamage = minDmg;
	}
	if (totalDamage > maxDmg)
	{
		totalDamage = maxDmg;
	}

	//Com_Printf("%i\n", totalDamage);

	return totalDamage;
}

//Get the point in the animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
GAME_INLINE float G_GetAnimPoint(gentity_t *self)
{
	int speedDif = 0;
	float attackAnimLength = bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].numFrames * fabs((float)(bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].frameLerp));
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	//[FatigueSys]
	BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, self->client->ps.torsoAnim, &animSpeedFactor, self->client->ps.brokenLimbs, self->client->ps.userInt3);
	//BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, self->client->ps.torsoAnim, &animSpeedFactor, self->client->ps.brokenLimbs);
	//[/FatigueSys]
	speedDif = attackAnimLength - (attackAnimLength * animSpeedFactor);
	attackAnimLength += speedDif;

	currentPoint = self->client->ps.torsoTimer;

	animPercentage = currentPoint/attackAnimLength;

	//Com_Printf("%f\n", animPercentage);

	return animPercentage;
}

GAME_INLINE qboolean G_ClientIdleInWorld(gentity_t *ent)
{
	if (ent->s.eType == ET_NPC)
	{
		return qfalse;
	}

	if (!ent->client->pers.cmd.upmove &&
		!ent->client->pers.cmd.forwardmove &&
		!ent->client->pers.cmd.rightmove &&
		!(ent->client->pers.cmd.buttons & BUTTON_GESTURE) &&
		!(ent->client->pers.cmd.buttons & BUTTON_FORCEGRIP) &&
		!(ent->client->pers.cmd.buttons & BUTTON_ALT_ATTACK) &&
		!(ent->client->pers.cmd.buttons & BUTTON_FORCEPOWER) &&
		!(ent->client->pers.cmd.buttons & BUTTON_FORCE_LIGHTNING) &&
		!(ent->client->pers.cmd.buttons & BUTTON_FORCE_DRAIN) &&
		//[SaberSys]
		//RAFIXME - This seems to keep getting an read error whenever
		//it hits the BUTTON_SABERTHROW check.  For now, I'm just not
		//using this function anymore.
		!(ent->client->pers.cmd.buttons & BUTTON_ATTACK) &&
		!(ent->client->pers.cmd.buttons & BUTTON_SABERTHROW))
		//!(ent->client->pers.cmd.buttons & BUTTON_ATTACK))
		//[/SaberSys]
	{
		return qtrue;
	}

	return qfalse;
}


//[SaberSys]
float CalcTraceFraction ( vec3_t Start, vec3_t End, vec3_t Endpos )
{
	float fulldist;
	float dist;

	fulldist = VectorDistance(Start, End);
	dist = VectorDistance(Start, Endpos);

	if ( fulldist > 0 )
	{
		if ( dist > 0 )
		{
			return dist/fulldist;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		//I'm going to let it return 1 when the EndPos = End = Start
		return 1;
	}
}
//[/SaberSys]

GAME_INLINE qboolean G_G2TraceCollide(trace_t *tr, vec3_t lastValidStart, vec3_t lastValidEnd, vec3_t traceMins, vec3_t traceMaxs)
{ //Hit the ent with the normal trace, try the collision trace.
	G2Trace_t		G2Trace;
	gentity_t		*g2Hit;
	vec3_t			angles;
	int				tN = 0;
	float			fRadius = 0;

	if (!d_saberGhoul2Collision.integer)
	{
		return qfalse;
	}

	if (!g_entities[tr->entityNum].inuse /*||
		(g_entities[tr->entityNum].s.eFlags & EF_DEAD)*/)
	{ //don't do perpoly on corpses.
		return qfalse;
	}

	if (traceMins[0] ||
		traceMins[1] ||
		traceMins[2] ||
		traceMaxs[0] ||
		traceMaxs[1] ||
		traceMaxs[2])
	{
		fRadius=(traceMaxs[0]-traceMins[0])/2.0f;
	}

	memset (&G2Trace, 0, sizeof(G2Trace));

	while (tN < MAX_G2_COLLISIONS)
	{
		G2Trace[tN].mEntityNum = -1;
		tN++;
	}
	g2Hit = &g_entities[tr->entityNum];

	if (g2Hit && g2Hit->inuse && g2Hit->ghoul2)
	{
		vec3_t g2HitOrigin;

		angles[ROLL] = angles[PITCH] = 0;

		if (g2Hit->client)
		{
			VectorCopy(g2Hit->client->ps.origin, g2HitOrigin);
			angles[YAW] = g2Hit->client->ps.viewangles[YAW];
		}
		else
		{
			VectorCopy(g2Hit->r.currentOrigin, g2HitOrigin);
			angles[YAW] = g2Hit->r.currentAngles[YAW];
		}

		if (g_optvehtrace.integer &&
			g2Hit->s.eType == ET_NPC &&
			g2Hit->s.NPC_class == CLASS_VEHICLE &&
			g2Hit->m_pVehicle)
		{
			trap_G2API_CollisionDetectCache ( G2Trace, g2Hit->ghoul2, angles, g2HitOrigin, level.time, g2Hit->s.number, lastValidStart, lastValidEnd, g2Hit->modelScale, 0, g_g2TraceLod.integer, fRadius );
		}
		else
		{
			trap_G2API_CollisionDetect ( G2Trace, g2Hit->ghoul2, angles, g2HitOrigin, level.time, g2Hit->s.number, lastValidStart, lastValidEnd, g2Hit->modelScale, 0, g_g2TraceLod.integer, fRadius );
		}

		if (G2Trace[0].mEntityNum != g2Hit->s.number)
		{
			tr->fraction = 1.0f;
			tr->entityNum = ENTITYNUM_NONE;
			tr->startsolid = 0;
			tr->allsolid = 0;
			return qfalse;
		}
		else
		{ //The ghoul2 trace result matches, so copy the collision position into the trace endpos and send it back.
			VectorCopy(G2Trace[0].mCollisionPosition, tr->endpos);
			VectorCopy(G2Trace[0].mCollisionNormal, tr->plane.normal);
			//[SaberSys]
			//Calculate the fraction point to keep all the code working correctly
			tr->fraction = CalcTraceFraction( lastValidStart, lastValidEnd, tr->endpos );
			//[/SaberSys]

			if (g2Hit->client)
			{
				g2Hit->client->g2LastSurfaceHit = G2Trace[0].mSurfaceIndex;
				g2Hit->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				g2Hit->client->g2LastSurfaceModel = G2Trace[0].mModelIndex;
				//[/BugFix12]
			}
			return qtrue;
		}
	}

	return qfalse;
}

GAME_INLINE qboolean G_SaberInBackAttack(int move)
{
	switch (move)
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
		return qtrue;
	}

	return qfalse;
}

qboolean saberCheckKnockdown_Thrown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);
qboolean saberCheckKnockdown_Smashed(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other, int damage);
qboolean saberCheckKnockdown_BrokenParry(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);


typedef struct saberFace_s
{
	vec3_t v1;
	vec3_t v2;
	vec3_t v3;
} saberFace_t;

//build faces around blade for collision checking -rww
GAME_INLINE void G_BuildSaberFaces(vec3_t base, vec3_t tip, float radius, vec3_t fwd,
										  vec3_t right, int *fNum, saberFace_t **fList)
{
	static saberFace_t faces[12];
	int i = 0;
	float *d1 = NULL, *d2 = NULL;
	vec3_t invFwd;
	vec3_t invRight;

	VectorCopy(fwd, invFwd);
	VectorInverse(invFwd);
	VectorCopy(right, invRight);
	VectorInverse(invRight);

	while (i < 8)
	{
		//yeah, this part is kind of a hack, but eh
		if (i < 2)
		{ //"left" surface
			d1 = &fwd[0];
			d2 = &invRight[0];
		}
		else if (i < 4)
		{ //"right" surface
			d1 = &fwd[0];
			d2 = &right[0];
		}
		else if (i < 6)
		{ //"front" surface
			d1 = &right[0];
			d2 = &fwd[0];
		}
		else if (i < 8)
		{ //"back" surface
			d1 = &right[0];
			d2 = &invFwd[0];
		}

		//first triangle for this surface
		VectorMA(base, radius/2.0f, d1, faces[i].v1);
		VectorMA(faces[i].v1, radius/2.0f, d2, faces[i].v1);

		VectorMA(tip, radius/2.0f, d1, faces[i].v2);
		VectorMA(faces[i].v2, radius/2.0f, d2, faces[i].v2);

		VectorMA(tip, -radius/2.0f, d1, faces[i].v3);
		VectorMA(faces[i].v3, radius/2.0f, d2, faces[i].v3);

		i++;

		//second triangle for this surface
		VectorMA(tip, -radius/2.0f, d1, faces[i].v1);
		VectorMA(faces[i].v1, radius/2.0f, d2, faces[i].v1);

		VectorMA(base, radius/2.0f, d1, faces[i].v2);
		VectorMA(faces[i].v2, radius/2.0f, d2, faces[i].v2);

		VectorMA(base, -radius/2.0f, d1, faces[i].v3);
		VectorMA(faces[i].v3, radius/2.0f, d2, faces[i].v3);

		i++;
	}

	//top surface
	//face 1
	VectorMA(tip, radius/2.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, -radius/2.0f, right, faces[i].v1);

	VectorMA(tip, radius/2.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, radius/2.0f, right, faces[i].v2);

	VectorMA(tip, -radius/2.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, -radius/2.0f, right, faces[i].v3);

	i++;

	//face 2
	VectorMA(tip, radius/2.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, radius/2.0f, right, faces[i].v1);

	VectorMA(tip, -radius/2.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, -radius/2.0f, right, faces[i].v2);

	VectorMA(tip, -radius/2.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, radius/2.0f, right, faces[i].v3);

	i++;

	//bottom surface
	//face 1
	VectorMA(base, radius/2.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, -radius/2.0f, right, faces[i].v1);

	VectorMA(base, radius/2.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, radius/2.0f, right, faces[i].v2);

	VectorMA(base, -radius/2.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, -radius/2.0f, right, faces[i].v3);

	i++;

	//face 2
	VectorMA(base, radius/2.0f, fwd, faces[i].v1);
	VectorMA(faces[i].v1, radius/2.0f, right, faces[i].v1);

	VectorMA(base, -radius/2.0f, fwd, faces[i].v2);
	VectorMA(faces[i].v2, -radius/2.0f, right, faces[i].v2);

	VectorMA(base, -radius/2.0f, fwd, faces[i].v3);
	VectorMA(faces[i].v3, radius/2.0f, right, faces[i].v3);

	i++;

	//yeah.. always going to be 12 I suppose.
	*fNum = i;
	*fList = &faces[0];
}

//collision utility function -rww
GAME_INLINE void G_SabCol_CalcPlaneEq(vec3_t x, vec3_t y, vec3_t z, float *planeEq)
{
	planeEq[0] = x[1]*(y[2]-z[2]) + y[1]*(z[2]-x[2]) + z[1]*(x[2]-y[2]);
	planeEq[1] = x[2]*(y[0]-z[0]) + y[2]*(z[0]-x[0]) + z[2]*(x[0]-y[0]);
	planeEq[2] = x[0]*(y[1]-z[1]) + y[0]*(z[1]-x[1]) + z[0]*(x[1]-y[1]);
	planeEq[3] = -(x[0]*(y[1]*z[2] - z[1]*y[2]) + y[0]*(z[1]*x[2] - x[1]*z[2]) + z[0]*(x[1]*y[2] - y[1]*x[2]) );
}

//collision utility function -rww
GAME_INLINE int G_SabCol_PointRelativeToPlane(vec3_t pos, float *side, float *planeEq)
{
	*side = planeEq[0]*pos[0] + planeEq[1]*pos[1] + planeEq[2]*pos[2] + planeEq[3];

	if (*side > 0.0f)
	{
		return 1;
	}
	else if (*side < 0.0f)
	{
		return -1;
	}

	return 0;
}

//do actual collision check using generated saber "faces"
GAME_INLINE qboolean G_SaberFaceCollisionCheck(int fNum, saberFace_t *fList, vec3_t atkStart,
											 vec3_t atkEnd, vec3_t atkMins, vec3_t atkMaxs, vec3_t impactPoint)
{
	static float planeEq[4];
	static float side, side2, dist;
	static vec3_t dir;
	static vec3_t point;
	int i = 0;

	if (VectorCompare(atkMins, vec3_origin) && VectorCompare(atkMaxs, vec3_origin))
	{
		VectorSet(atkMins, -1.0f, -1.0f, -1.0f);
		VectorSet(atkMaxs, 1.0f, 1.0f, 1.0f);
	}

	VectorSubtract(atkEnd, atkStart, dir);

	while (i < fNum)
	{
		G_SabCol_CalcPlaneEq(fList->v1, fList->v2, fList->v3, planeEq);

		if (G_SabCol_PointRelativeToPlane(atkStart, &side, planeEq) !=
			G_SabCol_PointRelativeToPlane(atkEnd, &side2, planeEq))
		{ //start/end points intersect with the plane
			static vec3_t extruded;
			static vec3_t minPoint, maxPoint;
			static vec3_t planeNormal;
			static int facing;

			VectorCopy(&planeEq[0], planeNormal);
			side2 = planeNormal[0]*dir[0] + planeNormal[1]*dir[1] + planeNormal[2]*dir[2];

			dist = side/side2;
			VectorMA(atkStart, -dist, dir, point);

			VectorAdd(point, atkMins, minPoint);
			VectorAdd(point, atkMaxs, maxPoint);

			//point is now the point at which we intersect on the plane.
			//see if that point is within the edges of the face.
            VectorMA(fList->v1, -2.0f, planeNormal, extruded);
			G_SabCol_CalcPlaneEq(fList->v1, fList->v2, extruded, planeEq);
			facing = G_SabCol_PointRelativeToPlane(point, &side, planeEq);

			if (facing < 0)
			{ //not intersecting.. let's try with the mins/maxs and see if they interesect on the edge plane
				facing = G_SabCol_PointRelativeToPlane(minPoint, &side, planeEq);
				if (facing < 0)
				{
					facing = G_SabCol_PointRelativeToPlane(maxPoint, &side, planeEq);
				}
			}

			if (facing >= 0)
			{ //first edge is facing...
				VectorMA(fList->v2, -2.0f, planeNormal, extruded);
				G_SabCol_CalcPlaneEq(fList->v2, fList->v3, extruded, planeEq);
				facing = G_SabCol_PointRelativeToPlane(point, &side, planeEq);

				if (facing < 0)
				{ //not intersecting.. let's try with the mins/maxs and see if they interesect on the edge plane
					facing = G_SabCol_PointRelativeToPlane(minPoint, &side, planeEq);
					if (facing < 0)
					{
						facing = G_SabCol_PointRelativeToPlane(maxPoint, &side, planeEq);
					}
				}

				if (facing >= 0)
				{ //second edge is facing...
					VectorMA(fList->v3, -2.0f, planeNormal, extruded);
					G_SabCol_CalcPlaneEq(fList->v3, fList->v1, extruded, planeEq);
					facing = G_SabCol_PointRelativeToPlane(point, &side, planeEq);

					if (facing < 0)
					{ //not intersecting.. let's try with the mins/maxs and see if they interesect on the edge plane
						facing = G_SabCol_PointRelativeToPlane(minPoint, &side, planeEq);
						if (facing < 0)
						{
							facing = G_SabCol_PointRelativeToPlane(maxPoint, &side, planeEq);
						}
					}

					if (facing >= 0)
					{ //third edge is facing.. success
						VectorCopy(point, impactPoint);
						return qtrue;
					}
				}
			}
		}

		i++;
		fList++;
	}
	
	//did not hit anything
	return qfalse;
}


//[SaberSys]
//Copies all the important data from one trace_t to another.  Please note that this doesn't transfer ALL
//of the trace_t data.
void TraceCopy( trace_t *a, trace_t *b)
{
	b->allsolid = a->allsolid;
	b->contents = a->contents;
	VectorCopy(a->endpos, b->endpos);
	b->entityNum = a->entityNum;
	b->fraction = a->fraction;
	//This is the only thing that's ever really used from the plane data.
	VectorCopy(a->plane.normal, b->plane.normal);
	b->startsolid = a->startsolid;
	b->surfaceFlags = a->surfaceFlags;
}


//Reset the trace to be "blank".
GAME_INLINE void TraceClear( trace_t *tr, vec3_t end )
{
		tr->fraction = 1;
		VectorCopy( end, tr->endpos );
		tr->entityNum = ENTITYNUM_NONE;
}
//[/SaberSys]


//[BugFix26]
qboolean OJP_SaberIsOff( gentity_t *self, int saberNum );
qboolean OJP_BladeIsOff(gentity_t *self, int saberNum, int bladeNum);
//[/BugFix26]
//check for collision of 2 blades -rww
GAME_INLINE qboolean G_SaberCollide(gentity_t *atk, gentity_t *def, vec3_t atkStart, 
//[SaberSys]
											vec3_t atkEnd, vec3_t atkMins, vec3_t atkMaxs, trace_t *tr)
//											vec3_t atkEnd, vec3_t atkMins, vec3_t atkMaxs, vec3_t impactPoint)
//[/SaberSys]
{
	static int i, j;

	if (!g_saberBladeFaces.integer)
	{ //detailed check not enabled
		return qtrue;
	}

	//[SaberSys]
	//Removed the atk gentity requirements for the function so my saber trace will work with
	//atk gentity checks.
	if (!def->inuse || !def->client)
	//if (!atk->inuse || !atk->client || !def->inuse || !def->client)
	{ //must have 2 clients and a valid saber entity
		TraceClear( tr, atkEnd);
		return qfalse;
	//[/SaberSys]
	}

	//[BugFix26]
	if(def->client->ps.saberHolstered == 2)
	{//no sabers on.
		TraceClear( tr, atkEnd);
		return qfalse;
	}
	//[/BugFix26]

	i = 0;
	while (i < MAX_SABERS)
	{
		j = 0;

		//[BugFix26]
		if( OJP_SaberIsOff(def, i) )
		{//saber is off and can't be used.
			i++;
			continue;
		}
		//[/BugFix26]

		if (def->client->saber[i].model[0])
		{ //valid saber on the defender
			bladeInfo_t *blade;
			vec3_t v, fwd, right, base, tip;
			int fNum;
			saberFace_t *fList;

			//go through each blade on the defender's sabers
			while (j < def->client->saber[i].numBlades)
			{
				blade = &def->client->saber[i].blade[j];

				//[BugFix26]
				if(OJP_BladeIsOff(def, i, j))
				{//this particular blade is turned off.
					j++;
					continue;
				}
				//[/BugFix26]

				if ((level.time-blade->storageTime) < 200)
				{ //recently updated
					//first get base and tip of blade
					VectorCopy(blade->muzzlePoint, base);
					VectorMA(base, blade->lengthMax, blade->muzzleDir, tip);

					//Now get relative angles between the points
					VectorSubtract(tip, base, v);
					vectoangles(v, v);
					AngleVectors(v, NULL, right, fwd);

					//now build collision faces for this blade
					G_BuildSaberFaces(base, tip, blade->radius*3.0f, fwd, right, &fNum, &fList);
					if (fNum > 0)
					{
#if 0
						//[SaberSys]
						//atk is no longer assumed to be valid at this point
						if (atk->inuse && atk->client && atk->s.number == 0)
						//if (atk->s.number == 0)
						//[/SaberSys]
						{
							int x = 0;
							saberFace_t *l = fList;
							while (x < fNum)
							{
								G_TestLine(fList->v1, fList->v2, 0x0000ff, 100);
								G_TestLine(fList->v2, fList->v3, 0x0000ff, 100);
								G_TestLine(fList->v3, fList->v1, 0x0000ff, 100);

								fList++;
								x++;
							}
							fList = l;
						}
#endif
						//[SaberSys]
						if (G_SaberFaceCollisionCheck(fNum, fList, atkStart, atkEnd, atkMins, atkMaxs, tr->endpos))
						//if (G_SaberFaceCollisionCheck(fNum, fList, atkStart, atkEnd, atkMins, atkMaxs, impactPoint))
						{ //collided
							//determine the plane of impact for the viewlocking stuff.
							vec3_t result;

							tr->fraction = CalcTraceFraction( atkStart, atkEnd, tr->endpos );

							G_FindClosestPointOnLineSegment( base, tip, tr->endpos, result );
							VectorSubtract(tr->endpos, result, result);
							VectorCopy(result, tr->plane.normal);
							if( atk && atk->client )
							{
								atk->client->lastSaberCollided = i;
								atk->client->lastBladeCollided = j;
							}
						//[/SaberSys]
							return qtrue;
						}
					}
				}
				j++;
			}
		}
		i++;
	}

	//[SaberSys]
	//Make sure the trace has the correct trace data
	TraceClear( tr, atkEnd);
	//[SaberSys]
	return qfalse;
}


//[SaberSys]
int BasicSaberBlockCost(int attackerStyle)
{//returns the basic saber block cost of blocking an attack from the given saber style.
	switch(attackerStyle)
	{
	case SS_DUAL:
		return 15;
		break;
	case SS_STAFF:
		return 15;
		break;
	case SS_TAVION:
		return 13;
		break;
	case SS_FAST:
		return 10;
		break;
	case SS_MEDIUM:
		return 15;
		break;
	case SS_DESANN:
		return 18;
		break;
	case SS_STRONG:
		return 20;
		break;
	default:
		G_Printf("Unknown Style type %i in BasicSaberBlockCost()\n", attackerStyle);
		return 0;
		break;
	};
}


qboolean /*GAME_INLINE*/ WalkCheck( gentity_t * self );
qboolean G_BlockIsParry( gentity_t *self, gentity_t *attacker, vec3_t hitLoc );
int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc)
{//returns the DP cost to block this attack for this attacker/defender combo.
	float saberBlockCost = 0;

	//===========================
	// Determine Base Block Cost
	//===========================

	if(!attacker	//don't have attacker
		|| !attacker->client	//attacker isn't a NPC/player
		|| attacker->client->ps.weapon != WP_SABER ) //or the player that is attacking isn't using a saber
	{//standard bolt block!
			//[BlasterDP]
		if(attacker->activator)
		{
			if(attacker->activator->s.weapon == WP_BRYAR_PISTOL)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else 
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}				
			
			}
			else if(attacker->activator->s.weapon == WP_BLASTER)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}	
				else 
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}				
			
			}			
			else if(attacker->activator->s.weapon == WP_DISRUPTOR)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=20*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				}
				}	
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=20*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				}
				}	
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=20*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				}
				}		
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=20*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				}
				}	
				else
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=20*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=5*DODGE_BOLTBLOCK;
					break;
				}
				}				
			}	
			else if(attacker->activator->s.weapon == WP_BOWCASTER)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=10*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				}
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=10*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				}
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=10*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				}
				}		
				else
				{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=10*DODGE_BOLTBLOCK;
					break;
				case 4:
				case 3:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				case 2:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=3*DODGE_BOLTBLOCK;
					break;
				}
				}			
			}
			else if(attacker->activator->s.weapon == WP_REPEATER)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}			
				else
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}				
			}
			else if(attacker->activator->s.weapon == WP_DEMP2)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK*2;
				else
					saberBlockCost = DODGE_BOLTBLOCK;
				}	
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK*2;
				else
					saberBlockCost = DODGE_BOLTBLOCK;
				}				
				else
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}				
			}
			else if(attacker->activator->s.weapon == WP_FLECHETTE)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				if(WalkCheck(defender))
					saberBlockCost= DODGE_BOLTBLOCK/2;
				else
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
				if(WalkCheck(defender))
					saberBlockCost= DODGE_BOLTBLOCK/2;
				else
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
				if(WalkCheck(defender))
					saberBlockCost= DODGE_BOLTBLOCK/2;
				else
					saberBlockCost= DODGE_BOLTBLOCK;
				}		
				else
				{
				if(WalkCheck(defender))
					saberBlockCost= DODGE_BOLTBLOCK/2;
				else
					saberBlockCost= DODGE_BOLTBLOCK;
				}			
			}
			else if(attacker->activator->s.weapon == WP_CONCUSSION)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}	
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3/2;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK;
				else
					saberBlockCost = DODGE_BOLTBLOCK/2;
				}	
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
					saberBlockCost=20*DODGE_BOLTBLOCK;
				}			
				else
				{
					saberBlockCost=20*DODGE_BOLTBLOCK;
				}				
			}
			else if(attacker->activator->s.weapon == WP_BRYAR_OLD)
			{
				if (attacker->activator->s.eFlags & EF_WP_OPTION_2)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_3)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else if (attacker->activator->s.eFlags & EF_WP_OPTION_4)	
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}
				else 
				{
					saberBlockCost= DODGE_BOLTBLOCK;
				}				
			
			}
			else if(attacker->activator->s.weapon == WP_EMPLACED_GUN)
			{

				float distance = VectorDistance(attacker->activator->r.currentOrigin,defender->r.currentOrigin);
				if(distance <= 125.0f)
					saberBlockCost = DODGE_BOLTBLOCK*3;
				else if(distance <= 300.0f)
					saberBlockCost = DODGE_BOLTBLOCK*2;
				else
					saberBlockCost = DODGE_BOLTBLOCK;			
			
			}	
			else
			{
				switch(attacker->s.generic1)
				{
				case 5:
					saberBlockCost=DODGE_BOLTBLOCK*5;
					break;
				case 4:
				case 3:
					saberBlockCost=DODGE_BOLTBLOCK*3/2;
					break;
				case 2:
					saberBlockCost=DODGE_BOLTBLOCK;
					break;
				default:
					saberBlockCost=DODGE_BOLTBLOCK;
					break;
				}
			}			
		}		
		else
		{
		saberBlockCost = DODGE_BOLTBLOCK;
		}				
				

	}
	else if(attacker->client->ps.saberMove == LS_A_LUNGE
		|| attacker->client->ps.saberMove == LS_SPINATTACK
		|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL)
	{//lunge attacks
		saberBlockCost = .75*BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if(attacker->client->ps.saberMove == LS_ROLL_STAB)
	{//roll stab
		saberBlockCost = 2*BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if(attacker->client->ps.saberMove == LS_A_JUMP_T__B_)
	{//DFA moves
		saberBlockCost = 4*BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else if(attacker->client->ps.saberMove == LS_A_FLIP_STAB 
		|| attacker->client->ps.saberMove == LS_A_FLIP_SLASH )
	{//flip stabs do more DP
		saberBlockCost = 2*BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
	}
	else
	{//"normal" swing moves
		if(attacker->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) 
		{//attacker is in an attack fake
			if(attacker->client->ps.fd.saberAnimLevel == SS_STRONG
				&& !G_BlockIsParry(defender, attacker, hitLoc))
			{//Red does additional DP damage with attack fakes if they aren't parried.
				saberBlockCost = (BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel) * 1.35);
			}
			else
			{
				saberBlockCost = (BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel) * 1.25);
			}
		}
		else
		{//normal saber block
			saberBlockCost = BasicSaberBlockCost(attacker->client->ps.fd.saberAnimLevel);
		}

		//add running damage bonus to normal swings but don't apply if the defender is slowbouncing
		if(!WalkCheck(attacker) 
			&& !(defender->client->ps.userInt3 & ( 1 << FLAG_SLOWBOUNCE ))
			&& !(defender->client->ps.userInt3 & ( 1 << FLAG_OLDSLOWBOUNCE ))) 
		{

			if(attacker->client->saber[0].numBlades == 1 && attacker->client->ps.fd.saberAnimLevel == SS_DUAL)//Ataru's other perk more powerful running hits
			{
				saberBlockCost *= 3.0;
			}
			else
			{
				saberBlockCost *= 1.5;
			}
		}
	}
	    

	//======================
	// Block Cost Modifiers
	//======================

	if(attacker && attacker->client)
	{//attacker is a player so he must have just hit you with a saber blow.
		if(G_BlockIsParry(defender, attacker, hitLoc))
		{//parried this attack, cost is less
			if(defender->client->ps.fd.saberAnimLevel == SS_FAST)
			{//blue parries cheaper
				saberBlockCost = (saberBlockCost/3.25);
			}
			else
			{
				saberBlockCost = (saberBlockCost/3);
			}
		}

		if(!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -.7f))
		{//player is behind us, costs more to block
				//staffs back block at normal cost.
			/*
			if(defender->client->ps.fd.saberAnimLevel != SS_STAFF 
				//level 3 saber defenders do back blocks for normal cost.
				&& defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_3) 
			{
				saberBlockCost *= 2;
			}
			*/
		    
			//staffs back block at normal cost.
			if(defender->client->ps.fd.saberAnimLevel == SS_STAFF &&!(defender->client->saber[0].numBlades == 1)
				//level 3 saber defenders do back blocks for normal cost.
			&& defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_3) 
			{// Having both staff and defense 3 allow no extra back hit damage
				saberBlockCost *= 1.0;
			}
			else if((defender->client->ps.fd.saberAnimLevel == SS_STAFF && !(defender->client->saber[0].numBlades == 1))
				//level 3 saber defenders and staff users  have much lessback damage. Staff sabers perk
			|| defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_3) 
			{
				saberBlockCost *= 1.25;
			}
			else if(defender->client->ps.fd.saberAnimLevel != SS_STAFF 
			&& defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_2) 
			{//level 2 defense lowers back damage more
				saberBlockCost *= 1.50;
			}
			else if(defender->client->ps.fd.saberAnimLevel != SS_STAFF 
			&& defender->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_1) 
			{//level 1 defense lowers back damage a bit
				saberBlockCost *= 1.75;
			}
			else
			{
				saberBlockCost *= 2.0;
			}
		}

		//clamp to body dodge cost since it wouldn't be fair to cost more than that.
		if(saberBlockCost > BasicDodgeCosts[MOD_SABER])
		{
			saberBlockCost = BasicDodgeCosts[MOD_SABER];
		}
	}
    if(PM_SaberInBrokenParry(defender->client->ps.saberMove))
	{//we're stunned/stumbling, increase DP cost
		saberBlockCost *= 1.5;
	}

	if(BG_KickingAnim(defender->client->ps.legsAnim))
	{//kicking
		saberBlockCost *= 1.5;
	}

	if(!WalkCheck(defender))
	{
		if(defender->NPC)
		{
		  saberBlockCost *=1.0;	
		}
		else
		{
		  saberBlockCost *= 2.0;	
		}
	}
	if(defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//in mid-air
		if(defender->client->saber[0].numBlades == 1 && defender->client->ps.fd.saberAnimLevel == SS_DUAL)//Ataru's other perk much less cost for air hit
		{
			saberBlockCost *= 0.5;
		}
		else
		{
			saberBlockCost *= 2.0;
		} 
	}
	if(defender->client->ps.saberBlockTime > level.time)
	{//attempting to block something too soon after a saber bolt block
		saberBlockCost *= 2;
	}

	return (int) saberBlockCost;
}


qboolean OJP_UsingDualSaberAsPrimary(playerState_t *ps);
extern qboolean BG_SuperBreakWinAnim( int anim );
extern qboolean BG_SaberInNonIdleDamageMove(playerState_t *ps, int AnimIndex);
int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum)
{//similar to WP_SaberCanBlock but without the same sorts of restrictions.
	vec3_t bodyMin, bodyMax, closestBodyPoint, dirToBody, saberMoveDir;
//	float distance = VectorDistance(atk->r.currentOrigin,self->r.currentOrigin);

	if (!self || !self->client || !atk)
	{
		return 0;
	}

	if(atk && atk->s.eType == ET_MISSILE //is a missile
		&& (atk->s.weapon == WP_ROCKET_LAUNCHER ||
			atk->s.weapon == WP_THERMAL ||
			atk->s.weapon == WP_TRIP_MINE ||
			atk->s.weapon == WP_DET_PACK  || //fix for force destruction
			atk->methodOfDeath == MOD_REPEATER_ALT ||
			atk->methodOfDeath == MOD_FLECHETTE_ALT_SPLASH ||
			atk->methodOfDeath == MOD_CONC ||
			atk->methodOfDeath == MOD_CONC_ALT ||
			atk->methodOfDeath == MOD_BRYAR_PISTOL_ALT ||
			atk->methodOfDeath == MOD_FORCE_DESTRUCTION) )
	{//can't block this stuff with a saber
		return 0;
	}

	/*
	if(atk && atk->client->skillLevel[SK_PISTOL] == 3)
		// if the attacker have has level 3 pistol
	{
		if(atk && atk->s.eType == ET_MISSILE //is a missile
		   && atk->methodOfDeath == MOD_BRYAR_PISTOL_ALT)	
	    {//can't block this with a saber
			return 0;
		}
		else
		{
			return 1;
		}
	}
	*/
	/* racc - we need to block during stumbles because kicks cause stumbles...a lot!
	if (PM_SaberInBrokenParry(self->client->ps.saberMove))
	{//you've been stunned from a broken parry
		return 0;
	}
	*/

	if(BG_InGrappleMove(self->client->ps.torsoAnim))
	{//you can't block while doing a melee move.
		return 0;
	}
	
	if(BG_KickMove(self->client->ps.saberMove))
	{
		return 0;
	}

	if (!self->client->ps.saberEntityNum || self->client->ps.saberInFlight)
	{//our saber is currently dropped or in flight.
		if(!OJP_UsingDualSaberAsPrimary(&self->client->ps))
		{//don't have a saber to block with
			return 0;
		}
	}

	if (BG_SabersOff( &self->client->ps ))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER)
	{
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return 0;
	}
	
	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		if(self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
		{//can't saber block in the middle of a body dodge.
			return 0;
		}
		if(atk && atk->client && atk->client->ps.weapon == WP_SABER)
		{//can't block while using forceHandExtend except if their using a saber
			return 1;
		}
		else
		{//can't block while using forceHandExtend

			return 0;
		}
	}
	if(!WalkCheck(self) && self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
		{//can't block while running in force speed.
			return 0;
		}

	if (PM_InKnockDown(&self->client->ps))
	{//can't block while knocked down or getting up from knockdown.
		return 0;
	}
	 
	if(atk && atk->client && atk->client->ps.weapon == WP_SABER)
	{//player is attacking with saber
		if( !BG_SaberInNonIdleDamageMove(&atk->client->ps, atk->localAnimIndex) )
		{//saber attacker isn't in a real damaging move
			return 0;
		}

		if((atk->client->ps.saberMove == LS_A_LUNGE 
			|| atk->client->ps.saberMove == LS_SPINATTACK
			|| atk->client->ps.saberMove == LS_SPINATTACK_DUAL)
			&& self->client->ps.userInt3 & (1 << FLAG_FATIGUED_HEAVY) ) 
		{//saber attacker, we can't block lunge attacks while fatigued. 
			return 0;
		}

		if(BG_SuperBreakWinAnim(atk->client->ps.torsoAnim) && BG_SuperBreakLoseAnim(self->client->ps.torsoAnim))
		{//can't block super breaks when we lost a saber lock.
			return 0;
		}
		if(!WalkCheck(self) 
		&& (!InFront(atk->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)
		|| BG_SaberInAttack( self->client->ps.saberMove )
		|| PM_SaberInStart( self->client->ps.saberMove )))
		{//can't block saber swings while running and hit from behind or in swing.
		//G_Printf("%i: %i Can't block because I'm running.\n", level.time, self->s.number);
			if(self->NPC)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		
	}

	//check to see if we have the Dodge to do this.
	if(self->client->ps.stats[STAT_DODGE] < OJP_SaberBlockCost(self, atk, point))
	{
		return 0;
	}

	//ok, I'm removing this to get around the problems with long reach attacks cliping thru the player.
	//SABERSYSRAFIXME - allow for blocking behind our backs
	if (!InFront( point, self->client->ps.origin, self->client->ps.viewangles, -.2 ))
	{//can only
		if(self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
			return 1;
		else
			return 0;
	}
	

	if(!checkBBoxBlock)
	{//don't do the additional checkBBoxBlock checks.  As such, we're safe to saber block.
		return 1;
	}

	if(atk && atk->client && atk->client->ps.weapon == WP_SABER && BG_SuperBreakWinAnim(atk->client->ps.torsoAnim))
	{//never box block saberlock super break wins, it looks weird.
		return 0;
	}

	if(VectorCompare(point, vec3_origin))
	{//no hit position given, can't do blade movement check.
		return 0;
	}

	if(atk && atk->client && rSaberNum != -1 && rBladeNum != -1)
	{//player attacker, if they are here they're using their saber to attack.  
		//Check to make sure that we only block the blade if it is moving towards the player

		//create a line seqment thru the center of the player.
		VectorCopy(self->client->ps.origin, bodyMin);
		VectorCopy(self->client->ps.origin, bodyMax);

		bodyMax[2] += self->r.maxs[2];
		bodyMin[2] -= self->r.mins[2];
		
		//find dirToBody
		G_FindClosestPointOnLineSegment(bodyMin, bodyMax, point, closestBodyPoint);
		VectorSubtract(closestBodyPoint, point, dirToBody);

		//find current saber movement direction of the attacker
		VectorSubtract(atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, 
			atk->client->saber[rSaberNum].blade[rBladeNum].muzzlePointOld, saberMoveDir);

		if(DotProduct(dirToBody, saberMoveDir) < 0)
		{//saber is moving away from defender
			return 0;
		}
	}

	return 1;
}


//Number of objects that a RealTrace can passthru when the ghoul2 trace fails. 
#define MAX_REAL_PASSTHRU 8

//struct for saveing the 
typedef struct content_s
{
	int			content;
	int			entNum;
} content_t;

content_t	RealTraceContent[MAX_REAL_PASSTHRU];

#define REALTRACEDATADEFAULT	-2

void InitRealTraceContent(void)
{
	int i;
	for(i = 0;i<MAX_REAL_PASSTHRU;i++)
	{
		RealTraceContent[i].content = REALTRACEDATADEFAULT;
		RealTraceContent[i].entNum = REALTRACEDATADEFAULT;
	}
}


//returns true on success
qboolean AddRealTraceContent(int entityNum)
{
	int i;

	if(entityNum == ENTITYNUM_WORLD || entityNum == ENTITYNUM_NONE)
	{//can't blank out the world.  Give an error.
		G_Printf("Error: AddRealTraceContent was passed an bad EntityNum.\n");
		return qtrue;
	}

	for(i=0; i < MAX_REAL_PASSTHRU; i++)
	{
		if( RealTraceContent[i].content == REALTRACEDATADEFAULT && RealTraceContent[i].entNum == REALTRACEDATADEFAULT )
		{//found an empty slot.  Use it.
			//Stored Data
			RealTraceContent[i].entNum = entityNum;
			RealTraceContent[i].content = g_entities[entityNum].r.contents;

			//Blank it.
			g_entities[entityNum].r.contents = 0;
			return qtrue;
		}
	}

	//All slots already used. 
	return qfalse;
}


//Restored all the entities that have been blanked out in the RealTrace
void RestoreRealTraceContent(void)
{
	int i;
	for(i=0; i < MAX_REAL_PASSTHRU; i++)
	{
		if( RealTraceContent[i].entNum != REALTRACEDATADEFAULT )
		{
			if(RealTraceContent[i].content != REALTRACEDATADEFAULT)
			{
				g_entities[RealTraceContent[i].entNum].r.contents = RealTraceContent[i].content;

				//Let's clean things out to be sure.
				RealTraceContent[i].entNum = REALTRACEDATADEFAULT;
				RealTraceContent[i].content = REALTRACEDATADEFAULT;
			}
			else
			{
				G_Printf("Error: RestoreRealTraceContent: The stored Real Trace contents was the empty default!\n");
			}
		}
		else
		{//This data slot is blank.  This should mean that the rest are empty as well.
			break;
		}
	}
}


#define REALTRACE_MISS				0 //didn't hit anything
#define REALTRACE_HIT				1 //hit object normally
#define REALTRACE_SABERBLOCKHIT		2 //hit a player who used a bounding box dodge saber block
GAME_INLINE int Finish_RealTrace( trace_t *results, trace_t *closestTrace, vec3_t start, vec3_t end )
{//this function reverts the real trace content removals and finishs up the realtrace
	//restore all the entities we blanked out.
	RestoreRealTraceContent();

	if(VectorCompare(closestTrace->endpos, end))
	{//No hit. Make sure that tr is correct.
		TraceClear(results, end);
		return REALTRACE_MISS;
	}

	TraceCopy(closestTrace, results);
	return REALTRACE_HIT;
}


//This function is setup to give much more realistic traces for saber attack traces.
//It's not 100% perfect, but the situations where this won't work right are very rare and
//probably not worth the additional hassle.
//
//gentity_t attacker is an optional input variable to give information about the attacker.  This is used to give the attacker 
//		information about which saber blade they hit (if any) and to see if the victim should use a bounding box saber block.
//		Not providing an attacker will make the function just return REALTRACE_MISS or REALTRACE_HIT.

//return:	REALTRACE_MISS = didn't hit anything	
//			REALTRACE_HIT = hit object normally
//			REALTRACE_SABERBLOCKHIT = hit a player who used a bounding box dodge saber block
int G_RealTrace(gentity_t *attacker, trace_t *tr, vec3_t start, vec3_t mins, 
										vec3_t maxs, vec3_t end, int passEntityNum, 
										int contentmask, int rSaberNum, int rBladeNum)
{
	//the current start position of the traces.  
	//This is advanced to the edge of each bound box after each saber/ghoul2 entity is processed.
	vec3_t currentStart;
	trace_t closestTrace; 		//this is the trace struct of the closest successful trace.
	float closestFraction = 1.1; 	//the fraction of the closest trace so far.  Initially set higher than one so that we have an actualy tr even if the tr is clear.
	int misses = 0;
	qboolean atkIsSaberer = (attacker && attacker->client && attacker->client->ps.weapon == WP_SABER) ? qtrue : qfalse;
	InitRealTraceContent();

	if( atkIsSaberer )
	{//attacker is using a saber to attack us, blank out their saber/blade data so we have a fresh start for this trace.
		attacker->client->lastSaberCollided = -1;
		attacker->client->lastBladeCollided = -1;
	}

	//make the default closestTrace be nothing
	TraceClear(&closestTrace, end);

	VectorCopy(start, currentStart);

	for(misses = 0; misses < MAX_REAL_PASSTHRU; misses++)
	{
		vec3_t currentEndPos;
		int currentEntityNum;
		gentity_t *currentEnt;

		//Fire a standard trace and see what we find.
		trap_Trace(tr, currentStart, mins, maxs, end, passEntityNum, contentmask);

		//save the point where we hit.  This is either our end point or the point where we hit our next bounding box.
		VectorCopy(tr->endpos, currentEndPos);

		//also save the storedEntityNum since the internal traces normally blank out the trace_t if they fail.
		currentEntityNum = tr->entityNum;

		if(tr->startsolid)
		{//make sure that tr->endpos is at the start point as it should be for startsolid.
			VectorCopy(currentStart, tr->endpos);
		}

		if(tr->entityNum == ENTITYNUM_NONE)
		{//We've run out of things to hit so we're done.
			if(!VectorCompare(start, currentStart))
			{//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
				tr->fraction = CalcTraceFraction(start, end, tr->endpos);
			}


			if(tr->fraction < closestFraction)
			{//this is the closest hit, make it so.
				TraceCopy(tr, &closestTrace);
				closestFraction = tr->fraction;
			}
			return Finish_RealTrace(tr, &closestTrace, start, end);
		}

		//set up a pointer to the entity we hit.
		currentEnt = &g_entities[tr->entityNum];

		if (currentEnt->inuse && currentEnt->client)
		{//initial trace hit a humanoid
			if(attacker && OJP_SaberCanBlock(currentEnt, attacker, qtrue, tr->endpos, rSaberNum, rBladeNum))
			{//hit victim is willing to bbox block with their jedi saber abilities.  Can only do this if we have data on the attacker.
				if(!VectorCompare(start, currentStart))
				{//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
					tr->fraction = CalcTraceFraction(start, end, tr->endpos);
				}

				if(tr->fraction < closestFraction)
				{//this is the closest known hit object for this trace, so go ahead and count the bbox block as the closest impact.
					RestoreRealTraceContent();

					//act like the saber was hit instead of us.
					tr->entityNum = currentEnt->client->saberStoredIndex;
					return REALTRACE_SABERBLOCKHIT;
				}
				else
				{//something else ghoul2 related was already hit and was closer, skip to end of function
					return Finish_RealTrace(tr, &closestTrace, start, end);
				}
			}

			//ok, no bbox block this time.  So, try a ghoul2 trace then.
			G_G2TraceCollide(tr, currentStart, end, mins, maxs);
		}
		else if((currentEnt->r.contents & CONTENTS_LIGHTSABER) &&
			currentEnt->r.contents != -1 &&
			currentEnt->inuse)
		{//hit a lightsaber, do the approprate collision detection checks.
			gentity_t* saberOwner = &g_entities[currentEnt->r.ownerNum];

			G_SaberCollide( (atkIsSaberer ? attacker : NULL), saberOwner, currentStart, end, mins, maxs, tr);
		}
		else if (tr->entityNum < ENTITYNUM_WORLD)
		{
			if (currentEnt->inuse && currentEnt->ghoul2)
			{ //hit a non-client entity with a g2 instance
				G_G2TraceCollide(tr, currentStart, end, mins, maxs);
			}
			else 
			{//this object doesn't have a ghoul2 or saber internal trace.  
				if(!VectorCompare(start, currentStart))
				{//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
					tr->fraction = CalcTraceFraction(start, end, tr->endpos);
				}

				//As such, it's the last trace on our layered trace.
				if(tr->fraction < closestFraction)
				{//this is the closest hit, make it so.
					TraceCopy(tr, &closestTrace);
					closestFraction = tr->fraction;
				}
				return Finish_RealTrace(tr, &closestTrace, start, end);
			}
		}
		else
		{//world hit.  We either hit something closer or this is the final trace of our layered tracing.
			if(!VectorCompare(start, currentStart))
			{//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
				tr->fraction = CalcTraceFraction(start, end, tr->endpos);
			}

			if(tr->fraction < closestFraction)
			{//this is the closest hit, make it so.
				TraceCopy(tr, &closestTrace);
				closestFraction = tr->fraction;
			}
			return Finish_RealTrace(tr, &closestTrace, start, end);
		}

		//ok, we're just completed an internal ghoul2 or saber internal trace on an entity.  
		//At this point, we need to make this the closest impact if it was and continue scanning.
		//We do this since this ghoul2/saber entities have true impact positions that aren't the same as their bounding box
		//exterier impact position.  As such, another entity could be slightly inside that bounding box but have a closest
		//actual impact position.
		if(!VectorCompare(start, currentStart))
		{//didn't do trace with original start point.  Recalculate the real fraction before we do our comparision.
			tr->fraction = CalcTraceFraction(start, end, tr->endpos);
		}

		if(tr->fraction < closestFraction)
		{//current impact was the closest impact.
			TraceCopy(tr, &closestTrace);
			closestFraction = tr->fraction;
		}

		//remove the last hit entity from the trace and try again.
		if(!AddRealTraceContent(currentEntityNum))
		{//crap!  The data structure is full.  We're done.
			break;
		}

		//move our start trace point up to the point where we hit the bbox for the last ghoul2/saber object.
		VectorCopy( currentEndPos, currentStart );
	}

	return Finish_RealTrace(tr, &closestTrace, start, end);
}
//[/SaberSys]


float WP_SaberBladeLength( saberInfo_t *saber )
{//return largest length
	int	i;
	float len = 0.0f;
	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].lengthMax > len )
		{
			len = saber->blade[i].lengthMax; 
		}
	}
	return len;
}

float WP_SaberLength( gentity_t *ent )
{//return largest length
	if ( !ent || !ent->client )
	{
		return 0.0f;
	}
	else
	{
		int i;
		float len, bestLen = 0.0f;
		for ( i = 0; i < MAX_SABERS; i++ )
		{
			len = WP_SaberBladeLength( &ent->client->saber[i] );
			if ( len > bestLen )
			{
				bestLen = len;
			}
		}
		return bestLen;
	}
}
int WPDEBUG_SaberColor( saber_colors_t saberColor )
{
	switch( (int)(saberColor) )
	{
		case SABER_RED:
			return 0x000000ff;
			break;
		case SABER_ORANGE:
			return 0x000088ff;
			break;
		case SABER_YELLOW:
			return 0x0000ffff;
			break;
		case SABER_GREEN:
			return 0x0000ff00;
			break;
		case SABER_BLUE:
			return 0x00ff0000;
			break;
		case SABER_PURPLE:
			return 0x00ff00ff;
			break;
		case SABER_CYAN:
			return 0x00ffff00;
			break;
		default:
			return 0x00ffffff;//white
			break;
	}
}
/*
WP_SabersIntersect

Breaks the two saber paths into 2 tris each and tests each tri for the first saber path against each of the other saber path's tris

FIXME: subdivide the arc into a consistant increment
FIXME: test the intersection to see if the sabers really did intersect (weren't going in the same direction and/or passed through same point at different times)?
*/
extern qboolean tri_tri_intersect(vec3_t V0,vec3_t V1,vec3_t V2,vec3_t U0,vec3_t U1,vec3_t U2);
#define SABER_EXTRAPOLATE_DIST 16.0f
qboolean WP_SabersIntersect( gentity_t *ent1, int ent1SaberNum, int ent1BladeNum, gentity_t *ent2, qboolean checkDir )
{
	vec3_t	saberBase1, saberTip1, saberBaseNext1, saberTipNext1;
	vec3_t	saberBase2, saberTip2, saberBaseNext2, saberTipNext2;
	int		ent2SaberNum = 0, ent2BladeNum = 0;
	vec3_t	dir;

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( BG_SabersOff( &ent1->client->ps )
		|| BG_SabersOff( &ent2->client->ps ) )
	{
		return qfalse;
	}

	for ( ent2SaberNum = 0; ent2SaberNum < MAX_SABERS; ent2SaberNum++ )
	{
		if ( ent2->client->saber[ent2SaberNum].type != SABER_NONE )
		{
			for ( ent2BladeNum = 0; ent2BladeNum < ent2->client->saber[ent2SaberNum].numBlades; ent2BladeNum++ )
			{
				if ( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].lengthMax > 0 )
				{//valid saber and this blade is on
					//if ( ent1->client->saberInFlight )
					{
						VectorCopy( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointOld, saberBase1 );
						VectorCopy( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, saberBaseNext1 );

						VectorSubtract( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointOld, dir );
						VectorNormalize( dir );
						VectorMA( saberBaseNext1, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext1 );

						VectorMA( saberBase1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].lengthMax+SABER_EXTRAPOLATE_DIST, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzleDirOld, saberTip1 );
						VectorMA( saberBaseNext1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].lengthMax+SABER_EXTRAPOLATE_DIST, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, saberTipNext1 );

						VectorSubtract( saberTipNext1, saberTip1, dir );
						VectorNormalize( dir );
						VectorMA( saberTipNext1, SABER_EXTRAPOLATE_DIST, dir, saberTipNext1 );
					}
					/*
					else
					{
						VectorCopy( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, saberBase1 );
						VectorCopy( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointNext, saberBaseNext1 );
						VectorMA( saberBase1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].lengthMax, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, saberTip1 );
						VectorMA( saberBaseNext1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].lengthMax, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzleDirNext, saberTipNext1 );
					}
					*/

					//if ( ent2->client->saberInFlight )
					{
						VectorCopy( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointOld, saberBase2 );
						VectorCopy( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, saberBaseNext2 );

						VectorSubtract( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointOld, dir );
						VectorNormalize( dir );
						VectorMA( saberBaseNext2, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext2 );

						VectorMA( saberBase2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].lengthMax+SABER_EXTRAPOLATE_DIST, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzleDirOld, saberTip2 );
						VectorMA( saberBaseNext2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].lengthMax+SABER_EXTRAPOLATE_DIST, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir, saberTipNext2 );

						VectorSubtract( saberTipNext2, saberTip2, dir );
						VectorNormalize( dir );
						VectorMA( saberTipNext2, SABER_EXTRAPOLATE_DIST, dir, saberTipNext2 );
					}
					/*
					else
					{
						VectorCopy( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, saberBase2 );
						VectorCopy( ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointNext, saberBaseNext2 );
						VectorMA( saberBase2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].lengthMax, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir, saberTip2 );
						VectorMA( saberBaseNext2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].lengthMax, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzleDirNext, saberTipNext2 );
					}
					*/
					if ( checkDir )
					{//check the direction of the two swings to make sure the sabers are swinging towards each other
						vec3_t saberDir1, saberDir2;
						float dot = 0.0f;

						VectorSubtract( saberTipNext1, saberTip1, saberDir1 );
						VectorSubtract( saberTipNext2, saberTip2, saberDir2 );
						VectorNormalize( saberDir1 );
						VectorNormalize( saberDir2 );
						if ( DotProduct( saberDir1, saberDir2 ) > 0.6f )
						{//sabers moving in same dir, probably didn't actually hit
							continue;
						}
						//now check orientation of sabers, make sure they're not parallel or close to it
						dot = DotProduct( ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir );
						if ( dot > 0.9f || dot < -0.9f )
						{//too parallel to really block effectively?
							continue;
						}
					}

#ifdef DEBUG_SABER_BOX
					if ( g_saberDebugBox.integer == 2 || g_saberDebugBox.integer == 4 )
					{
						G_TestLine(saberBase1, saberTip1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].color, 500);
						G_TestLine(saberTip1, saberTipNext1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].color, 500);
						G_TestLine(saberTipNext1, saberBase1, ent1->client->saber[ent1SaberNum].blade[ent1BladeNum].color, 500);

						G_TestLine(saberBase2, saberTip2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].color, 500);
						G_TestLine(saberTip2, saberTipNext2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].color, 500);
						G_TestLine(saberTipNext2, saberBase2, ent2->client->saber[ent2SaberNum].blade[ent2BladeNum].color, 500);
					}
#endif
					if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberBaseNext2 ) )
					{
						return qtrue;
					}
					if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberTipNext2 ) )
					{
						return qtrue;
					}
					if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberBaseNext2 ) )
					{
						return qtrue;
					}
					if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberTipNext2 ) )
					{
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

GAME_INLINE int G_PowerLevelForSaberAnim( gentity_t *ent, int saberNum, qboolean mySaberHit )
{
	if ( !ent || !ent->client || saberNum >= MAX_SABERS )
	{
		return FORCE_LEVEL_0;
	}
	else
	{
		int anim = ent->client->ps.torsoAnim;
		int	animTimer = ent->client->ps.torsoTimer;
		int	animTimeElapsed = BG_AnimLength( ent->localAnimIndex, (animNumber_t)anim ) - animTimer;
		saberInfo_t *saber = &ent->client->saber[saberNum];
		if ( anim >= BOTH_A1_T__B_ && anim <= BOTH_D1_B____ )
		{
			//FIXME: these two need their own style
			if ( saber->type == SABER_LANCE )
			{
				return FORCE_LEVEL_4;
			}
			else if ( saber->type == SABER_TRIDENT )
			{
				return FORCE_LEVEL_3;
			}
			return FORCE_LEVEL_1;
		}
		if ( anim >= BOTH_A2_T__B_ && anim <= BOTH_D2_B____ )
		{
			return FORCE_LEVEL_2;
		}
		if ( anim >= BOTH_A3_T__B_ && anim <= BOTH_D3_B____ )
		{
			return FORCE_LEVEL_3;
		}
		if ( anim >= BOTH_A4_T__B_ && anim <= BOTH_D4_B____ )
		{//desann
			return FORCE_LEVEL_4;
		}
		if ( anim >= BOTH_A5_T__B_ && anim <= BOTH_D5_B____ )
		{//tavion
			return FORCE_LEVEL_2;
		}
		if ( anim >= BOTH_A6_T__B_ && anim <= BOTH_D6_B____ )
		{//dual
			return FORCE_LEVEL_2;
		}
		if ( anim >= BOTH_A7_T__B_ && anim <= BOTH_D7_B____ )
		{//staff
			return FORCE_LEVEL_2;
		}
		if ( anim >= BOTH_P1_S1_T_ && anim <= BOTH_H1_S1_BR )
		{//parries, knockaways and broken parries
			return FORCE_LEVEL_1;//FIXME: saberAnimLevel?
		}
		switch ( anim )
		{
		case BOTH_A2_STABBACK1:
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimer < 450 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 400 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_ATTACK_BACK:
			if ( animTimer < 500 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_CROUCHATTACKBACK1:
			if ( animTimer < 800 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_BUTTERFLY_LEFT:
		case BOTH_BUTTERFLY_RIGHT:
		case BOTH_BUTTERFLY_FL1:
		case BOTH_BUTTERFLY_FR1:
			//FIXME: break up?
			return FORCE_LEVEL_3;
			break;
		case BOTH_FJSS_TR_BL:
		case BOTH_FJSS_TL_BR:
			//FIXME: break up?
			return FORCE_LEVEL_3;
			break;
		case BOTH_K1_S1_T_:	//# knockaway saber top
		case BOTH_K1_S1_TR:	//# knockaway saber top right
		case BOTH_K1_S1_TL:	//# knockaway saber top left
		case BOTH_K1_S1_BL:	//# knockaway saber bottom left
		case BOTH_K1_S1_B_:	//# knockaway saber bottom
		case BOTH_K1_S1_BR:	//# knockaway saber bottom right
			//FIXME: break up?
			return FORCE_LEVEL_3;
			break;
		case BOTH_LUNGE2_B__T_:
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimer < 400 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 150 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_FORCELEAP2_T__B_:
			if ( animTimer < 400 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 550 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_VS_ATR_S:
		case BOTH_VS_ATL_S:
		case BOTH_VT_ATR_S:
		case BOTH_VT_ATL_S:
			return FORCE_LEVEL_3;//???
			break;
		case BOTH_JUMPFLIPSLASHDOWN1:
			if ( animTimer <= 1000 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 600 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_JUMPFLIPSTABDOWN:
			if ( animTimer <= 1300 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed <= 300 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_JUMPATTACK6:
			/*
			if (pm->ps)
			{
				if ( ( pm->ps->legsAnimTimer >= 1450
						&& BG_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, BOTH_JUMPATTACK6 ) - pm->ps->legsAnimTimer >= 400 ) 
					||(pm->ps->legsAnimTimer >= 400
						&& BG_AnimLength( g_entities[ps->clientNum].client->clientInfo.animFileIndex, BOTH_JUMPATTACK6 ) - pm->ps->legsAnimTimer >= 1100 ) )
				{//pretty much sideways
					return FORCE_LEVEL_3;
				}
			}
			*/
			if ( ( animTimer >= 1450
					&& animTimeElapsed >= 400 )
				||(animTimer >= 400
					&& animTimeElapsed >= 1100 ) )
			{//pretty much sideways
				return FORCE_LEVEL_3;
			}
			return FORCE_LEVEL_0;
			break;
		case BOTH_JUMPATTACK7:
			if ( animTimer <= 1200 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 200 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_SPINATTACK6:
			if ( animTimeElapsed <= 200 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_2;//FORCE_LEVEL_3;
			break;
		case BOTH_SPINATTACK7:
			if ( animTimer <= 500 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 500 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_2;//FORCE_LEVEL_3;
			break;
		case BOTH_FORCELONGLEAP_ATTACK:
			if ( animTimeElapsed <= 200 )
			{//1st four frames of anim
				return FORCE_LEVEL_3;
			}
			break;
		/*
		case BOTH_A7_KICK_F://these kicks attack, too
		case BOTH_A7_KICK_B:
		case BOTH_A7_KICK_R:
		case BOTH_A7_KICK_L:
			//FIXME: break up
			return FORCE_LEVEL_3;
			break;
		*/
		case BOTH_STABDOWN:
			if ( animTimer <= 900 )
			{//end of anim
				return FORCE_LEVEL_3;
			}
			break;
		case BOTH_STABDOWN_STAFF:
			if ( animTimer <= 850 )
			{//end of anim
				return FORCE_LEVEL_3;
			}
			break;
		case BOTH_STABDOWN_DUAL:
			if ( animTimer <= 900 )
			{//end of anim
				return FORCE_LEVEL_3;
			}
			break;
		case BOTH_A6_SABERPROTECT:
			if ( animTimer < 650 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 250 )
			{//start of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A7_SOULCAL:
			if ( animTimer < 650 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 600 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A1_SPECIAL:
			if ( animTimer < 600 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 200 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A2_SPECIAL:
			if ( animTimer < 300 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 200 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A3_SPECIAL:
			if ( animTimer < 700 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 200 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_FLIP_ATTACK7:
			return FORCE_LEVEL_3;
			break;
		case BOTH_PULL_IMPALE_STAB:
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimer < 1000 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_PULL_IMPALE_SWING:
			if ( animTimer < 500 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 650 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_ALORA_SPIN_SLASH:
			if ( animTimer < 900 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 250 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A6_FB:
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimer < 250 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 250 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A6_LR:	
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimer < 250 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 250 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_3;
			break;
		case BOTH_A7_HILT:
			return FORCE_LEVEL_0;
			break;
	//===SABERLOCK SUPERBREAKS START===========================================================================
		case BOTH_LK_S_DL_T_SB_1_W:
			if ( animTimer < 700 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_S_ST_S_SB_1_W:
			if ( animTimer < 300 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_S_DL_S_SB_1_W:
		case BOTH_LK_S_S_S_SB_1_W:
			if ( animTimer < 700 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 400 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_S_ST_T_SB_1_W:
		case BOTH_LK_S_S_T_SB_1_W:
			if ( animTimer < 150 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 400 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_DL_DL_T_SB_1_W:
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_DL_DL_S_SB_1_W:
		case BOTH_LK_DL_ST_S_SB_1_W:
			if ( animTimeElapsed < 1000 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_DL_ST_T_SB_1_W:
			if ( animTimer < 950 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 650 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_DL_S_S_SB_1_W:
			if ( saberNum != 0 )
			{//only right hand saber does damage in this suberbreak
				return FORCE_LEVEL_0;
			}
			if ( animTimer < 900 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 450 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_DL_S_T_SB_1_W:
			if ( saberNum != 0 )
			{//only right hand saber does damage in this suberbreak
				return FORCE_LEVEL_0;
			}
			if ( animTimer < 250 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 150 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_ST_DL_S_SB_1_W:
			return FORCE_LEVEL_5;
			break;
			
		case BOTH_LK_ST_DL_T_SB_1_W:
			//special suberbreak - doesn't kill, just kicks them backwards
			return FORCE_LEVEL_0;
			break; 
		case BOTH_LK_ST_ST_S_SB_1_W:
		case BOTH_LK_ST_S_S_SB_1_W:
			if ( animTimer < 800 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 350 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			return FORCE_LEVEL_5;
			break;
		case BOTH_LK_ST_ST_T_SB_1_W:
		case BOTH_LK_ST_S_T_SB_1_W:
			return FORCE_LEVEL_5;
			break;
	//===SABERLOCK SUPERBREAKS START===========================================================================
		case BOTH_HANG_ATTACK:
			//FIME: break up
			if ( animTimer < 1000 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else if ( animTimeElapsed < 250 )
			{//beginning of anim
				return FORCE_LEVEL_0;
			}
			else
			{//sweet spot
				return FORCE_LEVEL_5;
			}
			break;
		case BOTH_ROLL_STAB:
			if ( mySaberHit )
			{//someone else hit my saber, not asking for damage level, but defense strength
				return FORCE_LEVEL_1;
			}
			if ( animTimeElapsed > 400 )
			{//end of anim
				return FORCE_LEVEL_0;
			}
			else
			{
				return FORCE_LEVEL_3;
			}
			break;
		}
		return FORCE_LEVEL_0;
	}
}

#define MAX_SABER_VICTIMS 16
static int		victimEntityNum[MAX_SABER_VICTIMS];
static qboolean victimHitEffectDone[MAX_SABER_VICTIMS];
static float	totalDmg[MAX_SABER_VICTIMS];
static vec3_t	dmgDir[MAX_SABER_VICTIMS];
static vec3_t	dmgSpot[MAX_SABER_VICTIMS];
static qboolean dismemberDmg[MAX_SABER_VICTIMS];
static int saberKnockbackFlags[MAX_SABER_VICTIMS];
static int numVictims = 0;
void WP_SaberClearDamage( void )
{
	int ven;
	for ( ven = 0; ven < MAX_SABER_VICTIMS; ven++ )
	{
		victimEntityNum[ven] = ENTITYNUM_NONE;
	}
	memset( victimHitEffectDone, 0, sizeof( victimHitEffectDone ) );
	memset( totalDmg, 0, sizeof( totalDmg ) );
	memset( dmgDir, 0, sizeof( dmgDir ) );
	memset( dmgSpot, 0, sizeof( dmgSpot ) );
	memset( dismemberDmg, 0, sizeof( dismemberDmg ) );
	memset( saberKnockbackFlags, 0, sizeof( saberKnockbackFlags ) );
	numVictims = 0;
}

void WP_SaberDamageAdd( int trVictimEntityNum, vec3_t trDmgDir, vec3_t trDmgSpot, 
					   int trDmg, qboolean doDismemberment, int knockBackFlags )
{
	if ( trVictimEntityNum < 0 || trVictimEntityNum >= ENTITYNUM_WORLD )
	{
		return;
	}
	
	if ( trDmg )
	{//did some damage to something
		int curVictim = 0;
		int i;

		for ( i = 0; i < numVictims; i++ )
		{
			if ( victimEntityNum[i] == trVictimEntityNum )
			{//already hit this guy before
				curVictim = i;
				break;
			}
		}
		if ( i == numVictims )
		{//haven't hit his guy before
			if ( numVictims + 1 >= MAX_SABER_VICTIMS )
			{//can't add another victim at this time
				return;
			}
			//add a new victim to the list
			curVictim = numVictims;
			victimEntityNum[numVictims++] = trVictimEntityNum;
		}

		totalDmg[curVictim] += trDmg;
		if ( VectorCompare( dmgDir[curVictim], vec3_origin ) )
		{
			VectorCopy( trDmgDir, dmgDir[curVictim] );
		}
		if ( VectorCompare( dmgSpot[curVictim], vec3_origin ) )
		{
			VectorCopy( trDmgSpot, dmgSpot[curVictim] );
		}
		if ( doDismemberment )
		{
			dismemberDmg[curVictim] = qtrue;
		}
		saberKnockbackFlags[curVictim] |= knockBackFlags;
	}
}

void WP_SaberApplyDamage( gentity_t *self )
{
	int i;
	if ( !numVictims )
	{
		return;
	}
	for ( i = 0; i < numVictims; i++ )
	{
		gentity_t *victim = NULL;
		int dflags = 0;

		victim = &g_entities[victimEntityNum[i]];

// nmckenzie: SABER_DAMAGE_WALLS
		if ( !victim->client )
		{
			totalDmg[i] *= g_saberWallDamageScale.value;
		}

		if ( !dismemberDmg[i] )
		{//don't do dismemberment!
			dflags |= DAMAGE_NO_DISMEMBER;
		}
		dflags |= saberKnockbackFlags[i];

		G_Damage( victim, self, self, dmgDir[i], dmgSpot[i], totalDmg[i], dflags, MOD_SABER );
	}
}


//[SaberSys]
void WP_SaberSpecificDoHit ( gentity_t *self, int saberNum, int bladeNum, gentity_t *victim, vec3_t impactpoint, int dmg )
{
	gentity_t *te = NULL;
	qboolean isDroid = qfalse;

	if ( victim->client )
	{
		class_t npc_class = victim->client->NPC_class;

		if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE || npc_class == CLASS_REMOTE ||
					npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
					npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )  
			{ //don't make "blood" sparks for droids.
				isDroid = qtrue;
			}
		}

		te = G_TempEntity( impactpoint, EV_SABER_HIT );
		if ( te )
		{
			te->s.otherEntityNum = victim->s.number;
			te->s.otherEntityNum2 = self->s.number;
			te->s.weapon = saberNum;
			te->s.legsAnim = bladeNum;

			VectorCopy(impactpoint, te->s.origin);
			//VectorCopy(tr.plane.normal, te->s.angles);
			VectorScale( impactpoint, -1, te->s.angles);
			
			if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
			{ //don't let it play with no direction
				te->s.angles[1] = 1;
			}

			if (!isDroid && (victim->client || victim->s.eType == ET_NPC ||
				victim->s.eType == ET_BODY))
			{
				if ( dmg < 5 )
				{
					te->s.eventParm = 3;
				}
				else if (dmg < 20 )
				{
					te->s.eventParm = 2;
				}
				else
				{
					te->s.eventParm = 1;
				}
			}
			else
			{
				if ( !WP_SaberBladeUseSecondBladeStyle( &self->client->saber[saberNum], bladeNum )
					&& (self->client->saber[saberNum].saberFlags2&SFL2_NO_CLASH_FLARE) )
				{//don't do clash flare
				}
				else if ( WP_SaberBladeUseSecondBladeStyle( &self->client->saber[saberNum], bladeNum )
					&& (self->client->saber[saberNum].saberFlags2&SFL2_NO_CLASH_FLARE2) )
				{//don't do clash flare
				}
				else
				{
					if (dmg > SABER_NONATTACK_DAMAGE)
					{ //I suppose I could tie this into the saberblock event, but I'm tired of adding flags to that thing.
						gentity_t *teS = G_TempEntity( te->s.origin, EV_SABER_CLASHFLARE );
						VectorCopy(te->s.origin, teS->s.origin);
					}
					te->s.eventParm = 0;
				}
			}
		}
}
//[/SaberSys]


void WP_SaberDoHit( gentity_t *self, int saberNum, int bladeNum )
{
	int i;
	if ( !numVictims )
	{
		return;
	}
	for ( i = 0; i < numVictims; i++ )
	{
		gentity_t *te = NULL, *victim = NULL;
		qboolean isDroid = qfalse;

		if ( victimHitEffectDone[i] )
		{
			continue;
		}

		victimHitEffectDone[i] = qtrue;

		victim = &g_entities[victimEntityNum[i]];

		if ( victim->client )
		{
			class_t npc_class = victim->client->NPC_class;

			if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE || npc_class == CLASS_REMOTE ||
					npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
					npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )  
			{ //don't make "blood" sparks for droids.
				isDroid = qtrue;
			}
		}

		te = G_TempEntity( dmgSpot[i], EV_SABER_HIT );
		if ( te )
		{
			te->s.otherEntityNum = victimEntityNum[i];
			te->s.otherEntityNum2 = self->s.number;
			te->s.weapon = saberNum;
			te->s.legsAnim = bladeNum;

			VectorCopy(dmgSpot[i], te->s.origin);
			//VectorCopy(tr.plane.normal, te->s.angles);
			VectorScale( dmgDir[i], -1, te->s.angles);
			
			if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
			{ //don't let it play with no direction
				te->s.angles[1] = 1;
			}

			if (!isDroid && (victim->client || victim->s.eType == ET_NPC ||
				victim->s.eType == ET_BODY))
			{
				if ( totalDmg[i] < 5 )
				{
					te->s.eventParm = 3;
				}
				else if (totalDmg[i] < 20 )
				{
					te->s.eventParm = 2;
				}
				else
				{
					te->s.eventParm = 1;
				}
			}
			else
			{
				if ( !WP_SaberBladeUseSecondBladeStyle( &self->client->saber[saberNum], bladeNum )
					&& (self->client->saber[saberNum].saberFlags2&SFL2_NO_CLASH_FLARE) )
				{//don't do clash flare
				}
				else if ( WP_SaberBladeUseSecondBladeStyle( &self->client->saber[saberNum], bladeNum )
					&& (self->client->saber[saberNum].saberFlags2&SFL2_NO_CLASH_FLARE2) )
				{//don't do clash flare
				}
				else
				{
					if (totalDmg[i] > SABER_NONATTACK_DAMAGE)
					{ //I suppose I could tie this into the saberblock event, but I'm tired of adding flags to that thing.
						gentity_t *teS = G_TempEntity( te->s.origin, EV_SABER_CLASHFLARE );
						VectorCopy(te->s.origin, teS->s.origin);
					}
					te->s.eventParm = 0;
				}
			}
		}
	}
}

extern qboolean G_EntIsBreakable( int entityNum );
//[KnockdownSys]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//extern void G_Knockdown( gentity_t *victim );
//[/KnockdownSys]
void WP_SaberRadiusDamage( gentity_t *ent, vec3_t point, float radius, int damage, float knockBack )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	else if ( radius <= 0.0f || (damage <= 0 && knockBack <= 0) )
	{
		return;
	}
	else
	{
		vec3_t		mins, maxs, entDir;
		int			radiusEnts[128];
		gentity_t	*radiusEnt = NULL;
		int			numEnts, i;
		float		dist;

		//Setup the bbox to search in
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = point[i] - radius;
			maxs[i] = point[i] + radius;
		}

		//Get the number of entities in a given space
		numEnts = trap_EntitiesInBox( mins, maxs, radiusEnts, 128 );

		for ( i = 0; i < numEnts; i++ )
		{
			radiusEnt = &g_entities[radiusEnts[i]];
			if ( !radiusEnt->inuse )
			{
				continue;
			}
			
			if ( radiusEnt == ent )
			{//Skip myself
				continue;
			}
			
			if ( radiusEnt->client == NULL )
			{//must be a client
				if ( G_EntIsBreakable( radiusEnt->s.number ) )
				{//damage breakables within range, but not as much
					G_Damage( radiusEnt, ent, ent, vec3_origin, radiusEnt->r.currentOrigin, 10, 0, MOD_MELEE );
				}
				continue;
			}

			if ( (radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
			{//can't be one being held
				continue;
			}
			
			VectorSubtract( radiusEnt->r.currentOrigin, point, entDir );
			dist = VectorNormalize( entDir );
			if ( dist <= radius )
			{//in range
				if ( damage > 0 )
				{//do damage
					int points = ceil((float)damage*dist/radius);
					G_Damage( radiusEnt, ent, ent, vec3_origin, radiusEnt->r.currentOrigin, points, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
				if ( knockBack > 0 )
				{//do knockback
					if ( radiusEnt->client
						&& radiusEnt->client->NPC_class != CLASS_RANCOR
						&& radiusEnt->client->NPC_class != CLASS_ATST
						&& !(radiusEnt->flags&FL_NO_KNOCKBACK) )//don't throw them back
					{
						float knockbackStr = knockBack*dist/radius;
						entDir[2] += 0.1f;
						VectorNormalize( entDir );
						G_Throw( radiusEnt, entDir, knockbackStr );
						if ( radiusEnt->health > 0 )
						{//still alive
							if ( knockbackStr > 50 )
							{//close enough and knockback high enough to possibly knock down
								if ( dist < (radius*0.5f)
									|| radiusEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
								{//within range of my fist or within ground-shaking range and not in the air
									//[KnockdownSys]
									//ported multi-direction knockdowns from SP.
									//racc - FYI, this appears to be code pulled from Tavion's spector slam, not sure
									//why it's in the saber radius damage stuff.
									G_Knockdown( radiusEnt, ent, entDir, 500, qtrue );
									//G_Knockdown( radiusEnt );//, ent, entDir, 500, qtrue );
									//[/KnockdownSys]
								}
							}
						}
					}
				}
			}
		}
	}
}

static qboolean saberDoClashEffect = qfalse;
static vec3_t saberClashPos = {0};
static vec3_t saberClashNorm = {0};
static int saberClashEventParm = 1;
//[SaberSys]
static int saberClashOther = -1;  //the clientNum for the other player involved in the saber clash.
GAME_INLINE void G_SetViewLock( gentity_t *self, vec3_t impactPos, vec3_t impactNormal );
GAME_INLINE void G_SetViewLockDebounce( gentity_t *self );
//[/SaberSys]
void WP_SaberDoClash( gentity_t *self, int saberNum, int bladeNum )
{
	if ( saberDoClashEffect )
	{
		//[SaberSys]
		gentity_t *otherOwner;
		//[/SaberSys]
		gentity_t *te = G_TempEntity( saberClashPos, EV_SABER_BLOCK );
		VectorCopy(saberClashPos, te->s.origin);
		VectorCopy(saberClashNorm, te->s.angles);
		te->s.eventParm = saberClashEventParm;
		te->s.otherEntityNum2 = self->s.number;
		te->s.weapon = saberNum;
		te->s.legsAnim = bladeNum;

		//[SaberSys]
		if(saberClashOther != -1 && PM_SaberInParry(g_entities[saberClashOther].client->ps.saberMove))
		{//only viewlock if we're passed valid other otherOwner, DD -- And if we parryed
			otherOwner = &g_entities[saberClashOther];

			G_SetViewLock(self, saberClashPos, saberClashNorm);
			G_SetViewLockDebounce(self);

			G_SetViewLock(otherOwner, saberClashPos, saberClashNorm);
			G_SetViewLockDebounce(otherOwner);
		}
		//[/SaberSys]
	}

	//[BugFix5]
	//I think the clasheffect is repeating for each blade on a saber.
	//This is bad, so I'm hacking it to reset saberDoClashEffect to prevent duplicate clashes
	//from occuring.
	saberDoClashEffect = qfalse;
	//[/BugFix5]
}

void WP_SaberBounceSound( gentity_t *ent, int saberNum, int bladeNum )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	index = Q_irand( 1, 4 );
	if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->saber[saberNum], bladeNum )
		&& ent->client->saber[saberNum].bounceSound[0] )
	{
		G_Sound( ent, CHAN_AUTO, ent->client->saber[saberNum].bounceSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->saber[saberNum], bladeNum )
		&& ent->client->saber[saberNum].bounce2Sound[0] )
	{
		G_Sound( ent, CHAN_AUTO, ent->client->saber[saberNum].bounce2Sound[Q_irand( 0, 2 )] );
	}
	else if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->saber[saberNum], bladeNum )
		&& ent->client->saber[saberNum].blockSound[0] )
	{
		G_Sound( ent, CHAN_AUTO, ent->client->saber[saberNum].blockSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->saber[saberNum], bladeNum )
		&& ent->client->saber[saberNum].block2Sound[0] )
	{
		G_Sound( ent, CHAN_AUTO, ent->client->saber[saberNum].block2Sound[Q_irand( 0, 2 )] );
	}
	else
	{	
		G_Sound( ent, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", index ) ) );
	}
}



//[SaberSys]
//Not used anymore.
//static qboolean saberHitWall = qfalse;
//static qboolean saberHitSaber = qfalse;
//static float saberHitFraction = 1.0f;
//[/SaberSys]

//rww - MP version of the saber damage function. This is where all the things like blocking, triggering a parry,
//triggering a broken parry, doing actual damage, etc. are done for the saber. It doesn't resemble the SP
//version very much, but functionality is (hopefully) about the same.
//This is a large function. I feel sort of bad inlining it. But it does get called tons of times per frame.
//[SaberSys] moved this prototype up for ojp_SaberCanBlock
/*
qboolean BG_SuperBreakWinAnim( int anim );
*/
//[/SaberSys]

//[SaberSys]
GAME_INLINE void G_SetViewLockDebounce( gentity_t *self )
{
	if(!WalkCheck(self))
	{//running pauses you longer
		self->client->viewLockTime = level.time + 500;
	}
	else if( PM_SaberInParry(G_GetParryForBlock(self->client->ps.saberBlocked)) //normal block (not a parry)
		|| (!PM_SaberInKnockaway(self->client->ps.saberMove) //didn't parry
		&& self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE]*.50) )
	{//normal block or attacked with less than %50 DP
		self->client->viewLockTime = level.time + 300;
	}
	else
	{
        self->client->viewLockTime = level.time;
	}
}


GAME_INLINE void G_SetViewLock( gentity_t *self, vec3_t impactPos, vec3_t impactNormal )
{//Sets the view/movement lock flags based on the given information
	vec3_t	cross;
	vec3_t	length;
	vec3_t  forward;
	vec3_t  right;
	vec3_t  up;

	if( !self || !self->client )
	{
		return;
	}

	//Since this is the only function that sets/unsets these flags.  We need to clear them
	//all before messing with them.  Otherwise we end up with all the flags piling up until
	//they are cleared.
	self->client->ps.userInt1 = 0;

	if( VectorCompare( impactNormal, vec3_origin ) 
		|| self->client->ps.saberInFlight )
	{//bad impact surface normal or our saber is in flgiht
		return;
	}

	//find the impact point in terms of the player origin
	VectorSubtract( impactPos, self->client->ps.origin, length );

	//Check for very low hits.  If it's very close to the lower bbox edge just skip view/move locking.  
	//This is to prevent issues with many of the saber moves touching the ground naturally.
	if ( length[2] < (.8 * self->r.mins[2]))
	{
		return;
	}

	CrossProduct( length, impactNormal, cross );

	if( cross[0] > 0 )
	{
		self->client->ps.userInt1 |= LOCK_UP;
	}
	else if ( cross[0] < 0 )
	{
		self->client->ps.userInt1 |= LOCK_DOWN;
	}

	if( cross[2] > 0 )
	{
		self->client->ps.userInt1 |= LOCK_RIGHT;
	}
	else if ( cross[2] < 0 )
	{
		self->client->ps.userInt1 |= LOCK_LEFT;
	}


	//Movement lock

	//Forward
	AngleVectors(self->client->ps.viewangles, forward, right, up);
	if ( DotProduct(length, forward) < 0 )
	{//lock backwards
		self->client->ps.userInt1 |= LOCK_MOVEBACK;
	}
	else if ( DotProduct(length, forward) > 0 )
	{//lock forwards
		self->client->ps.userInt1 |= LOCK_MOVEFORWARD;
	}

	if ( DotProduct(length, right) < 0 )
	{
		self->client->ps.userInt1 |= LOCK_MOVELEFT;
	}
	else if ( DotProduct(length, right) > 0 )
	{
		self->client->ps.userInt1 |= LOCK_MOVERIGHT;
	}

	if ( DotProduct(length, up) > 0 )
	{
		self->client->ps.userInt1 |= LOCK_MOVEDOWN;
	}
	else if ( DotProduct(length, up) < 0 )
	{
		self->client->ps.userInt1 |= LOCK_MOVEUP;
	}
}

 		
//[SaberSys]


GAME_INLINE void AnimateKnockaway( gentity_t *self, gentity_t * inflictor, vec3_t impact )
{//do an knockaway.
	if( !PM_SaberInKnockaway(self->client->ps.saberMove) && !PM_InKnockDown(&self->client->ps) )
	{
		if (!PM_SaberInParry(self->client->ps.saberMove))
		{
			WP_SaberBlockNonRandom(self, impact, qfalse);
			self->client->ps.saberMove = BG_KnockawayForParry( self->client->ps.saberBlocked );
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
		else
		{
			self->client->ps.saberMove = G_KnockawayForParry(self->client->ps.saberMove); //BG_KnockawayForParry( otherOwner->client->ps.saberBlocked );
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
	}
}



extern void NPC_SetPainEvent( gentity_t *self );
extern void NPC_Pain(gentity_t *self, gentity_t *attacker, int damage);
void AnimateStun( gentity_t *self, gentity_t * inflictor, vec3_t impact )
{//place self into a stunned state.
	//RAFIXME - I need to figure out a way to make non-saberers stumble.
	if(self->client->ps.weapon != WP_SABER)
	{//knock them down instead
		G_Knockdown(self, inflictor, vec3_origin, 300, qtrue);
	}
	else if( !PM_SaberInBrokenParry(self->client->ps.saberMove) 
		&& !PM_InKnockDown(&self->client->ps) )
	{
		if (!PM_SaberInParry(G_GetParryForBlock(self->client->ps.saberBlocked)))
		{//not already in a parry position, get one
			WP_SaberBlockNonRandom(self, impact, qfalse);
		}
					
		self->client->ps.saberMove = BG_BrokenParryForParry(G_GetParryForBlock(self->client->ps.saberBlocked));
		self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;

		//make pain noise
		if ( self->s.number < MAX_CLIENTS )
		{
			G_AddEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0 );
		}
		else
		{//npc
			NPC_Pain( self, inflictor, 0 );
		}
	}
}


void AnimateKnockdown( gentity_t * self, gentity_t * inflictor )
{
	if (self->health > 0 && !PM_InKnockDown(&self->client->ps))
	{ //if still alive knock them down
		//[KnockdownSys]
		//use SP style knockdown
		int throwStr = Q_irand(50, 75);

		//push person away from attacker
		vec3_t pushDir;

		if(inflictor)
		{
			VectorSubtract(self->client->ps.origin, inflictor->r.currentOrigin, pushDir);
		}
		else
		{//this is possible in debug situations or where there's mishaps without another player (IE rarely)
			AngleVectors(self->client->ps.viewangles, pushDir, NULL, NULL);
			//reverse it
			VectorScale(pushDir, -1, pushDir);
		}
		pushDir[2] = 0;
		VectorNormalize(pushDir);
		G_Throw(self, pushDir, throwStr);

		//translate throw strength to the force used for determining knockdown anim.
		if(throwStr > 65)
		{//really got nailed, play the hard version of the knockdown anims
			throwStr = 300;
		}	

		//knock them on their ass!
		G_Knockdown(self, inflictor, pushDir, throwStr, qtrue);

		/* the old basejka way
		self->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
		self->client->ps.forceHandExtendTime = level.time + 1300;
		*/
		//[/KnockdownSys]

		//Count as kill for attacker if the other player falls to his death.
		if( inflictor && inflictor->inuse && inflictor->client )
		{
			self->client->ps.otherKiller = self->s.number;
			self->client->ps.otherKillerTime = level.time + 8000;
			self->client->ps.otherKillerDebounceTime = level.time + 100;
		}
	}
}


qboolean PM_RunningAnim( int anim );
//Check to see if the player is actually walking or just standing
qboolean /*GAME_INLINE*/ WalkCheck( gentity_t * self )
{
	if(!self->client)
	{
		return qfalse;
	}

	if(PM_RunningAnim(self->client->ps.legsAnim))
	{
		return qfalse;
	}

	/* old method.  seems to have issues failing players when they're walking diagonally. :|
	float velocity = VectorLength(self->client->ps.velocity);
	if (velocity == 0)
	{
		return qtrue;
	}

	if( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//count as walking!
		return qtrue;
	}

	if(velocity >= (g_speed.value * (.37)) )
	{//on the ground and moving at run speeds.
		//G_Printf("Player %i is running: %f\n", self->s.number, velocity);
		return qfalse;
	}
	*/

	return qtrue;
}
//[/SaberSys]

//[DodgeSys]
void DoNormalDodge(gentity_t *self, int dodgeAnim, qboolean partial)
{//have self go into the given dodgeAnim				   
	//Our own happy way of forcing an anim:
	self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
	self->client->ps.forceDodgeAnim = dodgeAnim;
	self->client->ps.forceHandExtendTime = level.time + 300;
	self->client->ps.weaponTime = 300;
	self->client->ps.saberMove = LS_NONE;

	if(!partial)
	{//only do the after image during full dodges.
		self->client->ps.powerups[PW_SPEEDBURST] = level.time + 100;
	}

	G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );
}


qboolean DodgeRollCheck(gentity_t *self, int dodgeAnim, vec3_t forward, vec3_t right)
{//checks to see if there's a cliff in the direction that dodgeAnim would travel.
	vec3_t mins, maxs, traceto_mod, tracefrom_mod, moveDir;
	trace_t tr;
	int contents;
	int dodgeDistance = 200;//how far away do we do the scans.

	//determine which direction we're travelling in.
	if( dodgeAnim == BOTH_ROLL_F || dodgeAnim == BOTH_HOP_F )
	{//forward rolls
		VectorCopy(forward, moveDir);
	}
	else if( dodgeAnim == BOTH_ROLL_B || dodgeAnim == BOTH_HOP_B )
	{//backward rolls
		VectorCopy(forward, moveDir);
		VectorScale(moveDir, -1, moveDir);
	}
	else if( dodgeAnim == BOTH_GETUP_BROLL_R 
		|| dodgeAnim == BOTH_GETUP_FROLL_R
		|| dodgeAnim == BOTH_ROLL_R
		|| dodgeAnim == BOTH_HOP_R )
	{//right rolls
		VectorCopy(right, moveDir);
	}
	else if( dodgeAnim == BOTH_GETUP_BROLL_L 
		|| dodgeAnim == BOTH_GETUP_FROLL_L
		|| dodgeAnim == BOTH_ROLL_L
		|| dodgeAnim == BOTH_HOP_L )
	{//left rolls
		VectorCopy(right, moveDir);
		VectorScale(moveDir, -1, moveDir);
	}
	else
	{
		G_Printf("Error: unknown dodge roll animation %i given to DodgeRollFallCheck.\n", dodgeAnim);
		return qfalse;
	}

	if( dodgeAnim == BOTH_HOP_L || dodgeAnim == BOTH_HOP_R )
	{//left/right hops are shorter
		dodgeDistance = 120;
	}
		
	//set up the trace positions
	VectorCopy(self->client->ps.origin, tracefrom_mod);
	VectorMA(tracefrom_mod, dodgeDistance, moveDir, traceto_mod);

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -10; //was -18, changed because bots were hopping over the flag stands
					//on ctf1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	//check for solids/or players in the way.
	trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, self->client->ps.clientNum, 
		MASK_PLAYERSOLID);
	if (tr.fraction != 1 || tr.startsolid)
	{//something is in the way.
		return qfalse;
	}

	VectorCopy(traceto_mod, tracefrom_mod);


	//check for 20+ feet drops
	traceto_mod[2] -= 200;

	trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, self->client->ps.clientNum, 
		MASK_SOLID);
	if (tr.fraction == 1 && !tr.startsolid)
	{//CLIFF!
		return qfalse;
	}

	contents = trap_PointContents( tr.endpos, -1 );
	if(contents & (CONTENTS_SLIME|CONTENTS_LAVA))
	{//the fall point is inside something we don't want to fall to
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_HopAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern int DetermineDisruptorCharge(gentity_t *ent);
//Returns qfalse if hit effects/damage is still suppose to be applied.
qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod )
{
	int	dodgeAnim = -1;
	int dpcost = BasicDodgeCosts[mod];

	//saved copy of the original damage level.  This is used determine the kickback power for slashdamage dodges.
	int savedDmg = *dmg; 

	//partial dodge flag
	qboolean partial = qfalse;

	//Don't do any visuals.
	qboolean NoAction = qfalse;
/*
	if( !ojp_allowBodyDodge.integer )
	{//body dodges have been disabled.  
		return qfalse;
	}
*/


	if(self->NPC 
		&& (self->client->NPC_class == CLASS_SABER_DROID ||
			self->client->NPC_class == CLASS_ASSASSIN_DROID ||
			self->client->NPC_class == CLASS_GONK ||
			self->client->NPC_class == CLASS_MOUSE ||
			self->client->NPC_class == CLASS_PROBE ||
			self->client->NPC_class == CLASS_PROTOCOL ||
			self->client->NPC_class == CLASS_R2D2 ||
			self->client->NPC_class == CLASS_R5D2 ||
			self->client->NPC_class == CLASS_SEEKER  ||
			self->client->NPC_class == CLASS_INTERROGATOR) )
	{//non-humans don't get Dodge.
		return qfalse;
	}

	if( dpcost == -1 )
	{//can't dodge this.
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Can't Dodge this type of damage.\n", 
					level.time, self->s.number);
		}
		return qfalse;
	}

	if(!self || !self->client || !self->inuse || self->health <= 0 )
	{		
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Entity is dead or bad.  Can't Dodge.\n", 
					level.time, self->s.number);
		}
		return qfalse;
	}
	//[DodgeChange]
	if (self->client->ps.stats[STAT_DODGE] < 30)
	{//Not enough dodge
		return qfalse;
	}
	//[/DodgeChange]
	//check for private duel conditions
	if (shooter && shooter->client)
	{
		if(shooter->client->ps.duelInProgress && shooter->client->ps.duelIndex != self->s.number)
		{//enemy client is in duel with someone else.
			return qfalse;
		}
	
		if (self->client->ps.duelInProgress && self->client->ps.duelIndex != shooter->s.number)
		{//we're in a duel with someone else.
			return qfalse;
		}
	}

	if(BG_HopAnim(self->client->ps.legsAnim) //in dodge hop
		|| (BG_InRoll(&self->client->ps, self->client->ps.legsAnim) 
			&& (self->client->ps.userInt3 & (1 << FLAG_DODGEROLL)))  //in dodge roll
			//dodging non-saber damage and in dodge animation
			|| (mod != MOD_SABER && self->client->ps.forceHandExtend == HANDEXTEND_DODGE) )
	{//already doing a dodge
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i continues Dodge Roll.\n", level.time, self->s.number );
		}
		return qtrue;
	}

	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE 
		&& mod != MOD_REPEATER_ALT_SPLASH 
		&& mod != MOD_FLECHETTE_ALT_SPLASH
		&& mod != MOD_ROCKET_SPLASH
		&& mod != MOD_ROCKET_HOMING_SPLASH
		&& mod != MOD_THERMAL_SPLASH
		&& mod != MOD_TRIP_MINE_SPLASH
		&& mod != MOD_TIMED_MINE_SPLASH
		&& mod != MOD_DET_PACK_SPLASH 
		&& mod != MOD_INCINERATOR_EXPLOSION_SPLASH
		&& mod != MOD_DIOXIS_EXPLOSION_SPLASH
		&& mod != MOD_FREEZER_EXPLOSION_SPLASH 
		&& mod != MOD_ION_EXPLOSION_SPLASH)
	{//can't dodge direct fire in mid-air
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Can't dodge in mid air.\n", level.time, self->s.number);
		}
		return qfalse;
	}

	if ( self->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| BG_InGrappleMove(self->client->ps.torsoAnim) > 1 )
	{//in some effect that stops me from moving on my own
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Being held, can't dodge.\n", level.time, self->s.number);
		}
		return qfalse;
	}

	if(mod == MOD_MELEE)
	{//don't dodge melee attacks for now.
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Can't dodge melee damage\n", level.time, self->s.number);
		}
		return qfalse;
	}

	if(self->client->ps.legsAnim == BOTH_MEDITATE)
	{//can't dodge while meditating.
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Meditating, Can't Dodge.\n", level.time, self->s.number);
		}
		return qfalse;
	}

	if(mod == MOD_SABER && shooter && shooter->client)
	{//special saber moves have special effects.
		if(shooter->client->ps.saberMove == LS_A_LUNGE 
				|| shooter->client->ps.saberMove == LS_SPINATTACK
				|| shooter->client->ps.saberMove == LS_SPINATTACK_DUAL)
		{//attacker is doing lunge special
			if(self->client->ps.userInt3 & (1 << FLAG_FATIGUED_HEAVY))
			{//can't dodge a lunge special while fatigued
				return qfalse;
			}
		}

		if(BG_SuperBreakWinAnim(shooter->client->ps.torsoAnim) && self->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
		{//can't block super breaks if we're low on DP.
			return qfalse;
		}

		if(!WalkCheck(self)
			&& (!InFront(shooter->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -.7f)
			|| BG_SaberInAttack( self->client->ps.saberMove )
			|| PM_SaberInStart( self->client->ps.saberMove )))
		{
			if(self->NPC)
			{//can't Dodge saber swings while running and hit from behind or in swing.
				return qtrue;
			}
			else
			{//can't Dodge saber swings while running and hit from behind or in swing.
				return qfalse;
			}
		}
	}

	if( *dmg <= DODGE_MINDAM && mod != MOD_REPEATER )
	{
		dpcost = (int)(dpcost * ((float) *dmg/DODGE_MINDAM));
		NoAction = qtrue;
	}
	else if ( self->client->ps.forceHandExtend == HANDEXTEND_DODGE )
	{//you're already dodging but you got hit again.
		dpcost = 0;
	}

	if(mod == MOD_DISRUPTOR_SNIPER && shooter && shooter->client)
	{//25,50,75,100
	 //15,30,45,60
		int damage=0;

		if(shooter->genericValue6 <= 10)
			damage=10;
		else if(shooter->genericValue6 <= 25)
			damage=20;
		else if(shooter->genericValue6 < 59)
			damage=50;
		else if(shooter->genericValue6 == 60)
			damage=100;

		if(self->client->ps.stats[STAT_DODGE] > damage)
			dpcost=damage;
		else
			return qfalse;
		/*
		switch(shooter->client->skillLevel[SK_DISRUPTOR])
		{
		case FORCE_LEVEL_3:
			return qfalse;
		case FORCE_LEVEL_2:
			if(self->client->ps.stats[STAT_DODGE] >100)
				dpcost=100;
			else
				return qfalse;
			break;
		case FORCE_LEVEL_1:
			if(self->client->ps.stats[STAT_DODGE] >50)
				dpcost=50;
			else
				return qfalse;
			break;
			//G_Damage(self,shooter,shooter,NULL,NULL,200,DAMAGE_NO_ARMOR,MOD_DISRUPTOR_SNIPER);
			//break;
		}
		return qtrue;
		//G_DodgeDrain(self,shooter,200);
		*/
	}

	if(dpcost < self->client->ps.stats[STAT_DODGE])
	{
		//[ExpSys]
		G_DodgeDrain(self, shooter, dpcost);
		//self->client->ps.stats[STAT_DODGE] -= dpcost; 
		//[/ExpSys]
	}
	else if( dpcost != 0 && self->client->ps.stats[STAT_DODGE])
	{//still have enough DP for a partial dodge.
		//Scale damage as is approprate
		*dmg =(int)(*dmg * (self->client->ps.stats[STAT_DODGE]/ (float) dpcost));
		//[ExpSys]
		G_DodgeDrain(self, shooter, self->client->ps.stats[STAT_DODGE]);
		//self->client->ps.stats[STAT_DODGE] = 0;
		//[/ExpSys]
		partial = qtrue;
	}
	else
	{//not enough DP left
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i out of DP.  Couldn't Dodge.\n", level.time, self->s.number);
		}
		return qfalse;
	}

	if(NoAction)
	{
		if(partial)
		{
			if(g_debugdodge.integer)
			{
				G_Printf("%i: Client %i No-Action Partially Dodged %i points of damage.\n", 
					level.time, self->s.number, *dmg);
			}
			return qfalse;
		}
		else
		{
			if(g_debugdodge.integer)
			{
				G_Printf("%i: Client %i No-Action Dodged %i points of damage.\n", 
					level.time, self->s.number, *dmg);
			}
			return qtrue;
		}
	}


	if(mod == MOD_REPEATER_ALT_SPLASH 
		|| mod == MOD_FLECHETTE_ALT_SPLASH
		|| mod == MOD_ROCKET_SPLASH
		|| mod == MOD_ROCKET_HOMING_SPLASH
		|| mod == MOD_THERMAL_SPLASH
		|| mod == MOD_TRIP_MINE_SPLASH
		|| mod == MOD_TIMED_MINE_SPLASH
		|| mod == MOD_DET_PACK_SPLASH
		|| mod == MOD_ROCKET
		|| mod == MOD_INCINERATOR_EXPLOSION_SPLASH
		|| mod == MOD_DIOXIS_EXPLOSION_SPLASH
		|| mod == MOD_FREEZER_EXPLOSION_SPLASH 
		|| mod == MOD_ION_EXPLOSION_SPLASH)
	{//splash damage dodge, dodged by throwing oneself away from the blast into a knockdown
		vec3_t blowBackDir;//[ExplosivesKnockback]
		int blowBackPower = savedDmg;
		VectorSubtract(self->client->ps.origin, dmgOrigin, blowBackDir);
		VectorNormalize(blowBackDir);

		if(blowBackPower > 100)
		{//clamp blow back power level 
			blowBackPower = 100;
		}
		blowBackPower*=2;
		G_Throw( self, blowBackDir, blowBackPower );
		G_Knockdown(self, shooter, blowBackDir, 600, qtrue); 
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Splash Dodged %i points of damage.\n", level.time, self->s.number, *dmg);
		}

		if(partial)
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}


	/*===========================================================================
	doing a positional dodge for direct hit damage (like sabers or blaster bolts)
	===========================================================================*/
	if(hitLoc == -1)
	{//Use the last surface impact data as the hit location
		//[BugFix12]
		if ((d_saberGhoul2Collision.integer && self->client 
			&& self->client->g2LastSurfaceModel == G2MODEL_PLAYER
			&& self->client->g2LastSurfaceTime == level.time))

		//if ((d_saberGhoul2Collision.integer && self->client && self->client->g2LastSurfaceTime == level.time))
		//[/BugFix12]
		{
			char hitSurface[MAX_QPATH];

			trap_G2API_GetSurfaceName(self->ghoul2, self->client->g2LastSurfaceHit, 0, hitSurface);

			if (hitSurface[0])
			{
				G_GetHitLocFromSurfName(self, hitSurface, &hitLoc, dmgOrigin, vec3_origin, vec3_origin, MOD_SABER);
			}
		}
		else
		{//ok, that didn't work.  Try the old math way.
			hitLoc = G_GetHitLocation(self, dmgOrigin);
		}
	}

	switch(hitLoc)
	{
	case HL_NONE:
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Dodge failed:  Bad hitlocation given.\n", 
				level.time, self->s.number);
		}
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
		dodgeAnim = BOTH_JUMP1;
		break;

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
		if(g_debugdodge.integer)
		{
			G_Printf("%i: Client %i Dodge failed:  Bad hitlocation given.\n", 
				level.time, self->s.number);
		}
		return qfalse;
	}

	if ( dodgeAnim != -1 )
	{
		if( self->client->ps.forceHandExtend != HANDEXTEND_DODGE //not already in a dodge
			&& !PM_InKnockDown(&self->client->ps) )
		{//do a simple dodge
			DoNormalDodge(self, dodgeAnim, partial);
		}
		else
		{//can't just do a simple dodge
			int rolled;
			vec3_t tangles;
			vec3_t forward;
			vec3_t right;
			float fdot;
			float rdot;
			vec3_t impactpoint;
			int x;
			int FallCheckMax;

			VectorSubtract(dmgOrigin, self->client->ps.origin, impactpoint);
			VectorNormalize(impactpoint);

			VectorSet(tangles, 0, self->client->ps.viewangles[YAW], 0);

			AngleVectors(tangles, forward, right, NULL);
				
			fdot = DotProduct(impactpoint, forward);
			rdot = DotProduct(impactpoint, right);

			if( PM_InKnockDown(&self->client->ps) )
			{//ground dodge roll
				FallCheckMax = 2;
				if(rdot < 0)
				{//Right
					if ( self->client->ps.legsAnim == BOTH_KNOCKDOWN3 
						|| self->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| self->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L )
					{
						rolled = BOTH_GETUP_FROLL_R;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_R;
					}
				}
				else
				{//left
					if ( self->client->ps.legsAnim == BOTH_KNOCKDOWN3 
						|| self->client->ps.legsAnim == BOTH_KNOCKDOWN5
						|| self->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L )
					{
						rolled = BOTH_GETUP_FROLL_L;
					}
					else
					{
						rolled = BOTH_GETUP_BROLL_L;
					}
				}
			}
			else
			{//normal Dodge rolls.
				FallCheckMax = 4;
				if( fabs(fdot) > fabs(rdot) )
				{//Forward/Back
					if(fdot < 0 )
					{//Forward
						rolled = BOTH_HOP_F; //BOTH_ROLL_F;
					}
					else
					{//Back
						rolled = BOTH_HOP_B; //BOTH_ROLL_B;
					}
				}
				else
				{//Right/Left
					if(rdot < 0)
					{//Right
						rolled = BOTH_HOP_R; //BOTH_ROLL_R;
					}
					else
					{//Left
						rolled = BOTH_HOP_L;//BOTH_ROLL_L;
					}
				}
			}

			for(x = 0; x < FallCheckMax; x++)
			{//check for a valid rolling direction..namely someplace where we
				//won't roll off cliffs
				if(DodgeRollCheck(self, rolled, forward, right))
				{//passed check
					break;
				}

				//otherwise, rotate to try the next possible direction
				//reset to start of possible moves if we're at the end of the list
				//for the perspective roll type.
				if(rolled == BOTH_GETUP_BROLL_R)
				{//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_L;
				}
				else if(rolled == BOTH_GETUP_BROLL_L)
				{//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_BROLL_R;
				}
				else if(rolled == BOTH_GETUP_FROLL_R)
				{//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_L;
				}
				else if(rolled == BOTH_GETUP_FROLL_L)
				{//there's only two possible evasions in this mode.
					rolled = BOTH_GETUP_FROLL_R;
				}
				else if ( rolled == BOTH_ROLL_R )
				{//reset to start of the possible normal roll directions
					rolled = BOTH_ROLL_F;
				}
				else if ( rolled == BOTH_HOP_R )
				{//reset to the start of possible hop directions
					rolled = BOTH_HOP_F;
				}
				else
				{//just advance the roll move
					rolled++;
				}
			}

			if( x == FallCheckMax )
			{//we don't have a valid position to dodge roll to. just do a normal dodge
				DoNormalDodge(self, dodgeAnim, partial);
			}
			else
			{//ok, we can do the dodge hops/rolls
				if( PM_InKnockDown(&self->client->ps)
					|| BG_HopAnim(rolled))
				{//ground dodge roll
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, rolled, 
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
				}
				else
				{//normal dodge roll
					self->client->ps.legsTimer = 0;
					self->client->ps.legsAnim = 0;
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH,rolled,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 150);
					G_AddEvent(self, EV_ROLL, 0 );
					self->r.maxs[2] = self->client->ps.crouchheight;//CROUCH_MAXS_2;
					self->client->ps.viewheight = DEFAULT_VIEWHEIGHT;
					self->client->ps.pm_flags &= ~PMF_DUCKED;
					self->client->ps.pm_flags |= PMF_ROLLING;			
				}

				//set the dodge roll flag
				self->client->ps.userInt3 |= (1 << FLAG_DODGEROLL);

				//set weapontime
				self->client->ps.weaponTime = self->client->ps.torsoTimer;

				//clear out the old dodge move.
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
				self->client->ps.forceDodgeAnim = 0;
				self->client->ps.forceHandExtendTime = 0;
				self->client->ps.saberMove = LS_NONE;

				//do dodge special effects.
				if(!partial)
				{//only do the after image during full dodges.
					self->client->ps.powerups[PW_SPEEDBURST] = level.time + self->client->ps.legsTimer;
				}
			
				G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.wav") );	
			}
		}
		if( partial && *dmg > 0)
		{
			if(g_debugdodge.integer)
			{
				G_Printf("%i: Client %i Partially Dodged %i points of damage.\n", level.time, self->s.number, *dmg);
			}
			return qfalse;
		}
		else
		{
			if(g_debugdodge.integer)
			{
				G_Printf("%i: Client %i Dodged %i points of damage.\n", level.time, self->s.number, *dmg);
			}
			return qtrue;
		}
			
	}
	return qfalse;
}
//[/DodgeSys]

//[SaberSys]
//racc - OJP Enhanced completely rewritten CheckSaberDamage function.  This is the heart of the saber system beast.
void SabBeh_SetupSaberMechanics( gentity_t *self, sabmech_t *mechSelf, 
								gentity_t *otherOwner, sabmech_t *mechOther, vec3_t hitLoc, 
								qboolean *didHit, qboolean otherHitSaberBlade );
extern qboolean BG_SaberInTransitionDamageMove( playerState_t *ps );
extern qboolean BG_SaberInFullDamageMove( playerState_t *ps, int AnimIndex );
extern qboolean BG_InSlowBounce(playerState_t *ps);
void DebounceSaberImpact(gentity_t *self, gentity_t *otherSaberer, 
						 int rSaberNum, int rBladeNum, int sabimpactentitynum);
void OJP_SetSlowBounce(gentity_t* self, gentity_t* attacker);
qboolean G_InAttackParry(gentity_t *self);
GAME_INLINE qboolean CheckSaberDamage(gentity_t *self, int rSaberNum, int rBladeNum, vec3_t saberStart, vec3_t saberEnd, qboolean doInterpolate, int trMask, qboolean extrapolate )
{
	static trace_t tr;
	static vec3_t dir;
	static vec3_t saberTrMins, saberTrMaxs;
//	static vec3_t lastValidStart;
//	static vec3_t lastValidEnd;
	static int selfSaberLevel;
//	static int otherSaberLevel;
	int dmg = 0;
	float saberBoxSize = d_saberBoxTraceSize.value;
	qboolean idleDamage = qfalse;
	qboolean didHit = qfalse;
	qboolean hitSaberBlade = qfalse;  //this indicates weither an saber blade was hit vs just using the auto-block system.

	sabmech_t	mechSelf;		//saber mechanics data for self
	
	//saber mechanics data for the other dude with a lightsaber (if any)
	sabmech_t	mechOther;

	//passthru flag.  Saber passthru only occurs if you successful chopped the target
	//to death, in half, etc.
	qboolean	passthru = qfalse;

	//holds the return code from our realTrace.  This is used to help 
	//determine if you actually it the saber blade for hitSaberBlade
	int realTraceResult;  

	//sabimpactdebounce
	int sabimpactdebounce;
	int sabimpactentitynum;

	//self doing a damage move
	qboolean	DamageMove = qfalse;

	qboolean	saberHitWall = qfalse;

	gentity_t *otherOwner = NULL;

	if (BG_SabersOff( &self->client->ps ))
	{
		//register as a hit so we don't do a lot of interplotation. 
		return qtrue;
	}

	selfSaberLevel = G_SaberAttackPower(self, SaberAttacking(self));

	//Add the standard radius into the box size
	saberBoxSize += (self->client->saber[rSaberNum].blade[rBladeNum].radius*0.5f);

	//Setting things up so the game always does realistic box traces for the sabers.
	VectorSet(saberTrMins, -saberBoxSize, -saberBoxSize, -saberBoxSize);
	VectorSet(saberTrMaxs, saberBoxSize, saberBoxSize, saberBoxSize);


	realTraceResult = G_RealTrace(self, &tr, saberStart, saberTrMins, saberTrMaxs, saberEnd, 
		self->s.number, trMask, rSaberNum, rBladeNum);


	if ( tr.fraction == 1 && !tr.startsolid )
	{
		return qfalse;
	}
	/* 
	//racc - I've modified the super duper interpolation so that it checks for possible collisions at the starting area
	//before tracing thru to the current position.  As such, you'll only get startsolid on that initial trace and if that's 
	//the case, we want to count that as a true impact anyway.  Therefore, this bit of code is no longer needed.
	else if ( tr.startsolid && !extrapolate && d_saberInterpolate.integer == 2)
	{//ok, we started inside an object and we're using the super duper interpolation system
		return 2;
	}
	*/


	//added damage setting for thrown sabers
	if ( self->client->ps.saberInFlight && rSaberNum== 0 && self->client->ps.saberAttackWound < level.time )
	{
		dmg = DAMAGE_THROWN;
	}
	else if ( BG_SaberInFullDamageMove(&self->client->ps, self->localAnimIndex) )
	//if ( self->client->ps.saberAttackWound < level.time
	{//full damage moves
		dmg = DAMAGE_ATTACK;
		DamageMove = qtrue;
	}
	else if( BG_SaberInTransitionDamageMove(&self->client->ps) )
	{//use idle damage for transition moves.  
		//Playtesting has indicated that using attack damage for transition moves results in unfair insta-hits.
		dmg = SABER_NONATTACK_DAMAGE;
		idleDamage = qtrue;
	}
	else
	{//idle saber damage
		dmg = SABER_NONATTACK_DAMAGE;
		idleDamage = qtrue;
	}


	if (!dmg)
	{
		if (tr.entityNum < MAX_CLIENTS ||
			(g_entities[tr.entityNum].inuse && (g_entities[tr.entityNum].r.contents & CONTENTS_LIGHTSABER)))
		{
			return qtrue;
		}
		return qfalse;
	}

	if (dmg > SABER_NONATTACK_DAMAGE)
	{
		dmg *= g_saberDamageScale.value;

		//see if this specific saber has a damagescale
		if ( !WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
			&& self->client->saber[rSaberNum].damageScale != 1.0f )
		{
			dmg = ceil( (float)dmg*self->client->saber[rSaberNum].damageScale );
		}
		else if ( WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
			&& self->client->saber[rSaberNum].damageScale2 != 1.0f )
		{
			dmg = ceil( (float)dmg*self->client->saber[rSaberNum].damageScale2 );
		}

		if ((self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)) ||
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //weaken it if an arm is broken
			dmg *= 0.3;
			if (dmg <= SABER_NONATTACK_DAMAGE)
			{
				dmg = SABER_NONATTACK_DAMAGE+1;
			}
		}
	}

	if (dmg > SABER_NONATTACK_DAMAGE && self->client->ps.isJediMaster)
	{ //give the Jedi Master more saber attack power
		dmg *= 2;
	}

	if (dmg > SABER_NONATTACK_DAMAGE && g_gametype.integer == GT_SIEGE &&
		self->client->siegeClass != -1 && (bgSiegeClasses[self->client->siegeClass].classflags & (1<<CFL_MORESABERDMG)))
	{ //this class is flagged to do extra saber damage. I guess 2x will do for now.
		dmg *= 2;
	}

	if (g_gametype.integer == GT_POWERDUEL &&
		self->client->sess.duelTeam == DUELTEAM_LONE)
	{ //always x2 when we're powerdueling alone... er, so, we apparently no longer want this?  So they say.
		if ( g_duel_fraglimit.integer )
		{
			//dmg *= 1.5 - (.4 * (float)self->client->sess.wins / (float)g_duel_fraglimit.integer);
				
		}
		//dmg *= 2;
	}

	//SABERSYSRAFIXME - do we still want this sort of dealie?
	//disabling the saber movement bonus/penalty stuff for now.
	//dmg = SaberMoveDamage(self, &tr, dmg);

	VectorSubtract(saberEnd, saberStart, dir);
	VectorNormalize(dir);

	if (tr.entityNum == ENTITYNUM_WORLD ||
		g_entities[tr.entityNum].s.eType == ET_TERRAIN)
	{ //register this as a wall hit for jedi AI
        self->client->ps.saberEventFlags |= SEF_HITWALL;
		saberHitWall = qtrue;
	}

	//if(tr.entityNum == ENTITYNUM_WORLD)
		//return qfalse;

	if (g_entities[tr.entityNum].takedamage &&
		(g_entities[tr.entityNum].health > 0 || !(g_entities[tr.entityNum].s.eFlags & EF_DISINTEGRATION)) &&
		tr.entityNum != self->s.number &&
		g_entities[tr.entityNum].inuse)
	{//hit something that had health and takes damage

		otherOwner = &g_entities[tr.entityNum];

		/* //racc - we don't check for friendly fire here now since it might interfere with friendly fire strikes 
			//still causing blocks/bounces.
		if (idleDamage &&
			g_entities[tr.entityNum].client &&
			OnSameTeam(self, &g_entities[tr.entityNum]) &&
			!g_friendlySaber.integer)
		{
			if(!idleDamage)
			{//bounce the saber.
				self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
				//G_Printf("Attacker Saber Bounced.\n");
			}
			return qfalse;
		}
		*/

		if (g_entities[tr.entityNum].client &&
			g_entities[tr.entityNum].client->ps.duelInProgress &&
			g_entities[tr.entityNum].client->ps.duelIndex != self->s.number)
		{
			return qfalse;
		}

		if (g_entities[tr.entityNum].client &&
			self->client->ps.duelInProgress &&
			self->client->ps.duelIndex != g_entities[tr.entityNum].s.number)
		{
			return qfalse;
		}

		if(OJP_SaberCanBlock(&g_entities[tr.entityNum], self, qfalse, tr.endpos, -1, -1))
		{//hit victim is able to block, block!
			didHit = qfalse;
			otherOwner = &g_entities[tr.entityNum];
			WP_SaberBlockNonRandom(otherOwner, tr.endpos, qfalse);
			//G_Printf("Hit Player Saber Blocked the Attack on the Body %i\n", self->s.number); 
		}
		else
		{//hit something that can actually take damage
			didHit = qtrue;
			otherOwner = NULL;
		}

	}
	else if (((g_entities[tr.entityNum].r.contents & CONTENTS_LIGHTSABER) &&
		g_entities[tr.entityNum].r.contents != -1 &&
		g_entities[tr.entityNum].inuse))
	{ //hit a saber blade
		otherOwner = &g_entities[g_entities[tr.entityNum].r.ownerNum];
		//G_Printf("Hit saber!\n");
		if (!otherOwner->inuse || !otherOwner->client)
		{//Bad defender saber owner state
			return qfalse;
		}

		if (otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != self->s.number)
		{	
			return qfalse;
		}

		if (self->client->ps.duelInProgress &&
			self->client->ps.duelIndex != otherOwner->s.number)
		{		
			return qfalse;
		}

		if(otherOwner->client->ps.saberInFlight && !OJP_UsingDualSaberAsPrimary(&otherOwner->client->ps))
		{//Hit a thrown saber, deactive it.
			saberCheckKnockdown_Smashed(&g_entities[tr.entityNum], otherOwner, self, dmg);
			otherOwner = NULL;
		}
		else if(realTraceResult == REALTRACE_SABERBLOCKHIT)
		{//this is actually a faked lightsaber hit to make the bounding box saber blocking work.
			//As such, we know that the player can block, set the approprate block position for this attack.
			WP_SaberBlockNonRandom(otherOwner, tr.endpos, qfalse);
		}
		else if(realTraceResult == REALTRACE_HIT)
		{//successfully hit another player's saber blade directly
			hitSaberBlade = qtrue;
		}
	}

	//saber impact debouncer stuff
	if(idleDamage)
	{
		sabimpactdebounce = g_saberDmgDelay_Idle.integer;
	}
	else
	{
		sabimpactdebounce = g_saberDmgDelay_Wound.integer;
	}

	if(otherOwner)
	{
		sabimpactentitynum = otherOwner->client->ps.saberEntityNum;
	}
	else
	{
		sabimpactentitynum = tr.entityNum;
	}

	if(self->client->sabimpact[rSaberNum][rBladeNum].EntityNum == sabimpactentitynum
		&& ((level.time - self->client->sabimpact[rSaberNum][rBladeNum].Debounce) < sabimpactdebounce))
	{//the impact debounce for this entity isn't up yet.
		if( self->client->sabimpact[rSaberNum][rBladeNum].BladeNum != -1 
			|| self->client->sabimpact[rSaberNum][rBladeNum].SaberNum != -1)
		{//the last impact was a saber on this entity
			if(otherOwner)
			{
				if (self->client->sabimpact[rSaberNum][rBladeNum].BladeNum == self->client->lastBladeCollided
					&& self->client->sabimpact[rSaberNum][rBladeNum].SaberNum == self->client->lastSaberCollided)
				{
					return qtrue;
				}
			}
		}
		else
		{//last impact was this saber.
			return qtrue;
		}
	}
	//end saber impact debouncer stuff

	if(otherOwner)
	{//Do the saber clash effects here now.
		saberDoClashEffect = qtrue;
		VectorCopy( tr.endpos, saberClashPos );
		VectorCopy( tr.plane.normal, saberClashNorm );
		saberClashEventParm = 1;
		if(!idleDamage 
			|| BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex) )
		{//only do viewlocks if one player or the other is in an attack move
			saberClashOther = otherOwner->s.number;
			//G_Printf("%i: %i: Saber-on-Saber Impact.\n", level.time, self->s.number);
		}
		else
		{//make the saberClashOther be invalid
			saberClashOther = -1;
		}
	}

	//update/roll mishaps.
	SabBeh_RunSaberBehavior(self, &mechSelf, otherOwner, &mechOther, tr.endpos, &didHit, hitSaberBlade);

	if(didHit && (!OnSameTeam(self, &g_entities[tr.entityNum]) || g_friendlySaber.integer))
	{//deal damage
		//damage the thing we hit
		int dflags=0;
		gentity_t *victim = &g_entities[tr.entityNum];

		if(G_DoDodge( victim, self, tr.endpos, -1, & dmg, MOD_SABER ))
		{
			//I'm going to return qtrue to prevent the system from continueing to try 
			//to find an impact.  That could cause multi impact situations otherwise.
			
			//add saber impact debounce
			DebounceSaberImpact(self, otherOwner, rSaberNum, rBladeNum, sabimpactentitynum);
			return qtrue;
		}
		else
		{//hit!
			//G_Printf("%i: %i:  Caused damage with saber.\n", level.time, self->s.number); 

			//determine if this saber blade does dismemberment or not.
			if ( !WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
				&& !(self->client->saber[rSaberNum].saberFlags2&SFL2_NO_DISMEMBERMENT) )
			{
				dflags |= DAMAGE_NO_DISMEMBER;
			}
			if ( WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
				&& !(self->client->saber[rSaberNum].saberFlags2&SFL2_NO_DISMEMBERMENT2) )
			{
				dflags |= DAMAGE_NO_DISMEMBER;
			}

			if ( !WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
				&& self->client->saber[rSaberNum].knockbackScale > 0.0f )
			{
				if ( rSaberNum < 1 )
				{
					dflags |= DAMAGE_SABER_KNOCKBACK1;
				}
				else
				{
					dflags |= DAMAGE_SABER_KNOCKBACK2;
				}
			}

			if ( WP_SaberBladeUseSecondBladeStyle( &self->client->saber[rSaberNum], rBladeNum )
				&& self->client->saber[rSaberNum].knockbackScale > 0.0f )
			{
				if ( rSaberNum < 1 )
				{
					dflags |= DAMAGE_SABER_KNOCKBACK1_B2;
				}
				else
				{
					dflags |= DAMAGE_SABER_KNOCKBACK2_B2;
				}
			}

			//I don't see the purpose of wall damage scaling...but oh well. :)
			if ( !victim->client )
			{
				float damage = dmg;
				damage *= g_saberWallDamageScale.value;
				dmg = (int) damage;
			}

			//We need the final damage total to know if we need to bounce the saber back or not.
			G_Damage( victim, self, self, dir, tr.endpos, dmg, dflags, MOD_SABER );

			WP_SaberSpecificDoHit( self, rSaberNum, rBladeNum, victim, tr.endpos, dmg );
		
		#ifndef FINAL_BUILD // MJN - define fix :/
			if (g_saberDebugPrint.integer > 2 && dmg > 1)
			{ 
				Com_Printf("Client %i hit Entity %i for %i points of Damage.\n", self->s.number, victim->s.number, dmg);
			}
		#endif	
			if(victim->health <= 0)
			{//The attack killed your opponent, don't bounce the saber back to prevent nasty passthru.
				passthru = qtrue;
			}

			if (victim->client)
			{
				//Let jedi AI know if it hit an enemy
				if ( self->enemy && self->enemy == victim )
				{
					self->client->ps.saberEventFlags |= SEF_HITENEMY;
				}
				else
				{
					self->client->ps.saberEventFlags |= SEF_HITOBJECT;
				}
			}
		}
	}

	//attacker animation stuff
	if(mechSelf.doButterFingers)
	{
		ButterFingers(&g_entities[self->client->ps.saberEntityNum], self, otherOwner, &tr);
	}

	if(self->client->ps.saberInFlight && !OJP_UsingDualSaberAsPrimary(&self->client->ps))
	{//saber in flight and it has hit something.  deactivate it.
		saberCheckKnockdown_Smashed(&g_entities[self->client->ps.saberEntityNum], self, otherOwner, dmg);
	}	
	else if(mechSelf.doKnockdown)
	{
		AnimateKnockdown(self, otherOwner);
	}
	else if (mechSelf.doStun)
	{
		AnimateStun(self, otherOwner, tr.endpos);
	}
	else if(mechSelf.doSlowBounce)
	{
		SabBeh_AnimateSlowBounce(self, otherOwner);
	}
	else if(mechSelf.doHeavySlowBounce)
	{
		SabBeh_AnimateHeavySlowBounce(self, otherOwner);
	}
	else if (mechOther.doStun)
	{//we stunned them, do the knockaway animation
		AnimateKnockaway(self, otherOwner, tr.endpos);
	}
	else if(mechSelf.doParry)
	{
		AnimateKnockaway(self, otherOwner, tr.endpos);
	}
	else
	{//Didn't go into a special animation, so do a saber lock/bounce/deflection/etc that's approprate
		if(otherOwner && otherOwner->inuse && otherOwner->client 
			&& !mechOther.doButterFingers
			&& !mechOther.doHeavySlowBounce
			&& !mechOther.doSlowBounce
			&& !mechOther.doParry
			&& !mechOther.doStun)
		{//saberlock test		
			if (dmg > SABER_NONATTACK_DAMAGE || BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex))
			{
				int lockFactor = g_saberLockFactor.integer;

				if (Q_irand(1, 20) < lockFactor)
				{
					if (WP_SabersCheckLock(self, otherOwner))
					{	
						self->client->ps.userInt3 |= ( 1 << FLAG_SABERLOCK_ATTACKER );
						self->client->ps.saberBlocked = BLOCKED_NONE;
						otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
						//add saber impact debounce
						DebounceSaberImpact(self, otherOwner, rSaberNum, rBladeNum, sabimpactentitynum);
						return qtrue;
					}
				}
			}
		}

		//ok, that didn't happen.  Just bounce the sucker then.
		if(!passthru)
		{
			if(saberHitWall)
			{
				//G_Printf("%i: %i: Saber hit wall\n", level.time, self->s.number);
				if ((self->client->saber[rSaberNum].saberFlags & SFL_BOUNCE_ON_WALLS)
					&& (BG_SaberInAttackPure( self->client->ps.saberMove ) //only in a normal attack anim
					|| self->client->ps.saberMove == LS_A_JUMP_T__B_ ) ) //or in the strong jump-fwd-attack "death from above" move
				{
					//do bounce sound & force feedback
					WP_SaberBounceSound( self, rSaberNum, rBladeNum );
					self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
				}
			}
			else
			{
				if(!idleDamage)
				{
					//RAFIXME - maybe make this a generic function check at some point?
					if( self->client->ps.saberLockTime >= level.time  //in a saberlock
						//otherOwner wasn't in an attack and didn't block the blow.
						|| (otherOwner && otherOwner->inuse && otherOwner->client 
						&& !BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex) //they aren't in an attack swing
						&& otherOwner->client->ps.saberBlocked == BLOCKED_NONE)) //and they didn't block the attack.
					{//don't bounce!
						/*
						G_Printf("%i: %i: Saber passed thru opponent's saber! %s %s\n", level.time, self->s.number, 
							GetStringForID(animTable, self->client->ps.torsoAnim),
							GetStringForID(animTable, otherOwner->client->ps.torsoAnim) );
						*/
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
						//G_Printf("%i: %i: Saber Bounced %s\n", level.time, self->s.number, 
						//	GetStringForID( animTable, self->client->ps.torsoAnim ));
					}
				}
				else
				{
					//G_Printf("%i: %i: Idle Damage Move doesn't do bounce %s\n", level.time, self->s.number, 
					//		GetStringForID(animTable, self->client->ps.torsoAnim) );
				}
			}
		}
		else
		{
			//G_Printf("%i: %i: Attacker Passthru!\n", level.time, self->s.number);
		}
	}

	//defender animations
	if(otherOwner && otherOwner->inuse && otherOwner->client)
	{
		if(mechOther.doButterFingers)
		{
			ButterFingers(&g_entities[otherOwner->client->ps.saberEntityNum], otherOwner, self, &tr);
		}

		if(mechOther.doKnockdown )
		{
			AnimateKnockdown(otherOwner, self);
		}
		else if (mechOther.doStun)
		{
			AnimateStun(otherOwner, self, tr.endpos);
		}
		else if(mechOther.doSlowBounce)
		{
			SabBeh_AnimateSlowBounce(otherOwner, self);
		}
		else if(mechOther.doHeavySlowBounce)
		{
			SabBeh_AnimateHeavySlowBounce(otherOwner, self);
		}
		else if (mechSelf.doStun)
		{//we stunned them, do the knockaway animation
			AnimateKnockaway(otherOwner, self, tr.endpos);
		}
		else if (mechOther.doParry)
		{
			AnimateKnockaway(otherOwner, self, tr.endpos);
		}
		else
		{//Didn't go into a special animation, so do a saber lock/bounce/deflection/etc that's approprate
			if(BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex)  //otherOwner was doing a damaging move
				&& otherOwner->client->ps.saberLockTime < level.time //otherOwner isn't in a saberlock (this can change mid-impact)
				&& (self->client->ps.saberBlocked != BLOCKED_NONE //self reacted to this impact.
						|| !idleDamage) )  //or self was in an attack move (and probably mishaped from this impact)  
			{
				//G_Printf("%i: %i: Saber Bounced %s\n", level.time, otherOwner->s.number, 
				//	GetStringForID( animTable, otherOwner->client->ps.torsoAnim ));
				otherOwner->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
		}
	}

	//add saber impact debounce
	DebounceSaberImpact(self, otherOwner, rSaberNum, rBladeNum, sabimpactentitynum);
	return qtrue;
}
//[/SaberSys]

/*GAME_INLINE int VectorCompare2( const vec3_t v1, const vec3_t v2 ) 
{
	if ( v1[0] > v2[0]+0.0001f || v1[0] < v2[0]-0.0001f
		|| v1[1] > v2[1]+0.0001f || v1[1] < v2[1]-0.0001f
		|| v1[2] > v2[2]+0.0001f || v1[2] < v2[2]-0.0001f ) 
	{
		return 0;
	}			
	return 1;
}*/

#define MAX_SABER_SWING_INC 0.33f

//[SaberSys]
//Not Used Anymore.
/*
void G_SPSaberDamageTraceLerped( gentity_t *self, int saberNum, int bladeNum, vec3_t baseNew, vec3_t endNew, int clipmask )
{
	vec3_t baseOld, endOld;
	vec3_t mp1, mp2;
	vec3_t md1, md2;

	if ( (level.time-self->client->saber[saberNum].blade[bladeNum].trail.lastTime) > 100 )
	{//no valid last pos, use current
		VectorCopy(baseNew, baseOld);
		VectorCopy(endNew, endOld);
	}
	else
	{//trace from last pos
		VectorCopy( self->client->saber[saberNum].blade[bladeNum].trail.base, baseOld );
		VectorCopy( self->client->saber[saberNum].blade[bladeNum].trail.tip, endOld );
	}

	VectorCopy( baseOld, mp1 );
	VectorCopy( baseNew, mp2 );
	VectorSubtract( endOld, baseOld, md1 );
	VectorNormalize( md1 );
	VectorSubtract( endNew, baseNew, md2 );
	VectorNormalize( md2 );

	saberHitWall = qfalse;
	saberHitSaber = qfalse;
	saberHitFraction = 1.0f;
	if ( VectorCompare2( baseOld, baseNew ) && VectorCompare2( endOld, endNew ) )
	{//no diff
		CheckSaberDamage( self, saberNum, bladeNum, baseNew, endNew, qfalse, clipmask, qfalse );
	}
	else
	{//saber moved, lerp
		float step = 8, stepsize = 8;//aveLength, 
		vec3_t	ma1, ma2, md2ang, curBase1, curBase2;
		int	xx;
		vec3_t curMD1, curMD2;//, mdDiff, dirDiff;
		float dirInc, curDirFrac;
		vec3_t baseDiff, bladePointOld, bladePointNew;
		qboolean extrapolate = qtrue;

		//do the trace at the base first
		VectorCopy( baseOld, bladePointOld );
		VectorCopy( baseNew, bladePointNew );
		CheckSaberDamage( self, saberNum, bladeNum, bladePointOld, bladePointNew, qfalse, clipmask, qtrue );

		//if hit a saber, shorten rest of traces to match
		if ( saberHitFraction < 1.0f )
		{
			//adjust muzzleDir...
			vec3_t ma1, ma2;
			vectoangles( md1, ma1 );
			vectoangles( md2, ma2 );
			for ( xx = 0; xx < 3; xx++ )
			{
				md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], saberHitFraction );
			}
			AngleVectors( md2ang, md2, NULL, NULL );
			//shorten the base pos
			VectorSubtract( mp2, mp1, baseDiff );
			VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
			VectorMA( baseNew, self->client->saber[saberNum].blade[bladeNum].lengthMax, md2, endNew );
		}

		//If the angle diff in the blade is high, need to do it in chunks of 33 to avoid flattening of the arc
		if ( BG_SaberInAttack( self->client->ps.saberMove ) 
			|| BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) 
			|| BG_SpinningSaberAnim( self->client->ps.torsoAnim ) 
			|| BG_InSpecialJump( self->client->ps.torsoAnim ) )
			//|| (g_timescale->value<1.0f&&BG_SaberInTransitionAny( ent->client->ps.saberMove )) )
		{
			curDirFrac = DotProduct( md1, md2 );
		}
		else
		{
			curDirFrac = 1.0f;
		}
		//NOTE: if saber spun at least 180 degrees since last damage trace, this is not reliable...!
		if ( fabs(curDirFrac) < 1.0f - MAX_SABER_SWING_INC )
		{//the saber blade spun more than 33 degrees since the last damage trace
			curDirFrac = dirInc = 1.0f/((1.0f - curDirFrac)/MAX_SABER_SWING_INC);
		}
		else
		{
			curDirFrac = 1.0f;
			dirInc = 0.0f;
		}
		//qboolean hit_saber = qfalse;

		vectoangles( md1, ma1 );
		vectoangles( md2, ma2 );

		//VectorSubtract( md2, md1, mdDiff );
		VectorCopy( md1, curMD2 );
		VectorCopy( baseOld, curBase2 );

		while ( 1 )
		{
			VectorCopy( curMD2, curMD1 );
			VectorCopy( curBase2, curBase1 );
			if ( curDirFrac >= 1.0f )
			{
				VectorCopy( md2, curMD2 );
				VectorCopy( baseNew, curBase2 );
			}
			else
			{
				for ( xx = 0; xx < 3; xx++ )
				{
					md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], curDirFrac );
				}
				AngleVectors( md2ang, curMD2, NULL, NULL );
				//VectorMA( md1, curDirFrac, mdDiff, curMD2 );
				VectorSubtract( baseNew, baseOld, baseDiff );
				VectorMA( baseOld, curDirFrac, baseDiff, curBase2 );
			}
			// Move up the blade in intervals of stepsize
			for ( step = stepsize; step <= self->client->saber[saberNum].blade[bladeNum].lengthMax *//*&& step < self->client->saber[saberNum].blade[bladeNum].lengthOld*//*; step += stepsize )
			{
				VectorMA( curBase1, step, curMD1, bladePointOld );
				VectorMA( curBase2, step, curMD2, bladePointNew );
				
				if ( step+stepsize >= self->client->saber[saberNum].blade[bladeNum].lengthMax )
				{
					extrapolate = qfalse;
				}
				//do the damage trace
				CheckSaberDamage( self, saberNum, bladeNum, bladePointOld, bladePointNew, qfalse, clipmask, extrapolate );
				*//*
				if ( WP_SaberDamageForTrace( ent->s.number, bladePointOld, bladePointNew, baseDamage, curMD2, 
					qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qtrue,
					saberNum, bladeNum ) )
				{
					hit_wall = qtrue;
				}
				*/
/*
				//if hit a saber, shorten rest of traces to match
				if ( saberHitFraction < 1.0f )
				{
					vec3_t curMA1, curMA2;
					//adjust muzzle endpoint
					VectorSubtract( mp2, mp1, baseDiff );
					VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
					VectorMA( baseNew, self->client->saber[saberNum].blade[bladeNum].lengthMax, curMD2, endNew );
					//adjust muzzleDir...
					vectoangles( curMD1, curMA1 );
					vectoangles( curMD2, curMA2 );
					for ( xx = 0; xx < 3; xx++ )
					{
						md2ang[xx] = LerpAngle( curMA1[xx], curMA2[xx], saberHitFraction );
					}
					AngleVectors( md2ang, curMD2, NULL, NULL );
					saberHitSaber = qtrue;
				}
				if (saberHitWall)
				{
					break;
				}
			}
			if ( saberHitWall || saberHitSaber )
			{
				break;
			}
			if ( curDirFrac >= 1.0f )
			{
				break;
			}
			else
			{
				curDirFrac += dirInc;
				if ( curDirFrac >= 1.0f )
				{
					curDirFrac = 1.0f;
				}
			}
		}

		//do the trace at the end last
		//Special check- adjust for length of blade not being a multiple of 12
		*//*
		aveLength = (ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld + ent->client->ps.saber[saberNum].blade[bladeNum].length)/2;
		if ( step > aveLength )
		{//less dmg if the last interval was not stepsize
			tipDmgMod = (stepsize-(step-aveLength))/stepsize;
		}
		//NOTE: since this is the tip, we do not extrapolate the extra 16
		if ( WP_SaberDamageForTrace( ent->s.number, endOld, endNew, tipDmgMod*baseDamage, md2, 
			qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qfalse,
			saberNum, bladeNum ) )
		{
			hit_wall = qtrue;
		}
		*//*
	}
}
*/
//[/SaberSys]

qboolean BG_SaberInTransitionAny( int move );

qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower );
qboolean InFOV3( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );
qboolean Jedi_WaitingAmbush( gentity_t *self );
void Jedi_Ambush( gentity_t *self );
evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist );
void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
//[SaberSys]
int BlockedforQuad(int quad);
int InvertQuad(int quad);
//[/SaberSys]
void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd  )
{
	//[SaberSys]
	qboolean	swingBlock;
	qboolean	closestSwingBlock = qfalse;  //default setting makes the compiler happy.
	int 		swingBlockQuad = Q_T;
	int			closestSwingQuad = Q_T;
	//[/SaberSys]
	float		dist;
	gentity_t	*ent, *incoming = NULL;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e;
	float		closestDist, radius = 256;
	vec3_t		forward, dir, missile_dir, fwdangles = {0};
	trace_t		trace;
	vec3_t		traceTo, entDir;
	float		dot1, dot2;
	float		lookTDist = -1;
	gentity_t	*lookT = NULL;
	qboolean	doFullRoutine = qtrue;

	//keep this updated even if we don't get below
	if ( !(self->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
	{//lookTarget is set by and to the monster that's holding you, no other operations can change that
		self->client->ps.hasLookTarget = qfalse;
	}

	if ( self->client->ps.weapon != WP_SABER && self->client->NPC_class != CLASS_BOBAFETT )
	{
		doFullRoutine = qfalse;
	}
	
	else if ( self->client->ps.saberInFlight )
	{
		doFullRoutine = qfalse;
	}
	
		if ( self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1 )
	{//you have not the SKILLZ
		doFullRoutine = qfalse;
	}
	if(!WalkCheck(self)
	&& (BG_SaberInAttack( self->client->ps.saberMove )
	|| PM_SaberInStart( self->client->ps.saberMove )))
	{//this was put in to help bolts stop swings a bit. I dont knwo why it helps but it does :p
		doFullRoutine = qfalse;
	}

	else if ( self->client->ps.fd.forcePowersActive&(1<<FP_LIGHTNING) )
	{//can't block while zapping
		doFullRoutine = qfalse;
	}
	else if ( self->client->ps.fd.forcePowersActive&(1<<FP_TEAM_FORCE) )
	{//can't block while draining
		doFullRoutine = qfalse;
	}
	else if ( self->client->ps.fd.forcePowersActive&(1<<FP_DRAIN) )
	{//can't block while draining
		doFullRoutine = qfalse;
	}
	else if ( self->client->ps.fd.forcePowersActive&(1<<FP_PUSH) )
	{//can't block while shoving
		doFullRoutine = qfalse;
	}
	else if ( self->client->ps.fd.forcePowersActive&(1<<FP_GRIP) )
	{//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		doFullRoutine = qfalse;
	}
	//[SaberSys]
	//you should be able to update block positioning if you're already in a block.
	if (self->client->ps.weaponTime > 0 && !PM_SaberInParry(self->client->ps.saberMove))
	//if (self->client->ps.weaponTime > 0)
	//[/SaberSys]
	{ //don't autoblock while busy with stuff
		return;
	}

	if ( (self->client->saber[0].saberFlags&SFL_NOT_ACTIVE_BLOCKING) )
	{//can't actively block with this saber type
		return;
	}

	if ( self->health <= 0 )
	{//dead don't try to block (NOTE: actual deflection happens in missile code)
		return;
	}
	if ( PM_InKnockDown( &self->client->ps ) )
	{//can't block when knocked down
		return;
	}

	if ( BG_SabersOff( &self->client->ps ) && self->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( self->s.eType != ET_NPC )
		{//player doesn't auto-activate
			doFullRoutine = qfalse;
		}
	}

	if ( self->s.eType == ET_PLAYER )
	{//don't do this if already attacking!
		if ( ucmd->buttons & BUTTON_ATTACK 
			|| BG_SaberInAttack( self->client->ps.saberMove )
			|| BG_SaberInSpecialAttack( self->client->ps.torsoAnim )
			|| BG_SaberInTransitionAny( self->client->ps.saberMove ))
		{
			doFullRoutine = qfalse;
		}
	}

	//[RACC] - I beleive that this has to do with how the bots do blocking.  I'm 
	//leaving it now.
	if ( self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		doFullRoutine = qfalse;
	}

	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = self->r.currentOrigin[i] - radius;
		maxs[i] = self->r.currentOrigin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	closestDist = radius;

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		ent = &g_entities[entityList[ e ]];
		//[SaberSys]
		swingBlock = qfalse;
		//[/SaberSys]

		if (ent == self)
			continue;

		//as long as we're here I'm going to get a looktarget too, I guess. -rww
		if (self->s.eType == ET_PLAYER &&
			ent->client &&
			(ent->s.eType == ET_NPC || ent->s.eType == ET_PLAYER) &&
			!OnSameTeam(ent, self) &&
			ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			!(ent->client->ps.pm_flags & PMF_FOLLOW) &&
			(ent->s.eType != ET_NPC || ent->s.NPC_class != CLASS_VEHICLE) && //don't look at vehicle NPCs
			ent->health > 0)
		{ //seems like a valid enemy to look at.
			vec3_t vecSub;
			float vecLen;

			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, vecSub);
			vecLen = VectorLength(vecSub);

			if (lookTDist == -1 || vecLen < lookTDist)
			{
				trace_t tr;
				vec3_t myEyes;

				VectorCopy(self->client->ps.origin, myEyes);
				myEyes[2] += self->client->ps.viewheight;

				trap_Trace(&tr, myEyes, NULL, NULL, ent->client->ps.origin, self->s.number, MASK_PLAYERSOLID);

				if (tr.fraction == 1.0f || tr.entityNum == ent->s.number)
				{ //we have a clear line of sight to him, so it's all good.
					lookT = ent;
					lookTDist = vecLen;
				}
			}
		}

		//[DodgeSys]
		//moved down so the player will take defensive action even they they can't pre-block
		/*
		if (!doFullRoutine)
		{ //don't care about the rest then
			continue;
		}
		*/
		//[/DodgeSys]

		if (ent->r.ownerNum == self->s.number)
			continue;
		if ( !(ent->inuse) )
			continue;

		if ( ent->s.eType != ET_MISSILE && !(ent->s.eFlags&EF_MISSILE_STICK) )
		{//not a normal projectile
			gentity_t *pOwner;

			if (ent->r.ownerNum < 0 || ent->r.ownerNum >= ENTITYNUM_WORLD)
			{ //not going to be a client then.
				continue;
			}
				
			pOwner = &g_entities[ent->r.ownerNum];

			if (!pOwner->inuse || !pOwner->client)
			{
				continue; //not valid cl owner
			}
			
			if (pOwner->client->sess.sessionTeam == TEAM_SPECTATOR ||
				pOwner->client->tempSpectate >= level.time)
			{ // 74145: ignore specs
				continue;
			}
			if (!pOwner->client->ps.saberEntityNum ||
				//[SaberSys]
				//!pOwner->client->ps.saberInFlight ||
				//[/SaberSys]
				pOwner->client->ps.saberEntityNum != ent->s.number)
			{ //the saber is knocked away and/or not flying actively, or this ent is not the cl's saber ent at all
				continue;
			}

			//[SaberSys]
			//allow the blocking of normal saber swings
			if(!pOwner->client->ps.saberInFlight)	
			{//active saber blade, treat differently.
				swingBlock = qtrue;
				if(BG_SaberInNonIdleDamageMove(&pOwner->client->ps, pOwner->localAnimIndex) )
				{//attacking
					swingBlockQuad = InvertQuad(saberMoveData[pOwner->client->ps.saberMove].startQuad);
				}
				else if(PM_SaberInStart(pOwner->client->ps.saberMove) 
					|| PM_SaberInTransition(pOwner->client->ps.saberMove) )
				{//preparing to attack
					// 74145: Ignore windup / transition, happens too often.
				        //swingBlockQuad = InvertQuad(saberMoveData[pOwner->client->ps.saberMove].endQuad);
					continue;						
				}
				else
				{//not attacking
					continue;
				}
			}
			//[/SaberSys]

			//If we get here then it's ok to be treated as a thrown saber, I guess.
		}
		else
		{
			if ( ent->s.pos.trType == TR_STATIONARY && self->s.eType == ET_PLAYER )
			{//nothing you can do with a stationary missile if you're the player
				continue;
			}
		}

		//see if they're in front of me
		VectorSubtract( ent->r.currentOrigin, self->r.currentOrigin, dir );
		dist = VectorNormalize( dir );

		//[SaberSys]
		if(dist > 150 && swingBlock)
		{//don't block swings that are too far away.
			continue;
		}
		//[/SaberSys]

		//[DodgeSys]
		/* disabled all this stuff for now, kind of buggy and people think it's unbalanced.
		//handle Dodging for explosive bad things		
		if ( ent->splashDamage && ent->splashRadius ) 
		{//this thingy can explode
			if(dist < ent->splashRadius //we've in its blast radius
				&&PreCogDodgeCosts[ent->methodOfDeath] != -1 &&  self->client->ps.stats[STAT_DODGE] > PreCogDodgeCosts[ent->methodOfDeath]) //we can Dodge this thingy
			{//attempt to Dodge this sucker
				if(WP_ForcePowerUsable(self, FP_PUSH)  //can use Force Push
					&& DotProduct( dir, forward ) > SABER_REFLECT_MISSILE_CONE) //in our push field (roughly)
				{//use force push to knock the thingy away.
					ForceThrow( self, qfalse );
					G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[ent->methodOfDeath]);

					//re-add the used up FP points since this should count as a DP cost only.
					self->client->ps.fd.forcePower += forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH];
				}
				else if(WP_ForcePowerUsable(self, FP_LEVITATION))
				{//jump out of the way
					self->client->ps.fd.forceJumpCharge = 480;
					G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[ent->methodOfDeath]);
				}
			}
		
			//done everything we can for an explosive
			continue;
		}
		*/


		/* old method.  borked and messy.
		//FIXME: handle detpacks, proximity mines and tripmines
		if ( ent->s.weapon == WP_THERMAL )
		{//thermal detonator!
			//[DodgeSys]
			//Do Dodge for thermal detonators.
			if ( dist < ent->splashRadius  && !OnSameTeam(&g_entities[ent->r.ownerNum], self))
			//if ( self->NPC && dist < ent->splashRadius )
			{
				if (self->client->ps.stats[STAT_DODGE] > PreCogDodgeCosts[MOD_THERMAL]) 
				{
					if ( dist < ent->splashRadius && 
						ent->nextthink < level.time + 600 && 
						ent->count && 
						self->client->ps.groundEntityNum != ENTITYNUM_NONE && 
							(ent->s.pos.trType == TR_STATIONARY||
							ent->s.pos.trType == TR_INTERPOLATE||
							(dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE||
							//!WP_ForcePowerUsable( self, FP_PUSH )) )
							//racc - replaced with the below since we're going to override push's debounce
							!(self->client->ps.fd.forcePowersKnown & ( 1 << FP_PUSH )) ) ) //don't have force push
					{//TD is close enough to hurt me, I'm on the ground and the thing is at rest or behind me and about to blow up, or I don't have force-push so force-jump!
						//FIXME: sometimes this might make me just jump into it...?
						self->client->ps.fd.forceJumpCharge = 480;
						//[ExpSys]
						G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[MOD_THERMAL]);
						//self->client->ps.stats[STAT_DODGE] -= PreCogDodgeCosts[MOD_THERMAL];
						//[/ExpSys]
					}
					else if ( self->client->NPC_class != CLASS_BOBAFETT )
					{//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
						//racc - make sure that force push has been disabled
						WP_ForcePowerStop(self, FP_PUSH);
						ForceThrow( self, qfalse );
						//[ExpSys]
						G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[MOD_THERMAL]);
						//self->client->ps.stats[STAT_DODGE] -= PreCogDodgeCosts[MOD_THERMAL];
						//[/ExpSys]
						//re-add the used up FP points since this should count as a DP cost only.
						self->client->ps.fd.forcePower += forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH];
					}
				}
			}
			//[/DodgeSys]
			continue;
		}
		else if ( ent->splashDamage && ent->splashRadius )
		{//exploding missile
			//[DodgeSys]
			*//*
			//FIXME: handle tripmines and detpacks somehow... 
			//			maybe do a force-gesture that makes them explode?  
			//			But what if we're within it's splashradius?
			if ( self->s.eType == ET_PLAYER )
			{//players don't auto-handle these at all
				continue;
			}
			else 
			{
			*//*
			if(PreCogDodgeCosts[ent->methodOfDeath] != -1 &&  self->client->ps.stats[STAT_DODGE] > PreCogDodgeCosts[ent->methodOfDeath])
			{
				//if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) 
				//	&& 	self->client->NPC_class != CLASS_BOBAFETT )
				if (0) //Maybe handle this later?
				{//a placed explosive like a tripmine or detpack
					if ( InFOV3( ent->r.currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 90, 90 ) )
					{//in front of me
						if ( G_ClearLOS4( self, ent ) )
						{//can see it
							vec3_t throwDir;
							//make the gesture
							ForceThrow( self, qfalse );
							//take it off the wall and toss it
							ent->s.pos.trType = TR_GRAVITY;
							ent->s.eType = ET_MISSILE;
							ent->s.eFlags &= ~EF_MISSILE_STICK;
							ent->flags |= FL_BOUNCE_HALF;
							AngleVectors( ent->r.currentAngles, throwDir, NULL, NULL );
							VectorMA( ent->r.currentOrigin, ent->r.maxs[0]+4, throwDir, ent->r.currentOrigin );
							VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
							VectorScale( throwDir, 300, ent->s.pos.trDelta );
							ent->s.pos.trDelta[2] += 150;
							VectorMA( ent->s.pos.trDelta, 800, dir, ent->s.pos.trDelta );
							ent->s.pos.trTime = level.time;		// move a bit on the very first frame
							VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
							ent->r.ownerNum = self->s.number;
							// make it explode, but with less damage
							ent->splashDamage /= 3;
							ent->splashRadius /= 3;
							//ent->think = WP_Explode;
							ent->nextthink = level.time + Q_irand( 500, 3000 );
						}
					}
				}
				else if ( dist < ent->splashRadius && 
				self->client->ps.groundEntityNum != ENTITYNUM_NONE && 
					(DotProduct( dir, forward ) < SABER_REFLECT_MISSILE_CONE
					//|| !WP_ForcePowerUsable( self, FP_PUSH )  //racc - replaced with the below since we're going to override push's debounce
					|| !(self->client->ps.fd.forcePowersKnown & ( 1 << FP_PUSH )) ) ) //don't have force push
				{//NPCs try to evade it
					self->client->ps.fd.forceJumpCharge = 480;
					//[ExpSys]
					G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[ent->methodOfDeath]);
					//self->client->ps.stats[STAT_DODGE] -= PreCogDodgeCosts[ent->methodOfDeath];
					//[/ExpSys]
				}
				else if ( self->client->NPC_class != CLASS_BOBAFETT )
				{//else, try to force-throw it away
					//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
					//racc - make sure that force push has been disabled
					WP_ForcePowerStop(self, FP_PUSH);
					ForceThrow( self, qfalse );
					//[ExpSys]
					G_DodgeDrain(self, &g_entities[ent->r.ownerNum], PreCogDodgeCosts[ent->methodOfDeath]);
					//self->client->ps.stats[STAT_DODGE] -= PreCogDodgeCosts[ent->methodOfDeath];
					//[/ExpSys]
					//re-add the used up FP points since this should count as a DP cost only.
					self->client->ps.fd.forcePower += forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH];
				}
			}
			//[/DodgeSys]
			//otherwise, can't block it, so we're screwed
			continue;
		}
		*/
		
		if (!doFullRoutine)
		{ //don't care about the rest then
			continue;
		}
		//[/DodgeSys]

		if ( ent->s.weapon != WP_SABER )
		{//only block shots coming from behind
			if ( (dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE )
				continue;
		}
		//[SaberSys]
		/* racc - don't want this with the swing blocking
		else if ( self->s.eType == ET_PLAYER )
		{//player never auto-blocks thrown sabers
			continue;
		}//NPCs always try to block sabers coming from behind!
		*/

		//see if they're heading towards me
		if(!swingBlock)
		{
			VectorCopy( ent->s.pos.trDelta, missile_dir );
			VectorNormalize( missile_dir );
			if ( (dot2 = DotProduct( dir, missile_dir )) > 0 )
				continue;
		}

		/* basejka
		VectorCopy( ent->s.pos.trDelta, missile_dir );
		VectorNormalize( missile_dir );
		if ( (dot2 = DotProduct( dir, missile_dir )) > 0 )
			continue;
		*/
		//[/SaberSys]

		//FIXME: must have a clear trace to me, too...
		if ( dist < closestDist )
		{
			VectorCopy( self->r.currentOrigin, traceTo );
			traceTo[2] = self->r.absmax[2] - 4;
			trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, traceTo, ent->s.number, ent->clipmask);
			if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
			{//okay, try one more check
				VectorNormalize2( ent->s.pos.trDelta, entDir );
				VectorMA( ent->r.currentOrigin, radius, entDir, traceTo );
				trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, traceTo, ent->s.number, ent->clipmask);
				if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
				{//can't hit me, ignore it
					continue;
				}
			}
			if ( self->s.eType == ET_NPC )
			{//An NPC
				if ( self->NPC && !self->enemy && ent->r.ownerNum != ENTITYNUM_NONE )
				{
					gentity_t *owner = &g_entities[ent->r.ownerNum];
					if ( owner->health >= 0 && (!owner->client || owner->client->playerTeam != self->client->playerTeam) )
					{
						G_SetEnemy( self, owner );
					}
				}
			}
			//FIXME: if NPC, predict the intersection between my current velocity/path and the missile's, see if it intersects my bounding box (+/-saberLength?), don't try to deflect unless it does?
			closestDist = dist;
			incoming = ent;
			//[SaberSys]
			closestSwingBlock = swingBlock;
			closestSwingQuad = swingBlockQuad;
			//[/SaberSys]
		}
	}

	if (self->s.eType == ET_NPC && self->localAnimIndex <= 1)
	{ //humanoid NPCs don't set angles based on server angles for looking, unlike other NPCs
		if (self->client && self->client->renderInfo.lookTarget < ENTITYNUM_WORLD)
		{
			lookT = &g_entities[self->client->renderInfo.lookTarget];
		}
	}

	if (lookT)
	{ //we got a looktarget at some point so we'll assign it then.
		if ( !(self->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
		{//lookTarget is set by and to the monster that's holding you, no other operations can change that
			self->client->ps.hasLookTarget = qtrue;
			self->client->ps.lookTarget = lookT->s.number;
		}
	}

	if (!doFullRoutine)
	{ //then we're done now
		return;
	}

	if ( incoming )
	{
		if ( self->NPC /*&& !G_ControlledByPlayer( self )*/ )
		{
			if ( Jedi_WaitingAmbush( self ) )
			{
				Jedi_Ambush( self );
			}
			if ( self->client->NPC_class == CLASS_BOBAFETT 
				&& (self->client->ps.eFlags2&EF2_FLYING)//moveType == MT_FLYSWIM 
				&& incoming->methodOfDeath != MOD_ROCKET_HOMING )
			{//a hovering Boba Fett, not a tracking rocket
				if ( !Q_irand( 0, 1 ) )
				{//strafe
					self->NPC->standTime = 0;
					self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand( 1000, 2000 );
				}
				if ( !Q_irand( 0, 1 ) )
				{//go up/down
					TIMER_Set( self, "heightChange", Q_irand( 1000, 3000 ) );
					self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand( 1000, 2000 );
				}
			}
			else if ( Jedi_SaberBlockGo( self, &self->NPC->last_ucmd, NULL, NULL, incoming, 0.0f ) != EVASION_NONE )
			{//make sure to turn on your saber if it's not on
				if ( self->client->NPC_class != CLASS_BOBAFETT )
				{
					//self->client->ps.SaberActivate();
					WP_ActivateSaber(self);
				}
			}
		}
		else//player
		{
			gentity_t *owner = &g_entities[incoming->r.ownerNum];

			//[DodgeSys]
			//make sure your saber is on but only if it's turned off now.
			if(self->client->ps.saberHolstered == 2)
			{
				WP_ActivateSaber(self);
			}
			//[/DodgeSys]

			//[SaberSys]
			if(closestSwingBlock && owner->health > 0)//&& !self->client->ps.duelInProgress)
			{
				self->client->ps.saberBlocked = BlockedforQuad(closestSwingQuad);
				self->client->ps.userInt3 |= ( 1 << FLAG_PREBLOCK );
			}
			else if(owner->health > 0)//!self->client->ps.duelInProgress)
			{
				WP_SaberBlockNonRandom( self, incoming->r.currentOrigin, qtrue );
			}
			//WP_SaberBlockNonRandom( self, incoming->r.currentOrigin, qtrue );
			//[/SaberSys]
			if ( owner && owner->client && (!self->enemy || self->enemy->s.weapon != WP_SABER) )//keep enemy jedi over shooters
			{
				self->enemy = owner;
				//NPC_SetLookTarget( self, owner->s.number, level.time+1000 );
				//player looktargetting done differently
			}
		}
	}
}

#define MIN_SABER_SLICE_DISTANCE 50

#define MIN_SABER_SLICE_RETURN_DISTANCE 50

#define SABER_THROWN_HIT_DAMAGE 100
#define SABER_THROWN_RETURN_HIT_DAMAGE 100

void thrownSaberTouch (gentity_t *saberent, gentity_t *other, trace_t *trace);

GAME_INLINE qboolean CheckThrownSaberDamaged(gentity_t *saberent, gentity_t *saberOwner, gentity_t *ent, int dist, int returning, qboolean noDCheck)
{
	vec3_t vecsub;
	float veclen;
	gentity_t *te;

	if (!saberOwner || !saberOwner->client)
	{
		return qfalse;
	}

	if (saberOwner->client->ps.saberAttackWound > level.time)
	{
		return qfalse;
	}

	if (ent && ent->client && ent->inuse && ent->s.number != saberOwner->s.number &&
		ent->health > 0 && ent->takedamage &&
		trap_InPVS(ent->client->ps.origin, saberent->r.currentOrigin) &&
		ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
		(ent->client->pers.connected || ent->s.eType == ET_NPC))
	{ //hit a client
		if (ent->inuse && ent->client &&
			ent->client->ps.duelInProgress &&
			ent->client->ps.duelIndex != saberOwner->s.number)
		{
			return qfalse;
		}

		if (ent->inuse && ent->client &&
			saberOwner->client->ps.duelInProgress &&
			saberOwner->client->ps.duelIndex != ent->s.number)
		{
			return qfalse;
		}

		VectorSubtract(saberent->r.currentOrigin, ent->client->ps.origin, vecsub);
		veclen = VectorLength(vecsub);

		if (veclen < dist)
		{ //within range
			trace_t tr;

			trap_Trace(&tr, saberent->r.currentOrigin, NULL, NULL, ent->client->ps.origin, saberent->s.number, MASK_SHOT);

			if (tr.fraction == 1 || tr.entityNum == ent->s.number)
			{ //Slice them
				if (!saberOwner->client->ps.isJediMaster && WP_SaberCanBlock(ent, tr.endpos, 0, MOD_SABER, qfalse, 999))
				{ //they blocked it
					WP_SaberBlockNonRandom(ent, tr.endpos, qfalse);

					te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
					VectorCopy(tr.endpos, te->s.origin);
					VectorCopy(tr.plane.normal, te->s.angles);
					if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
					{
						te->s.angles[1] = 1;
					}
					te->s.eventParm = 1;
					te->s.weapon = 0;//saberNum
					te->s.legsAnim = 0;//bladeNum

					if (saberCheckKnockdown_Thrown(saberent, saberOwner, &g_entities[tr.entityNum]))
					{ //it was knocked out of the air
						return qfalse;
					}

					if (!returning)
					{ //return to owner if blocked
						thrownSaberTouch(saberent, saberent, NULL);
					}

					saberOwner->client->ps.saberAttackWound = level.time + 500;
					return qfalse;
				}
				else
				{ //a good hit
					vec3_t dir;
					int dflags = 0;

					VectorSubtract(tr.endpos, saberent->r.currentOrigin, dir);
					VectorNormalize(dir);

					if (!dir[0] && !dir[1] && !dir[2])
					{
						dir[1] = 1;
					}

					if ( (saberOwner->client->saber[0].saberFlags2&SFL2_NO_DISMEMBERMENT) )
					{
						dflags |= DAMAGE_NO_DISMEMBER;
					}

					if ( saberOwner->client->saber[0].knockbackScale > 0.0f )
					{
						dflags |= DAMAGE_SABER_KNOCKBACK1;
					}

					if (saberOwner->client->ps.isJediMaster)
					{ //2x damage for the Jedi Master
						G_Damage(ent, saberOwner, saberOwner, dir, tr.endpos, saberent->damage*2, dflags, MOD_SABER);
					}
					else
					{
						G_Damage(ent, saberOwner, saberOwner, dir, tr.endpos, saberent->damage, dflags, MOD_SABER);
					}

					te = G_TempEntity( tr.endpos, EV_SABER_HIT );
					te->s.otherEntityNum = ent->s.number;
					te->s.otherEntityNum2 = saberOwner->s.number;
					te->s.weapon = 0;//saberNum
					te->s.legsAnim = 0;//bladeNum
					VectorCopy(tr.endpos, te->s.origin);
					VectorCopy(tr.plane.normal, te->s.angles);
					if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
					{
						te->s.angles[1] = 1;
					}

					te->s.eventParm = 1;

					if (!returning)
					{ //return to owner if blocked
						thrownSaberTouch(saberent, saberent, NULL);
					}
				}

				saberOwner->client->ps.saberAttackWound = level.time + 500;
			}
		}
	}
	else if (ent && !ent->client && ent->inuse && ent->takedamage && ent->health > 0 && ent->s.number != saberOwner->s.number &&
		ent->s.number != saberent->s.number && (noDCheck ||trap_InPVS(ent->r.currentOrigin, saberent->r.currentOrigin)))
	{ //hit a non-client

		if (noDCheck)
		{
			veclen = 0;
		}
		else
		{
			VectorSubtract(saberent->r.currentOrigin, ent->r.currentOrigin, vecsub);
			veclen = VectorLength(vecsub);
		}

		if (veclen < dist)
		{
			trace_t tr;
			vec3_t entOrigin;

			if (ent->s.eType == ET_MOVER)
			{
				VectorSubtract( ent->r.absmax, ent->r.absmin, entOrigin );
				VectorMA( ent->r.absmin, 0.5, entOrigin, entOrigin );
				VectorAdd( ent->r.absmin, ent->r.absmax, entOrigin );
				VectorScale( entOrigin, 0.5f, entOrigin );
			}
			else
			{
				VectorCopy(ent->r.currentOrigin, entOrigin);
			}

			trap_Trace(&tr, saberent->r.currentOrigin, NULL, NULL, entOrigin, saberent->s.number, MASK_SHOT);

			if (tr.fraction == 1 || tr.entityNum == ent->s.number)
			{
				vec3_t dir;
				int dflags = 0;

				VectorSubtract(tr.endpos, entOrigin, dir);
				VectorNormalize(dir);

				if ( (saberOwner->client->saber[0].saberFlags2&SFL2_NO_DISMEMBERMENT) )
				{
					dflags |= DAMAGE_NO_DISMEMBER;
				}
				if ( saberOwner->client->saber[0].knockbackScale > 0.0f )
				{
					dflags |= DAMAGE_SABER_KNOCKBACK1;
				}

				if (ent->s.eType == ET_NPC)
				{ //an animent
					G_Damage(ent, saberOwner, saberOwner, dir, tr.endpos, 40, dflags, MOD_SABER);
				}
				else
				{
					G_Damage(ent, saberOwner, saberOwner, dir, tr.endpos, 5, dflags, MOD_SABER);
				}

				te = G_TempEntity( tr.endpos, EV_SABER_HIT );
				te->s.otherEntityNum = ENTITYNUM_NONE; //don't do this for throw damage
				//te->s.otherEntityNum = ent->s.number;
				te->s.otherEntityNum2 = saberOwner->s.number;//actually, do send this, though - for the overridden per-saber hit effects/sounds
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}

				if ( ent->s.eType == ET_MOVER )
				{
					if ( saberOwner
						&& saberOwner->client
						&& (saberOwner->client->saber[0].saberFlags2&SFL2_NO_CLASH_FLARE) ) 
					{//don't do clash flare - NOTE: assumes same is true for both sabers if using dual sabers!
						G_FreeEntity( te );//kind of a waste, but...
					}
					else
					{
						//I suppose I could tie this into the saberblock event, but I'm tired of adding flags to that thing.
						gentity_t *teS = G_TempEntity( te->s.origin, EV_SABER_CLASHFLARE );
						VectorCopy(te->s.origin, teS->s.origin);

						te->s.eventParm = 0;
					}
				}
				else
				{
					te->s.eventParm = 1;
				}

				if (!returning)
				{ //return to owner if blocked
					thrownSaberTouch(saberent, saberent, NULL);
				}

				saberOwner->client->ps.saberAttackWound = level.time + 500;
			}
		}
	}

	return qtrue;
}

GAME_INLINE void saberCheckRadiusDamage(gentity_t *saberent, int returning)
{ //we're going to cheat and damage players within the saber's radius, just for the sake of doing things more "efficiently" (and because the saber entity has no server g2 instance)
	int i = 0;
	int dist = 0;
	gentity_t *ent;
	gentity_t *saberOwner = &g_entities[saberent->r.ownerNum];

	if (returning && returning != 2)
	{
		dist = MIN_SABER_SLICE_RETURN_DISTANCE;
	}
	else
	{
		dist = MIN_SABER_SLICE_DISTANCE;
	}

	if (!saberOwner || !saberOwner->client)
	{
		return;
	}

	if (saberOwner->client->ps.saberAttackWound > level.time)
	{
		return;
	}

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		CheckThrownSaberDamaged(saberent, saberOwner, ent, dist, returning, qfalse);

		i++;
	}
}

#define THROWN_SABER_COMP

GAME_INLINE void saberMoveBack( gentity_t *ent, qboolean goingBack ) 
{//racc - This function does the actual movement thrown sabers (ones that aren't "dead").
	vec3_t		origin, oldOrg;

	//[SaberThrowSys]
	/*
	if (ent->s.eFlags&EF_MISSILE_STICK)
	{// If its stuck don't move it
		ent->s.pos.trType = TR_STATIONARY;
		ent->s.apos.trType = TR_STATIONARY;
		return;
	}
	*/
	//[/SaberThrowSys]
	
	ent->s.pos.trType = TR_LINEAR;

	VectorCopy( ent->r.currentOrigin, oldOrg );
	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	//Get current angles?
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );

	//[SaberThrowSys]
/* //racc - I'm not sure why Keshire pulled this code, but whatever. *shrug*
	//compensation test code..
#ifdef THROWN_SABER_COMP
	if (!goingBack && ent->s.pos.trType != TR_GRAVITY)
	{ //acts as a fallback in case touch code fails, keeps saber from going through things between predictions
		float originalLength = 0;
		int iCompensationLength = 32;
		trace_t tr;
		vec3_t mins, maxs;
		vec3_t calcComp, compensatedOrigin;
		VectorSet( mins, -24.0f, -24.0f, -8.0f );
		VectorSet( maxs, 24.0f, 24.0f, 8.0f );

		VectorSubtract(origin, oldOrg, calcComp);
		originalLength = VectorLength(calcComp);

		VectorNormalize(calcComp);

		compensatedOrigin[0] = oldOrg[0] + calcComp[0]*(originalLength+iCompensationLength);		
		compensatedOrigin[1] = oldOrg[1] + calcComp[1]*(originalLength+iCompensationLength);
		compensatedOrigin[2] = oldOrg[2] + calcComp[2]*(originalLength+iCompensationLength);

		trap_Trace(&tr, oldOrg, mins, maxs, compensatedOrigin, ent->r.ownerNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 || tr.startsolid || tr.allsolid) && tr.entityNum != ent->r.ownerNum && !(g_entities[tr.entityNum].r.contents & CONTENTS_LIGHTSABER))
		{
			VectorClear(ent->s.pos.trDelta);

			//Unfortunately doing this would defeat the purpose of the compensation. We will have to settle for a jerk on the client.
			//VectorCopy( origin, ent->r.currentOrigin );

			//we'll skip the dist check, since we don't really care about that (we just hit it physically)
			CheckThrownSaberDamaged(ent, &g_entities[ent->r.ownerNum], &g_entities[tr.entityNum], 256, 0, qtrue);

			if (ent->s.pos.trType == TR_GRAVITY)
			{ //got blocked and knocked away in the damage func
				return;
			}

			tr.startsolid = 0;
			if (tr.entityNum == ENTITYNUM_NONE)
			{ //eh, this is a filthy lie. (obviously it had to hit something or it wouldn't be in here, so we'll say it hit the world)
				tr.entityNum = ENTITYNUM_WORLD;
			}
			thrownSaberTouch(ent, &g_entities[tr.entityNum], &tr);
			return;
		}
	}
#endif
*/
	//[/SaberThrowSys]
	VectorCopy( origin, ent->r.currentOrigin );
}


void SaberBounceSound( gentity_t *self, gentity_t *other, trace_t *trace )
{//racc - This function makes the saber lay flat if it touches something.
	VectorCopy(self->r.currentAngles, self->s.apos.trBase);
	self->s.apos.trBase[PITCH] = 90;
}


void DeadSaberThink(gentity_t *saberent)
{
	//[SaberThrowSys]
    if(saberent->s.owner != ENTITYNUM_NONE && g_entities[saberent->s.owner].s.eFlags & EF_DEAD)
	{//kill the connection from the saber to the player since they're dead and probably going to reset their
		//blade status soon.
		saberent->s.owner = ENTITYNUM_NONE;
	}
	//[/SaberThrowSys]

	if (saberent->speed < level.time)
	{
		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	G_RunObject(saberent);
}

void MakeDeadSaber(gentity_t *ent)
{	//spawn a "dead" saber entity here so it looks like the saber fell out of the air.
	//This entity will remove itself after a very short time period.
	//[BugFix23]
	//trace stuct used for determining if it's safe to spawn at current location
	trace_t		tr;  
	//[/BugFix23]
	vec3_t startorg;
	vec3_t startang;
	gentity_t *saberent;
	gentity_t *owner = NULL;
	
	if (g_gametype.integer == GT_JEDIMASTER)
	{ //never spawn a dead saber in JM, because the only saber on the level is really a world object
		//G_Sound(ent, CHAN_AUTO, saberOffSound);
		return;
	}

	saberent = G_Spawn();

	VectorCopy(ent->r.currentOrigin, startorg);
	VectorCopy(ent->r.currentAngles, startang);

	saberent->classname = "deadsaber";
			
	saberent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	saberent->r.ownerNum = ent->s.number;

	saberent->clipmask = MASK_PLAYERSOLID;
	saberent->r.contents = CONTENTS_TRIGGER;//0;

	VectorSet( saberent->r.mins, -3.0f, -3.0f, -1.5f );
	VectorSet( saberent->r.maxs, 3.0f, 3.0f, 1.5f );

	saberent->touch = SaberBounceSound;

	saberent->think = DeadSaberThink;
	saberent->nextthink = level.time;

	//[BugFix23]
	//perform a trace before attempting to spawn at currently location.
	//unfortunately, it's a fairly regular occurance that current saber location
	//(normally at the player's right hand) could result in the saber being stuck 
	//in the the map and then freaking out.
	trap_Trace(&tr, startorg, saberent->r.mins, saberent->r.maxs,
		startorg, saberent->s.number, saberent->clipmask);
	if(tr.startsolid || tr.fraction != 1)
	{//bad position, try popping our origin up a bit
		startorg[2] += 20;
		trap_Trace(&tr, startorg, saberent->r.mins, saberent->r.maxs,
			startorg, saberent->s.number, saberent->clipmask);
		if(tr.startsolid || tr.fraction != 1)
		{//still no luck, try using our owner's origin
			owner = &g_entities[ent->r.ownerNum];
			if( owner->inuse && owner->client )
			{
				G_SetOrigin(saberent, owner->client->ps.origin); 
			}
			
			//since this is our last chance, we don't care if this works or not.
		}
	}
	//[/BugFix23]

	VectorCopy(startorg, saberent->s.pos.trBase);
	VectorCopy(startang, saberent->s.apos.trBase);

	VectorCopy(startorg, saberent->s.origin);
	VectorCopy(startang, saberent->s.angles);

	VectorCopy(startorg, saberent->r.currentOrigin);
	VectorCopy(startang, saberent->r.currentAngles);

	saberent->s.apos.trType = TR_GRAVITY;
	saberent->s.apos.trDelta[0] = Q_irand(200, 800);
	saberent->s.apos.trDelta[1] = Q_irand(200, 800);
	saberent->s.apos.trDelta[2] = Q_irand(200, 800);
	saberent->s.apos.trTime = level.time-50;

	saberent->s.pos.trType = TR_GRAVITY;
	saberent->s.pos.trTime = level.time-50;
	saberent->flags = FL_BOUNCE_HALF;
	if (ent->r.ownerNum >= 0 && ent->r.ownerNum < ENTITYNUM_WORLD)
	{
		owner = &g_entities[ent->r.ownerNum];

		if (owner->inuse && owner->client &&
			owner->client->saber[0].model[0])
		{
			WP_SaberAddG2Model( saberent, owner->client->saber[0].model, owner->client->saber[0].skin );
			//[SaberThrowSys]
			//indicate that this saber is associated with this player
			saberent->s.owner = owner->s.number;
			//[/SaberThrowSys]
		}
		else
		{
			//WP_SaberAddG2Model( saberent, NULL, 0 );
			//argh!!!!
			G_FreeEntity(saberent);
			return;
		}
	}

	saberent->s.modelGhoul2 = 1;
	saberent->s.g2radius = 20;

	saberent->s.eType = ET_MISSILE;
	saberent->s.weapon = WP_SABER;

	saberent->speed = level.time + 4000;

	saberent->bounceCount = 12;

	//fall off in the direction the real saber was headed
	VectorCopy(ent->s.pos.trDelta, saberent->s.pos.trDelta);

	saberMoveBack(saberent, qtrue);
	saberent->s.pos.trType = TR_GRAVITY;

	trap_LinkEntity(saberent);	
}

#define MAX_LEAVE_TIME 20000

void saberReactivate(gentity_t *saberent, gentity_t *saberOwner);
void saberBackToOwner(gentity_t *saberent);

void DownedSaberThink(gentity_t *saberent)
{//racc - this is the think function for a thrown saber entity that has been knocked to the ground.
	gentity_t *saberOwn = NULL;
	qboolean notDisowned = qfalse;
	qboolean pullBack = qfalse;

	saberent->nextthink = level.time;

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{//racc - We've lost our owner.
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	saberOwn = &g_entities[saberent->r.ownerNum];

	if (!saberOwn ||
		!saberOwn->inuse ||
		!saberOwn->client ||
		saberOwn->client->sess.sessionTeam == TEAM_SPECTATOR ||
		(saberOwn->client->ps.pm_flags & PMF_FOLLOW))
	{//racc - owner is invalid.
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saberOwn->client->ps.saberEntityNum)
	{//racc - our owner thinks he's still got a saber in flight or in his hand.  Not good.
		if (saberOwn->client->ps.saberEntityNum == saberent->s.number)
		{ //owner shouldn't have this set if we're thinking in here. Must've fallen off a cliff and instantly respawned or something.
			notDisowned = qtrue;
		}
		else
		{ //This should never happen, but just in case..
			assert(!"ULTRA BAD THING");
			MakeDeadSaber(saberent);

			saberent->think = G_FreeEntity;
			saberent->nextthink = level.time;
			return;
		}
	}

	if (notDisowned || saberOwn->health < 1 || !saberOwn->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{ //He's dead, just go back to our normal saber status
		saberOwn->client->ps.saberEntityNum = saberOwn->client->saberStoredIndex;

		//MakeDeadSaber(saberent); //spawn a dead saber on top of where we are now. The "bodyqueue" method.
		//Actually this will get taken care of when the thrown saber func sees we're dead.

#ifdef _DEBUG
		if (saberOwn->client->saberStoredIndex != saberent->s.number)
		{ //I'm paranoid.		
			//[test]
			//kill the saber entity to recover
			G_LogPrintf("Saber index didn't match that of owner in DownedSaberThink!\n");
			saberent->think = G_FreeEntity;
			saberent->nextthink = level.time;
			return;
			//assert(!"Bad saber index!!!");
			//[/test]
		}
#endif

		saberReactivate(saberent, saberOwn);

		if (saberOwn->health < 1)
		{
			saberOwn->client->ps.saberInFlight = qfalse;
			MakeDeadSaber(saberent);
		}

		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.svFlags |= (SVF_NOCLIENT);
		//saberent->r.contents = CONTENTS_LIGHTSABER;
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;

		if (saberOwn->health > 0)
		{ //only set this if he's alive. If dead we want to reflect the lack of saber on the corpse, as he died with his saber out.
			saberOwn->client->ps.saberInFlight = qfalse;
			WP_SaberRemoveG2Model( saberent );
		}
		saberOwn->client->ps.saberEntityState = 0;
		saberOwn->client->ps.saberThrowDelay = level.time + 500;
		saberOwn->client->ps.saberCanThrow = qfalse;

		return;
	}

	//[SaberThrowSys]
	if(saberent->s.pos.trType == TR_STATIONARY //saber isn't bouncing around anymore
		&&( (saberOwn->client->pers.cmd.buttons & BUTTON_SABERTHROW) ||  //pressing the saber throw button
		saberOwn->client->ps.forceHandExtend == HANDEXTEND_SABERPULL || //trying useing toggle saber
		((saberOwn->client->pers.cmd.buttons & BUTTON_FORCEPOWER) 
		&& saberOwn->client->ps.fd.forcePowerSelected == FP_SABERTHROW) ) ) //using saber throw thru force power selection
	{//we want to pull back the saber.
		pullBack = qtrue;
	}
	/* basejka code
	if (saberOwn->client->saberKnockedTime < level.time && (saberOwn->client->pers.cmd.buttons & BUTTON_ATTACK))
	{ //He wants us back
		pullBack = qtrue;
	}
	else if ((level.time - saberOwn->client->saberKnockedTime) > MAX_LEAVE_TIME)
	{ //Been sitting around for too long, go back no matter what he wants.
		pullBack = qtrue;
	}
	*/
	//[/SaberThrowSys]

	if (pullBack)
	{ //Get going back to the owner.
		saberOwn->client->ps.saberEntityNum = saberOwn->client->saberStoredIndex;

#ifdef _DEBUG
		if (saberOwn->client->saberStoredIndex != saberent->s.number)
		{ //I'm paranoid.
			//[Test]
			//racc - I've seen this assert before so I'm making a correction code routine
			//this should still technically never happen.
			G_Printf("Client %i:  saberStoredIndex %i in DownedSaberThink, it should have been %i.\n", 
				saberOwn->s.number, saberOwn->client->saberStoredIndex, saberent->s.number);
			saberOwn->client->saberStoredIndex = saberOwn->client->ps.saberEntityNum = saberent->s.number;
			//assert(!"Bad saber index!!!");
			//[/Test]
		}
#endif
		saberReactivate(saberent, saberOwn);

		saberent->touch = SaberGotHit;

		saberent->think = saberBackToOwner;
		saberent->speed = 0;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.contents = CONTENTS_LIGHTSABER;

		G_Sound( saberOwn, CHAN_BODY, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
		//[SaberThrowSys]
		//reset the angle physics so we don't have don't have weird spinning on return
		VectorCopy(saberent->r.currentAngles, saberent->s.apos.trBase);
		VectorClear(saberent->s.apos.trDelta);
		saberent->s.apos.trType = TR_STATIONARY;

		//saber doesn't auto turn on when pulled back anymore.
		/* basejka code
		if (saberOwn->client->saber[0].soundOn)
		{
			G_Sound( saberent, CHAN_BODY, saberOwn->client->saber[0].soundOn );
		}
		if (saberOwn->client->saber[1].soundOn)
		{
			G_Sound( saberOwn, CHAN_BODY, saberOwn->client->saber[1].soundOn );
		}
		*/
		//[/SaberThrowSys]

		return;
	}

	G_RunObject(saberent);
	saberent->nextthink = level.time;
}


//[SaberThrowSys]
qboolean BG_CrouchAnim( int anim );
void DrownedSaberTouch( gentity_t *self, gentity_t *other, trace_t *trace )
{//similar to SaberBounceSound but the saber's owners can also pick up their saber by crouching or rolling over it
	VectorCopy(self->r.currentAngles, self->s.apos.trBase);
	self->s.apos.trBase[PITCH] = 90;

	//be able to pick up a dead saber by crouching/rolling over it while on the ground or by catching it mid-air.
	if(other->s.number != self->r.ownerNum || !other || !other->client)
	{//not our owner, or our owner is bad, ignore touch
		return;
	}

	if(self->s.pos.trType == TR_STATIONARY //saber isn't bouncing around.
		//and in a roll or crouching
		&& (BG_InRoll(&other->client->ps, other->client->ps.legsAnim) || BG_CrouchAnim(other->client->ps.legsAnim)))
	{//racc - picked up the saber.
		//SABERSYSRAFIXME - this could be shorter as I simply copy/pasted the entire 
		//downed saber -> returning -> caught sequence.
		other->client->ps.saberEntityNum = other->client->saberStoredIndex;

#ifdef _DEBUG
		if (other->client->saberStoredIndex != self->s.number)
		{ //I'm paranoid.
			assert(!"Bad saber index!!!");
		}
#endif
		saberReactivate(self, other);

		self->r.contents = CONTENTS_LIGHTSABER;

		G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/weapons/saber/saber_catch.wav" ) );

		other->client->ps.saberInFlight = qfalse;
		other->client->ps.saberEntityState = 0;
		other->client->ps.saberCanThrow = qfalse;
		other->client->ps.saberThrowDelay = level.time + 300;

		if(other->client->ps.forceHandExtend == HANDEXTEND_SABERPULL)
		{
			//stop holding hand out if we still are.
			other->client->ps.forceHandExtend = HANDEXTEND_NONE;
			other->client->ps.forceHandExtendTime = level.time;
		}

		self->touch = SaberGotHit;

		self->think = SaberUpdateSelf;
		self->genericValue5 = 0;
		self->nextthink = level.time + 50;
		WP_SaberRemoveG2Model( self );

		//auto reactive the blade
		other->client->ps.saberHolstered = 0;

		if (other->client->saber[0].soundOn)
		{//make activation noise if we have one.
			G_Sound(other, CHAN_WEAPON, other->client->saber[0].soundOn);
		}
	}
}
//[/SaberThrowSys]


void saberReactivate(gentity_t *saberent, gentity_t *saberOwner)
{//racc - I think this function reactivates the saberentity after it was set to be a dropped
	//saber.
	saberent->s.saberInFlight = qtrue;

	saberent->s.apos.trType = TR_LINEAR;
	saberent->s.apos.trDelta[0] = 0;
	saberent->s.apos.trDelta[1] = 800;
	saberent->s.apos.trDelta[2] = 0;

	saberent->s.pos.trType = TR_LINEAR;
	saberent->s.eType = ET_GENERAL;
	saberent->s.eFlags = 0;

	//[SaberThrowSys]
	//since the saber is now under the active control of the player, have it be rendered 
	//as part of the player's rendering process.  However, we switch to a general entity
	//so the clients have positional data about the saber.
	saberent->s.modelGhoul2 = 127;
	//[/SaberThrowSys]

	saberent->parent = saberOwner;

	saberent->genericValue5 = 0;

	SetSaberBoxSize(saberent);

	saberent->touch = thrownSaberTouch;

	saberent->s.weapon = WP_SABER;

	saberOwner->client->ps.saberEntityState = 1;

	trap_LinkEntity(saberent);
}

#define SABER_RETRIEVE_DELAY 3000 //3 seconds for now. This will leave you nice and open if you lose your saber.

//[RACC] - create a loose saber for this saber entity.
void saberKnockDown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other)
{
	//[BugFix23]
	//trace stuct used for determining if it's safe to spawn at current location
	trace_t		tr;  
	//[/BugFix23]
	saberOwner->client->ps.saberEntityNum = 0; //still stored in client->saberStoredIndex
	saberOwner->client->saberKnockedTime = level.time + SABER_RETRIEVE_DELAY;

	saberent->clipmask = MASK_SOLID;

	if(saberOwner->client->ps.fd.saberAnimLevel != SS_DUAL)
		saberent->r.contents = CONTENTS_TRIGGER;//0;

	VectorSet( saberent->r.mins, -3.0f, -3.0f, -1.5f );
	VectorSet( saberent->r.maxs, 3.0f, 3.0f, 1.5f );

	//[BugFix23]
	//perform a trace before attempting to spawn at currently location.
	//unfortunately, it's a fairly regular occurance that current saber location
	//(normally at the player's right hand) could result in the saber being stuck 
	//in the the map and then freaking out.
	trap_Trace(&tr, saberent->r.currentOrigin, saberent->r.mins, saberent->r.maxs,
		saberent->r.currentOrigin, saberent->s.number, saberent->clipmask);
	if(tr.startsolid || tr.fraction != 1)
	{//bad position, try popping our origin up a bit
		saberent->r.currentOrigin[2] += 20;
		G_SetOrigin(saberent, saberent->r.currentOrigin);
		trap_Trace(&tr, saberent->r.currentOrigin, saberent->r.mins, saberent->r.maxs,
			saberent->r.currentOrigin, saberent->s.number, saberent->clipmask);
		if(tr.startsolid || tr.fraction != 1)
		{//still no luck, try using our owner's origin
			G_SetOrigin(saberent, saberOwner->client->ps.origin); 
			
			//since this is our last chance, we don't care if this works or not.
		}
	}
	//[/BugFix23]

	saberent->s.apos.trType = TR_GRAVITY;
	saberent->s.apos.trDelta[0] = Q_irand(200, 800);
	saberent->s.apos.trDelta[1] = Q_irand(200, 800);
	saberent->s.apos.trDelta[2] = Q_irand(200, 800);
	saberent->s.apos.trTime = level.time-50;

	saberent->s.pos.trType = TR_GRAVITY;
	saberent->s.pos.trTime = level.time-50;
	saberent->flags |= FL_BOUNCE_HALF;

	WP_SaberAddG2Model( saberent, saberOwner->client->saber[0].model, saberOwner->client->saber[0].skin );

	saberent->s.modelGhoul2 = 1;
	saberent->s.g2radius = 20;

	saberent->s.eType = ET_MISSILE;
	saberent->s.weapon = WP_SABER;

	saberent->speed = level.time + 4000;

	saberent->bounceCount = -5;//8;
	//[SaberThrowSys]
	//don't try to move during the frame of our creation.
	//saberMoveBack(saberent, qtrue);

	//make sure that this saber has it's owner associated with it so we can render blades on this dropped saber properly.
	saberent->s.owner = saberOwner->s.number;
	//[/SaberThrowSys]
	saberent->s.pos.trType = TR_GRAVITY;

	saberent->s.loopSound = 0; //kill this in case it was spinning.
	saberent->s.loopIsSoundset = qfalse;

	saberent->r.svFlags &= ~(SVF_NOCLIENT); //make sure the client is getting updates on where it is and such.

	//[SaberThrowSys]
	//created new function so we could have dropped sabers return then they are manually grabbed by their owners.
	saberent->touch = DrownedSaberTouch;
	//saberent->touch = SaberBounceSound;
	//[/SaberThrowSys]
	saberent->think = DownedSaberThink;
	saberent->nextthink = level.time;

	if (saberOwner != other)
	{ //if someone knocked it out of the air and it wasn't turned off, go in the direction they were facing.
		if (other->inuse && other->client)
		{
			vec3_t otherFwd;
			float deflectSpeed = 200;

			AngleVectors(other->client->ps.viewangles, otherFwd, 0, 0);

			saberent->s.pos.trDelta[0] = otherFwd[0]*deflectSpeed;
			saberent->s.pos.trDelta[1] = otherFwd[1]*deflectSpeed;
			saberent->s.pos.trDelta[2] = otherFwd[2]*deflectSpeed;
		}
	}

	trap_LinkEntity(saberent);

	if (saberOwner->client->saber[0].soundOff)
	{
		G_Sound( saberent, CHAN_BODY, saberOwner->client->saber[0].soundOff );
	}

	if (saberOwner->client->saber[1].soundOff &&
		saberOwner->client->saber[1].model[0])
	{
		G_Sound( saberOwner, CHAN_BODY, saberOwner->client->saber[1].soundOff );
	}

	//[SaberThrowSys]
	//properly holster the saber so the blade turns off
	if(saberOwner->client->ps.fd.saberAnimLevel == SS_DUAL)
	{//only switch off one blade if player is in the dual styley.
		saberOwner->client->ps.saberHolstered = 1;
	}
	else
	{
		saberOwner->client->ps.saberHolstered = 2;
	}
	//[/SaberThrowSys]
}

//sort of a silly macro I guess. But if I change anything in here I'll probably want it to be everywhere.
#define SABERINVALID (!saberent || !saberOwner || !other || !saberent->inuse || !saberOwner->inuse || !other->inuse || !saberOwner->client || !other->client || !saberOwner->client->ps.saberEntityNum || saberOwner->client->ps.saberLockTime > (level.time-100))

void WP_SaberRemoveG2Model( gentity_t *saberent )
{
	if ( saberent->ghoul2 )
	{
		trap_G2API_RemoveGhoul2Models( &saberent->ghoul2 );
	}
}

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin )
{
	WP_SaberRemoveG2Model( saberent );
	if ( saberModel && saberModel[0] )
	{
		saberent->s.modelindex = G_ModelIndex(saberModel);
	}
	else
	{
		saberent->s.modelindex = G_ModelIndex( DEFAULT_SABER_MODEL );
	}
	//FIXME: use customSkin?
	trap_G2API_InitGhoul2Model( &saberent->ghoul2, saberModel, saberent->s.modelindex, saberSkin, 0, 0, 0 );
}

//Make the saber go flying directly out of the owner's hand in the specified direction
qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity)
{
	if (!saberent || !saberOwner ||
		!saberent->inuse || !saberOwner->inuse ||
		!saberOwner->client)
	{
		return qfalse;
	}

	if (!saberOwner->client->ps.saberEntityNum)
	{ //already gone
		return qfalse;
	}

	if ((level.time - saberOwner->client->lastSaberStorageTime) > 50)
	{ //must have a reasonably updated saber base pos
		return qfalse;
	}

	if (saberOwner->client->ps.saberLockTime > (level.time-100))
	{
		return qfalse;
	}
	if ( (saberOwner->client->saber[0].saberFlags&SFL_NOT_DISARMABLE) )
	{
		return qfalse;
	}
		//make pain noise
	if ( saberOwner->s.number < MAX_CLIENTS )
	{
		G_AddEvent( saberOwner, Q_irand(EV_PUSHED1, EV_PUSHED3), 0 );
	}

	saberOwner->client->ps.saberInFlight = qtrue;
	saberOwner->client->ps.saberEntityState = 1;

	saberent->s.saberInFlight = qfalse;//qtrue;

	saberent->s.pos.trType = TR_LINEAR;
	saberent->s.eType = ET_GENERAL;
	saberent->s.eFlags = 0;

	WP_SaberAddG2Model( saberent, saberOwner->client->saber[0].model, saberOwner->client->saber[0].skin );

	saberent->s.modelGhoul2 = 127;

	saberent->parent = saberOwner;

	saberent->damage = SABER_THROWN_HIT_DAMAGE;
	saberent->methodOfDeath = MOD_SABER;
	saberent->splashMethodOfDeath = MOD_SABER;
	saberent->s.solid = 2;
	saberent->r.contents = CONTENTS_LIGHTSABER;

	saberent->genericValue5 = 0;

	VectorSet( saberent->r.mins, -24.0f, -24.0f, -8.0f );
	VectorSet( saberent->r.maxs, 24.0f, 24.0f, 8.0f );

	saberent->s.genericenemyindex = saberOwner->s.number+1024;
	saberent->s.weapon = WP_SABER;

	saberent->genericValue5 = 0;

	G_SetOrigin(saberent, saberOwner->client->lastSaberBase_Always); //use this as opposed to the right hand bolt,
	//because I don't want to risk reconstructing the skel again to get it here. And it isn't worth storing.
	saberKnockDown(saberent, saberOwner, saberOwner);
	VectorCopy(velocity, saberent->s.pos.trDelta); //override the velocity on the knocked away saber.
	
	return qtrue;
}

//Called at the result of a circle lock duel - the loser gets his saber tossed away and is put into a reflected attack anim
qboolean saberCheckKnockdown_DuelLoss(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other)
{
	vec3_t dif;
	float totalDistance = 1;
	float distScale = 6.5f;
	qboolean validMomentum = qtrue;
	int	disarmChance = 1;

	if (SABERINVALID)
	{
		return qfalse;
	}

	VectorClear(dif);

	if (!other->client->olderIsValid || (level.time - other->client->lastSaberStorageTime) >= 200)
	{ //see if the spots are valid
		validMomentum = qfalse;
	}

	if (validMomentum)
	{
		//Get the difference 
		VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, dif);
		totalDistance = VectorNormalize(dif);

		if (!totalDistance)
		{ //fine, try our own
			if (!saberOwner->client->olderIsValid || (level.time - saberOwner->client->lastSaberStorageTime) >= 200)
			{
				validMomentum = qfalse;
			}

			if (validMomentum)
			{
				VectorSubtract(saberOwner->client->lastSaberBase_Always, saberOwner->client->olderSaberBase, dif);
				totalDistance = VectorNormalize(dif);
			}
		}

		if (validMomentum)
		{
			if (!totalDistance)
			{ //try the difference between the two blades
				VectorSubtract(saberOwner->client->lastSaberBase_Always, other->client->lastSaberBase_Always, dif);
				totalDistance = VectorNormalize(dif);
			}

			if (totalDistance)
			{ //if we still have no difference somehow, just let it fall to the ground when the time comes.
				if (totalDistance < 20)
				{
					totalDistance = 20;
				}
				VectorScale(dif, totalDistance*distScale, dif);
			}
		}
	}

	saberOwner->client->ps.saberMove = LS_V1_BL; //rwwFIXMEFIXME: Ideally check which lock it was exactly and use the proper anim (same goes for the attacker)
	saberOwner->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if ( other && other->client )
	{
		disarmChance += other->client->saber[0].disarmBonus;
		if (  other->client->saber[1].model[0]
			&& !other->client->ps.saberHolstered )
		{
			disarmChance += other->client->saber[1].disarmBonus;
		}
	}
	if ( Q_irand( 0, disarmChance ) )
	{
		return saberKnockOutOfHand(saberent, saberOwner, dif);
	}
	else
	{
		return qfalse;
	}
}

//[SaberSys]
//Knock the saber out of the hands of saberOwner using the net momentum between saberOwner and other's net momentums
qboolean ButterFingers(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other, trace_t *tr)
{
	vec3_t	svelocity, ovelocity;
	vec3_t	sswing,	oswing;
	vec3_t	dir;

	VectorClear(svelocity);
	VectorClear(ovelocity);
	VectorClear(sswing);
	VectorClear(oswing);

	if (!saberOwner->client->olderIsValid || (level.time - saberOwner->client->lastSaberStorageTime) >= 200
		|| !saberOwner->client->ps.saberEntityNum || saberOwner->client->ps.saberInFlight)
	{//old or bad saberOwner data or you don't have a saber in your hand.  We're kind of screwed so just return.
		return qfalse;
	}
	else
	{
		VectorSubtract(saberOwner->client->lastSaberBase_Always, saberOwner->client->olderSaberBase, sswing);
		VectorAdd(saberOwner->client->ps.velocity, sswing, sswing);

		if(other && other->client && other->inuse)
		{
			if(other->client->olderIsValid && (level.time - other->client->lastSaberStorageTime) >= 200
				&& other->client->ps.saberEntityNum && !other->client->ps.saberInFlight)
			{
				VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, oswing);
				VectorCopy(other->client->ps.velocity, ovelocity);
				VectorAdd(ovelocity, oswing, oswing);
			}
			else if(other->client->ps.velocity) // Ensi: fixme This will always be true
			{
				VectorCopy(other->client->ps.velocity, oswing);
			}
		}
		else
		{//No useable client data....ok  Let's try just bouncing off of the impact surface then.

			//scale things back a bit for realism.
			VectorScale(sswing, SABER_ELASTIC_RATIO, sswing);

			if(DotProduct(tr->plane.normal, sswing) > 0)
			{//weird impact as the saber is moving away from the impact plane.  Oh well.
				VectorScale(tr->plane.normal, -(VectorLength(sswing)), oswing);
			}
			else
			{
				VectorScale(tr->plane.normal, VectorLength(sswing), oswing);
			}
		}
	}

	if(DotProduct(sswing, oswing) > 0)
	{
		VectorSubtract(oswing, sswing, dir);
	}
	else
	{
		VectorAdd(oswing, sswing, dir);
	}

	return saberKnockOutOfHand(saberent, saberOwner, dir);
}
//[/SaberSys]

	
//Called when we want to try knocking the saber out of the owner's hand upon them going into a broken parry.
//Also called on reflected attacks.
//[RACC] - Checks the two player's saber attack levels and determines if the saberOwner should lose his
//saber or not.  This also calculates the lost saber's ejection velocity and actually does the lost
//saber if it's suppose to happen.
qboolean saberCheckKnockdown_BrokenParry(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other)
{
	int myAttack;
	int otherAttack;
	qboolean doKnock = qfalse;
	int	disarmChance = 1;

	if (SABERINVALID)
	{
		return qfalse;
	}

	//Neither gets an advantage based on attack state, when it comes to knocking
	//saber out of hand.
	myAttack = G_SaberAttackPower(saberOwner, qfalse);
	otherAttack = G_SaberAttackPower(other, qfalse);

	if (!other->client->olderIsValid || (level.time - other->client->lastSaberStorageTime) >= 200)
	{ //if we don't know which way to throw the saber based on momentum between saber positions, just don't throw it
		return qfalse;
	}

	//only knock the saber out of the hand if they're in a stronger stance I suppose. Makes strong more advantageous.
	if (otherAttack > myAttack+1 && Q_irand(1, 10) <= 7)
	{ //This would be, say, strong stance against light stance.
		doKnock = qtrue;
	}
	else if (otherAttack > myAttack && Q_irand(1, 10) <= 3)
	{ //Strong vs. medium, medium vs. light
		doKnock = qtrue;
	}

	if (doKnock)
	{
		vec3_t dif;
		float totalDistance;
		float distScale = 6.5f;

		VectorSubtract(other->client->lastSaberBase_Always, other->client->olderSaberBase, dif);
		totalDistance = VectorNormalize(dif);

		if (!totalDistance)
		{ //fine, try our own
			if (!saberOwner->client->olderIsValid || (level.time - saberOwner->client->lastSaberStorageTime) >= 200)
			{ //if we don't know which way to throw the saber based on momentum between saber positions, just don't throw it
				return qfalse;
			}

			VectorSubtract(saberOwner->client->lastSaberBase_Always, saberOwner->client->olderSaberBase, dif);
			totalDistance = VectorNormalize(dif);
		}

		if (!totalDistance)
		{ //...forget it then.
			return qfalse;
		}

		if (totalDistance < 20)
		{
			totalDistance = 20;
		}
		VectorScale(dif, totalDistance*distScale, dif);

		if ( other && other->client )
		{
			disarmChance += other->client->saber[0].disarmBonus;
			if (  other->client->saber[1].model[0]
				&& !other->client->ps.saberHolstered )
			{
				disarmChance += other->client->saber[1].disarmBonus;
			}
		}
		if ( Q_irand( 0, disarmChance ) )
		{
			return saberKnockOutOfHand(saberent, saberOwner, dif);
		}
	}

	return qfalse;
}

qboolean BG_InExtraDefenseSaberMove( int move );

//Called upon an enemy actually slashing into a thrown saber
qboolean saberCheckKnockdown_Smashed(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other, int damage)
{
	if (SABERINVALID)
	{
		return qfalse;
	}

	if (!saberOwner->client->ps.saberInFlight)
	{ //can only do this if the saber is already actually in flight
		return qfalse;
	}

	//[SaberSys]
	//Just do it now.
	saberKnockDown(saberent, saberOwner, other);
	return qtrue;

	/*
	if ( other
		&& other->inuse
		&& other->client 
		&& BG_InExtraDefenseSaberMove( other->client->ps.saberMove ) )
	{ //make sure the blow was strong enough
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}

	if (damage > 10)
	{ //make sure the blow was strong enough
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}

	return qfalse;
	*/
	//[/SaberSys]
}

//Called upon blocking a thrown saber. If the throw level compared to the blocker's defense level
//is inferior, or equal and a random factor is met, then the saber will be tossed to the ground.
qboolean saberCheckKnockdown_Thrown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other)
{
	int throwLevel = 0;
	int defenLevel = 0;
	qboolean tossIt = qfalse;

	if (SABERINVALID)
	{
		return qfalse;
	}

	defenLevel = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];
	throwLevel = saberOwner->client->ps.fd.forcePowerLevel[FP_SABERTHROW];

	if (defenLevel > throwLevel)
	{
		tossIt = qtrue;
	}
	else if (defenLevel == throwLevel && Q_irand(1, 10) <= 4)
	{
		tossIt = qtrue;
	}
	//otherwise don't

	if (tossIt)
	{
		saberKnockDown(saberent, saberOwner, other);
		return qtrue;
	}

	return qfalse;
}

void saberBackToOwner(gentity_t *saberent)
{//racc - Think func for sabers that are actively trying to fly back to their owners.
	gentity_t *saberOwner = &g_entities[saberent->r.ownerNum];
	vec3_t dir;
	float ownerLen;

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{//racc - saber lost track of owner.
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (!saberOwner->inuse ||
		!saberOwner->client ||
		saberOwner->client->sess.sessionTeam == TEAM_SPECTATOR)
	{//racc - saber owner not valid 
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saberOwner->health < 1 || !saberOwner->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{ //He's dead, just go back to our normal saber status
		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		if (saberOwner->client &&
			saberOwner->client->saber[0].soundOff)
		{
			G_Sound(saberent, CHAN_AUTO, saberOwner->client->saber[0].soundOff);
		}
		MakeDeadSaber(saberent);

		saberent->r.svFlags |= (SVF_NOCLIENT);
		saberent->r.contents = CONTENTS_LIGHTSABER;
		SetSaberBoxSize(saberent);
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;
		WP_SaberRemoveG2Model( saberent );

		saberOwner->client->ps.saberInFlight = qfalse;
		saberOwner->client->ps.saberEntityState = 0;
		saberOwner->client->ps.saberThrowDelay = level.time + 500;
		saberOwner->client->ps.saberCanThrow = qfalse;

		return;
	}

	//make sure this is set alright
	assert(saberOwner->client->ps.saberEntityNum == saberent->s.number ||
		saberOwner->client->saberStoredIndex == saberent->s.number);
	saberOwner->client->ps.saberEntityNum = saberent->s.number;

	saberent->r.contents = CONTENTS_LIGHTSABER;

	//racc - Apprenently pos1 is the hand bolt.
	VectorSubtract(saberent->pos1, saberent->r.currentOrigin, dir);

	ownerLen = VectorLength(dir);

	if (saberent->speed < level.time)
	{//racc - update the saber's speed
		float baseSpeed = 900;

		VectorNormalize(dir);

		saberMoveBack(saberent, qtrue);
		VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);

		if (saberOwner->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{ //allow players with high saber throw rank to control the return speed of the saber
			baseSpeed = 900;

			saberent->speed = level.time;// + 200;
		}
		else
		{
			baseSpeed = 700;
			saberent->speed = level.time + 50;
		}

		//Gradually slow down as it approaches, so it looks smoother coming into the hand.
		if (ownerLen < 64)
		{
			VectorScale(dir, baseSpeed-200, saberent->s.pos.trDelta );
		}
		else if (ownerLen < 128)
		{
			VectorScale(dir, baseSpeed-150, saberent->s.pos.trDelta );
		}
		else if (ownerLen < 256)
		{
			VectorScale(dir, baseSpeed-100, saberent->s.pos.trDelta );
		}
		else
		{
			VectorScale(dir, baseSpeed, saberent->s.pos.trDelta );
		}

		saberent->s.pos.trTime = level.time;
	}

	/*
	if (ownerLen <= 512)
	{
		saberent->s.saberInFlight = qfalse;
		saberent->s.loopSound = saberHumSound;
		saberent->s.loopIsSoundset = qfalse;
	}
	*/

	//I'm just doing this now. I don't really like the spin on the way back. And it does weird stuff with the new saber-knocked-away code.
	if (saberOwner->client->ps.saberEntityNum == saberent->s.number)
	{
		if ( !(saberOwner->client->saber[0].saberFlags&SFL_RETURN_DAMAGE)
			|| saberOwner->client->ps.saberHolstered )
		{
			saberent->s.saberInFlight = qfalse;
		}
		saberent->s.loopSound = saberOwner->client->saber[0].soundLoop;
		saberent->s.loopIsSoundset = qfalse;

		if (ownerLen <= 32)
		{//racc - picked up the saber.
			G_Sound( saberent, CHAN_AUTO, G_SoundIndex( "sound/weapons/saber/saber_catch.wav" ) );

			saberOwner->client->ps.saberInFlight = qfalse;
			saberOwner->client->ps.saberEntityState = 0;
			saberOwner->client->ps.saberCanThrow = qfalse;
			saberOwner->client->ps.saberThrowDelay = level.time + 300;

			//[SaberThrowSys]
			if(saberOwner->client->ps.forceHandExtend == HANDEXTEND_SABERPULL)
			{
				//stop holding hand out if we still are.
				saberOwner->client->ps.forceHandExtend = HANDEXTEND_NONE;
				saberOwner->client->ps.forceHandExtendTime = level.time;
			}
			//[/SaberThrowSys]

			saberent->touch = SaberGotHit;

			saberent->think = SaberUpdateSelf;
			saberent->genericValue5 = 0;
			saberent->nextthink = level.time + 50;
			WP_SaberRemoveG2Model( saberent );

			//[SaberThrowSys]
			//auto reactive the blade
			saberOwner->client->ps.saberHolstered = 0;

			if (saberOwner->client->saber[0].soundOn)
			{//make activation noise if we have one.
				G_Sound(saberOwner, CHAN_WEAPON, saberOwner->client->saber[0].soundOn);
			}
			//[/SaberThrowSys]
			return;
		}

		//[SaberThrowSys]
		//Blade not drawn during returns now. So, no damage!
		/* basejka code
		if (!saberent->s.saberInFlight)
		{
			saberCheckRadiusDamage(saberent, 1);
		}
		else
		{
			saberCheckRadiusDamage(saberent, 2);
		}
		*/
		//[/SaberThrowSys]

		saberMoveBack(saberent, qtrue);
	}

	saberent->nextthink = level.time;
}

void saberFirstThrown(gentity_t *saberent);

//[SaberThrowSys]
void thrownSaberBallistics(gentity_t *saberEnt, gentity_t *saberOwn, qboolean stuck);
//[/SaberThrowSys]

void thrownSaberTouch (gentity_t *saberent, gentity_t *other, trace_t *trace)
{//racc - this function sets up a thrown saber to start returning to the player.  In addition,
	//it tries to damage the hit target (other) if possible.  This can be reached by
	//situations that don't involve impacts.
	gentity_t *hitEnt = other;
	//[SaberThrowSys]
	gentity_t *saberOwn = &g_entities[saberent->r.ownerNum];
	//[/SaberThrowSys]

	if (other && other->s.number == saberent->r.ownerNum)
	{//racc - we hit our owner, just ignore it.
		return;
	}
	
	//[SaberThrowSys]
	if ( other 
		&& other->s.number == ENTITYNUM_WORLD //hit solid object.
		&& saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3 )
	{//hit something we can stick to and we threw the saber hard enough to stick.
		/*  racc - I'm not sure this is even needed.
		if (trace->plane.normal[2] >= 0.8f || trace->plane.normal[2] <= -0.8f)
		{//surface is too horizontal to stick to.
		}
		else
		*/
		vec3_t saberFwd;
		AngleVectors(saberent->r.currentAngles, saberFwd, NULL, NULL);

		if(DotProduct( saberFwd, trace->plane.normal) > 0)
		{//hit straight enough on to stick to the surface!
			thrownSaberBallistics(saberent, saberOwn, qtrue);
			return;
		}
	}

	/* basejka code - I think it's just trying to move back to the owner.
	VectorClear(saberent->s.pos.trDelta);
	saberent->s.pos.trTime = level.time;

	saberent->s.apos.trType = TR_LINEAR;
	saberent->s.apos.trDelta[0] = 0;
	saberent->s.apos.trDelta[1] = 800;
	saberent->s.apos.trDelta[2] = 0;

	VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);

	saberent->think = saberBackToOwner;
	saberent->nextthink = level.time;
	*/
	//[/SaberThrowSys]

	if (other && other->r.ownerNum < MAX_CLIENTS &&
		(other->r.contents & CONTENTS_LIGHTSABER) &&
		g_entities[other->r.ownerNum].client &&
		g_entities[other->r.ownerNum].inuse)
	{//racc - hit another lightsaber entity, hit the saber's owner instead.
		hitEnt = &g_entities[other->r.ownerNum];
	}

	//we'll skip the dist check, since we don't really care about that (we just hit it physically)
	CheckThrownSaberDamaged(saberent, &g_entities[saberent->r.ownerNum], hitEnt, 256, 0, qtrue);

	//[SaberThrowSys]
	//get knocked down instead immediately after hitting anything.

	//updating the current trBases because trTime is reset in saberKnockDown
	//and for some reason it doesn't work right unless we refresh it manually for this
	//particular call.
	VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);
	VectorCopy(saberent->r.currentAngles, saberent->s.apos.trBase);
	if(saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] == FORCE_LEVEL_3)
	{
		saberReactivate(saberent, saberOwn);

		saberent->touch = SaberGotHit;

		saberent->think = saberBackToOwner;
		saberent->speed = 0;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		saberent->r.contents = CONTENTS_LIGHTSABER;
	}
	else
		saberKnockDown(saberent, saberOwn, saberOwn);

	
	/* basejka code - setting up the speed check stuff for returns.
	saberent->speed = 0;
	*/
	//[/SaberThrowSys]
	
}

#define SABER_MAX_THROW_DISTANCE 512
extern void G_StopObjectMoving( gentity_t *object );
void saberFirstThrown(gentity_t *saberent)
{//racc - this is the think function for live thrown sabers.
	vec3_t		vSub;
	float		vLen;
	gentity_t	*saberOwn = &g_entities[saberent->r.ownerNum];

	if (saberent->r.ownerNum == ENTITYNUM_NONE)
	{//racc - lost track of our owner.
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (!saberOwn ||
		!saberOwn->inuse ||
		!saberOwn->client ||
		saberOwn->client->sess.sessionTeam == TEAM_SPECTATOR)
	{//racc - owner is invalid.
		MakeDeadSaber(saberent);

		saberent->think = G_FreeEntity;
		saberent->nextthink = level.time;
		return;
	}

	if (saberOwn->health < 1 || !saberOwn->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
	{ //He's dead, just go back to our normal saber status
		saberent->touch = SaberGotHit;
		saberent->think = SaberUpdateSelf;
		saberent->genericValue5 = 0;
		saberent->nextthink = level.time;

		if (saberOwn->client &&
			saberOwn->client->saber[0].soundOff)
		{
			G_Sound(saberent, CHAN_AUTO, saberOwn->client->saber[0].soundOff);
		}
		MakeDeadSaber(saberent);

		saberent->r.svFlags |= (SVF_NOCLIENT);
		saberent->r.contents = CONTENTS_LIGHTSABER;
		SetSaberBoxSize(saberent);
		saberent->s.loopSound = 0;
		saberent->s.loopIsSoundset = qfalse;
		WP_SaberRemoveG2Model( saberent );

		saberOwn->client->ps.saberInFlight = qfalse;
		saberOwn->client->ps.saberEntityState = 0;
		saberOwn->client->ps.saberThrowDelay = level.time + 500;
		saberOwn->client->ps.saberCanThrow = qfalse;

		return;
	}

	//[SaberThrowSys]
	//Sorry Razor the saber doesn't return
	/*
	if ((level.time - saberOwn->client->ps.saberDidThrowTime) > 500)
	{
		//[SaberSys]
		if (!(saberOwn->client->buttons & BUTTON_ALT_ATTACK) && !((saberOwn->client->buttons & BUTTON_FORCEPOWER) && saberOwn->client->ps.fd.forcePowerSelected == FP_SABERTHROW) )
		//if (!(saberOwn->client->buttons & BUTTON_ALT_ATTACK))	
		//[/SaberSys]
		{ //If owner releases altattack 500ms or later after throwing saber, it autoreturns
			thrownSaberTouch(saberent, saberent, NULL);
			goto runMin;
		}
		else if ((level.time - saberOwn->client->ps.saberDidThrowTime) > 6000)
		{ //if it's out longer than 6 seconds, return it
			thrownSaberTouch(saberent, saberent, NULL);
			goto runMin;
		}
	}
	*/
	//[/SaberThrowSys]

	if (BG_HasYsalamiri(g_gametype.integer, &saberOwn->client->ps))
	{//racc - we have the Ysalamiri, we can't throw sabers.
		//[SaberThrowSys]
		//lost force concentration, switch the saber into ballistics mode.
		thrownSaberBallistics(saberent, saberOwn, qfalse);
		//thrownSaberTouch(saberent, saberent, NULL);
		//[/SaberThrowSys]
		goto runMin;
	}
	
	if (!BG_CanUseFPNow(g_gametype.integer, &saberOwn->client->ps, level.time, FP_SABERTHROW))
	{//racc - can't use saber throw at the moment.
		//[SaberThrowSys]
		//lost force concentration, switch the saber into ballistics mode.
		thrownSaberBallistics(saberent, saberOwn, qfalse);
		//thrownSaberTouch(saberent, saberent, NULL);
		//[/SaberThrowSys]
		goto runMin;
	}

	VectorSubtract(saberOwn->client->ps.origin, saberent->r.currentOrigin, vSub);
	vLen = VectorLength(vSub);

	if(vLen >= 300 && saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] == FORCE_LEVEL_1)
	{
		thrownSaberBallistics(saberent, saberOwn, qfalse);
		goto runMin;
	}

	if (vLen >= (SABER_MAX_THROW_DISTANCE*saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW]))
	{//racc - saber has reached maximum throw range, start returning the saber.
		//[SaberThrowSys]
		//lost force concentration, switch the saber into ballistics mode.
		thrownSaberBallistics(saberent, saberOwn, qfalse);
		//thrownSaberTouch(saberent, saberent, NULL);
		//[/SaberThrowSys]
		goto runMin;
	}

	//[SaberThrowSys]
	if( !((saberOwn->client->pers.cmd.buttons & BUTTON_SABERTHROW) 
		|| ((saberOwn->client->pers.cmd.buttons & BUTTON_FORCEPOWER) 
			&& saberOwn->client->ps.fd.forcePowerSelected == FP_SABERTHROW)) )
	{//we're not using the saber throw button, that means we don't want to maintain
		//our force concentration on holding the saber in the air.
		//run the object for this frame and then convert to a ballistic saber.
		if(saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			saberent->s.eFlags &= ~EF_MISSILE_STICK;

			saberReactivate(saberent, saberOwn);

			saberent->touch = SaberGotHit;

			saberent->think = saberBackToOwner;
			saberent->speed = 0;
			saberent->genericValue5 = 0;
			saberent->nextthink = level.time;

			saberent->r.contents = CONTENTS_LIGHTSABER;
		}
		else
		{
			G_RunObject(saberent);
			thrownSaberBallistics(saberent, saberOwn, qfalse);
		}
	}

	if (saberOwn->client->ps.fd.forcePowerDebounce[FP_SABERTHROW] < level.time
		&& saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
	{
		BG_ForcePowerDrain( &saberOwn->client->ps, FP_SABERTHROW, 2 );
		if (saberOwn->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(saberOwn, FP_SABERTHROW);
		}

		saberOwn->client->ps.fd.forcePowerDebounce[FP_SABERTHROW] = level.time + 1000;
	}

	// we don't want to have any physical control over the thrown saber's path. basejka code.
		//Yes we do!
	if (saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3 &&
		saberent->speed < level.time)
	{ 
		vec3_t fwd, traceFrom, traceTo, dir;
		trace_t tr;

		AngleVectors(saberOwn->client->ps.viewangles, fwd, 0, 0);

		VectorCopy(saberOwn->client->ps.origin, traceFrom);
		traceFrom[2] += saberOwn->client->ps.viewheight;

		VectorCopy(traceFrom, traceTo);
		if(saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{
			traceTo[0] += fwd[0]*2048;
			traceTo[1] += fwd[1]*2048;
			traceTo[2] += fwd[2]*2048;
		}
		else
		{
			//traceTo[0] += fwd[0];
			//traceTo[1] += fwd[1];
			//traceTo[2] += fwd[2];
		}

		//racc - move the saber for this frame based on it's current movement.
		saberMoveBack(saberent, qfalse);

		
		VectorCopy(saberent->r.currentOrigin, saberent->s.pos.trBase);
		
		if (saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{ //if highest saber throw rank, we can direct the saber toward players directly by looking at them
			trap_Trace(&tr, traceFrom, NULL, NULL, traceTo, saberOwn->s.number, MASK_PLAYERSOLID);
		}
		else
		{
			trap_Trace(&tr, traceFrom, NULL, NULL, traceTo, saberOwn->s.number, MASK_SOLID);
		}

		VectorSubtract(tr.endpos, saberent->r.currentOrigin, dir);

		VectorNormalize(dir);

		VectorScale(dir, 500, saberent->s.pos.trDelta );
		saberent->s.pos.trTime = level.time;

		//VectorCopy(dir,saberent->r.currentOrigin);

		if (saberOwn->client->ps.fd.forcePowerLevel[FP_SABERTHROW] >= FORCE_LEVEL_3)
		{ //we'll treat them to a quicker update rate if their throw rank is high enough
			saberent->speed = level.time + 300;
		}
		else
		{
			saberent->speed = level.time + 400;
		}
	}
	

	//[/SaberThrowSys]
runMin:

	//[SaberThrowSys]
	//don't do radius damage anymore.  We use the actual saber
	//racc - check for damage
	//saberCheckRadiusDamage(saberent, 0);
	//[/SaberThrowSys]
	G_RunObject(saberent);
}

void UpdateClientRenderBolts(gentity_t *self, vec3_t renderOrigin, vec3_t renderAngles)
{
	mdxaBone_t boltMatrix;
	renderInfo_t *ri = &self->client->renderInfo;

	if (!self->ghoul2)
	{
		VectorCopy(self->client->ps.origin, ri->headPoint);
		VectorCopy(self->client->ps.origin, ri->handRPoint);
		VectorCopy(self->client->ps.origin, ri->handLPoint);
		VectorCopy(self->client->ps.origin, ri->torsoPoint);
		VectorCopy(self->client->ps.origin, ri->crotchPoint);
		VectorCopy(self->client->ps.origin, ri->footRPoint);
		VectorCopy(self->client->ps.origin, ri->footLPoint);
	}
	else
	{
		//head
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->headBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->headPoint[0] = boltMatrix.matrix[0][3];
		ri->headPoint[1] = boltMatrix.matrix[1][3];
		ri->headPoint[2] = boltMatrix.matrix[2][3];

		//right hand
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->handRPoint[0] = boltMatrix.matrix[0][3];
		ri->handRPoint[1] = boltMatrix.matrix[1][3];
		ri->handRPoint[2] = boltMatrix.matrix[2][3];

		//left hand
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->handLBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->handLPoint[0] = boltMatrix.matrix[0][3];
		ri->handLPoint[1] = boltMatrix.matrix[1][3];
		ri->handLPoint[2] = boltMatrix.matrix[2][3];


		//chest
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->torsoBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->torsoPoint[0] = boltMatrix.matrix[0][3];
		ri->torsoPoint[1] = boltMatrix.matrix[1][3];
		ri->torsoPoint[2] = boltMatrix.matrix[2][3];

		//crotch
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->crotchBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->crotchPoint[0] = boltMatrix.matrix[0][3];
		ri->crotchPoint[1] = boltMatrix.matrix[1][3];
		ri->crotchPoint[2] = boltMatrix.matrix[2][3];

		//right foot
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->footRBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->footRPoint[0] = boltMatrix.matrix[0][3];
		ri->footRPoint[1] = boltMatrix.matrix[1][3];
		ri->footRPoint[2] = boltMatrix.matrix[2][3];

		//left foot
		trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->footLBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
		ri->footLPoint[0] = boltMatrix.matrix[0][3];
		ri->footLPoint[1] = boltMatrix.matrix[1][3];
		ri->footLPoint[2] = boltMatrix.matrix[2][3];
	}

	self->client->renderInfo.boltValidityTime = level.time;
}

void UpdateClientRenderinfo(gentity_t *self, vec3_t renderOrigin, vec3_t renderAngles)
{
	renderInfo_t *ri = &self->client->renderInfo;
	if ( ri->mPCalcTime < level.time )
	{
		//We're just going to give rough estimates on most of this stuff,
		//it's not like most of it matters.

	#if 0 //#if 0'd since it's a waste setting all this to 0 each frame.
		//Should you wish to make any of this valid then feel free to do so.
		ri->headYawRangeLeft = ri->headYawRangeRight = ri->headPitchRangeUp = ri->headPitchRangeDown = 0;
		ri->torsoYawRangeLeft = ri->torsoYawRangeRight = ri->torsoPitchRangeUp = ri->torsoPitchRangeDown = 0;

		ri->torsoFpsMod = ri->legsFpsMod = 0;

		VectorClear(ri->customRGB);
		ri->customAlpha = 0;
		ri->renderFlags = 0;
		ri->lockYaw = 0;

		VectorClear(ri->headAngles);
		VectorClear(ri->torsoAngles);

		//VectorClear(ri->eyeAngles);

		ri->legsYaw = 0;
	#endif

		if (self->ghoul2 &&
			self->ghoul2 != ri->lastG2)
		{ //the g2 instance changed, so update all the bolts.
			//rwwFIXMEFIXME: Base on skeleton used? Assuming humanoid currently.
			ri->lastG2 = self->ghoul2;

			if (self->localAnimIndex <= 1)
			{
				ri->headBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*head_eyes");
				ri->handRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_hand");
				ri->handLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_hand");
				ri->torsoBolt = trap_G2API_AddBolt(self->ghoul2, 0, "thoracic");
				ri->crotchBolt = trap_G2API_AddBolt(self->ghoul2, 0, "pelvis");
				ri->footRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_leg_foot");
				ri->footLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_leg_foot");
				ri->motionBolt = trap_G2API_AddBolt(self->ghoul2, 0, "Motion");
			}
			else
			{
				ri->headBolt = -1;
				ri->handRBolt = -1;
				ri->handLBolt = -1;
				ri->torsoBolt = -1;
				ri->crotchBolt = -1;
				ri->footRBolt = -1;
				ri->footLBolt = -1;
				ri->motionBolt = -1;
			}

			ri->lastG2 = self->ghoul2;
		}

		VectorCopy( self->client->ps.viewangles, self->client->renderInfo.eyeAngles );

		//we'll just say the legs/torso are whatever the first frame of our current anim is.
		ri->torsoFrame = bgAllAnims[self->localAnimIndex].anims[self->client->ps.torsoAnim].firstFrame;
		ri->legsFrame = bgAllAnims[self->localAnimIndex].anims[self->client->ps.legsAnim].firstFrame;
		if (g_debugServerSkel.integer)
		{	//Alright, I was doing this, but it's just too slow to do every frame.
			//From now on if we want this data to be valid we're going to have to make a verify call for it before
			//accessing it. I'm only doing this now if we want to debug the server skel by drawing lines from bolt
			//positions every frame.
			mdxaBone_t boltMatrix;

			if (!self->ghoul2)
			{
				VectorCopy(self->client->ps.origin, ri->headPoint);
				VectorCopy(self->client->ps.origin, ri->handRPoint);
				VectorCopy(self->client->ps.origin, ri->handLPoint);
				VectorCopy(self->client->ps.origin, ri->torsoPoint);
				VectorCopy(self->client->ps.origin, ri->crotchPoint);
				VectorCopy(self->client->ps.origin, ri->footRPoint);
				VectorCopy(self->client->ps.origin, ri->footLPoint);
			}
			else
			{
				//head
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->headBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->headPoint[0] = boltMatrix.matrix[0][3];
				ri->headPoint[1] = boltMatrix.matrix[1][3];
				ri->headPoint[2] = boltMatrix.matrix[2][3];

				//right hand
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->handRPoint[0] = boltMatrix.matrix[0][3];
				ri->handRPoint[1] = boltMatrix.matrix[1][3];
				ri->handRPoint[2] = boltMatrix.matrix[2][3];

				//left hand
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->handLBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->handLPoint[0] = boltMatrix.matrix[0][3];
				ri->handLPoint[1] = boltMatrix.matrix[1][3];
				ri->handLPoint[2] = boltMatrix.matrix[2][3];

				//chest
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->torsoBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->torsoPoint[0] = boltMatrix.matrix[0][3];
				ri->torsoPoint[1] = boltMatrix.matrix[1][3];
				ri->torsoPoint[2] = boltMatrix.matrix[2][3];

				//crotch
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->crotchBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->crotchPoint[0] = boltMatrix.matrix[0][3];
				ri->crotchPoint[1] = boltMatrix.matrix[1][3];
				ri->crotchPoint[2] = boltMatrix.matrix[2][3];

				//right foot
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->footRBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->footRPoint[0] = boltMatrix.matrix[0][3];
				ri->footRPoint[1] = boltMatrix.matrix[1][3];
				ri->footRPoint[2] = boltMatrix.matrix[2][3];

				//left foot
				trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->footLBolt, &boltMatrix, renderAngles, renderOrigin, level.time, NULL, self->modelScale);
				ri->footLPoint[0] = boltMatrix.matrix[0][3];
				ri->footLPoint[1] = boltMatrix.matrix[1][3];
				ri->footLPoint[2] = boltMatrix.matrix[2][3];
			}

			//Now draw the skel for debug
			G_TestLine(ri->headPoint, ri->torsoPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->handRPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->handLPoint, 0x000000ff, 50);
			G_TestLine(ri->torsoPoint, ri->crotchPoint, 0x000000ff, 50);
			G_TestLine(ri->crotchPoint, ri->footRPoint, 0x000000ff, 50);
			G_TestLine(ri->crotchPoint, ri->footLPoint, 0x000000ff, 50);
		}

		//muzzle point calc (we are going to be cheap here)
		VectorCopy(ri->muzzlePoint, ri->muzzlePointOld);
		VectorCopy(self->client->ps.origin, ri->muzzlePoint);
		VectorCopy(ri->muzzleDir, ri->muzzleDirOld);
		AngleVectors(self->client->ps.viewangles, ri->muzzleDir, 0, 0);
		ri->mPCalcTime = level.time;

		VectorCopy(self->client->ps.origin, ri->eyePoint);
		ri->eyePoint[2] += self->client->ps.viewheight;
	}
}

#define STAFF_KICK_RANGE 16
extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex ); //NPC_utils.c

extern qboolean BG_InKnockDown( int anim );
#if 0
static qboolean G_KickDownable(gentity_t *ent)
{
	if (!d_saberKickTweak.integer)
	{
		return qtrue;
	}

	if (!ent || !ent->inuse || !ent->client)
	{
		return qfalse;
	}

	if (BG_InKnockDown(ent->client->ps.legsAnim) ||
		BG_InKnockDown(ent->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (ent->client->ps.weaponTime <= 0 &&
		ent->client->ps.weapon == WP_SABER &&
		ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		return qfalse;
	}

	return qtrue;
}
#endif

//[KnockdownSys]
/* racc - replaced with SP style knockdowns.
static void G_TossTheMofo(gentity_t *ent, vec3_t tossDir, float tossStr)
{
	if (!ent->inuse || !ent->client)
	{ //no good
		return;
	}

	if (ent->s.eType == ET_NPC && ent->s.NPC_class == CLASS_VEHICLE)
	{ //no, silly
		return;
	}

	VectorMA(ent->client->ps.velocity, tossStr, tossDir, ent->client->ps.velocity);
	ent->client->ps.velocity[2] = 200;
	if (ent->health > 0 && ent->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN &&
		BG_KnockDownable(&ent->client->ps) &&
		G_KickDownable(ent))
	{ //if they are alive, knock them down I suppose
		ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
		ent->client->ps.forceHandExtendTime = level.time + 700;
		ent->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
		//ent->client->ps.quickerGetup = qtrue;
	}
}
*/
//[/KnockdownSys]

//[DodgeSys]
qboolean OJP_DodgeKick( gentity_t *self, gentity_t *pusher, const vec3_t pushDir )
{//a function similar to the Jedi_StopKnockdown function, except it's only a fake backflip in this case.
	vec3_t pLAngles, pLFwd;

	if( !self || !self->client )
	{//non-humanoids can't dodge kicks.
		return qfalse;
	}

	if ( self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1 )
	{//only force-users
		return qfalse;
	}

	if ( self->client->ps.eFlags2&EF2_FLYING ) //moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qfalse;
	}
    
	if (self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY)
	{
		return qfalse;
	}
	if (self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT
		&& pusher->client->ps.fd.saberAnimLevel == SS_DESANN)//can't block if we're too off balance and their using juyo. Juyo's perk
	{
		return qfalse;
	}

	if(self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT
		|| self->client->ps.stats[STAT_DODGE] <= DODGE_CRITICALLEVEL )
	{//above the light mishap level or at low DP, you don't automatically do dodge kicks.
		if(self->r.svFlags & SVF_BOT)
		{//bots cheat and auto counter kicks.
			if(Q_irand(0, 2))
			{//failed!
				return qfalse;
			}
		}
		else if(!(self->client->buttons & BUTTON_15))
		{
			return qfalse;
		}

	}

	if ( self->client->ps.stats[STAT_DODGE] < DODGE_KICKCOST )
	{//not enough DP to avoid this kick
		return qfalse;
	}

	if(BG_InGrappleMove(self->client->ps.torsoAnim))
	{//can't block while grappling
		return qfalse;
	}

	if(!WalkCheck(self))
	{//running players can't do this
		return qfalse;
	}

	if(self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't dodge kicks while in midair
		return qfalse;
	}

	VectorSet(pLAngles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors( pLAngles, pLFwd, NULL, NULL );
	if ( DotProduct( pLFwd, pushDir ) > 0.2f )
	{//not hit in the front, can't dodge it.
		return qfalse;
	}

	//self->client->ps.legsTimer = 0;
	//self->client->ps.torsoTimer = 0;
	if(pusher->client->ps.legsAnim == BOTH_A7_KICK_B)//for the slap
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_RUNBACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
		G_DodgeDrain(self, pusher, DODGE_KICKCOST);
	}
	else
	{
	G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_FLIP_BACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
	//[ExpSys]
	G_DodgeDrain(self, pusher, DODGE_KICKCOST);
	//self->client->ps.stats[STAT_DODGE] -= DODGE_KICKCOST;
	//[/ExpSys]
	}
	return qtrue;
}

//[KnockdownSys]
extern void G_ThrownDeathAnimForDeathAnim( gentity_t *hitEnt, vec3_t impactPoint );
//[/KnockdownSys]
//[SaberSys]
extern void BG_ReduceMishapLevel(playerState_t *ps);
//[/SaberSys]
static gentity_t *G_KickTrace( gentity_t *ent, vec3_t kickDir, float kickDist, vec3_t kickEnd, int kickDamage, float kickPush )
{
	vec3_t	traceOrg, traceEnd, kickMins, kickMaxs;
	trace_t	trace;
	gentity_t	*hitEnt = NULL;
	VectorSet(kickMins, -6.0f, -6.0f, -6.0f);
	VectorSet(kickMaxs, 6.0f, 6.0f, 6.0f);
	//FIXME: variable kick height?
	if ( kickEnd && !VectorCompare( kickEnd, vec3_origin ) )
	{//they passed us the end point of the trace, just use that
		//this makes the trace flat
		VectorSet( traceOrg, ent->r.currentOrigin[0], ent->r.currentOrigin[1], kickEnd[2] );
		VectorCopy( kickEnd, traceEnd );
	}
	else
	{//extrude
		VectorSet( traceOrg, ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2]+ent->r.maxs[2]*0.5f );
		VectorMA( traceOrg, kickDist, kickDir, traceEnd );
	}

	if (d_saberKickTweak.integer)
	{
		trap_G2Trace( &trace, traceOrg, kickMins, kickMaxs, traceEnd, ent->s.number, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
	}
	else
	{
		trap_Trace( &trace, traceOrg, kickMins, kickMaxs, traceEnd, ent->s.number, MASK_SHOT);
	}




	//G_TestLine(traceOrg, traceEnd, 0x0000ff, 5000);
	//[SaberSys]
	/* racc - debug message
	if(trace.startsolid || trace.allsolid)
	{
		G_Printf("G_KickTrace was startsolid or allsolid.\n");
	}
	*/

	//improving the hit detection for the kicks.  
	if ( trace.fraction < 1.0f || trace.startsolid )
	//if ( trace.fraction < 1.0f && !trace.startsolid && !trace.allsolid )
	//[/SaberSys]
	{
		if (ent->client->jediKickTime > level.time)
		{
			if (trace.entityNum == ent->client->jediKickIndex)
			{ //we are hitting the same ent we last hit in this same anim, don't hit it again
				return NULL;
			}
		}
		ent->client->jediKickIndex = trace.entityNum;
		ent->client->jediKickTime = level.time + ent->client->ps.legsTimer;

		hitEnt = &g_entities[trace.entityNum];
		//FIXME: regardless of what we hit, do kick hit sound and impact effect
		//G_PlayEffect( "misc/kickHit", trace.endpos, trace.plane.normal );
	
		if ( ent->client->ps.torsoAnim == BOTH_A7_HILT )
		{
			G_Sound( ent, CHAN_AUTO, G_SoundIndex( "sound/movers/objects/saber_slam" ) );
		}
		else
		{
			G_Sound( ent, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );
			
		}
		if ( hitEnt->inuse )
		{//we hit an entity
			//[BugFix29]
			//don't allow players to kick private duels.
			if (hitEnt->client &&
			hitEnt->client->ps.duelInProgress &&
			hitEnt->client->ps.duelIndex != ent->s.number)
			{//their dueling and we're not their dueling partner
				return (hitEnt);
			}

			if (hitEnt->client &&
				ent->client->ps.duelInProgress &&
				ent->client->ps.duelIndex != hitEnt->s.number)
			{
				return (hitEnt);
			}
			//[/BugFix29]
			//[DodgeSys]
			if(OJP_DodgeKick(hitEnt, ent, kickDir))
			{//but the lucky devil dodged it by backflipping
				//toss them back a bit and make sure that the kicker gets the kill if the 
				//player falls off a cliff or something.
				if (hitEnt->client)
				{
					hitEnt->client->ps.otherKiller = ent->s.number;
					hitEnt->client->ps.otherKillerDebounceTime = level.time + 10000;
					hitEnt->client->ps.otherKillerTime = level.time + 10000;
					//[Asteroids]
					hitEnt->client->otherKillerMOD = MOD_MELEE;
					hitEnt->client->otherKillerVehWeapon = 0;
					hitEnt->client->otherKillerWeaponType = WP_NONE;
					//[Asteroids]
				}

				G_Throw( hitEnt, kickDir, kickPush );
				return (hitEnt);
			}
			//[/DodgeSys]

			//FIXME: don't hit same ent more than once per kick
			if ( hitEnt->takedamage )
			{//hurt it
				if (hitEnt->client)
				{
					hitEnt->client->ps.otherKiller = ent->s.number;
					hitEnt->client->ps.otherKillerDebounceTime = level.time + 10000;
					hitEnt->client->ps.otherKillerTime = level.time + 10000;
					//[Asteroids]
					hitEnt->client->otherKillerMOD = MOD_MELEE;
					hitEnt->client->otherKillerVehWeapon = 0;
					hitEnt->client->otherKillerWeaponType = WP_NONE;
					//[Asteroids]

					//[FatigueSys]
					if(PM_SaberInBrokenParry(hitEnt->client->ps.saberMove))
					{//kicks do fatigue damage if they were stunned.
						BG_ForcePowerDrain(&hitEnt->client->ps, FP_GRIP, FATIGUE_KICKHIT);
					}
					//[/FatigueSys]
				}

				if (d_saberKickTweak.integer)
				{
					G_Damage( hitEnt, ent, ent, kickDir, trace.endpos, kickDamage*0.2f, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
				else
				{
					G_Damage( hitEnt, ent, ent, kickDir, trace.endpos, kickDamage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
				}
			}
			if ( hitEnt->client 
				&& !(hitEnt->client->ps.pm_flags&PMF_TIME_KNOCKBACK) //not already flying through air?  Intended to stop multiple hits, but...
				&& G_CanBeEnemy(ent, hitEnt) )
			{//FIXME: this should not always work
				if ( hitEnt->health <= 0 )
				{//we kicked a dead guy
					//throw harder - FIXME: no matter how hard I push them, they don't go anywhere... corpses use less physics???
					//[KnockdownSys]
					//reenabled SP code since the knockdown code now based on the SP code again.
					G_Throw( hitEnt, kickDir, kickPush*4 );
					//see if we should play a better looking death on them
					G_ThrownDeathAnimForDeathAnim( hitEnt, trace.endpos );
					//G_TossTheMofo(hitEnt, kickDir, kickPush*4.0f);
					//[/KnockdownSys]
				}
				else
				{
					//[KnockdownSys]
					//reenabled SP code since the knockdown code now based on the SP code again.
					G_Throw( hitEnt, kickDir, kickPush*10 );

					//[SaberSys]
					//made the knockdown behavior of kicks be based on the player's mishap level or low DP and not hold alt attack.
					if ((hitEnt->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY
						|| hitEnt->client->ps.stats[STAT_DODGE] <= DODGE_CRITICALLEVEL)
						&& !(hitEnt->client->buttons & BUTTON_ALT_ATTACK))
					{//knockdown
						if(hitEnt->client->ps.fd.saberAnimLevel == SS_STAFF)
						{
							SabBeh_AnimateSlowBounce(hitEnt,ent);
						}
						else
						{
							if ( kickPush >= 75.0f && !Q_irand( 0, 2 ) )
							{
								G_Knockdown( hitEnt, ent, kickDir, 300, qtrue );
							}
							else
							{
								G_Knockdown( hitEnt, ent, kickDir, kickPush, qtrue );
							}
						}
					}
					else if (ent->client->ps.fd.saberAnimLevel == SS_DESANN
						&& (hitEnt->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT
						|| hitEnt->client->ps.stats[STAT_DODGE] <= DODGE_CRITICALLEVEL)
						&& !(hitEnt->client->buttons & BUTTON_ALT_ATTACK))
					{//knockdown
						if ( kickPush >= 75.0f && !Q_irand( 0, 2 ) )
							{
								G_Knockdown( hitEnt, ent, kickDir, 300, qtrue );
							}
						else
							{
								G_Knockdown( hitEnt, ent, kickDir, kickPush, qtrue );
							}
						}
					else
					{//stumble
						AnimateStun(hitEnt, ent, trace.endpos);   
					}

					BG_ReduceMishapLevel(&hitEnt->client->ps);
					//[/SaberSys]

					/* original basejka code
					if ( kickPush >= 75.0f && !Q_irand( 0, 2 ) )
					{
						G_TossTheMofo(hitEnt, kickDir, 300.0f);
					}
					else
					{
						G_TossTheMofo(hitEnt, kickDir, kickPush);
					}
					*/
					//[/KnockdownSys]
				}
			}
		}
	}
	return (hitEnt);
}


//[Melee]
//Maded the punching system animation based so we can use it whenever by
//just calling the punch animation.
static void G_PunchSomeMofos(gentity_t *ent)
{
	vec3_t mins, maxs;
	trace_t tr;
	vec3_t Hand;
	renderInfo_t *ri = &ent->client->renderInfo;
	int BoltIndex;

	if( ent->client->ps.torsoAnim == BOTH_MELEE1)
	{
		BoltIndex = ri->handLBolt;
	}
	else
	{
		BoltIndex = ri->handRBolt;
	}


	if( BoltIndex != -1 )
	{//safety check, this shouldn't ever not be valid
		G_GetBoltPosition(ent, BoltIndex, Hand, 0);

		//Set bbox size
		VectorSet( maxs, 6, 6, 6 );
		VectorScale( maxs, -1, mins );

		trap_G2Trace( &tr, ent->client->ps.origin, mins, maxs, Hand, ent->s.number, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
		//G_RealTrace(NULL, &tr, ent->client->ps.origin, mins, maxs, Hand, ent->s.number, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT)); 

		if (tr.fraction != 1)
		{ //hit something
			vec3_t forward;
			gentity_t * tr_ent = &g_entities[tr.entityNum];

			VectorSubtract(Hand, ent->client->ps.origin, forward);
			VectorNormalize(forward);

			//Prevent punches from hitting the same entity more than once
			if (ent->client->jediKickTime > level.time)
			{
				if (tr.entityNum == ent->client->jediKickIndex)
				{ //we are hitting the same ent we last hit in this same anim, don't hit it again
					return;
				}
			}
			ent->client->jediKickIndex = tr.entityNum;
			ent->client->jediKickTime = level.time + ent->client->ps.torsoTimer;
			
			G_Sound( ent, CHAN_AUTO, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );

			if (tr_ent->takedamage && tr_ent->client)
			{ //special duel checks
				if (tr_ent->client->ps.duelInProgress &&
					tr_ent->client->ps.duelIndex != ent->s.number)
				{
					return;
				}

				if (ent->client &&
					ent->client->ps.duelInProgress &&
					ent->client->ps.duelIndex != tr_ent->s.number)
				{
					return;
				}

				tr_ent->client->ps.otherKiller = ent->s.number;
				tr_ent->client->ps.otherKillerDebounceTime = level.time + 10000;
				tr_ent->client->ps.otherKillerTime = level.time + 10000;
				//[Asteroids]
				tr_ent->client->otherKillerMOD = MOD_MELEE;
				tr_ent->client->otherKillerVehWeapon = 0;
				tr_ent->client->otherKillerWeaponType = WP_NONE;
				//[Asteroids]
			}

			if ( tr_ent->takedamage )
			{ //damage them, do more damage if we're in the second right hook
			int dmg = MELEE_SWING1_DAMAGE;
			
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_3)
				{	
					dmg = 4*MELEE_SWING1_DAMAGE;
				}
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_2)
				{	
					dmg = 3*MELEE_SWING1_DAMAGE;
				}
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_1)
				{	
					dmg = 2*MELEE_SWING1_DAMAGE;
				}			

				if (ent->client && ent->client->ps.torsoAnim == BOTH_MELEE2)
				{ //do a tad bit more damage on the second swing
					dmg = MELEE_SWING2_DAMAGE;
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_3)
				{	
					dmg = 4*MELEE_SWING2_DAMAGE;
				}
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_2)
				{	
					dmg = 3*MELEE_SWING2_DAMAGE;
				}
				if(ent->client->skillLevel[SK_STRENGTH] == FORCE_LEVEL_1)
				{	
					dmg = 2*MELEE_SWING2_DAMAGE;
				}
				}

				if ( G_HeavyMelee( ent ) )
				{ //2x damage for heavy melee class
					dmg *= 2;
				}

				G_Damage( tr_ent, ent, ent, forward, tr.endpos, dmg, DAMAGE_NO_ARMOR, MOD_MELEE );
			}
		}
	}
}
//[/Melee]

static void G_KickSomeMofos(gentity_t *ent)
{
	vec3_t	kickDir, kickEnd, fwdAngs;
	float animLength = BG_AnimLength( ent->localAnimIndex, (animNumber_t)ent->client->ps.legsAnim );
	float elapsedTime = (float)(animLength-ent->client->ps.legsTimer);
	float remainingTime = (animLength-elapsedTime);
	float kickDist = (ent->r.maxs[0]*1.5f)+STAFF_KICK_RANGE+8.0f;//fudge factor of 8
	int	  kickDamage = Q_irand(10, 15);//Q_irand( 3, 8 ); //since it can only hit a guy once now
	int	  kickPush = flrand( 50.0f, 100.0f );
	qboolean doKick = qfalse;
	renderInfo_t *ri = &ent->client->renderInfo;

	VectorSet(kickDir, 0.0f, 0.0f, 0.0f);
	VectorSet(kickEnd, 0.0f, 0.0f, 0.0f);
	VectorSet(fwdAngs, 0.0f, ent->client->ps.viewangles[YAW], 0.0f);

	//HMM... or maybe trace from origin to footRBolt/footLBolt?  Which one?  G2 trace?  Will do hitLoc, if so...
	if ( ent->client->ps.torsoAnim == BOTH_A7_HILT )
	{
		if ( elapsedTime >= 250 && remainingTime >= 250 )
		{//front
			doKick = qtrue;
			if ( ri->handRBolt != -1 )
			{//actually trace to a bolt
				G_GetBoltPosition( ent, ri->handRBolt, kickEnd, 0 );
				VectorSubtract( kickEnd, ent->client->ps.origin, kickDir );
				kickDir[2] = 0;//ah, flatten it, I guess...
				VectorNormalize( kickDir );
			}
			else
			{//guess
				AngleVectors( fwdAngs, kickDir, NULL, NULL );
			}
		}
	}
	else
	{
		switch ( ent->client->ps.legsAnim | ent->client->ps.torsoAnim)//added this for slap, not sure if it will help
		{
		case BOTH_GETUP_BROLL_B:
		case BOTH_GETUP_BROLL_F:
		case BOTH_GETUP_FROLL_B:
		case BOTH_GETUP_FROLL_F:
			if ( elapsedTime >= 250 && remainingTime >= 250 )
			{//front
				doKick = qtrue;
				if ( ri->footRBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->client->ps.origin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
			}
			break;
		case BOTH_A7_KICK_F_AIR:
		case BOTH_A7_KICK_B_AIR:
		case BOTH_A7_KICK_R_AIR:
		case BOTH_A7_KICK_L_AIR:
			if ( elapsedTime >= 100 && remainingTime >= 250 )
			{//air
				doKick = qtrue;
				if ( ri->footRBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
			}
			break;
		case BOTH_A7_KICK_F:
			//FIXME: push forward?
			if ( elapsedTime >= 250 && remainingTime >= 250 )
			{//front
				doKick = qtrue;
				if ( ri->footRBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
			}
			break;
		case BOTH_A7_KICK_B:
			//FIXME: push back?
			if ( elapsedTime >= 415 && remainingTime >= 200 )
			{//back
				doKick = qtrue;
				kickDist=70;
				if ( ri->handLBolt != -1 )//changed to accomadate teh new hand hand anim
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->handLBolt, kickEnd, 0 );//changed to accomadate teh new hand hand anim
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
			}
			break;
		case BOTH_A7_KICK_R:
			//FIXME: push right?
			if ( elapsedTime >= 250 && remainingTime >= 250 )
			{//right
				doKick = qtrue;
				if ( ri->footRBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
				}
			}
			break;
		case BOTH_A7_KICK_L:
			//FIXME: push left?
			if ( elapsedTime >= 250 && remainingTime >= 250 )
			{//left
				doKick = qtrue;
				if ( ri->footLBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footLBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
				}
				else
				{//guess
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
			}
			break;
		case BOTH_A7_KICK_S:
			kickPush = flrand( 75.0f, 125.0f );
			if ( ri->footRBolt != -1 )
			{//actually trace to a bolt
				if ( elapsedTime >= 550 
					&& elapsedTime <= 1050 )
				{
					doKick = qtrue;
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA( kickEnd, 8.0f, kickDir, kickEnd );
				}
			}
			else
			{//guess
				if ( elapsedTime >= 400 && elapsedTime < 500 )
				{//front
					doKick = qtrue;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
				else if ( elapsedTime >= 500 && elapsedTime < 600 )
				{//front-right?
					doKick = qtrue;
					fwdAngs[YAW] += 45;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
				else if ( elapsedTime >= 600 && elapsedTime < 700 )
				{//right
					doKick = qtrue;
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
				}
				else if ( elapsedTime >= 700 && elapsedTime < 800 )
				{//back-right?
					doKick = qtrue;
					fwdAngs[YAW] += 45;
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
				}
				else if ( elapsedTime >= 800 && elapsedTime < 900 )
				{//back
					doKick = qtrue;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
				else if ( elapsedTime >= 900 && elapsedTime < 1000 )
				{//back-left?
					doKick = qtrue;
					fwdAngs[YAW] += 45;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
				else if ( elapsedTime >= 1000 && elapsedTime < 1100 )
				{//left
					doKick = qtrue;
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
				else if ( elapsedTime >= 1100 && elapsedTime < 1200 )
				{//front-left?
					doKick = qtrue;
					fwdAngs[YAW] += 45;
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
			}
			break;
		case BOTH_A7_KICK_BF:
			kickPush = flrand( 75.0f, 125.0f );
			kickDist += 20.0f;
			if ( elapsedTime < 1500 )
			{//auto-aim!
	//			overridAngles = PM_AdjustAnglesForBFKick( ent, ucmd, fwdAngs, qboolean(elapsedTime<850) )?qtrue:overridAngles;
				//FIXME: if we haven't done the back kick yet and there's no-one there to
				//			kick anymore, go into some anim that returns us to our base stance
			}
			if ( ri->footRBolt != -1 )
			{//actually trace to a bolt
				if ( ( elapsedTime >= 750 && elapsedTime < 850 )
					|| ( elapsedTime >= 1400 && elapsedTime < 1500 ) )
				{//right, though either would do
					doKick = qtrue;
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA( kickEnd, 8, kickDir, kickEnd );
				}
			}
			else if ( ri->handLBolt != -1) //for hand slap
			{//actually trace to a bolt
				if ( ( elapsedTime >= 750 && elapsedTime < 860 )
					|| ( elapsedTime >= 1400 && elapsedTime < 1500 ) )
				{//right, though either would do
					doKick = qtrue;
					G_Printf(":wtf:");
					G_GetBoltPosition( ent, ri->handLBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA( kickEnd, 8, kickDir, kickEnd );
				}
			}
			else
			{//guess
				if ( elapsedTime >= 250 && elapsedTime < 350 )
				{//front
					doKick = qtrue;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
				}
				else if ( elapsedTime >= 350 && elapsedTime < 450 )
				{//back
					doKick = qtrue;
					AngleVectors( fwdAngs, kickDir, NULL, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
			}
			break;
		case BOTH_A7_KICK_RL:
			kickPush = flrand( 75.0f, 125.0f );
			kickDist += 20.0f;

			//ok, I'm tracing constantly on these things, they NEVER hit otherwise (in MP at least)

			//FIXME: auto aim at enemies on the side of us?
			//overridAngles = PM_AdjustAnglesForRLKick( ent, ucmd, fwdAngs, qboolean(elapsedTime<850) )?qtrue:overridAngles;
			//if ( elapsedTime >= 250 && elapsedTime < 350 )
			if (level.framenum&1)
			{//right
				doKick = qtrue;
				if ( ri->footRBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footRBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA( kickEnd, 8, kickDir, kickEnd );
				}
				else
				{//guess
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
				}
			}
			//else if ( elapsedTime >= 350 && elapsedTime < 450 )
			else
			{//left
				doKick = qtrue;
				if ( ri->footLBolt != -1 )
				{//actually trace to a bolt
					G_GetBoltPosition( ent, ri->footLBolt, kickEnd, 0 );
					VectorSubtract( kickEnd, ent->r.currentOrigin, kickDir );
					kickDir[2] = 0;//ah, flatten it, I guess...
					VectorNormalize( kickDir );
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA( kickEnd, 8, kickDir, kickEnd );
				}
				else
				{//guess
					AngleVectors( fwdAngs, NULL, kickDir, NULL );
					VectorScale( kickDir, -1, kickDir );
				}
			}
			break;
		}
	}

	if ( doKick )
	{
		//[MeleeSys]
		//use the actual foot bolt positions rather than hackish directions for more accurate kicks.
		G_KickTrace( ent, kickDir, kickDist, kickEnd, kickDamage, kickPush );
		//G_KickTrace( ent, kickDir, kickDist, NULL, kickDamage, kickPush );
		//[/MeleeSys]
	}
}

GAME_INLINE qboolean G_PrettyCloseIGuess(float a, float b, float tolerance)
{
    if ((a-b) < tolerance &&
		(a-b) > -tolerance)
	{
		return qtrue;
	}

	return qfalse;
}

static void G_GrabSomeMofos(gentity_t *self)
{
	renderInfo_t *ri = &self->client->renderInfo;
	mdxaBone_t boltMatrix;
	vec3_t flatAng;
	vec3_t pos;
	vec3_t grabMins, grabMaxs;
	trace_t trace;

	if (!self->ghoul2 || ri->handRBolt == -1)
	{ //no good
		return;
	}

    VectorSet(flatAng, 0.0f, self->client->ps.viewangles[1], 0.0f);
	trap_G2API_GetBoltMatrix(self->ghoul2, 0, ri->handRBolt, &boltMatrix, flatAng, self->client->ps.origin,
		level.time, NULL, self->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, pos);

	VectorSet(grabMins, -4.0f, -4.0f, -4.0f);
	VectorSet(grabMaxs, 4.0f, 4.0f, 4.0f);

	//trace from my origin to my hand, if we hit anyone then get 'em
	trap_G2Trace( &trace, self->client->ps.origin, grabMins, grabMaxs, pos, self->s.number, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
    
	if (trace.fraction != 1.0f &&
		trace.entityNum < ENTITYNUM_WORLD)
	{
		gentity_t *grabbed = &g_entities[trace.entityNum];

		if (grabbed->inuse && (grabbed->s.eType == ET_PLAYER || grabbed->s.eType == ET_NPC) &&
			grabbed->client && grabbed->health > 0 &&
			G_CanBeEnemy(self, grabbed) &&
			G_PrettyCloseIGuess(grabbed->client->ps.origin[2], self->client->ps.origin[2], 4.0f) &&
			(!BG_InGrappleMove(grabbed->client->ps.torsoAnim) || grabbed->client->ps.torsoAnim == BOTH_KYLE_GRAB) &&
			(!BG_InGrappleMove(grabbed->client->ps.legsAnim) || grabbed->client->ps.legsAnim == BOTH_KYLE_GRAB)
			&& grabbed->s.NPC_class != CLASS_VEHICLE)
		{ //grabbed an active player/npc
			int tortureAnim = -1;
			int correspondingAnim = -1;

			if (self->client->pers.cmd.forwardmove > 0)
			{ //punch grab
				tortureAnim = BOTH_KYLE_PA_1;
				correspondingAnim = BOTH_PLAYER_PA_1;
			}
			else if (self->client->pers.cmd.forwardmove < 0)
			{ //knee-throw
				tortureAnim = BOTH_KYLE_PA_2;
				correspondingAnim = BOTH_PLAYER_PA_2;
			}
			//[HEADLOCK]
			else
			{ //head lock
				tortureAnim = BOTH_KYLE_PA_3;
				correspondingAnim = BOTH_PLAYER_PA_3;
			}
			//[/HEADLOCK]

			if (tortureAnim == -1 || correspondingAnim == -1)
			{
				if (self->client->ps.torsoTimer < 300 && !self->client->grappleState)
				{ //you failed to grab anyone, play the "failed to grab" anim
					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
					if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
					{ //providing the anim set succeeded..
						self->client->ps.weaponTime = self->client->ps.torsoTimer;
					}
				}
				return;
			}

			self->client->grappleIndex = grabbed->s.number;
			self->client->grappleState = 1;

			grabbed->client->grappleIndex = self->s.number;
			grabbed->client->grappleState = 20;

			//time to crack some heads
			G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, tortureAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			if (self->client->ps.torsoAnim == tortureAnim)
			{ //providing the anim set succeeded..
				self->client->ps.weaponTime = self->client->ps.torsoTimer;
			}

			G_SetAnim(grabbed, &grabbed->client->pers.cmd, SETANIM_BOTH, correspondingAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			if (grabbed->client->ps.torsoAnim == correspondingAnim)
			{ //providing the anim set succeeded..
				if (grabbed->client->ps.weapon == WP_SABER)
				{ //turn it off
					if (!grabbed->client->ps.saberHolstered)
					{
						grabbed->client->ps.saberHolstered = 2;
						if (grabbed->client->saber[0].soundOff)
						{
							G_Sound(grabbed, CHAN_AUTO, grabbed->client->saber[0].soundOff);
						}
						if (grabbed->client->saber[1].soundOff &&
							grabbed->client->saber[1].model[0])
						{
							G_Sound(grabbed, CHAN_AUTO, grabbed->client->saber[1].soundOff);
						}
					}
				}
				if (grabbed->client->ps.torsoTimer < self->client->ps.torsoTimer)
				{ //make sure they stay in the anim at least as long as the grabber
					grabbed->client->ps.torsoTimer = self->client->ps.torsoTimer;
				}
				grabbed->client->ps.weaponTime = grabbed->client->ps.torsoTimer;
			}
		}
	}

	if (self->client->ps.torsoTimer < 300 && !self->client->grappleState)
	{ //you failed to grab anyone, play the "failed to grab" anim
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
		if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
		{ //providing the anim set succeeded..
			self->client->ps.weaponTime = self->client->ps.torsoTimer;
		}
	}
}

void Q3_SetInvisible( int entID, qboolean invisible );
Vehicle_t *G_IsRidingVehicle( gentity_t *pEnt );
extern gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle );

//[SaberLockSys]
//used for server side new saber lock clash effect.  Attempting a bgame solution instead.
//extern float ShortestLineSegBewteen2LineSegs( vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2 );
//[/SaberLockSys]
void WP_SaberPositionUpdate( gentity_t *self, usercmd_t *ucmd )
{ //rww - keep the saber position as updated as possible on the server so that we can try to do realistic-looking contact stuff
  //Note that this function also does the majority of working in maintaining the server g2 client instance (updating angles/anims/etc)
	gentity_t *mySaber = NULL;
	mdxaBone_t	boltMatrix;
	vec3_t properAngles, properOrigin;
	vec3_t boltAngles, boltOrigin;
	vec3_t end;
	vec3_t legAxis[3];
	//[SaberSys]
	//vec3_t addVel;
	//[/SaberSys]
	vec3_t rawAngles;
	//[SaberSys]
	//float fVSpeed = 0;
	//[/SaberSys]
	int returnAfterUpdate = 0;
	float animSpeedScale = 1.0f;
	int saberNum;
	qboolean clientOverride;
	gentity_t *vehEnt = NULL;
	int rSaberNum = 0;
	int rBladeNum = 0;

	//[SaberSys]
#ifndef FINAL_BUILD
	int viewlock = 0;
#endif
	//[/SaberSys]

#ifdef _DEBUG
	if (g_disableServerG2.integer)
	{
		return;
	}
#endif

	if (self && self->inuse && self->client)
	{
		if (self->client->saberCycleQueue)
		{
			self->client->ps.fd.saberDrawAnimLevel = self->client->saberCycleQueue;
		}
		else
		{
			self->client->ps.fd.saberDrawAnimLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (self &&
		self->inuse &&
		self->client &&
		self->client->saberCycleQueue &&
		(self->client->ps.weaponTime <= 0 || self->health < 1))
	{ //we cycled attack levels while we were busy, so update now that we aren't (even if that means we're dead)
		self->client->ps.fd.saberAnimLevel = self->client->saberCycleQueue;
		self->client->saberCycleQueue = 0;
	}

	if (!self ||
		!self->inuse ||
		!self->client ||
		!self->ghoul2 ||
		!g2SaberInstance)
	{
		return;
	}

//[SaberSys]
#ifndef FINAL_BUILD
	viewlock = self->client->ps.userInt1;
#endif

	//[/SnapThrow]
	
	//I'm leaving these tests in so someone might find more open buttons eventually.
	/*

	if (ucmd->buttons & BUTTON_15)
	{
		G_Printf("Button Flag 15 Pressed.\n");
	}
	*/
	//[SnapThrow]
	if (ucmd->buttons & BUTTON_THERMALTHROW)
	{//player wants to snap throw a gernade
		if(self->client->ps.weaponTime <= 0//not currently using a weapon
			&& self->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_THERMAL ) && self->client->ps.ammo[AMMO_THERMAL] > 0 )//have a thermal
		{//throw!
			self->s.weapon = WP_THERMAL;  //temp switch weapons so we can toss it.
			self->client->ps.weaponChargeTime = level.time - 450; //throw at medium power
			FireWeapon( self, qfalse );
			self->s.weapon = self->client->ps.weapon; //restore weapon
			self->client->ps.weaponTime = weaponData[WP_THERMAL].fireTime;
			self->client->ps.ammo[AMMO_THERMAL]--;
			G_SetAnim(self, NULL, SETANIM_TORSO, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			
		}
	}

	//[/SnapThrow]

	
	if (ucmd->buttons & BUTTON_16)
	{
		G_Printf("Button Flag 16 Pressed.\n");
	}
	//BUTTON_17 seems to get trigger when you land hard and then randomly triggers afterwards.
	//if (ucmd->buttons & BUTTON_17)
	//{
	//	G_Printf("Button Flag 17 Pressed.\n");
	//}
	if (ucmd->buttons & BUTTON_18)
	{
		G_Printf("Button Flag 18 Pressed.\n");
	}
	if (ucmd->buttons & BUTTON_19)
	{
		G_Printf("Button Flag 19 Pressed.\n");
	}
	if (ucmd->buttons & BUTTON_20)
	{
		G_Printf("Button Flag 20 Pressed.\n");
	}
//[/SaberSys]

	//[Melee]
	//Punching damage/effects now handled by animation vs as an attack event
	if( BG_PunchAnim(self->client->ps.torsoAnim) )
	{
		G_PunchSomeMofos(self);
	}
	else if (BG_KickingAnim(self->client->ps.legsAnim))
	//if (BG_KickingAnim(self->client->ps.legsAnim))
	//[/Melee]
	{ //do some kick traces and stuff if we're in the appropriate anim
		G_KickSomeMofos(self);
	}
	else if (self->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{ //try to grab someone
		G_GrabSomeMofos(self);
	}
	else if (self->client->grappleState)
	{
		gentity_t *grappler = &g_entities[self->client->grappleIndex];

		if (!grappler->inuse || !grappler->client || grappler->client->grappleIndex != self->s.number ||
			!BG_InGrappleMove(grappler->client->ps.torsoAnim) || !BG_InGrappleMove(grappler->client->ps.legsAnim) ||
			!BG_InGrappleMove(self->client->ps.torsoAnim) || !BG_InGrappleMove(self->client->ps.legsAnim) ||
			!self->client->grappleState || !grappler->client->grappleState ||
			grappler->health < 1 || self->health < 1 ||
			!G_PrettyCloseIGuess(self->client->ps.origin[2], grappler->client->ps.origin[2], 4.0f))
		{
			self->client->grappleState = 0;
		
			//[MELEE]
			//Special case for BOTH_KYLE_PA_3 This don't look correct otherwise.
			if ( self->client->ps.torsoAnim == BOTH_KYLE_PA_3 && (self->client->ps.torsoTimer > 100 ||
				self->client->ps.legsTimer > 100))
			{
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE, 0);
				self->client->ps.weaponTime = self->client->ps.torsoTimer = 0;
			}
			//if ((BG_InGrappleMove(self->client->ps.torsoAnim) && self->client->ps.torsoTimer > 100) ||
			//	(BG_InGrappleMove(self->client->ps.legsAnim) && self->client->ps.legsTimer > 100))
			else if ((BG_InGrappleMove(self->client->ps.torsoAnim) && self->client->ps.torsoTimer > 100) ||
				(BG_InGrappleMove(self->client->ps.legsAnim) && self->client->ps.legsTimer > 100))
			//[/MELEE]
			{ //if they're pretty far from finishing the anim then shove them into another anim
				G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
				if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
				{ //providing the anim set succeeded..
					self->client->ps.weaponTime = self->client->ps.torsoTimer;
				}
			}
		}
		else
		{
			vec3_t grapAng;

			VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, grapAng);

			if (VectorLength(grapAng) > 64.0f)
			{ //too far away, break it off
				if ((BG_InGrappleMove(self->client->ps.torsoAnim) && self->client->ps.torsoTimer > 100) ||
					(BG_InGrappleMove(self->client->ps.legsAnim) && self->client->ps.legsTimer > 100))
				{
					self->client->grappleState = 0;

					G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
					if (self->client->ps.torsoAnim == BOTH_KYLE_MISS)
					{ //providing the anim set succeeded..
						self->client->ps.weaponTime = self->client->ps.torsoTimer;
					}
				}
			}
			else
			{
				vectoangles(grapAng, grapAng);
				SetClientViewAngle(self, grapAng);

				if (self->client->grappleState >= 20)
				{ //grapplee
					//try to position myself at the correct distance from my grappler
					float idealDist;
					vec3_t gFwd, idealSpot;
					trace_t trace;

					if (grappler->client->ps.torsoAnim == BOTH_KYLE_PA_1)
					{ //grab punch
						idealDist = 46.0f;
					}
					//[MELEE]
					//else
					else if (grappler->client->ps.torsoAnim == BOTH_KYLE_PA_2)
					{ //knee-throw
						idealDist = 46.0f;
						//idealDist = 34.0f;
					}
					else
					{ //head lock
						idealDist = 46.0f;
					}
					

					AngleVectors(grappler->client->ps.viewangles, gFwd, 0, 0);
					VectorMA(grappler->client->ps.origin, idealDist, gFwd, idealSpot);

					trap_Trace(&trace, self->client->ps.origin, self->r.mins, self->r.maxs, idealSpot, self->s.number, self->clipmask);
					if (!trace.startsolid && !trace.allsolid && trace.fraction == 1.0f)
					{ //go there
						G_SetOrigin(self, idealSpot);
						VectorCopy(idealSpot, self->client->ps.origin);
					}
				}
				else if (self->client->grappleState >= 1)
				{ //grappler
					if (grappler->client->ps.weapon == WP_SABER)
					{ //make sure their saber is shut off
						if (!grappler->client->ps.saberHolstered)
						{
							grappler->client->ps.saberHolstered = 2;
							if (grappler->client->saber[0].soundOff)
							{
								G_Sound(grappler, CHAN_AUTO, grappler->client->saber[0].soundOff);
							}
							if (grappler->client->saber[1].soundOff &&
								grappler->client->saber[1].model[0])
							{
								G_Sound(grappler, CHAN_AUTO, grappler->client->saber[1].soundOff);
							}
						}
					}

					//check for smashy events
					if (self->client->ps.torsoAnim == BOTH_KYLE_PA_1)
					{ //grab punch
                        if (self->client->grappleState == 1)
						{ //smack
							if (self->client->ps.torsoTimer < 3400)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 10, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 2)
						{ //smack!
							if (self->client->ps.torsoTimer < 2550)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 10, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else
						{ //SMACK!
							if (self->client->ps.torsoTimer < 1300)
							{
								vec3_t tossDir;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 30, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								self->client->grappleState = 0;

								VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, tossDir);
								VectorNormalize(tossDir);
								VectorScale(tossDir, 500.0f, tossDir);
								tossDir[2] = 200.0f;

								VectorAdd(grappler->client->ps.velocity, tossDir, grappler->client->ps.velocity);

								if (grappler->health > 0)
								{ //if still alive knock them down
									grappler->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									grappler->client->ps.forceHandExtendTime = level.time + 1300;

									//Count as kill for attacker if the other player falls to his death.
									grappler->client->ps.otherKiller = self->s.number;
									grappler->client->ps.otherKillerTime = level.time + 8000;
									grappler->client->ps.otherKillerDebounceTime = level.time + 100;
								}
							}
						}
					}
					else if (self->client->ps.torsoAnim == BOTH_KYLE_PA_2)
					{ //knee throw
                        if (self->client->grappleState == 1)
						{ //knee to the face
							if (self->client->ps.torsoTimer < 3200)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 20, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 2)
						{ //smashed on the ground
							if (self->client->ps.torsoTimer < 2000)
							{
								//G_Damage(grappler, self, self, NULL, self->client->ps.origin, 10, 0, MOD_MELEE);
								//don't do damage on this one, it would look very freaky if they died
								G_EntitySound( grappler, CHAN_VOICE, G_SoundIndex("*pain100.wav") );
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );
								self->client->grappleState++;
							}
						}
						else
						{ //and another smash
							if (self->client->ps.torsoTimer < 1000)
							{
								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 30, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoTimer = 1000;
									//[MELEE]
									//Play get up animation and make sure you're facing the right direction
									//set the correct exit player angle
									SetClientViewAngle(grappler, grapAng);
									G_SetAnim(grappler, &grappler->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
									//[MELEE]

									grappler->client->grappleState = 0;
								}
								else
								{ //override death anim
									//[MELEE]
									//set the correct exit player angle
									SetClientViewAngle(grappler, grapAng);
									//[/MELEE]
									grappler->client->ps.torsoAnim = BOTH_DEADFLOP1;
									grappler->client->ps.legsAnim = BOTH_DEADFLOP1;

								}

								self->client->grappleState = 0;
							}
						}
					}
					//[MELEE]
					else if (self->client->ps.torsoAnim == BOTH_KYLE_PA_3)
					{//Head Lock
						if ((self->client->grappleState == 1 && self->client->ps.torsoTimer < 5034) ||
							(self->client->grappleState == 2 && self->client->ps.torsoTimer < 3965)) //||
							//(self->client->grappleState == 3 && self->client->ps.torsoTimer < 3138))
							
						{//choke noises				
							//G_EntitySound( grappler, CHAN_VOICE, G_SoundIndex(va( "*choke%d.wav", Q_irand( 1, 3 ) )) );
							self->client->grappleState++;
						}
						else if (self->client->grappleState == 3)
						{ //throw to ground 
							if (self->client->ps.torsoTimer < 2379)
							{
								int grapplerAnim = grappler->client->ps.torsoAnim;
								int grapplerTime = grappler->client->ps.torsoTimer;

								G_Damage(grappler, self, self, NULL, self->client->ps.origin, 50, 0, MOD_MELEE);
								//G_Sound( grappler, CHAN_AUTO, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );

								//it might try to put them into a pain anim or something, so override it back again
								if (grappler->health > 0)
								{
									grappler->client->ps.torsoAnim = grapplerAnim;
									grappler->client->ps.torsoTimer = grapplerTime;
									grappler->client->ps.legsAnim = grapplerAnim;
									grappler->client->ps.legsTimer = grapplerTime;
									grappler->client->ps.weaponTime = grapplerTime;
								}
								else
								{//he bought it.  Make sure it looks right.
									//set the correct exit player angle
									SetClientViewAngle(grappler, grapAng);

									grappler->client->ps.torsoAnim = BOTH_DEADFLOP1;
									grappler->client->ps.legsAnim = BOTH_DEADFLOP1;
								}
								self->client->grappleState++;
							}
						}
						else if (self->client->grappleState == 4)
						{	
							if (self->client->ps.torsoTimer < 758)
							{
								vec3_t tossDir;

								VectorSubtract(grappler->client->ps.origin, self->client->ps.origin, tossDir);
								VectorNormalize(tossDir);
								VectorScale(tossDir, 500.0f, tossDir);
								tossDir[2] = 200.0f;

								VectorAdd(grappler->client->ps.velocity, tossDir, grappler->client->ps.velocity);
								
								if (grappler->health > 0)
								{//racc - knock this mofo down
									grappler->client->ps.torsoTimer = 1000;
									//G_SetAnim(grappler, &grappler->client->pers.cmd, SETANIM_BOTH, BOTH_PLAYER_PA_3_FLY, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
									grappler->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									grappler->client->ps.forceHandExtendTime = level.time + 1300;
									grappler->client->grappleState = 0;
									
									//Count as kill for attacker if the other player falls to his death.
									grappler->client->ps.otherKiller = self->s.number;
									grappler->client->ps.otherKillerTime = level.time + 8000;
									grappler->client->ps.otherKillerDebounceTime = level.time + 100;
								}
							}
						}
					}
					//[/MELEE]
					else
					{ //?
					}
				}
			}
		}
	}

	//If this is a listen server (client+server running on same machine),
	//then lets try to steal the skeleton/etc data off the client instance
	//for this entity to save us processing time.
	clientOverride = trap_G2API_OverrideServer(self->ghoul2);

	saberNum = self->client->ps.saberEntityNum;

	if (!saberNum)
	{
		saberNum = self->client->saberStoredIndex;
	}

	if (!saberNum)
	{
		returnAfterUpdate = 1;
		goto nextStep;
	}

	mySaber = &g_entities[saberNum];

	if (self->health < 1)
	{ //we don't want to waste precious CPU time calculating saber positions for corpses. But we want to avoid the saber ent position lagging on spawn, so..
		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && ((mySaber->r.contents & CONTENTS_LIGHTSABER) || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{ //Since we haven't got a bolt position, place it on top of the player origin.
			VectorCopy(self->client->ps.origin, mySaber->r.currentOrigin);
		}
	
		//I don't want to return now actually, I want to keep g2 instances for corpses up to
		//date because I'm doing better corpse hit detection/dismem (particularly for the
		//npc's)
		//return;
	}

	if ( BG_SuperBreakWinAnim( self->client->ps.torsoAnim ) )
	{//racc - we've won a saber lock, indicate that we're firing.
		self->client->ps.weaponstate = WEAPON_FIRING;
	}
	if (self->client->ps.weapon != WP_SABER ||
		self->client->ps.weaponstate == WEAPON_RAISING ||
		self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->health < 1)
	{//racc - we're not using a lightsaber.
		if (!self->client->ps.saberInFlight)
		{//racc - and our saber isn't in flight. skip the saber damage stuff.
			returnAfterUpdate = 1;
		}
	}

	if (self->client->ps.saberThrowDelay < level.time)
	{//racc - update saberCanThrow.
		if ( (self->client->saber[0].saberFlags&SFL_NOT_THROWABLE) )
		{//cant throw it normally!
			if ( (self->client->saber[0].saberFlags&SFL_SINGLE_BLADE_THROWABLE) )
			{//but can throw it if only have 1 blade on
				if ( self->client->saber[0].numBlades > 1
					&& self->client->ps.saberHolstered == 1 )
				{//have multiple blades and only one blade on
					self->client->ps.saberCanThrow = qtrue;//qfalse;
					//huh? want to be able to throw then right?
				}
				else
				{//multiple blades on, can't throw
					self->client->ps.saberCanThrow = qfalse;
				}
			}
			else
			{//never can throw it
				self->client->ps.saberCanThrow = qfalse;
			}
		}
		else
		{//can throw it!
			self->client->ps.saberCanThrow = qtrue;
		}
	}
nextStep:
	if (self->client->ps.fd.forcePowersActive & (1 << FP_RAGE))
	{
		animSpeedScale = 2;
	}
	
	VectorCopy(self->client->ps.origin, properOrigin);

	//[SaberSys]
	//racc - I think this is a terrible idea.  The server instance is always "correct".
	//the clients can suck it.
	/*
	//try to predict the origin based on velocity so it's more like what the client is seeing
	VectorCopy(self->client->ps.velocity, addVel);
	VectorNormalize(addVel);

	if (self->client->ps.velocity[0] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[0]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[0];
	}
	if (self->client->ps.velocity[1] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[1]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[1];
	}
	if (self->client->ps.velocity[2] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[2]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[2];
	}

	//fVSpeed *= 0.08;
	fVSpeed *= 1.6f/g_svfps.value;

	//Cap it off at reasonable values so the saber box doesn't go flying ahead of us or
	//something if we get a big speed boost from something.
	if (fVSpeed > 70)
	{
		fVSpeed = 70;
	}
	if (fVSpeed < -70)
	{
		fVSpeed = -70;
	}

	properOrigin[0] += addVel[0]*fVSpeed;
	properOrigin[1] += addVel[1]*fVSpeed;
	properOrigin[2] += addVel[2]*fVSpeed;
	*/
	//[/SaberSys]

	properAngles[0] = 0;
	if (self->s.number < MAX_CLIENTS && self->client->ps.m_iVehicleNum)
	{
		vehEnt = &g_entities[self->client->ps.m_iVehicleNum];
		if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
		{
			properAngles[1] = vehEnt->m_pVehicle->m_vOrientation[YAW];
		}
		else
		{
			properAngles[1] = self->client->ps.viewangles[YAW];
			vehEnt = NULL;
		}
	}
	else
	{
		properAngles[1] = self->client->ps.viewangles[YAW];
	}
	properAngles[2] = 0;

	AnglesToAxis( properAngles, legAxis );

	UpdateClientRenderinfo(self, properOrigin, properAngles);

	if (!clientOverride)
	{ //if we get the client instance we don't need to do this
		G_G2PlayerAngles( self, legAxis, properAngles );
	}

	if (vehEnt)
	{
		properAngles[1] = vehEnt->m_pVehicle->m_vOrientation[YAW];
	}

	if (returnAfterUpdate && saberNum)
	{ //We don't even need to do GetBoltMatrix if we're only in here to keep the g2 server instance in sync
		//but keep our saber entity in sync too, just copy it over our origin.

		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && ((mySaber->r.contents & CONTENTS_LIGHTSABER) || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{ //Since we haven't got a bolt position, place it on top of the player origin.
			VectorCopy(self->client->ps.origin, mySaber->r.currentOrigin);
		}

		goto finalUpdate;
	}

	if (returnAfterUpdate)
	{
		goto finalUpdate;
	}

	//racc - at this point, we're going to do the calculations for sabers.  This only
	//runs if we're alive and have the saber selected. (it doesn't have to be on or
	//in our hand.

	//We'll get data for blade 0 first no matter what it is and stick them into
	//the constant ("_Always") values. Later we will handle going through each blade.
	trap_G2API_GetBoltMatrix(self->ghoul2, 1, 0, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrigin);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, boltAngles);

	//immediately store these values so we don't have to recalculate this again
	if (self->client->lastSaberStorageTime && (level.time - self->client->lastSaberStorageTime) < 200)
	{ //alright
		VectorCopy(self->client->lastSaberBase_Always, self->client->olderSaberBase);
		self->client->olderIsValid = qtrue;
	}
	else
	{
		self->client->olderIsValid = qfalse;
	}

	VectorCopy(boltOrigin, self->client->lastSaberBase_Always);
	VectorCopy(boltAngles, self->client->lastSaberDir_Always);
	self->client->lastSaberStorageTime = level.time;

	VectorCopy(boltAngles, rawAngles);

	VectorMA( boltOrigin, self->client->saber[0].blade[0].lengthMax, boltAngles, end );

	if (self->client->ps.saberEntityNum)
	{//racc - update the position of our saber entity if it's not in flight.
		//I guess it's good to keep the position updated even when contents are 0
		if (mySaber && ((mySaber->r.contents & CONTENTS_LIGHTSABER) || mySaber->r.contents == 0) && !self->client->ps.saberInFlight)
		{ //place it roughly in the middle of the saber..
			VectorMA( boltOrigin, self->client->saber[0].blade[0].lengthMax, boltAngles, mySaber->r.currentOrigin );
		}
	}

	//racc - I think this is set this way so that the saber always starts facing in the 
	//direction the player is facing.  This isn't techincally the detection of the saber 
	//blade anymore.
	boltAngles[YAW] = self->client->ps.viewangles[YAW];

/*	{
		static int lastDTime = 0;
		if (lastDTime < level.time)
		{
			G_TestLine(boltOrigin, end, 0x0000ff, 200);
			lastDTime = level.time + 200;
		}
	}
*/
	if (self->client->ps.saberInFlight)
	{ //do the thrown-saber stuff
		gentity_t *saberent = &g_entities[saberNum];

		if (saberent)
		{
			if (!self->client->ps.saberEntityState && self->client->ps.saberEntityNum)
			{//RACC - starting the saber throw, since our saber is supposed to be inflight and we haven't lost it.
				vec3_t startorg, startang, dir;

				VectorCopy(boltOrigin, saberent->r.currentOrigin);

				VectorCopy(boltOrigin, startorg);
				VectorCopy(boltAngles, startang);

				//[SaberThrowSys]
				//We now want the saber to imitate the actual spinning like what occurs
				//on the client side.
				startang[0] = 90;
				//[/SaberThrowSys]
				//Instead of this we'll sort of fake it and slowly tilt it down on the client via
				//a perframe method (which doesn't actually affect where or how the saber hits)

				saberent->r.svFlags &= ~(SVF_NOCLIENT);
				VectorCopy(startorg, saberent->s.pos.trBase);
				VectorCopy(startang, saberent->s.apos.trBase);

				VectorCopy(startorg, saberent->s.origin);
				VectorCopy(startang, saberent->s.angles);

				saberent->s.saberInFlight = qtrue;

				saberent->s.apos.trType = TR_LINEAR;
				saberent->s.apos.trDelta[0] = 0;
				saberent->s.apos.trDelta[1] = 800;
				saberent->s.apos.trDelta[2] = 0;

				saberent->s.pos.trType = TR_LINEAR;
				saberent->s.eType = ET_GENERAL;
				saberent->s.eFlags = 0;

				WP_SaberAddG2Model( saberent, self->client->saber[0].model, self->client->saber[0].skin );

				saberent->s.modelGhoul2 = 127;

				saberent->parent = self;

				self->client->ps.saberEntityState = 1;

				//Projectile stuff:
				AngleVectors(self->client->ps.viewangles, dir, NULL, NULL);

				saberent->nextthink = level.time + FRAMETIME;
				saberent->think = saberFirstThrown;

				saberent->damage = SABER_THROWN_HIT_DAMAGE;
				saberent->methodOfDeath = MOD_SABER;
				saberent->splashMethodOfDeath = MOD_SABER;
				saberent->s.solid = 2;
				saberent->r.contents = CONTENTS_LIGHTSABER;

				saberent->genericValue5 = 0;

				VectorSet( saberent->r.mins, SABERMINS_X, SABERMINS_Y, SABERMINS_Z );
				VectorSet( saberent->r.maxs, SABERMAXS_X, SABERMAXS_Y, SABERMAXS_Z );

				saberent->s.genericenemyindex = self->s.number+1024;

				saberent->touch = thrownSaberTouch;

				saberent->s.weapon = WP_SABER;

				VectorScale(dir, 400, saberent->s.pos.trDelta );
				saberent->s.pos.trTime = level.time;

				if ( self->client->saber[0].spinSound )
				{
					saberent->s.loopSound = self->client->saber[0].spinSound;
				}
				else
				{
					saberent->s.loopSound = saberSpinSound;
				}
				saberent->s.loopIsSoundset = qfalse;

				//[SaberThrowSys]
				//racc - we don't really need this variable anymore.
				//self->client->ps.saberDidThrowTime = level.time;
				//[/SaberThrowSys]

				self->client->dangerTime = level.time;
				self->client->ps.eFlags &= ~EF_INVULNERABLE;
				self->client->invulnerableTimer = 0;
				saberent->s.otherEntityNum = self->client->ps.clientNum;
				trap_LinkEntity(saberent);
			}
			else if (self->client->ps.saberEntityNum) //only do this stuff if your saber is active and has not been knocked out of the air.
			{//racc - this a life tossed saber.
				//racc - update pos1 (the origin of first saber blade)
				VectorCopy(boltOrigin, saberent->pos1);
				trap_LinkEntity(saberent);

				if (saberent->genericValue5 == PROPER_THROWN_VALUE)
				{ //return to the owner now, this is a bad state to be in for here..
					saberent->genericValue5 = 0;
					saberent->think = SaberUpdateSelf;
					saberent->nextthink = level.time;
					WP_SaberRemoveG2Model( saberent );
					
					self->client->ps.saberInFlight = qfalse;
					self->client->ps.saberEntityState = 0;
					self->client->ps.saberThrowDelay = level.time + 500;
					self->client->ps.saberCanThrow = qfalse;
				}
			}
		}
	}

	/*
	if (self->client->ps.saberInFlight)
	{ //if saber is thrown then only do the standard stuff for the left hand saber
		rSaberNum = 1;
	}
	*/

	if (!BG_SabersOff(&self->client->ps))
	{
		gentity_t *saberent = &g_entities[saberNum];

		if (!self->client->ps.saberInFlight && saberent)
		{//RACC - make sure the saber entity is the correct size if it's in the hand and
		//turned on.  This also resets the saber entity after it returns to the owner's hand.
			saberent->r.svFlags |= (SVF_NOCLIENT);
			saberent->r.contents = CONTENTS_LIGHTSABER;
			SetSaberBoxSize(saberent);
			saberent->s.loopSound = 0;
			saberent->s.loopIsSoundset = qfalse;
		}

		//racc - Don't do hit detection/updating on your saber while in saber locks.  
		//Bare in mind that you're going to still have hit detection for the blade that starts the saberlock.
		if (self->client->ps.saberLockTime > level.time && self->client->ps.saberEntityNum)
		{
			//[SaberLockSys]
			//racc - this is the server side method of rendering the new saberlock effect.
			//however, I've decided to do a bgame event method to reduce the lag potential of this badboy.
			/* 
			gentity_t *lockEnemy = &g_entities[self->client->ps.saberLockEnemy];
			
			//do a clash effect at the intersection point.
			if(lockEnemy && lockEnemy->inuse && self->client->ps.saberIdleWound < level.time)
			{
				vec3_t enemyBladeStart;
				vec3_t enemyBladeTip;
				vec3_t tempPoint;

				VectorCopy(lockEnemy->client->saber[0].blade[0].muzzlePoint, enemyBladeStart);
				VectorMA( lockEnemy->client->saber[0].blade[0].muzzlePoint, 
					lockEnemy->client->saber[0].blade[0].length, 
					lockEnemy->client->saber[0].blade[0].muzzleDir, enemyBladeTip );
				ShortestLineSegBewteen2LineSegs( boltOrigin, end, enemyBladeStart, enemyBladeTip, saberClashPos, tempPoint);

				saberDoClashEffect = qtrue;

				VectorSubtract(tempPoint, saberClashPos, saberClashNorm);
				VectorNormalize(saberClashNorm);

				saberClashOther = lockEnemy->s.number;

				WP_SaberDoClash(self, 0, 0);

				self->client->ps.saberIdleWound = level.time + 50;

			}
			*/
			//[/SaberLockSys]

			/* yeah, this looks stupid with the new saber impact effects.
			if (self->client->ps.saberIdleWound < level.time)
			{//RACC - spark effects for saberlocks
				gentity_t *te;
				vec3_t dir;
				te = G_TempEntity( g_entities[saberNum].r.currentOrigin, EV_SABER_BLOCK );
				VectorSet( dir, 0, 1, 0 );
				VectorCopy(g_entities[saberNum].r.currentOrigin, te->s.origin);
				VectorCopy(dir, te->s.angles);
				te->s.eventParm = 1;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				self->client->ps.saberIdleWound = level.time + Q_irand(400, 600);
			}
			*/
			//[/SaberLockSys]

			while (rSaberNum < MAX_SABERS)
			{
				rBladeNum = 0;
				while (rBladeNum < self->client->saber[rSaberNum].numBlades)
				{ //Don't bother updating the bolt for each blade for this, it's just a very rough fallback method for during saberlocks
					VectorCopy(boltOrigin, self->client->saber[saberNum].blade[rBladeNum].trail.base);
					VectorCopy(end, self->client->saber[saberNum].blade[rBladeNum].trail.tip);
					self->client->saber[saberNum].blade[rBladeNum].trail.lastTime = level.time;

					rBladeNum++;
				}

				rSaberNum++;
			}
			self->client->hasCurrentPosition = qtrue;

			self->client->ps.saberBlocked = BLOCKED_NONE;

			goto finalUpdate;
		}

		//reset it in case we used it for cycling before
		rSaberNum = rBladeNum = 0;

		if (self->client->ps.saberInFlight)
		{ //if saber is thrown then only do the standard stuff for the left hand saber
			if (!self->client->ps.saberEntityNum)
			{ //however, if saber is not in flight but rather knocked away, our left saber is off, and thus we may do nothing.
				rSaberNum = 1;//was 2?
			}
			else
			{//thrown saber still in flight, so do damage
				rSaberNum = 0;//was 1?
			}
		}

		//[SaberSys]
		//The saber doesn't use this system for damage dealing anymore.
		//WP_SaberClearDamage();
		//[/SaberSys]

		//racc - reset the clash flag.
		saberDoClashEffect = qfalse;

		//Now cycle through each saber and each blade on the saber and do damage traces.
		while (rSaberNum < MAX_SABERS)
		{
			if (!self->client->saber[rSaberNum].model[0])
			{
				rSaberNum++;
				continue;
			}

			/*
			if (rSaberNum == 0 && (self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
			{ //don't do saber 0 is the right arm is broken
				rSaberNum++;
				continue;
			}
			*/
			//for now I'm keeping a broken right arm swingable, it will just look and act damaged
			//but still be useable
			
			if (rSaberNum == 1 && (self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
			{ //don't to saber 1 if the left arm is broken
				break;
			}
			
			if (rSaberNum > 0 
				&& self->client->saber[1].model[0]
				//[SaberThrowSys]
				//add special case to do saber damage for second saber if the player has dropped their first saber.
				&& self->client->ps.saberHolstered == 1
				&& (!self->client->ps.saberInFlight || self->client->ps.saberEntityNum) )
				//&& self->client->ps.saberHolstered == 1 )
				//[/SaberThrowSys]
			{ //don't to saber 2 if it's off
				break;
			}
			rBladeNum = 0;
			while (rBladeNum < self->client->saber[rSaberNum].numBlades)
			{
				//update muzzle data for the blade
				VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePointOld);
				VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDirOld);

				if ( rBladeNum > 0 //more than one blade
					&& (!self->client->saber[1].model[0])//not using dual blades
					&& self->client->saber[rSaberNum].numBlades > 1//using a multi-bladed saber
					&& self->client->ps.saberHolstered == 1 )//
				{ //don't to extra blades if they're off
					break;
				}
				//get the new data
				//then update the bolt pos/dir. rBladeNum corresponds to the bolt index because blade bolts are added in order.
				if ( rSaberNum == 0 && self->client->ps.saberInFlight )
				{//RACC - Thrown saber 
					if ( !self->client->ps.saberEntityNum )
					{//dropped it... shouldn't get here, but...
						//assert(0);
						//FIXME: It's getting here a lot actually....
						rSaberNum++;
						rBladeNum = 0;
						continue;
					}
					else
					{
						gentity_t *saberEnt = &g_entities[self->client->ps.saberEntityNum];
						vec3_t saberOrg, saberAngles;
						if ( !saberEnt 
							|| !saberEnt->inuse
							|| !saberEnt->ghoul2 )
						{//wtf?
							rSaberNum++;
							rBladeNum = 0;
							continue;
						}
						if ( saberent->s.saberInFlight )
						{//spinning
							BG_EvaluateTrajectory( &saberEnt->s.pos, level.time+50, saberOrg );
							BG_EvaluateTrajectory( &saberEnt->s.apos, level.time+50, saberAngles );
						}
						else
						{//coming right back
							vec3_t saberDir;
							BG_EvaluateTrajectory( &saberEnt->s.pos, level.time, saberOrg );
							VectorSubtract( self->r.currentOrigin, saberOrg, saberDir );
							vectoangles( saberDir, saberAngles );
						}
						trap_G2API_GetBoltMatrix(saberEnt->ghoul2, 0, rBladeNum, &boltMatrix, saberAngles, saberOrg, level.time, NULL, self->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint);
						BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir);
						VectorCopy( self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, boltOrigin );
						VectorMA( boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].lengthMax, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end );
					}

				}
				else
				{	//[EnhancedImpliment] - Actually I think this should be done before the 
					//damage checks instead of after.  This should make the blade collision
					//detection work better.
					//racc - update the saber blade position stuff.
					trap_G2API_GetBoltMatrix(self->ghoul2, rSaberNum+1, rBladeNum, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint);
					BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir);
					VectorCopy( self->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, boltOrigin );
					VectorMA( boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].lengthMax, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end );
				}

				self->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;

				//[SaberSys]
				if (self->client->hasCurrentPosition && d_saberInterpolate.integer == 1)
				//if (self->client->hasCurrentPosition && d_saberInterpolate.integer)
				//[/SaberSys]
				{
					if (self->client->ps.weaponTime <= 0)
					{ //rww - 07/17/02 - don't bother doing the extra stuff unless actually attacking. This is in attempt to save CPU.
						CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qfalse);
					}
					else if (d_saberInterpolate.integer == 1)
					{
						int trMask = CONTENTS_LIGHTSABER|CONTENTS_BODY;
						int sN = 0;
						qboolean gotHit = qfalse;
						qboolean clientUnlinked[MAX_CLIENTS];
						qboolean skipSaberTrace = qfalse;
						
						if (!g_saberTraceSaberFirst.integer)
						{
							skipSaberTrace = qtrue;
						}
						else if (g_saberTraceSaberFirst.integer >= 2 &&
							g_gametype.integer != GT_DUEL &&
							g_gametype.integer != GT_POWERDUEL &&
							!self->client->ps.duelInProgress)
						{ //if value is >= 2, and not in a duel, skip
							skipSaberTrace = qtrue;
						}

						if (skipSaberTrace)
						{ //skip the saber-contents-only trace and get right to the full trace
							trMask = (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT);
						}
						else
						{
							while (sN < MAX_CLIENTS)
							{
								if (g_entities[sN].inuse && g_entities[sN].client && g_entities[sN].r.linked && g_entities[sN].health > 0 && (g_entities[sN].r.contents & CONTENTS_BODY))
								{ //Take this mask off before the saber trace, because we want to hit the saber first
									g_entities[sN].r.contents &= ~CONTENTS_BODY;
									clientUnlinked[sN] = qtrue;
								}
								else
								{
									clientUnlinked[sN] = qfalse;
								}
								sN++;
							}
						}

						while (!gotHit)
						{
							if (!CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qfalse, trMask, qfalse))
							{
								if (!CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qtrue, trMask, qfalse))
								{
									vec3_t oldSaberStart;
									vec3_t oldSaberEnd;
									vec3_t saberAngleNow;
									vec3_t saberAngleBefore;
									vec3_t saberMidDir;
									vec3_t saberMidAngle;
									vec3_t saberMidPoint;
									vec3_t saberMidEnd;
									vec3_t saberSubBase;
									float deltaX, deltaY, deltaZ;

									if ( (level.time-self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime) > 100 )
									{//no valid last pos, use current
										VectorCopy(boltOrigin, oldSaberStart);
										VectorCopy(end, oldSaberEnd);
									}
									else
									{//trace from last pos
										VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].trail.base, oldSaberStart);
										VectorCopy(self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, oldSaberEnd);
									}

									VectorSubtract(oldSaberEnd, oldSaberStart, saberAngleBefore);
									vectoangles(saberAngleBefore, saberAngleBefore);

									VectorSubtract(end, boltOrigin, saberAngleNow);
									vectoangles(saberAngleNow, saberAngleNow);

									deltaX = AngleDelta(saberAngleBefore[0], saberAngleNow[0]);
									deltaY = AngleDelta(saberAngleBefore[1], saberAngleNow[1]);
									deltaZ = AngleDelta(saberAngleBefore[2], saberAngleNow[2]);

									if ( (deltaX != 0 || deltaY != 0 || deltaZ != 0) && deltaX < 180 && deltaY < 180 && deltaZ < 180 && (BG_SaberInAttack(self->client->ps.saberMove) || PM_SaberInTransition(self->client->ps.saberMove)) )
									{ //don't go beyond here if we aren't attacking/transitioning or the angle is too large.
									//and don't bother if the angle is the same
										saberMidAngle[0] = saberAngleBefore[0] + (deltaX/2);
										saberMidAngle[1] = saberAngleBefore[1] + (deltaY/2);
										saberMidAngle[2] = saberAngleBefore[2] + (deltaZ/2);

										//Now that I have the angle, I'll just say the base for it is the difference between the two start
										//points (even though that's quite possibly completely false)
										VectorSubtract(boltOrigin, oldSaberStart, saberSubBase);
										saberMidPoint[0] = boltOrigin[0] + (saberSubBase[0]*0.5);
										saberMidPoint[1] = boltOrigin[1] + (saberSubBase[1]*0.5);
										saberMidPoint[2] = boltOrigin[2] + (saberSubBase[2]*0.5);

										AngleVectors(saberMidAngle, saberMidDir, 0, 0);
										saberMidEnd[0] = saberMidPoint[0] + saberMidDir[0]*self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;
										saberMidEnd[1] = saberMidPoint[1] + saberMidDir[1]*self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;
										saberMidEnd[2] = saberMidPoint[2] + saberMidDir[2]*self->client->saber[rSaberNum].blade[rBladeNum].lengthMax;

										//I'll just trace straight out and not even trace between positions to save speed.
										if (CheckSaberDamage(self, rSaberNum, rBladeNum, saberMidPoint, saberMidEnd, qfalse, trMask, qfalse))
										{
											gotHit = qtrue;
										}
									}
								}
								else
								{
									gotHit = qtrue;
								}
							}
							else
							{
								gotHit = qtrue;
							}

							if (g_saberTraceSaberFirst.integer)
							{
								sN = 0;
								while (sN < MAX_CLIENTS)
								{
									if (clientUnlinked[sN])
									{ //Make clients clip properly again.
										if (g_entities[sN].inuse && g_entities[sN].health > 0)
										{
											g_entities[sN].r.contents |= CONTENTS_BODY;
										}
									}
									sN++;
								}
							}

							if (!gotHit)
							{
								if (trMask != (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT))
								{
									trMask = (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT);
								}
								else
								{//racc - no luck hitting anything.
									gotHit = qtrue; //break out of the loop
								}
							}
						}
					}
					else if (d_saberInterpolate.integer) //anything but 0 or 1, use the old plain method.
					{
						if (!CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qfalse))
						{
							CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qtrue, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qfalse);
						}
					}
				}
				//[SaberSys]
				//Not Used Anymore.  Removing to prevent problems.
				/*
				else if ( d_saberSPStyleDamage.integer )
				{
					G_SPSaberDamageTraceLerped( self, rSaberNum, rBladeNum, boltOrigin, end, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT) );
				}
				*/

				else if(self->client->hasCurrentPosition && d_saberInterpolate.integer == 2)
				{//Super duper interplotation system
					if ( (level.time-self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime) < 100 && BG_SaberInFullDamageMove(&self->client->ps, self->localAnimIndex) )
					{//only do the full swing interpolation while in a true attack swing.
						vec3_t olddir, endpos, startpos;
						float dist = (d_saberBoxTraceSize.value + self->client->saber[rSaberNum].blade[rBladeNum].radius)*0.5f;
						VectorSubtract(self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, self->client->saber[rSaberNum].blade[rBladeNum].trail.base, olddir);
						VectorNormalize(olddir);

						//start off by firing a trace down the old saber position to see if it's still inside something.
						if(CheckSaberDamage(self, rSaberNum, rBladeNum, self->client->saber[rSaberNum].blade[rBladeNum].trail.base, 
							self->client->saber[rSaberNum].blade[rBladeNum].trail.tip, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qtrue) )
						{//saber was still in something at it's previous position, just check damage there.
							
						}
						else
						{//fire a series of traces thru the space the saber moved thru where it moved during the last frame.
							//This is done linearly so it's not going to 100% accurately reflect the normally curved movement
							//of the saber.
							while( dist < self->client->saber[rSaberNum].blade[rBladeNum].lengthMax )
							{
								//set new blade position
								VectorMA( boltOrigin, dist, self->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, endpos );
							
								//set old blade position
								VectorMA( self->client->saber[rSaberNum].blade[rBladeNum].trail.base, dist, olddir, startpos );
								
								if(CheckSaberDamage(self, rSaberNum, rBladeNum, startpos, endpos, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qtrue))
								{//saber hit something, that's good enough for us!
									break;
								}

								//didn't hit anything, slide down the blade a bit and try again.
								dist += d_saberBoxTraceSize.value + self->client->saber[rSaberNum].blade[rBladeNum].radius;
							}
						}
					}
					else
					{//out of date blade position data or not in full damage move, 
						//just do a single ghoul2 trace to keep the CPU useage down.					
						CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qtrue);
					}
				//[/SaberSys]
				}
				else
				{
					CheckSaberDamage(self, rSaberNum, rBladeNum, boltOrigin, end, qfalse, (MASK_PLAYERSOLID|CONTENTS_LIGHTSABER|MASK_SHOT), qfalse);
				}

				VectorCopy(boltOrigin, self->client->saber[rSaberNum].blade[rBladeNum].trail.base);
				VectorCopy(end, self->client->saber[rSaberNum].blade[rBladeNum].trail.tip);
				self->client->saber[rSaberNum].blade[rBladeNum].trail.lastTime = level.time;
				//VectorCopy(boltOrigin, self->client->lastSaberBase);
				//VectorCopy(end, self->client->lastSaberTip);
				self->client->hasCurrentPosition = qtrue;

				//do hit effects
				//[SaberSys]
				//WP_SaberSpecificDoHit is now used instead at the time of damage infliction.
				//WP_SaberDoHit( self, rSaberNum, rBladeNum );
				//[/SaberSys]
				WP_SaberDoClash( self, rSaberNum, rBladeNum );

				rBladeNum++;
			}

			rSaberNum++;
		}
		
		//[SaberSys]
		//Needed to move this into the CheckSaberDamage function for bounce calculations
		//WP_SaberApplyDamage( self );
		//[/SaberSys]

		//NOTE: doing one call like this after the 2 loops above is a bit cheaper, tempentity-wise... but won't use the correct saber and blade numbers...
		//now actually go through and apply all the damage we did
		//WP_SaberDoHit( self, 0, 0 );
		//WP_SaberDoClash( self, 0, 0 );

		if (mySaber && mySaber->inuse)
		{
			trap_LinkEntity(mySaber);
		}

		if (!self->client->ps.saberInFlight)
		{
			self->client->ps.saberEntityState = 0;
		}
	}

finalUpdate:
	//[SaberSys]

	//Update the previous frame viewangle storage.
	VectorCopy(self->client->ps.viewangles, self->client->prevviewangle);
	self->client->prevviewtime = level.time;
	
#ifndef FINAL_BUILD
	if (viewlock != self->client->ps.userInt1 && g_debugviewlock.integer)
	{//view lock changed.  Report
		G_Printf("View/Move Lock changed from %i to %i\n", viewlock, self->client->ps.userInt1);
	}
#endif

	//debounce viewlock
	if(self->client->viewLockTime < level.time)
	{
		self->client->ps.userInt1 = 0;
	}
	//[/SaberSys]

	if (clientOverride)
	{ //if we get the client instance we don't even need to bother setting anims and stuff
		return;
	}

	G_UpdateClientAnims(self, animSpeedScale);
}

int WP_MissileBlockForBlock( int saberBlock )
{
	switch( saberBlock )
	{
	case BLOCKED_UPPER_RIGHT:
		return BLOCKED_UPPER_RIGHT_PROJ;
		break;
	case BLOCKED_UPPER_LEFT:
		return BLOCKED_UPPER_LEFT_PROJ;
		break;
	case BLOCKED_LOWER_RIGHT:
		return BLOCKED_LOWER_RIGHT_PROJ;
		break;
	case BLOCKED_LOWER_LEFT:
		return BLOCKED_LOWER_LEFT_PROJ;
		break;
	case BLOCKED_TOP:
		return BLOCKED_TOP_PROJ;
		break;
	}
	return saberBlock;
}


//[RACC] - Pick the correct saberBlocked block setting based on the hitlocation, your current
// viewheight, and the attack type.  This is also used as a general quadrant finder for other
//moves.
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock )
{
	vec3_t diff, fwdangles={0,0,0}, right;
	vec3_t clEye;
	float rightdot;
	float zdiff;
	qboolean inFront = InFront(hitloc,self->client->ps.origin,self->client->ps.viewangles,-.2f);

	VectorCopy(self->client->ps.origin, clEye);
	clEye[2] += self->client->ps.viewheight;

	VectorSubtract( hitloc, clEye, diff );
	diff[2] = 0;
	VectorNormalize( diff );

	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);
	zdiff = hitloc[2] - clEye[2];
	if(!inFront && self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
	{
		switch(self->client->ps.fd.saberAnimLevel)
		{
			
					case SS_STAFF:
						G_SetAnim(self,&self->client->pers.cmd,SETANIM_BOTH,BOTH_P7_S1_B_, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
						break;
					case SS_DUAL:
						G_SetAnim(self,&self->client->pers.cmd,SETANIM_BOTH,BOTH_P6_S1_B_, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
						break;
					default:
						G_SetAnim(self,&self->client->pers.cmd,SETANIM_BOTH,BOTH_P1_S1_B_, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
						break;
		}
		self->client->ps.saberBlocked = BLOCKED_BACK;
	}
	else if ( zdiff > 0 )
	{
		if ( rightdot > 0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else if ( zdiff > -20 )//20 )
	{
		if ( zdiff < -10 )//30 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			
		}
		if ( rightdot > 0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else
	{
		if ( rightdot >= 0 )
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

	if ( missileBlock )
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
	}

	//[SaberSys]
	//this is a "real" block so don't allow it to be interrupted
	self->client->ps.userInt3 &= ~( 1 << FLAG_PREBLOCK );
	//[/SaberSys]
}

void WP_SaberBlock( gentity_t *playerent, vec3_t hitloc, qboolean missileBlock )
{
	vec3_t diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;

	VectorSubtract(hitloc, playerent->client->ps.origin, diff);
	VectorNormalize(diff);

	fwdangles[1] = playerent->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff) + RandFloat(-0.2f,0.2f);
	zdiff = hitloc[2] - playerent->client->ps.origin[2] + Q_irand(-8,8);
	
	// Figure out what quadrant the block was in.
	if (zdiff > 24)
	{	// Attack from above
		if (Q_irand(0,1))
		{
			playerent->client->ps.saberBlocked = BLOCKED_TOP;
		}
		else
		{
			playerent->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
	}
	else if (zdiff > 13)
	{	// The upper half has three viable blocks...
		if (rightdot > 0.25)
		{	// In the right quadrant...
			if (Q_irand(0,1))
			{
				playerent->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
			}
			else
			{
				playerent->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
			}
		}
		else
		{
			switch(Q_irand(0,3))
			{
			case 0:
				playerent->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
				break;
			case 1:
			case 2:
				playerent->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
				break;
			case 3:
				playerent->client->ps.saberBlocked = BLOCKED_TOP;
				break;
			}
		}
	}
	else
	{	// The lower half is a bit iffy as far as block coverage.  Pick one of the "low" ones at random.
		if (Q_irand(0,1))
		{
			playerent->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			playerent->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

	if ( missileBlock )
	{
		playerent->client->ps.saberBlocked = WP_MissileBlockForBlock( playerent->client->ps.saberBlocked );
	}
}

int WP_SaberCanBlock(gentity_t *self, vec3_t point, int dflags, int mod, qboolean projectile, int attackStr)
{
	qboolean thrownSaber = qfalse;
	float blockFactor = 0;

	if (!self || !self->client || !point)
	{
		return 0;
	}

	if (attackStr == 999)
	{
		attackStr = 0;
		thrownSaber = qtrue;
	}

	if (BG_SaberInAttack(self->client->ps.saberMove))
	{
		return 0;
	}

	if (PM_InSaberAnim(self->client->ps.torsoAnim) && !self->client->ps.saberBlocked &&
		self->client->ps.saberMove != LS_READY && self->client->ps.saberMove != LS_NONE)
	{
		if ( self->client->ps.saberMove < LS_PARRY_UP || self->client->ps.saberMove > LS_REFLECT_LL )
		{
			return 0;
		}
	}

	if (PM_SaberInBrokenParry(self->client->ps.saberMove))
	{
		return 0;
	}

	if (!self->client->ps.saberEntityNum)
	{ //saber is knocked away
		return 0;
	}

	if (BG_SabersOff( &self->client->ps ))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER)
	{
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return 0;
	}

	if (self->client->ps.saberInFlight)
	{
		return 0;
	}

	if ((self->client->pers.cmd.buttons & BUTTON_ATTACK)/* &&
		(projectile || attackStr == FORCE_LEVEL_3)*/)
	{ //don't block when the player is trying to slash, if it's a projectile or he's doing a very strong attack
		return 0;
	}

	//Removed this for now, the new broken parry stuff should handle it. This is how
	//blocks were decided before the 1.03 patch (as you can see, it was STUPID.. for the most part)
	/*
	if (attackStr == FORCE_LEVEL_3)
	{
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (Q_irand(1, 10) < 3)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}

	if (attackStr == FORCE_LEVEL_2 && Q_irand(1, 10) < 3)
	{
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			//do nothing for now
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_2)
		{
			if (Q_irand(1, 10) < 5)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	
	if (attackStr == FORCE_LEVEL_1 && !self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] &&
		Q_irand(1, 40) < 3)
	{ //if I have no defense level at all then I might be unable to block a level 1 attack (but very rarely)
		return 0;
	}
	*/
/*
	if (SaberAttacking(self))
	{ //attacking, can't block now
		return 0;
	}
*/
	if (self->client->ps.saberMove != LS_READY &&
		!self->client->ps.saberBlocking)
	{
		return 0;
	}

	if (self->client->ps.saberBlockTime >= level.time)
	{
		return 0;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && projectile)
	{
		return 0;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_3)
	{
		if (d_saberGhoul2Collision.integer)
		{
			blockFactor = 0.3f;
		}
		else
		{
			blockFactor = 0.05f;
		}
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_2)
	{
		blockFactor = 0.6f;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] == FORCE_LEVEL_1)
	{
		blockFactor = 0.9f;
	}
	else
	{ //for now we just don't get to autoblock with no def
		return 0;
	}

	if (thrownSaber)
	{
		blockFactor -= 0.25f;
	}

	if (attackStr)
	{ //blocking a saber, not a projectile.
		blockFactor -= 0.25f;
	}

	if (!InFront( point, self->client->ps.origin, self->client->ps.viewangles, blockFactor )) //orig 0.2f
	{
		return 0;
	}

	if (projectile && !(self->client->ps.forceHandExtend != HANDEXTEND_NONE))
	{
		WP_SaberBlockNonRandom(self, point, projectile);
	}
	return 1;
}

qboolean HasSetSaberOnly(void)
{
	int i = 0;
	int wDisable = 0;

	if (g_gametype.integer == GT_JEDIMASTER)
	{ //set to 0 
		return qfalse;
	}

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	while (i < WP_NUM_WEAPONS)
	{
		if (!(wDisable & (1 << i)) &&
			i != WP_SABER && i != WP_NONE)
		{
			return qfalse;
		}

		i++;
	}

	int iDisable = 0;
	iDisable = g_itemDisable.integer;
	

	while (i < HI_NUM_HOLDABLE)
	{
		if (!(iDisable & (1 << i)) &&
			i != HI_NONE )
		{
			return qfalse;
		}

		i++;
	}
	
	
	return qtrue;
}

//[CoOp]
//additional kick code used by NPCs to choice kick moves.
extern float G_GroundDistance( gentity_t *self );
saberMoveName_t G_PickAutoKick( gentity_t *self, gentity_t *enemy, qboolean storeMove )
{//find the apprprate non-multikick kickmove to kick this enemy
	vec3_t	v_fwd, v_rt, enemyDir, fwdAngs;
	float fDot, rDot;
	saberMoveName_t kickMove = LS_NONE;
	if ( !self || !self->client )
	{
		return LS_NONE;
	}
	if ( !enemy )
	{
		return LS_NONE;
	}
	VectorSet(fwdAngs, 0, self->client->ps.viewangles[YAW], 0);
	VectorSubtract( enemy->r.currentOrigin, self->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );//not necessary, I guess, but doesn't happen often
	AngleVectors( fwdAngs, v_fwd, v_rt, NULL );
	fDot = DotProduct( enemyDir, v_fwd );
	rDot = DotProduct( enemyDir, v_rt );
	if ( fabs( rDot ) > 0.5f && fabs( fDot ) < 0.5f )
	{//generally to one side
		if ( rDot > 0 )
		{//kick right
			kickMove = LS_KICK_R;
		}
		else
		{//kick left
			kickMove = LS_KICK_L;
		}
	}
	else if ( fabs( fDot ) > 0.5f && fabs( rDot ) < 0.5f )
	{//generally in front or behind us
		if ( fDot > 0 )
		{//kick fwd
			kickMove = LS_KICK_F;
		}
		else
		{//kick back
			kickMove = LS_KICK_B;
		}
	}
	else
	{//diagonal to us, kick would miss
	}
	if ( kickMove != LS_NONE )
	{//have a valid one to do
		if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//if in air, convert kick to an in-air kick

			float gDist = G_GroundDistance( self );
			//let's only allow air kicks if a certain distance from the ground
			//it's silly to be able to do them right as you land.
			//also looks wrong to transition from a non-complete flip anim...
			if ((!BG_FlippingAnim( self->client->ps.legsAnim ) || self->client->ps.legsTimer <= 0) &&
				gDist > 64.0f && //strict minimum
				gDist > (-self->client->ps.velocity[2])-64.0f //make sure we are high to ground relative to downward velocity as well
				)
			{
				switch ( kickMove )
				{
				case LS_KICK_F:
					kickMove = LS_KICK_F_AIR;
					break;
				case LS_KICK_B:
					kickMove = LS_KICK_B_AIR;
					break;
				case LS_KICK_R:
					kickMove = LS_KICK_R_AIR;
					break;
				case LS_KICK_L:
					kickMove = LS_KICK_L_AIR;
					break;
				default: //oh well, can't do any other kick move while in-air
					kickMove = LS_NONE;
					break;
				}
			}
			else
			{//leave it as a normal kick unless we're too high up
				if ( gDist > 128.0f || self->client->ps.velocity[2] >= 0 )
				{ //off ground, but too close to ground
					kickMove = LS_NONE;
				}
			}
		}
		/* not used in MP
		if ( storeMove )
		{
			self->client->ps.saberMoveNext = kickMove;
		}
		*/
	}
	if(kickMove != LS_NONE)
	{//we have a kickmove, do it!
		int kickAnim = saberMoveData[kickMove].animToUse;
		if (kickAnim != -1)
		{
			G_SetAnim(self, NULL, SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
			self->client->ps.weaponTime = self->client->ps.legsTimer;
			self->client->ps.saberMove = kickMove;
		}
	}
	return kickMove;
}

qboolean G_EnemyInKickRange( gentity_t *self, gentity_t *enemy )
{//is the enemy within kick range?
	if ( !self || !enemy )
	{
		return qfalse;
	}
	if ( fabs(self->r.currentOrigin[2]-enemy->r.currentOrigin[2]) < 32 )
	{//generally at same height
		if ( DistanceHorizontal( self->r.currentOrigin, enemy->r.currentOrigin ) <= (STAFF_KICK_RANGE+8.0f+(self->r.maxs[0]*1.5f)+(enemy->r.maxs[0]*1.5f)) )
		{//within kicking range!
			return qtrue;
		}
	}
	return qfalse;
}


extern qboolean BG_InKnockDown( int anim );
qboolean G_CanKickEntity( gentity_t *self, gentity_t *target )
{//can we kick the given target?
	if ( target && target->client
		&& !BG_InKnockDown( target->client->ps.legsAnim )
		&& G_EnemyInKickRange( self, target ) )
	{
		return qtrue;
	}
	return qfalse;
}
//[/CoOp]

//[SaberSys]
qboolean G_BlockIsParry( gentity_t *self, gentity_t *attacker, vec3_t hitLoc )
{//determines if self (who is blocking) is activing blocking (parrying)
	vec3_t pAngles;
	vec3_t pRight;
	vec3_t parrierMove;
	vec3_t hitPos;
	vec3_t hitFlat; //flatten 2D version of the hitPos.
	float blockDot; //the dot product of our desired parry direction
					//vs the actual attack location.
	qboolean inFront = InFront(attacker->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.0f);

	if(!inFront)
	{//can't parry attacks to the rear.
		return qfalse;
	}
	else if(PM_SaberInKnockaway(self->client->ps.saberMove) )
	{//already in parry move, continue parrying anything that hits us as long as 
		//the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if(BG_KickingAnim(self->client->ps.legsAnim))
	{//can't parry in kick.
		return qfalse;
	}

	if(BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex)
		|| PM_SaberInBounce(self->client->ps.saberMove) || BG_InSlowBounce(&self->client->ps))
	{//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if(self->client->ps.pm_flags & PMF_DUCKED)
	{//can't parry while ducked or running
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hitLoc, self->client->ps.origin, hitPos);
	VectorSet(pAngles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors(pAngles, NULL, pRight, NULL);
	hitFlat[0] = 0;
	hitFlat[1] = DotProduct(pRight, hitPos);
	
	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hitFlat[2] = hitPos[2] - 10;
	VectorNormalize(hitFlat);

	//set up the vector for the direction the player is trying to parry in.
	parrierMove[0] = 0;
	parrierMove[1] = (self->client->pers.cmd.rightmove);
	parrierMove[2] = -(self->client->pers.cmd.forwardmove);
	VectorNormalize(parrierMove);

	
	blockDot = DotProduct(hitFlat, parrierMove);

	if(blockDot >= .4)
	{//player successfully blocked in the right direction to do a full parry.
		//G_Printf("%i: %i: parried\n", level.time, self->s.number);
		return qtrue;
	}
	else
	{//player didn't parry in the correct direction, do the minimal parry bonus.
		//SABERSYSRAFIXME - this is a hack, please try to fix this since it's not really
		//fair to players.
		if(self->r.svFlags & SVF_BOT )
		{//bots just randomly parry to make up for them not intelligently parrying.
			if(BOT_PARRYRATE * botstates[self->s.number]->settings.skill > Q_irand(0,999))
			{
				//G_Printf("%i: %i: Bot cheat parried\n", level.time, self->s.number);
				return qtrue;
			}
		}

		return qfalse;
	}
}
//[/SaberSys]
//[SaberSys]
qboolean G_BlockIsQuickParry( gentity_t *self, gentity_t *attacker, vec3_t hitLoc )
{//determines if self (who is blocking) is activing blocking (quickparrying)//JRHockney addition
	vec3_t pAngles;
	vec3_t pRight;
	vec3_t parrierMove;
	vec3_t hitPos;
	vec3_t hitFlat; //flatten 2D version of the hitPos.
	float blockDot; //the dot product of our desired parry direction
					//vs the actual attack location.
	qboolean inFront = InFront(attacker->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.0f);
    
	if(!(self->client->pers.cmd.buttons & BUTTON_15))
	{
		return qfalse;
	}

	if(!inFront)
	{//can't parry attacks to the rear.
		return qfalse;
	}
	else if(PM_SaberInKnockaway(self->client->ps.saberMove) )
	{//already in parry move, continue parrying anything that hits us as long as 
		//the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if(BG_KickingAnim(self->client->ps.legsAnim))
	{//can't parry in kick.
		return qfalse;
	}

	if(BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex)
		|| PM_SaberInBounce(self->client->ps.saberMove) || BG_InSlowBounce(&self->client->ps))
	{//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if(self->client->ps.pm_flags & PMF_DUCKED)
	{//can't parry while ducked or running
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hitLoc, self->client->ps.origin, hitPos);
	VectorSet(pAngles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors(pAngles, NULL, pRight, NULL);
	hitFlat[0] = 0;
	hitFlat[1] = DotProduct(pRight, hitPos);
	
	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hitFlat[2] = hitPos[2] - 10;
	VectorNormalize(hitFlat);

	//set up the vector for the direction the player is trying to parry in.
	parrierMove[0] = 0;
	parrierMove[1] = (self->client->pers.cmd.rightmove);
	parrierMove[2] = -(self->client->pers.cmd.forwardmove);
	VectorNormalize(parrierMove);

	
	blockDot = DotProduct(hitFlat, parrierMove);

	if(blockDot >= .4)
	{//player successfully blocked in the right direction to do a full parry.
		//G_Printf("%i: %i: parried\n", level.time, self->s.number);
		return qtrue;
	}
	else
	{//player didn't parry in the correct direction, do the minimal parry bonus.
		//SABERSYSRAFIXME - this is a hack, please try to fix this since it's not really
		//fair to players.
		//self->client->ps.MISHAP_VARIABLE += 3;
		if(self->r.svFlags & SVF_BOT)
		{//bots just randomly parry to make up for them not intelligently parrying.
			if(BOT_PARRYRATE * botstates[self->s.number]->settings.skill > Q_irand(0,999))
			{
				//G_Printf("%i: %i: Bot cheat parried\n", level.time, self->s.number);
				return qtrue;
			}
		}

		return qfalse;
	}
}
//[/SaberSys]

//[SaberThrowSys]
void SaberBallisticsTouch(gentity_t *saberent, gentity_t *other, trace_t *trace)
{//touch function for sabers in ballistics mode
	gentity_t *saberOwn = &g_entities[saberent->r.ownerNum];

	if (other && other->s.number == saberent->r.ownerNum)
	{//racc - we hit our owner, just ignore it.
		return;
	}

	if(saberent->s.eFlags & EF_MISSILE_STICK)
	{
		G_Printf("this isn't good\n");
	}

	//knock the saber down after impact.
	saberKnockDown(saberent, saberOwn, other);
}


//This is the bounce count used for ballistic sabers.  We watch this value to know
//when to make the switch to a simple dropped saber.
#define BALLISTICSABER_BOUNCECOUNT 10


void SaberBallisticsThink(gentity_t *saberEnt)
{//think function for sabers in ballistics mode
	//G_RunObject(saberEnt);

	saberEnt->nextthink = level.time;

	if(saberEnt->s.eFlags & EF_MISSILE_STICK)
	{//we're stuck in something
		float ownerLen;
		vec3_t dir;
		gentity_t *saberOwner = &g_entities[saberEnt->r.ownerNum];

		//racc - Apprenently pos1 is the hand bolt.
		VectorSubtract(saberOwner->r.currentOrigin, saberEnt->r.currentOrigin, dir);

		ownerLen = VectorLength(dir);

		if (ownerLen <= 32)
		{//racc - picked up the saber.
			G_Sound( saberEnt, CHAN_AUTO, G_SoundIndex( "sound/weapons/saber/saber_catch.wav" ) );

			saberEnt->s.eFlags &= ~EF_MISSILE_STICK;
			saberReactivate(saberEnt, saberOwner);

			saberEnt->speed = 0;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time;

			saberEnt->r.contents = CONTENTS_LIGHTSABER;

			saberOwner->client->ps.saberInFlight = qfalse;
			saberOwner->client->ps.saberEntityState = 0;
			saberOwner->client->ps.saberCanThrow = qfalse;
			saberOwner->client->ps.saberThrowDelay = level.time + 300;

			saberEnt->touch = SaberGotHit;

			saberEnt->think = SaberUpdateSelf;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time + 50;
			WP_SaberRemoveG2Model( saberEnt );

			saberOwner->client->ps.saberEntityNum = saberOwner->client->saberStoredIndex;
			return;
		}
		else if ( saberOwner->client->saberKnockedTime < level.time && 
			((saberOwner->client->buttons & BUTTON_FORCEPOWER && 
			saberOwner->client->ps.fd.forcePowerSelected == FP_SABERTHROW) 
			|| saberOwner->client->buttons & BUTTON_SABERTHROW
			|| saberOwner->client->ps.forceHandExtend == HANDEXTEND_SABERPULL) )
		{//we want to pull the saber back.
			// need an if player in front of in straight line
			// need to set it up so the thing wiggles before freeing
			//ForceThrow(saberOwner, qtrue); //play effect to unstick.
			saberEnt->s.eFlags &= ~EF_MISSILE_STICK;

			saberReactivate(saberEnt, saberOwner);

			saberEnt->touch = SaberGotHit;

			saberEnt->think = saberBackToOwner;
			saberEnt->speed = 0;
			saberEnt->genericValue5 = 0;
			saberEnt->nextthink = level.time;

			saberEnt->r.contents = CONTENTS_LIGHTSABER;
		}
		else if ((level.time - saberOwner->client->saberKnockedTime) > MAX_LEAVE_TIME)
		{//We left it too long.  Just have it turn off and fall to the ground. 
			VectorClear(saberEnt->s.pos.trDelta);
			VectorClear(saberEnt->s.apos.trDelta);
			saberEnt->speed = 0;
			//saberEnt->s.eFlags &= ~EF_MISSILE_STICK;
			saberKnockDown(saberEnt, saberOwner, saberOwner);
		}
	}
	else
	{//flying thru the air
		if(saberEnt->bounceCount != BALLISTICSABER_BOUNCECOUNT)
		{//we've hit an object and bounced off it, go to dead saber mode
			gentity_t *saberOwn = &g_entities[saberEnt->r.ownerNum];
			saberKnockDown(saberEnt, saberOwn, saberOwn);
		}
		//Bug fix: sabers were falling through certain solid objects
		else
			G_RunObject(saberEnt);
	}
}

void thrownSaberBallistics(gentity_t *saberEnt, gentity_t *saberOwn, qboolean stuck)
{//this function converts the saber from thrown saber that's being held on course by
	//the force into a saber that's just ballastically moving.

	if(stuck)
	{//lightsaber is stuck in something, just hang there.
		VectorClear(saberEnt->s.pos.trDelta);
		VectorClear(saberEnt->s.apos.trDelta);

		//set the sticky eflag
		saberEnt->s.eFlags = EF_MISSILE_STICK;

		//don't move at all.
		saberEnt->s.pos.trType = TR_STATIONARY;
		saberEnt->s.apos.trType = TR_STATIONARY;

		//set the force retrieve timer so we'll know when to pull it out 
		//of the object with the force/turn off saber.
		saberOwn->client->saberKnockedTime = level.time + SABER_RETRIEVE_DELAY;

		//no more loop sound!
		saberEnt->s.loopSound = saberOwn->client->saber[0].soundLoop;
		saberEnt->s.loopIsSoundset = qfalse;

		//don't actually bounce on impact with walls.
		saberEnt->bounceCount = 0;
	}
	else
	{//otherwise, just move by normal ballistic physics
		//spin just like we were in our saber throw.
		saberEnt->s.apos.trType = TR_LINEAR;
		saberEnt->s.apos.trDelta[0] = 0;
		saberEnt->s.apos.trDelta[1] = 800;
		saberEnt->s.apos.trDelta[2] = 0;

		//but now gravity has an effect.
		saberEnt->s.pos.trType = TR_GRAVITY;

		//clear the entity flags
		saberEnt->s.eFlags = 0;

		saberEnt->bounceCount = BALLISTICSABER_BOUNCECOUNT;
	}
	
	//set up for saber style bouncing
	saberEnt->flags |= FL_BOUNCE_HALF;


	//set traqec timers and initial positions
	saberEnt->s.apos.trTime = level.time;
	VectorCopy(saberEnt->r.currentAngles, saberEnt->s.apos.trBase);

	saberEnt->s.pos.trTime = level.time;
	VectorCopy(saberEnt->r.currentOrigin, saberEnt->s.pos.trBase);

	//let the player know that they've lost control of the saber.
	saberOwn->client->ps.saberEntityNum = 0;

	//set the approprate function pointer stuff
	saberEnt->think = SaberBallisticsThink;
	saberEnt->touch = SaberBallisticsTouch;
	saberEnt->nextthink = level.time + FRAMETIME;

	trap_LinkEntity(saberEnt);

	//add the saber model to our gentity ghoul2 instance
	WP_SaberAddG2Model( saberEnt, saberOwn->client->saber[0].model, 
							saberOwn->client->saber[0].skin );

	saberEnt->s.modelGhoul2 = 1;
	saberEnt->s.g2radius = 20;

	saberEnt->s.eType = ET_MISSILE;
	saberEnt->s.weapon = WP_SABER;
}
//[/SaberThrowSys]


void DebounceSaberImpact(gentity_t *self, gentity_t *otherSaberer, 
						 int rSaberNum, int rBladeNum, int sabimpactentitynum)
{//this function adds the nessicary debounces for saber impacts so that we can do
	//consistant damamge to players
	/*
	self = player of the checksaberdamage.
	otherSaberer = the other saber owner if we hit their saber as well.
	rSaberNum = the saber number with the impact
	rBladeNum = the blade number on the saber with the impact
	sabimpactentitynum = number of the entity that we hit 
	(if a saber was hit, it's owner is used for this value)
	*/

	//add basic saber impact debounce
	self->client->sabimpact[rSaberNum][rBladeNum].EntityNum = sabimpactentitynum;
	self->client->sabimpact[rSaberNum][rBladeNum].Debounce = level.time;

	if(otherSaberer)
	{//we hit an enemy saber so update our sabimpact data with that info for us and the enemy.
		self->client->sabimpact[rSaberNum][rBladeNum].SaberNum = self->client->lastSaberCollided;
		self->client->sabimpact[rSaberNum][rBladeNum].BladeNum = self->client->lastBladeCollided;

		//Also add this impact to the otherowner so he doesn't do do his behavior rolls twice.
		otherSaberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].EntityNum = self->client->ps.saberEntityNum;
		otherSaberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].Debounce = level.time;
		otherSaberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].SaberNum = rSaberNum;
		otherSaberer->client->sabimpact[self->client->lastSaberCollided][self->client->lastBladeCollided].BladeNum = rBladeNum;
	}
	else
	{//blank out the saber blade impact stuff since we didn't hit another guy's saber
		self->client->sabimpact[rSaberNum][rBladeNum].SaberNum = -1;
		self->client->sabimpact[rSaberNum][rBladeNum].BladeNum = -1;
	}
}


//[SaberSys]
extern qboolean PM_SaberInReturn( int move );
qboolean G_InAttackParry(gentity_t *self)
{//checks to see if a player is doing an attack parry

	if((self->client->pers.cmd.buttons & BUTTON_ATTACK)
		|| (self->client->pers.cmd.buttons & BUTTON_ALT_ATTACK))
	{//can't be pressing an attack button.
		return qfalse;
	}

	if(self->client->ps.userInt3 & (1 << FLAG_PARRIED))
	{//can't attack parry when parried.
		return qfalse;
	}

	if(BG_SaberInTransitionAny(self->client->ps.saberMove))
	{//in transition, start, or return
		return qtrue;
	}

	return qfalse;
}
//[/SaberSys]


//[BugFix26]
qboolean OJP_SaberIsOff( gentity_t *self, int saberNum )
{//this function checks to see if a given saber is off.  This function doesn't check to see if this player actually has
	//that saber.  We only have this function to account for the weird special case for dual sabers where one saber has 
	//been dropped.
	//ENHANCED NOTE:  This function is a bit different in Enhanced because of the retooled saber throw that allows a dual saberer
	//to use their secondary saber while their primary saber has been dropped.

	switch (self->client->ps.saberHolstered)
	{
	case 0:
		//all sabers on
		return qfalse;
		break;

	case 1:
		//one saber off, one saber on, depends on situation.
		if(self->client->ps.fd.saberAnimLevel == SS_DUAL && self->client->ps.saberInFlight && !self->client->ps.saberEntityNum)
		{//special case where the secondary blade is lit instead of the primary
			if(saberNum == 0)
			{//primary is off
				return qtrue;
			}
			else
			{//secondary is on
				return qfalse;
			}
		}
		else
		{//the normal case
			if(saberNum == 0)
			{//primary is on
				return qfalse;
			}
			else
			{//secondary is off
				return qtrue;
			}
		}
		break;
	case 2:
		//all sabers off
		return qtrue;
		break;
	default:
		G_Printf("Unknown saberHolstered value %i in OJP_SaberIsOff\n", self->client->ps.saberHolstered);
		return qtrue;
		break;
	};
}


qboolean OJP_BladeIsOff(gentity_t *self, int saberNum, int bladeNum)
{//checks to see if a given saber blade is supposed to be off.  This function does not check to see if the 
	//saber or saber blade actually exists.

	//We have this function to account for the special cases with dual sabers where one saber has been dropped.

	if(saberNum > 0)
	{//secondary sabers are all on/all off.
		if(OJP_SaberIsOff(self, saberNum))
		{//blades are all off
			return qtrue;
		}
		else
		{//blades are all on
			return qfalse;
		}
	}
	else
	{//primary blade
		//based on number of blades on saber
		if(OJP_SaberIsOff(self, saberNum)) //This function accounts for the weird saber throw situations.
		{//saber is off, all blades are off
			return qtrue;
		}
		else
		{//saber is on, secondary blades status is based on saberHolstered value.
			if(bladeNum > 0 && self->client->ps.saberHolstered == 1)
			{//secondaries are off
				return qtrue;
			}
			else
			{//blade is on.
				return qfalse;
			}
		}
	}
}
//[/BugFix26]


qboolean OJP_UsingDualSaberAsPrimary(playerState_t *ps)
{//indicates that the player is in the very special case of using their dual saber
	//as their primary saber when their primary saber is dropped.
	if(ps->fd.saberAnimLevel == SS_DUAL 
			&& ps->saberInFlight
			&& ps->saberHolstered)
	{
		return qtrue;
	}

	return qfalse;
}

