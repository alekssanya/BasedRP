//[SPPortComplete]
//NPC_behavior.cpp
/*
FIXME - MCG:
These all need to make use of the snapshots.  Write something that can look for only specific
things in a snapshot or just go through the snapshot every frame and save the info in case
we need it...
*/

#include "b_local.h"
#include "g_nav.h"
#include "../icarus/Q3_Interface.h"

extern	qboolean	showBBoxes;
extern vec3_t NPCDEBUG_BLUE;
extern void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void NPC_CheckGetNewWeapon( void );

extern qboolean PM_InKnockDown( playerState_t *ps );

extern void NPC_AimAdjust( int change );
extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);
/*
 void NPC_BSAdvanceFight (void)

Advance towards your captureGoal and shoot anyone you can along the way.
*/
void NPC_BSAdvanceFight (void)
{//FIXME: IMPLEMENT
//Head to Goal if I can

	//Make sure we're still headed where we want to capture
	if ( NPCInfo->captureGoal )
	{//FIXME: if no captureGoal, what do we do?
		//VectorCopy( NPCInfo->captureGoal->r.currentOrigin, NPCInfo->tempGoal->r.currentOrigin );
		//NPCInfo->goalEntity = NPCInfo->tempGoal;

		NPC_SetMoveGoal( NPC, NPCInfo->captureGoal->r.currentOrigin, 16, qtrue, -1, NULL );

//		NAV_ClearLastRoute(NPC);
		NPCInfo->goalTime = level.time + 100000;
	}

//	NPC_BSRun();

	NPC_CheckEnemy(qtrue, qfalse, qtrue);

	//FIXME: Need melee code
	if( NPC->enemy )
	{//See if we can shoot him
		vec3_t		delta, forward;
		vec3_t		angleToEnemy;
		vec3_t		hitspot, muzzle, diff, enemy_org, enemy_head;
		float		distanceToEnemy;
		qboolean	attack_ok = qfalse;
		qboolean	dead_on = qfalse;
		float		attack_scale = 1.0;
		float		aim_off;
		float		max_aim_off = 64;

		//Yaw to enemy
		VectorMA(NPC->enemy->r.absmin, 0.5, NPC->enemy->r.maxs, enemy_org);
		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		
		VectorSubtract (enemy_org, muzzle, delta);
		vectoangles ( delta, angleToEnemy );
		distanceToEnemy = VectorNormalize(delta);

		if(!NPC_EnemyTooFar(NPC->enemy, distanceToEnemy*distanceToEnemy, qtrue))
		{
			attack_ok = qtrue;
		}

		if(attack_ok)
		{
			NPC_UpdateShootAngles(angleToEnemy, qfalse, qtrue);

			NPCInfo->enemyLastVisibility = enemyVisibility;
			enemyVisibility = NPC_CheckVisibility ( NPC->enemy, CHECK_FOV);//CHECK_360|//CHECK_PVS|

			if(enemyVisibility == VIS_FOV)
			{//He's in our FOV
				
				attack_ok = qtrue;
				CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_head);

				if(attack_ok)
				{
					trace_t		tr;
					gentity_t	*traceEnt;
					//are we gonna hit him if we shoot at his center?
					trap_Trace ( &tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_SHOT );
					traceEnt = &g_entities[tr.entityNum];
					if( traceEnt != NPC->enemy &&
						(!traceEnt || !traceEnt->client || !NPC->client->enemyTeam || NPC->client->enemyTeam != traceEnt->client->playerTeam) )
					{//no, so shoot for the head
						attack_scale *= 0.75;
						trap_Trace ( &tr, muzzle, NULL, NULL, enemy_head, NPC->s.number, MASK_SHOT );
						traceEnt = &g_entities[tr.entityNum];
					}

					VectorCopy( tr.endpos, hitspot );

					if( traceEnt == NPC->enemy || (traceEnt->client && NPC->client->enemyTeam && NPC->client->enemyTeam == traceEnt->client->playerTeam) )
					{
						dead_on = qtrue;
					}
					else
					{
						attack_scale *= 0.5;
						if(NPC->client->playerTeam)
						{
							if(traceEnt && traceEnt->client && traceEnt->client->playerTeam)
							{
								if(NPC->client->playerTeam == traceEnt->client->playerTeam)
								{//Don't shoot our own team
									attack_ok = qfalse;
								}
							}
						}
					}
				}

				if( attack_ok )
				{
					//ok, now adjust pitch aim
					VectorSubtract (hitspot, muzzle, delta);
					vectoangles ( delta, angleToEnemy );
					NPC->NPC->desiredPitch = angleToEnemy[PITCH];
					NPC_UpdateShootAngles(angleToEnemy, qtrue, qfalse);

					if( !dead_on )
					{//We're not going to hit him directly, try a suppressing fire
						//see if where we're going to shoot is too far from his origin
						AngleVectors (NPCInfo->shootAngles, forward, NULL, NULL);
						VectorMA ( muzzle, distanceToEnemy, forward, hitspot);
						VectorSubtract(hitspot, enemy_org, diff);
						aim_off = VectorLength(diff);
						if(aim_off > random() * max_aim_off)//FIXME: use aim value to allow poor aim?
						{
							attack_scale *= 0.75;
							//see if where we're going to shoot is too far from his head
							VectorSubtract(hitspot, enemy_head, diff);
							aim_off = VectorLength(diff);
							if(aim_off > random() * max_aim_off)
							{
								attack_ok = qfalse;
							}
						}
						attack_scale *= (max_aim_off - aim_off + 1)/max_aim_off;
					}
				}
			}
		}

		if( attack_ok )
		{
			if( NPC_CheckAttack( attack_scale ))
			{//check aggression to decide if we should shoot
				enemyVisibility = VIS_SHOOT;
				WeaponThink(qtrue);
			}
			else
				attack_ok = qfalse;
		}
//Don't do this- only for when stationary and trying to shoot an enemy
//		else
//			NPC->cantHitEnemyCounter++;
	}
	else
	{//FIXME: 
		NPC_UpdateShootAngles(NPC->client->ps.viewangles, qtrue, qtrue);
	}

	if(!ucmd.forwardmove && !ucmd.rightmove)
	{//We reached our captureGoal
		if(trap_ICARUS_IsInitialized(NPC->s.number))
		{
			trap_ICARUS_TaskIDComplete( NPC, TID_BSTATE );
		}
	}
}

//[CoOp]
/* Not Used By Anything
void Disappear(gentity_t *self)
{//RACC - make self become invisible and quit thinking.  Used for BeamOut().
//	ClientDisconnect(self);
	self->s.eFlags |= EF_NODRAW;
	self->think = 0;
	self->nextthink = -1;
}

void MakeOwnerInvis (gentity_t *self);
void BeamOut (gentity_t *self)
{
//	gentity_t *tent = G_Spawn();
	
*//*
	tent->owner = self;
	tent->think = MakeOwnerInvis;
	tent->nextthink = level.time + 1800;
	//G_AddEvent( ent, EV_PLAYER_TELEPORT, 0 );
	tent = G_TempEntity( self->client->pcurrentOrigin, EV_PLAYER_TELEPORT );
*//*
	//fixme: doesn't actually go away!
	self->nextthink = level.time + 1500;
	self->think = Disappear;
	self->client->squadname = NULL;
	self->client->playerTeam = self->s.teamowner = TEAM_FREE;
	//self->r.svFlags |= SVF_BEAMING; //this appears unused in SP as well
}
*/
//[/CoOp]

