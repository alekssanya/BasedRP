﻿// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "../ghoul2/g2.h"
#include "bg_saga.h"
#include "g_db.h"

// g_client.c -- client functions that don't happen every frame

static vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
static vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

extern int g_siegeRespawnCheck;
extern int ojp_ffaRespawnTimerCheck;//[FFARespawnTimer]

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void WP_SaberRemoveG2Model( gentity_t *saberent );
//[StanceSelection]
//extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
//[/StanceSelection]
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );

forcedata_t Client_Force[MAX_CLIENTS];

//[LastManStanding]
void OJP_Spectator(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		ent->client->tempSpectate = Q3_INFINITE;
		ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
		ent->client->ps.weapon = WP_NONE;
		ent->client->ps.stats[STAT_WEAPONS] = 0;
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		ent->takedamage = qfalse;
		trap_LinkEntity(ent);
	}
}
//[/LastManStanding]


/*QUAKED info_player_duel (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for duelists in duel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel1 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for lone duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel1( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel2 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for paired duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel2( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	//[CoOp]
	//make sure the spawnpoint's genericValue1 is cleared so that our magical waypoint
	//system for CoOp will work.
	ent->genericValue1 = 0;
	//[/CoOp]
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Targets will be fired when someone spawns in on them.
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_red (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Red Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_red(gentity_t *ent) {
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_blue (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Blue Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_blue(gentity_t *ent) {
	SP_info_player_deathmatch( ent );
}

void SiegePointUse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	//Toggle the point on/off
	if (self->genericValue1)
	{
		self->genericValue1 = 0;
	}
	else
	{
		self->genericValue1 = 1;
	}
}

/*QUAKED info_player_siegeteam1 (1 0 0) (-16 -16 -24) (16 16 32)
siege start point - team1
name and behavior of team1 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam1(gentity_t *ent) {
	int soff = 0;

	if (g_gametype.integer != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_siegeteam2 (0 0 1) (-16 -16 -24) (16 16 32)
siege start point - team2
name and behavior of team2 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam2(gentity_t *ent) {
	int soff = 0;

	if (g_gametype.integer != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) RED BLUE
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
RED - In a Siege game, the intermission will happen here if the Red (attacking) team wins
BLUE - In a Siege game, the intermission will happen here if the Blue (defending) team wins
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_red (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Red (attacking) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_red( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_blue (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Blue (defending) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_blue( gentity_t *ent ) {

}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent = &g_entities[self->client->ps.saberIndex];
	vec3_t a;
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{ //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{ //someone killed us and we had the saber thrown, so actually move this saber to the saber location
	  //if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
	  //apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	WP_SaberAddG2Model( ent, self->client->saber[0].model, self->client->saber[0].skin );

	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
		return;
	}

	if (!altVelocity)
	{
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0]*256;
		ent->s.pos.trDelta[1] = a[1]*256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap_LinkEntity(ent);
}

void JMSaberThink(gentity_t *ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{//RACC - someone has the JediMaster saber.
		if (!ent->enemy->client || !ent->enemy->inuse)
		{ //disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex(DEFAULT_SABER_MODEL);
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2[0] = 1;
			ent->pos2[1] = 0; //respawn next think
			trap_LinkEntity(ent);
		}
		else
		{
			ent->pos2[1] = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->pos2[0] && ent->pos2[1] < level.time)
	{//RACC - the jedimaster saber is loose.
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap_LinkEntity(ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}


//[ExpSys]
void DetermineDodgeMax(gentity_t *ent);
//[/ExpSys]
void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int i = 0;
//	gentity_t *te;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (other->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
	other->client->ps.stats[STAT_WEAPONS] |= (1 << WP_SABER);
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master 
	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, va("%i", other->s.number ) );

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap_SendServerCommand( -1, va("cp \"%s %s\n\"", other->client->pers.netname, G_GetStringEdString("MP_SVGAME", "BECOMEJM")) );

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < 999 && other->health > 0)
	{ //full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = other->client->ps.stats[STAT_MAX_HEALTH] = 999;		
	}
	
	if (other->client->ps.stats[STAT_ARMOR] < 999)
	{

		other->client->ps.stats[STAT_ARMOR] = 999;
	}
	
	if (other->client->ps.stats[STAT_DODGE] < 250)
	{

		other->client->ps.stats[STAT_DODGE] = other->client->ps.stats[STAT_MAX_DODGE] =250;
	}
	
	if (other->client->ps.fd.forcePower < 250)
	{

		other->client->ps.fd.forcePower = other->client->ps.fd.forcePowerMax = 250;
	}
	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

		i++;
	}
	if (other->client->skillLevel[SK_BLUESTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_BLUESTYLE] |= FORCE_LEVEL_1;
	}	
	if (other->client->skillLevel[SK_REDSTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_REDSTYLE] |= FORCE_LEVEL_1;
	}	
	if (other->client->skillLevel[SK_PURPLESTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_PURPLESTYLE] |= FORCE_LEVEL_1;
	}
	if (other->client->skillLevel[SK_GREENSTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_GREENSTYLE] |= FORCE_LEVEL_1;
	}		
	if (other->client->skillLevel[SK_DUALSTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_DUALSTYLE] |= FORCE_LEVEL_1;
	}	
	if (other->client->skillLevel[SK_STAFFSTYLE] < FORCE_LEVEL_1)
	{
	other->client->skillLevel[SK_STAFFSTYLE] |= FORCE_LEVEL_1;
	}	
	//[ExpSys]
	//recalc DP
	//set dp to max.
	//[/ExpSys]

	self->pos2[0] = 1;
	self->pos2[1] = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue(self->s.number);

	return;
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t *ent)
{
	if (g_gametype.integer != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex(DEFAULT_SABER_MODEL);
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( ent->r.maxs, 3, 3, 3 );
	VectorSet( ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = JMSaberTouch;

	trap_LinkEntity(ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest ) 
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( dest, mover->r.mins, mins );
	VectorAdd( dest, mover->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = &g_entities[touch[i]];
		if ( hit == mover )
		{
			continue;
		}

		if ( hit->r.contents & mover->r.contents )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	//in Team DM, look for a team start spot first, if any
	if ( g_gametype.integer == GT_TEAM 
		&& team != TEAM_FREE 
		&& team != TEAM_SPECTATOR )
	{
		char *classname = NULL;
		if ( team == TEAM_RED )
		{
			classname = "info_player_start_red";
		}
		else
		{
			classname = "info_player_start_blue";
		}
		while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
			if ( SpotWouldTelefrag( spot ) ) {
				continue;
			}
			VectorSubtract( spot->s.origin, avoidPoint, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					if (numSpots > 64)
						numSpots = 64;
					break;
				}
			}
			if (i >= numSpots && numSpots < 64) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
	}

	if ( !numSpots )
	{//couldn't find any of the above
		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
			if ( SpotWouldTelefrag( spot ) ) {
				continue;
			}
			VectorSubtract( spot->s.origin, avoidPoint, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					if (numSpots > 64)
						numSpots = 64;
					break;
				}
			}
			if (i >= numSpots && numSpots < 64) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
		if (!numSpots) {
			spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
			if (!spot)
				G_Error( "Couldn't find a spawn point" );
			VectorCopy (spot->s.origin, origin);
			origin[2] += 9;
			VectorCopy (spot->s.angles, angles);
			return spot;
		}
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

gentity_t *SelectDuelSpawnPoint( int team, vec3_t avoidPoint, vec3_t origin, vec3_t angles )
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;
	char		*spotName;

	if (team == DUELTEAM_LONE)
	{
		spotName = "info_player_duel1";
	}
	else if (team == DUELTEAM_DOUBLE)
	{
		spotName = "info_player_duel2";
	}
	else if (team == DUELTEAM_SINGLE)
	{
		spotName = "info_player_duel";
	}
	else
	{
		spotName = "info_player_deathmatch";
	}
tryAgain:

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), spotName)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots)
	{
		if (Q_stricmp(spotName, "info_player_deathmatch"))
		{ //try the loop again with info_player_deathmatch as the target if we couldn't find a duel spot
			spotName = "info_player_deathmatch";
			goto tryAgain;
		}

		//If we got here we found no free duel or DM spots, just try the first DM spot
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team ) {
	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, team );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, team_t team ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles, team );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

//[NOBODYQUE]
// replaced by g_corpseRemovalTime.
//#define BODY_SINK_TIME		30000//45000
//[/NOBODYQUE]

/*
===============
InitBodyQue
===============
*/
//[NOBODYQUE]
//we don't want to use a reserved body que system anymore.
/*
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}
*/
//[/NOBODYQUE]


/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	//[NOBODYQUE]
	//while we're at it, I'm making the corpse removal time be set by a cvar like in SP.
	if(g_corpseRemovalTime.integer && (level.time - ent->timestamp) > g_corpseRemovalTime.integer*1000 + 2500)
	{
	//if ( level.time - ent->timestamp > BODY_SINK_TIME + 2500 ) {

		//removed body que code.  replaced with dynamic body creation.
		//for now we need to manually call the G_KillG2Queue to remove the client's corpse
		//instance.
		G_KillG2Queue(ent->s.number);
		G_FreeEntity(ent);
		// the body ques are never actually freed, they are just unlinked
		//trap_UnlinkEntity( ent );
		//ent->physicsObject = qfalse;
	//[/NOBODYQUE]
		return;	
	}
//	ent->nextthink = level.time + 100;
//	ent->s.pos.trBase[2] -= 1;

	G_AddEvent(ent, EV_BODYFADE, 0);
	ent->nextthink = level.time + 18000;
	ent->takedamage = qfalse;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
static qboolean CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;
	int			islight = 0;

	if (level.intermissiontime)
	{
		return qfalse;
	}

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return qfalse;
	}

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{ //for now, just don't spawn a body if you got disint'd
		return qfalse;
	}

	//[NOBODYQUE]
	body = G_Spawn();

	body->classname = "body";

	// grab a body que and cycle to the next one
	/*
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;
	*/
	//[/NOBODYQUE]

	trap_UnlinkEntity (body);
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.loopIsSoundset = qfalse;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}

	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		//[NOBODYQUE]
		//actually we shouldn't use have any weapon at all if we died like this.
		body->s.weapon = WP_MELEE;
		//body->s.weapon = WP_BLASTER; //lie to keep from putting a saber on the corpse, because it was thrown at death
		//[/NOBODYQUE]
	}

	//G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.clientNum);
	//Now doing this through a modified version of the rcg reliable command.
	if (ent->client && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
	{
		islight = 1;
	}
	trap_SendServerCommand(-1, va("ircg %i %i %i %i", ent->s.number, body->s.number, body->s.weapon, islight));

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;

	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim;

	body->s.customRGBA[0] = ent->client->ps.customRGBA[0];
	body->s.customRGBA[1] = ent->client->ps.customRGBA[1];
	body->s.customRGBA[2] = ent->client->ps.customRGBA[2];
	body->s.customRGBA[3] = ent->client->ps.customRGBA[3];

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	//[NOBODYQUE]
	if(g_corpseRemovalTime.integer)
	{
		body->nextthink = level.time + g_corpseRemovalTime.integer*1000;
		//body->nextthink = level.time + BODY_SINK_TIME;

		body->think = BodySink;
	}
	//[/NOBODYQUE]

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);

	return qtrue;
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

void MaintainBodyQueue(gentity_t *ent)
{ //do whatever should be done taking ragdoll and dismemberment states into account.
	qboolean doRCG = qfalse;

	assert(ent && ent->client);
	if (ent->client->tempSpectate > level.time ||
		(ent->client->ps.eFlags2 & EF2_SHIP_DEATH))
	{
		ent->client->noCorpse = qtrue;
	}

	if (!ent->client->noCorpse && !ent->client->ps.fallingToDeath)
	{//racc - we're suppose to have a corpse so attempt to create one.
		if (!CopyToBodyQue (ent))
		{//racc - no luck, but still remember to restore all the limb damage on the player
			doRCG = qtrue;
		}
	}
	else
	{
		ent->client->noCorpse = qfalse; //clear it for next time
		ent->client->ps.fallingToDeath = qfalse;
		doRCG = qtrue;
	}

	if (doRCG)
	{ //bodyque func didn't manage to call ircg so call this to assure our limbs and ragdoll states are proper on the client.
		trap_SendServerCommand(-1, va("rcg %i", ent->s.clientNum));
	}
}

