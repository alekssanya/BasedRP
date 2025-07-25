// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "../ghoul2/g2.h"
#include "q_shared.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/


#define	RESPAWN_ARMOR		20
#define	RESPAWN_TEAM_WEAPON	30
#define	RESPAWN_HEALTH		30
#define	RESPAWN_AMMO		40
#define	RESPAWN_HOLDABLE	60
#define	RESPAWN_MEGAHEALTH	120
#define	RESPAWN_POWERUP		120

// Item Spawn flags
#define ITMSF_SUSPEND		1
#define ITMSF_NOPLAYER		2
#define ITMSF_ALLOWNPC		4
#define ITMSF_NOTSOLID		8
#define ITMSF_VERTICAL		16
#define ITMSF_INVISIBLE		32

extern gentity_t *droppedRedFlag;
extern gentity_t *droppedBlueFlag;

//[MOREFORCEOPTIONS]
qboolean AllForceDisabled(int force);
//[/MOREFORCEOPTIONS]


//======================================================================
#define MAX_MEDPACK_HEAL_AMOUNT		10
#define MAX_SHIELDBOOSTER_REPAIR_AMOUNT		10
#define MAX_SENTRY_DISTANCE			512

// For more than four players, adjust the respawn times, up to 1/4.
int adjustRespawnTime(float preRespawnTime, int itemType, int itemTag)
{
	float respawnTime = preRespawnTime;

	if (itemType == IT_WEAPON)
	{
		if (itemTag == WP_THERMAL ||
			itemTag == WP_TRIP_MINE ||
			itemTag == WP_DET_PACK)
		{ //special case for these, use ammo respawn rate
			respawnTime = RESPAWN_AMMO;
		}
	}

	if (!g_adaptRespawn.integer)
	{
		return((int)respawnTime);
	}

	if (level.numPlayingClients > 4)
	{	// Start scaling the respawn times.
		if (level.numPlayingClients > 32)
		{	// 1/4 time minimum.
			respawnTime *= 0.25;
		}
		else if (level.numPlayingClients > 12)
		{	// From 12-32, scale from 0.5 to 0.25;
			respawnTime *= 20.0 / (float)(level.numPlayingClients + 8);
		}
		else 
		{	// From 4-12, scale from 1.0 to 0.5;
			respawnTime *= 8.0 / (float)(level.numPlayingClients + 4);
		}
	}

	if (respawnTime < 1.0)
	{	// No matter what, don't go lower than 1 second, or the pickups become very noisy!
		respawnTime = 1.0;
	}

	return ((int)respawnTime);
}

//[TicketFix234]
#define SHIELD_HEALTH				2000//Was 250
//[/TicketFix234]
#define MAX_SHIELD_HEIGHT			254
#define MAX_SHIELD_HALFWIDTH		255
#define SHIELD_HALFTHICKNESS		4
#define SHIELD_PLACEDIST			64

#define SHIELD_SIEGE_HEALTH			2000
#define SHIELD_SIEGE_HEALTH_DEC		(SHIELD_SIEGE_HEALTH/25)	// still 25 seconds.

static qhandle_t	shieldLoopSound=0;
static qhandle_t	shieldAttachSound=0;
static qhandle_t	shieldActivateSound=0;
static qhandle_t	shieldDeactivateSound=0;
static qhandle_t	shieldDamageSound=0;


void ShieldRemove(gentity_t *self)
{
	//[Forcefield]
	self->parent->forceFieldThink = level.time + 30000;
	//[/Forcefield]
	self->think = G_FreeEntity;
	self->nextthink = level.time + 100;

	// Play kill sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDeactivateSound);
	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;

	return;
}


// Count down the health of the shield.
void ShieldThink(gentity_t *self)
{
	self->s.trickedentindex = 0;

	self->nextthink = level.time + 1000;
	if (self->health <= 0)
	{
		ShieldRemove(self);
	}
	if ( g_entities[self->s.owner].client->ps.stats[STAT_HEALTH] < 0)
	{
		ShieldRemove(self);
	}		
	return;
}


// The shield was damaged to below zero health.
void ShieldDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	ShieldRemove(self);
}


// The shield had damage done to it.  Make it flicker.
void ShieldPain(gentity_t *self, gentity_t *attacker, int damage)
{
	// Set the itemplaceholder flag to indicate the the shield drawing that the shield pain should be drawn.
	self->think = ShieldThink;
	self->nextthink = level.time + 400;

	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	self->s.trickedentindex = 1;

	return;
}


// Try to turn the shield back on after a delay.
void ShieldGoSolid(gentity_t *self)
{
	trace_t		tr;

	// see if we're valid
	self->health--;
	if (self->health <= 0)
	{
		ShieldRemove(self);
		return;
	}
	
	trap_Trace (&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, self->r.currentOrigin, self->s.number, CONTENTS_BODY );
	if(tr.startsolid)
	{	// gah, we can't activate yet
		self->nextthink = level.time + 200;
		self->think = ShieldGoSolid;
		trap_LinkEntity(self);
	}
	else
	{ // get hard... huh-huh...
		self->s.eFlags &= ~EF_NODRAW;

		self->r.contents = CONTENTS_SOLID;
		self->nextthink = level.time + 1000;
		self->think = ShieldThink;
		self->takedamage = qtrue;
		trap_LinkEntity(self);

		// Play raising sound...
		G_AddEvent(self, EV_GENERAL_SOUND, shieldActivateSound);
		self->s.loopSound = shieldLoopSound;
		self->s.loopIsSoundset = qfalse;
	}

	return;
}


// Turn the shield off to allow a friend to pass through.
void ShieldGoNotSolid(gentity_t *self)
{
	// make the shield non-solid very briefly
	self->r.contents = 0;
	self->s.eFlags |= EF_NODRAW;
	// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
	self->nextthink = level.time + 200;
	self->think = ShieldGoSolid;
	self->takedamage = qfalse;
	trap_LinkEntity(self);

	// Play kill sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDeactivateSound);
	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;
}


//[StunShield]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//[/StunShield]
// Somebody (a player) has touched the shield.  See if it is a "friend".
void ShieldTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	//[StunShield]
	vec3_t tempDir, stunDir;
	//[/StunShield]

	if (g_gametype.integer >= GT_TEAM)
	{ // let teammates through
		// compare the parent's team to the "other's" team
		if (self->parent && ( self->parent->client) && (other->client))
		{
			if (OnSameTeam(self->parent, other))
			{
				ShieldGoNotSolid(self);
				//[StunShield]
				return;
				//[/StunShield]
			}
		}
	}
	else
	{//let the person who dropped the shield through
		if (self->parent && self->parent->s.number == other->s.number)
		{
			ShieldGoNotSolid(self);
			//[StunShield]
			return;
			//[/StunShield]
		}
	}

	//[StunShield]
	if(!other->client)
	{//can't knock over non-clients
		return;
	}

	//player touched the shield, knock them on their ass.
	if(self->s.time2 & (1 << 24))
	{//shield on xaxis
		VectorSet(stunDir, 0, 1, 0);
	}
	else
	{
		VectorSet(stunDir, 1, 0, 0);
	}

	VectorSubtract(self->r.currentOrigin, other->client->ps.origin, tempDir);

	if(DotProduct(tempDir, stunDir) > 0)
	{//stun the player away from the shield
		VectorScale(stunDir, -1, stunDir);
	}

	stunDir[2] = 0.5; //bump the kickback up into the air a bit.
	G_Throw( other, stunDir, 75 );
	G_Knockdown(other, self, stunDir, 300, qtrue);
	//[/StunShield]
}


// After a short delay, create the shield by expanding in all directions.
void CreateShield(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		mins, maxs, end, posTraceEnd, negTraceEnd, start;
	int			height, posWidth, negWidth, halfWidth = 0;
	qboolean	xaxis;
	int			paramData = 0;
//	static int	shieldID;

	// trace upward to find height of shield
	VectorCopy(ent->r.currentOrigin, end);
	end[2] += MAX_SHIELD_HEIGHT;
	trap_Trace (&tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );
	height = (int)(MAX_SHIELD_HEIGHT * tr.fraction);

	// use angles to find the proper axis along which to align the shield
	VectorSet(mins, -SHIELD_HALFTHICKNESS, -SHIELD_HALFTHICKNESS, 0);
	VectorSet(maxs, SHIELD_HALFTHICKNESS, SHIELD_HALFTHICKNESS, height);
	VectorCopy(ent->r.currentOrigin, posTraceEnd);
	VectorCopy(ent->r.currentOrigin, negTraceEnd);

	if ((int)(ent->s.angles[YAW]) == 0) // shield runs along y-axis
	{
		posTraceEnd[1]+=MAX_SHIELD_HALFWIDTH;
		negTraceEnd[1]-=MAX_SHIELD_HALFWIDTH;
		xaxis = qfalse;
	}
	else  // shield runs along x-axis
	{
		posTraceEnd[0]+=MAX_SHIELD_HALFWIDTH;
		negTraceEnd[0]-=MAX_SHIELD_HALFWIDTH;
		xaxis = qtrue;
	}

	// trace horizontally to find extend of shield
	// positive trace
	VectorCopy(ent->r.currentOrigin, start);
	start[2] += (height>>1);
	trap_Trace (&tr, start, 0, 0, posTraceEnd, ent->s.number, MASK_SHOT );
	posWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;
	// negative trace
	trap_Trace (&tr, start, 0, 0, negTraceEnd, ent->s.number, MASK_SHOT );
	negWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;

	// kef -- monkey with dimensions and place origin in center
	halfWidth = (posWidth + negWidth)>>1;
	if (xaxis)
	{
		ent->r.currentOrigin[0] = ent->r.currentOrigin[0] - negWidth + halfWidth;
	}
	else
	{
		ent->r.currentOrigin[1] = ent->r.currentOrigin[1] - negWidth + halfWidth;
	}
	ent->r.currentOrigin[2] += (height>>1);

	// set entity's mins and maxs to new values, make it solid, and link it
	if (xaxis)
	{
		VectorSet(ent->r.mins, -halfWidth, -SHIELD_HALFTHICKNESS, -(height>>1));
		VectorSet(ent->r.maxs, halfWidth, SHIELD_HALFTHICKNESS, height>>1);
	}
	else
	{
		VectorSet(ent->r.mins, -SHIELD_HALFTHICKNESS, -halfWidth, -(height>>1));
		VectorSet(ent->r.maxs, SHIELD_HALFTHICKNESS, halfWidth, height);
	}
	ent->clipmask = MASK_SHOT;

	// Information for shield rendering.

//	xaxis - 1 bit
//	height - 0-254 8 bits
//	posWidth - 0-255 8 bits
//  negWidth - 0 - 255 8 bits

	paramData = (xaxis << 24) | (height << 16) | (posWidth << 8) | (negWidth);
	ent->s.time2 = paramData;

	if ( g_gametype.integer == GT_SIEGE )
	{
		ent->health = ceil((float)(SHIELD_SIEGE_HEALTH*1));
	}
//	else if (ent->client->skillLevel[SK_FORCEFIELD] == FORCE_LEVEL_3)
//	{
//		ent->health = ceil((float)(SHIELD_HEALTH*3));
//	}		
//	else if (ent->client->skillLevel[SK_FORCEFIELD] == FORCE_LEVEL_2)
//	{
//		ent->health = ceil((float)(SHIELD_HEALTH*2));
//	}		
	else
	{
		ent->health = ceil((float)(SHIELD_HEALTH*1));
	}

	ent->s.time = ent->health;//???
	ent->pain = ShieldPain;
	ent->die = ShieldDie;
	ent->touch = ShieldTouch;

	// see if we're valid
	trap_Trace (&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, CONTENTS_BODY ); 

	if (tr.startsolid)
	{	// Something in the way!
		// make the shield non-solid very briefly
		ent->r.contents = 0;
		ent->s.eFlags |= EF_NODRAW;
		// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
		ent->nextthink = level.time + 200;
		ent->think = ShieldGoSolid;
		ent->takedamage = qfalse;
		trap_LinkEntity(ent);
	}
	else
	{	// Get solid.
		ent->r.contents = CONTENTS_PLAYERCLIP|CONTENTS_SHOTCLIP;//CONTENTS_SOLID;

		ent->nextthink = level.time;
		ent->think = ShieldThink;

		ent->takedamage = qtrue;
		trap_LinkEntity(ent);

		// Play raising sound...
		G_AddEvent(ent, EV_GENERAL_SOUND, shieldActivateSound);
		ent->s.loopSound = shieldLoopSound;
		ent->s.loopIsSoundset = qfalse;
	}

	ShieldGoSolid(ent);

	return;
}

qboolean PlaceShield(gentity_t *playerent)
{
	static const gitem_t *shieldItem = NULL;
	gentity_t	*shield = NULL;
	trace_t		tr;
	vec3_t		fwd, pos, dest, mins = {-4,-4, 0}, maxs = {4,4,4};

	if (shieldAttachSound==0)
	{
		shieldLoopSound = G_SoundIndex("sound/movers/doors/forcefield_lp.wav");
		shieldAttachSound = G_SoundIndex("sound/weapons/detpack/stick.wav");
		shieldActivateSound = G_SoundIndex("sound/movers/doors/forcefield_on.wav");
		shieldDeactivateSound = G_SoundIndex("sound/movers/doors/forcefield_off.wav");
		shieldDamageSound = G_SoundIndex("sound/effects/bumpfield.wav");
		shieldItem = BG_FindItemForHoldable(HI_SHIELD);
	}

	// can we place this in front of us?
	AngleVectors (playerent->client->ps.viewangles, fwd, NULL, NULL);
	fwd[2] = 0;
	VectorMA(playerent->client->ps.origin, SHIELD_PLACEDIST, fwd, dest);
	trap_Trace (&tr, playerent->client->ps.origin, mins, maxs, dest, playerent->s.number, MASK_SHOT );
	if (tr.fraction > 0.9)
	{//room in front
		VectorCopy(tr.endpos, pos);
		// drop to floor
		VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
		trap_Trace( &tr, pos, mins, maxs, dest, playerent->s.number, MASK_SOLID );
		if ( !tr.startsolid && !tr.allsolid )
		{
			// got enough room so place the portable shield
			shield = G_Spawn();

			// Figure out what direction the shield is facing.
			if (fabs(fwd[0]) > fabs(fwd[1]))
			{	// shield is north/south, facing east.
				shield->s.angles[YAW] = 0;
			}
			else
			{	// shield is along the east/west axis, facing north
				shield->s.angles[YAW] = 90;
			}
			shield->think = CreateShield;
			shield->nextthink = level.time + 500;	// power up after .5 seconds
			shield->parent = playerent;

			// Set team number.
			shield->s.otherEntityNum2 = playerent->client->sess.sessionTeam;

			shield->s.eType = ET_SPECIAL;
			shield->s.modelindex =  HI_SHIELD;	// this'll be used in CG_Useable() for rendering.
			shield->classname = shieldItem->classname;

			shield->r.contents = CONTENTS_TRIGGER;

			shield->touch = 0;
			// using an item causes it to respawn
			shield->use = 0; //Use_Item;

			// allow to ride movers
			shield->s.groundEntityNum = tr.entityNum;

			G_SetOrigin( shield, tr.endpos );

			shield->s.eFlags &= ~EF_NODRAW;
			shield->r.svFlags &= ~SVF_NOCLIENT;

			trap_LinkEntity (shield);

			shield->s.owner = playerent->s.number;
			shield->s.shouldtarget = qtrue;
			if (g_gametype.integer >= GT_TEAM)
			{
				shield->s.teamowner = playerent->client->sess.sessionTeam;
			}
			else
			{
				shield->s.teamowner = 16;
			}

			// Play placing sound...
			G_AddEvent(shield, EV_GENERAL_SOUND, shieldAttachSound);

			return qtrue;
		}
	}
	// no room
	return qfalse;
}