void NPC_BSCinematic( void ) 
{//RACC - AI Cinematic Behavior State.  
	//Basically, just follow scripting orders and don't think.

	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}
	//[CoOp]
	/* RAFIXME - Impliment flag
	if (NPCInfo->scriptFlags&SCF_FIRE_WEAPON_NO_ANIM)
	{
		if (TIMER_Done(NPC, "NoAnimFireDelay"))
		{
			TIMER_Set(NPC, "NoAnimFireDelay", NPC_AttackDebounceForWeapon());
			FireWeapon(NPC, (NPCInfo->scriptFlags&SCF_ALT_FIRE)) ;
		}
	}
	*/
	//[/CoOp]

	if ( UpdateGoal() )
	{//have a goalEntity
		//move toward goal, should also face that goal
		NPC_MoveToGoal( qtrue );
	}

	if ( NPCInfo->watchTarget )
	{//have an entity which we want to keep facing
		//NOTE: this will override any angles set by NPC_MoveToGoal
		vec3_t eyes, viewSpot, viewvec, viewangles;

		CalcEntitySpot( NPC, SPOT_HEAD_LEAN, eyes );
		CalcEntitySpot( NPCInfo->watchTarget, SPOT_HEAD_LEAN, viewSpot );

		VectorSubtract( viewSpot, eyes, viewvec );
		
		vectoangles( viewvec, viewangles );

		NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw = viewangles[YAW];
		NPCInfo->lockedDesiredPitch = NPCInfo->desiredPitch = viewangles[PITCH];
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSWait( void ) 
{
	NPC_UpdateAngles( qtrue, qtrue );
}


//[CoOp]
/* Not Used By Anything
void NPC_BSInvestigate (void)
{
*//*
	//FIXME: maybe allow this to be set as a tempBState in a script?  Just specify the
	//investigateGoal, investigateDebounceTime and investigateCount? (Needs a macro)
	vec3_t		invDir, invAngles, spot;
	gentity_t	*saveGoal;
	//BS_INVESTIGATE would turn toward goal, maybe take a couple steps towards it,
	//look for enemies, then turn away after your investigate counter was down-
	//investigate counter goes up every time you set it...

	if(level.time > NPCInfo->enemyCheckDebounceTime)
	{
		NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 1000);
		NPC_CheckEnemy(qtrue, qfalse);
		if(NPC->enemy)
		{//FIXME: do anger script
			NPCInfo->goalEntity = NPC->enemy;
//			NAV_ClearLastRoute(NPC);
			NPCInfo->behaviorState = BS_RUN_AND_SHOOT;
			NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_AngerSound();
			return;
		}
	}

	NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );

	if(NPCInfo->stats.vigilance <= 1.0 && NPCInfo->eventOwner)
	{
		VectorCopy(NPCInfo->eventOwner->r.currentOrigin, NPCInfo->investigateGoal);
	}

	saveGoal = NPCInfo->goalEntity;
	if(	level.time > NPCInfo->walkDebounceTime )
	{
		vec3_t	vec;

		VectorSubtract(NPCInfo->investigateGoal, NPC->r.currentOrigin, vec);
		vec[2] = 0;
		if(VectorLength(vec) > 64)
		{
			if(Q_irand(0, 100) < NPCInfo->investigateCount)
			{//take a full step
				//NPCInfo->walkDebounceTime = level.time + 1400;
				//actually finds length of my BOTH_WALK anim
				NPCInfo->walkDebounceTime = PM_AnimLength( NPC->client->clientInfo.animFileIndex, BOTH_WALK1 );
			}
		}
	}

	if(	level.time < NPCInfo->walkDebounceTime )
	{//walk toward investigateGoal
		
		*//*
		NPCInfo->goalEntity = NPCInfo->tempGoal;
//		NAV_ClearLastRoute(NPC);
		VectorCopy(NPCInfo->investigateGoal, NPCInfo->tempGoal->r.currentOrigin);
		*/

/*		NPC_SetMoveGoal( NPC, NPCInfo->investigateGoal, 16, qtrue );

		NPC_MoveToGoal( qtrue );

		//FIXME: walk2?
		NPC_SetAnim(NPC,SETANIM_LEGS,BOTH_WALK1,SETANIM_FLAG_NORMAL);

		ucmd.buttons |= BUTTON_WALKING;
	}
	else
	{

		NPC_SetAnim(NPC,SETANIM_LEGS,BOTH_STAND1,SETANIM_FLAG_NORMAL);

		if(NPCInfo->hlookCount > 30)
		{
			if(Q_irand(0, 10) > 7) 
			{
				NPCInfo->hlookCount = 0;
			}
		}
		else if(NPCInfo->hlookCount < -30)
		{
			if(Q_irand(0, 10) > 7) 
			{
				NPCInfo->hlookCount = 0;
			}
		}
		else if(NPCInfo->hlookCount == 0)
		{
			NPCInfo->hlookCount = Q_irand(-1, 1);
		}
		else if(Q_irand(0, 10) > 7) 
		{
			if(NPCInfo->hlookCount > 0)
			{
				NPCInfo->hlookCount++;
			}
			else//lookCount < 0
			{
				NPCInfo->hlookCount--;
			}
		}

		if(NPCInfo->vlookCount >= 15)
		{
			if(Q_irand(0, 10) > 7) 
			{
				NPCInfo->vlookCount = 0;
			}
		}
		else if(NPCInfo->vlookCount <= -15)
		{
			if(Q_irand(0, 10) > 7) 
			{
				NPCInfo->vlookCount = 0;
			}
		}
		else if(NPCInfo->vlookCount == 0)
		{
			NPCInfo->vlookCount = Q_irand(-1, 1);
		}
		else if(Q_irand(0, 10) > 8) 
		{
			if(NPCInfo->vlookCount > 0)
			{
				NPCInfo->vlookCount++;
			}
			else//lookCount < 0
			{
				NPCInfo->vlookCount--;
			}
		}

		//turn toward investigateGoal
		CalcEntitySpot( NPC, SPOT_HEAD, spot );
		VectorSubtract(NPCInfo->investigateGoal, spot, invDir);
		VectorNormalize(invDir);
		vectoangles(invDir, invAngles);
		NPCInfo->desiredYaw = AngleNormalize360(invAngles[YAW] + NPCInfo->hlookCount);
		NPCInfo->desiredPitch = AngleNormalize360(invAngles[PITCH] + NPCInfo->hlookCount);
	}

	NPC_UpdateAngles(qtrue, qtrue);

	NPCInfo->goalEntity = saveGoal;
//	NAV_ClearLastRoute(NPC);

	if(level.time > NPCInfo->investigateDebounceTime)
	{
		NPCInfo->tempBehavior = BS_DEFAULT;
	}

	NPC_CheckSoundEvents();
	*//*
}


qboolean NPC_CheckInvestigate( int alertEventNum )
{
	gentity_t	*owner = level.alertEvents[alertEventNum].owner;
	int		invAdd = level.alertEvents[alertEventNum].level;
	vec3_t	soundPos;
	float	soundRad = level.alertEvents[alertEventNum].radius;
	float	earshot = NPCInfo->stats.earshot;

	VectorCopy( level.alertEvents[alertEventNum].position, soundPos );

	//NOTE: Trying to preserve previous investigation behavior
	if ( !owner )
	{
		return qfalse;
	}

	if ( owner->s.eType != ET_PLAYER && owner->s.eType != ET_NPC && owner == NPCInfo->goalEntity ) 
	{
		return qfalse;
	}

	if ( owner->s.eFlags & EF_NODRAW ) 
	{
		return qfalse;
	}

	if ( owner->flags & FL_NOTARGET ) 
	{
		return qfalse;
	}

	if ( soundRad < earshot )
	{
		return qfalse;
	}

	//if(!trap_InPVSIgnorePortals(ent->r.currentOrigin, NPC->r.currentOrigin))//should we be able to hear through areaportals?
	if ( !trap_InPVS( soundPos, NPC->r.currentOrigin ) )
	{//can hear through doors?
		return qfalse;
	}

	if ( owner->client && owner->client->playerTeam && NPC->client->playerTeam && owner->client->playerTeam != NPC->client->playerTeam )
	{
		if( (float)NPCInfo->investigateCount >= (NPCInfo->stats.vigilance*200) && owner )
		{//If investigateCount == 10, just take it as enemy and go
			if ( ValidEnemy( owner ) )
			{//FIXME: run angerscript
				G_SetEnemy( NPC, owner );
				NPCInfo->goalEntity = NPC->enemy;
				NPCInfo->goalRadius = 12;
				NPCInfo->behaviorState = BS_HUNT_AND_KILL;
				return qtrue;
			}
		}
		else
		{
			NPCInfo->investigateCount += invAdd;
		}
		//run awakescript
		G_ActivateBehavior(NPC, BSET_AWAKE);

		*//*
		if ( Q_irand(0, 10) > 7 )
		{
			NPC_AngerSound();
		}
		*//*

		//NPCInfo->hlookCount = NPCInfo->vlookCount = 0;
		NPCInfo->eventOwner = owner;
		VectorCopy( soundPos, NPCInfo->investigateGoal );
		if ( NPCInfo->investigateCount > 20 )
		{
			NPCInfo->investigateDebounceTime = level.time + 10000;
		}
		else
		{
			NPCInfo->investigateDebounceTime = level.time + (NPCInfo->investigateCount*500);
		}
		NPCInfo->tempBehavior = BS_INVESTIGATE;
		return qtrue;
	}

	return qfalse;
}
*/
//[/CoOp]


/*
void NPC_BSSleep( void ) 
*/
void NPC_BSSleep( void ) 
{//RACC - Follow a path, looking for enemies
	//[CoOp]
	int alertEvent = NPC_CheckAlertEvents( qtrue, qfalse, -1, qfalse, AEL_MINOR, qfalse );
	//int alertEvent = NPC_CheckAlertEvents( qtrue, qfalse, -1, qfalse, AEL_MINOR );
	//[/CoOp]

	//There is an event to look at
	if ( alertEvent >= 0 )
	{
		G_ActivateBehavior(NPC, BSET_AWAKE);
		return;
	}

	/*
	if ( level.time > NPCInfo->enemyCheckDebounceTime )
	{
		if ( NPC_CheckSoundEvents() != -1 )
		{//only 1 alert per second per 0.1 of vigilance
			NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 10000);
			G_ActivateBehavior(NPC, BSET_AWAKE);
		}
	}
	*/
}

extern qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset );

//[CoOp]
qboolean NPC_BSFollowLeader_UpdateLeader(void)
{//racc - checks the status of our leader.  If the leader is invalid, do some backup behavior.
	
	if ( NPC->client->leader//have a leader
		&& NPC->client->leader->s.number < MAX_CLIENTS //player
		&& NPC->client->leader->client//player is a client
		&& !NPC->client->leader->client->pers.enterTime )//player has not finished spawning in yet
	{//don't do anything just yet, but don't clear the leader either
		return qfalse;
	}

	if (NPC->client->leader && NPC->client->leader->health<=0)
	{
		//[test]
		if(NPC->client->leader->s.number < MAX_CLIENTS)
		{//leader is a player.  Check to look for another player on this 
			//team to follow.  Otherwise just wait, we don't want to lose 
			//our leader.
			gentity_t *ClosestPlayer = FindClosestPlayer(NPC->client->leader->r.currentOrigin, NPC->client->playerTeam);
			if(ClosestPlayer)
			{
				NPC->client->leader = ClosestPlayer;
			}
		}
		else
		{
			NPC->client->leader = NULL;
		}
		//NPC->client->leader = NULL;
		//[/test]
	}

	if ( !NPC->client->leader )
	{//ok, stand guard until we find an enemy
		if( NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else
		{
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
		}
		if ( NPCInfo->behaviorState == BS_FOLLOW_LEADER )
		{
			NPCInfo->behaviorState = BS_DEFAULT; 
		}
		if ( NPCInfo->defaultBehavior == BS_FOLLOW_LEADER )
		{
			NPCInfo->defaultBehavior = BS_DEFAULT; 
		}
		return qfalse;
	}
	return qtrue; 
}


void NPC_BSFollowLeader_UpdateEnemy(void)
{
	if ( !NPC->enemy  )
	{//no enemy, find one
		NPC_CheckEnemy( NPCInfo->confusionTime<level.time, qfalse, qtrue );//don't find new enemy if this is tempbehav
		if ( NPC->enemy )
		{//just found one
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else
		{
			if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
			{//RACC - check for enemies that you can see thru alerts.
				//[CoOp]
				int eventID = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR, qfalse );
				//int eventID = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR );
				//[/CoOp]
				if ( eventID > -1 && level.alertEvents[eventID].level >= AEL_SUSPICIOUS && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				{
					//NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
					if ( !level.alertEvents[eventID].owner || 
						!level.alertEvents[eventID].owner->client || 
						level.alertEvents[eventID].owner->health <= 0 ||
						level.alertEvents[eventID].owner->client->playerTeam != NPC->client->enemyTeam )
					{//not an enemy
					}
					else
					{//RACC - noticed an enemy.
						//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
						G_SetEnemy( NPC, level.alertEvents[eventID].owner );
						NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
						NPCInfo->enemyLastSeenTime = level.time;
						TIMER_Set( NPC, "attackDelay", Q_irand( 500, 1000 ) );
					}
				}

			}
		}
		if ( !NPC->enemy )
		{//racc - still no dice.
			if ( NPC->client->leader 
				&& NPC->client->leader->enemy 
				&& NPC->client->leader->enemy != NPC
				//[RAFIXME] - impliment these flags.
				&& (NPC->client->leader->enemy->client&&NPC->client->leader->enemy->client->playerTeam==NPC->client->enemyTeam)
				//&& ( (NPC->client->leader->enemy->client&&NPC->client->leader->enemy->client->playerTeam==NPC->client->enemyTeam)
				//	||(NPC->client->leader->enemy->svFlags&SVF_NONNPC_ENEMY&&NPC->client->leader->enemy->noDamageTeam==NPC->client->enemyTeam) )
				//[/RAFIXME]
				&& NPC->client->leader->enemy->health > 0 )
			{//racc - our leader has a valid enemy.  Attack them.
				G_SetEnemy( NPC, NPC->client->leader->enemy );
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
				NPCInfo->enemyLastSeenTime = level.time;
			}
		}
	}
	else 
	{//already have an enemy targeted
		if ( NPC->enemy->health <= 0 || (NPC->enemy->flags&FL_NOTARGET) )
		{//dead enemy or can't target this enemy anymore.
			G_ClearEnemy( NPC );
			if ( NPCInfo->enemyCheckDebounceTime > level.time + 1000 )
			{//refresh the debounce if it's already fairly active.
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 2000 );
			}
		}
		else if ( NPC->client->ps.weapon && NPCInfo->enemyCheckDebounceTime < level.time )
		{//we have a weapon and we need to check for an enemy.
			NPC_CheckEnemy( (NPCInfo->confusionTime<level.time||NPCInfo->tempBehavior!=BS_FOLLOW_LEADER), qfalse, qtrue );//don't find new enemy if this is tempbehav
		}
	}
}


qboolean NPC_BSFollowLeader_AttackEnemy(void)
{//attack our enemy!
	if ( NPC->client->ps.weapon == WP_SABER )//|| NPCInfo->confusionTime>level.time )
	{//lightsaber user or charmed enemy
		if ( NPCInfo->tempBehavior != BS_FOLLOW_LEADER )
		{//not already in a temp bState
			//go after the guy
			NPCInfo->tempBehavior = BS_HUNT_AND_KILL;
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
	}

	enemyVisibility = NPC_CheckVisibility ( NPC->enemy, CHECK_FOV|CHECK_SHOOT );//CHECK_360|CHECK_PVS|
	if ( enemyVisibility > VIS_PVS )
	{//face
		vec3_t	enemy_org, muzzle, delta, angleToEnemy;
		float	distanceToEnemy;

		CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org );
		NPC_AimWiggle( enemy_org );

		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		
		VectorSubtract( enemy_org, muzzle, delta);
		vectoangles( delta, angleToEnemy );
		distanceToEnemy = VectorNormalize( delta );

		NPCInfo->desiredYaw = angleToEnemy[YAW];
		NPCInfo->desiredPitch = angleToEnemy[PITCH];
		NPC_UpdateFiringAngles( qtrue, qtrue );

		if ( enemyVisibility >= VIS_SHOOT )
		{//shoot
			NPC_AimAdjust( 2 );
			if ( NPC_GetHFOVPercentage( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, NPCInfo->stats.hfov ) > 0.6f 
				&& NPC_GetHFOVPercentage( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, NPCInfo->stats.vfov ) > 0.5f )
			{//actually withing our front cone
				WeaponThink( qtrue );
			}
		}
		else
		{//focus more on aiming
			NPC_AimAdjust( 1 );
		}
		
		//NPC_CheckCanAttack(1.0, qfalse);
	}
	else
	{//defocus aim
		NPC_AimAdjust( -1 );
	}
	return qfalse;
}


qboolean NPC_BSFollowLeader_CanAttack(void)
{//check to see if we can attack
	 return (NPC->enemy 
		&& NPC->client->ps.weapon 
		&& !(NPCInfo->aiFlags&NPCAI_HEAL_ROSH) //Kothos twins never go after their enemy
		);
}

qboolean NPC_BSFollowLeader_InFullBodyAttack(void)
{//check to see if we're melee attacking.  Used to prevent us from accidently hitting
	//our leader I think.
	return (
		NPC->client->ps.legsAnim==BOTH_ATTACK1 || 
		NPC->client->ps.legsAnim==BOTH_ATTACK2 || 
		NPC->client->ps.legsAnim==BOTH_ATTACK3 || 
		NPC->client->ps.legsAnim==BOTH_MELEE1 || 
		NPC->client->ps.legsAnim==BOTH_MELEE2
		);
}


void NPC_BSFollowLeader_LookAtLeader(void)
{//look at our leader
	vec3_t	head, leaderHead, delta, angleToLeader;

	CalcEntitySpot( NPC->client->leader, SPOT_HEAD, leaderHead );
	CalcEntitySpot( NPC, SPOT_HEAD, head );
	VectorSubtract (leaderHead, head, delta);
	vectoangles ( delta, angleToLeader );
	VectorNormalize(delta);
	NPC->NPC->desiredYaw = angleToLeader[YAW];
	NPC->NPC->desiredPitch = angleToLeader[PITCH];
	
	NPC_UpdateAngles(qtrue, qtrue);
}
//[/CoOp]


void NPC_BSFollowLeader (void)
{
	vec3_t		vec;
	float		leaderDist;
	visibility_t	leaderVis;
	//[CoOp]
	//int			curAnim;
	


	// If In A Jump, Return
	//----------------------
	if (NPC_Jumping())
	{
		return;
	}

	// If There Is No Leader, Return
	//-------------------------------
	if (!NPC_BSFollowLeader_UpdateLeader())
	{
		return;
	}

	// Don't Do Anything Else If In A Full Body Attack
	//-------------------------------------------------
	if (NPC_BSFollowLeader_InFullBodyAttack())
	{
		return;
	}

	// Update The Enemy
	//------------------
	NPC_BSFollowLeader_UpdateEnemy();


	// Do Any Attacking
	//------------------
	if (NPC_BSFollowLeader_CanAttack())
	{
		if (NPC_BSFollowLeader_AttackEnemy())
		{
			return;
		}
	}
	else
	{
		NPC_BSFollowLeader_LookAtLeader();
	}

	/* Replaced all this stuff with easier to follow function calls
	if ( !NPC->client->leader )
	{//ok, stand guard until we find an enemy
		if( NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else
		{
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
		}
		return;
	}

	if ( !NPC->enemy  )
	{//no enemy, find one
		NPC_CheckEnemy( NPCInfo->confusionTime<level.time, qfalse, qtrue );//don't find new enemy if this is tempbehav
		if ( NPC->enemy )
		{//just found one
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
		}
		else
		{
			if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
			{
				int eventID = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR );
				//[CoOp]
				//don't do anything if nothing happened.
				if ( eventID >= 0 && level.alertEvents[eventID].level >= AEL_SUSPICIOUS && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				//if ( level.alertEvents[eventID].level >= AEL_SUSPICIOUS && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				//[/CoOp]
				{
					NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
					if ( !level.alertEvents[eventID].owner || 
						!level.alertEvents[eventID].owner->client || 
						level.alertEvents[eventID].owner->health <= 0 ||
						level.alertEvents[eventID].owner->client->playerTeam != NPC->client->enemyTeam )
					{//not an enemy
					}
					else
					{
						//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
						G_SetEnemy( NPC, level.alertEvents[eventID].owner );
						NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
						NPCInfo->enemyLastSeenTime = level.time;
						TIMER_Set( NPC, "attackDelay", Q_irand( 500, 1000 ) );
					}
				}

			}
		}
		if ( !NPC->enemy )
		{
			if ( NPC->client->leader 
				&& NPC->client->leader->enemy 
				&& NPC->client->leader->enemy != NPC
				&& ( (NPC->client->leader->enemy->client&&NPC->client->leader->enemy->client->playerTeam==NPC->client->enemyTeam)
					||(*//*NPC->client->leader->enemy->r.svFlags&SVF_NONNPC_ENEMY*//*0&&NPC->client->leader->enemy->alliedTeam==NPC->client->enemyTeam) )
				&& NPC->client->leader->enemy->health > 0 )
			{ //rwwFIXMEFIXME: use SVF_NONNPC_ENEMY?
				G_SetEnemy( NPC, NPC->client->leader->enemy );
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
				NPCInfo->enemyLastSeenTime = level.time;
			}
		}
	}
	else 
	{
		if ( NPC->enemy->health <= 0 || (NPC->enemy->flags&FL_NOTARGET) )
		{
			G_ClearEnemy( NPC );
			if ( NPCInfo->enemyCheckDebounceTime > level.time + 1000 )
			{
				NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 2000 );
			}
		}
		else if ( NPC->client->ps.weapon && NPCInfo->enemyCheckDebounceTime < level.time )
		{
			NPC_CheckEnemy( (NPCInfo->confusionTime<level.time||NPCInfo->tempBehavior!=BS_FOLLOW_LEADER), qfalse, qtrue );//don't find new enemy if this is tempbehav
		}
	}
	
	if ( NPC->enemy && NPC->client->ps.weapon )
	{//If have an enemy, face him and fire
		if ( NPC->client->ps.weapon == WP_SABER )//|| NPCInfo->confusionTime>level.time )
		{//lightsaber user or charmed enemy
			if ( NPCInfo->tempBehavior != BS_FOLLOW_LEADER )
			{//not already in a temp bState
				//go after the guy
				NPCInfo->tempBehavior = BS_HUNT_AND_KILL;
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}

		enemyVisibility = NPC_CheckVisibility ( NPC->enemy, CHECK_FOV|CHECK_SHOOT );//CHECK_360|CHECK_PVS|
		if ( enemyVisibility > VIS_PVS )
		{//face
			vec3_t	enemy_org, muzzle, delta, angleToEnemy;
			float	distanceToEnemy;

			CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org );
			NPC_AimWiggle( enemy_org );

			CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
			
			VectorSubtract( enemy_org, muzzle, delta);
			vectoangles( delta, angleToEnemy );
			distanceToEnemy = VectorNormalize( delta );

			NPCInfo->desiredYaw = angleToEnemy[YAW];
			NPCInfo->desiredPitch = angleToEnemy[PITCH];
			NPC_UpdateFiringAngles( qtrue, qtrue );

			if ( enemyVisibility >= VIS_SHOOT )
			{//shoot
				NPC_AimAdjust( 2 );
				if ( NPC_GetHFOVPercentage( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, NPCInfo->stats.hfov ) > 0.6f 
					&& NPC_GetHFOVPercentage( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, NPCInfo->stats.vfov ) > 0.5f )
				{//actually withing our front cone
					WeaponThink( qtrue );
				}
			}
			else
			{
				NPC_AimAdjust( 1 );
			}
			
			//NPC_CheckCanAttack(1.0, qfalse);
		}
		else
		{
			NPC_AimAdjust( -1 );
		}
	}
	else
	{//FIXME: combine with vector calc below
		vec3_t	head, leaderHead, delta, angleToLeader;

		CalcEntitySpot( NPC->client->leader, SPOT_HEAD, leaderHead );
		CalcEntitySpot( NPC, SPOT_HEAD, head );
		VectorSubtract (leaderHead, head, delta);
		vectoangles ( delta, angleToLeader );
		VectorNormalize(delta);
		NPC->NPC->desiredYaw = angleToLeader[YAW];
		NPC->NPC->desiredPitch = angleToLeader[PITCH];
		
		NPC_UpdateAngles(qtrue, qtrue);
	}
	*/
	//[/CoOp]

	//[RAFIXME] - Replace the below with SP Navigation code!

	//leader visible?
	leaderVis = NPC_CheckVisibility( NPC->client->leader, CHECK_PVS|CHECK_360|CHECK_SHOOT );//			ent->e_UseFunc = useF_NULL;


	//Follow leader, stay within visibility and a certain distance, maintain a distance from.
	//[CoOp]
	//this check is done earlier.
	//curAnim = NPC->client->ps.legsAnim;
	//if ( curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 )
	//[/CoOp]
	{//Don't move toward leader if we're in a full-body attack anim
		//FIXME, use IdealDistance to determine if we need to close distance
		//[CoOp]
		float	followDist = (NPCInfo->followDist)?(NPCInfo->followDist):(110.0f);
		//float	followDist = 96.0f;//FIXME:  If there are enmies, make this larger?
		//[/CoOp]
		float	backupdist, walkdist, minrundist;
		float	leaderHDist;

		if ( NPCInfo->followDist )
		{
			followDist = NPCInfo->followDist;
		}
		backupdist = followDist/2.0f;
		walkdist = followDist*0.83;
		minrundist = followDist*1.33;

		VectorSubtract(NPC->client->leader->r.currentOrigin, NPC->r.currentOrigin, vec);
		leaderDist = VectorLength( vec );//FIXME: make this just nav distance?
		//never get within their radius horizontally
		vec[2] = 0;
		leaderHDist = VectorLength( vec );
		if( leaderHDist > backupdist && (leaderVis != VIS_SHOOT || leaderDist > walkdist) )
		{//We should close in?
			NPCInfo->goalEntity = NPC->client->leader;

			NPC_SlideMoveToGoal();
			if ( leaderVis == VIS_SHOOT && leaderDist < minrundist )
			{
				ucmd.buttons |= BUTTON_WALKING;
			}
		}
		else if ( leaderDist < backupdist )
		{//We should back off?
			NPCInfo->goalEntity = NPC->client->leader;
			NPC_SlideMoveToGoal();

			//reversing direction
			ucmd.forwardmove = -ucmd.forwardmove;
			ucmd.rightmove   = -ucmd.rightmove;
			VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
		}//otherwise, stay where we are
		//check for do not enter and stop if there's one there...
		
		
		
		
		if ( ucmd.forwardmove || ucmd.rightmove || VectorCompare( vec3_origin, NPC->client->ps.moveDir ) )
		{
			NPC_MoveDirClear( ucmd.forwardmove, ucmd.rightmove, qtrue );
		}
		
	}

	//[/RAFIXME]
}
#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f
void NPC_BSJump (void)
{
	vec3_t		dir, angles, p1, p2, apex;
	float		time, height, forward, z, xy, dist, yawError, apexHeight;

	if( !NPCInfo->goalEntity )
	{//Should have task completed the navgoal
		return;
	}

	if ( NPCInfo->jumpState != JS_JUMPING && NPCInfo->jumpState != JS_LANDING )
	{
		//Face navgoal
		VectorSubtract(NPCInfo->goalEntity->r.currentOrigin, NPC->r.currentOrigin, dir);
		vectoangles(dir, angles);
		NPCInfo->desiredPitch = NPCInfo->lockedDesiredPitch = AngleNormalize360(angles[PITCH]);
		NPCInfo->desiredYaw = NPCInfo->lockedDesiredYaw = AngleNormalize360(angles[YAW]);
	}

	NPC_UpdateAngles ( qtrue, qtrue );
	yawError = AngleDelta ( NPC->client->ps.viewangles[YAW], NPCInfo->desiredYaw );
	//We don't really care about pitch here

	switch ( NPCInfo->jumpState )
	{
	case JS_FACING:
		if ( yawError < MIN_ANGLE_ERROR )
		{//Facing it, Start crouching
			NPC_SetAnim(NPC, SETANIM_LEGS, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			NPCInfo->jumpState = JS_CROUCHING;
		}
		break;
	case JS_CROUCHING:
		//racc - crouch before jump
		if ( NPC->client->ps.legsTimer > 0 )
		{//Still playing crouching anim
			return;
		}

		//racc - jump!

		//Create a parabola

		if ( NPC->r.currentOrigin[2] > NPCInfo->goalEntity->r.currentOrigin[2] )
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( NPCInfo->goalEntity->r.currentOrigin, p2 );
		}
		else if ( NPC->r.currentOrigin[2] < NPCInfo->goalEntity->r.currentOrigin[2] )
		{
			VectorCopy( NPCInfo->goalEntity->r.currentOrigin, p1 );
			VectorCopy( NPC->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( NPCInfo->goalEntity->r.currentOrigin, p2 );
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );
		z = p1[2] - p2[2];

		apexHeight = APEX_HEIGHT/2;
		/*
		//Determine most desirable apex height
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5 )
		{
			apexHeight = APEX_HEIGHT*0.5;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
			apexHeight = APEX_HEIGHT*2;
		}
		*/

		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH
		
		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		// Don't need to set apex xy if NPC is jumping directly up.
		if ( xy > 0.0f )
		{
			xy -= z;
			xy *= 0.5;

		  
			
  
			assert(xy > 0);
		}
		

		VectorMA( p1, xy, dir, apex );
		apex[2] += apexHeight;
	
		VectorCopy(apex, NPC->pos1);
		
		//Now we have the apex, aim for it
		height = apex[2] - NPC->r.currentOrigin[2];
		time = sqrt( height / ( .5 * NPC->client->ps.gravity ) );
		if ( !time ) 
		{
//			Com_Printf("ERROR no time in jump\n");
			return;
		}

		// set s.origin2 to the push velocity
		VectorSubtract ( apex, NPC->r.currentOrigin, NPC->client->ps.velocity );
		NPC->client->ps.velocity[2] = 0;
		dist = VectorNormalize( NPC->client->ps.velocity );

		forward = dist / time;
		VectorScale( NPC->client->ps.velocity, forward, NPC->client->ps.velocity );

		NPC->client->ps.velocity[2] = time * NPC->client->ps.gravity;

//		Com_Printf( "%s jumping %s, gravity at %4.0f percent\n", NPC->targetname, vtos(NPC->client->ps.velocity), NPC->client->ps.gravity/8.0f );

		//[CoOp] SP Code
		//NPC->flags |= FL_NO_KNOCKBACK;
		//[/CoOp]
		NPCInfo->jumpState = JS_JUMPING;
		//FIXME: jumpsound?
		break;
	case JS_JUMPING:
		//RACC - I'm flying!
		if ( showBBoxes )
		{
			VectorAdd(NPC->r.mins, NPC->pos1, p1);
			VectorAdd(NPC->r.maxs, NPC->pos1, p2);
			G_Cube( p1, p2, NPCDEBUG_BLUE, 0.5 );
		}

		if ( NPC->s.groundEntityNum != ENTITYNUM_NONE)
		{//Landed, start landing anim
			//FIXME: if the 
			VectorClear(NPC->client->ps.velocity);
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			NPCInfo->jumpState = JS_LANDING;
			//FIXME: landsound?
		}
		else if ( NPC->client->ps.legsTimer > 0 )
		{//Still playing jumping anim
			//FIXME: apply jump velocity here, a couple frames after start, not right away
			return;
		}
		else
		{//still in air, but done with jump anim, play inair anim
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE);
		}
		break;
	case JS_LANDING:
		if ( NPC->client->ps.legsTimer > 0 )
		{//Still playing landing anim
			return;
		}
		else
		{
			NPCInfo->jumpState = JS_WAITING;

			//[CoOp]
			NPCInfo->goalEntity = UpdateGoal();
			// If he made it to his goal or his task is no longer pending.
			if ( !NPCInfo->goalEntity || !trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) )
			{
				NPC_ClearGoal();
				NPCInfo->goalTime = level.time;
				NPCInfo->aiFlags &= ~NPCAI_MOVING;
				ucmd.forwardmove = 0;
				NPC->flags &= ~FL_NO_KNOCKBACK;
				//Return that the goal was reached
				trap_ICARUS_TaskIDComplete( NPC, TID_MOVE_NAV );
			}

			/*
			//task complete no matter what...  
			NPC_ClearGoal();
			NPCInfo->goalTime = level.time;
			NPCInfo->aiFlags &= ~NPCAI_MOVING;
			ucmd.forwardmove = 0;
			NPC->flags &= ~FL_NO_KNOCKBACK;
			//Return that the goal was reached
			trap_ICARUS_TaskIDComplete( NPC, TID_MOVE_NAV );
			*/
			//[/CoOp]

			//Or should we keep jumping until reached goal?
			
			/*
			NPCInfo->goalEntity = UpdateGoal();
			if ( !NPCInfo->goalEntity )
			{
				NPC->flags &= ~FL_NO_KNOCKBACK;
				Q3_TaskIDComplete( NPC, TID_MOVE_NAV );
			}
			*/
			
		}
		break;
	case JS_WAITING:
		//RACC - Finished Jump.  reset state to start state.
	default:
		NPCInfo->jumpState = JS_FACING;
		break;
	}
}