/*
================
respawn
================
*/
void SiegeRespawn(gentity_t *ent);
void respawn( gentity_t *ent ) {
	MaintainBodyQueue(ent);

	if (gEscaping || g_gametype.integer == GT_POWERDUEL)
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		ent->client->sess.spectatorTime = level.time;
		ClientSpawn(ent);
		ent->client->iAmALoser = qtrue;
		return;
	}

	trap_UnlinkEntity (ent);

	//[LastManStanding]
	if (ojp_lms.integer > 0 && ent->lives < 1 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers())
	{//playing LMS and we're DEAD!  Just start chillin in tempSpec.
		OJP_Spectator(ent);
	}

	else if (g_gametype.integer == GT_SIEGE)
	//if (g_gametype.integer == GT_SIEGE)
	//[/LastManStanding]
	{
		if (g_siegeRespawn.integer)
		{
			if (ent->client->tempSpectate <= level.time)
			{
				int minDel = g_siegeRespawn.integer* 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				ent->client->tempSpectate = level.time + minDel;
				ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
				ent->client->ps.weapon = WP_NONE;
				ent->client->ps.stats[STAT_WEAPONS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				ent->takedamage = qfalse;
				trap_LinkEntity(ent);

				// Respawn time.
				if ( ent->s.number < MAX_CLIENTS )
				{
					gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
					te->s.time = g_siegeRespawnCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		}
		SiegeRespawn(ent);
	}
	else
	{
		gentity_t	*tent;

		if (ojp_ffaRespawnTimer.integer)
		{
			if (ent->client->tempSpectate <= level.time)
			{
				int minDel = g_siegeRespawn.integer* 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				OJP_Spectator(ent);
				ent->client->tempSpectate = level.time + minDel;

				// Respawn time.
				if ( ent->s.number < MAX_CLIENTS )
				{
					gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
					te->s.time = ojp_ffaRespawnTimerCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		ClientSpawn(ent);
		}
		else
		{
		// add a teleportation effect
		ClientSpawn(ent);		
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
		}
		//[LastManStanding]
		if ( ojp_lms.integer > 0 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers())
		{//reduce our number of lives since we respawned and we're not the only player.
			ent->lives--;
		}
		//[/LastManStanding]
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
		else if (g_gametype.integer == GT_SIEGE &&
            level.clients[i].sess.siegeDesiredTeam == team)
		{
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
//[AdminSys]
int G_CountHumanPlayers( int ignoreClientNum, int team );
team_t PickTeam( int ignoreClientNum, qboolean isBot ) {
//team_t PickTeam( int ignoreClientNum ) {
//[/AdminSys]
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	//[AdminSys]
	if(g_teamForceBalance.integer == 4 && !isBot)
	{//human is autojoining the game, place them on the team with the fewest humans.  
		//This will make things work out in the end. :)
		int humanCount[TEAM_NUM_TEAMS];
		humanCount[TEAM_RED] = G_CountHumanPlayers( ignoreClientNum, TEAM_RED );
		humanCount[TEAM_BLUE] = G_CountHumanPlayers( ignoreClientNum, TEAM_BLUE );
		if( humanCount[TEAM_RED] > humanCount[TEAM_BLUE] )
		{
			return TEAM_BLUE;
		}

		if( humanCount[TEAM_BLUE] > humanCount[TEAM_RED] )
		{
			return TEAM_RED;
		}
	}
	//[/AdminSys]

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int		len, colorlessLen;
	char	ch;
	char	*p;
	int		spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}

		// don't allow leading spaces
		if( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if( !*in ) {
				break;
			}

			// don't allow black in a name, period
			/*
			if( ColorIndex(*in) == 0 ) {
				in++;
				continue;
			}
			*/

			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if( ch == ' ' ) {
			spaces++;
			if( spaces > 3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "Padawan", outSize );
	}
}

#ifdef _DEBUG
void G_DebugWrite(const char *path, const char *text)
{
	fileHandle_t f;

	trap_FS_FOpenFile( path, &f, FS_APPEND );
	trap_FS_Write(text, strlen(text), f);
	trap_FS_FCloseFile(f);
}
#endif

qboolean G_SaberModelSetup(gentity_t *ent)
{
	int i = 0;
	qboolean fallbackForSaber = qtrue;

	while (i < MAX_SABERS)
	{
		if (ent->client->saber[i].model[0])
		{
			//first kill it off if we've already got it
			if (ent->client->weaponGhoul2[i])
			{
				trap_G2API_CleanGhoul2Models(&(ent->client->weaponGhoul2[i]));
			}
			trap_G2API_InitGhoul2Model(&ent->client->weaponGhoul2[i], ent->client->saber[i].model, 0, 0, -20, 0, 0);

			if (ent->client->weaponGhoul2[i])
			{
				int j = 0;
				char *tagName;
				int tagBolt;

				if (ent->client->saber[i].skin)
				{
					trap_G2API_SetSkin(ent->client->weaponGhoul2[i], 0, ent->client->saber[i].skin, ent->client->saber[i].skin);
				}

				if (ent->client->saber[i].saberFlags & SFL_BOLT_TO_WRIST)
				{
					trap_G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, 3+i);
				}
				else
				{ // bolt to right hand for 0, or left hand for 1
					trap_G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, i);
				}

				//Add all the bolt points
				while (j < ent->client->saber[i].numBlades)
				{
					tagName = va("*blade%i", j+1);
					tagBolt = trap_G2API_AddBolt(ent->client->weaponGhoul2[i], 0, tagName);

					if (tagBolt == -1)
					{
						if (j == 0)
						{ //guess this is an 0ldsk3wl saber
							tagBolt = trap_G2API_AddBolt(ent->client->weaponGhoul2[i], 0, "*flash");
							fallbackForSaber = qfalse;
							break;
						}

						if (tagBolt == -1)
						{
							assert(0);
							break;

						}
					}
					j++;

					fallbackForSaber = qfalse; //got at least one custom saber so don't need default
				}

				//Copy it into the main instance
				trap_G2API_CopySpecificGhoul2Model(ent->client->weaponGhoul2[i], 0, ent->ghoul2, i+1); 
			}
		}
		else
		{
			break;
		}

		i++;
	}

	return fallbackForSaber;
}

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side 
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally 
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void *g2SaberInstance = NULL;

qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName);
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors );
void BG_GetVehicleModelName(char *modelname);

void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName)
{
	int handle;
	char		afilename[MAX_QPATH];
#if 0
	char		/**GLAName,*/ *slash;
#endif
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
	}

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		int defSkin;

		Com_sprintf( afilename, sizeof( afilename ), "models/players/" DEFAULT_MODEL "/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}

		defSkin = trap_R_RegisterSkin("models/players/" DEFAULT_MODEL "/model_default.skin");
		trap_G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
	}

	if (precachedKyle && trap_G2_HaveWeGhoul2Models(precachedKyle))
	{
		if (d_perPlayerGhoul2.integer || ent->s.number >= MAX_CLIENTS ||
			G_PlayerHasCustomSkeleton(ent))
		{ //rww - allow option for perplayer models on server for collision and bolt stuff.
			char modelFullPath[MAX_QPATH];
			char truncModelName[MAX_QPATH];
			char skin[MAX_QPATH];
			char vehicleName[MAX_QPATH];
			int skinHandle = 0;
			int i = 0;
			char *p;

			// If this is a vehicle, get it's model name.
			if ( ent->client->NPC_class == CLASS_VEHICLE )
			{
				strcpy(vehicleName, modelname);
				BG_GetVehicleModelName(modelname);
				strcpy(truncModelName, modelname);
				skin[0] = 0;
				if ( ent->m_pVehicle
					&& ent->m_pVehicle->m_pVehicleInfo
					&& ent->m_pVehicle->m_pVehicleInfo->skin
					&& ent->m_pVehicle->m_pVehicleInfo->skin[0] )
				{
					skinHandle = trap_R_RegisterSkin(va("models/players/%s/model_%s.skin", modelname, ent->m_pVehicle->m_pVehicleInfo->skin));
				}
				else
				{
					skinHandle = trap_R_RegisterSkin(va("models/players/%s/model_default.skin", modelname));
				}
			}
			else
			{
				if (skinName && skinName[0])
				{
					strcpy(skin, skinName);
					strcpy(truncModelName, modelname);
				}
				else
				{
					strcpy(skin, "default");

					strcpy(truncModelName, modelname);
					p = Q_strrchr(truncModelName, '/');

					if (p)
					{
						*p = 0;
						p++;

						while (p && *p)
						{
							skin[i] = *p;
							i++;
							p++;
						}
						skin[i] = 0;
						i = 0;
					}

					if (!BG_IsValidCharacterModel(truncModelName, skin))
					{
						strcpy(truncModelName, DEFAULT_MODEL);
						strcpy(skin, "default");
					}
					

					
					if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_SIEGE && !g_trueJedi.integer )
					{
						//[BugFix32]
						//Also adjust customRGBA for team colors.
						float colorOverride[3];

						colorOverride[0] = colorOverride[1] = colorOverride[2] = 0.0f;

						BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, colorOverride);
						if (colorOverride[0] != 0.0f ||
							colorOverride[1] != 0.0f ||
							colorOverride[2] != 0.0f)
						{
							ent->client->ps.customRGBA[0] = colorOverride[0]*255.0f;
							ent->client->ps.customRGBA[1] = colorOverride[1]*255.0f;
							ent->client->ps.customRGBA[2] = colorOverride[2]*255.0f;
						}

						//BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, NULL );
						//[/BugFix32]
					}
					else if (g_gametype.integer == GT_SIEGE)
					{ //force skin for class if appropriate
						if (ent->client->siegeClass != -1)
						{
							siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];
							if (scl->forcedSkin[0])
							{
								strcpy(skin, scl->forcedSkin);
							}
						}
					}
				}
			}

			if (skin[0])
			{
				char *useSkinName;

				if (strchr(skin, '|'))
				{//three part skin
					useSkinName = va("models/players/%s/|%s", truncModelName, skin);
				}
				else
				{
					useSkinName = va("models/players/%s/model_%s.skin", truncModelName, skin);
				}

				skinHandle = trap_R_RegisterSkin(useSkinName);
			}

			strcpy(modelFullPath, va("models/players/%s/model.glm", truncModelName));
			handle = trap_G2API_InitGhoul2Model(&ent->ghoul2, modelFullPath, 0, skinHandle, -20, 0, 0);

			if (handle<0)
			{ //Huh. Guess we don't have this model. Use the default.

				if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
				{
					trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
				}
				ent->ghoul2 = NULL;
				trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
			}
			else
			{
				trap_G2API_SetSkin(ent->ghoul2, 0, skinHandle, skinHandle);

				GLAName[0] = 0;
				trap_G2API_GetGLAName( ent->ghoul2, 0, GLAName);

				if (!GLAName[0] || (!strstr(GLAName, "players/_humanoid/") && ent->s.number < MAX_CLIENTS && !G_PlayerHasCustomSkeleton(ent)))
				{ //a bad model
					trap_G2API_CleanGhoul2Models(&(ent->ghoul2));
					ent->ghoul2 = NULL;
					trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
				}

				if (ent->s.number >= MAX_CLIENTS)
				{
					ent->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

					if (skin[0])
					{ //append it after a *
						strcat( modelFullPath, va("*%s", skin) );
					}

					if ( ent->client->NPC_class == CLASS_VEHICLE )
					{ //vehicles are tricky and send over their vehicle names as the model (the model is then retrieved based on the vehicle name)
						ent->s.modelindex = G_ModelIndex(vehicleName);
					}
					else
					{
						ent->s.modelindex = G_ModelIndex(modelFullPath);
					}
				}
			}
		}
		else
		{
			trap_G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
		}
	}
	else
	{
		return;
	}

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap_G2API_AttachInstanceToEntNum(ent->ghoul2, ent->s.number, qtrue);

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		if (BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf( "Failed to load humanoid animation file\n");
			return;
		}
	}

	if (ent->s.number >= MAX_CLIENTS || G_PlayerHasCustomSkeleton(ent))
	{
		ent->localAnimIndex = -1;

		GLAName[0] = 0;
		trap_G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (GLAName[0] &&
			!strstr(GLAName, "players/_humanoid/") /*&&
			!strstr(GLAName, "players/rockettrooper/")*/)
		{ //it doesn't use humanoid anims.
			char *slash = Q_strrchr( GLAName, '/' );
			if ( slash )
			{
				strcpy(slash, "/animation.cfg");

				ent->localAnimIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
			}
		}
		else
		{ //humanoid index.
			if (strstr(GLAName, "players/rockettrooper/"))
			{
				ent->localAnimIndex = 1;
			}
			else
			{
				ent->localAnimIndex = 0;
			}
		}

		if (ent->localAnimIndex == -1)
		{
			Com_Error(ERR_DROP, "NPC had an invalid GLA\n");
		}
	}
	else
	{
		GLAName[0] = 0;
		trap_G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (strstr(GLAName, "players/rockettrooper/"))
		{
			//assert(!"Should not have gotten in here with rockettrooper skel");
			ent->localAnimIndex = 1;
		}
		else
		{
			ent->localAnimIndex = 0;
		}
	}

	if (ent->s.NPC_class == CLASS_VEHICLE &&
		ent->m_pVehicle)
	{ //do special vehicle stuff
		char strTemp[128];
		int i;

		// Setup the default first bolt
		i = trap_G2API_AddBolt( ent->ghoul2, 0, "model_root" );

		// Setup the droid unit.
		ent->m_pVehicle->m_iDroidUnitTag = trap_G2API_AddBolt( ent->ghoul2, 0, "*droidunit" );

		// Setup the Exhausts.
		for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
		{
			Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
			ent->m_pVehicle->m_iExhaustTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
		}

		// Setup the Muzzles.
		for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
		{
			Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
			ent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
			if ( ent->m_pVehicle->m_iMuzzleTag[i] == -1 )
			{//ergh, try *flash?
				Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
				ent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, strTemp );
			}
		}

		// Setup the Turrets.
		for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
		{
			if ( ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag )
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = trap_G2API_AddBolt( ent->ghoul2, 0, ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
			}
			else
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = -1;
			}
		}
	}
	
	if (ent->client->ps.weapon == WP_SABER || ent->s.number < MAX_CLIENTS)
	{ //a player or NPC saber user
		trap_G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
		trap_G2API_AddBolt(ent->ghoul2, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap_G2API_AddBolt(ent->ghoul2, 0, "*chestg");

		//claw bolts
		trap_G2API_AddBolt(ent->ghoul2, 0, "*r_hand_cap_r_arm");
		trap_G2API_AddBolt(ent->ghoul2, 0, "*l_hand_cap_l_arm");

		trap_G2API_SetBoneAnim(ent->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
		trap_G2API_SetBoneAngles(ent->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
		trap_G2API_SetBoneAngles(ent->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time);

		if (!g2SaberInstance)
		{
			trap_G2API_InitGhoul2Model(&g2SaberInstance, DEFAULT_SABER_MODEL, 0, 0, -20, 0, 0);

			if (g2SaberInstance)
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
				// now set up the gun bolt on it
				trap_G2API_AddBolt(g2SaberInstance, 0, "*blade1");
			}
		}

		if (G_SaberModelSetup(ent))
		{
			if (g2SaberInstance)
			{
				trap_G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, ent->ghoul2, 1); 
			}
		}
	}

	if (ent->s.number >= MAX_CLIENTS)
	{ //some extra NPC stuff
		if (trap_G2API_AddBolt(ent->ghoul2, 0, "lower_lumbar") == -1)
		{ //check now to see if we have this bone for setting anims and such
			ent->noLumbar = qtrue;
		}

		//[CoOp]
		//add some extra bolts for some of the NPC classes
		if ( ent->client->NPC_class == CLASS_HOWLER )
		{
			ent->NPC->genericBolt1 = trap_G2API_AddBolt(&ent->ghoul2, 0, "Tongue01" );// tongue base
			ent->NPC->genericBolt2 = trap_G2API_AddBolt(&ent->ghoul2, 0, "Tongue08" );// tongue tip
		}
		//[/CoOp]
	}
}




/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
//[CoOp]
qboolean WinterGear = qfalse;  //sets weither or not the models go for winter gear skins
								//or not.