void ItemUse_Binoculars(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->client->ps.weaponstate != WEAPON_READY)
	{ //So we can't fool it and reactivate while switching to the saber or something.
		return;
	}

	//[Ticket318]
	if (BG_InLedgeMove( ent->client->ps.legsAnim ))
	{//No binocs while hanging!
		return;
	}
	//[/Ticket318]

	/*
	if (ent->client->ps.weapon == WP_SABER)
	{ //No.
		return;
	}
	*/

	if (ent->client->ps.zoomMode == 0) // not zoomed or currently zoomed with the disruptor
	{
		ent->client->ps.zoomMode = 2;
		ent->client->ps.zoomLocked = qfalse;
		ent->client->ps.zoomFov = 40.0f;
	}
	else if (ent->client->ps.zoomMode == 2)
	{
		ent->client->ps.zoomMode = 0;
		ent->client->ps.zoomTime = level.time;
	}
}
void ItemUse_Shield(gentity_t *ent)
{

	PlaceShield(ent);
}

//--------------------------
// PERSONAL ASSAULT SENTRY
//--------------------------

#define PAS_DAMAGE	40

void SentryTouch(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	return;
}
qboolean BG_CrouchAnim( int anim );
qboolean PM_InKnockDown( playerState_t *ps );
//----------------------------------------------------------------
void pas_fire( gentity_t *ent )
//----------------------------------------------------------------
{
	vec3_t fwd, myOrg, enOrg;

	VectorCopy(ent->r.currentOrigin, myOrg);
	myOrg[2] += 24;

	VectorCopy(ent->enemy->client->ps.origin, enOrg);
	enOrg[2] += 24;

	VectorSubtract(enOrg, myOrg, fwd);
	VectorNormalize(fwd);
	
	myOrg[0] += fwd[0]*16;
	myOrg[1] += fwd[1]*16;
	//[Ticket
	if (BG_CrouchAnim(ent->enemy->s.legsAnim))
	{
	myOrg[2] += fwd[2]-10;
	}
	else if (PM_InKnockDown(ent->enemy->playerState ) )
	{

	}
	else
	{
	myOrg[2] += fwd[2]*16;
	}

	WP_FireTurretMissile(&g_entities[ent->genericValue3], myOrg, fwd, qfalse, PAS_DAMAGE, 3000, MOD_SENTRY, ent );
	G_RunObject(ent);

	
}

#define TURRET_RADIUS 800

//-----------------------------------------------------
static qboolean pas_find_enemies( gentity_t *self )
//-----------------------------------------------------
{
	qboolean	found = qfalse;
	int			count, i;
	float		bestDist = TURRET_RADIUS*TURRET_RADIUS;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;
	gentity_t	*entity_list[MAX_GENTITIES], *target;
	trace_t		tr;

	if ( self->aimDebounceTime > level.time ) // time since we've been shut off
	{
		// We were active and alert, i.e. had an enemy in the last 3 secs
		if ( self->painDebounceTime < level.time )
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex( "sound/chars/turret/ping.wav" ));
			self->painDebounceTime = level.time + 1000;
		}
	}

	VectorCopy(self->s.pos.trBase, org2);

	count = G_RadiusList( org2, TURRET_RADIUS, self, qtrue, entity_list );

	for ( i = 0; i < count; i++ )
	{
		target = entity_list[i];

		if ( !target->client )
		{
			continue;
		}
		if ( target == self || !target->takedamage || target->health <= 0 || ( target->flags & FL_NOTARGET ))
		{
			continue;
		}
		if ( self->alliedTeam && target->client->sess.sessionTeam == self->alliedTeam )
		{ 
			continue;
		}
		if (self->genericValue3 == target->s.number)
		{//racc - don't attack owner
			continue;
		}
		if(self->s.NPC_class == CLASS_SEEKER)
			continue;


		//[SentryGun]
		//don't allow sentry gun to attack our own stuff (specifically our seeker item)
		if(target->r.ownerNum == self->genericValue3)
		{//something owned by our owner, don't attack it.
			continue;
		}
		//[/SentryGun]

		if ( !trap_InPVS( org2, target->r.currentOrigin ))
		{
			continue;
		}

		if (target->s.eType == ET_NPC &&
			target->s.NPC_class == CLASS_VEHICLE)
		{ //don't get mad at vehicles, silly.
			continue;
		}

		if ( target->client )
		{
			VectorCopy( target->client->ps.origin, org );
		}
		else
		{
			VectorCopy( target->r.currentOrigin, org );
		}

		trap_Trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT );

		if ( !tr.allsolid && !tr.startsolid && ( tr.fraction == 1.0 || tr.entityNum == target->s.number ))
		{
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < bestDist )// all things equal, keep current
			{
				if ( self->attackDebounceTime + 100 < level.time )
				{
					// We haven't fired or acquired an enemy in the last 2 seconds-start-up sound
					G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/chars/turret/startup.wav" ));

					// Wind up turrets for a bit
					self->attackDebounceTime = level.time + 900 + random() * 200;
				}

				G_SetEnemy( self, target );
				bestDist = enemyDist;
				found = qtrue;
			}
		}
	}

	return found;
}

//---------------------------------
void pas_adjust_enemy( gentity_t *ent )
//---------------------------------
{
	trace_t	tr;
	qboolean keep = qtrue;

	if ( ent->enemy->health <= 0 )
	{
		keep = qfalse;
	}
	else
	{
		vec3_t		org, org2;

		VectorCopy(ent->s.pos.trBase, org2);

		if ( ent->enemy->client )
		{
			VectorCopy( ent->enemy->client->ps.origin, org );
			org[2] -= 15;
		}
		else
		{
			VectorCopy( ent->enemy->r.currentOrigin, org );
		}

		trap_Trace( &tr, org2, NULL, NULL, org, ent->s.number, MASK_SHOT );

		if ( tr.allsolid || tr.startsolid || tr.fraction < 0.9f || tr.entityNum == ent->s.number )
		{
			if (tr.entityNum != ent->enemy->s.number)
			{
				// trace failed
				keep = qfalse;
			}
		}
	}

	if ( keep )
	{
		//ent->bounceCount = level.time + 500 + random() * 150;
	}
	else if ( ent->bounceCount < level.time && ent->enemy ) // don't ping pong on and off
	{
		ent->enemy = NULL;
		// shut-down sound
		G_Sound( ent, CHAN_BODY, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
	
		ent->bounceCount = level.time + 500 + random() * 150;

		// make turret play ping sound for 5 seconds
		ent->aimDebounceTime = level.time + 5000;
	}
}

#define TURRET_DEATH_DELAY 2000
#define TURRET_LIFETIME 60000

void turret_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

void sentryExpire(gentity_t *self)
{
	turret_die(self, self, self, 1000, MOD_UNKNOWN);	
}

//---------------------------------
void pas_think( gentity_t *ent )
//---------------------------------
{//racc - sentry gun think
	qboolean	moved;
	float		diffYaw, diffPitch;
	vec3_t		enemyDir, org;
	vec3_t		frontAngles, backAngles;
	vec3_t		desiredAngles;
	int			iEntityList[MAX_GENTITIES];
	int			numListedEntities;
	int			i = 0;
	qboolean	clTrapped = qfalse;
	vec3_t		testMins, testMaxs;

	testMins[0] = ent->r.currentOrigin[0] + ent->r.mins[0]+4;
	testMins[1] = ent->r.currentOrigin[1] + ent->r.mins[1]+4;
	testMins[2] = ent->r.currentOrigin[2] + ent->r.mins[2]+4;

	testMaxs[0] = ent->r.currentOrigin[0] + ent->r.maxs[0]-4;
	testMaxs[1] = ent->r.currentOrigin[1] + ent->r.maxs[1]-4;
	testMaxs[2] = ent->r.currentOrigin[2] + ent->r.maxs[2]-4;

	numListedEntities = trap_EntitiesInBox( testMins, testMaxs, iEntityList, MAX_GENTITIES );

	while (i < numListedEntities)
	{
		if (iEntityList[i] < MAX_CLIENTS)
		{ //client stuck inside me. go nonsolid.
			int clNum = iEntityList[i];

			numListedEntities = trap_EntitiesInBox( g_entities[clNum].r.absmin, g_entities[clNum].r.absmax, iEntityList, MAX_GENTITIES );

			i = 0;
			while (i < numListedEntities)
			{
				if (iEntityList[i] == ent->s.number)
				{
					clTrapped = qtrue;
					break;
				}
				i++;
			}
			break;
		}

		i++;
	}

	if (clTrapped)
	{
		ent->r.contents = 0;
		ent->s.fireflag = 0;
		ent->nextthink = level.time + FRAMETIME;
		return;
	}
	else
	{
		ent->r.contents = CONTENTS_SOLID;
	}


	
	if (!g_entities[ent->genericValue3].inuse || !g_entities[ent->genericValue3].client || //racc - owner is in bad state
		g_entities[ent->genericValue3].client->sess.sessionTeam != ent->genericValue2) //racc - owner isn't on the same team as we remember
	{//racc - delete self
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}
	if ( g_entities[ent->s.owner].client->ps.stats[STAT_HEALTH] < 0)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/turret/shutdown.wav"));
		ent->s.bolt2 = ENTITYNUM_NONE;
		ent->s.fireflag = 2;

		ent->think = sentryExpire;
		ent->nextthink = level.time + TURRET_DEATH_DELAY;
		return;
	}
//	G_RunObject(ent);

	if ( !ent->damage )
	{
		ent->damage = 1;
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	//[SentryGun]
	//don't self-destruct from age
/*	
	if ((ent->genericValue8+TURRET_LIFETIME) < level.time)
	{
		G_Sound( ent, CHAN_BODY, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
		ent->s.bolt2 = ENTITYNUM_NONE;
		ent->s.fireflag = 2;

		ent->think = sentryExpire;
		ent->nextthink = level.time + TURRET_DEATH_DELAY;
		return;
	}
*/	
	//[/SentryGun]

	ent->nextthink = level.time + FRAMETIME;

	if ( ent->enemy )
	{
		// make sure that the enemy is still valid
		pas_adjust_enemy( ent );
	}

	if (ent->enemy)
	{
		if (!ent->enemy->client)
		{
			ent->enemy = NULL;
		}
		else if (ent->enemy->s.number == ent->s.number)
		{
			ent->enemy = NULL;
		}
		else if (ent->enemy->health < 1)
		{
			ent->enemy = NULL;
		}
	}

	if ( !ent->enemy )
	{
		pas_find_enemies( ent );
	}

	if (ent->enemy)
	{
		ent->s.bolt2 = ent->enemy->s.number;
	}
	else
	{
		ent->s.bolt2 = ENTITYNUM_NONE;
	}

	moved = qfalse;
	diffYaw = 0.0f; diffPitch = 0.0f;

	ent->speed = AngleNormalize360( ent->speed );
	ent->random = AngleNormalize360( ent->random );

	if ( ent->enemy )
	{
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		if ( ent->enemy->client )
		{
			VectorCopy( ent->enemy->client->ps.origin, org );
		}
		else
		{
			VectorCopy( ent->enemy->r.currentOrigin, org );
		}

		VectorSubtract( org, ent->r.currentOrigin, enemyDir );
		vectoangles( enemyDir, desiredAngles );

		diffYaw = AngleSubtract( ent->speed, desiredAngles[YAW] );
		diffPitch = AngleSubtract( ent->random, desiredAngles[PITCH] );
	}
	else
	{
		// no enemy, so make us slowly sweep back and forth as if searching for a new one
		diffYaw = sin( level.time * 0.0001f + ent->count ) * 2.0f;
	}

	if ( fabs(diffYaw) > 0.25f )
	{
		moved = qtrue;

		if ( fabs(diffYaw) > 10.0f )
		{
			// cap max speed
			ent->speed += (diffYaw > 0.0f) ? -10.0f : 10.0f;
		}
		else
		{
			// small enough
			ent->speed -= diffYaw;
		}
	}


	if ( fabs(diffPitch) > 0.25f )
	{
		moved = qtrue;

		if ( fabs(diffPitch) > 4.0f )
		{
			// cap max speed
			ent->random += (diffPitch > 0.0f) ? -4.0f : 4.0f;
		}
		else
		{
			// small enough
			ent->random -= diffPitch;
		}
	}

	// the bone axes are messed up, so hence some dumbness here
	VectorSet( frontAngles, -ent->random, 0.0f, 0.0f );
	VectorSet( backAngles, 0.0f, 0.0f, ent->speed );

	if ( moved )
	{
	//ent->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );
	}
	else
	{
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	}

	if ( ent->enemy && ent->attackDebounceTime < level.time )
	{
		ent->count--;

		if ( ent->count )
		{
			pas_fire( ent );
			ent->s.fireflag = 1;
			ent->attackDebounceTime = level.time + 200;
		}
		else
		{
			//ent->nextthink = 0;
			G_Sound( ent, CHAN_BODY, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
			ent->s.bolt2 = ENTITYNUM_NONE;
			ent->s.fireflag = 2;

			ent->think = sentryExpire;
			ent->nextthink = level.time + TURRET_DEATH_DELAY;
		}
	}
	else
	{
		ent->s.fireflag = 0;
	}
}

//------------------------------------------------------------------------------------------------------------
void turret_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
//------------------------------------------------------------------------------------------------------------
{
	gentity_t *owner = &g_entities[self->genericValue3];
	// Turn off the thinking of the base & use it's targets
	self->think = 0;//NULL;
	self->use = 0;//NULL;

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	if (!g_entities[self->genericValue3].inuse || !g_entities[self->genericValue3].client)
	{
		G_FreeEntity(self);
		return;
	}

	// clear my data
	self->die  = 0;//NULL;
	self->takedamage = qfalse;
	self->health = 0;

	// hack the effect angle so that explode death can orient the effect properly
	VectorSet( self->s.angles, 0, 0, 1 );

	G_PlayEffect(EFFECT_EXPLOSION_PAS, self->s.pos.trBase, self->s.angles);
	G_RadiusDamage(self->s.pos.trBase, &g_entities[self->genericValue3], 30, 256, self, self, MOD_UNKNOWN);

	//[SentryGun]
	//not used anymore since we allow multiple sentry guns now.
	//g_entities[self->genericValue3].client->ps.fd.sentryDeployed = qfalse;
	//[/SentryGun]

	//ExplodeDeath( self );
	G_FreeEntity( self );

	//[SentryGun]
	if (owner->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_0)
	{
	owner->sentryDeadThink = level.time + 60000;
	}
	//[/SentryGun]

}

#define TURRET_AMMO_COUNT 200

//---------------------------------
void SP_PAS( gentity_t *base )
//---------------------------------
{
	//racc - give ammo to the unit
	if ( base->count == 0 )
	{
		// give ammo
		//[SentryGun]
		//racc - these things carry as much ammo as players do.
		base->count = ammoData[AMMO_BLASTER].max;
		//base->count = TURRET_AMMO_COUNT;
		//[/SentryGun]
	}

	base->s.bolt1 = 1; //This is a sort of hack to indicate that this model needs special turret things done to it
	base->s.bolt2 = ENTITYNUM_NONE; //store our current enemy index

	base->damage = 0; // start animation flag

	VectorSet( base->r.mins, -8, -8, 0 );
	VectorSet( base->r.maxs, 8, 8, 24 );

	G_RunObject(base);

	base->think = pas_think;
	base->nextthink = level.time + FRAMETIME;

	if ( !base->health )
	{
		base->health = 50;
	}

	base->takedamage = qtrue;
	base->die  = turret_die;

	base->physicsObject = qtrue;

	G_Sound( base, CHAN_BODY, G_SoundIndex( "sound/chars/turret/startup.wav" ));
}

//------------------------------------------------------------------------

void ItemUse_Sentry(gentity_t *ent)
{
	ent->client->isHacking = -100;
	ent->client->ps.hackingTime = level.time+150;
	ent->client->ps.hackingBaseTime = level.time+100;
}

void ItemUse_Sentry2( gentity_t *ent )
//------------------------------------------------------------------------
{
	vec3_t fwd, fwdorg;
	vec3_t yawonly;
	vec3_t mins, maxs;
	gentity_t *sentry;

	if (!ent || !ent->client)
	{
		return;
	}

	VectorSet( mins, -8, -8, 0 );
	VectorSet( maxs, 8, 8, 24 );


	yawonly[ROLL] = 0;
	yawonly[PITCH] = 0;
	yawonly[YAW] = ent->client->ps.viewangles[YAW];

	AngleVectors(yawonly, fwd, NULL, NULL);

	fwdorg[0] = ent->client->ps.origin[0] + fwd[0]*64;
	fwdorg[1] = ent->client->ps.origin[1] + fwd[1]*64;
	fwdorg[2] = ent->client->ps.origin[2] + fwd[2]*64;

	sentry = G_Spawn();

	sentry->classname = "sentryGun";
	sentry->s.modelindex = G_ModelIndex("models/items/psgun.glm"); //replace ASAP

	sentry->s.g2radius = 30.0f;
	sentry->s.modelGhoul2 = 1;

	G_SetOrigin(sentry, fwdorg);
	sentry->parent = ent;
	sentry->r.contents = CONTENTS_SOLID;
	sentry->s.solid = 2;
	sentry->clipmask = MASK_SOLID;
	VectorCopy(mins, sentry->r.mins);
	VectorCopy(maxs, sentry->r.maxs);
	sentry->genericValue3 = ent->s.number;
	sentry->genericValue2 = ent->client->sess.sessionTeam; //so we can remove ourself if our owner changes teams
	sentry->r.absmin[0] = sentry->s.pos.trBase[0] + sentry->r.mins[0];
	sentry->r.absmin[1] = sentry->s.pos.trBase[1] + sentry->r.mins[1];
	sentry->r.absmin[2] = sentry->s.pos.trBase[2] + sentry->r.mins[2];
	sentry->r.absmax[0] = sentry->s.pos.trBase[0] + sentry->r.maxs[0];
	sentry->r.absmax[1] = sentry->s.pos.trBase[1] + sentry->r.maxs[1];
	sentry->r.absmax[2] = sentry->s.pos.trBase[2] + sentry->r.maxs[2];
	sentry->s.eType = ET_GENERAL;
	sentry->s.pos.trType = TR_GRAVITY;//STATIONARY;
	sentry->s.pos.trTime = level.time;
	sentry->touch = SentryTouch;
	sentry->nextthink = level.time;
	sentry->genericValue4 = ENTITYNUM_NONE; //genericValue4 used as enemy index

	sentry->genericValue5 = 1000;

	sentry->genericValue8 = level.time;

	sentry->alliedTeam = ent->client->sess.sessionTeam;

	//[SentryGun]
	//not used anymore since we allow multiple sentry guns now.
	//ent->client->ps.fd.sentryDeployed = qtrue;
	//[/SentryGun]

	trap_LinkEntity(sentry);

	sentry->parent->sentryDeadThink = 0;

	sentry->s.owner = ent->s.number;
	sentry->s.shouldtarget = qtrue;
	if (g_gametype.integer >= GT_TEAM)
	{
		sentry->s.teamowner = ent->client->sess.sessionTeam;
	}
	else
	{
		sentry->s.teamowner = 16;
	}

	SP_PAS( sentry );
}

extern gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle );

//[SeekerItemNpc]
void NPC_SetMoveGoal( gentity_t *ent, vec3_t point, int radius, qboolean isNavGoal, int combatPoint, gentity_t *targetEnt );
gentity_t *ViewTarget(gentity_t *ent, int length, vec3_t *target, cplane_t *plane);
qboolean NPC_MoveToGoal( qboolean tryStraight );
//[/SeekerItemNpc]

void ItemUse_Seeker(gentity_t *ent)
{
	//[SeekerItemNpc]

	gentity_t *remote = ent->client->remote;

	if(!remote || !remote->inuse || !remote->client || (remote->activator != ent && !remote->originalactivator))
	{//actualy spawn a remote NPC

		remote = NPC_SpawnType( ent, "seeker", va("player%iseeker", ent->s.number), qfalse );
		if ( remote && remote->client )
		{//set it to my team
			remote->s.owner = remote->r.ownerNum = ent->s.number;
			remote->originalactivator = ent;
			//remote->NPC->goalEntity is being cleared after we spawn
			remote->NPC->goalEntity = remote->client->leader = ent;
			//remote->NPC->behaviorState = BS_FOLLOW_LEADER;
			//remote->NPC->followDist = 200;
			ent->client->remote = remote;

			//seeker's ammo count is set in NPC_SetMiscDefaultData();

			//TODO: set 'remote->health' according to player skill
			//remember, demp2 pwns seekers

			//TODO: set these based on player skill
			remote->genericValue1 = 200; //minimum time between shots
			remote->genericValue2 = 200; //maximum time between shots
			if(ent->client->skillLevel[SK_SEEKER]== FORCE_LEVEL_3)
				{
			remote->genericValue1 = 100; //minimum time between shots
			remote->genericValue2 = 100; //maximum time between shots 
				}
			else if(ent->client->skillLevel[SK_SEEKER]== FORCE_LEVEL_2)
				{
			remote->genericValue1 = 150; //minimum time between shots
			remote->genericValue2 = 150; //maximum time between shots  
				}
			//TODO: set this based on player skill
			//[SeekerNerf]
				remote->damage = 40; //damage per shot 

			//remote->damage = 97; //damage per shot (does damage at bryer level)
			//[/SeekerNerf]

			//TODO: should beeping on seeing enemy change based on skill?
			//disabling this for now, but the code to beep every 900 ms while attacking an enemy is still active, 
			//set remote->genericValue3 to the sound index if you still want it
			//remote->genericValue3 = G_SoundIndex( "sound/chars/turret/ping.wav" );

			//IDEA: instead of beeping when we are attacking, lets beep when there is an enemy within a radar range!
			remote->genericValue4 = G_SoundIndex( "sound/chars/turret/ping.wav" );
			//time between beeps will scale to how far away the enemy is
			//STOPPING POINT: this isnt beeping... fix!  and the radius seems to be needing to be uberlarge even when the enemies
			//					are close...
			remote->radius = -20000; //if this is POSITIVE, it beeps only if the enemy is within the sight range.
								  //if this is NEGITIVE, it has a 'radar' and beeps for enemies not in sight range.
								  //FIXME: should this beep for players with cloaking?  currently it wont.
			

			//TODO: if our player dies, the seeker explodes.  Should it wander around looking for players still?
			//if it should explode, then reactivate and set up the code to have the seeker fall down and hit the ground and explode
			//instead of exploding in the air... OR: let it sit there as a seeker item?  conserve its ammo to use the same amount for
			//the next person who uses it?

			
			//dont chance enemies unless we were specificly told to
			remote->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
			if ( g_gametype.integer == GT_SIEGE )
			{
				if ( ent->client->sess.sessionTeam == TEAM_BLUE )
				{
					remote->client->playerTeam = NPCTEAM_PLAYER;
					remote->client->enemyTeam = NPCTEAM_ENEMY;
				}
				else if ( ent->client->sess.sessionTeam == TEAM_RED )
				{
					remote->client->playerTeam = NPCTEAM_ENEMY;
					remote->client->enemyTeam = NPCTEAM_PLAYER;
				}
				//else
				//{
				//	remote->client->playerTeam = NPCTEAM_NEUTRAL;
				//}
			}
			//this stops it moving and it just sits there, I need to check why
			//else
			//{
			//	remote->client->playerTeam = NPCTEAM_NEUTRAL;
			//}
		}	
	}
	else
	{
		//TODO: have commandable seekers be based on skill level?
		if(remote->NPC->goalEntity == ent /*&& remote->NPC->behaviorState == BS_FOLLOW_LEADER*/)
		{
			vec3_t targVec;
			gentity_t *targ;
			cplane_t plane; //for getting aim offset so seeker doesnt try to enter the walls (in which case it will just stay put)
			int range = 1024; //TODO: set this based on skill level
			targ = ViewTarget(ent, range, &targVec, &plane); //racc - find the entity of whatever we're looking at.
			if(targ)
			{//racc - found viewed entity.
				if(targ->client && targ->client->playerTeam != ent->client->playerTeam){
					//remote->NPC->behaviorState = BS_HUNT_AND_KILL;
					remote->NPC->goalEntity = remote->enemy = targ;
					remote->NPC->scriptFlags |= SCF_CHASE_ENEMIES; //we are allowed to hunt down our target
				}
				else{
					//remote->NPC->behaviorState = BS_FOLLOW_LEADER;
					remote->NPC->goalEntity = remote->client->leader = ent;
				}
			}
			else if(!VectorCompare(targVec, ent->r.currentOrigin)){
				//remote->NPC->behaviorState = BS_INVESTIGATE;
				VectorMA(targVec, 15, plane.normal, targVec);
				NPC_SetMoveGoal(remote, targVec, 50, qfalse, -1, NULL);
				remote->NPC->scriptFlags |= SCF_CHASE_ENEMIES; //we are allowed to hunt down enemies
			}
		}
		else{
			remote->NPC->goalEntity = remote->client->leader = ent;
			remote->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;

			//remote->NPC->behaviorState = BS_FOLLOW_LEADER;
		}	
	}

	/*
	if ( g_gametype.integer == GT_SIEGE && d_siegeSeekerNPC.integer )
	{//actualy spawn a remote NPC
		gentity_t *remote = NPC_SpawnType( ent, "remote", NULL, qfalse );
		if ( remote && remote->client )
		{//set it to my team
			remote->s.owner = remote->r.ownerNum = ent->s.number;
			remote->activator = ent;
			if ( ent->client->sess.sessionTeam == TEAM_BLUE )
			{
				remote->client->playerTeam = NPCTEAM_PLAYER;
			}
			else if ( ent->client->sess.sessionTeam == TEAM_RED )
			{
				remote->client->playerTeam = NPCTEAM_ENEMY;
			}
			else
			{
				remote->client->playerTeam = NPCTEAM_NEUTRAL;
			}
		}	
	}
	else
	{
		ent->client->ps.eFlags |= EF_SEEKERDRONE;
		ent->client->ps.droneExistTime = level.time + 30000;
		ent->client->ps.droneFireTime = level.time + 1500;
	}
	*/
	//[/SeekerItemNpc]
}

static void MedPackGive(gentity_t *ent, int amount)
{
	if (!ent || !ent->client)
	{
		return;
	}
	//[DuelSys]
	// MJN - No Bacta in duels!
	if ( ent->client->ps.duelInProgress )
	{
		return;
	}
	//[/DuelSys]

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD))
	{
		return;
	}

	if (ent->health >= ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		return;
	}

	ent->health += amount;

	if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
	}
}