//[CoOp]
extern qboolean InPlayersPVS(vec3_t point);
//[/CoOp]
void NPC_BSRemove (void)
{//RACC - remove ourselves from the game world.  Just don't do it in front of players.
	NPC_UpdateAngles ( qtrue, qtrue );
	//[CoOp]
	if( !InPlayersPVS( NPC->r.currentOrigin ) )
	//if( !trap_InPVS( NPC->r.currentOrigin, g_entities[0].r.currentOrigin ) )//FIXME: use cg.vieworg?
	//[/CoOp]
	{ //rwwFIXMEFIXME: Care about all clients instead of just 0?
		G_UseTargets2( NPC, NPC, NPC->target3 );
		NPC->s.eFlags |= EF_NODRAW;
		//[CoOp]
		//[RAFIXME] - Impliment flag?
		//NPC->svFlags &= ~SVF_NPC;
		//[/RAFIXME]
		//[/CoOp]
		NPC->s.eType = ET_INVISIBLE;
		NPC->r.contents = 0;
		NPC->health = 0;
		NPC->targetname = NULL;

		//Disappear in half a second
		NPC->think = G_FreeEntity;
		NPC->nextthink = level.time + FRAMETIME;
	}//FIXME: else allow for out of FOV???
}


void NPC_BSSearch (void)
{//RACC - Using current waypoint as a base, search the immediate branches of waypoints for enemies
	//[CoOp]
	NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER, qfalse);
	//FIXME: do something with these alerts...?
	//FIXME: do the Stormtrooper alert reaction?  (investigation)
	if ( (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES)
		&& NPC->client->enemyTeam != NPCTEAM_NEUTRAL )
	{//look for enemies
		NPC_CheckEnemy(qtrue, qfalse, qtrue);
		if ( NPC->enemy )
		{//found one
			if( NPCInfo->tempBehavior == BS_SEARCH )
			{//if tempbehavior, set tempbehavior to default
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			else
			{//if bState, change to run and shoot
				NPCInfo->behaviorState = BS_DEFAULT;//BS_HUNT_AND_KILL;
				//NPC_BSRunAndShoot();
			}
			return;
		}
	}
	/*
	NPC_CheckEnemy(qtrue, qfalse, qtrue);
	//Look for enemies, if find one:
	if ( NPC->enemy )
	{
		if( NPCInfo->tempBehavior == BS_SEARCH )
		{//if tempbehavior, set tempbehavior to default
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else
		{//if bState, change to run and shoot
			NPCInfo->behaviorState = BS_HUNT_AND_KILL;
			NPC_BSRunAndShoot();
		}
		return;
	}
	*/
	//[/CoOp]

	//FIXME: what if our goalEntity is not NULL and NOT our tempGoal - they must
	//want us to do something else?  If tempBehavior, just default, else set
	//to run and shoot...?

	//FIXME: Reimplement

	if ( !NPCInfo->investigateDebounceTime )
	{//On our way to a tempGoal
		float	minGoalReachedDistSquared = 32*32;
		vec3_t	vec;

		//Keep moving toward our tempGoal
		NPCInfo->goalEntity = NPCInfo->tempGoal;

		VectorSubtract ( NPCInfo->tempGoal->r.currentOrigin, NPC->r.currentOrigin, vec);
		if ( vec[2] < 24 )
		{
			vec[2] = 0;
		}

		//[CoOp]
		//redundent code
		/*
		if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE )
		{
			*//*
			//FIXME: can't get the radius...
			float	wpRadSq = waypoints[NPCInfo->tempGoal->waypoint].radius * waypoints[NPCInfo->tempGoal->waypoint].radius;
			if ( minGoalReachedDistSquared > wpRadSq )
			{
				minGoalReachedDistSquared = wpRadSq;
			}
			*//*

			minGoalReachedDistSquared = 32*32;//12*12;
		}
		*/
		//[/CoOp]

		if ( VectorLengthSquared( vec ) < minGoalReachedDistSquared )
		{//RACC - reached goal.
			//Close enough, just got there
			//[CoOp]
			//[RAFIXME] - Impliment navigation code
			//NPC->waypoint = NAV::GetNearestNode(NPC);
			//[/RAFIXME]
			//[/CoOp]
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if ( ( NPCInfo->homeWp == WAYPOINT_NONE ) || ( NPC->waypoint == WAYPOINT_NONE ) )
			{
				//Heading for or at an invalid waypoint, get out of this bState
				if( NPCInfo->tempBehavior == BS_SEARCH )
				{//if tempbehavior, set tempbehavior to default
					NPCInfo->tempBehavior = BS_DEFAULT;
				}
				else
				{//if bState, change to stand guard
					NPCInfo->behaviorState = BS_STAND_GUARD;
					NPC_BSRunAndShoot();
				}
				return;
			}

			if ( NPC->waypoint == NPCInfo->homeWp )
			{
				//Just Reached our homeWp, if this is the first time, run your lostenemyscript
				if ( NPCInfo->aiFlags & NPCAI_ENROUTE_TO_HOMEWP )
				{
					NPCInfo->aiFlags &= ~NPCAI_ENROUTE_TO_HOMEWP;
					G_ActivateBehavior( NPC, BSET_LOSTENEMY );
				}

			}

			//Com_Printf("Got there.\n");
			//Com_Printf("Looking...");
			if( !Q_irand(0, 1) )
			{
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL);
			}
			else
			{
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL);
			}
			NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
		}
		else
		{
			NPC_MoveToGoal( qtrue );
		}
	}
	else
	{//RACC - standing at a waypoint investigating.
		//We're there
		if ( NPCInfo->investigateDebounceTime > level.time )
		{//RACC - still investigating.
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE )
			{
				if ( !Q_irand( 0, 30 ) )
				{
					//[CoOp]
					//[RAFIXME] - Impliment SP Nav code.
					/*
					// NAV_TODO: What if there are no neighbors?
					vec3_t	branchPos, lookDir;

					NAV::GetNodePosition(NAV::ChooseRandomNeighbor(NPCInfo->tempGoal->waypoint), branchPos);
					
					VectorSubtract( branchPos, NPCInfo->tempGoal->currentOrigin, lookDir );
					NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + Q_flrand( -45, 45 ) );
					*/
					//[/RAFIXME]
					//[/CoOp]

					int	numEdges = trap_Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE )
					{
						int branchNum = Q_irand( 0, numEdges - 1 );

						vec3_t	branchPos, lookDir;

						int nextWp = trap_Nav_GetNodeEdge( NPCInfo->tempGoal->waypoint, branchNum );
						trap_Nav_GetNodePosition( nextWp, branchPos );

						VectorSubtract( branchPos, NPCInfo->tempGoal->r.currentOrigin, lookDir );
						NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + flrand( -45, 45 ) );
					}

					//pick an angle +-45 degrees off of the dir of a random branch
					//from NPCInfo->tempGoal->waypoint
					//int branch = Q_irand( 0, (waypoints[NPCInfo->tempGoal->waypoint].numNeighbors - 1) );
					//int	nextWp = waypoints[NPCInfo->tempGoal->waypoint].nextWaypoint[branch][NPC->client->moveType];
					//vec3_t	lookDir;

					//VectorSubtract( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin, lookDir );
					//Look in that direction +- 45 degrees
					//NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + Q_flrand( -45, 45 ) );
				}
			}
			//Com_Printf(".");
		}
		else
		{//Just finished waiting
			//[CoOp]
			//[RAFIXME] - impliment SP nav code
			/*
			NPC->waypoint = NAV::GetNearestNode(NPC);

			if ( NPC->waypoint == NPCInfo->homeWp )
			{
				// NAV_TODO: What if there are no neighbors?

				int nextWp = NAV::ChooseRandomNeighbor(NPCInfo->tempGoal->waypoint);
				NAV::GetNodePosition(nextWp, NPCInfo->tempGoal->currentOrigin);
				NPCInfo->tempGoal->waypoint = nextWp;

			}
			else
			{//At a branch, so return home
				NAV::GetNodePosition(NPCInfo->homeWp, NPCInfo->tempGoal->currentOrigin);
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
			}
			*/
			//[/RAFIXME]
			//[/CoOp]

			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );
			
			if ( NPC->waypoint == NPCInfo->homeWp )
			{
				int	numEdges = trap_Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

				if ( numEdges != WAYPOINT_NONE )
				{
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap_Nav_GetNodeEdge( NPCInfo->homeWp, branchNum );
					trap_Nav_GetNodePosition( nextWp, NPCInfo->tempGoal->r.currentOrigin );
					NPCInfo->tempGoal->waypoint = nextWp;
				}

				/*
				//Pick a random branch
				int branch = Q_irand( 0, (waypoints[NPCInfo->homeWp].numNeighbors - 1) );
				int	nextWp = waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType];

				VectorCopy( waypoints[nextWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = nextWp;
				//Com_Printf("\nHeading for wp %d...\n", waypoints[NPCInfo->homeWp].nextWaypoint[branch][NPC->client->moveType]);
				*/
			}
			else
			{//At a branch, so return home
				trap_Nav_GetNodePosition( NPCInfo->homeWp, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
				/*
				VectorCopy( waypoints[NPCInfo->homeWp].origin, NPCInfo->tempGoal->r.currentOrigin );
				NPCInfo->tempGoal->waypoint = NPCInfo->homeWp;
				//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
				*/
			}

			NPCInfo->investigateDebounceTime = 0;
			//Start moving toward our tempGoal
			NPCInfo->goalEntity = NPCInfo->tempGoal;
			NPC_MoveToGoal( qtrue );
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSSearchStart
-------------------------
*/

void NPC_BSSearchStart( int homeWp, bState_t bState )
{
	//[CoOp]
	/* not in SP code
	//FIXME: Reimplement
	if ( homeWp == WAYPOINT_NONE )
	{
		homeWp = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );
		if( NPC->waypoint == WAYPOINT_NONE )
		{
			NPC->waypoint = homeWp;
		}
	}
	*/
	//[/CoOp]

	NPCInfo->homeWp = homeWp;
	NPCInfo->tempBehavior = bState;
	NPCInfo->aiFlags |= NPCAI_ENROUTE_TO_HOMEWP;
	NPCInfo->investigateDebounceTime = 0;
	//[CoOp]
	//[RAFIXME] - impliment SP nav code.
	//NAV::GetNodePosition(homeWp, NPCInfo->tempGoal->currentOrigin);
	//[/CoOp]
	trap_Nav_GetNodePosition( homeWp, NPCInfo->tempGoal->r.currentOrigin );
	NPCInfo->tempGoal->waypoint = homeWp;
	//Com_Printf("\nHeading for wp %d...\n", NPCInfo->homeWp);
}

/*
-------------------------
NPC_BSNoClip

  Use in extreme circumstances only
-------------------------
*/

void NPC_BSNoClip ( void )
{//RACC - I think this is for NPCs set to noclip.  The bot just attempts to move directly
	//to goal.
	if ( UpdateGoal() )
	{
		vec3_t	dir, forward, right, angles, up = {0, 0, 1};
		float	fDot, rDot, uDot;

		VectorSubtract( NPCInfo->goalEntity->r.currentOrigin, NPC->r.currentOrigin, dir );
		
		vectoangles( dir, angles );
		NPCInfo->desiredYaw = angles[YAW];

		AngleVectors( NPC->r.currentAngles, forward, right, NULL );

		VectorNormalize( dir );

		fDot = DotProduct(forward, dir) * 127;
		rDot = DotProduct(right, dir) * 127;
		uDot = DotProduct(up, dir) * 127;

		ucmd.forwardmove = floor(fDot);
		ucmd.rightmove = floor(rDot);
		ucmd.upmove = floor(uDot);
	}
	else
	{
		//Cut velocity?
		VectorClear( NPC->client->ps.velocity );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSWander (void)
{//FIXME: don't actually go all the way to the next waypoint, just move in fits and jerks...?
	//[CoOp] SP Code
	NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER, qfalse);
	//FIXME: do something with these alerts...?
	//FIXME: do the Stormtrooper alert reaction?  (investigation)
	if ( (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES)
		&& NPC->client->enemyTeam != NPCTEAM_NEUTRAL )
	{//look for enemies
		NPC_CheckEnemy(qtrue, qfalse, qtrue);
		if ( NPC->enemy )
		{//found one
			if( NPCInfo->tempBehavior == BS_WANDER )
			{//if tempbehavior, set tempbehavior to default
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			else
			{//if bState, change to run and shoot
				NPCInfo->behaviorState = BS_DEFAULT;//BS_HUNT_AND_KILL;
				//NPC_BSRunAndShoot();
			}
			return;
		}
	}

	
	/* RAFIXME - impliment SP Nav Code?
		STEER::Activate(NPC);


	// Are We Doing A Path?
	//----------------------
	bool	HasPath = NAV::HasPath(NPC);
	if (HasPath)
	{
		HasPath = NAV::UpdatePath(NPC);
		if (HasPath)
		{
			STEER::Path(NPC);	// Follow The Path
			STEER::AvoidCollisions(NPC);

			if ((NPCInfo->aiFlags&NPCAI_BLOCKED) &&	(level.time-NPCInfo->blockedDebounceTime)>1000)
			{
				HasPath = false;// find a new one
			}
		}
	}

	if (!HasPath)
	{
		// If Debounce Time Has Expired, Choose A New Sub State
		//------------------------------------------------------
	 	if (NPCInfo->investigateDebounceTime<level.time || 
			((NPCInfo->aiFlags&NPCAI_BLOCKED) &&	(level.time-NPCInfo->blockedDebounceTime)>1000))
		{
			// Clear Out Flags From The Previous Substate
			//--------------------------------------------
			NPCInfo->aiFlags	&= ~NPCAI_OFF_PATH;
			NPCInfo->aiFlags	&= ~NPCAI_WALKING;


			// Pick Another Spot
			//-------------------
			int		NEXTSUBSTATE = Q_irand(0, 10);

			bool	RandomPathNode = (NEXTSUBSTATE<9);  //(NEXTSUBSTATE<4);
			bool	PathlessWander = false;				//(NEXTSUBSTATE<9)

			

			// Random Path Node
			//------------------
			if (RandomPathNode)
			{
				// Sometimes, Walk
				//-----------------
				if (Q_irand(0, 1)==0)
				{
					NPCInfo->aiFlags	|= NPCAI_WALKING;
				}

				NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
				NAV::FindPath(NPC, NAV::ChooseRandomNeighbor(NAV::GetNearestNode(NPC)));
			}

			// Pathless Wandering
			//--------------------
			else if (PathlessWander)
			{
				// Sometimes, Walk
				//-----------------
				if (Q_irand(0, 1)==0)
				{
					NPCInfo->aiFlags	|= NPCAI_WALKING;
				}

				NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
				NPCInfo->aiFlags |= NPCAI_OFF_PATH;
			}

			// Just Stand Here
			//-----------------
			else
			{
				NPCInfo->investigateDebounceTime = level.time + Q_irand(2000, 10000);
				NPC_SetAnim(NPC, SETANIM_BOTH, ((Q_irand(0, 1)==0)?(BOTH_GUARD_LOOKAROUND1):(BOTH_GUARD_IDLE1)), SETANIM_FLAG_NORMAL);
			}
		}

		// Ok, So We Don't Have A Path, And Debounce Time Is Still Active, So We Are Either Wandering Or Looking Around
		//--------------------------------------------------------------------------------------------------------------
		else
		{
			if (NPCInfo->aiFlags & NPCAI_OFF_PATH)
			{
				STEER::Wander(NPC);
				STEER::AvoidCollisions(NPC);
			}
			else
			{
				STEER::Stop(NPC);
			}
		}
	}
	STEER::DeActivate(NPC, &ucmd);

	NPC_UpdateAngles( qtrue, qtrue );
	return;
	*/
	//[/CoOp]


	if ( !NPCInfo->investigateDebounceTime )
	{//Starting out
		float	minGoalReachedDistSquared = 64;//32*32;
		vec3_t	vec;

		//Keep moving toward our tempGoal
		NPCInfo->goalEntity = NPCInfo->tempGoal;

		VectorSubtract ( NPCInfo->tempGoal->r.currentOrigin, NPC->r.currentOrigin, vec);

		if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE )
		{
			minGoalReachedDistSquared = 64;
		}

		if ( VectorLengthSquared( vec ) < minGoalReachedDistSquared )
		{
			//Close enough, just got there
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );

			if( !Q_irand(0, 1) )
			{
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_NORMAL);
			}
			else
			{
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_NORMAL);
			}
			//Just got here, so Look around for a while
			NPCInfo->investigateDebounceTime = level.time + Q_irand(3000, 10000);
		}
		else
		{
			//Keep moving toward goal
			NPC_MoveToGoal( qtrue );
		}
	}
	else
	{
		//We're there
		if ( NPCInfo->investigateDebounceTime > level.time )
		{
			//Still waiting around for a bit
			//Turn angles every now and then to look around
			if ( NPCInfo->tempGoal->waypoint != WAYPOINT_NONE )
			{
				if ( !Q_irand( 0, 30 ) )
				{
					int	numEdges = trap_Nav_GetNodeNumEdges( NPCInfo->tempGoal->waypoint );

					if ( numEdges != WAYPOINT_NONE )
					{
						int branchNum = Q_irand( 0, numEdges - 1 );

						vec3_t	branchPos, lookDir;

						int	nextWp = trap_Nav_GetNodeEdge( NPCInfo->tempGoal->waypoint, branchNum );
						trap_Nav_GetNodePosition( nextWp, branchPos );

						VectorSubtract( branchPos, NPCInfo->tempGoal->r.currentOrigin, lookDir );
						NPCInfo->desiredYaw = AngleNormalize360( vectoyaw( lookDir ) + flrand( -45, 45 ) );
					}
				}
			}
		}
		else
		{//Just finished waiting
			NPC->waypoint = NAV_FindClosestWaypointForEnt( NPC, WAYPOINT_NONE );
			
			if ( NPC->waypoint != WAYPOINT_NONE )
			{
				int	numEdges = trap_Nav_GetNodeNumEdges( NPC->waypoint );

				if ( numEdges != WAYPOINT_NONE )
				{
					int branchNum = Q_irand( 0, numEdges - 1 );

					int nextWp = trap_Nav_GetNodeEdge( NPC->waypoint, branchNum );
					trap_Nav_GetNodePosition( nextWp, NPCInfo->tempGoal->r.currentOrigin );
					NPCInfo->tempGoal->waypoint = nextWp;
				}

				NPCInfo->investigateDebounceTime = 0;
				//Start moving toward our tempGoal
				NPCInfo->goalEntity = NPCInfo->tempGoal;
				NPC_MoveToGoal( qtrue );
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
void NPC_BSFaceLeader (void)
{
	vec3_t	head, leaderHead, delta, angleToLeader;

	if ( !NPC->client->leader )
	{//uh.... okay.
		return;
	}

	CalcEntitySpot( NPC->client->leader, SPOT_HEAD, leaderHead );
	CalcEntitySpot( NPC, SPOT_HEAD, head );
	VectorSubtract( leaderHead, head, delta );
	vectoangles( delta, angleToLeader );
	VectorNormalize( delta );
	NPC->NPC->desiredYaw = angleToLeader[YAW];
	NPC->NPC->desiredPitch = angleToLeader[PITCH];
	
	NPC_UpdateAngles(qtrue, qtrue);
}
*/
/*
-------------------------
NPC_BSFlee
-------------------------
*/
//[CoOp]
//check to see if this type of NPC can surrender
extern qboolean G_StandardHumanoid( gentity_t *self );
qboolean NPC_CanSurrender( void )
{
	if ( NPC->client )
	{
		switch ( NPC->client->NPC_class )
		{
		case CLASS_ATST:
		case CLASS_CLAW:
		case CLASS_DESANN:
		case CLASS_FISH:
		case CLASS_FLIER2:
		case CLASS_GALAK:
		case CLASS_GLIDER:
		case CLASS_GONK:				// droid
		case CLASS_HOWLER:
		case CLASS_RANCOR:
		//[NPCSandCreature]
		case CLASS_SAND_CREATURE:
		//[/NPCSandCreature]
		case CLASS_WAMPA:
		case CLASS_INTERROGATOR:		// droid 
		case CLASS_JAN:				
		case CLASS_JEDI:				
		case CLASS_KYLE:
		case CLASS_LANDO:			
		case CLASS_LIZARD:
		case CLASS_LUKE:				
		case CLASS_MARK1:			// droid
		case CLASS_MARK2:			// droid
		case CLASS_GALAKMECH:		// droid
		case CLASS_MINEMONSTER:
		case CLASS_MONMOTHA:			
		case CLASS_MORGANKATARN:
		case CLASS_MOUSE:			// droid
		case CLASS_MURJJ:
		case CLASS_PROBE:			// droid
		case CLASS_PROTOCOL:			// droid
		case CLASS_R2D2:				// droid
		case CLASS_R5D2:				// droid
		case CLASS_REBORN:
		case CLASS_REELO:
		case CLASS_REMOTE:
		case CLASS_SEEKER:			// droid
		case CLASS_SENTRY:
		case CLASS_SHADOWTROOPER:
		case CLASS_SWAMP:
		case CLASS_TAVION:
		case CLASS_ALORA:
		//case CLASS_TUSKEN:	//RAFIXME - impliment this
		case CLASS_BOBAFETT:
		case CLASS_ROCKETTROOPER:
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
		case CLASS_HAZARD_TROOPER:
		case CLASS_PLAYER:
		case CLASS_VEHICLE:
			return qfalse;
			break;
		default:
			break;

		}
		if ( !G_StandardHumanoid( NPC ) )
		{
			return qfalse;
		}
		if ( NPC->client->ps.weapon == WP_SABER )
		{
			return qfalse;
		}
	}

	if ( NPCInfo )
	{
		if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
		{
			return qfalse;
		}
		if ( (NPCInfo->aiFlags&NPCAI_SUBBOSS_CHARACTER) )
		{
			return qfalse;
		}
		if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
		{
			return qfalse;
		}
		if ( (NPCInfo->aiFlags&NPCAI_HEAL_ROSH) )
		{
			return qfalse;
		}
	}
	return qtrue;
}
//[/CoOp]


extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void WP_DropWeapon( gentity_t *dropper, vec3_t velocity );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void NPC_Surrender( void )
{//FIXME: say "don't shoot!" if we weren't already surrendering
	if ( NPC->client->ps.weaponTime || PM_InKnockDown( &NPC->client->ps ) )
	{
		return;
	}
	//[CoOp]
	//certain NPC classes should surrender
	if ( !NPC_CanSurrender() )
	//[/CoOp]
	{
		return;
	}
	if ( NPC->s.weapon != WP_NONE && 
		//[CoOp]
		NPC->s.weapon != WP_MELEE &&
		//NPC->s.weapon != WP_STUN_BATON &&
		//[/CoOp]
		NPC->s.weapon != WP_SABER )
	{
		//[CoOp]
		//I think this will work.
		vec3_t tossAngle;

		AngleVectors( NPC->client->ps.viewangles, tossAngle, NULL, NULL );

		TossClientWeapon(NPC, tossAngle, 500);
		//[/CoOp]
		//WP_DropWeapon( NPC, NULL ); //rwwFIXMEFIXME: Do this (gonna need a system for notifying client of removal)
	}
	if ( NPCInfo->surrenderTime < level.time - 5000 )
	{//haven't surrendered for at least 6 seconds, tell them what you're doing
		//FIXME: need real dialogue EV_SURRENDER
		NPCInfo->blockedSpeechDebounceTime = 0;//make sure we say this
		G_AddVoiceEvent( NPC, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 3000 );
	}

	//[CoOp]
	// Already Surrendering?  If So, Just Update Animations
	//------------------------------------------------------
	if (NPCInfo->surrenderTime>level.time)
	{
		if (NPC->client->ps.torsoAnim==BOTH_COWER1_START && NPC->client->ps.torsoTimer<=100)
		{
		 	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_COWER1, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
	 	 	NPCInfo->surrenderTime = level.time + NPC->client->ps.torsoTimer;
		}
 		if (NPC->client->ps.torsoAnim==BOTH_COWER1 && NPC->client->ps.torsoTimer<=100)
		{
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_COWER1_STOP, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
		 	NPCInfo->surrenderTime = level.time + NPC->client->ps.torsoTimer;
		}
	}

	// New To The Surrender, So Start The Animation
	//----------------------------------------------
	else
	{
		if ( NPC->client->NPC_class == CLASS_JAWA && NPC->client->ps.weapon == WP_NONE )
		{//an unarmed Jawa is very scared
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_COWER1, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			//FIXME: stop doing this if decide to take off and run
		}
		else
		{
			// A Big Monster?  OR: Being Tracked By A Homing Rocket?  So Do The Cower Sequence
			//------------------------------------------
			if ( (NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class==CLASS_RANCOR) || !TIMER_Done( NPC, "rocketChasing" ) )
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_COWER1_START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			}

			// Otherwise, Use The Old Surrender "Arms In Air" Animation
			//----------------------------------------------------------
			else
			{
				NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
				NPC->client->ps.torsoTimer = Q_irand(3000, 8000);				// Pretend the anim lasts longer
			}
		}
	 	NPCInfo->surrenderTime = level.time + NPC->client->ps.torsoTimer + 1000;
	}

//	NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
//	NPC->client->ps.torsoTimer = 1000;
	//NPCInfo->surrenderTime = level.time + 1000;//stay surrendered for at least 1 second
	//FIXME: while surrendering, make a big sight/sound alert? Or G_AlertTeam?
	//[/CoOp]
}

qboolean NPC_CheckSurrender( void )
{
	if ( !trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) //racc - not scripted to go somewhere
		&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE //racc - not in the air
		&& !NPC->client->ps.weaponTime && !PM_InKnockDown( &NPC->client->ps )//racc - not firing and not on the ground
		//RAFIXME - Does the NPC->enemy->enemy == NPC prevent players from causing surrenders?
		&& NPC->enemy && NPC->enemy->client && NPC->enemy->enemy == NPC && NPC->enemy->s.weapon != WP_NONE 
		&& NPC->enemy->health > 20 && NPC->enemy->painDebounceTime < level.time - 3000 && NPC->enemy->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time - 1000 )
	{//don't surrender if scripted to run somewhere or if we're in the air or if we're busy or if we don't have an enemy or if the enemy is not mad at me or is hurt or not a threat or busy being attacked
		//FIXME: even if not in a group, don't surrender if there are other enemies in the PVS and within a certain range?
		if ( NPC->s.weapon != WP_ROCKET_LAUNCHER 
			//[CoOp]
			&& NPC->s.weapon != WP_CONCUSSION
			//[/CoOp]
			&& NPC->s.weapon != WP_REPEATER
			&& NPC->s.weapon != WP_FLECHETTE
			&& NPC->s.weapon != WP_SABER )
		{//jedi and heavy weapons guys never surrender
			//FIXME: rework all this logic into some orderly fashion!!!
			if ( NPC->s.weapon != WP_NONE )
			{//they have a weapon so they'd have to drop it to surrender
				//don't give up unless low on health
				//[CoOp] SP Code
				if ( NPC->health > 25 || NPC->health >= NPC->client->pers.maxHealth )
				//if ( NPC->health > 25 /*|| NPC->health >= NPC->max_health*/ )
				//[/CoOp]
				{ //rwwFIXMEFIXME: Keep max health not a ps state?
					return qfalse;
				}
				//if ( g_crosshairEntNum == NPC->s.number && NPC->painDebounceTime > level.time )
				if (NPC_SomeoneLookingAtMe(NPC) && NPC->painDebounceTime > level.time)
				{//if he just shot me, always give up
					//fall through
				}
				else
				{//don't give up unless facing enemy and he's very close
					if ( !InFOV( NPC->enemy, NPC, 60, 30 ) )
					{//I'm not looking at them
						return qfalse;
					}
					else if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 65536/*256*256*/ )
					{//they're not close
						return qfalse;
					}
					else if ( !trap_InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
					{//they're not in the same room
						return qfalse;
					}
				}
			}

			//fixme: this logic keeps making npc's randomly surrender
			//[CoOp]
			if ( !NPCInfo->group || (NPCInfo->group && NPCInfo->group->numGroup <= 1) )
			//if ( NPCInfo->group && NPCInfo->group->numGroup <= 1 )
			//[/CoOp]
			{//I'm alone but I was in a group//FIXME: surrender anyway if just melee or no weap?
				if ( NPC->s.weapon == WP_NONE 
					//NPC has a weapon
					|| (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS) //racc - battling player
					//[CoOp]
					//accounting for partially lit sabers
					|| (NPC->enemy->s.weapon == WP_SABER&&NPC->enemy->client&&NPC->enemy->client->ps.saberHolstered < 2) //racc - a jedi
					//|| (NPC->enemy->s.weapon == WP_SABER&&NPC->enemy->client&&!NPC->enemy->client->ps.saberHolstered)
					|| (NPC->enemy->NPC && NPC->enemy->NPC->group && NPC->enemy->NPC->group->numGroup > 2) )
				{//surrender only if have no weapon or fighting a player or jedi or if we are outnumbered at least 3 to 1
					if ( (NPC->enemy && NPC->enemy->s.number < MAX_CLIENTS) )
					{//player is the guy I'm running from
						//if ( g_crosshairEntNum == NPC->s.number )
						if (NPC_SomeoneLookingAtMe(NPC))
						{//give up if player is aiming at me
							NPC_Surrender();
							NPC_UpdateAngles( qtrue, qtrue );
							return qtrue;
						}
						else if ( NPC->enemy->s.weapon == WP_SABER )
						{//player is using saber
							if ( InFOV( NPC, NPC->enemy, 60, 30 ) )
							{//they're looking at me
								if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 16384 )
								{//they're close
									if ( trap_InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
									{//they're in the same room
										NPC_Surrender();
										NPC_UpdateAngles( qtrue, qtrue );
										return qtrue;
									}
								}
							}
						}
					}
					else if ( NPC->enemy )
					{//???
						//should NPC's surrender to others?
						if ( InFOV( NPC, NPC->enemy, 30, 30 ) )
						{//they're looking at me
							//[CoOp]
							float maxDist = (64+(NPC->r.maxs[0]*1.5)+(NPC->enemy->r.maxs[0]*1.5));
							maxDist *= maxDist;
							if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < maxDist )
							//if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < 4096 )
							//[/CoOp]
							{//they're close
								if ( trap_InPVS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
								{//they're in the same room
									//FIXME: should player-team NPCs not fire on surrendered NPCs?
									NPC_Surrender();
									NPC_UpdateAngles( qtrue, qtrue );
									return qtrue;
								}
							}
						}
					}
				}
			//[CoOp]
			}
			//[/CoOp]
		}
	}
	return qfalse;
}


//[CoOp]
extern void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath );
void NPC_JawaFleeSound( void )
{
	if ( NPC
		&& NPC->client
		&& NPC->client->NPC_class == CLASS_JAWA
		&& !Q_irand( 0, 3 ) 
		&& NPCInfo->blockedSpeechDebounceTime < level.time
		&& !trap_ICARUS_TaskIDPending(NPC, TID_CHAN_VOICE ) )
	{//ooteenee!!!!  
		//Com_Printf( "ooteenee!!!!\n" ); 
		G_SoundOnEnt(NPC, CHAN_VOICE, "sound/chars/jawa/misc/ooh-tee-nee.wav" );
		NPCInfo->blockedSpeechDebounceTime = level.time + 2000;
	}
}
//[/CoOp]