//[/CoOp]
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride);
void G_ValidateSiegeClassForTeam(gentity_t *ent, int team);
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	char	*s;
	char	model[MAX_QPATH];
	//char	headModel[MAX_QPATH];
	//[GameTweaks]
	//not used.
	//char	forcePowers[MAX_QPATH];
	//[/GameTweaks]
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
//	char	redTeam[MAX_INFO_STRING];
//	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
	char	className[MAX_QPATH]; //name of class type to use in siege
	char	saberName[MAX_QPATH];
	char	saber2Name[MAX_QPATH];
	char	*value;
	int		maxHealth;

	//[RGBSabers]
	char	rgb1[MAX_INFO_STRING];
	char	rgb2[MAX_INFO_STRING];
	char	script1[MAX_INFO_STRING];
	char	script2[MAX_INFO_STRING];
	//[/RGBSabers]

	qboolean	modelChanged = qfalse, female = qfalse;

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) 
		{
			if ( client->pers.netnameTime > level.time  )
			{
				trap_SendServerCommand( clientNum, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NONAMECHANGE")) );

				Info_SetValueForKey( userinfo, "name", oldname );
				trap_SetUserinfo( clientNum, userinfo );			
				strcpy ( client->pers.netname, oldname );
			}
			else
			{				
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname, G_GetStringEdString("MP_SVGAME", "PLRENAME"), client->pers.netname) );
				client->pers.netnameTime = level.time + 5000;
			}
		}
	}

	// set model
	Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );

	//[CoOp]
	if (WinterGear)
	{//use winter gear
		char *skinname = strstr ( model, "|" );
		if(skinname)
		{//we're using a species player model, try to use their hoth clothes.
			skinname++;
			strcpy( skinname, "torso_g1|lower_e1\0" );
		}
	}
	//[/CoOp]

	if (d_perPlayerGhoul2.integer)
	{
		if (Q_stricmp(model, client->modelname))
		{
			strcpy(client->modelname, model);
			modelChanged = qtrue;
		}
	}

	//Get the skin RGB based on his userinfo
	value = Info_ValueForKey (userinfo, "char_color_red");
	if (value)
	{
		client->ps.customRGBA[0] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[0] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_green");
	if (value)
	{
		client->ps.customRGBA[1] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[1] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_blue");
	if (value)
	{
		client->ps.customRGBA[2] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[2] = 255;
	}

	if ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100)
	{ //hmm, too dark!
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	}

	client->ps.customRGBA[3]=255;

	//[BugFix32]
	//update our customRGBA for team colors. 
	if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_SIEGE && !g_trueJedi.integer )
	{
		char skin[MAX_QPATH];
		float colorOverride[3];

		colorOverride[0] = colorOverride[1] = colorOverride[2] = 0.0f;

		BG_ValidateSkinForTeam( model, skin, client->sess.sessionTeam, colorOverride);
		if (colorOverride[0] != 0.0f ||
			colorOverride[1] != 0.0f ||
			colorOverride[2] != 0.0f)
		{
			client->ps.customRGBA[0] = colorOverride[0]*255.0f;
			client->ps.customRGBA[1] = colorOverride[1]*255.0f;
			client->ps.customRGBA[2] = colorOverride[2]*255.0f;
		}
	}
	//[/BugFix32]

	//[GameTweaks]
	//not used.
	//Q_strncpyz( forcePowers, Info_ValueForKey (userinfo, "forcepowers"), sizeof( forcePowers ) );
	//[/GameTweaks]

	//[BugFix14]
	//Testing to see if this fixes the problem with a bot's team getting set incorrectly.
	team = client->sess.sessionTeam;
	/* basejka code
	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
	}
	else {
		team = client->sess.sessionTeam;
	}
	*/
	//[/BugFix14]

	//Set the siege class
	if (g_gametype.integer == GT_SIEGE)
	{
		strcpy(className, client->sess.siegeClass);

		//This function will see if the given class is legal for the given team.
		//If not className will be filled in with the first legal class for this team.
/*		if (!BG_SiegeCheckClassLegality(team, className) &&
			Q_stricmp(client->sess.siegeClass, "none"))
		{ //if it isn't legal pop up the class menu
			trap_SendServerCommand(ent-g_entities, "scl");
		}
*/
		//Now that the team is legal for sure, we'll go ahead and get an index for it.
		client->siegeClass = BG_SiegeFindClassIndexByName(className);
		if (client->siegeClass == -1)
		{ //ok, get the first valid class for the team you're on then, I guess.
			BG_SiegeCheckClassLegality(team, className);
			strcpy(client->sess.siegeClass, className);
			client->siegeClass = BG_SiegeFindClassIndexByName(className);
		}
		else
		{ //otherwise, make sure the class we are using is legal.
			G_ValidateSiegeClassForTeam(ent, team);
			strcpy(className, client->sess.siegeClass);
		}

		//Set the sabers if the class dictates
		if (client->siegeClass != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

			if (scl->saber1[0])
			{
				G_SetSaber(ent, 0, scl->saber1, qtrue);
			}
			else
			{ //default I guess
				G_SetSaber(ent, 0, DEFAULT_SABER, qtrue);
			}
			if (scl->saber2[0])
			{
				G_SetSaber(ent, 1, scl->saber2, qtrue);
			}
			else
			{ //no second saber then
				G_SetSaber(ent, 1, "none", qtrue);
			}

			//make sure the saber models are updated
			G_SaberModelSetup(ent);

			if (scl->forcedModel[0])
			{ //be sure to override the model we actually use
				strcpy(model, scl->forcedModel);
				if (d_perPlayerGhoul2.integer)
				{
					if (Q_stricmp(model, client->modelname))
					{
						strcpy(client->modelname, model);
						modelChanged = qtrue;
					}
				}
			}

			//force them to use their class model on the server, if the class dictates
			if (G_PlayerHasCustomSkeleton(ent))
			{
				if (Q_stricmp(model, client->modelname) || ent->localAnimIndex == 0)
				{
					strcpy(client->modelname, model);
					modelChanged = qtrue;
				}
			}
		}
	}
	else
	{
		strcpy(className, "none");
	}

	//Set the saber name
	strcpy(saberName, client->sess.saberType);
	strcpy(saber2Name, client->sess.saber2Type);

	// set max health


	if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_3)
		{
		maxHealth = 999;
		}
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_2)
		{
		maxHealth = 500;
		}	
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_1)
		{
		maxHealth = 250;
		}
	else 
		{
			if (g_gametype.integer == GT_SIEGE && client->siegeClass != -1)
			{
				siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
				maxHealth = 100;

				if (scl->maxhealth)
				{
				maxHealth = scl->maxhealth;
				}


			}
			else
			{
			maxHealth = 100;
			}
		}
		health = maxHealth;

	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) 
	{
  	if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_3)
		{
		client->pers.maxHealth = 999;
		}
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_2)
		{
		client->pers.maxHealth = 500;
		}	
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_1)
		{
		client->pers.maxHealth = 250;
		}
	else 
		{
			if (g_gametype.integer == GT_SIEGE && client->siegeClass != -1)
			{
				siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
				client->pers.maxHealth = 100;

				if (scl->maxhealth)
				{
				client->pers.maxHealth = scl->maxhealth;
				}


			}
			else
			{
			client->pers.maxHealth = 100;
			}
		}

	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

/*	NOTE: all client side now

	// team
	switch( team ) {
	case TEAM_RED:
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
		break;
	case TEAM_BLUE:
		ForceClientSkin(client, model, "blue");
//		ForceClientSkin(client, headModel, "blue");
		break;
	}
	// don't ever use a default skin in teamplay, it would just waste memory
	// however bots will always join a team but they spawn in as spectator
	if ( g_gametype.integer >= GT_TEAM && team == TEAM_SPECTATOR) {
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
	}
*/

	if (g_gametype.integer >= GT_TEAM) {
		client->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		} else {
			client->pers.teamInfo = qfalse;
		}
	}
	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if ( !*s || atoi( s ) == 0 ) {
		client->pers.pmoveFixed = qfalse;
	}
	else {
		client->pers.pmoveFixed = qtrue;
	}
	*/

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));

//	strcpy(redTeam, Info_ValueForKey( userinfo, "g_redteam" ));
//	strcpy(blueTeam, Info_ValueForKey( userinfo, "g_blueteam" ));
	//Raz: Gender hints
	s = Info_ValueForKey( userinfo, "sex" );
	if ( !Q_stricmp( s, "female" ) ) 
	{
		female = qtrue;
	}
	//[ClientPlugInDetect]
	s = Info_ValueForKey( userinfo, "ojp_clientplugin" );
	if ( !*s  ) 
	{
		client->pers.ojpClientPlugIn = qfalse;
	}
	else if(!strcmp(CURRENT_OJPENHANCED_CLIENTVERSION, s))
	{
		client->pers.ojpClientPlugIn = qtrue;
	}
	//[/ClientPlugInDetect]

	//[RGBSabers]
	Q_strncpyz(rgb1,Info_ValueForKey(userinfo, "rgb_saber1"), sizeof(rgb1));
	Q_strncpyz(rgb2,Info_ValueForKey(userinfo, "rgb_saber2"), sizeof(rgb2));

	Q_strncpyz(script1,Info_ValueForKey(userinfo, "rgb_script1"), sizeof(script1));
	Q_strncpyz(script2,Info_ValueForKey(userinfo, "rgb_script2"), sizeof(script2));


//	Com_Printf("game > newinfo update > sab1 \"%s\" sab2 \"%s\" \n",rgb1,rgb2);

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	//[RGBSabers]
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d\\siegeclass\\%s\\st\\%s\\st2\\%s\\dt\\%i\\sdt\\%i\\tc1\\%s\\tc2\\%s\\ss1\\%s\\ss2\\%s",
			client->pers.netname, team, model,  c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader, className, saberName, saber2Name, client->sess.duelTeam, client->sess.siegeDesiredTeam,
			rgb1,rgb2,script1,script2);
	} else {
		if (g_gametype.integer == GT_SIEGE)
		{ //more crap to send
			s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\siegeclass\\%s\\st\\%s\\st2\\%s\\dt\\%i\\sdt\\%i\\tc1\\%s\\tc2\\%s\\ss1\\%s\\ss2\\%s",
				client->pers.netname, client->sess.sessionTeam, model, c1, c2, 
				client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, className, saberName, saber2Name, client->sess.duelTeam, client->sess.siegeDesiredTeam,rgb1,rgb2,script1,script2);
		}
		else
		{
			s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\st\\%s\\st2\\%s\\dt\\%i\\tc1\\%s\\tc2\\%s\\ss1\\%s\\ss2\\%s",
				client->pers.netname, client->sess.sessionTeam, model, c1, c2, 
				client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, saberName, saber2Name, client->sess.duelTeam,rgb1,rgb2,script1,script2);
	//[/RGBSabers]
		}
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	if (modelChanged) //only going to be true for allowable server-side custom skeleton cases
	{ //update the server g2 instance if appropriate
		char *modelname = Info_ValueForKey (userinfo, "model");
		SetupGameGhoul2Model(ent, modelname, NULL);

		if (ent->ghoul2 && ent->client)
		{
			ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
		}

		client->torsoAnimExecute = client->legsAnimExecute = -1;
		client->torsoLastFlip = client->legsLastFlip = qfalse;
	}

	if (g_logClientInfo.integer)
	{
		G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
	}
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	//[AdminSys]
	char		IPstring[32]={0};
	//[/AdminSys]
	gentity_t	*ent;
	gentity_t	*te;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	//[LastManStanding]
	if(ojp_lms.integer > 0 && BG_IsLMSGametype(g_gametype.integer) )
	{//LMS mode, set up lives.
		ent->lives = (ojp_lmslives.integer >= 1) ? ojp_lmslives.integer : 1; 
		//[Coop]
		if (g_gametype.integer == GT_SINGLE_PLAYER)
		{//LMS mode, playing Coop, setup liveExp
			ent->liveExp = 0;
		}
		//[/Coop]
	}
	//[/LastManStanding]

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	//[AdminSys]
	Q_strncpyz(IPstring, value, sizeof(IPstring) );
	//[/AdminSys]

	if ( G_FilterPacket( value ) ) {
		return "Banned.";
	}

	//[BugFix11]
	//thanks to ensiform.  SVF_BOT isn't set until later in this function.
	if ( !isBot && g_needpass.integer ) 
	{
	//if ( !( ent->r.svFlags & SVF_BOT ) && !isBot && g_needpass.integer ) {
	//[/BugFix11]
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		//[PrivatePasswordFix]
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) && strcmp( g_password.string, value) != 0) {
			if( !sv_privatepassword.string[0] || strcmp( sv_privatepassword.string, value ) ) {
				static char sTemp[1024];
				Q_strncpyz(sTemp, G_GetStringEdString("MP_SVGAME","INVALID_ESCAPE_TO_MAIN"), sizeof (sTemp) );
				return sTemp;// return "Invalid password";
			}
		}
		//[PrivatePasswordFix]
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	//[ExpSys]
	//initialize the player's entity now so that ent->s.number will be valid.  Otherwise, poor clientNum 0 will always get 
	//spammed whenever someone joins the game.
	G_InitGentity(ent);
	//[/ExpSys]

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		//[ExpSys]
		//pass first time so we know if we need to reset skill levels or not.
		G_InitSessionData( client, userinfo, isBot, firstTime );
		//G_InitSessionData( client, userinfo, isBot );
		//[/ExpSys]
	}
	G_ReadSessionData( client );

	//[AdminSys]
	client->sess.IPstring[0] = 0;
	Q_strncpyz(client->sess.IPstring, IPstring, sizeof(client->sess.IPstring) );
	//[/AdminSys]

	if (g_gametype.integer == GT_SIEGE &&
		(firstTime || level.newSession))
	{ //if this is the first time then auto-assign a desired siege team and show briefing for that team
		client->sess.siegeDesiredTeam = 0;//PickTeam(ent->s.number);
		/*
		trap_SendServerCommand(ent->s.number, va("sb %i", client->sess.siegeDesiredTeam));
		*/
		//don't just show it - they'll see it if they switch to a team on purpose.
	}


	if (g_gametype.integer == GT_SIEGE && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		if (firstTime || level.newSession)
		{ //start as spec
			client->sess.siegeDesiredTeam = client->sess.sessionTeam;
			client->sess.sessionTeam = TEAM_SPECTATOR;
		}
	}
	//[BugFix22]
	//this was boning over all the careful player selection stuff that happened before rounds in powerduel.
	/* basejka code
	else if (g_gametype.integer == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		client->sess.sessionTeam = TEAM_SPECTATOR;
	}
	*/
	//[/BugFix22]

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		//[Linux]
		//if( !G_BotConnect( clientNum, !firstTime ) ) {
		if( !G_BotConnect( clientNum, (qboolean) (!firstTime) ) ) {
		//[/Linux]
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );
	//[AdminSys]
	if( !isBot ){// MJN - bots don't have IP's ;)
		G_LogPrintf( "%s" S_COLOR_WHITE " connected with IP: %s\n", client->pers.netname, client->sess.IPstring );
	}
	else{// MJN - We'll say this instead.
		G_LogPrintf( "*****Spawning Bot %s" S_COLOR_WHITE "***** \n", client->pers.netname );
	}
	//G_LogPrintf(  "%s" S_COLOR_WHITE " connected with IP: %s\n", client->pers.netname, client->sess.IPstring );
	//[/AdminSys]

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCONNECT")) );
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();


	te = G_TempEntity( vec3_origin, EV_CLIENTJOIN );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = clientNum;

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );



	return NULL;
}

void G_WriteClientSessionData( gclient_t *client );

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags, i;
	//[BugFix48]
	int			spawnCount;
	//[/BugFix48]
	char		userinfo[MAX_INFO_VALUE], *modelname;

	//[ExpandedMOTD]
	//contains the message of the day that is sent to new players.
	char		motd[1024];  

	ent = g_entities + clientNum;

	if ((ent->r.svFlags & SVF_BOT) && g_gametype.integer >= GT_TEAM)
	{
		if (allowTeamReset)
		{
			const char *team = "Red";
			int preSess;

			//SetTeam(ent, "");
			//[AdminSys]
			ent->client->sess.sessionTeam = PickTeam(-1, qtrue);
			//ent->client->sess.sessionTeam = PickTeam(-1);
			//[/AdminSys]
			trap_GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				ent->client->sess.sessionTeam = TEAM_RED;
			}

			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				team = "Red";
			}
			else
			{
				team = "Blue";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap_SetUserinfo( clientNum, userinfo );

			ent->client->ps.persistant[ PERS_TEAM ] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData( ent->client );
			ent->client->sess.sessionTeam = preSess;
			G_WriteClientSessionData(ent->client);
			ClientUserinfoChanged( clientNum );
			ClientBegin(clientNum, qfalse);
			return;
		}
	}

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	//[BugFix48]
	spawnCount = client->ps.persistant[PERS_SPAWN_COUNT];
	//[/BugFix48]

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	//[BugFix48]
	client->ps.persistant[PERS_SPAWN_COUNT] = spawnCount;
	//[/BugFix48]

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	//init saber ent
	WP_SaberInitBladeData( ent );

	// First time model setup for that player.
	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	modelname = Info_ValueForKey (userinfo, "model");
	SetupGameGhoul2Model(ent, modelname, NULL);

	if (ent->ghoul2 && ent->client)
	{
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.
	}

	if (g_gametype.integer == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR &&
		client->sess.duelTeam == DUELTEAM_FREE)
	{
		SetTeam(ent, "s");
	}
	else
	{
		if (g_gametype.integer == GT_SIEGE && (!gSiegeRoundBegun || gSiegeRoundEnded))
		{
			SetTeamQuick(ent, TEAM_SPECTATOR, qfalse);
		}
        
		if ((ent->r.svFlags & SVF_BOT) &&
			g_gametype.integer != GT_SIEGE)
		{
			char *saberVal = Info_ValueForKey(userinfo, "saber1");
			char *saber2Val = Info_ValueForKey(userinfo, "saber2");

			if (!saberVal || !saberVal[0])
			{ //blah, set em up with a random saber
				int r = rand()%50;
				char sab1[1024];
				char sab2[1024];

				if (r <= 17)
				{
					strcpy(sab1, "Katarn");
					strcpy(sab2, "none");
				}
				else if (r <= 34)
				{
					strcpy(sab1, "Katarn");
					strcpy(sab2, "Katarn");
				}
				else
				{
					strcpy(sab1, "dual_1");
					strcpy(sab2, "none");
				}
				G_SetSaber(ent, 0, sab1, qfalse);
				G_SetSaber(ent, 0, sab2, qfalse);
				Info_SetValueForKey( userinfo, "saber1", sab1 );
				Info_SetValueForKey( userinfo, "saber2", sab2 );
				trap_SetUserinfo( clientNum, userinfo );
			}
			else
			{
				G_SetSaber(ent, 0, saberVal, qfalse);
			}

			if (saberVal && saberVal[0] &&
				(!saber2Val || !saber2Val[0]))
			{
				G_SetSaber(ent, 0, "none", qfalse);
				Info_SetValueForKey( userinfo, "saber2", "none" );
				trap_SetUserinfo( clientNum, userinfo );
			}
			else
			{
				G_SetSaber(ent, 0, saber2Val, qfalse);
			}
		}

		// locate ent at a spawn point
		//[LastManStanding]
		if (ojp_lms.integer > 0 && ent->lives < 1 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers() && client->sess.sessionTeam != TEAM_SPECTATOR)
		{//don't allow players to respawn in LMS by switching teams.
			OJP_Spectator(ent);
		}
		else
		{
			ClientSpawn(ent);
			if(client->sess.sessionTeam != TEAM_SPECTATOR && LMS_EnoughPlayers())
			{//costs a life to switch teams to something other than spectator.
				ent->lives--;
			}
		}
		//[/LastManStanding]
	}

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// send event
		//[LastManStanding]
		if (ojp_lms.integer <= 0 || ent->lives >= 1 || !BG_IsLMSGametype(g_gametype.integer))
		{//don't do the "teleport in" effect if we're playing LMS and we're "out"
			tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
			tent->s.clientNum = ent->s.clientNum;
		}
		//tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		//tent->s.clientNum = ent->s.clientNum;
		//[/LastManStanding]

		if ( g_gametype.integer != GT_DUEL || g_gametype.integer == GT_POWERDUEL ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLENTER")) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	//[ExpandedMOTD]
	//prepare and send MOTD message to new client.
	if(client->pers.ojpClientPlugIn)
	{//send this client the MOTD for clients using the right version of OJP.
		TextWrapCenterPrint(ojp_clientMOTD.string, motd);
	}
	else
	{//send this client the MOTD for clients aren't running OJP or just not the right version.
		TextWrapCenterPrint(ojp_MOTD.string, motd);
	}

	trap_SendServerCommand( clientNum, va("cp \"%s\n\"", motd ) );
	//[/ExpandedMOTD]

	// count current clients and rank for scoreboard
	CalculateRanks();

	G_ClearClientLog(clientNum);
}