void ItemUse_MedPack(gentity_t *ent)
{

	if (ent->client->skillLevel[SK_BACTA] == FORCE_LEVEL_3)
	{
		MedPackGive(ent, 5.0*MAX_MEDPACK_HEAL_AMOUNT);
	}	
	else if (ent->client->skillLevel[SK_BACTA] == FORCE_LEVEL_2)
	{
		MedPackGive(ent, 2.5*MAX_MEDPACK_HEAL_AMOUNT);
	}	
	else 
	{
		MedPackGive(ent, MAX_MEDPACK_HEAL_AMOUNT);
	}

}


static void ShieldBoosterGive(gentity_t *ent, int amount)
{
	if (!ent || !ent->client)
	{
		return;
	}
	//[DuelSys]
	// MJN - No Bacta in duels!
	if ( ent->client->ps.duelInProgress )
	{
		return;
	}
	//[/DuelSys]

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD))
	{
		return;
	}

	if (ent->client->ps.stats[STAT_ARMOR] >= ent->client->ps.stats[STAT_MAX_ARMOR])
	{
		return;
	}

	ent->client->ps.stats[STAT_ARMOR] += amount;

	if (ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_ARMOR])
	{
		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_ARMOR];
	}

}



void ItemUse_ShieldBooster(gentity_t *ent)
{

	if (ent->client->skillLevel[SK_REPAIR] == FORCE_LEVEL_3)
	{
		ShieldBoosterGive(ent, 5.0*MAX_SHIELDBOOSTER_REPAIR_AMOUNT);
	}	
	else if (ent->client->skillLevel[SK_REPAIR] == FORCE_LEVEL_2)
	{
		ShieldBoosterGive(ent, 2.5*MAX_SHIELDBOOSTER_REPAIR_AMOUNT);
	}	
	else 
	{
		ShieldBoosterGive(ent, MAX_SHIELDBOOSTER_REPAIR_AMOUNT);
	}

}

#define JETPACK_TOGGLE_TIME			1000
void Jetpack_Off(gentity_t *ent)
{ //create effects?
	assert(ent && ent->client);
	
	if(!ent || !ent->client)
		return;

	if (!ent->client->jetPackOn)
	{ //aready off
		return;
	}

	//[DualPistols]
	if(!PM_InKnockDown(&ent->client->ps))
	{
		if(ent->client->ps.eFlags & EF_DUAL_WEAPONS)
		{
				
					if (ent->client->ps.eFlags & EF_WP_OPTION_2)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim4[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim4[ent->client->ps.weapon];
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_3)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim6[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim6[ent->client->ps.weapon];
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_4)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim8[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim8[ent->client->ps.weapon];
					}
					else
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim2[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim2[ent->client->ps.weapon];
					}
				

		}
		else
		{
					if (ent->client->ps.eFlags & EF_WP_OPTION_2)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim3[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim3[ent->client->ps.weapon];
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_3)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim5[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim5[ent->client->ps.weapon];
					}
					else if (ent->client->ps.eFlags & EF_WP_OPTION_4)
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim7[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim7[ent->client->ps.weapon];
					}
					else
					{
			ent->client->ps.torsoAnim=WeaponReadyAnim[ent->client->ps.weapon];
			ent->client->ps.legsAnim=WeaponReadyAnim[ent->client->ps.weapon];
					}
		}
	}
	//[/DualPistols]
	
	ent->client->jetPackOn = qfalse;
}

void Jetpack_On(gentity_t *ent)
{ //create effects?
	assert(ent && ent->client);

	if (ent->client->jetPackOn)
	{ //aready on
		return;
	}

	if (ent->client->ps.fd.forceGripBeingGripped >= level.time)
	{ //can't turn on during grip interval
		return;
	}

	if (ent->client->ps.fallingToDeath)
	{ //too late!
		return;
	}

	G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/boba/JETON"));

	ent->client->jetPackOn = qtrue;
}

extern void Jedi_Decloak( gentity_t *self );
//[FlameThrower]
#define FLAMETHROWER_RADIUS 512
void Flamethrower_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;
	int BURN_TIME = 2500;
	float	radius = FLAMETHROWER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;
	qboolean saberBlocked = qfalse;
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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

		if(traceEnt->client)
		{
			vec3_t pushDir;
			VectorSubtract( traceEnt->client->ps.origin, self->client->ps.origin, pushDir );
			VectorNormalize(pushDir);
			VectorScale( pushDir, 150, traceEnt->client->ps.velocity );
			//VectorCopy(pushDir,traceEnt->client->ps.velocity);
		

			if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}

			
				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}			

	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN );

					//[ForceSys]
					//lightning also blasts the target back.


					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if (traceEnt->client->burnTime < (level.time + BURN_TIME/2) && damage)
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						gentity_t	*tent;
						tent = G_TempEntity(traceEnt->r.currentOrigin, EV_BURNED);
						tent->s.eventParm = DirToByte(dir);
						tent->s.owner = traceEnt->s.number;
						traceEnt->client->burnTime = level.time + BURN_TIME;
					}

				}
		}

	}
}


void Dioxisthrower_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;
	int	TOXIC_TIME = 2500;
	float	radius = FLAMETHROWER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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

		if(traceEnt->client)
		{

		if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}

				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}


	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage(traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK |/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN);

					//[ForceSys]
					//lightning also blasts the target back.


					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if (traceEnt->client->toxicTime < (level.time + TOXIC_TIME/2) &&  damage)
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
					traceEnt->client->toxicTime = level.time + TOXIC_TIME;
					G_EntitySound(traceEnt, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

					traceEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
					traceEnt->client->ps.forceHandExtendTime = level.time + TOXIC_TIME/3;
					}		
	
		
				}
		}

		

	}
}