void NPC_BSFlee( void )
{//FIXME: keep checking for danger
	//[CoOp] RAFIXME - Impliment Nav Code.
	/*
	bool		enemyRecentlySeen	= false;
	float		enemyTooCloseDist	= 50.0f;
	bool		reachedEscapePoint	= false; 
	bool		hasEscapePoint		= false;
	bool		moveSuccess			= false;
	bool		inSurrender			= (level.time<NPCInfo->surrenderTime);



	// Check For Enemies And Alert Events
	//------------------------------------
	NPC_CheckEnemy(qtrue, qfalse);
	NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER, qfalse);
	if (NPC->enemy && G_ClearLOS(NPC, NPC->enemy))
	{
		NPCInfo->enemyLastSeenTime = level.time;
	}
	enemyRecentlySeen = (NPC->enemy && (level.time - NPCInfo->enemyLastSeenTime)<3000);
	if (enemyRecentlySeen)
	{
		if (NPC->enemy->client && NPC->enemy->client->NPC_class==CLASS_RANCOR)
		{
			enemyTooCloseDist = 400.0f;
		}
		enemyTooCloseDist += NPC->maxs[0] + NPC->enemy->maxs[0];
	}


	// Look For Weapons To Pick Up
	//-----------------------------
	if (enemyRecentlySeen &&						// Is There An Enemy Near?
		NPC->client->NPC_class!=CLASS_PRISONER &&	// Prisoners can't pickup weapons
		NPCInfo->rank>RANK_CIVILIAN &&				// Neither can civilians
		TIMER_Done(NPC, "panic") &&					// Panic causes him to run for a bit, don't pickup weapons
		TIMER_Done(NPC, "CheckForWeaponToPickup") && 
		G_CanPickUpWeapons( NPC )					//Allowed To Pick Up Dropped Weapons
		)
	{
		gentity_t *foundWeap = NPC_SearchForWeapons();

		// Ok, There Is A Weapon!  Try Going To It!
		//------------------------------------------
		if (foundWeap && NAV::SafePathExists(NPC->currentOrigin, foundWeap->currentOrigin, NPC->enemy->currentOrigin, 150.0f))
		{
			NAV::ClearPath(NPC);				// Remove Any Old Path

			NPCInfo->goalEntity = foundWeap;	// Change Our Target Goal
			NPCInfo->goalRadius	= 30.0f;		// 30 good enough?

			TIMER_Set(NPC, "CheckForWeaponToPickup", Q_irand(10000, 50000));
		}

		// Look Again Soon
		//-----------------
		else
		{
			TIMER_Set(NPC, "CheckForWeaponToPickup", Q_irand(1000, 5000));
		}
	}

	// If Attempting To Get To An Entity That Is Gone, Clear The Pointer
	//-------------------------------------------------------------------
	if ( NPCInfo->goalEntity
		&& !Q3_TaskIDPending(NPC, TID_MOVE_NAV)
		&& NPC->enemy
		&& Distance( NPCInfo->goalEntity->currentOrigin, NPC->enemy->currentOrigin ) < enemyTooCloseDist )
	{
		//our goal is too close to our enemy, dump it...
		NPCInfo->goalEntity = NULL;
	}
	if (NPCInfo->goalEntity && !NPCInfo->goalEntity->inuse)
	{
		NPCInfo->goalEntity = 0;
	}
	hasEscapePoint		= (NPCInfo->goalEntity && NPCInfo->goalRadius!=0.0f);




	STEER::Activate(NPC);
	{
		// Have We Reached The Escape Point?
		//-----------------------------------
		if (hasEscapePoint && STEER::Reached(NPC, NPCInfo->goalEntity, NPCInfo->goalRadius, false))
		{
			if (Q3_TaskIDPending(NPC, TID_MOVE_NAV))
			{
				Q3_TaskIDComplete(NPC, TID_MOVE_NAV);
			}
			reachedEscapePoint = true;
		}


		// If Super Close To The Enemy, Run In The Other Direction
		//---------------------------------------------------------
		if (enemyRecentlySeen && 
			Distance(NPC->enemy->currentOrigin, NPC->currentOrigin)<enemyTooCloseDist)
		{
			STEER::Evade(NPC, NPC->enemy);
			STEER::AvoidCollisions(NPC);
		}

		// If Already At The Escape Point, Or Surrendering, Don't Move
		//-------------------------------------------------------------
		else if (reachedEscapePoint || inSurrender)
		{
			STEER::Stop(NPC);
		}
		else
		{
			// Try To Get To The Escape Point
			//--------------------------------
			if (hasEscapePoint)
			{
				moveSuccess = STEER::GoTo(NPC, NPCInfo->goalEntity, true);
				if (!moveSuccess)
				{
					moveSuccess = NAV::GoTo(NPC, NPCInfo->goalEntity, 0.3f);
				}
			}

			// Cant Get To The Escape Point, So If There Is An Enemy
			//-------------------------------------------------------
			if (!moveSuccess && enemyRecentlySeen)
			{
				// Try To Get To The Farthest Combat Point From Him
				//--------------------------------------------------
				NAV::TNodeHandle Nbr = NAV::ChooseFarthestNeighbor(NPC, NPC->enemy->currentOrigin, 0.25f);
				if (Nbr>0)
				{
					moveSuccess = STEER::GoTo(NPC, NAV::GetNodePosition(Nbr), true);
					if (!moveSuccess)
					{
						moveSuccess = NAV::GoTo(NPC, Nbr, 0.3f);
					}
				}
			}

			// If We Still Can't (Or Don't Need To) Move, Just Stop
			//------------------------------------------------------
			if (!moveSuccess)
			{
				STEER::Stop(NPC);
			}
		}
	}
	STEER::DeActivate(NPC, &ucmd);


	// Is There An Enemy Around?
	//---------------------------
	if (enemyRecentlySeen)
	{
		// Time To Surrender?
		//--------------------
		if ( TIMER_Done( NPC, "panic" ) )
		{
			//done panicking, time to realize we're dogmeat, if we haven't been able to flee for a few seconds
			if ((level.time-NPC->lastMoveTime)>3000
				&& (level.time-NPCInfo->surrenderTime) > 3000 )//and haven't just finished surrendering
			{
				NPC_FaceEnemy();
				NPC_Surrender(); 
 			}
		}
 
		// Time To Choose A New Escape Point?
		//------------------------------------
		if ((!hasEscapePoint || reachedEscapePoint) && TIMER_Done(NPC, "FindNewEscapePointDebounce"))
		{
			TIMER_Set(NPC, "FindNewEscapePointDebounce", 2500);

			int escapePoint = NPC_FindCombatPoint( 
				NPC->currentOrigin, 
				NPC->enemy->currentOrigin, 
				NPC->currentOrigin, 
				CP_COVER|CP_AVOID_ENEMY|CP_HAS_ROUTE, 
				128 );
			if (escapePoint!=-1)
			{
				NPC_JawaFleeSound();
				NPC_SetCombatPoint(escapePoint);
				NPC_SetMoveGoal(NPC, level.combatPoints[escapePoint].origin, 8, qtrue, escapePoint);
			}
		}
	}


	// If Only Temporarly In Flee, Think About Perhaps Returning To Combat
	//---------------------------------------------------------------------
	if (NPCInfo->tempBehavior==BS_FLEE &&
		TIMER_Done(NPC, "flee") && 
		NPC->s.weapon != WP_NONE && 
		NPC->s.weapon != WP_MELEE
		)
	{
 		NPCInfo->tempBehavior = BS_DEFAULT;
	}

	// Always Update Angles
	//----------------------
	NPC_UpdateAngles( qtrue, qtrue );
	if (reachedEscapePoint)
	{
		return qtrue;
	}
	return qfalse;
	*/
	//[/CoOp]

	gentity_t *goal;

	if ( TIMER_Done( NPC, "flee" ) && NPCInfo->tempBehavior == BS_FLEE )
	{
		NPCInfo->tempBehavior = BS_DEFAULT;
		NPCInfo->squadState = SQUAD_IDLE;
		//FIXME: should we set some timer to make him stay in this spot for a bit, 
		//so he doesn't just suddenly turn around and come back at the enemy?
		//OR, just stop running toward goal for last second or so of flee?
	}
	if ( NPC_CheckSurrender() )
	{
		return;
	}
	goal = NPCInfo->goalEntity;
	if ( !goal )
	{
		goal = NPCInfo->lastGoalEntity;
		if ( !goal )
		{//???!!!
			goal = NPCInfo->tempGoal;
		}
	}

	if ( goal )
	{
		qboolean moved;
		qboolean reverseCourse = qtrue;

		//FIXME: if no weapon, find one and run to pick it up?

		//Let's try to find a waypoint that gets me away from this thing
		if ( NPC->waypoint == WAYPOINT_NONE )
		{
			NPC->waypoint = NAV_GetNearestNode( NPC, NPC->lastWaypoint );
		}
		if ( NPC->waypoint != WAYPOINT_NONE )
		{
			int	numEdges = trap_Nav_GetNodeNumEdges( NPC->waypoint );

			if ( numEdges != WAYPOINT_NONE )
			{
				vec3_t	dangerDir;
				int		nextWp;
				int		branchNum;

				VectorSubtract( NPCInfo->investigateGoal, NPC->r.currentOrigin, dangerDir );
				VectorNormalize( dangerDir );

				for ( branchNum = 0; branchNum < numEdges; branchNum++ )
				{
					vec3_t	branchPos, runDir;

					nextWp = trap_Nav_GetNodeEdge( NPC->waypoint, branchNum );
					trap_Nav_GetNodePosition( nextWp, branchPos );

					VectorSubtract( branchPos, NPC->r.currentOrigin, runDir );
					VectorNormalize( runDir );
					if ( DotProduct( runDir, dangerDir ) > flrand( 0, 0.5 ) )
					{//don't run toward danger
						continue;
					}
					//FIXME: don't want to ping-pong back and forth
					NPC_SetMoveGoal( NPC, branchPos, 0, qtrue, -1, NULL );
					reverseCourse = qfalse;
					break;
				}
			}
		}

		moved = NPC_MoveToGoal( qfalse );//qtrue? (do try to move straight to (away from) goal)

		if ( NPC->s.weapon == WP_NONE && (moved == qfalse || reverseCourse) )
		{//No weapon and no escape route... Just cower?  Need anim.
			NPC_Surrender();
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		//If our move failed, then just run straight away from our goal
		//FIXME: We really shouldn't do this.
		if ( moved == qfalse )
		{
			vec3_t	dir;
			float	dist;
			if ( reverseCourse )
			{
				VectorSubtract( NPC->r.currentOrigin, goal->r.currentOrigin, dir );
			}
			else
			{
				VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, dir );
			}
			NPCInfo->distToGoal	= dist = VectorNormalize( dir );
			NPCInfo->desiredYaw = vectoyaw( dir );
			NPCInfo->desiredPitch = 0;
			ucmd.forwardmove = 127;
		}
		else if ( reverseCourse )
		{
			//ucmd.forwardmove *= -1;
			//ucmd.rightmove *= -1;
			//VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
			NPCInfo->desiredYaw *= -1;
		}
		//FIXME: can stop after a safe distance?
		//ucmd.upmove = 0;
		ucmd.buttons &= ~BUTTON_WALKING;
		//FIXME: what do we do once we've gotten to our goal?
	}
	NPC_UpdateAngles( qtrue, qtrue );

	NPC_CheckGetNewWeapon();
}