//[MOREFORCEOPTIONS]
qboolean AllForceDisabled(int force)
//static qboolean AllForceDisabled(int force)
//[/MOREFORCEOPTIONS]
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

//Convenient interface to set all my limb breakage stuff up -rww
void G_BreakArm(gentity_t *ent, int arm)
{
	int anim = -1;

	assert(ent && ent->client);

	if (ent->s.NPC_class == CLASS_VEHICLE || ent->localAnimIndex > 1)
	{ //no broken limbs for vehicles and non-humanoids
		return;
	}

	if (!arm)
	{ //repair him
		ent->client->ps.brokenLimbs = 0;
		return;
	}

	if (ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{ //I'm too lazy to deal with this as well for now.
		return;
	}

	if (arm == BROKENLIMB_LARM)
	{
		if (ent->client->saber[1].model[0] &&
			ent->client->ps.weapon == WP_SABER &&
			!ent->client->ps.saberHolstered &&
			ent->client->saber[1].soundOff)
		{ //the left arm shuts off its saber upon being broken
			G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
		}
	}

	ent->client->ps.brokenLimbs = 0; //make sure it's cleared out
	ent->client->ps.brokenLimbs |= (1 << arm); //this arm is now marked as broken

	//Do a pain anim based on the side. Since getting your arm broken does tend to hurt.
	if (arm == BROKENLIMB_LARM)
	{
		anim = BOTH_PAIN2;
	}
	else if (arm == BROKENLIMB_RARM)
	{
		anim = BOTH_PAIN3;
	}

	if (anim == -1)
	{
		return;
	}

	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);

	//This could be combined into a single event. But I guess limbs don't break often enough to
	//worry about it.
	G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain25.wav") );
	//FIXME: A nice bone snapping sound instead if possible
	G_Sound(ent, CHAN_AUTO, G_SoundIndex( va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3)) ));
}

//Update the ghoul2 instance anims based on the playerstate values
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
//[SaberLockSys]
extern stringID_table_t animTable [MAX_ANIMATIONS+1];
//[/SaberLockSys]
void G_UpdateClientAnims(gentity_t *self, float animSpeedScale)
{
	static int f;
	static int torsoAnim;
	static int legsAnim;
	static int firstFrame, lastFrame;
	static int aFlags;
	static float animSpeed, lAnimSpeedScale;
	qboolean setTorso = qfalse;

	torsoAnim = (self->client->ps.torsoAnim);
	legsAnim = (self->client->ps.legsAnim);

	if (self->client->ps.saberLockFrame)
	{
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "model_root", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "Motion", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		return;
	}

	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames == 0)
	{ //We'll allow this for non-humanoids.
		goto tryTorso;
	}

	if (self->client->legsAnimExecute != legsAnim || self->client->legsLastFlip != self->client->ps.legsFlip)
	{
		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[legsAnim].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[legsAnim].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on server position, but it's here just for the sake of matching them.

		//[NewGLA]
		//G2API_SetAnimIndex(self->ghoul2, bgAllAnims[self->localAnimIndex].anims[legsAnim].glaIndex);
		//[/NewGLA]
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "model_root", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
		self->client->legsAnimExecute = legsAnim;
		self->client->legsLastFlip = self->client->ps.legsFlip;
	}

tryTorso:
	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].numFrames == 0)

	{ //If this fails as well just return.
		return;
	}
	else if (self->s.number >= MAX_CLIENTS &&
		self->s.NPC_class == CLASS_VEHICLE)
	{ //we only want to set the root bone for vehicles
		return;
	}

	if ((self->client->torsoAnimExecute != torsoAnim || self->client->torsoLastFlip != self->client->ps.torsoFlip) &&
		!self->noLumbar)
	{
		aFlags = 0;
		animSpeed = 0;

		f = torsoAnim;

		//[FatigueSys]
		BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, f, &animSpeedScale, self->client->ps.brokenLimbs, self->client->ps.userInt3);
		//BG_SaberStartTransAnim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, f, &animSpeedScale, self->client->ps.brokenLimbs);
		//[/FatigueSys]

		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[f].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[f].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on client position, but it's here just for the sake of matching them.

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}

		trap_G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, /*firstFrame why was it this before?*/-1, 150);

		self->client->torsoAnimExecute = torsoAnim;
		self->client->torsoLastFlip = self->client->ps.torsoFlip;
		
		setTorso = qtrue;
		
		//[SaberLockSys]
		//G_Printf("%i: %i: Server Started Torso Animation %s\n", level.time, self->s.number, GetStringForID(animTable, torsoAnim) );
		//[/SaberLockSys]
	}

	if (setTorso &&
		self->localAnimIndex <= 1)
	{ //only set the motion bone for humanoids.
		trap_G2API_SetBoneAnim(self->ghoul2, 0, "Motion", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
	}

#if 0 //disabled for now
	if (self->client->ps.brokenLimbs != self->client->brokenLimbs ||
		setTorso)
	{
		if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_DEAD21 ];
			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

			trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
		}
		else if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//self->client->ps.weapon == WP_MELEE ||
				self->client->ps.weapon != WP_SABER ||
				BG_SaberStanceAnim(self->client->ps.torsoAnim) ||
				PM_RunningAnim(self->client->ps.torsoAnim)) &&
				(!self->client->saber[1].model[0] || self->client->ps.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (self->client->ps.weapon == WP_MELEE ||
					self->client->ps.weapon == WP_SABER ||
					self->client->ps.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}

				if (self->client->ps.torsoAnim != BOTH_MELEE1 &&
					self->client->ps.torsoAnim != BOTH_MELEE2 &&
					(self->client->ps.torsoAnim == TORSO_WEAPONREADY2 || self->client->ps.torsoAnim == BOTH_ATTACK2 || self->client->ps.weapon < WP_BRYAR_PISTOL)
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}
			}
			else
			{ //otherwise, keep it set to the same as the torso
				trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				trap_G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
			}
		}
		else if (self->client->brokenLimbs)
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (self->client->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (self->client->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap_G2API_SetBoneAnim(self->ghoul2, 0, "lhumerus", 0, 1, 0, 0, level.time, -1, 0);
				trap_G2API_RemoveBone(self->ghoul2, "lhumerus", 0);
			}

			assert(brokenBone);

			//Set the flags and stuff to 0, so that the remove will succeed
			trap_G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, 0, 1, 0, 0, level.time, -1, 0);

			//Now remove it
			trap_G2API_RemoveBone(self->ghoul2, brokenBone, 0);
			self->client->brokenLimbs &= ~broken;
		}
	}
#endif
}


//[ExpSys]
int TotalAllociatedSkillPoints(gentity_t *ent)
{//returns the total number of points the player has allociated to various skills.
	int i, countDown;
	int usedPoints = 0;
	qboolean freeSaber = HasSetSaberOnly();

	for(i = 0; i < NUM_TOTAL_SKILLS; i++)
	{
		if(i < NUM_FORCE_POWERS)
		{//force power
			countDown = ent->client->ps.fd.forcePowerLevel[i];
		}
		else
		{
			countDown = ent->client->skillLevel[i-NUM_FORCE_POWERS];
		}

		while (countDown > 0)
		{
			usedPoints += bgForcePowerCost[i][countDown];
			if ( countDown == 1 &&
				((i == FP_SABER_OFFENSE && freeSaber) ||
				 (i == FP_SABER_DEFENSE && freeSaber)) )
			{
				usedPoints -= bgForcePowerCost[i][countDown];
			}
			countDown--;
		}
	}

	return usedPoints;
}
//[/ExpSys]