void Icethrower_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;
	int	FREEZE_TIME = 2500;
	float	radius = FLAMETHROWER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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


		if(traceEnt->client)
		{
		if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}

				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}


	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage(traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK |/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN);

					//[ForceSys]
					//lightning also blasts the target back.


					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if (traceEnt->client->freezeTime < (level.time + FREEZE_TIME/2) &&  damage)
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
					gentity_t	*tent;
					tent = G_TempEntity(traceEnt->r.currentOrigin, EV_FROZEN);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;
					traceEnt->client->freezeTime = level.time + FREEZE_TIME;
					traceEnt->client->ps.userInt1 |= LOCK_MOVERIGHT;
					traceEnt->client->ps.userInt1 |= LOCK_MOVELEFT;
					traceEnt->client->ps.userInt1 |= LOCK_MOVEFORWARD;
					traceEnt->client->ps.userInt1 |= LOCK_MOVEBACK;
					traceEnt->client->ps.userInt1 |= LOCK_MOVEUP;
					traceEnt->client->ps.userInt1 |= LOCK_MOVEDOWN;
					traceEnt->client->ps.userInt1 |= LOCK_UP;
					traceEnt->client->ps.userInt1 |= LOCK_DOWN;
					traceEnt->client->ps.userInt1 |= LOCK_RIGHT;
					traceEnt->client->ps.userInt1 |= LOCK_LEFT;		
					traceEnt->client->viewLockTime = level.time + FREEZE_TIME/3;
					traceEnt->client->ps.legsTimer = traceEnt->client->ps.torsoTimer = level.time + FREEZE_TIME/3;
					traceEnt->client->ps.saberMove = LS_READY;//don't finish whatever saber anim you may have been in
					traceEnt->client->ps.saberBlocked = BLOCKED_NONE;
					if (traceEnt->client->ps.eFlags & EF_WP_OPTION_2)
					{
					G_SetAnim(traceEnt, NULL, SETANIM_BOTH, WeaponReadyAnim3[traceEnt->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, FREEZE_TIME/3);
					}
					else if (traceEnt->client->ps.eFlags & EF_WP_OPTION_3)
					{
					G_SetAnim(traceEnt, NULL, SETANIM_BOTH, WeaponReadyAnim5[traceEnt->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, FREEZE_TIME/3);
					}
					else if (traceEnt->client->ps.eFlags & EF_WP_OPTION_4)
					{
					G_SetAnim(traceEnt, NULL, SETANIM_BOTH, WeaponReadyAnim7[traceEnt->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, FREEZE_TIME/3);
					}
					else
					{
					G_SetAnim(traceEnt, NULL, SETANIM_BOTH, WeaponReadyAnim[traceEnt->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, FREEZE_TIME/3);
					}	

					}

		
				}

		}
		

	}
}




void ItemUse_FlameThrower(gentity_t *ent)
{

	if (ent->client->ps.jetpackFuel < FLAMETHROWER_FUELCOST)
		return;

	if(BG_InLedgeMove(ent->client->ps.legsAnim))
	{//can't use flamethrower while in ledgegrab
		return;
	}
	
	if(ent->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_1)
	{
	ent->client->flameTime = level.time + 300;		
	}
	else if(ent->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_2)
	{
	ent->client->dioxisTime = level.time + 300;		
	}	
	else if(ent->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_3)
	{
	ent->client->iceTime = level.time + 300;		
	}	
	else
	{
	//VectorCopy(ang,ent->client->ps.velocity);
	ent->client->flameTime = level.time + 300;
	}
}
//[/Flamethrower]


//[Electroshocker]
#define ELECTROSHOCKER_RADIUS 512
void Electroshocker_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;
	int SHOCK_TIME = 2500;
	float	radius = ELECTROSHOCKER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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

		if(traceEnt->client)
		{

		if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}

				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}

	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN );

					//[ForceSys]
					//lightning also blasts the target back.


					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					//don't do the electrical effect unless we didn't block with the saber.

					
					if (traceEnt->client->shockTime < (level.time + SHOCK_TIME/2) && damage)
					//if (traceEnt->client->ps.electrifyTime < (level.time + 400))
					//[/ForceSys]
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						gentity_t	*tent;
						tent = G_TempEntity(traceEnt->r.currentOrigin, EV_SHOCKED);
						tent->s.eventParm = DirToByte(dir);
						tent->s.owner = traceEnt->s.number;
						traceEnt->client->shockTime = level.time + SHOCK_TIME;
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

void Lasersupport_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;

	float	radius = ELECTROSHOCKER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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

		if(traceEnt->client)
		{

		if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}


				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}

	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN );

					//[ForceSys]
					//lightning also blasts the target back.



					//[/ForceSys]
				}
				if(damage)
				{
				G_Throw(traceEnt, dir, 100);					
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
					if ( traceEnt->client->ps.stats[STAT_HEALTH]+ traceEnt->client->ps.stats[STAT_ARMOR]-damage < 1 )
					{//electrocution effect
						traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					}						
		
				}
		}
	}
}


extern qboolean /*GAME_INLINE*/ WalkCheck( gentity_t * self );
extern qboolean PM_SaberInBrokenParry( int move );

void Orbitalstrike_Fire( gentity_t *self )
{
	trace_t	tr;
	vec3_t	forward;
	gentity_t	*traceEnt;
	vec3_t	center, mins, maxs, dir, ent_org, size, v;

	float	radius = ELECTROSHOCKER_RADIUS, dot, dist;
	int damage = 1;
	gentity_t	*entityList[MAX_GENTITIES];
	int			iEntityList[MAX_GENTITIES];
	int		e, numListedEntities, i;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

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

		if(traceEnt->client)
		{

		if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_1)
					{
						damage = 1;
						}			
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_2)
					{
						damage = 3;
						}
		else if (self->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3)
					{
			if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					damage = 7;
				}
			else
				{
					damage =5;
				}
		}

				if (traceEnt->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)  )
				{
					if (traceEnt->client->ps.userInt3 & (1 << FLAG_PROTECT2))
					{
						
					}
					else
					{
					damage = 0;						
					}
				}
				if ( traceEnt->client->ps.powerups[PW_SPHERESHIELDED] )
				{
					damage = 0;						
				}

	//			if (modPowerLevel != -1)
				{

	//					dmg = 0;


				}
				//[ForceSys]
	//			saberBlocked = OJP_BlockEnergy(self, traceEnt, impactPoint, dmg);

				if (damage //&& !saberBlocked
				)
				//if (dmg)
				//[/ForceSys]
				{
					//rww - Shields can now absorb lightning too.
					G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_UNKNOWN );

					//[ForceSys]
					//lightning also blasts the target back.
				if(((!WalkCheck(traceEnt) 
					|| (WalkCheck(traceEnt) && traceEnt->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY) 
					|| BG_IsUsingHeavyWeap(&traceEnt->client->ps)
					|| PM_SaberInBrokenParry(traceEnt->client->ps.saberMove)
					|| traceEnt->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)) && damage)
					{
						G_Knockdown(traceEnt, self, dir, 300, qtrue);
					}

					//[/ForceSys]
				}
				if ( traceEnt->client )
				{
					if ( !Q_irand( 0, 2 ) )
					{
	//					G_Sound( traceEnt, CHAN_BODY, G_SoundIndex( va("sound/weapons/force/lightninghit%i", Q_irand(1, 3) )) );
					}

	// 					if ( traceEnt->client->ps.weapon == WP_SABER )
					{//Serenitysabersystems saber can block lightning
	//					int rSaberNum = 0;
	//					int rBladeNum = 0;						
	//					traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
	//					if ( saberBlocked
	//						&& traceEnt->client
	//						&& !traceEnt->client->ps.saberHolstered
	//						&& !traceEnt->client->ps.saberInFlight )
						{
	//						vec3_t	end2;
	//						vec3_t ang = { 0, 0, 0};
	//						ang[0] = flrand(0,360);
	//						ang[1] = flrand(0,360);
	//						ang[2] = flrand(0,360);
	//						VectorMA( traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax*flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2 );
	//						G_PlayEffectID( G_EffectIndex( "saber/saber_friction.efx"),end2, ang );
						}
					}
					//[ForceSys]
					//don't do the electrical effect unless we didn't block with the saber.
			
		
				}
		}
	}
}