void NPC_StartFlee( gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax )
{
	int cp = -1;

	if ( trap_ICARUS_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//running somewhere that a script requires us to go, don't interrupt that!
		return;
	}

	//[CoOp]
	if (NPCInfo->scriptFlags & SCF_DONT_FLEE )	// no flee for you
	{
		return;
	}
	//[/CoOp]

	//if have a fleescript, run that instead
	if ( G_ActivateBehavior( NPC, BSET_FLEE ) )
	{
		return;
	}
	//FIXME: play a flee sound?  Appropriate to situation?
	if ( enemy )
	{
		//[CoOp]
		NPC_JawaFleeSound();
		//[/CoOp]
		G_SetEnemy( NPC, enemy );
	}
	
	//FIXME: if don't have a weapon, find nearest one we have a route to and run for it?
	if ( dangerLevel > AEL_DANGER || NPC->s.weapon == WP_NONE || ((!NPCInfo->group || NPCInfo->group->numGroup <= 1) && NPC->health <= 10 ) )
	{//IF either great danger OR I have no weapon OR I'm alone and low on health, THEN try to find a combat point out of PVS
		//[CoOp] RAFIXME - fix the FindCombatPoint code.
		//cp = NPC_FindCombatPoint( NPC->currentOrigin, dangerPoint, NPC->currentOrigin, CP_COVER|CP_AVOID|CP_HAS_ROUTE|CP_NO_PVS, 128 );
		//[/CoOp]
		cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, dangerPoint, CP_COVER|CP_AVOID|CP_HAS_ROUTE|CP_NO_PVS, 128, -1 );
	}
	//FIXME: still happens too often...
	if ( cp == -1 )
	{//okay give up on the no PVS thing
		//[CoOp] RAFIXME - fix the FindCombatPoint code.
		//cp = NPC_FindCombatPoint( NPC->currentOrigin, dangerPoint, NPC->currentOrigin, CP_COVER|CP_AVOID|CP_HAS_ROUTE, 128 );
		//[/CoOp]
		cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, dangerPoint, CP_COVER|CP_AVOID|CP_HAS_ROUTE, 128, -1 );
		if ( cp == -1 )
		{//okay give up on the avoid
			//[CoOp] RAFIXME - fix the FindCombatPoint code.
			//cp = NPC_FindCombatPoint( NPC->currentOrigin, dangerPoint, NPC->currentOrigin, CP_COVER|CP_HAS_ROUTE, 128 );
			//[/CoOp]
			cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, dangerPoint, CP_COVER|CP_HAS_ROUTE, 128, -1 );
			if ( cp == -1 )
			{//okay give up on the cover
				//[CoOp] RAFIXME - fix the FindCombatPoint code.
				//cp = NPC_FindCombatPoint( NPC->currentOrigin, dangerPoint, NPC->currentOrigin, CP_HAS_ROUTE, 128 );
				//[/CoOp]
				cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, dangerPoint, CP_HAS_ROUTE, 128, -1 );
			}
		}
	}

	//see if we got a valid one
	if ( cp != -1 )
	{//found a combat point
		NPC_SetCombatPoint( cp );
		NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
		//[CoOp] Not in SP Code
		/*
		NPCInfo->behaviorState = BS_HUNT_AND_KILL;
		NPCInfo->tempBehavior = BS_DEFAULT;
		*/
		//[/CoOp]
	}
	else
	{
	//[CoOp]
		//couldn't find a place to hide
		//FIXME: re-implement the old BS_FLEE behavior of following any the waypoint edge 
		//			that leads away from the danger point.
		NPC_SetMoveGoal( NPC, NPC->r.currentOrigin, 0/*goalRadius*/, qtrue, cp, NULL );

		/* old code
		//need to just run like hell!
		if ( NPC->s.weapon != WP_NONE )
		{
			return;//let's just not flee?
		}
		else
		{
			//FIXME: other evasion AI?  Duck?  Strafe?  Dodge?
			NPCInfo->tempBehavior = BS_FLEE;
			//Run straight away from here... FIXME: really want to find farthest waypoint/navgoal from this pos... maybe based on alert event radius?
			NPC_SetMoveGoal( NPC, dangerPoint, 0, qtrue, -1, NULL );
			//store the danger point
			VectorCopy( dangerPoint, NPCInfo->investigateGoal );//FIXME: make a new field for this?
		}
		*/
	}

	if ( dangerLevel > AEL_DANGER//geat danger always makes people turn and run
		|| NPC->s.weapon == WP_NONE	//melee/unarmed guys turn and run, others keep facing you and shooting
		|| NPC->s.weapon == WP_MELEE
		/*|| NPC->s.weapon == WP_TUSKEN_STAFF*/ ) //RAFIXME - impliment weapon?
	{
		NPCInfo->tempBehavior = BS_FLEE;//we don't want to do this forever!
		//FIXME: only make it temp if you have a weapon?  Otherwise, permanent?
	//	NPCInfo->behaviorState = BS_FLEE;
	//	NPCInfo->tempBehavior = BS_DEFAULT;
	}
	//[/CoOp]

	//FIXME: localize this Timer?
	TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
	//FIXME: is this always applicable?
	NPCInfo->squadState = SQUAD_RETREAT;
	TIMER_Set( NPC, "flee", Q_irand( fleeTimeMin, fleeTimeMax ) );
	TIMER_Set( NPC, "panic", Q_irand( 1000, 4000 ) );//how long to wait before trying to nav to a dropped weapon
	
	//[CoOp] not in SP code
	//if (NPC->client->NPC_class != CLASS_PROTOCOL)
	//[/CoOp]
	{
		TIMER_Set( NPC, "duck", 0 );
	}
}