extern int ClipSize(int ammo,gentity_t *pog);//[Reload]

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
void player_touch(gentity_t *self, gentity_t *other, trace_t *trace )
{
	if(!other)
		return;

	if(!other->client)
		return;


	if(other->client->pushEffectTime > level.time
		|| other->client->ps.fd.forceGripBeingGripped > level.time)
	{//Other player was pushed!
		float speed = (vec_t)sqrt (other->client->ps.velocity[0]*
			other->client->ps.velocity[0] + other->client->ps.velocity[1]*
			other->client->ps.velocity[1])/2;
		//G_Printf("Speed %f\n",speed);
		if(speed > 50)
		{
			int damage = (speed >= 100 ? 35 : 10);
			gentity_t *gripper = NULL;
			int i=0;

			G_Knockdown(self,other,other->client->ps.velocity,100,qfalse);
			self->client->ps.velocity[1] = other->client->ps.velocity[1]*5.5f;
			self->client->ps.velocity[0] = other->client->ps.velocity[0]*5.5f;

			for(i=0;i<1024;i++)
			{
				gripper = &g_entities[i];
				if(gripper && gripper->client)
				{
					if(gripper->client->ps.fd.forceGripEntityNum == other->client->ps.clientNum)
						break;
				}
			}

			if(gripper == NULL)
				return;
		
			//G_Printf("Damage: %i\n",damage);
			//G_Damage(gripEnt, self, self, NULL, NULL, 2, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
			G_Damage(other,gripper,gripper,NULL,NULL,damage,DAMAGE_NO_ARMOR,MOD_FORCE_DARK);
			G_Damage(self,other,other,NULL,NULL,damage,DAMAGE_NO_ARMOR,0);
		}
	}
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
//[CoOp]
extern void UpdatePlayerScriptTarget(void);
extern qboolean UseSpawnWeapons;
extern int SpawnWeapons;
//[/CoOp]
//[VisualWeapons]
//prototype
qboolean OJP_AllPlayersHaveClientPlugin(void);
//[/VisualWeapons]
//[TABBots]
extern int FindBotType(int clientNum);
//[/TABBots]
extern qboolean WP_HasForcePowers( const playerState_t *ps );
//[StanceSelection]
extern qboolean G_ValidSaberStyle(gentity_t *ent, int saberStyle);
extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
//[StanceSelection]
void ClientSpawn(gentity_t *ent) {
	int					index;
	vec3_t				spawn_origin, spawn_angles;
	gclient_t			*client;
	int					i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int					persistant[MAX_PERSISTANT];
	gentity_t			*spawnPoint;
	int					flags, gameFlags;
	int					savedPing;
	int					accuracy_hits, accuracy_shots;
	int					eventSequence;
	char				userinfo[MAX_INFO_STRING];
	forcedata_t			savedForce;
	int					saveSaberNum = ENTITYNUM_NONE;
	int					wDisable = 0;
	int					savedSiegeIndex = 0;
	//[ExpSys]
	int					savedSkill[NUM_SKILLS];
	//[/ExpSys]
	//[DodgeSys]
	int					savedDodgeMax;
	//[/DodgeSys]
	int					maxHealth;
	saberInfo_t			saberSaved[MAX_SABERS];
	int					l = 0;
	void				*g2WeaponPtrs[MAX_SABERS];
	char				*value;
	char				*saber;
	qboolean			changedSaber = qfalse;
	qboolean			inSiegeWithClass = qfalse;


	index = ent - g_entities;
	client = ent->client;

	//[ExpSys]
	/*
	if(ent->client->sess.skillPoints < g_minForceRank.value)
	{//the minForceRank was changed to a higher value than the player has
		ent->client->sess.skillPoints = g_minForceRank.value;
		ent->client->skillUpdated = qtrue;
	}*/
	//[/ExpSys]]


	//first we want the userinfo so we can see if we should update this client's saber -rww
	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	while (l < MAX_SABERS)
	{
		switch (l)
		{
		case 0:
			saber = &ent->client->sess.saberType[0];
			break;
		case 1:
			saber = &ent->client->sess.saber2Type[0];
			break;
		default:
			saber = NULL;
			break;
		}

		value = Info_ValueForKey (userinfo, va("saber%i", l+1));
		if (saber &&
			value &&
			(Q_stricmp(value, saber) || !saber[0] || !ent->client->saber[0].model[0]))
		{ //doesn't match up (or our session saber is BS), we want to try setting it
			if (G_SetSaber(ent, l, value, qfalse))
			{
				changedSaber = qtrue;
			}
			else if (!saber[0] || !ent->client->saber[0].model[0])
			{ //Well, we still want to say they changed then (it means this is siege and we have some overrides)
				changedSaber = qtrue;
			}
		}
		l++;
	}

	if (changedSaber)
	{ //make sure our new info is sent out to all the other clients, and give us a valid stance
		ClientUserinfoChanged( ent->s.number );

		//make sure the saber models are updated
		G_SaberModelSetup(ent);

		l = 0;
		while (l < MAX_SABERS)
		{ //go through and make sure both sabers match the userinfo
			switch (l)
			{
			case 0:
				saber = &ent->client->sess.saberType[0];
				break;
			case 1:
				saber = &ent->client->sess.saber2Type[0];
				break;
			default:
				saber = NULL;
				break;
			}

			value = Info_ValueForKey (userinfo, va("saber%i", l+1));

			if (Q_stricmp(value, saber))
			{ //they don't match up, force the user info
				Info_SetValueForKey(userinfo, va("saber%i", l+1), saber);
				trap_SetUserinfo( ent->s.number, userinfo );
			}
			l++;
		}

		//[StanceSelection]
		/*
		if (ent->client->saber[0].model[0] &&
			ent->client->saber[1].model[0])
		{ //dual
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
		}
		else if ((ent->client->saber[0].saberFlags&SFL_TWO_HANDED))
		{ //staff
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
		}
		else
		{
			if (ent->client->sess.saberLevel < SS_FAST)
			{
				ent->client->sess.saberLevel = SS_FAST;
			}
			//[SaberSys]
			//Revised to handle the hidden styles.
			else if (ent->client->sess.saberLevel > SS_TAVION)
			{
				ent->client->sess.saberLevel = SS_TAVION;
			}
			*//*
			else if (ent->client->sess.saberLevel > SS_STRONG)
			{
				ent->client->sess.saberLevel = SS_STRONG;
			}
			*//*
			//[/SaberSys]
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

			//[SaberSys]
			//don't want this anymore since we have more styles than saber offense powers at the moment with the hidden styles.
			*//*
			if (g_gametype.integer != GT_SIEGE &&
				ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
			{
				ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
			}
			*//*
			//[/SaberSys]
		}
		if ( g_gametype.integer != GT_SIEGE )
		{
			//let's just make sure the styles we chose are cool
			if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
			{
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
				ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
			}
		}
		*/
		//[/StanceSelection]
	}
	l = 0;

	//[ExpSys]
	//always reinit force powers for bots since they're stupid and don't change their skill points when they gain experience.
	if (client->ps.fd.forceDoInit || ent->r.svFlags & SVF_BOT)
	//if (client->ps.fd.forceDoInit)
	//[/ExpSys]
	{ //force a reread of force powers
		WP_InitForcePowers( ent );
		client->ps.fd.forceDoInit = 0;
	}

	//[TABBots]
	if ( ent->r.svFlags & SVF_BOT && FindBotType(ent->s.number) == BOT_TAB
		&& ent->client->ps.fd.saberAnimLevel != SS_STAFF 
		&& ent->client->ps.fd.saberAnimLevel != SS_DUAL) 
	{//TABBots randomly switch styles on respawn if not using a staff or dual
		//[StanceSelection]
		int newLevel = Q_irand( SS_FAST, SS_STAFF );

		//new validation technique.
		if ( !G_ValidSaberStyle(ent, newLevel) )
		{//had an illegal style, revert to a valid one
			int count;
			for(count = SS_FAST; count < SS_STAFF; count++)
			{
				newLevel++;
				if(newLevel > SS_STAFF)
				{
					newLevel = SS_FAST;
				}

				if(G_ValidSaberStyle(ent, newLevel))
				{
					break;
				}
			}
		}

		ent->client->ps.fd.saberAnimLevel = newLevel;
		/*
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel 
			= ent->client->sess.saberLevel = Q_irand(SS_FAST, SS_TAVION);
		*/
		//[/StanceSelection]
	}
	//[/TABBots]

	//[StanceSelection]
	/*
	if (ent->client->ps.fd.saberAnimLevel != SS_STAFF &&
		ent->client->ps.fd.saberAnimLevel != SS_DUAL &&
		ent->client->ps.fd.saberAnimLevel == ent->client->ps.fd.saberDrawAnimLevel &&
		ent->client->ps.fd.saberAnimLevel == ent->client->sess.saberLevel)
	{
		if (ent->client->sess.saberLevel < SS_FAST)
		{
			ent->client->sess.saberLevel = SS_FAST;
		}
		//[SaberSys]
		//Revised to handle the hidden styles
		else if (ent->client->sess.saberLevel > SS_TAVION)
		{
			ent->client->sess.saberLevel = SS_TAVION;
		}
		*//*
		else if (ent->client->sess.saberLevel > SS_STRONG)
		{
			ent->client->sess.saberLevel = SS_STRONG;
		}
		*//*
		//[/SaberSys]
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

		//[SaberSys]
		//don't want this anymore since we have more styles than saber offense powers at the moment with the hidden styles.
		*//*
		if (g_gametype.integer != GT_SIEGE &&
			ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
		{
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
		*//*
		//[/SaberSys]
	}
	*/

	ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;
	if ( g_gametype.integer != GT_SIEGE )
	{
		//let's just make sure the styles we chose are cool
		if ( !G_ValidSaberStyle(ent, ent->client->ps.fd.saberAnimLevel) )
		{//had an illegal style, revert to default
			ent->client->ps.fd.saberAnimLevel = SS_MEDIUM;
			ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
		}
	}
	//[/StanceSelection]

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						spawn_origin, spawn_angles);
	} else if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( 
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						spawn_origin, spawn_angles);
	}
	else if (g_gametype.integer == GT_SIEGE)
	{
		spawnPoint = SelectSiegeSpawnPoint (
						client->siegeClass,
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						spawn_origin, spawn_angles);
	}
	//[CoOp]
	else if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		spawnPoint = SelectSPSpawnPoint(spawn_origin, spawn_angles);
	}
	//[/CoOp]
	else {
		do {
			if (g_gametype.integer == GT_POWERDUEL)
			{
				spawnPoint = SelectDuelSpawnPoint(client->sess.duelTeam, client->ps.origin, spawn_origin, spawn_angles);
			}
			else if (g_gametype.integer == GT_DUEL)
			{	// duel 
				spawnPoint = SelectDuelSpawnPoint(DUELTEAM_SINGLE, client->ps.origin, spawn_origin, spawn_angles);
			}
			else
			{
				// the first spawn should be at a good looking spot
				if ( !client->pers.initialSpawn && client->pers.localClient ) {
					client->pers.initialSpawn = qtrue;
					spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles, client->sess.sessionTeam );
				} else {
					// don't spawn near existing origin if possible
					spawnPoint = SelectSpawnPoint ( 
						client->ps.origin, 
						spawn_origin, spawn_angles, client->sess.sessionTeam );
				}
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT );
	flags ^= EF_TELEPORT_BIT;
	gameFlags = ent->client->mGameFlags & ( PSG_VOTED | PSG_TEAMVOTED);

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	saveSaberNum = client->ps.saberEntityNum;

	savedSiegeIndex = client->siegeClass;

	//[ExpSys]

	for(i = 0; i < NUM_SKILLS; i++)
	{
		savedSkill[i] = client->skillLevel[i];
	}
	//[/ExpSys]

	//[DodgeSys]
	savedDodgeMax = client->ps.stats[STAT_MAX_DODGE];
	//[/DodgeSys]

	l = 0;
	while (l < MAX_SABERS)
	{
		saberSaved[l] = client->saber[l];
		g2WeaponPtrs[l] = client->weaponGhoul2[l];
		l++;
	}

	i = 0;
	while (i < HL_MAX)
	{
		ent->locationDamage[i] = 0;
		i++;
	}


	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?
	client->bodyGrabIndex = ENTITYNUM_NONE;



	//Get the skin RGB based on his userinfo
	value = Info_ValueForKey (userinfo, "char_color_red");
	if (value)
	{
		client->ps.customRGBA[0] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[0] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_green");
	if (value)
	{
		client->ps.customRGBA[1] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[1] = 255;
	}

	value = Info_ValueForKey (userinfo, "char_color_blue");
	if (value)
	{
		client->ps.customRGBA[2] = atoi(value);
	}
	else
	{
		client->ps.customRGBA[2] = 255;
	}

	if ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100)
	{ //hmm, too dark!
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;
	}

	client->ps.customRGBA[3]=255;

	//[BugFix32]
	//update our customRGBA for team colors. 
	if ( g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_SIEGE && !g_trueJedi.integer )
	{
		char skin[MAX_QPATH];
		char model[MAX_QPATH];
		float colorOverride[3];

		colorOverride[0] = colorOverride[1] = colorOverride[2] = 0.0f;
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );

		BG_ValidateSkinForTeam( model, skin, savedSess.sessionTeam, colorOverride);
		if (colorOverride[0] != 0.0f ||
			colorOverride[1] != 0.0f ||
			colorOverride[2] != 0.0f)
		{
			client->ps.customRGBA[0] = colorOverride[0]*255.0f;
			client->ps.customRGBA[1] = colorOverride[1]*255.0f;
			client->ps.customRGBA[2] = colorOverride[2]*255.0f;
		}
	}
	//[/BugFix32]

	client->siegeClass = savedSiegeIndex;

	//[ExpSys]

	for(i = 0; i < NUM_SKILLS; i++)
	{
		client->skillLevel[i] = savedSkill[i];
	}
	//[/ExpSys]

	//[DodgeSys]
	client->ps.stats[STAT_MAX_DODGE] = savedDodgeMax;
	//[/DodgeSys]

	l = 0;
	while (l < MAX_SABERS)
	{
		client->saber[l] = saberSaved[l];
		client->weaponGhoul2[l] = g2WeaponPtrs[l];
		l++;
	}

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;
	client->saberStoredIndex = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;
	

	

		if(client->skillLevel[SK_JETPACK] == FORCE_LEVEL_3 || client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3 )
		{
			client->ps.jetpackFuel = 250;
		}
		else if(client->skillLevel[SK_JETPACK] == FORCE_LEVEL_2 || client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_2 )		
		{
			client->ps.jetpackFuel = 150;
		}
		else if(client->skillLevel[SK_JETPACK] == FORCE_LEVEL_1 || client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_1)
		{
			client->ps.jetpackFuel = 100;
		}
		else if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK) || client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_FLAMETHROWER))
		{	
			client->ps.jetpackFuel = 100;			
		}
		else
		{
			client->ps.jetpackFuel = 0;
		}



		if(client->skillLevel[SK_CLOAK] == FORCE_LEVEL_3 || client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3 || client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_3 || client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_3)
		{
			client->ps.cloakFuel = 250;
		}	
		else if(client->skillLevel[SK_CLOAK] == FORCE_LEVEL_2 || client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_2 || client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_2 || client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_2)
		{
			client->ps.cloakFuel = 150;
		}		
		else if(client->skillLevel[SK_CLOAK] == FORCE_LEVEL_1 || client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_1 || client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_1 || client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_1)
		{
			client->ps.cloakFuel = 100;
		}	
		else if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_CLOAK) || client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_ELECTROSHOCKER) || client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SPHERESHIELD) || client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_OVERLOAD))
		{	
			client->ps.cloakFuel = 100;			
		}
		else
		{
			client->ps.cloakFuel = 0;
		}

	//spawn with 100
	//client->ps.jetpackFuel = 100;
	//client->ps.cloakFuel = 100;

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// set max health


	if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_3)
		{
		maxHealth = 999;
		}
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_2)
		{
		maxHealth = 500;
		}	
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_1)
		{
		maxHealth = 250;
		}
	else 
		{
		if (g_gametype.integer == GT_SIEGE && client->siegeClass != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
			maxHealth = 100;

			if (scl->maxhealth)
			{
				maxHealth = scl->maxhealth;
			}
		}
		else
		{
		maxHealth = 100;
		}
		}
			

	client->pers.maxHealth = maxHealth;//atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) {
	if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_3)
		{
		client->pers.maxHealth = 999;
		}
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_2)
		{
		client->pers.maxHealth = 500;
		}	
	else if(client->skillLevel[SK_HEALTH] == FORCE_LEVEL_1)
		{
		client->pers.maxHealth = 250;
		}
	else 
		{
		if (g_gametype.integer == GT_SIEGE && client->siegeClass != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
			client->pers.maxHealth = 100;

			if (scl->maxhealth)
			{
				client->pers.maxHealth = scl->maxhealth;
			}
		}
		else
		{
		client->pers.maxHealth = 100;
		}
		}
	}		
		//client->pers.maxHealth = 100;
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	client->mGameFlags = gameFlags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->playerState = &ent->client->ps;
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->touch = player_touch;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);
	client->ps.crouchheight = CROUCH_MAXS_2;
	client->ps.standheight = DEFAULT_MAXS_2;

	client->ps.clientNum = index;
	//give default weapons
	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_NONE );

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}


	int iDisable = 0;
	iDisable = g_itemDisable.integer;
	
	
	
	//[MELEE]
	//Give everyone fists as long as they aren't disabled.
	if (!wDisable || !(wDisable & (1 << WP_MELEE)))
	{
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
	}
	//[/MELEE]


	//racc - set weapons for everything except siege
	//[CoOp]
	//[ExpSys]
	/* spawn weapons screws up the experience system
	//[/ExpSys]
	if( UseSpawnWeapons )
	{//we have ICARUS overrides on the weapons you should spawn with.
		client->ps.stats[STAT_WEAPONS] = SpawnWeapons;

		//[CoOp]
		if(g_gametype.integer == GT_SINGLE_PLAYER)
		{//I want MP to have Melee all the time, so you'll be able to punch/kick your
		//NPC allies if they get stuck
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
		}
		//[/CoOp]

		//set initial selected weapon
		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			client->ps.weapon = WP_SABER;
		}
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL))
		{
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
		else
		{
			client->ps.weapon = WP_MELEE;
		}
	}
	//[ExpSys]
	else if ( g_gametype.integer != GT_HOLOCRON 
	*/
	if ( g_gametype.integer != GT_HOLOCRON 
	//[/ExpSys]
	//[/CoOp]
		 
		&& !HasSetSaberOnly()
		&& !AllForceDisabled( g_forcePowerDisable.integer )
		&& g_trueJedi.integer )
	{
		if ( g_gametype.integer >= GT_TEAM && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam == TEAM_RED) )
		{//In Team games, force one side to be merc and other to be jedi
			if ( level.numPlayingClients > 0 )
			{//already someone in the game
				int		i, forceTeam = TEAM_SPECTATOR;
				for ( i = 0 ; i < level.maxclients ; i++ ) 
				{
					if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
						continue;
					}
					if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED ) 
					{//in-game
						if ( WP_HasForcePowers( &level.clients[i].ps ) )
						{//this side is using force
							forceTeam = level.clients[i].sess.sessionTeam;
						}
						else
						{//other team is using force
							if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							{
								forceTeam = TEAM_RED;
							}
							else
							{
								forceTeam = TEAM_BLUE;
							}
						}
						break;
					}
				}
				if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam )
				{//using force but not on right team, switch him over
					const char *teamName = TeamName( forceTeam );
					//client->sess.sessionTeam = forceTeam;
					SetTeam( ent, (char *)teamName );
					return;
				}
			}
		}

		if ( WP_HasForcePowers( &client->ps ) )
		{
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
			//make sure they only use the saber
			client->ps.weapon = WP_SABER;
			client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
		else
		{//no force powers set
			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;
			if (!wDisable || !(wDisable & (1 << WP_STUN_BATON)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STUN_BATON );
			}
			if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
			}
			if (!wDisable || !(wDisable & (1 << WP_BLASTER)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BLASTER );
			}
			if (!wDisable || !(wDisable & (1 << WP_DISRUPTOR)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DISRUPTOR );
			}
			if (!wDisable || !(wDisable & (1 << WP_BOWCASTER)))	
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BOWCASTER );
			}
			if (!wDisable || !(wDisable & (1 << WP_REPEATER)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_REPEATER );
			}
			if (!wDisable || !(wDisable & (1 << WP_DEMP2)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DEMP2 );
			}
			if (!wDisable || !(wDisable & (1 << WP_FLECHETTE)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_FLECHETTE );
			}
			if (!wDisable || !(wDisable & (1 << WP_ROCKET_LAUNCHER)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_ROCKET_LAUNCHER );
			}			
			if (!wDisable || !(wDisable & (1 << WP_THERMAL)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_THERMAL );
			}
			if (!wDisable || !(wDisable & (1 << WP_TRIP_MINE)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TRIP_MINE );
			}
			if (!wDisable || !(wDisable & (1 << WP_DET_PACK)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DET_PACK );
			}
			if (!wDisable || !(wDisable & (1 << WP_CONCUSSION)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_CONCUSSION );
			}
			if (!wDisable || !(wDisable & (1 << WP_BRYAR_OLD)))//JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_OLD );
			}
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
	}
	else
	{//jediVmerc is incompatible with this gametype, turn it off!
		trap_Cvar_Set( "g_jediVmerc", "0" );
		if (g_gametype.integer == GT_HOLOCRON)
		{
			//[/MOREWEAPOPTIONS]
			if (!wDisable || !(wDisable & (1 << WP_MELEE)))
			{
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			}
		
			if (!wDisable || !(wDisable & (1 << WP_SABER)))
			{
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );
				
			}


			
			//always get free saber level 1 in holocron
			//client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
			//[/MOREWEAPOPTIONS]
		}
		else
		{
			if (client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
			{
				//[/MOREWEAPOPTIONS]
				if (!wDisable || !(wDisable & (1 << WP_SABER)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
				}
				//client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SABER );	//these are precached in g_items, ClearRegisteredItems()
				
			}
			//[ExpSys]
			/* NUAM already get melee for free earlier in the function.
			else
			{ //if you don't have saber attack rank then you don't get a saber
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			}

			//[CoOp]
			if(g_gametype.integer == GT_SINGLE_PLAYER)
			{//I want MP to have Melee all the time, so you'll be able to punch/kick your
			//NPC allies if they get stuck
				client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			}
			//[/CoOp]
			*/
			if(client->skillLevel[SK_WRIST])
			{//player has blaster
				if (!wDisable || !(wDisable & (1 << WP_STUN_BATON)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STUN_BATON );
				}
				
			}
			if(client->skillLevel[SK_PISTOL])
			{//player has blaster
				if (!wDisable || !(wDisable & (1 << WP_BRYAR_PISTOL)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_PISTOL );
				}
				
			}

			if(client->skillLevel[SK_BLASTER])
			{//player has blaster
				if (!wDisable || !(wDisable & (1 << WP_BLASTER)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BLASTER );
				}

			}
			if(client->skillLevel[SK_DISRUPTOR])
			{//player has disruptor
				if (!wDisable || !(wDisable & (1 << WP_DISRUPTOR)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DISRUPTOR );
				}

			}
			if(client->skillLevel[SK_BOWCASTER])
			{//player has bowcaster skill
				if (!wDisable || !(wDisable & (1 << WP_BOWCASTER)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BOWCASTER );
				}

				if(client->skillLevel[SK_BOWCASTER] >= FORCE_LEVEL_2)
			
					client->ps.eFlags2 |= EF2_BOWCASTERSCOPE;

			}
			if(client->skillLevel[SK_REPEATER])
			{//player has repeater
				if (!wDisable || !(wDisable & (1 << WP_REPEATER)))
																			   
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_REPEATER );
				}

			}
			if(client->skillLevel[SK_DEMP2])
			{//player has concussion
				if (!wDisable || !(wDisable & (1 << WP_DEMP2)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DEMP2 );
				}

			}
			if(client->skillLevel[SK_FLECHETTE])
			{
				if (!wDisable || !(wDisable & (1 << WP_FLECHETTE)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_FLECHETTE );
				}

			}
			if(client->skillLevel[SK_ROCKET])
			{//player has rocket launcher
				if (!wDisable || !(wDisable & (1 << WP_ROCKET_LAUNCHER)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_ROCKET_LAUNCHER );
				}

			}
			if(client->skillLevel[SK_THERMAL])
			{//player has thermals
				if (!wDisable || !(wDisable & (1 << WP_THERMAL)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_THERMAL );
				}

			}
			if(client->skillLevel[SK_TRIPMINE])
			{//player has thermals
				if (!wDisable || !(wDisable & (1 << WP_TRIP_MINE)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TRIP_MINE );
				}

			}
			if(client->skillLevel[SK_DETPACK])
			{//player has blaster
				if (!wDisable || !(wDisable & (1 << WP_DET_PACK)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DET_PACK );
				}
			}
			if(client->skillLevel[SK_CONCUSSION])
			{//player has concussion
				if (!wDisable || !(wDisable & (1 << WP_CONCUSSION)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_CONCUSSION );
				}


			}
			if(client->skillLevel[SK_OLD])
			{//player has blaster
				if (!wDisable || !(wDisable & (1 << WP_BRYAR_OLD)))
				{
					client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BRYAR_OLD );
				}

			}

			//[/ExpSys]
		}


		

		//[MOREWEAPOPTIONS]
		if( wDisable == WP_ALLDISABLED )
		{//some joker disabled all the weapons.  Give everyone Melee
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			G_Printf( "ERROR:  The game doesn't like it when you disable ALL the weapons.\nReenabling Melee.\n");
			if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
			{
				trap_Cvar_Set( "g_duelWeaponDisable", va("%i", WP_MELEEONLY) );
			}
			else
			{
				trap_Cvar_Set( "g_weaponDisable", va("%i", WP_MELEEONLY) );
		//[/MOREWEAPOPTIONS]
			}
		}
	
		if (g_gametype.integer == GT_JEDIMASTER)
		{
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			//[MOREWEAPOPTIONS]

			//client->ps.stats[STAT_WEAPONS] |= (1 << WP_MELEE);
			//[/MOREWEAPOPTIONS]
		}

		//[ExpSys]
		//racc - set initial weapon selected.
		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{//sabers auto selected if we have them.
			client->ps.weapon = WP_SABER;
		}
		else
		{
			int i;
			for ( i = LAST_USEABLE_WEAPON ; i > WP_NONE ; i-- )
			{
				if(client->ps.stats[STAT_WEAPONS] & (1 << i) )
				{//have this one, equip it.
					client->ps.weapon = i;
					break;
				}
			}
		}

		/* 
		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			client->ps.weapon = WP_SABER;
		}
		else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL))
		{
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
		else
		{
			client->ps.weapon = WP_MELEE;
		}
		*/
		//[/ExpSys]
	}

	/*
	client->ps.stats[STAT_HOLDABLE_ITEMS] |= ( 1 << HI_BINOCULARS );
	client->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_BINOCULARS, IT_HOLDABLE);
	*/

	if (g_gametype.integer == GT_SIEGE && client->siegeClass != -1 &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //well then, we will use a custom weaponset for our class
		int m = 0;

		client->ps.stats[STAT_WEAPONS] = bgSiegeClasses[client->siegeClass].weapons;

		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			client->ps.weapon = WP_SABER;
		}
		else
		{
			client->ps.weapon = WP_MELEE;
		}
		inSiegeWithClass = qtrue;

		while (m < WP_NUM_WEAPONS)
		{
			if (client->ps.stats[STAT_WEAPONS] & (1 << m))
			{
				if (client->ps.weapon != WP_SABER)
				{ //try to find the highest ranking weapon we have
					if (m > client->ps.weapon)
					{
						client->ps.weapon = m;
					}
				}

				if (m >= WP_BRYAR_PISTOL)
				{ //Max his ammo out for all the weapons he has.
					if ( g_gametype.integer == GT_SIEGE 
						&& m == WP_ROCKET_LAUNCHER )
					{//don't give full ammo!
						//FIXME: extern this and check it when getting ammo from supplier, pickups or ammo stations!
						if ( client->siegeClass != -1 &&
							(bgSiegeClasses[client->siegeClass].classflags & (1<<CFL_SINGLE_ROCKET)) )
						{
							client->ps.ammo[weaponData[m].ammoIndex] = 1;
						}
						else
						{
							client->ps.ammo[weaponData[m].ammoIndex] = 10;
						}
					}
					else
					{
						//if ( g_gametype.integer == GT_SIEGE 
						//	&& client->siegeClass != -1
						//	&& (bgSiegeClasses[client->siegeClass].classflags & (1<<CFL_EXTRA_AMMO)) )
						//{//double ammo
						//	client->ps.ammo[weaponData[m].ammoIndex] = ammoData[weaponData[m].ammoIndex].max*2;
						//	client->ps.eFlags |= EF_DOUBLE_AMMO;
						//}
						//else
						{
							client->ps.ammo[weaponData[m].ammoIndex] = ammoData[weaponData[m].ammoIndex].max;
						}
					}
				}
			}
			m++;
		}
	}

	if (g_gametype.integer == GT_SIEGE &&
		client->siegeClass != -1 &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //use class-specified inventory
		client->ps.stats[STAT_HOLDABLE_ITEMS] = bgSiegeClasses[client->siegeClass].invenItems;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
						 
	}
	
	

	
		//[ExpSys]
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;

		
		if(client->skillLevel[SK_JETPACK])
		{//player has jetpack
			if (!iDisable || !(iDisable & (1 << HI_JETPACK)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
			}

		}

		if(client->skillLevel[SK_FORCEFIELD])
		{//give the player the force field item
			if (!iDisable || !(iDisable & (1 << HI_SHIELD)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SHIELD);
			}
		}

		if(client->skillLevel[SK_CLOAK])
		{//give the player the cloaking device
			if (!iDisable || !(iDisable & (1 << HI_CLOAK)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_CLOAK);
			}
		}
		if(client->skillLevel[SK_SPHERESHIELD])
		{//give the player the cloaking device
			if (!iDisable || !(iDisable & (1 << HI_SPHERESHIELD)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SPHERESHIELD);
			}
		}
		if(client->skillLevel[SK_OVERLOAD])
		{//give the player the cloaking device
			if (!iDisable || !(iDisable & (1 << HI_OVERLOAD)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_OVERLOAD);
			}
		}
		//gain squadteam		
		if(client->skillLevel[SK_SQUADTEAM])
			{
			if (!iDisable || !(iDisable & (1 << HI_SQUADTEAM)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SQUADTEAM);
			}
									   
										 
			}
		//gain vehiclemount		
		if(client->skillLevel[SK_VEHICLEMOUNT])
			{
			if (!iDisable || !(iDisable & (1 << HI_VEHICLEMOUNT)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_VEHICLEMOUNT);
			}
									   
										 
			}


	
		if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_3)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 999;
		}
		else if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_2)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 500;
		}
		else if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_1)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 250;
		}
		else 
		{

			if (g_gametype.integer == GT_SIEGE &&
			client->siegeClass != -1 /*&&
			bgSiegeClasses[client->siegeClass].startarmor*/)
			{ //class specifies a start armor amount, so use it
				siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
				client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 100;
				if (scl->maxarmor)
				{
				client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = scl->maxarmor;
				}
			}
			else
			{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 100;					
			}

		}
		

			if(client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3)
			{
			ent->client->ps.fd.forcePower=250;
			ent->client->ps.fd.forcePowerMax=250;
			client->ps.stats[STAT_MAX_DODGE] = 250;
			}
			else if(client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2)
			{
			ent->client->ps.fd.forcePower=150;
			ent->client->ps.fd.forcePowerMax=150;
			client->ps.stats[STAT_MAX_DODGE] = 150;

			}
			else if(client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1)
			{
			ent->client->ps.fd.forcePower=100;
			ent->client->ps.fd.forcePowerMax=100;
			client->ps.stats[STAT_MAX_DODGE] = 100;
			}
			else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
			{ //recharge cloak
			ent->client->ps.fd.forcePower=100;
			ent->client->ps.fd.forcePowerMax=100;
			client->ps.stats[STAT_MAX_DODGE] = 100;
			}
			else 
			{
			ent->client->ps.fd.forcePower=25;
			ent->client->ps.fd.forcePowerMax=25;
			client->ps.stats[STAT_MAX_DODGE] = 25;
			}



						   
		

		if(client->skillLevel[SK_BACTA])
		{
			if (!iDisable || !(iDisable & (1 << HI_MEDPAC)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC);
			}
		}

		if(client->skillLevel[SK_REPAIR])
		{
			if (!iDisable || !(iDisable & (1 << HI_SHIELDBOOSTER)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SHIELDBOOSTER);
			}
		}		
		//gain flamethrower
		if(client->skillLevel[SK_FLAMETHROWER])
		{
			if (!iDisable || !(iDisable & (1 << HI_FLAMETHROWER)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_FLAMETHROWER);
			}
		}

		if(client->skillLevel[SK_SEEKER])
		{
			if (!iDisable || !(iDisable & (1 << HI_SEEKER)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SEEKER);
			}
		}

		if(client->skillLevel[SK_SENTRY])
		{
			if (!iDisable || !(iDisable & (1 << HI_SENTRY_GUN)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SENTRY_GUN);
			}
		}
	//gain EWEB
		if(client->skillLevel[SK_EWEB])
		{
			if (!iDisable || !(iDisable & (1 << HI_EWEB)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_EWEB);
			}
		}
	//gain BINOCULARS
		if(client->skillLevel[SK_BINOCULARS])
		{
			if (!iDisable || !(iDisable & (1 << HI_BINOCULARS)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_BINOCULARS);
			}
		}



				//gain electroshocker
		if(client->skillLevel[SK_ELECTROSHOCKER])
		{
			if (!iDisable || !(iDisable & (1 << HI_ELECTROSHOCKER)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_ELECTROSHOCKER);
			}
		}
		//client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		//[/ExpSys]

		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		

		
		//[DualPistols]
		if(client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
		{
			ent->client->ps.eFlags |= EF_DUAL_WEAPONS;
		}
		if(client->skillLevel[SK_OLD] >= FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_OLD)
		{
			ent->client->ps.eFlags |= EF_DUAL_WEAPONS;
		}
		if(client->skillLevel[SK_WRIST] >= FORCE_LEVEL_3 && ent->client->ps.weapon == WP_STUN_BATON)
		{
			ent->client->ps.eFlags |= EF_DUAL_WEAPONS;
		}
		
		
		
		
		if(client->skillLevel[SK_WRIST] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_STUN_BATON)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}		
		if(client->skillLevel[SK_PISTOL] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_BLASTER] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BLASTER)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_DISRUPTOR] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_DISRUPTOR)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_REPEATER] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_REPEATER)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_DEMP2] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_DEMP2)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_FLECHETTE] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_FLECHETTE)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_CONCUSSION] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_CONCUSSION)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_ROCKET] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_THERMAL] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_THERMAL)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}			
		if(client->skillLevel[SK_TRIPMINE] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_TRIP_MINE)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}	
		if(client->skillLevel[SK_OLD] < FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BRYAR_OLD)
		{
			ent->client->ps.eFlags2 |= EF2_NOALTFIRE;
		}			



		if(client->skillLevel[SK_WRIST] >= FORCE_LEVEL_1 && client->skillLevel[SK_WRISTA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_STUN_BATON)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_WRIST] >= FORCE_LEVEL_1 && client->skillLevel[SK_WRISTA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_STUN_BATON)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_WRIST] >= FORCE_LEVEL_1 && client->skillLevel[SK_WRISTB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_STUN_BATON)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		
		
		if(client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_1 && client->skillLevel[SK_PISTOLA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_1 && client->skillLevel[SK_PISTOLA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_1 && client->skillLevel[SK_PISTOLB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		
		
		

		if(client->skillLevel[SK_BLASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BLASTERA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BLASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_BLASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BLASTERA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BLASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_BLASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BLASTERB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_BLASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}

		


		if(client->skillLevel[SK_DISRUPTOR] >= FORCE_LEVEL_1 && client->skillLevel[SK_DISRUPTORA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_DISRUPTOR)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_DISRUPTOR] >= FORCE_LEVEL_1 && client->skillLevel[SK_DISRUPTORA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_DISRUPTOR)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_DISRUPTOR] >= FORCE_LEVEL_1 && client->skillLevel[SK_DISRUPTORB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_DISRUPTOR)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}


		
		
		if(client->skillLevel[SK_BOWCASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BOWCASTERA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BOWCASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_BOWCASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BOWCASTERA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BOWCASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_BOWCASTER] >= FORCE_LEVEL_1 && client->skillLevel[SK_BOWCASTERB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_BOWCASTER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}


		

		if(client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_1 && client->skillLevel[SK_REPEATERA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_REPEATER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_1 && client->skillLevel[SK_REPEATERA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_REPEATER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_1 && client->skillLevel[SK_REPEATERB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_REPEATER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}		
		
	
	
	
	
		if(client->skillLevel[SK_DEMP2] >= FORCE_LEVEL_1 && client->skillLevel[SK_DEMP2A] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_DEMP2)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_DEMP2] >= FORCE_LEVEL_1 && client->skillLevel[SK_DEMP2A] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_DEMP2)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_DEMP2] >= FORCE_LEVEL_1 && client->skillLevel[SK_DEMP2B] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_DEMP2)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		
		


		if(client->skillLevel[SK_FLECHETTE] >= FORCE_LEVEL_1 && client->skillLevel[SK_FLECHETTEA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_FLECHETTE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_FLECHETTE] >= FORCE_LEVEL_1 && client->skillLevel[SK_FLECHETTEA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_FLECHETTE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_FLECHETTE] >= FORCE_LEVEL_1 && client->skillLevel[SK_FLECHETTEB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_FLECHETTE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		



		if(client->skillLevel[SK_CONCUSSION] >= FORCE_LEVEL_1 && client->skillLevel[SK_CONCUSSIONA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_CONCUSSION)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_CONCUSSION] >= FORCE_LEVEL_1 && client->skillLevel[SK_CONCUSSIONA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_CONCUSSION)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_CONCUSSION] >= FORCE_LEVEL_1 && client->skillLevel[SK_CONCUSSIONB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_CONCUSSION)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		
		

		if(client->skillLevel[SK_ROCKET] >= FORCE_LEVEL_1 && client->skillLevel[SK_ROCKETA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_ROCKET] >= FORCE_LEVEL_1 && client->skillLevel[SK_ROCKETA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_ROCKET] >= FORCE_LEVEL_1 && client->skillLevel[SK_ROCKETB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}



		if(client->skillLevel[SK_THERMAL] >= FORCE_LEVEL_1 && client->skillLevel[SK_THERMALA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_THERMAL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_THERMAL] >= FORCE_LEVEL_1 && client->skillLevel[SK_THERMALA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_THERMAL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_THERMAL] >= FORCE_LEVEL_1 && client->skillLevel[SK_THERMALB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_THERMAL)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}
		
		if(client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && client->skillLevel[SK_TRIPMINEA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_TRIP_MINE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && client->skillLevel[SK_TRIPMINEA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_TRIP_MINE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && client->skillLevel[SK_TRIPMINEB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_TRIP_MINE)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}		

		if(client->skillLevel[SK_DETPACK] >= FORCE_LEVEL_1 && client->skillLevel[SK_DETPACKA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_DET_PACK)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}
		else if(client->skillLevel[SK_DETPACK] >= FORCE_LEVEL_1 && client->skillLevel[SK_DETPACKA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_DET_PACK)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}
		else if(client->skillLevel[SK_DETPACK] >= FORCE_LEVEL_1 && client->skillLevel[SK_DETPACKB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_DET_PACK)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}	

		
		if(client->skillLevel[SK_OLD] >= FORCE_LEVEL_1 && client->skillLevel[SK_OLDA] == FORCE_LEVEL_2 && ent->client->ps.weapon == WP_BRYAR_OLD)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_2;
		}					
		else if(client->skillLevel[SK_OLD] >= FORCE_LEVEL_1 && client->skillLevel[SK_OLDA] == FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_OLD)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_3;
		}			
		else if(client->skillLevel[SK_OLD] >= FORCE_LEVEL_1 && client->skillLevel[SK_OLDB] == FORCE_LEVEL_1 && ent->client->ps.weapon == WP_BRYAR_OLD)
		{
			ent->client->ps.eFlags |= EF_WP_OPTION_4;
		}	

	if (ent->client->ps.fd.forcePowerSelected == FP_PUSH && ent->client->skillLevel[SK_PUSHA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_PULL && ent->client->skillLevel[SK_PULLA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_HEAL && ent->client->skillLevel[SK_HEALA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_HEAL] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_PROTECT && ent->client->skillLevel[SK_PROTECTA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_PROTECT] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_ABSORB && ent->client->skillLevel[SK_ABSORBA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_ABSORB] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_TELEPATHY && ent->client->skillLevel[SK_TELEPATHYA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_TELEPATHY] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_TEAM_HEAL && ent->client->skillLevel[SK_STASISA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_GRIP && ent->client->skillLevel[SK_GRIPA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_GRIP] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_LIGHTNING && ent->client->skillLevel[SK_LIGHTNINGA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_LIGHTNING] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_DRAIN && ent->client->skillLevel[SK_DRAINA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_DRAIN] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_RAGE && ent->client->skillLevel[SK_RAGEA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_RAGE] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}
	
	if (ent->client->ps.fd.forcePowerSelected == FP_TEAM_FORCE && ent->client->skillLevel[SK_DESTRUCTIONA] == FORCE_LEVEL_2 && ent->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_FP_OPTION_2;
	}		
	
		
	if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_FLAMETHROWER) && ent->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_2 && ent->client->skillLevel[SK_FLAMETHROWER] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_2;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_FLAMETHROWER) && ent->client->skillLevel[SK_FLAMETHROWERA] == FORCE_LEVEL_3 && ent->client->skillLevel[SK_FLAMETHROWER] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_3;	
	}
		
	if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_ELECTROSHOCKER) && ent->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_2 && ent->client->skillLevel[SK_ELECTROSHOCKER] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_2;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_ELECTROSHOCKER) && ent->client->skillLevel[SK_ELECTROSHOCKERA] == FORCE_LEVEL_3 && ent->client->skillLevel[SK_ELECTROSHOCKER] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_3;	
	}		

	if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM) && ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_2 && ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_2;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM) && ent->client->skillLevel[SK_SQUADTEAMA] == FORCE_LEVEL_3 && ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_3;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM) && ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_1 && ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_2;
	ent->client->ps.eFlags |= EF_HI_OPTION_3;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM) && ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_2 && ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_2;
	ent->client->ps.eFlags |= EF_FP_OPTION_2;	
	}
	else if(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM) && ent->client->skillLevel[SK_SQUADTEAMB] == FORCE_LEVEL_3 && ent->client->skillLevel[SK_SQUADTEAM] >= FORCE_LEVEL_1)
	{
	ent->client->ps.eFlags |= EF_HI_OPTION_3;
	ent->client->ps.eFlags |= EF_FP_OPTION_2;	
	}	
		//[/DualPistols]
			if(client->skillLevel[SK_GRAPPLE])
		{
			if (!iDisable || !(iDisable & (1 << HI_GRAPPLE)))
			{
				client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_GRAPPLE);
			}
		}								
	
	if (g_gametype.integer == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].powerups &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //this class has some start powerups
		i = 0;
		while (i < PW_NUM_POWERUPS)
		{
			if (bgSiegeClasses[client->siegeClass].powerups & (1 << i))
			{
				client->ps.powerups[i] = Q3_INFINITE;
			}
			i++;
		}
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		client->ps.stats[STAT_WEAPONS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

// nmckenzie: DESERT_SIEGE... or well, siege generally.  This was over-writing the max value, which was NOT good for siege.
	if ( inSiegeWithClass == qfalse )
	{//racc - not playing siege, assign ammo levels.
		//[ExpSys][Reload]
		//client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max * (float) (client->skillLevel[SK_BOWCASTER] < client->skillLevel[SK_DISRUPTOR] ? client->skillLevel[SK_DISRUPTOR] : client->skillLevel[SK_BOWCASTER])/FORCE_LEVEL_3;
		client->ps.ammo[AMMO_POWERCELL] = ClipSize(AMMO_POWERCELL,ent);
		//client->ps.ammo[AMMO_METAL_BOLTS] = ammoData[AMMO_METAL_BOLTS].max * (float) client->skillLevel[SK_REPEATER]/FORCE_LEVEL_3;
		client->ps.ammo[AMMO_METAL_BOLTS] = ClipSize(AMMO_METAL_BOLTS,ent);
		//client->ps.ammo[AMMO_BLASTER] = ammoData[AMMO_BLASTER].max * (float) client->skillLevel[SK_BLASTER]/FORCE_LEVEL_3;
		client->ps.ammo[AMMO_BLASTER] = ClipSize(AMMO_BLASTER,ent);
		client->ps.ammo[AMMO_TRIPMINE] = ClipSize(AMMO_TRIPMINE,ent);
		client->ps.ammo[AMMO_DETPACK] = ClipSize(AMMO_DETPACK,ent);		
		client->ps.ammo[AMMO_ROCKETS] = ClipSize(AMMO_ROCKETS,ent);
		//[/Reload]
		client->ps.ammo[AMMO_THERMAL] = ammoData[AMMO_THERMAL].max * (float) client->skillLevel[SK_THERMAL]/FORCE_LEVEL_3;
		client->ps.ammo[AMMO_TRIPMINE] = ammoData[AMMO_TRIPMINE].max * (float) client->skillLevel[SK_TRIPMINE]/FORCE_LEVEL_3;
		client->ps.ammo[AMMO_DETPACK] = ammoData[AMMO_DETPACK].max * (float) client->skillLevel[SK_DETPACK]/FORCE_LEVEL_3;
		//client->ps.ammo[AMMO_BLASTER] = 100; //ammoData[AMMO_BLASTER].max; //100 seems fair.
		//[/ExpSys]
	}
//	client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
//	client->ps.ammo[AMMO_FORCE] = ammoData[AMMO_FORCE].max;
//	client->ps.ammo[AMMO_METAL_BOLTS] = ammoData[AMMO_METAL_BOLTS].max;
//	client->ps.ammo[AMMO_ROCKETS] = ammoData[AMMO_ROCKETS].max;
/*
	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_BRYAR_PISTOL);
	if ( g_gametype.integer == GT_TEAM ) {
		client->ps.ammo[WP_BRYAR_PISTOL] = 50;
	} else {
		client->ps.ammo[WP_BRYAR_PISTOL] = 100;
	}
*/
	client->ps.rocketLockIndex = ENTITYNUM_NONE;
	client->ps.rocketLockTime = 0;

	//rww - Set here to initialize the circling seeker drone to off.
	//A quick note about this so I don't forget how it works again:
	//ps.genericEnemyIndex is kept in sync between the server and client.
	//When it gets set then an entitystate value of the same name gets
	//set along with an entitystate flag in the shared bg code. Which
	//is why a value needs to be both on the player state and entity state.
	//(it doesn't seem to just carry over the entitystate value automatically
	//because entity state value is derived from player state data or some
	//such)
	client->ps.genericEnemyIndex = -1;

	//[VisualWeapons]
	//update the weapon stats for this player since they have changed.
	if(OJP_AllPlayersHaveClientPlugin())
	{//don't send the weapon updates if someone isn't able to process this new event type (IE anyone without
		//the OJP client plugin)
		G_AddEvent(ent, EV_WEAPINVCHANGE, client->ps.stats[STAT_WEAPONS]);
	}
	//[/VisualWeapons]

	//[Reload]
	for(i=0;i<WP_NUM_WEAPONS;i++)
		ent->bullets[i] = ammoPool[SkillLevelForWeap(ent,i)][i].max;

	ent->reloadTime =-1;
	ent->bulletsToReload = 0;
	client->ps.stats[STAT_AMMOPOOL] = ammoPool[SkillLevelForWeap(ent,ent->client->ps.weapon)][ent->client->ps.weapon].max;
	//[/Reload]

	client->ps.isJediMaster = qfalse;

	if (client->ps.fallingToDeath)
	{
		client->ps.fallingToDeath = 0;
		client->noCorpse = qtrue;
	}

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers( ent );

	//[StanceSelection]
	//set saberAnimLevelBase
	if(ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_DUAL;
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ))
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_STAFF;
	}
	else
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_MEDIUM;
	}

	//set initial saberholstered mode
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0]
			&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) 
			&& ent->client->ps.fd.saberAnimLevel != SS_DUAL)
	{//using dual sabers, but not the dual style, turn off blade
		ent->client->ps.saberHolstered = 1;
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) 
		&& ent->client->ps.fd.saberAnimLevel != SS_STAFF)
	{//using staff saber, but not the staff style, turn off blade
		ent->client->ps.saberHolstered = 1;
	}
	else
	{
		ent->client->ps.saberHolstered = 0;
	}
	//[/StanceSelection]

	// health will count down towards max_health
	if (g_gametype.integer == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].starthealth)
	{ //class specifies a start health, so use it
		ent->health = client->ps.stats[STAT_HEALTH] = bgSiegeClasses[client->siegeClass].starthealth;
	}
	else
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
	}

	// Start with a small amount of armor as well.

	//[ExpSys]

		//armor is now the number of skill points a player has not allociated. 
		if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_3)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 999;
		}
		else if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_2)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 500;
		}
		else if(client->skillLevel[SK_SHIELDS] == FORCE_LEVEL_1)
		{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 250;
		}
		else 
		{
			if (g_gametype.integer == GT_SIEGE &&
			client->siegeClass != -1 /*&&
			bgSiegeClasses[client->siegeClass].startarmor*/)
			{ //class specifies a start armor amount, so use it
				siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
				client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 100;
				if (scl->maxarmor)
				{
				client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = scl->maxarmor;
				}
			}
			else
			{
			client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] = 100;				
			}


			
		}	

		
	//	client->ps.stats[STAT_ARMOR] = client->sess.skillPoints - TotalAllociatedSkillPoints(ent);

	/*
	else if ( g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL )
	{//no armor in duel
		client->ps.stats[STAT_ARMOR] = 0;
	}
	else
	{
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_ARMOR] * 0.25;
	}
	*/
	//[/ExpSys]

	//[DodgeSys]
	//Init dodge stat.
	client->ps.stats[STAT_DODGE] = client->ps.stats[STAT_MAX_DODGE];
	//[/DodgeSys]

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {

	} else {
		G_KillBox( ent );
		trap_LinkEntity (ent);

		// force the base weapon up
		//client->ps.weapon = WP_BRYAR_PISTOL;
		//client->ps.weaponstate = FIRST_WEAPON;
		if (client->ps.weapon <= WP_NONE)
		{
			client->ps.weapon = WP_BRYAR_PISTOL;
		}

		client->ps.torsoTimer = client->ps.legsTimer = 0;

		if (client->ps.weapon == WP_SABER)
		{
			G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
		}
		else
		{
			G_SetAnim(ent, NULL, SETANIM_TORSO, TORSO_RAISEWEAP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
			//[DualPistols]
			if (client->ps.eFlags & EF_DUAL_WEAPONS)
				{
					if (client->ps.eFlags & EF_WP_OPTION_2)
					{
				client->ps.legsAnim = WeaponReadyAnim4[client->ps.weapon];
					}
					else if (client->ps.eFlags & EF_WP_OPTION_3)
					{
				client->ps.legsAnim = WeaponReadyAnim6[client->ps.weapon];
					}
					else if (client->ps.eFlags & EF_WP_OPTION_4)
					{
				client->ps.legsAnim = WeaponReadyAnim8[client->ps.weapon];
					}
					else
					{
				client->ps.legsAnim = WeaponReadyAnim2[client->ps.weapon];
					}
				}

			else
				{
					if (client->ps.eFlags & EF_WP_OPTION_2)
					{
				client->ps.legsAnim = WeaponReadyAnim3[client->ps.weapon];
					}
					else if (client->ps.eFlags & EF_WP_OPTION_3)
					{
				client->ps.legsAnim = WeaponReadyAnim5[client->ps.weapon];
					}
					else if (client->ps.eFlags & EF_WP_OPTION_4)
					{
				client->ps.legsAnim = WeaponReadyAnim7[client->ps.weapon];
					}
					else
					{
				client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];
					}
				}
			//[/DualPistols]
		}
		client->ps.weaponstate = WEAPON_RAISING;
		client->ps.weaponTime = client->ps.torsoTimer;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		//[CoOp]
		//update the player script name so that any scripts run off this spawnpoint are sure
		//to have a "player" script target to effect.
		UpdatePlayerScriptTarget();
		/* moved down so that the player script name can propogate thru ICARUS.
		G_UseTargets( spawnPoint, ent );
		if (g_gametype.integer == GT_SINGLE_PLAYER && spawnPoint)
		{//remove the target of the spawnpoint to prevent multiple target firings
			spawnPoint->target = NULL;
		}
		*/
		//[/CoOp]
		
		// select the highest weapon number available, after any
		// spawn given items have fired
		/*
		client->ps.weapon = 1;
		for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
			if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
				client->ps.weapon = i;
				break;
			}
		}
		*/
	}

	//set teams for NPCs to recognize
	if (g_gametype.integer == GT_SIEGE)
	{ //Imperial (team1) team is allied with "enemy" NPCs in this mode
		if (client->sess.sessionTeam == SIEGETEAM_TEAM1)
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_ENEMY;
			client->enemyTeam = NPCTEAM_PLAYER;
		}
		else
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
			client->enemyTeam = NPCTEAM_ENEMY;
		}
	}
	else 
	{
		if (client->sess.sessionTeam == TEAM_RED)
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_ENEMY;
			client->enemyTeam = NPCTEAM_PLAYER;
		}
		else
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
			client->enemyTeam = NPCTEAM_ENEMY;
		}
	}

	/*
	//scaling for the power duel opponent
	if (g_gametype.integer == GT_POWERDUEL &&
		client->sess.duelTeam == DUELTEAM_LONE)
	{
		client->ps.iModelScale = 125;
		VectorSet(ent->modelScale, 1.25f, 1.25f, 1.25f);
	}
	*/
	//Disabled. At least for now. Not sure if I'll want to do it or not eventually.

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities, NULL );

	// Устанавливаем масштаб
	ent->client->ps.iModelScale = ent->client->sess.modelScale;
	/*
	trap_SendServerCommand(ent - g_entities, va(
		"print \"^5[DEBUG]\n^2ClientID: ^7%d\n^2ModelScale (sess): ^7%d\n^2ModelScale (ps): ^7%d\n^2SkillPoints: ^7%d\n^2Statuses: ^7%d\n\"",
		ent->s.number,
		ent->client->sess.modelScale,
		ent->client->ps.iModelScale,
		ent->client->sess.skillPoints,
		ent->client->pers.player_statuses
	));
	*/
	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	if (g_spawnInvulnerability.integer)
	{
		ent->client->ps.eFlags |= EF_INVULNERABLE;
		ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	//rww - make sure client has a valid icarus instance
	trap_ICARUS_FreeEnt( ent );
	trap_ICARUS_InitEnt( ent );

	//[CoOp]
	if(!level.intermissiontime)
	{//we want to use all the spawnpoint's triggers if we're not in intermission.
		G_UseTargets( spawnPoint, ent );
		if (g_gametype.integer == GT_SINGLE_PLAYER && spawnPoint)
		{//remove the target of the spawnpoint to prevent multiple target firings
			spawnPoint->target = NULL;
		}
	}
	//[/CoOp]
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
//[BugFix38]
extern void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck );
//[/BugFix38]
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	void Clear_DB(ent);

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	
	//[BugFix38]
	G_LeaveVehicle( ent, qtrue );

	/*if (ent->client->ps.m_iVehicleNum)
	{ //tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			int pCon = ent->client->pers.connected;

			ent->client->pers.connected = 0;
			veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
			ent->client->pers.connected = pCon;
		}
	}*/
	//[/BugFix38]

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED 
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );
	//[AdminSys]
	G_LogPrintf( "%s" S_COLOR_WHITE " disconnected with IP: %s\n", ent->client->pers.netname, ent->client->sess.IPstring );
	//[/AdminSys]

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ( (g_gametype.integer == GT_DUEL )
		&& !level.intermissiontime
		&& !level.warmupTime ) 
	{
		if ( level.sortedClients[1] == clientNum ) 
		{
			level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[0] ].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[0] );
		}
		else if ( level.sortedClients[0] == clientNum ) {
			level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[1] ].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[1] );
		}
	}

	if (ent->ghoul2 && trap_G2_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&ent->ghoul2);
	}
	i = 0;
	while (i < MAX_SABERS)
	{
		if (ent->client->weaponGhoul2[i] && trap_G2_HaveWeGhoul2Models(ent->client->weaponGhoul2[i]))
		{
			trap_G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[i]);
		}
		i++;
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->r.contents = 0;
	
	//============================grapplemod===================
	if (ent->client->hook)
	{ // free it!
		Weapon_HookFree(ent->client->hook);
	}
  // ent->client->pers.ampunish = qfalse;
  // ent->client->pers.tzone = qfalse;
 //  ent->client->pers.bitvalue = 0;
  // ent->client->pers.iamanadmin = 0;
  // G_RemoveFromAllIgnoreLists( clientNum );


	if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL)
	{
		ent->client->sess.losses = 0;
	}	  																																	 
	
	//[BugFix39]
	// we call this after all the clearing because the objectiveItem's
	// think checks if the client entity is still inuse
	// (which it is not anymore.) << ent->inuse >>
	if (ent->client->holdingObjectiveItem > 0)
	{ //carrying a siege objective item - make sure it updates and removes itself from us now in case this is a reconnecting client to make the ent remove
		gentity_t *objectiveItem = &g_entities[ent->client->holdingObjectiveItem];

		if (objectiveItem->inuse && objectiveItem->think)
		{
            objectiveItem->think(objectiveItem);
		}
	}
	//[/BugFix39]

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}

	G_ClearClientLog(clientNum);
}