void ItemUse_Electroshocker(gentity_t *ent)
{

	if (ent->client->ps.cloakFuel < ELECTROSHOCKER_FUELCOST)
		return;

	if(BG_InLedgeMove(ent->client->ps.legsAnim))
	{//can't use electroshocker while in ledgegrab
		return;
	}
	
	if(ent->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_1)
	{
	ent->client->electroshockerTime = level.time + 300;
	}
	else if(ent->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_2)
	{
	ent->client->lasersupportTime = level.time + 300;
	}
	else if(ent->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_3)
	{
	ent->client->orbitalstrikeTime = level.time + 300;
	}
	else
	{
	ent->client->electroshockerTime = level.time + 300;
	}
	//VectorCopy(ang,ent->client->ps.velocity);

}
//[Electroshocker]	
void ItemUse_Jetpack( gentity_t *ent )
{
	assert(ent && ent->client);

	if (ent->client->jetPackToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->jetPackOn &&
		ent->client->ps.jetpackFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if (ent->client->jetPackOn)
	{
		Jetpack_Off(ent);
	}
	else
	{
		Jetpack_On(ent);
	}

	ent->client->jetPackToggleTime = level.time + JETPACK_TOGGLE_TIME;
}

#define CLOAK_TOGGLE_TIME			1000
#define SPHERESHIELD_TOGGLE_TIME			1000		
#define OVERLOAD_TOGGLE_TIME			1000							   
extern void Jedi_Cloak( gentity_t *self );
extern void Jedi_Decloak( gentity_t *self );
void ItemUse_UseCloak( gentity_t *ent )
{
	assert(ent && ent->client);

	if (ent->client->cloakToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.powerups[PW_CLOAKED] &&
		ent->client->ps.cloakFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if ( ent->client->ps.powerups[PW_CLOAKED] )
	{//decloak
		Jedi_Decloak( ent );
	}
	else
	{//cloak
		Jedi_Cloak( ent );
	}

	ent->client->cloakToggleTime = level.time + CLOAK_TOGGLE_TIME;
}
extern void Sphereshield_On( gentity_t *self );
extern void Sphereshield_Off( gentity_t *self );
void ItemUse_UseSphereshield( gentity_t *ent )
{
	assert(ent && ent->client);

	if (ent->client->sphereshieldToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.powerups[PW_SPHERESHIELDED] &&
		ent->client->ps.cloakFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if ( ent->client->ps.powerups[PW_SPHERESHIELDED] )
	{//decloak
		Sphereshield_Off( ent );
	}
	else
	{//cloak
		Sphereshield_On( ent );
	}

	ent->client->sphereshieldToggleTime = level.time + SPHERESHIELD_TOGGLE_TIME;
}

extern void Overload_On( gentity_t *self );
extern void Overload_Off( gentity_t *self );
void ItemUse_UseOverload( gentity_t *ent )
{
	assert(ent && ent->client);

	if (ent->client->overloadToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.powerups[PW_OVERLOADED] &&
		ent->client->ps.cloakFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if ( ent->client->ps.powerups[PW_OVERLOADED] )
	{//decloak
		Overload_Off( ent );
	}
	else
	{//cloak
		Overload_On( ent );
	}

	ent->client->overloadToggleTime = level.time + OVERLOAD_TOGGLE_TIME;
}

void ItemUse_SquadTeam(gentity_t *ent)
{

	if (ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_3)
	{
gentity_t *SquadTeam3 = ent->client->SquadTeam3;	

	if(!SquadTeam3 || !SquadTeam3->inuse || !SquadTeam3->client || (!SquadTeam3->originalactivator))
	{
		if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_1)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam3", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_2)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squadreb", va("player%iSquadTeam3", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_3)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squadmerc", va("player%iSquadTeam3", ent->s.number), qfalse );

			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_1)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squadcis", va("player%iSquadTeam3", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_2)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squadrep", va("player%iSquadTeam3", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_3)
			{
		SquadTeam3 = NPC_SpawnType( ent, "squadman", va("player%iSquadTeam3", ent->s.number), qfalse );
			}
		else
			{
		SquadTeam3 = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam3", ent->s.number), qfalse );
			}
		if ( SquadTeam3 && SquadTeam3->client )
		{//set it to my team
			SquadTeam3->s.owner = SquadTeam3->r.ownerNum = ent->s.number;	
			SquadTeam3->originalactivator = ent;
			//SquadTeam3->NPC->goalEntity is being cleared after we spawn
			SquadTeam3->NPC->goalEntity = SquadTeam3->client->leader = ent;
			SquadTeam3->NPC->behaviorState = BS_FOLLOW_LEADER;
			SquadTeam3->NPC->followDist = 50;
			SquadTeam3->NPC->distToGoal = 50;
			ent->client->SquadTeam3 = SquadTeam3;


		
	

			//seeker's ammo count is set in NPC_SetMiscDefaultData();

			//TODO: set 'SquadTeam3->health' according to player skill
			//remember, demp2 pwns seekers

			//TODO: set these based on player skill


			//TODO: set this based on player skill
			//[SeekerNerf]

			
			//SquadTeam3->damage = 97; //damage per shot (does damage at bryer level)
			//[/SeekerNerf]

			//TODO: should beeping on seeing enemy change based on skill?
			//disabling this for now, but the code to beep every 900 ms while attacking an enemy is still active, 
			//set SquadTeam3->genericValue3 to the sound index if you still want it
			//SquadTeam3->genericValue3 = G_SoundIndex( "sound/chars/turret/ping.wav" );

			//IDEA: instead of beeping when we are attacking, lets beep when there is an enemy within a radar range!
			//time between beeps will scale to how far away the enemy is
			//STOPPING POINT: this isnt beeping... fix!  and the radius seems to be needing to be uberlarge even when the enemies
			//					are close...
			
								  //if this is NEGITIVE, it has a 'radar' and beeps for enemies not in sight range.
								  //FIXME: should this beep for players with cloaking?  currently it wont.
			

			//TODO: if our player dies, the seeker explodes.  Should it wander around looking for players still?
			//if it should explode, then reactivate and set up the code to have the seeker fall down and hit the ground and explode
			//instead of exploding in the air... OR: let it sit there as a seeker item?  conserve its ammo to use the same amount for
			//the next person who uses it?

			
			//dont chance enemies unless we were specificly told to
			SquadTeam3->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;

			{
				if ( ent->client->sess.sessionTeam == TEAM_BLUE )
				{
					SquadTeam3->client->playerTeam = NPCTEAM_PLAYER;
					SquadTeam3->client->enemyTeam = NPCTEAM_ENEMY;
				}
				else if ( ent->client->sess.sessionTeam == TEAM_RED )
				{
					SquadTeam3->client->playerTeam = NPCTEAM_ENEMY;
					SquadTeam3->client->enemyTeam = NPCTEAM_PLAYER;
				}
				//else
				//{
				//	SquadTeam3->client->playerTeam = NPCTEAM_NEUTRAL;
				//}
			}
			//this stops it moving and it just sits there, I need to check why
			//else
			//{
			//	SquadTeam3->client->playerTeam = NPCTEAM_NEUTRAL;
			//}
		}	
	}
	else
	{
		//TODO: have commandable seekers be based on skill level?
		if(SquadTeam3->NPC->goalEntity == ent /*&& SquadTeam3->NPC->behaviorState == BS_FOLLOW_LEADER*/)
		{
			vec3_t targVec;
			gentity_t *targ;
			cplane_t plane; //for getting aim offset so seeker doesnt try to enter the walls (in which case it will just stay put)
			int range = 1024; //TODO: set this based on skill level
			targ = ViewTarget(ent, range, &targVec, &plane); //racc - find the entity of whatever we're looking at.
			if(targ)
			{//racc - found viewed entity.
				if(targ->client && targ->client->playerTeam != ent->client->playerTeam){
					SquadTeam3->NPC->goalEntity = SquadTeam3->enemy = targ;
					SquadTeam3->NPC->behaviorState = BS_HUNT_AND_KILL;
					SquadTeam3->NPC->scriptFlags |= SCF_CHASE_ENEMIES; //we are allowed to hunt down our target
				}
				else{
					SquadTeam3->NPC->goalEntity = SquadTeam3->client->leader = ent;
					SquadTeam3->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam3->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;


				}
			}

		}
		else{
					SquadTeam3->NPC->goalEntity = SquadTeam3->client->leader = ent;
					SquadTeam3->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam3->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
		}	
	}

	/*
	if ( g_gametype.integer == GT_SIEGE && d_siegeSeekerNPC.integer )
	{//actualy spawn a SquadTeam3 NPC
		gentity_t *SquadTeam3 = NPC_SpawnType( ent, "SquadTeam3", NULL, qfalse );
		if ( SquadTeam3 && SquadTeam3->client )
		{//set it to my team
			SquadTeam3->s.owner = SquadTeam3->r.ownerNum = ent->s.number;
			SquadTeam3->activator = ent;
			if ( ent->client->sess.sessionTeam == TEAM_BLUE )
			{
				SquadTeam3->client->playerTeam = NPCTEAM_PLAYER;
			}
			else if ( ent->client->sess.sessionTeam == TEAM_RED )
			{
				SquadTeam3->client->playerTeam = NPCTEAM_ENEMY;
			}
			else
			{
				SquadTeam3->client->playerTeam = NPCTEAM_NEUTRAL;
			}
		}	
	}
	else
	{
		ent->client->ps.eFlags |= EF_SEEKERDRONE;
		ent->client->ps.droneExistTime = level.time + 30000;
		ent->client->ps.droneFireTime = level.time + 1500;
	}
	*/
	//[/SeekerItemNpc]


		
	}
	if(ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_2)	
	{
		gentity_t *SquadTeam2 = ent->client->SquadTeam2;	

	if(!SquadTeam2 || !SquadTeam2->inuse || !SquadTeam2->client || ( !SquadTeam2->originalactivator))
	{
		if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_1)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam2", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_2)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squadreb", va("player%iSquadTeam2", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_3)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squadmerc", va("player%iSquadTeam2", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_1)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squadcis", va("player%iSquadTeam2", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_2)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squadrep", va("player%iSquadTeam2", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_3)
			{
		SquadTeam2 = NPC_SpawnType( ent, "squadman", va("player%iSquadTeam2", ent->s.number), qfalse );
			}
		else
			{
		SquadTeam2 = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam2", ent->s.number), qfalse );
			}
		if ( SquadTeam2 && SquadTeam2->client )
		{//set it to my team
			SquadTeam2->s.owner = SquadTeam2->r.ownerNum = ent->s.number;
			SquadTeam2->originalactivator = ent;
			//SquadTeam2->NPC->goalEntity is being cleared after we spawn
			SquadTeam2->NPC->goalEntity = SquadTeam2->client->leader = ent;
			SquadTeam2->NPC->behaviorState = BS_FOLLOW_LEADER;
			SquadTeam2->NPC->followDist = 50;
			SquadTeam2->NPC->distToGoal = 50;
			ent->client->SquadTeam2 = SquadTeam2;

			//seeker's ammo count is set in NPC_SetMiscDefaultData();

			//TODO: set 'SquadTeam2->health' according to player skill
			//remember, demp2 pwns seekers

			//TODO: set these based on player skill


			//TODO: set this based on player skill
			//[SeekerNerf]

			
			//SquadTeam2->damage = 97; //damage per shot (does damage at bryer level)
			//[/SeekerNerf]

			//TODO: should beeping on seeing enemy change based on skill?
			//disabling this for now, but the code to beep every 900 ms while attacking an enemy is still active, 
			//set SquadTeam2->genericValue3 to the sound index if you still want it
			//SquadTeam2->genericValue3 = G_SoundIndex( "sound/chars/turret/ping.wav" );

			//IDEA: instead of beeping when we are attacking, lets beep when there is an enemy within a radar range!
			//time between beeps will scale to how far away the enemy is
			//STOPPING POINT: this isnt beeping... fix!  and the radius seems to be needing to be uberlarge even when the enemies
			//					are close...
			
								  //if this is NEGITIVE, it has a 'radar' and beeps for enemies not in sight range.
								  //FIXME: should this beep for players with cloaking?  currently it wont.
			

			//TODO: if our player dies, the seeker explodes.  Should it wander around looking for players still?
			//if it should explode, then reactivate and set up the code to have the seeker fall down and hit the ground and explode
			//instead of exploding in the air... OR: let it sit there as a seeker item?  conserve its ammo to use the same amount for
			//the next person who uses it?

			
			//dont chance enemies unless we were specificly told to
			SquadTeam2->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
	
			{
				if ( ent->client->sess.sessionTeam == TEAM_BLUE )
				{
					SquadTeam2->client->playerTeam = NPCTEAM_PLAYER;
					SquadTeam2->client->enemyTeam = NPCTEAM_ENEMY;
				}
				else if ( ent->client->sess.sessionTeam == TEAM_RED )
				{
					SquadTeam2->client->playerTeam = NPCTEAM_ENEMY;
					SquadTeam2->client->enemyTeam = NPCTEAM_PLAYER;
				}
				//else
				//{
				//	SquadTeam2->client->playerTeam = NPCTEAM_NEUTRAL;
				//}
			}
			//this stops it moving and it just sits there, I need to check why
			//else
			//{
			//	SquadTeam2->client->playerTeam = NPCTEAM_NEUTRAL;
			//}
		}	
	}
	else
	{
		//TODO: have commandable seekers be based on skill level?
		if(SquadTeam2->NPC->goalEntity == ent /*&& SquadTeam2->NPC->behaviorState == BS_FOLLOW_LEADER*/)
		{
			vec3_t targVec;
			gentity_t *targ;
			cplane_t plane; //for getting aim offset so seeker doesnt try to enter the walls (in which case it will just stay put)
			int range = 1024; //TODO: set this based on skill level
			targ = ViewTarget(ent, range, &targVec, &plane); //racc - find the entity of whatever we're looking at.
			if(targ)
			{//racc - found viewed entity.
				if(targ->client && targ->client->playerTeam != ent->client->playerTeam){
					SquadTeam2->NPC->goalEntity = SquadTeam2->enemy = targ;
					SquadTeam2->NPC->behaviorState = BS_HUNT_AND_KILL;
					SquadTeam2->NPC->scriptFlags |= SCF_CHASE_ENEMIES; //we are allowed to hunt down our target
				}
				else{
					SquadTeam2->NPC->goalEntity = SquadTeam2->client->leader = ent;
					SquadTeam2->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam2->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
				}
			}

		}
		else{
					SquadTeam2->NPC->goalEntity = SquadTeam2->client->leader = ent;
					SquadTeam2->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam2->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
		}	
	}

	/*
	if ( g_gametype.integer == GT_SIEGE && d_siegeSeekerNPC.integer )
	{//actualy spawn a SquadTeam2 NPC
		gentity_t *SquadTeam2 = NPC_SpawnType( ent, "SquadTeam2", NULL, qfalse );
		if ( SquadTeam2 && SquadTeam2->client )
		{//set it to my team
			SquadTeam2->s.owner = SquadTeam2->r.ownerNum = ent->s.number;
			SquadTeam2->activator = ent;
			if ( ent->client->sess.sessionTeam == TEAM_BLUE )
			{
				SquadTeam2->client->playerTeam = NPCTEAM_PLAYER;
			}
			else if ( ent->client->sess.sessionTeam == TEAM_RED )
			{
				SquadTeam2->client->playerTeam = NPCTEAM_ENEMY;
			}
			else
			{
				SquadTeam2->client->playerTeam = NPCTEAM_NEUTRAL;
			}
		}	
	}
	else
	{
		ent->client->ps.eFlags |= EF_SEEKERDRONE;
		ent->client->ps.droneExistTime = level.time + 30000;
		ent->client->ps.droneFireTime = level.time + 1500;
	}
	*/
	//[/SeekerItemNpc]



	}
	gentity_t *SquadTeam = ent->client->SquadTeam;	

	if(!SquadTeam || !SquadTeam->inuse || !SquadTeam->client || (!SquadTeam->originalactivator))
	{
		if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_1)
			{
		SquadTeam = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_2)
			{
		SquadTeam = NPC_SpawnType( ent, "squadreb", va("player%iSquadTeam", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_3)
			{
		SquadTeam = NPC_SpawnType( ent, "squadmerc", va("player%iSquadTeam", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_1)
			{
		SquadTeam = NPC_SpawnType( ent, "squadcis", va("player%iSquadTeam", ent->s.number), qfalse );
			}
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_2)
			{
		SquadTeam = NPC_SpawnType( ent, "squadrep", va("player%iSquadTeam", ent->s.number), qfalse );
			}	
		else if(ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_3)
			{
		SquadTeam = NPC_SpawnType( ent, "squadman", va("player%iSquadTeam", ent->s.number), qfalse );
			}
		else
			{
		SquadTeam = NPC_SpawnType( ent, "squademp", va("player%iSquadTeam", ent->s.number), qfalse );
			}
		if ( SquadTeam && SquadTeam->client )
		{//set it to my team
			SquadTeam->s.owner = SquadTeam->r.ownerNum = ent->s.number;	
			SquadTeam->originalactivator = ent;
			//SquadTeam->NPC->goalEntity is being cleared after we spawn
			SquadTeam->NPC->goalEntity = SquadTeam->client->leader = ent;
			SquadTeam->NPC->behaviorState = BS_FOLLOW_LEADER;
			SquadTeam->NPC->followDist = 50;
			SquadTeam->NPC->distToGoal = 50;
			ent->client->SquadTeam = SquadTeam;

			//seeker's ammo count is set in NPC_SetMiscDefaultData();

			//TODO: set 'SquadTeam->health' according to player skill
			//remember, demp2 pwns seekers

			//TODO: set these based on player skill


			//TODO: set this based on player skill
			//[SeekerNerf]

			
			//SquadTeam->damage = 97; //damage per shot (does damage at bryer level)
			//[/SeekerNerf]

			//TODO: should beeping on seeing enemy change based on skill?
			//disabling this for now, but the code to beep every 900 ms while attacking an enemy is still active, 
			//set SquadTeam->genericValue3 to the sound index if you still want it
			//SquadTeam->genericValue3 = G_SoundIndex( "sound/chars/turret/ping.wav" );

			//IDEA: instead of beeping when we are attacking, lets beep when there is an enemy within a radar range!
			//time between beeps will scale to how far away the enemy is
			//STOPPING POINT: this isnt beeping... fix!  and the radius seems to be needing to be uberlarge even when the enemies
			//					are close...
			
								  //if this is NEGITIVE, it has a 'radar' and beeps for enemies not in sight range.
								  //FIXME: should this beep for players with cloaking?  currently it wont.
			

			//TODO: if our player dies, the seeker explodes.  Should it wander around looking for players still?
			//if it should explode, then reactivate and set up the code to have the seeker fall down and hit the ground and explode
			//instead of exploding in the air... OR: let it sit there as a seeker item?  conserve its ammo to use the same amount for
			//the next person who uses it?

			
			//dont chance enemies unless we were specificly told to
			SquadTeam->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;

			{
				if ( ent->client->sess.sessionTeam == TEAM_BLUE )
				{
					SquadTeam->client->playerTeam = NPCTEAM_PLAYER;
					SquadTeam->client->enemyTeam = NPCTEAM_ENEMY;
				}
				else if ( ent->client->sess.sessionTeam == TEAM_RED )
				{
					SquadTeam->client->playerTeam = NPCTEAM_ENEMY;
					SquadTeam->client->enemyTeam = NPCTEAM_PLAYER;
				}
				//else
				//{
				//	SquadTeam->client->playerTeam = NPCTEAM_NEUTRAL;
				//}
			}
			//this stops it moving and it just sits there, I need to check why
			//else
			//{
			//	SquadTeam->client->playerTeam = NPCTEAM_NEUTRAL;
			//}
		}	
	}
	else
	{
		//TODO: have commandable seekers be based on skill level?
		if(SquadTeam->NPC->goalEntity == ent /*&& SquadTeam->NPC->behaviorState == BS_FOLLOW_LEADER*/)
		{
			vec3_t targVec;
			gentity_t *targ;
			cplane_t plane; //for getting aim offset so seeker doesnt try to enter the walls (in which case it will just stay put)
			int range = 1024; //TODO: set this based on skill level
			targ = ViewTarget(ent, range, &targVec, &plane); //racc - find the entity of whatever we're looking at.
			if(targ)
			{//racc - found viewed entity.
				if(targ->client && targ->client->playerTeam != ent->client->playerTeam){
					SquadTeam->NPC->goalEntity = SquadTeam->enemy = targ;
					SquadTeam->NPC->behaviorState = BS_HUNT_AND_KILL;
					SquadTeam->NPC->scriptFlags |= SCF_CHASE_ENEMIES; //we are allowed to hunt down our target
				}
				else{
					SquadTeam->NPC->goalEntity = SquadTeam->client->leader = ent;
					SquadTeam->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
				}
			}

		}
		else{
					SquadTeam->NPC->goalEntity = SquadTeam->client->leader = ent;
					SquadTeam->NPC->behaviorState = BS_FOLLOW_LEADER;
					SquadTeam->NPC->scriptFlags &= ~SCF_CHASE_ENEMIES;
		}	
	}
	
	/*
	if ( g_gametype.integer == GT_SIEGE && d_siegeSeekerNPC.integer )
	{//actualy spawn a SquadTeam NPC
		gentity_t *SquadTeam = NPC_SpawnType( ent, "SquadTeam", NULL, qfalse );
		if ( SquadTeam && SquadTeam->client )
		{//set it to my team
			SquadTeam->s.owner = SquadTeam->r.ownerNum = ent->s.number;
			SquadTeam->activator = ent;
			if ( ent->client->sess.sessionTeam == TEAM_BLUE )
			{
				SquadTeam->client->playerTeam = NPCTEAM_PLAYER;
			}
			else if ( ent->client->sess.sessionTeam == TEAM_RED )
			{
				SquadTeam->client->playerTeam = NPCTEAM_ENEMY;
			}
			else
			{
				SquadTeam->client->playerTeam = NPCTEAM_NEUTRAL;
			}
		}	
	}
	else
	{
		ent->client->ps.eFlags |= EF_SEEKERDRONE;
		ent->client->ps.droneExistTime = level.time + 30000;
		ent->client->ps.droneFireTime = level.time + 1500;
	}
	*/
	//[/SeekerItemNpc]



}

void ItemUse_VehicleMount(gentity_t *ent)
{
							  
	gentity_t *VehicleMount=ent->client->VehicleMount;	

	if(!VehicleMount || !VehicleMount->inuse || !VehicleMount->client || VehicleMount->activator != ent)
	{
	if(ent->client->skillLevel[SK_LIGHTVEHICLEA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "speederbike", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LIGHTVEHICLEA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "tauntaun", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LIGHTVEHICLEA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "swoop_mp", va("player%iVehicleMount", ent->s.number), qtrue );
		}	
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "atst_vehicle", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "lukespeeder", va("player%iVehicleMount", ent->s.number), qtrue );
		}		
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "rancor_vehicle", va("player%iVehicleMount", ent->s.number), qtrue );
		}		
		
		
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "atat", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "snow-speeder-tfp", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "skiff", va("player%iVehicleMount", ent->s.number), qtrue );
		}
		
	else if(ent->client->skillLevel[SK_FIGHTERSHIPA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "tie-fighter", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_FIGHTERSHIPA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "x-wing", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_FIGHTERSHIPA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "tie-interceptor", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "tie-bomber", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "y-wing", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "a-wing", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "lambdashuttle", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "yt-1300", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "ravensclawvm", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LIGHTVEHICLEB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "stap_mst", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LIGHTVEHICLEB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "swoop", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LIGHTVEHICLEB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "eopie", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "aat_hat", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "atpt_vehicle", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_MEDIUMVEHICLEB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "bantha", va("player%iVehicleMount", ent->s.number), qtrue );
		}		
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "hailfire_droid", va("player%iVehicleMount", ent->s.number), qtrue );
		}	
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "atte", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_HEAVYVEHICLEB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "n1", va("player%iVehicleMount", ent->s.number), qtrue );
		}





	else if(ent->client->skillLevel[SK_FIGHTERSHIPB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "droidfighter", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_FIGHTERSHIPB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "arc-170", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_FIGHTERSHIPB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "droidtf", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "droid_starfighter", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "v-wing", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_BOMBERSHIPB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "JediStarFighter", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPB] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "tfshuttle", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPB] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "gunshipx", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_TRANSPORTSHIPB] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "slave1_jango", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LASERTURRETA] == FORCE_LEVEL_1)
		{
		VehicleMount = NPC_SpawnType( ent, "turbolaser_tower", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LASERTURRETA] == FORCE_LEVEL_2)
		{
		VehicleMount = NPC_SpawnType( ent, "laserturret1", va("player%iVehicleMount", ent->s.number), qtrue );
		}
	else if(ent->client->skillLevel[SK_LASERTURRETA] == FORCE_LEVEL_3)
		{
		VehicleMount = NPC_SpawnType( ent, "laserturret2", va("player%iVehicleMount", ent->s.number), qtrue );
		}		
	else
		{
		VehicleMount = NPC_SpawnType( ent, "swoop_mp", va("player%iVehicleMount", ent->s.number), qtrue );
		}
		if ( VehicleMount && VehicleMount->client )
		{	
			VehicleMount->NPC->scriptFlags|=SCF_IGNORE_ENEMIES;
			//VehicleMount->genericValue4 = G_SoundIndex( "sound/chars/droideka/foldout.mp3" );
		}

	}
}