void G_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax )
{//racc - g_ version of NPC_StartFlee.
	if ( !self->NPC )
	{//player
		return;
	}
	SaveNPCGlobals();
	SetNPCGlobals( self );

	NPC_StartFlee( enemy, dangerPoint, dangerLevel, fleeTimeMin, fleeTimeMax );

	RestoreNPCGlobals();
}

void NPC_BSEmplaced( void )
{
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean faceEnemy = qfalse;
	qboolean shoot = qfalse;
	vec3_t	impactPos;

	//Don't do anything if we're hurt
	if ( NPC->painDebounceTime > level.time )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}

	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		if ( !Q_irand( 0, 30 ) )
		{
			NPCInfo->desiredYaw = NPC->s.angles[1] + Q_irand( -90, 90 );
		}
		if ( !Q_irand( 0, 30 ) )
		{
			NPCInfo->desiredPitch = Q_irand( -20, 20 );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPC_ClearLOS4( NPC->enemy ) )
	{
		int hit;
		gentity_t *hitEnt;

		enemyLOS = qtrue;

		hit = NPC_ShotEntity( NPC->enemy, impactPos );
		hitEnt = &g_entities[hit];

		if ( hit == NPC->enemy->s.number || ( hitEnt && hitEnt->takedamage ) )
		{//can hit enemy or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
			enemyCS = qtrue;
			NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
			VectorCopy( NPC->enemy->r.currentOrigin, NPCInfo->enemyLastSeenLocation );
		}
	}
/*
	else if ( trap_InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
	}
*/

	if ( enemyLOS )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy = qtrue;
	}
	if ( enemyCS )
	{
		shoot = qtrue;
	}

	if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy( qtrue );
	}
	else
	{//we want to face in the dir we're running
		NPC_UpdateAngles( qtrue, qtrue );
	}

	if ( NPCInfo->scriptFlags & SCF_DONT_FIRE )
	{
		shoot = qfalse;
	}

	if ( NPC->enemy && NPC->enemy->enemy )
	{
		if ( NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER )
		{//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	if ( shoot )
	{//try to shoot if it's time
		if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
		{
			WeaponThink( qtrue );
		}
	}
}
//[/SPPortComplete]