//[AdminSys]
//stuff needed for new automatic team balancing system

//find the player that has been on the server the least amount of time for this team
//HumanOnly  = sets a scan for human only bots. False scans for any player but picks
//bots over humans.
gentity_t *FindYoungestPlayeronTeam(int team, qboolean HumanOnly)
{
	int		i;
	int		YoungestNum = -1;
	int		Age = -1;

	for ( i = 0 ; i < level.maxclients ; i++ ) 
	{
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		if ( level.clients[i].sess.sessionTeam != team  
				|| (g_gametype.integer == GT_SIEGE && level.clients[i].sess.siegeDesiredTeam != team) ) 
		{//not on the right team.
			continue;
		}

		//check to see if bot should replace a human as the youngest player.
		//This makes bots be found by age, but bots will be picked over humans reguardless of age.
		if(g_entities[i].r.svFlags & SVF_BOT)
		{//a bot
			if(HumanOnly)
			{//ignore bots
				continue;
			}
			else
			{
				if(YoungestNum != -1 && !(g_entities[YoungestNum].r.svFlags & SVF_BOT))
				{//the youngest player on this team is a human, replace him with this bot.
					Age = level.clients[i].pers.enterTime;
					YoungestNum = i;
					continue;
				}
			}
		}

		//age check
		if(level.clients[i].pers.enterTime > Age)
		{//found a younger player
			Age = level.clients[i].pers.enterTime;
			YoungestNum = i;
		}
	}

	if(YoungestNum != -1)
	{
		return &g_entities[YoungestNum];
	}

	return NULL;
}