#define TOSSED_ITEM_STAY_PERIOD			20000
#define TOSSED_ITEM_OWNER_NOTOUCH_DUR	1000

void SpecialItemThink(gentity_t *ent)
{
	float gravity = 3.0f;
	float mass = 0.09f;
	float bounce = 1.1f;

	if (ent->genericValue5 < level.time)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	G_RunExPhys(ent, gravity, mass, bounce, qfalse, NULL, 0);
	VectorCopy(ent->r.currentOrigin, ent->s.origin);
	ent->nextthink = level.time + 50;
}

void G_SpecialSpawnItem(gentity_t *ent, gitem_t *item)
{
	RegisterItem( item );
	ent->item = item;

	//go away if no one wants me
	ent->genericValue5 = level.time + TOSSED_ITEM_STAY_PERIOD;
	ent->think = SpecialItemThink;
	ent->nextthink = level.time + 50;
	ent->clipmask = MASK_SOLID;

	ent->physicsBounce = 0.50;		// items are bouncy
	VectorSet (ent->r.mins, -8, -8, -0);
	VectorSet (ent->r.maxs, 8, 8, 16);

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;

	//can't touch owner for x seconds
	ent->genericValue11 = ent->r.ownerNum;
	ent->genericValue10 = level.time + TOSSED_ITEM_OWNER_NOTOUCH_DUR;

	//so we know to remove when picked up, not respawn
	ent->genericValue9 = 1;

	//kind of a lame value to use, but oh well. This means don't
	//pick up this item clientside with prediction, because we
	//aren't sending over all the data necessary for the player
	//to know if he can.
	ent->s.brokenLimbs = 1;

	//since it uses my server-only physics
	ent->s.eFlags |= EF_CLIENTSMOOTH;
}

#define DISP_HEALTH_ITEM		"item_medpak_instant"
#define DISP_AMMO_ITEM			"ammo_all"

void G_PrecacheDispensers(void)
{
	gitem_t *item;
		
	item = BG_FindItem(DISP_HEALTH_ITEM);
	if (item)
	{
		RegisterItem(item);
	}

	item = BG_FindItem(DISP_AMMO_ITEM);
	if (item)
	{
		RegisterItem(item);
	}
}

void ItemUse_UseDisp(gentity_t *ent, int type)
{
	gitem_t *item = NULL;
	gentity_t *eItem;

	if (!ent->client ||
		ent->client->tossableItemDebounce > level.time)
	{ //can't use it again yet
		return;
	}

	if (ent->client->ps.weaponTime > 0 ||
		ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //busy doing something else
		return;
	}
	
	ent->client->tossableItemDebounce = level.time + TOSS_DEBOUNCE_TIME;

	if (item)
	{
		vec3_t fwd, pos;
		gentity_t	*te;

		eItem = G_Spawn();
		eItem->r.ownerNum = ent->s.number;
		eItem->classname = item->classname;

		VectorCopy(ent->client->ps.origin, pos);
		pos[2] += ent->client->ps.viewheight;

		G_SetOrigin(eItem, pos);
		VectorCopy(eItem->r.currentOrigin, eItem->s.origin);
		trap_LinkEntity(eItem);

		G_SpecialSpawnItem(eItem, item);

		AngleVectors(ent->client->ps.viewangles, fwd, NULL, NULL);
		VectorScale(fwd, 128.0f, eItem->epVelocity);
		eItem->epVelocity[2] = 16.0f;

	//	G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_THERMAL_THROW, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );

		te = G_TempEntity( ent->client->ps.origin, EV_LOCALTIMER );
		te->s.time = level.time;
		te->s.time2 = TOSS_DEBOUNCE_TIME;
		te->s.owner = ent->client->ps.clientNum;
	}
}


//===============================================
//Portable E-Web -rww
//===============================================
//put the e-web away/remove it from the owner
void EWebDisattach(gentity_t *owner, gentity_t *eweb)
{
    owner->client->ewebIndex = 0;
	owner->client->ps.emplacedIndex = 0;
	if (owner->health > 0)
	{
		owner->client->ps.stats[STAT_WEAPONS] = eweb->genericValue11;
	}
	else
	{
		owner->client->ps.stats[STAT_WEAPONS] = 0;
	}
	eweb->think = G_FreeEntity;
	eweb->nextthink = level.time;
}

//precache misc e-web assets
void EWebPrecache(void)
{
	RegisterItem( BG_FindItemForWeapon(WP_TURRET) );
	G_EffectIndex("detpack/explosion.efx");
	G_EffectIndex("turret/muzzle_flash.efx");
}

//e-web death
#define EWEB_DEATH_RADIUS		128
#define EWEB_DEATH_DMG			500

extern void BG_CycleInven(playerState_t *ps, int direction); //bg_misc.c

void EWebDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	vec3_t fxDir;

	G_RadiusDamage(self->r.currentOrigin, self, EWEB_DEATH_DMG, EWEB_DEATH_RADIUS, self, self, MOD_SUICIDE);

	VectorSet(fxDir, 1.0f, 0.0f, 0.0f);
	G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, fxDir);

	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			EWebDisattach(owner, self);

			//make sure it resets next time we spawn one in case we someone obtain one before death
			owner->client->ewebHealth = -1;

			//take it away from him, it is gone forever.
			owner->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1<<HI_EWEB);

			if (owner->client->ps.stats[STAT_HOLDABLE_ITEM] > 0 &&
				bg_itemlist[owner->client->ps.stats[STAT_HOLDABLE_ITEM]].giType == IT_HOLDABLE &&
				bg_itemlist[owner->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_EWEB)
			{ //he has it selected so deselect it and select the first thing available
				owner->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				BG_CycleInven(&owner->client->ps, 1);
			}
		}
	}
}

//e-web pain
void EWebPain(gentity_t *self, gentity_t *attacker, int damage)
{
	//update the owner's health status of me
	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			owner->client->ewebHealth = self->health;
		}
	}
}

//special routine for tracking angles between client and server
void EWeb_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles)
{
	int *thebone = &ent->s.boneIndex1;
	int *firstFree = NULL;
	int i = 0;
	int boneIndex = G_BoneIndex(bone);
	int flags, up, right, forward;
	vec3_t *boneVector = &ent->s.boneAngles1;
	vec3_t *freeBoneVec = NULL;

	while (thebone)
	{
		if (!*thebone && !firstFree)
		{ //if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			firstFree = thebone;
			freeBoneVec = boneVector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{ //this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			boneVector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			boneVector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			boneVector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			boneVector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{ //didn't find it, create it
		if (!firstFree)
		{ //no free bones.. can't do a thing then.
			Com_Printf("WARNING: E-Web has no free bone indexes\n");
			return;
		}

		thebone = firstFree;

		*thebone = boneIndex;
		boneVector = freeBoneVec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *boneVector);

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	flags = BONE_ANGLES_POSTMULT;
	up = POSITIVE_Y;
	right = NEGATIVE_Z;
	forward = NEGATIVE_X;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = ((forward)|(right<<3)|(up<<6));

	trap_G2API_SetBoneAngles( ent->ghoul2,
					0,
					bone,
					angles, 
					flags,
					up,
					right,
					forward,
					NULL,
					100,
					level.time ); 
}

//start an animation on model_root both server side and client side
void EWeb_SetBoneAnim(gentity_t *eweb, int startFrame, int endFrame)
{
	//set info on the entity so it knows to start the anim on the client next snapshot.
	eweb->s.eFlags |= EF_G2ANIMATING;

	if (eweb->s.torsoAnim == startFrame && eweb->s.legsAnim == endFrame)
	{ //already playing this anim, let's flag it to restart
		eweb->s.torsoFlip = !eweb->s.torsoFlip;
	}
	else
	{
		eweb->s.torsoAnim = startFrame;
		eweb->s.legsAnim = endFrame;
	}

	//now set the animation on the server ghoul2 instance.
	assert(eweb->ghoul2);
	trap_G2API_SetBoneAnim(eweb->ghoul2, 0, "model_root", startFrame, endFrame,
		(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND), 1.0f, level.time, -1, 100);
}

//fire a shot off
#define EWEB_MISSILE_DAMAGE			40
void EWebFire(gentity_t *owner, gentity_t *eweb)
{
	mdxaBone_t boltMatrix;
	gentity_t *missile;
	vec3_t p, d, bPoint;
	
	
	vec3_t		angs;
	int i;
	int shots;
	
	if (eweb->genericValue10 == -1)
	{ //oh no
		assert(!"Bad e-web bolt");
		return;
	}

	//get the muzzle point
	trap_G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue10, &boltMatrix, eweb->s.apos.trBase, eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, d);

	//Start the thing backwards into the bounding box so it can't start inside other solid things
	VectorMA(p, -16.0f, d, bPoint);
		

		missile = CreateMissile( bPoint, d, 3000.0, 10000, owner, qfalse);

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;

		VectorSet( missile->r.maxs, 1.0, 1.0, 1.0 );
		VectorScale( missile->r.maxs, -1, missile->r.mins );
	if (owner->client->skillLevel[SK_EWEB] == FORCE_LEVEL_3)
	{
		missile->damage = 3*EWEB_MISSILE_DAMAGE;
	}
	else if (owner->client->skillLevel[SK_EWEB] == FORCE_LEVEL_2)
	{
		missile->damage = 2*EWEB_MISSILE_DAMAGE;
	}		
	else
	{
		missile->damage = EWEB_MISSILE_DAMAGE;
	}	
		
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = 8;
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;

	//ignore the e-web entity
	missile->passThroughNum = eweb->s.number+1;


	//play the muzzle flash
	vectoangles(d, d);
	G_PlayEffectID(G_EffectIndex("turret/muzzle_flash.efx"), p, d);
	

}

//lock the owner into place relative to the cannon pos
void EWebPositionUser(gentity_t *owner, gentity_t *eweb)
{
	mdxaBone_t boltMatrix;
	vec3_t p, d;
	trace_t tr;

	trap_G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue9, &boltMatrix, eweb->s.apos.trBase, eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d);

	VectorMA(p, 32.0f, d, p);
	p[2] = eweb->r.currentOrigin[2];

	p[2] += 4.0f;

	trap_Trace(&tr, owner->client->ps.origin, owner->r.mins, owner->r.maxs, p, owner->s.number, MASK_PLAYERSOLID);

	if (!tr.startsolid && !tr.allsolid && tr.fraction == 1.0f)
	{ //all clear, we can move there
		vec3_t pDown;

		VectorCopy(p, pDown);
		pDown[2] -= 7.0f;
		trap_Trace(&tr, p, owner->r.mins, owner->r.maxs, pDown, owner->s.number, MASK_PLAYERSOLID);

		if (!tr.startsolid && !tr.allsolid)
		{
			VectorSubtract(owner->client->ps.origin, tr.endpos, d);
			if (VectorLength(d) > 1.0f)
			{ //we moved, do some animating
				vec3_t dAng;
				int aFlags = SETANIM_FLAG_HOLD;

				vectoangles(d, dAng);
				dAng[YAW] = AngleSubtract(owner->client->ps.viewangles[YAW], dAng[YAW]);
				if (dAng[YAW] > 0.0f)
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1)
					{ //reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}
					G_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_LEFT1, aFlags, 0);
				}
				else
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_LEFT1)
					{ //reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}
					G_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_RIGHT1, aFlags, 0);
				}
			}
			else if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1 || owner->client->ps.legsAnim == BOTH_STRAFE_LEFT1)
			{ //don't keep animating in place
				owner->client->ps.legsTimer = 0;
			}

			G_SetOrigin(owner, tr.endpos);
			VectorCopy(tr.endpos, owner->client->ps.origin);
		}
	}
	else
	{ //can't move here.. stop using the thing I guess
		EWebDisattach(owner, eweb);
	}
}

//keep the bone angles updated based on the owner's look dir
void EWebUpdateBoneAngles(gentity_t *owner, gentity_t *eweb)
{
	vec3_t yAng;
	float ideal;
	float incr;
	const float turnCap = 4.0f; //max degrees we can turn per update
	
	VectorClear(yAng);
	ideal = AngleSubtract(owner->client->ps.viewangles[YAW], eweb->s.angles[YAW]);
	incr = AngleSubtract(ideal, eweb->angle);

	if (incr > turnCap)
	{
		incr = turnCap;
	}
	else if (incr < -turnCap)
	{
		incr = -turnCap;
	}

	eweb->angle += incr;

	yAng[0] = eweb->angle;
	EWeb_SetBoneAngles(eweb, "cannon_Yrot", yAng);

	EWebPositionUser(owner, eweb);
	if (!owner->client->ewebIndex)
	{ //was removed during position function
		return;
	}

	VectorClear(yAng);
	yAng[2] = AngleSubtract(owner->client->ps.viewangles[PITCH], eweb->s.angles[PITCH])*0.8f;
	EWeb_SetBoneAngles(eweb, "cannon_Xrot", yAng);
}

//keep it updated
extern int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint); //bg_misc.c

void EWebThink(gentity_t *self)
{
	qboolean killMe = qfalse;
	const float gravity = 3.0f;
	const float mass = 0.09f;
	const float bounce = 1.1f;

	if (self->r.ownerNum == ENTITYNUM_NONE)
	{
		killMe = qtrue;
	}
	else
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (!owner->inuse || !owner->client || owner->client->pers.connected != CON_CONNECTED ||
			owner->client->ewebIndex != self->s.number || owner->health < 1)
		{
			killMe = qtrue;
		}
		else if (owner->client->ps.emplacedIndex != self->s.number)
		{ //just go back to the inventory then
			EWebDisattach(owner, self);
			return;
		}

		if (!killMe)
		{
			float yaw;

			if (BG_EmplacedView(owner->client->ps.viewangles, self->s.angles, &yaw, self->s.origin2[0]))
			{
				owner->client->ps.viewangles[YAW] = yaw;
			}
			owner->client->ps.weapon = WP_EMPLACED_GUN;
			owner->client->ps.stats[STAT_WEAPONS] = WP_EMPLACED_GUN;

			if (self->genericValue8 < level.time)
			{ //make sure the anim timer is done
				EWebUpdateBoneAngles(owner, self);
				if (!owner->client->ewebIndex)
				{ //was removed during position function
					return;
				}

				if (owner->client->pers.cmd.buttons & BUTTON_ATTACK)
				{
					if (self->genericValue5 < level.time)
					{ //we can fire another shot off
						EWebFire(owner, self);

						//cheap firing anim
						EWeb_SetBoneAnim(self, 2, 4);
						self->genericValue3 = 1;

						//set fire debounce time
						self->genericValue5 = level.time + 100;
					}
				}
				else if (self->genericValue5 < level.time && self->genericValue3)
				{ //reset the anim back to non-firing
					EWeb_SetBoneAnim(self, 0, 1);
					self->genericValue3 = 0;
				}
			}
		}
	}

	if (killMe)
	{ //something happened to the owner, let's explode
		EWebDie(self, self, self, 999, MOD_SUICIDE);
		return;
	}

	//run some physics on it real quick so it falls and stuff properly
	G_RunExPhys(self, gravity, mass, bounce, qfalse, NULL, 0);

	self->nextthink = level.time;
}

#define EWEB_HEALTH			500

//spawn and set up an e-web ent
gentity_t *EWeb_Create(gentity_t *spawner)
{
	const char *modelName = "models/map_objects/hoth/eweb_model.glm";
	int failSound = G_SoundIndex("sound/interface/shieldcon_empty");
	gentity_t *ent;
	trace_t tr;
	vec3_t fAng, fwd, pos, downPos, s;
	vec3_t mins, maxs;

	VectorSet(mins, -32, -32, -24);
	VectorSet(maxs, 32, 32, 24);

	VectorSet(fAng, 0, spawner->client->ps.viewangles[1], 0);
	AngleVectors(fAng, fwd, 0, 0);

	VectorCopy(spawner->client->ps.origin, s);
	//allow some fudge
	s[2] += 12.0f;

	VectorMA(s, 48.0f, fwd, pos);

	trap_Trace(&tr, s, mins, maxs, pos, spawner->s.number, MASK_PLAYERSOLID);

	if (tr.allsolid || tr.startsolid || tr.fraction != 1.0f)
	{ //can't spawn here, we are in solid
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	ent = G_Spawn();

	ent->clipmask = MASK_PLAYERSOLID;
	ent->r.contents = MASK_PLAYERSOLID;

	ent->physicsObject = qtrue;

	//for the sake of being able to differentiate client-side between this and an emplaced gun
	ent->s.weapon = WP_NONE;

	VectorCopy(pos, downPos);
	downPos[2] -= 18.0f;
	trap_Trace(&tr, pos, mins, maxs, downPos, spawner->s.number, MASK_PLAYERSOLID);

	if (tr.startsolid || tr.allsolid || tr.fraction == 1.0f || tr.entityNum < ENTITYNUM_WORLD)
	{ //didn't hit ground.
		G_FreeEntity(ent);
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	VectorCopy(tr.endpos, pos);

	G_SetOrigin(ent, pos);

	VectorCopy(fAng, ent->s.apos.trBase);
	VectorCopy(fAng, ent->r.currentAngles);

	ent->s.owner = spawner->s.number;
	ent->s.teamowner = spawner->client->sess.sessionTeam;

	ent->takedamage = qtrue;

	if (spawner->client->ewebHealth <= 0)
	{ //refresh the owner's e-web health if its last e-web did not exist or was killed
		spawner->client->ewebHealth = EWEB_HEALTH;
	}

	//resume health of last deployment
	ent->maxHealth = EWEB_HEALTH;
	ent->health = spawner->client->ewebHealth;
	G_ScaleNetHealth(ent);

	ent->die = EWebDie;
	ent->pain = EWebPain;

	ent->think = EWebThink;
	ent->nextthink = level.time;

	//set up the g2 model info
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 128;
	ent->s.modelindex = G_ModelIndex((char *)modelName);

	trap_G2API_InitGhoul2Model(&ent->ghoul2, modelName, 0, 0, 0, 0, 0);

	if (!ent->ghoul2)
	{ //should not happen, but just to be safe.
		G_FreeEntity(ent);
		return NULL;
	}

	//initialize bone angles
	EWeb_SetBoneAngles(ent, "cannon_Yrot", vec3_origin);
	EWeb_SetBoneAngles(ent, "cannon_Xrot", vec3_origin);

	ent->genericValue10 = trap_G2API_AddBolt(ent->ghoul2, 0, "*cannonflash"); //muzzle bolt
	ent->genericValue9 = trap_G2API_AddBolt(ent->ghoul2, 0, "cannon_Yrot"); //for placing the owner relative to rotation

	//set the constraints for this guy as an emplaced weapon, and his constraint angles
	ent->s.origin2[0] = 360.0f; //360 degrees in either direction

	VectorCopy(fAng, ent->s.angles); //consider "angle 0" for constraint

	//angle of y rot bone
	ent->angle = 0.0f;

	ent->r.ownerNum = spawner->s.number;
	trap_LinkEntity(ent);

	//store off the owner's current weapons, we will be forcing him to use the "emplaced" weapon
	ent->genericValue11 = spawner->client->ps.stats[STAT_WEAPONS];

	//start the "unfolding" anim
	EWeb_SetBoneAnim(ent, 4, 20);
	//don't allow use until the anim is done playing (rough time estimate)
	ent->genericValue8 = level.time + 500;

	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);

	return ent;
}

#define EWEB_USE_DEBOUNCE		1000
//use the e-web
void ItemUse_UseEWeb(gentity_t *ent)
{
	if (ent->client->ewebTime > level.time)
	{ //can't use again yet
		return;
	}

	if (ent->client->ps.weaponTime > 0 ||
		ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //busy doing something else
		return;
	}

	if (ent->client->ps.emplacedIndex && !ent->client->ewebIndex)
	{ //using an emplaced gun already that isn't our own e-web
		return;
	}

	if (ent->client->ewebIndex)
	{ //put it away
		EWebDisattach(ent, &g_entities[ent->client->ewebIndex]);
	}
	else
	{ //create it
		gentity_t *eweb = EWeb_Create(ent);

		if (eweb)
		{ //if it's null the thing couldn't spawn (probably no room)
			ent->client->ewebIndex = eweb->s.number;
			ent->client->ps.emplacedIndex = eweb->s.number;
		}
	}

	ent->client->ewebTime = level.time + EWEB_USE_DEBOUNCE;
}
//===============================================
//End E-Web
//===============================================


int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int			quantity;
	int			i;
	gclient_t	*client;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->giTag] = 
			level.time - ( level.time % 1000 );

		G_LogWeaponPowerup(other->s.number, ent->item->giTag);
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	if (ent->item->giTag == PW_YSALAMIRI)
	{
		other->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] = 0;
		other->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] = 0;
		other->client->ps.powerups[PW_FORCE_BOON] = 0;
	}

	// give any nearby players a "denied" anti-reward
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		vec3_t		delta;
		float		len;
		vec3_t		forward;
		trace_t		tr;

		client = &level.clients[i];
		if ( client == other->client ) {
			continue;
		}
		if ( client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
			continue;
		}

    // if same team in team game, no sound
    // cannot use OnSameTeam as it expects to g_entities, not clients
  	if ( g_gametype.integer >= GT_TEAM && other->client->sess.sessionTeam == client->sess.sessionTeam  ) {
      continue;
    }

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
		len = VectorNormalize( delta );
		if ( len > 192 ) {
			continue;
		}

		// if not facing, no sound
		AngleVectors( client->ps.viewangles, forward, NULL, NULL );
		if ( DotProduct( delta, forward ) < 0.4 ) {
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction != 1.0 ) {
			continue;
		}

		// anti-reward
		client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
	}
	return RESPAWN_POWERUP;
}

//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {

	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	other->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << ent->item->giTag);

	G_LogWeaponItem(other->s.number, ent->item->giTag);

	return adjustRespawnTime(RESPAWN_HOLDABLE, ent->item->giType, ent->item->giTag);
}


//======================================================================

//[AmmoSys]
void Add_Ammo3 (gentity_t *ent, int weapon, int count, int *stop, qboolean *gaveSome)
{ // weapon is actually ammotype
	//if ( ent->client->ps.eFlags & EF_DOUBLE_AMMO ) {
	//	if ( ent->client->ps.ammo[weapon] < ammoData[weapon].max*2 ) {
	//		*gaveSome = qtrue;
	//		ent->client->ps.ammo[weapon] += count;
	//		if ( ent->client->ps.ammo[weapon] >= ammoData[weapon].max*2 ) {
	//			ent->client->ps.ammo[weapon] = ammoData[weapon].max*2;
	//		} else {
	//			*stop = 0;
	//		}
	//	}
	//} 
	//else 
	{
		if ( ent->client->ps.ammo[weapon] < ammoData[weapon].max ) {
			*gaveSome = qtrue;
			ent->client->ps.ammo[weapon] += count;
			if ( ent->client->ps.ammo[weapon] >= ammoData[weapon].max ) {
				ent->client->ps.ammo[weapon] = ammoData[weapon].max;
			} else {
				*stop = 0;
			}
		}
	}
}
//[/AmmoSys]

void Add_Ammo (gentity_t *ent, int weapon, int count)
{ // weapon is actually type
	//[AmmoSys]
	//if ( ent->client->ps.eFlags & EF_DOUBLE_AMMO ) {
	//	if ( ent->client->ps.ammo[weapon] < ammoData[weapon].max*2 ) {
	//		ent->client->ps.ammo[weapon] += count;
	//		if ( ent->client->ps.ammo[weapon] >= ammoData[weapon].max*2 ) {
	//			ent->client->ps.ammo[weapon] = ammoData[weapon].max*2;
	//		}
	//	}
	//} 
	//else 
	{
		if ( ent->client->ps.ammo[weapon] < ammoData[weapon].max ) {
			ent->client->ps.ammo[weapon] += count;
			if ( ent->client->ps.ammo[weapon] >= ammoData[weapon].max ) {
				ent->client->ps.ammo[weapon] = ammoData[weapon].max;
			}
		}
	}
	/*if ( ent->client->ps.ammo[weapon] < ammoData[weapon].max )
	{
		ent->client->ps.ammo[weapon] += count;
		if ( ent->client->ps.ammo[weapon] > ammoData[weapon].max )
		{
			ent->client->ps.ammo[weapon] = ammoData[weapon].max;
		}
	}*/
	//[/AmmoSys]
}

int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
	int		quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	if (ent->item->giTag == -1)
	{ //an ammo_all, give them a bit of everything
		if ( g_gametype.integer == GT_SIEGE )	// complaints that siege tech's not giving enough ammo.  Does anything else use ammo all?
		{
			Add_Ammo(other, AMMO_BLASTER, 100);
			Add_Ammo(other, AMMO_POWERCELL, 100);
			Add_Ammo(other, AMMO_METAL_BOLTS, 100);
			Add_Ammo(other, AMMO_ROCKETS, 5);
			if (other->client->ps.stats[STAT_WEAPONS] & (1<<WP_DET_PACK))
			{
				Add_Ammo(other, AMMO_DETPACK, 2);
			}
			if (other->client->ps.stats[STAT_WEAPONS] & (1<<WP_THERMAL))
			{
				Add_Ammo(other, AMMO_THERMAL, 2);
			}
			if (other->client->ps.stats[STAT_WEAPONS] & (1<<WP_TRIP_MINE))
			{
				Add_Ammo(other, AMMO_TRIPMINE, 2);
			}
		}
		else
		{
			Add_Ammo(other, AMMO_BLASTER, 50);
			Add_Ammo(other, AMMO_POWERCELL, 50);
			Add_Ammo(other, AMMO_METAL_BOLTS, 50);
			Add_Ammo(other, AMMO_ROCKETS, 2);
		}
	}
	else
	{
		Add_Ammo (other, ent->item->giTag, quantity);
	}

	return adjustRespawnTime(RESPAWN_AMMO, ent->item->giType, ent->item->giTag);
}

//======================================================================

//[VisualWeapons]
qboolean OJP_AllPlayersHaveClientPlugin(void);
//[/VisualWeapons]
int Pickup_Weapon (gentity_t *ent, gentity_t *other) {
	int		quantity=10;

	/*
	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ent->item->quantity;
		}

		// dropped items and teamplay weapons always have full ammo
		if ( ! (ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM ) {
			// respawning rules

			// New method:  If the player has less than half the minimum, give them the minimum, else add 1/2 the min.

			// drop the quantity if the already have over the minimum
			if ( other->client->ps.ammo[ ent->item->giTag ] < quantity*0.5 ) {
				quantity = quantity - other->client->ps.ammo[ ent->item->giTag ];
			} else {
				quantity = quantity*0.5;		// only add half the value.
			}

			// Old method:  If the player has less than the minimum, give them the minimum, else just add 1.
*//*
			// drop the quantity if the already have over the minimum
			if ( other->client->ps.ammo[ ent->item->giTag ] < quantity ) {
				quantity = quantity - other->client->ps.ammo[ ent->item->giTag ];
			} else {
				quantity = 1;		// only add a single shot
			}
			*/
		//}	
	//}

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= ( 1 << ent->item->giTag );

	//[VisualWeapons]
	//update the weapon stats for this player since they have changed.
	if(OJP_AllPlayersHaveClientPlugin())
	{//don't send the weapon updates if someone isn't able to process this new event type (IE anyone without
		//the OJP client plugin)
		G_AddEvent(other, EV_WEAPINVCHANGE, other->client->ps.stats[STAT_WEAPONS]);
	}
	//[/VisualWeapons]

	//Add_Ammo( other, ent->item->giTag, quantity );
	Add_Ammo( other, weaponData[ent->item->giTag].ammoIndex, quantity );

	G_LogWeaponPickup(other->s.number, ent->item->giTag);
	
	// team deathmatch has slow weapon respawns
	if ( g_gametype.integer == GT_TEAM ) 
	{
		return adjustRespawnTime(RESPAWN_TEAM_WEAPON, ent->item->giType, ent->item->giTag);
	}

	return adjustRespawnTime(g_weaponRespawn.integer, ent->item->giType, ent->item->giTag);
}


//======================================================================