extern int G_CountBotPlayers( int team );
extern int G_CountHumanPlayers( int ignoreClientNum, int team );
void CheckTeamBotBalance(void)
{//Attempts to balance the human/bot ratio on the teams to keep it from becoming a human
	//vs bot situation.  This function does NOT do the standard checks for team balance
	//as since they are already done in CheckTeamBalance.
	
	int BotCount[TEAM_NUM_TEAMS];
	int HumanCount = G_CountHumanPlayers( -1, -1 );
	int	lesserBotTeam = -1;  //The team with the fewer bots
	int HumanChangedToTeam = -1;	//team where the human was moved to.
	gentity_t *HumanSwitchEnt = NULL;

	if(HumanCount < 2)
	{//not enough humans to need to check for bot imbalance.
		return;
	}

	//count up bots
	BotCount[TEAM_BLUE] = G_CountBotPlayers( TEAM_BLUE );
	BotCount[TEAM_RED] = G_CountBotPlayers( TEAM_RED );

	//check to see which team has fewer bots
	if(BotCount[TEAM_RED] - BotCount[TEAM_BLUE] > 1)
	{//red team has too many players
		lesserBotTeam = TEAM_BLUE;
	}
	else if(BotCount[TEAM_BLUE] - BotCount[TEAM_RED] > 1)
	{//blue team has too many players
		lesserBotTeam = TEAM_RED;
	}

	//scan for a human to switch to the bot strong team.
	if( lesserBotTeam == -1 )
	{//teams are good as is
		return;
	}
	else if( lesserBotTeam == TEAM_RED )
	{
		HumanSwitchEnt = FindYoungestPlayeronTeam( TEAM_RED, qtrue );
	}
	else
	{//( lesserBotTeam == TEAM_BLUE ) 
		HumanSwitchEnt = FindYoungestPlayeronTeam( TEAM_BLUE, qtrue );
	}

	if(HumanSwitchEnt)
	{//found someone, force them onto the other team
		if(lesserBotTeam == TEAM_RED)
		{//move to blue team
			SetTeam(HumanSwitchEnt, "b");
			HumanChangedToTeam = TEAM_BLUE;
		}
		else
		{//move to red team
			SetTeam(HumanSwitchEnt, "r");
			HumanChangedToTeam = TEAM_RED;
		}

		//send out the related messages.
		//[ClientNumFix]
		trap_SendServerCommand( HumanSwitchEnt-g_entities, "cp \"You were Team Switched for Game Balance\n\"" );
		//trap_SendServerCommand( HumanSwitchEnt->client->ps.clientNum, "cp \"You were Team Switched for Game Balance\n\"" );
		//[/ClientNumFix]
		trap_SendServerCommand(-1, va("print \"AutoGameBalancer: Moved %s" S_COLOR_WHITE " to %s Team because there wasn't enough humans on that team.\n\"",
			HumanSwitchEnt->client->pers.netname, TeamName(HumanChangedToTeam)));
		//[AdminSys]
		//switch events are now logged to the server log.
		G_LogPrintf("AutoGameBalancer: Moved %s" S_COLOR_WHITE " to %s Team because there wasn't enough humans on that team.\n\"",
			HumanSwitchEnt->client->pers.netname, TeamName(HumanChangedToTeam));
		//[/AdminSys]
	}
}