int Pickup_Health (gentity_t *ent, gentity_t *other) {
	int			max;
	int			quantity;

	// small and mega healths will go over the max
	if ( ent->item->quantity != 5 && ent->item->quantity != 100 ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->item->quantity == 100 ) {		// mega health respawns slow
		return RESPAWN_MEGAHEALTH;
	}

	return adjustRespawnTime(RESPAWN_HEALTH, ent->item->giType, ent->item->giTag);
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) 
{
	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
	if ( other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_ARMOR] * ent->item->giTag ) 
	{
		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_ARMOR] * ent->item->giTag;
	}

	return adjustRespawnTime(RESPAWN_ARMOR, ent->item->giType, ent->item->giTag);
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
	// randomly select from teamed entities
	if (ent->team) {
		gentity_t	*master;
		int	count;
		int choice;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.eFlags &= ~EF_NODRAW;
	ent->s.eFlags &= ~(EF_NODRAW | EF_ITEMPLACEHOLDER);
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/respawn1" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}

qboolean CheckItemCanBePickedUpByNPC( gentity_t *item, gentity_t *pickerupper )
{
	if ( (item->flags&FL_DROPPED_ITEM) 
		&& item->activator != &g_entities[0] 
		&& pickerupper->s.number 
		&& pickerupper->s.weapon == WP_NONE 
		&& pickerupper->enemy 
		&& pickerupper->painDebounceTime < level.time
		&& pickerupper->NPC && pickerupper->NPC->surrenderTime < level.time //not surrendering
		&& !(pickerupper->NPC->scriptFlags&SCF_FORCED_MARCH) //not being forced to march
		/*&& item->item->giTag != INV_SECURITY_KEY*/ )
	{//non-player, in combat, picking up a dropped item that does NOT belong to the player and it *not* a security key
		if ( level.time - item->s.time < 3000 )//was 5000
		{
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}

/*
===============
Touch_Item
===============
*/
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
	int			respawn;
	qboolean	predict;

	if (ent->genericValue10 > level.time &&
		other &&
		other->s.number == ent->genericValue11)
	{ //this is the ent that we don't want to be able to touch us for x seconds
		return;
	}

	if (ent->s.eFlags & EF_ITEMPLACEHOLDER)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (ent->item->giType == IT_WEAPON &&
		ent->s.powerups &&
		ent->s.powerups < level.time)
	{
		ent->s.generic1 = 0;
		ent->s.powerups = 0;
	}

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup

	//[ExpSys]
	//since players can use powers from both sides of the force, they should be able to use
	//both force enlightenment holocrons
	/*
	if (ent->item->giType == IT_POWERUP &&
		(ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT || ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK))
	{
		if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT)
		{
			if (other->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				return;
			}
		}
		else
		{
			if (other->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				return;
			}
		}
	}
	*/
	//[/ExpSys]

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &other->client->ps ) ) {
		return;
	}

	
	if ( other->client->NPC_class == CLASS_ATST || 
		other->client->NPC_class == CLASS_GONK || 
		other->client->NPC_class == CLASS_MARK1 || 
		other->client->NPC_class == CLASS_MARK2 || 
		other->client->NPC_class == CLASS_MOUSE || 
		other->client->NPC_class == CLASS_PROBE || 
		other->client->NPC_class == CLASS_PROTOCOL || 
		other->client->NPC_class == CLASS_R2D2 || 
		other->client->NPC_class == CLASS_R5D2 || 
		other->client->NPC_class == CLASS_SEEKER  || 
		other->client->NPC_class == CLASS_REMOTE || 
		other->client->NPC_class == CLASS_RANCOR || 
		other->client->NPC_class == CLASS_WAMPA || 
		//other->client->NPC_class == CLASS_JAWA || //FIXME: in some cases it's okay?
		other->client->NPC_class == CLASS_UGNAUGHT || //FIXME: in some cases it's okay?
		other->client->NPC_class == CLASS_SENTRY )
	{//FIXME: some flag would be better
		//droids can't pick up items/weapons!
		return;
	}

	if ( CheckItemCanBePickedUpByNPC( ent, other ) )
	{
		if ( other->NPC && other->NPC->goalEntity && other->NPC->goalEntity->enemy == ent )
		{//they were running to pick me up, they did, so clear goal
			other->NPC->goalEntity = NULL;
			other->NPC->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	else if ( !(ent->spawnflags &  ITMSF_ALLOWNPC) )
	{// NPCs cannot pick it up
		if ( other->s.eType == ET_NPC )
		{// Not the player?
			qboolean dontGo = qfalse;
			if (ent->item->giType == IT_AMMO &&
				ent->item->giTag == -1 &&
				other->s.NPC_class == CLASS_VEHICLE &&
				other->m_pVehicle &&
				other->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
			{ //yeah, uh, atst gets healed by these things
                if (other->maxHealth &&
					other->health < other->maxHealth)
				{
					other->health += 80;
					if (other->health > other->maxHealth)
					{
						other->health = other->maxHealth;
					}
					G_ScaleNetHealth(other);
					dontGo = qtrue;
				}
			}

			if (!dontGo)
			{
				return;
			}
		}
	}

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	predict = other->client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch( ent->item->giType ) {
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
		if (ent->item->giTag == AMMO_THERMAL || ent->item->giTag == AMMO_TRIPMINE || ent->item->giTag == AMMO_DETPACK)
		{
			int weapForAmmo = 0;

			if (ent->item->giTag == AMMO_THERMAL)
			{
				weapForAmmo = WP_THERMAL;
			}
			else if (ent->item->giTag == AMMO_TRIPMINE)
			{
				weapForAmmo = WP_TRIP_MINE;
			}
			else
			{
				weapForAmmo = WP_DET_PACK;
			}

			if (other && other->client && other->client->ps.ammo[weaponData[weapForAmmo].ammoIndex] > 0 )
			{
				other->client->ps.stats[STAT_WEAPONS] |= (1 << weapForAmmo);
				//[VisualWeapons]
				//update the weapon stats for this player since they have changed.
				if(OJP_AllPlayersHaveClientPlugin())
				{//don't send the weapon updates if someone isn't able to process this new event type (IE anyone without
				//the OJP client plugin)
					G_AddEvent(other, EV_WEAPINVCHANGE, other->client->ps.stats[STAT_WEAPONS]);
				}
				//[/VisualWeapons]
			}
		}
//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
//		predict = qtrue;
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	default:
		return;
	}

	if ( !respawn ) {
		return;
	}

	// play the normal pickup sound
	if (predict) {
		if (other->client)
		{
			BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, ent->s.number, &other->client->ps);
		}
		else
		{
			G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.number );
		}
	} else {
		G_AddEvent( other, EV_ITEM_PICKUP, ent->s.number );
	}

	// powerup pickups are global broadcasts
	if ( /*ent->item->giType == IT_POWERUP ||*/ ent->item->giType == IT_TEAM) {
		// if we want the global sound to play
		if (!ent->speed) {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			te->r.svFlags |= SVF_BROADCAST;
		} else {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	// fire item targets
	G_UseTargets (ent, other);

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if ( ent->wait ) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random ) {
		respawn += crandom() * ent->random;
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	if (!(ent->flags & FL_DROPPED_ITEM) && (ent->item->giType==IT_WEAPON || ent->item->giType==IT_POWERUP))
	{
		ent->s.eFlags |= EF_ITEMPLACEHOLDER;
		ent->s.eFlags &= ~EF_NODRAW;
	}
	else
	{
		ent->s.eFlags |= EF_NODRAW;
		ent->r.svFlags |= SVF_NOCLIENT;
	}
	ent->r.contents = 0;

	if (ent->genericValue9)
	{ //dropped item, should be removed when picked up
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	// ZOID
	// A negative respawn times means to never respawn this item (but don't 
	// delete it).  This is used by items that are respawned by third party 
	// events such as ctf flags
	if ( respawn <= 0 ) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity( ent );
}


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	if (dropped->s.modelindex < 0)
	{
		dropped->s.modelindex = 0;
	}
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);

	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->flags |= FL_BOUNCE_HALF;
	if ((g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY) && item->giType == IT_TEAM) { // Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem( dropped );

		//rww - so bots know
		if (strcmp(dropped->classname, "team_CTF_redflag") == 0)
		{
			droppedRedFlag = dropped;
		}
		else if (strcmp(dropped->classname, "team_CTF_blueflag") == 0)
		{
			droppedBlueFlag = dropped;
		}
	} else { // auto-remove after 30 seconds
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	if (item->giType == IT_WEAPON || item->giType == IT_POWERUP)
	{
		dropped->s.eFlags |= EF_DROPPEDWEAPON;
	}

	vectoangles(velocity, dropped->s.angles);
	dropped->s.angles[PITCH] = 0;

	if (item->giTag == WP_TRIP_MINE ||
		item->giTag == WP_DET_PACK)
	{
		dropped->s.angles[PITCH] = -90;
	}

	if (item->giTag != WP_BOWCASTER &&
		item->giTag != WP_DET_PACK &&
		item->giTag != WP_THERMAL)
	{
		dropped->s.angles[ROLL] = -90;
	}

	dropped->physicsObject = qtrue;

	trap_LinkEntity (dropped);

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	return LaunchItem( item, ent->s.pos.trBase, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;
//	gitem_t		*item;

//	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
//	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );


		if (HasSetSaberOnly())
		{
			if (ent->item->giType == IT_AMMO)
			{
				G_FreeEntity( ent );
				return;
			}

			if (ent->item->giType == IT_HOLDABLE)
			{
				if (ent->item->giTag == HI_SEEKER  ||
					ent->item->giTag == HI_SHIELD ||
					ent->item->giTag == HI_MEDPAC ||
					ent->item->giTag == HI_SHIELDBOOSTER ||
					ent->item->giTag == HI_BINOCULARS ||
					ent->item->giTag == HI_SENTRY_GUN ||
					ent->item->giTag == HI_JETPACK ||
					ent->item->giTag == HI_SQUADTEAM ||
					ent->item->giTag == HI_VEHICLEMOUNT ||
					ent->item->giTag == HI_EWEB ||
					ent->item->giTag == HI_CLOAK ||
					ent->item->giTag == HI_FLAMETHROWER ||
					ent->item->giTag == HI_ELECTROSHOCKER ||
					ent->item->giTag == HI_SPHERESHIELD ||
					ent->item->giTag == HI_OVERLOAD ||
					ent->item->giTag == HI_GRAPPLE)
				{
					G_FreeEntity( ent );
					return;
				}
			}
		}
	


	if (g_gametype.integer == GT_HOLOCRON)
	{
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	//[MOREFORCEOPTIONS]
	//if (g_forcePowerDisable.integer)
	if (AllForceDisabled(g_forcePowerDisable.integer))
	//[/MOREFORCEOPTIONS]
	{ //if force powers disabled, don't add force powerups
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK ||
				ent->item->giTag == PW_FORCE_BOON)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}



	if (g_gametype.integer != GT_CTF &&
		g_gametype.integer != GT_CTY &&
		ent->item->giType == IT_TEAM)
	{
		int killMe = 0;

		switch (ent->item->giTag)
		{
		case PW_REDFLAG:
			killMe = 1;
			break;
		case PW_BLUEFLAG:
			killMe = 1;
			break;
		case PW_NEUTRALFLAG:
			killMe = 1;
			break;
		default:
			break;
		}

		if (killMe)
		{
			G_FreeEntity( ent );
			return;
		}
	}

	VectorSet (ent->r.mins, -8, -8, -0);
	VectorSet (ent->r.maxs, 8, 8, 16);

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;

	// create a Ghoul2 model if the world model is a glm
/*	item = &bg_itemlist[ ent->s.modelindex ];
	if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
	{
		trap_G2API_InitGhoul2Model(&ent->s, item->world_model[0], G_ModelIndex(item->world_model[0] ), 0, 0, 0, 0);
		ent->s.radius = 60;
	}
*/
	if ( ent->spawnflags & ITMSF_SUSPEND ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor

		//if it is directly even with the floor it will return startsolid, so raise up by 0.1
		//and temporarily subtract 0.1 from the z maxs so that going up doesn't push into the ceiling
		ent->s.origin[2] += 0.1;
		ent->r.maxs[2] -= 0.1;

		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_Printf ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		//add the 0.1 back after the trace
		ent->r.maxs[2] += 0.1;

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	//[CoOp]
	//added back in vertical flag but the rack code doesn't use this anymore anyway.
	//leaving it in, just in case.
	if(ent->spawnflags & ITMSF_VERTICAL)
	{
		ent->s.angles[PITCH] += 75;
		G_SetAngles(ent, ent->s.angles);
	}
	//[/CoOp]

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// powerups don't spawn in for a while
	/*
	if ( ent->item->giType == IT_POWERUP ) {
		float	respawn;

		respawn = 45 + crandom() * 15;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}
	*/

	trap_LinkEntity (ent);
}


qboolean	itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

	// Set up team stuff
	Team_InitGame();

	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY ) {
		gitem_t	*item;

		// check for the two flags
		item = BG_FindItem( "team_CTF_redflag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map" );
		}
		item = BG_FindItem( "team_CTF_blueflag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map" );
		}
	}
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	memset( itemRegistered, 0, sizeof( itemRegistered ) );

	// players always start with the base weapon
														 
													   
	RegisterItem( BG_FindItemForWeapon( WP_MELEE ) );
	RegisterItem( BG_FindItemForWeapon( WP_STUN_BATON ) );
	RegisterItem( BG_FindItemForWeapon( WP_SABER ) );
	RegisterItem( BG_FindItemForWeapon( WP_BRYAR_PISTOL ) );
	//[ExpSys]
	//addition possible starting weapons
	RegisterItem( BG_FindItemForWeapon( WP_BLASTER ) );
	RegisterItem( BG_FindItemForWeapon( WP_DISRUPTOR ) );
																						  
	RegisterItem( BG_FindItemForWeapon( WP_BOWCASTER ) );
	RegisterItem( BG_FindItemForWeapon( WP_REPEATER ) );
	RegisterItem( BG_FindItemForWeapon( WP_DEMP2 ) );
	RegisterItem( BG_FindItemForWeapon( WP_FLECHETTE ) );
	RegisterItem( BG_FindItemForWeapon( WP_CONCUSSION ) );
	RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );
	RegisterItem( BG_FindItemForWeapon( WP_THERMAL ) );
	RegisterItem( BG_FindItemForWeapon( WP_TRIP_MINE ) );
	RegisterItem( BG_FindItemForWeapon( WP_DET_PACK ) );
	RegisterItem( BG_FindItemForWeapon( WP_BRYAR_OLD ) );
	//[/ExpSys]

	if (g_gametype.integer == GT_SIEGE)
	{ //kind of cheesy, maybe check if siege class with disp's is gonna be on this map too
		G_PrecacheDispensers();
	}
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

//	G_Printf( "%i items registered\n", count );
	trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled( gitem_t *item ) {

	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue( name );
}


//[MOREWEAPOPTIONS]
//This function checks an ammo type against the disabled weapons list.
qboolean G_AmmoDisabled (int wDisable, gitem_t *item)
{
	if (item->giType != IT_AMMO)
	{
		G_Printf("Error: G_AmmoDisabled run for an non-ammo item.\n"); 
		return qfalse;
	}

	switch ( item->giTag )
	{
		case AMMO_FORCE:
		case AMMO_EMPLACED:
			//never disable these since they aren't actually associated with a weapon
			return qfalse;
			break;
		case AMMO_BLASTER:
			if ( !(wDisable & (1 << WP_BRYAR_PISTOL)) 
				|| !(wDisable & (1 << WP_BLASTER))  
				|| !(wDisable & (1 << WP_BRYAR_OLD)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_POWERCELL:
			if ( !(wDisable & (1 << WP_DISRUPTOR)) 
				|| !(wDisable & (1 << WP_BOWCASTER))  
				|| !(wDisable & (1 << WP_DEMP2)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_METAL_BOLTS:
			if ( !(wDisable & (1 << WP_REPEATER)) 
				|| !(wDisable & (1 << WP_FLECHETTE)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_ROCKETS:
			if ( !(wDisable & (1 << WP_ROCKET_LAUNCHER)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_THERMAL:
			if ( !(wDisable & (1 << WP_THERMAL)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_TRIPMINE:
			if ( !(wDisable & (1 << WP_THERMAL)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		case AMMO_DETPACK:
			if ( !(wDisable & (1 << WP_THERMAL)) )
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
			break;
		default:
			G_Printf("Error: G_AmmoDisabled couldn't find ammo type.\n");
			return qfalse;
			break;
	};
}
//[/MOREWEAPOPTIONS]


/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
	int wDisable = 0;

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	if (item->giType == IT_WEAPON &&
		wDisable &&
		(wDisable & (1 << item->giTag)))
	{
	//[MOREWEAPOPTIONS]
		//if (g_gametype.integer != GT_JEDIMASTER)
		//{
			G_FreeEntity( ent );
			return;
		//}
	}

	int iDisable = 0;
	iDisable = g_itemDisable.integer;
	

	if (item->giType == IT_HOLDABLE &&
		iDisable &&
		(iDisable & (1 << item->giTag)))
	{
	//[MOREWEAPOPTIONS]
		//if (g_gametype.integer != GT_JEDIMASTER)
		//{
			G_FreeEntity( ent );
			return;
		//}
	}
	//make sure the 
	
	
	if (item->giType == IT_AMMO)
	{
		if(G_AmmoDisabled( wDisable, item ))
		{//This ammo is disabled.
			G_FreeEntity( ent );
			return;
		}
	}
	//[/MOREWEAPOPTIONS]

	RegisterItem( item );
	if ( G_ItemDisabled(item) )
		return;

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;		// items are bouncy

	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/respawn1" );
		G_SpawnFloat( "noglobalsound", "0", &ent->speed);
	}
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	if ((ent->s.weapon == WP_DET_PACK && ent->s.eType == ET_GENERAL && ent->physicsObject))
	{ //detpacks only
		if (ent->touch)
		{
			ent->touch(ent, &g_entities[trace->entityNum], trace);
			return;
		}
	}

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	if (ent->s.eType == ET_HOLOCRON ||
		(ent->s.shouldtarget && ent->s.eType == ET_GENERAL && ent->physicsObject))
	{ //holocrons and sentry guns
		if (ent->touch)
		{
			ent->touch(ent, &g_entities[trace->entityNum], trace);
		}
	}
}


/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY ) {
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, 
		ent->r.ownerNum, mask );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if (ent->item && ent->item->giType == IT_TEAM) {
			Team_FreeEntity(ent);
		} else {
			G_FreeEntity( ent );
		}
		return;
	}

	G_BounceItem( ent, &tr );
}