//debouncer for CheckTeamBalance function
static int CheckTeamDebounce = 0;

//checks the teams to make sure they are even.  If they aren't, bump the newest player to 
//even the teams.
extern int OJP_PointSpread(void);
void CheckTeamBalance(void)
{
	//only check team balance every 30 seconds	
	if(g_gametype.integer < GT_TEAM)
	{//non-team games.  Don't do checking.
		return;
	}

	else if((level.time - CheckTeamDebounce) < 30000)
	{
		return;
	}
	else
	{//go ahead and check.
		int	counts[TEAM_NUM_TEAMS];
		int	changeto = -1;
		int changefrom = -1;
		qboolean ScoreBalance = qfalse;

		CheckTeamDebounce = level.time;

		counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
		counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

		if(counts[TEAM_RED] - counts[TEAM_BLUE] > 1)
		{//red team has too many players
			changeto = TEAM_BLUE;
			changefrom = TEAM_RED;
		}
		else if(counts[TEAM_BLUE] - counts[TEAM_RED] > 1)
		{//blue team has too many players
			changeto = TEAM_RED;
			changefrom = TEAM_BLUE;
		}

		if(g_teamForceBalance.integer >= 3 && g_gametype.integer != GT_SIEGE)
		{//check the scores 
			if(level.teamScores[TEAM_BLUE] - OJP_PointSpread() >= level.teamScores[TEAM_RED] 
				&& counts[TEAM_BLUE] > counts[TEAM_RED])
			{//blue team is ahead but also has more players
				ScoreBalance = qtrue;
				changeto = TEAM_RED;
				changefrom = TEAM_BLUE;
			}
			else if(level.teamScores[TEAM_RED] - OJP_PointSpread() >= level.teamScores[TEAM_BLUE] 
				&& counts[TEAM_RED] > counts[TEAM_BLUE])
			{//red team is ahead but also has more players
				ScoreBalance = qtrue;
				changeto = TEAM_BLUE;
				changefrom = TEAM_RED;
			}
		}

		//try to even the teams if it's unbalanced
		if( changeto != -1 && changefrom != -1 )
		{//find the youngest player on changefrom team and place them on changeto's team
			gentity_t *ent = FindYoungestPlayeronTeam(changefrom, qfalse);

			if(ent)
			{//switch teams
				//[ClientNumFix]
				trap_SendServerCommand( ent-g_entities, "cp \"You were Team Switched for Game Balance\n\"" );
				//trap_SendServerCommand( ent->client->ps.clientNum, "cp \"You were Team Switched for Game Balance\n\"" );
				//[/ClientNumFix]
				SetTeam(ent, "");
				if(!ScoreBalance)
				{
					trap_SendServerCommand(-1, va("print \"AutoGameBalancer: Moved %s" S_COLOR_WHITE " to %s Team because the teams were uneven.\n\"",ent->client->pers.netname, TeamName(changeto)));
				}
				else
				{
					trap_SendServerCommand(-1, va("print \"AutoGameBalancer: Moved %s" S_COLOR_WHITE " to %s Team because the %s Team is ahead.\n\"",ent->client->pers.netname, TeamName(changeto), TeamName(changefrom)));
				}
			}
		}
		else if(g_teamForceBalance.integer == 4)
		{//check for the human/bot ratio on the teams since the teams are already balanced.
			CheckTeamBotBalance();
		}
	}
}


//[CoOp]
qboolean G_StandardHumanoid( gentity_t *self )
{
	char GLAName[MAX_QPATH];

	if ( !self || !self->ghoul2 )
	{
		return qfalse;
	}

	trap_G2API_GetGLAName( &self->ghoul2, 0, GLAName );
	assert(GLAName[0]);
	if ( GLAName[0] ) 
	{
		if ( !Q_stricmpn( "models/players/_humanoid", GLAName, 24 ) )///_humanoid", GLAName, 36) )
		{//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/protocol/protocol", GLAName ) )
		{//protocol droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/assassin_droid/model", GLAName ) )
		{//assassin_droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/saber_droid/model", GLAName ) )
		{//saber_droid duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/hazardtrooper/hazardtrooper", GLAName ) )
		{//hazardtrooper duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/rockettrooper/rockettrooper", GLAName ) )
		{//rockettrooper duplicates many of these
			return qtrue;
		}
		if ( !Q_stricmp( "models/players/wampa/wampa", GLAName ) )
		{//rockettrooper duplicates many of these
			return qtrue;
		}
	}
	return qfalse;
}
//[/CoOp]


//[ClientPlugInDetect]
qboolean OJP_AllPlayersHaveClientPlugin(void)
{//this function checks to see if all players are running OJP on their local systems or not.
	int i;
	for(i = 0; i < level.maxclients; i++)
	{
		if(g_entities[i].inuse && !g_entities[i].client->pers.ojpClientPlugIn)
		{//a live player that doesn't have the plugin
			return qfalse;
		}
	}

	return qtrue;
}
//[/ClientPlugInDetect]


//[LastManStanding]
qboolean LMS_EnoughPlayers()
{//checks to see if there's enough players in game to enable LMS rules
	if(level.numNonSpectatorClients < 2)
	{//definitely not enough.
		return qfalse;
	}

	if(g_gametype.integer >= GT_TEAM 
		&& (TeamCount( -1, TEAM_RED ) == 0 || TeamCount( -1, TEAM_BLUE ) == 0) )
	{//have to have at least one player on each team to be able to play LMS
		return qfalse;
	}

	return qtrue;
}
//[/LastManStanding]

