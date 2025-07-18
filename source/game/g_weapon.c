// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"
#include "be_aas.h"
#include "bg_saga.h"
#include "../ghoul2/g2.h"
#include "q_shared.h"

static	float	s_quadFactor;
static	vec3_t	forward, vright, up;
static	vec3_t	muzzle;
static  vec3_t  muzzle2;//[DualPistols]

// Bryar Pistol
//--------
//[WeaponSys]
//spread for the bryar pistol.
#define BRYAR_SPREAD				0.0
//Used to be 1600.  It's crazy-fast, and looks much better than it did before!
#define BRYAR_PISTOL_VEL			3000
//Pistol damage used to be 10.  Very piddly, all things considered.  We all know what blasters are SUPPOSED to do.
#define BRYAR_PISTOL_DAMAGE			70
//#define BRYAR_PISTOL_VEL			3000
//#define BRYAR_PISTOL_DAMAGE			10
//[/WeaponSys]
//[BryarSecondary]
#define BRYAR_CHARGE_UNIT		700.0f	// bryar charging gives us one more unit
//#define BRYAR_CHARGE_UNIT			200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove
//[/BryarSecondary]
#define BRYAR_ALT_SIZE				1.0f

// E11 Blaster
//---------
//racc - alt-fire spread
#define BLASTER_SPREAD				0.5f//1.2f
//[WeaponSys]
//racc - primary fire spread
#define BLASTER_SPREAD2				0.5f//1.2f
#define BLASTER_VELOCITY			3000  //Used to be 2300.  Again, way too slow.  You can almost outrun them.
//Better gun, better stopping power.  Kills in two hits if you don't have full shields.
#define BLASTER_DAMAGE				45
//#define BLASTER_DAMAGE				20
//[/WeaponSys]

// Tenloss Disruptor
//----------
//[WeaponSys]
#define DISRUPTOR_MAIN_DAMAGE			150 //was 30
#define DISRUPTOR_MAIN_DAMAGE_SIEGE		150 //was 50
//#define DISRUPTOR_MAIN_DAMAGE			30 //40
//#define DISRUPTOR_MAIN_DAMAGE_SIEGE		50
//[/WeaponSys]
#define DISRUPTOR_NPC_MAIN_DAMAGE_CUT	1.0f

//[WeaponSys]
//was 100.  Way I see it, a sniper rifle should be just that.  One shot kills, unless the Jedi does a dodge.  I'll leave that in more capable hands.
#define DISRUPTOR_ALT_DAMAGE			300
//#define DISRUPTOR_ALT_DAMAGE			100 //125
//[/WeaponSys]
#define DISRUPTOR_NPC_ALT_DAMAGE_CUT	1.0f
#define DISRUPTOR_ALT_TRACES			3		// can go through a max of 3 damageable(sp?) entities
#define DISRUPTOR_CHARGE_UNIT			50.0f	// distruptor charging gives us one more unit every 50ms--if you change this, you'll have to do the same in bg_pmove
//[WeaponSys]
#define DISRUPTOR_SHOT_SIZE				2		//disruptor shot size.  Was originally 0
//[/WeaponSys]


// Wookiee Bowcaster
//----------
//[WeaponSys]
#define	BOWCASTER_DAMAGE			120  //was 50 -- Was 80 [Bowcaster]
#define	BOWCASTER_VELOCITY			3000 //was 1300
//#define	BOWCASTER_DAMAGE			70

//[/WeaponSys]
#define BOWCASTER_SPLASH_DAMAGE		0
#define BOWCASTER_SPLASH_RADIUS		0
#define BOWCASTER_SIZE				2

#define BOWCASTER_ALT_SPREAD		0.0
#define BOWCASTER_VEL_RANGE			0.3f
#define BOWCASTER_CHARGE_UNIT		200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// Heavy Repeater
//----------
#define REPEATER_SHOTS				1
#define REPEATER_SPREAD				1.0f
//[WeaponSys]
#define	REPEATER_DAMAGE				10  //was 14
#define	REPEATER_VELOCITY			3000 //was 1600
//#define	REPEATER_DAMAGE				14

//[/WeaponSys]
#define REPEATER_SIZE				1
#define REPEATER_ALT_SIZE				3	// half of bbox size
//[WeaponSys]
#define	REPEATER_ALT_DAMAGE				150 //was 60.  You've seen the explosion.  You think anyone's going to survive a direct hit?
//#define	REPEATER_ALT_DAMAGE				150
//[/WeaponSys]
#define REPEATER_ALT_SPLASH_DAMAGE		100
#define REPEATER_ALT_SPLASH_RADIUS		256
#define REPEATER_ALT_SPLASH_RAD_SIEGE	384
#define	REPEATER_ALT_VELOCITY			1000

// DEMP2
//----------
#define	DEMP2_DAMAGE				50
//[WeaponSys]
#define	DEMP2_VELOCITY				2000

//[/WeaponSys]
#define	DEMP2_SIZE					3		// half of bbox size

#define DEMP2_ALT_DAMAGE			25 //12		// does 12, 36, 84 at each of the 3 charge levels.
#define DEMP2_CHARGE_UNIT			700.0f	// demp2 charging gives us one more unit every 700ms--if you change this, you'll have to do the same in bg_weapons
#define DEMP2_ALT_RANGE				4096
#define DEMP2_ALT_SPLASHRADIUS		256

// Golan Arms Flechette
//---------
#define FLECHETTE_SHOTS				1
#define FLECHETTE_SPREAD			3.0f
#define FLECHETTE_DAMAGE			20//15
#define FLECHETTE_VELOCITY			3000
#define FLECHETTE_SIZE				1
#define FLECHETTE_GRENADES			1
#define FLECHETTE_MINE_RADIUS_CHECK	384
#define FLECHETTE_ALT_DAMAGE		150.0
#define FLECHETTE_ALT_SPLASH_DAM	100
#define FLECHETTE_ALT_SPLASH_RAD	256

// Personal Rocket Launcher
//---------
//[WeaponSys]
#define	ROCKET_VELOCITY				2000  //was 900...  and again, could be outrun, just about. - Was 3500 DD

#define	ROCKET_DAMAGE				300
#define	ROCKET_SPLASH_DAMAGE		150
#define	ROCKET_SPLASH_RADIUS		512
//#define	ROCKET_DAMAGE				100
//#define	ROCKET_SPLASH_DAMAGE		100
//#define	ROCKET_SPLASH_RADIUS		160
//[/WeaponSys]
#define ROCKET_SIZE					3
#define ROCKET_ALT_THINK_TIME		100

// Concussion Rifle
//---------
//primary
//man, this thing is too absurdly powerful. having to
//slash the values way down from sp.
#define	CONC_VELOCITY				2000
#define	CONC_DAMAGE					300 //150
#define	CONC_NPC_DAMAGE_EASY		300
#define	CONC_NPC_DAMAGE_NORMAL		300
#define	CONC_NPC_DAMAGE_HARD		300
#define	CONC_SPLASH_DAMAGE			150 //50
#define	CONC_SPLASH_RADIUS			512 //300
//alt
#define CONC_ALT_DAMAGE				450 //100
#define CONC_ALT_NPC_DAMAGE_EASY	300
#define CONC_ALT_NPC_DAMAGE_MEDIUM	300
#define CONC_ALT_NPC_DAMAGE_HARD	300

// Bryar Old
//--------
//[WeaponSys]
//spread for the bryar pistol.
#define BRYAR_OLD_SPREAD				0.0
//Used to be 1600.  It's crazy-fast, and looks much better than it did before!
#define BRYAR_OLD_VEL			3000
//Pistol damage used to be 10.  Very piddly, all things considered.  We all know what blasters are SUPPOSED to do.
#define BRYAR_OLD_DAMAGE			70
//#define BRYAR_OLD_VEL			3000
//#define BRYAR_OLD_DAMAGE			10
//[/WeaponSys]
//[BryarSecondary]
#define BRYAR_OLD_CHARGE_UNIT		400.0f	// bryar charging gives us one more unit
//#define BRYAR_OLD_CHARGE_UNIT			200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove
//[/BryarSecondary]
#define BRYAR_OLD_ALT_SIZE				1.0f


// Stun Baton
//--------------
#define STUN_BATON_DAMAGE			20
#define STUN_BATON_ALT_DAMAGE		20
#define STUN_BATON_RANGE			8

								 
// Melee
//--------------
#define MELEE_SWING1_DAMAGE			10
#define MELEE_SWING2_DAMAGE			10
#define MELEE_RANGE					8

// ATST Main Gun
//--------------
#define ATST_MAIN_VEL				3000	// 
#define ATST_MAIN_DAMAGE			150		// 
#define ATST_MAIN_SIZE				3		// make it easier to hit things

// ATST Side Gun
//---------------
#define ATST_SIDE_MAIN_DAMAGE				150
#define ATST_SIDE_MAIN_VELOCITY				2000
#define ATST_SIDE_MAIN_NPC_DAMAGE_EASY		150
#define ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL	150
#define ATST_SIDE_MAIN_NPC_DAMAGE_HARD		150
#define ATST_SIDE_MAIN_SIZE					4
#define ATST_SIDE_MAIN_SPLASH_DAMAGE		50	// yeah, pretty small, either zero out or make it worth having?
#define ATST_SIDE_MAIN_SPLASH_RADIUS		16	// yeah, pretty small, either zero out or make it worth having?

#define ATST_SIDE_ALT_VELOCITY				2000
#define ATST_SIDE_ALT_NPC_VELOCITY			2000
#define ATST_SIDE_ALT_DAMAGE				300

#define ATST_SIDE_ROCKET_NPC_DAMAGE_EASY	300
#define ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL	300
#define ATST_SIDE_ROCKET_NPC_DAMAGE_HARD	300

#define	ATST_SIDE_ALT_SPLASH_DAMAGE			150
#define	ATST_SIDE_ALT_SPLASH_RADIUS			512
#define ATST_SIDE_ALT_ROCKET_SIZE			5
#define ATST_SIDE_ALT_ROCKET_SPLASH_SCALE	0.5f	// scales splash for NPC's

extern qboolean G_BoxInBounds( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs );
extern qboolean G_HeavyMelee( gentity_t *attacker );
extern void Jedi_Decloak( gentity_t *self );
static void WP_FireEmplaced( gentity_t *ent, qboolean altFire );

void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );

void touch_NULL( gentity_t *ent, gentity_t *other, trace_t *trace )
{

}

void laserTrapExplode( gentity_t *self );
void RocketDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

//[Asteroids]
extern vmCvar_t		g_vehAutoAimLead;
//[/Asteroids]

//We should really organize weapon data into tables or parse from the ext data so we have accurate info for this,
float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire )
{
	return 500;
}

//-----------------------------------------------------------------------------
void W_TraceSetStart( gentity_t *ent, vec3_t start, vec3_t mins, vec3_t maxs )
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t	tr;
	vec3_t	entMins;
	vec3_t	entMaxs;
	vec3_t	eyePoint;

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

	VectorCopy( ent->s.pos.trBase, eyePoint);
	eyePoint[2] += ent->client->ps.viewheight;
		
	trap_Trace( &tr, eyePoint, mins, maxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP );

	if ( tr.startsolid || tr.allsolid )
	{
		return;
	}

	if ( tr.fraction < 1.0f )
	{
		VectorCopy( tr.endpos, start );
	}
}


/*
----------------------------------------------
	PLAYER WEAPONS
----------------------------------------------
*/

/*
======================================================================

BRYAR PISTOL

======================================================================
*/

static void WP_FireBryarPistolMain(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";


	missile->s.weapon = WP_BRYAR_PISTOL;
	

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarPistolAlt(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE;
	int count;

	gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qtrue );
	gentity_t   *missile2;
	float boxSize = 0;

	//[DualPistols]
	if ((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2 = CreateMissile(muzzle2, forward, BRYAR_PISTOL_VEL, 10000, ent, qtrue);
	}
	//[/DualPistols]

	//if(ent->client->skillLevel[SK_PISTOL] != FORCE_LEVEL_3)
	//{
		//return;
	//}

	missile->classname = "bryar_proj";

	missile->s.weapon = WP_BRYAR_PISTOL;


	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	missile2->classname = "bryar_proj";
	
	missile2->s.weapon = WP_BRYAR_PISTOL;
	
	}
	//[/DualPistols]
	//[/DualPistols]

//	else if(ent->client->skillLevel[SK_OLD] != FORCE_LEVEL_3)
	//{
		//return;
	//}



	count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	//[BryarSecondary]
	else if ( count > BRYAR_MAX_CHARGE )
	{
		count = BRYAR_MAX_CHARGE;
	}

	damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count/BRYAR_MAX_CHARGE*(BRYAR_PISTOL_ALT_DPMAXDAMAGE-BRYAR_PISTOL_ALT_DPDAMAGE);

	//[/BryarSecondary]

	missile->s.generic1 = count; // The missile will then render according to the charge level.

	boxSize = BRYAR_ALT_SIZE*(count*0.5);

	VectorSet( missile->r.maxs, boxSize, boxSize, boxSize );
	VectorSet( missile->r.mins, -boxSize, -boxSize, -boxSize );

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->s.generic1 = count;
		VectorSet( missile2->r.maxs, boxSize, boxSize, boxSize );
		VectorSet( missile2->r.mins, -boxSize, -boxSize, -boxSize );
	}
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarPistol( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarPistolAlt(ent);
	else
		WP_FireBryarPistolMain(ent);
}


/*
======================================================================

BRYAR PISTOL2

======================================================================
*/

static void WP_FireBryarPistol2Main(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*6/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_2;
	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarPistol2Alt(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*6/7;
	int count;

	gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qtrue );
	gentity_t   *missile2;
	float boxSize = 0;

	//[DualPistols]
	if ((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2 = CreateMissile(muzzle2, forward, BRYAR_PISTOL_VEL, 10000, ent, qtrue);
	}
	//[/DualPistols]

	//if(ent->client->skillLevel[SK_PISTOL] != FORCE_LEVEL_3)
	//{
		//return;
	//}

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_2;


	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	missile2->classname = "bryar_proj";
	missile2->s.weapon = WP_BRYAR_PISTOL;
	missile2->s.eFlags |= EF_WP_OPTION_2;
	}
	//[/DualPistols]
	//[/DualPistols]

//	else if(ent->client->skillLevel[SK_OLD] != FORCE_LEVEL_3)
	//{
		//return;
	//}



	count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	//[BryarSecondary]
	else if ( count > BRYAR_MAX_CHARGE )
	{
		count = BRYAR_MAX_CHARGE;
	}

	damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count/BRYAR_MAX_CHARGE*(BRYAR_PISTOL_ALT_DPMAXDAMAGE-BRYAR_PISTOL_ALT_DPDAMAGE);

	//[/BryarSecondary]

	missile->s.generic1 = count; // The missile will then render according to the charge level.

	boxSize = BRYAR_ALT_SIZE*(count*0.5);

	VectorSet( missile->r.maxs, boxSize, boxSize, boxSize );
	VectorSet( missile->r.mins, -boxSize, -boxSize, -boxSize );

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->s.generic1 = count;
		VectorSet( missile2->r.maxs, boxSize, boxSize, boxSize );
		VectorSet( missile2->r.mins, -boxSize, -boxSize, -boxSize );
	}
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarPistol2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarPistol2Alt(ent);
	else
		WP_FireBryarPistol2Main(ent);
}

/*
======================================================================

BRYAR PISTOL3

======================================================================
*/

static void WP_FireBryarPistol3Main(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*5/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";

	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_3;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarPistol3Alt(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*5/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";

	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_3;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarPistol3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarPistol3Alt(ent);
	else
		WP_FireBryarPistol3Main(ent);
}

/*
======================================================================

BRYAR PISTOL4

======================================================================
*/

static void WP_FireBryarPistol4Main(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*4/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	
	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_4;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarPistol4Alt(gentity_t*ent)
{
	int damage = BRYAR_PISTOL_DAMAGE*4/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
		missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, qfalse );
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;
	
	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";

	missile->s.weapon = WP_BRYAR_PISTOL;
	missile->s.eFlags |= EF_WP_OPTION_4;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarPistol4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarPistol4Alt(ent);
	else
		WP_FireBryarPistol4Main(ent);
}

/*
======================================================================

GENERIC

======================================================================
*/

//---------------------------------------------------------
void WP_FireTurretMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire, int damage, int velocity, int mod, gentity_t *ignore )
//---------------------------------------------------------
{
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "generic_proj";
	missile->s.weapon = WP_TURRET;
	if(ent->client->skillLevel[SK_SENTRY] == FORCE_LEVEL_3)
	{
		missile->damage = 3*damage;
	}
	else if(ent->client->skillLevel[SK_SENTRY] == FORCE_LEVEL_2)
	{
		missile->damage = 2*damage;
	}	
	else
	{
		missile->damage = damage;
	}
	
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = mod;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	if (ignore)
	{
		missile->passThroughNum = ignore->s.number+1;
	}

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//Currently only the seeker drone uses this, but it might be useful for other things as well.

//---------------------------------------------------------
void WP_FireGenericBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire, int damage, int velocity, int mod )
//---------------------------------------------------------
{
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "generic_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = mod;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

/*
======================================================================

BLASTER

======================================================================
*/

//---------------------------------------------------------
void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE;
	gentity_t *missile;


	if(ent->client->skillLevel[SK_BLASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "blaster_proj";


	missile->s.weapon = WP_BLASTER;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireTurboLaserMissile( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	int velocity	= ent->mass; //FIXME: externalize
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, qfalse );
	
	//use a custom shot effect
	missile->s.otherEntityNum2 = ent->genericValue14;
	//use a custom impact effect
	missile->s.emplacedOwner = ent->genericValue15;

	missile->classname = "turbo_proj";
	missile->s.weapon = WP_TURRET;

	missile->damage = ent->damage;		//FIXME: externalize
	missile->splashDamage = ent->splashDamage;	//FIXME: externalize
	missile->splashRadius = ent->splashRadius;	//FIXME: externalize
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	//[Asteroids]
	missile->methodOfDeath = MOD_TARGET_LASER;//MOD_TURBLAST; //count as a heavy weap
	missile->splashMethodOfDeath = MOD_TARGET_LASER;//MOD_TURBLAST;// ?SPLASH;
	//[/Asteroids]
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//set veh as cgame side owner for purpose of fx overrides
	missile->s.owner = ent->s.number;

	//don't let them last forever
	missile->think = G_FreeEntity;
	missile->nextthink = level.time + 5000;//at 20000 speed, that should be more than enough
}

//---------------------------------------------------------
void WP_FireEmplacedMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire, gentity_t *ignore )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE;
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "emplaced_gun_proj";
	missile->s.weapon = WP_TURRET;//WP_EMPLACED_GUN;

	missile->activator = ignore;

	missile->damage = damage;
	missile->dflags = (DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS);
	missile->methodOfDeath = MOD_VEHICLE;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	if (ignore)
	{
		missile->passThroughNum = ignore->s.number+1;
	}

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_FireBlaster( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	//[WeapAccuracy]
	WP_FireBlasterMissile( ent, muzzle, forward, altFire );

	/* Don't want slop since our mishap inacurracy handles that now.
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire )
	{
		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BLASTER_SPREAD;
		angs[YAW]	+= crandom() * BLASTER_SPREAD;
	}
	//[WeaponSys] 
	else
	{//adding the primary fire inaccuracy.  It's slight enough that it won't affect at closer ranges, but it keeps folks on their toes, since they can't snipe from across the levels anymore.                                                       
		angs[PITCH] += crandom() * BLASTER_SPREAD2;
		angs[YAW]	+= crandom() * BLASTER_SPREAD2;
	}
	//[/WeaponSys]

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, altFire );
	*/
	//[/WeapAccuracy]
}

/*
======================================================================

BLASTER2

======================================================================
*/

//---------------------------------------------------------
void WP_FireBlaster2Missile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE*13/9;
	gentity_t *missile;


	if(ent->client->skillLevel[SK_BLASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "blaster_proj";

	missile->s.weapon = WP_BLASTER;
	missile->s.eFlags |= EF_WP_OPTION_2;
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}



//---------------------------------------------------------
static void WP_FireBlaster2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	//[WeapAccuracy]
	WP_FireBlaster2Missile( ent, muzzle, forward, altFire );

	/* Don't want slop since our mishap inacurracy handles that now.
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire )
	{
		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BLASTER_SPREAD;
		angs[YAW]	+= crandom() * BLASTER_SPREAD;
	}
	//[WeaponSys] 
	else
	{//adding the primary fire inaccuracy.  It's slight enough that it won't affect at closer ranges, but it keeps folks on their toes, since they can't snipe from across the levels anymore.                                                       
		angs[PITCH] += crandom() * BLASTER_SPREAD2;
		angs[YAW]	+= crandom() * BLASTER_SPREAD2;
	}
	//[/WeaponSys]

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, altFire );
	*/
	//[/WeapAccuracy]
}

/*
======================================================================

BLASTER3

======================================================================
*/

//---------------------------------------------------------
void WP_FireBlaster3Missile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE*7/9;
	gentity_t *missile;


	if(ent->client->skillLevel[SK_BLASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "blaster_proj";

	missile->s.weapon = WP_BLASTER;
	missile->s.eFlags |= EF_WP_OPTION_3;
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}


//---------------------------------------------------------
static void WP_FireBlaster3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	//[WeapAccuracy]
	WP_FireBlaster3Missile( ent, muzzle, forward, altFire );

	/* Don't want slop since our mishap inacurracy handles that now.
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire )
	{
		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BLASTER_SPREAD;
		angs[YAW]	+= crandom() * BLASTER_SPREAD;
	}
	//[WeaponSys] 
	else
	{//adding the primary fire inaccuracy.  It's slight enough that it won't affect at closer ranges, but it keeps folks on their toes, since they can't snipe from across the levels anymore.                                                       
		angs[PITCH] += crandom() * BLASTER_SPREAD2;
		angs[YAW]	+= crandom() * BLASTER_SPREAD2;
	}
	//[/WeaponSys]

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, altFire );
	*/
	//[/WeapAccuracy]
}

/*
======================================================================

BLASTER4

======================================================================
*/

//---------------------------------------------------------
void WP_FireBlaster4Missile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE*11/9;
	gentity_t *missile;


	if(ent->client->skillLevel[SK_BLASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "blaster_proj";

	missile->s.weapon = WP_BLASTER;
	missile->s.eFlags |= EF_WP_OPTION_4;
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}


//---------------------------------------------------------
static void WP_FireBlaster4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	//[WeapAccuracy]
	WP_FireBlaster4Missile( ent, muzzle, forward, altFire );

	/* Don't want slop since our mishap inacurracy handles that now.
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire )
	{
		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BLASTER_SPREAD;
		angs[YAW]	+= crandom() * BLASTER_SPREAD;
	}
	//[WeaponSys] 
	else
	{//adding the primary fire inaccuracy.  It's slight enough that it won't affect at closer ranges, but it keeps folks on their toes, since they can't snipe from across the levels anymore.                                                       
		angs[PITCH] += crandom() * BLASTER_SPREAD2;
		angs[YAW]	+= crandom() * BLASTER_SPREAD2;
	}
	//[/WeaponSys]

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, altFire );
	*/
	//[/WeapAccuracy]
}

int G_GetHitLocation(gentity_t *target, vec3_t ppoint);

/*
======================================================================

DISRUPTOR

======================================================================
*/
//[DodgeSys]
extern qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod );
extern int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
//[/DodgeSys]
//---------------------------------------------------------
static void WP_DisruptorMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_MAIN_DAMAGE;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192;
	int			ignore, traces;
	//[WeaponSys]
	vec3_t		shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t		shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]



	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	memset(&tr, 0, sizeof(tr)); //to shut the compiler up

	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight;//By eyes

	VectorMA( start, shotRange, forward, end );

	ignore = ent->s.number;
	traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT );
			//trap_Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				//BUGFIX12RAFIXME - ugh, can't seem to get the model index on the 
				//trap_G2Traces.  These probably need to be replaced with the more
				//indepth G2traces.  For now, just assume that the player model was hit.
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
				//[/BugFix12]
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}

		//[BoltBlockSys]
		//players can block or dodge disruptor shots.
		if(OJP_SaberCanBlock(traceEnt, ent, qfalse, tr.endpos, -1, -1) )
		{//saber can be used to block the shot.

			//broadcast shot blocked effect
			gentity_t *te = NULL;

			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
			VectorCopy( muzzle, tent->s.origin2 );
			tent->s.eventParm = ent->s.number;

			te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
			VectorCopy(tr.endpos, te->s.origin);
			VectorCopy(tr.plane.normal, te->s.angles);
			if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
			{
				te->s.angles[1] = 1;
			}
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			//reduce DP cost of the block
			//[ExpSys]
			G_DodgeDrain(traceEnt, ent, OJP_SaberBlockCost(traceEnt, ent, tr.endpos));
			//[/ExpSys]

			//force player into a projective block move.
			WP_SaberBlockNonRandom(traceEnt, tr.endpos, qtrue);
			return;
		}
		//[/BoltBlockSys]
		//[DodgeSys]
		else if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR))
		{//player physically dodged the damage.  Act like we didn't hit him.
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		/* basejka block/dodge code...redundent with dodgesys code in place.
		if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, G_GetHitLocation(traceEnt, tr.endpos) ) )
		{//act like we didn't even hit him
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		else if (traceEnt && traceEnt->client && traceEnt->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (WP_SaberCanBlock(traceEnt, tr.endpos, 0, MOD_DISRUPTOR, qtrue, 0))
			{ //broadcast and stop the shot because it was blocked
				gentity_t *te = NULL;

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
				VectorCopy( muzzle, tent->s.origin2 );
				tent->s.eventParm = ent->s.number;

				te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}
				te->s.eventParm = 0;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				return;
			}
		}
		*/
		//[/DodgeSys]
		else if ( (traceEnt->flags&FL_SHIELDED) )
		{//stopped cold
			return;
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	VectorCopy( muzzle, tent->s.origin2 );
	tent->s.eventParm = ent->s.number;

	traceEnt = &g_entities[tr.entityNum];

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
			{
				ent->client->accuracy_hits++;
			} 

			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NORMAL, MOD_DISRUPTOR );
			
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			if (traceEnt->client)
			{
				tent->s.weapon = 1;
			}
		}
		else 
		{
			 // Hmmm, maybe don't make any marks on things that could break
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = 1;
		}
	}
}


qboolean G_CanDisruptify(gentity_t *ent)
{
	if (!ent || !ent->inuse || !ent->client || ent->s.eType != ET_NPC ||
		ent->s.NPC_class != CLASS_VEHICLE || !ent->m_pVehicle)
	{ //not vehicle
		return qtrue;
	}

	if (ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
	{ //animal is only type that can be disintigeiteigerated
		return qtrue;
	}

	//don't do it to any other veh
	return qfalse;
}


//[DodgeSys]
int DetermineDisruptorCharge(gentity_t *ent)
{//returns the current charge level of the disruptor.  
//WARNING: This function doesn't check ent to see if it is using a disruptor or if it is in alt-fire mode.
	int count; 
	int maxCount = DISRUPTOR_MAX_CHARGE;
	if (ent->client)
	{
		count = ( level.time - ent->client->ps.weaponChargeTime ) / DISRUPTOR_CHARGE_UNIT;
	}
	else
	{
		count = ( 100 ) / DISRUPTOR_CHARGE_UNIT;
	}

	count *= 2;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count >= maxCount )
	{
		count = maxCount;
	}

	return count;
}
//[/DodgeSys]

//---------------------------------------------------------
void WP_DisruptorAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = 0, skip;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192.0f;
	int			i;
	//[DodgeSys]
	int			count;
	//int			count, maxCount = 60;
	//[/DodgeSys]
	int			traces = DISRUPTOR_ALT_TRACES;
	qboolean	fullCharge = qfalse;

	//[WeaponSys]
	vec3_t	shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t	shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]

	damage = DISRUPTOR_ALT_DAMAGE;
	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	if (ent->client)
	{
		VectorCopy( ent->client->ps.origin, start );
		start[2] += ent->client->ps.viewheight;//By eyes
	}
	else
	{
		VectorCopy( ent->r.currentOrigin, start );
		start[2] += 24;
	}

	//[DodgeSys]
	//moved into DetermineDisruptorCharge so we can use it for Dodge cost calcs
	count = DetermineDisruptorCharge(ent);

	if(count >= DISRUPTOR_MAX_CHARGE)
	{
		fullCharge = qtrue;
	}

	// more powerful charges go through more things
	if ( count < 10 )
	{
		traces = 1;
	}
	else if ( count < 20 )
	{
		traces = 2;
	}

	//ent->s.generic1=count;
	ent->genericValue6=count;

	damage += count;
	
	skip = ent->s.number;

	for (i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forward, end );

		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR_SNIPER))
		{//player physically dodged the damage.  Act like we didn't hit him.
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		VectorCopy( muzzle, tent->s.origin2 );
		tent->s.shouldtarget = fullCharge;
		tent->s.eventParm = ent->s.number;

		// If the beam hits a skybox, etc. it would look foolish to add impact effects
		if ( render_impact ) 
		{
			if ( traceEnt->takedamage && traceEnt->client )
			{
				tent->s.otherEntityNum = traceEnt->s.number;

				// Create a simple impact type mark
				tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				tent->s.eFlags |= EF_ALT_FIRING;
	
				if ( LogAccuracyHit( traceEnt, ent )) 
				{
					if (ent->client)
					{
						ent->client->accuracy_hits++;
					}
				}
			} 
			else 
			{
				 if ( traceEnt->r.svFlags & SVF_GLASS_BRUSH 
						|| traceEnt->takedamage 
						|| traceEnt->s.eType == ET_MOVER )
				 {
					if ( traceEnt->takedamage )
					{
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 
								DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

						tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
						tent->s.eventParm = DirToByte( tr.plane.normal );
					}
				 }
				 else
				 {
					 // Hmmm, maybe don't make any marks on things that could break
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
					tent->s.eventParm = DirToByte( tr.plane.normal );
				 }
				break; // and don't try any more traces
			}

			if ( (traceEnt->flags&FL_SHIELDED) )
			{//stops us cold
				break;
			}

			if ( traceEnt->takedamage )
			{
				vec3_t preAng;
				int preHealth = traceEnt->health;
				int preLegs = 0;
				int preTorso = 0;


				if (traceEnt->client)
				{
					preLegs = traceEnt->client->ps.legsAnim;
					preTorso = traceEnt->client->ps.torsoAnim;
					VectorCopy(traceEnt->client->ps.viewangles, preAng);
				}

				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

				if (traceEnt->client && preHealth > 0 && traceEnt->health <= 0 && fullCharge &&
					G_CanDisruptify(traceEnt))
				{ //was killed by a fully charged sniper shot, so disintegrate
					VectorCopy(preAng, traceEnt->client->ps.viewangles);

					traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					VectorCopy(tr.endpos, traceEnt->client->ps.lastHitLoc);

					traceEnt->client->ps.legsAnim = preLegs;
					traceEnt->client->ps.torsoAnim = preTorso;

					traceEnt->r.contents = 0;

					VectorClear(traceEnt->client->ps.velocity);
				}

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
				tent->s.eventParm = DirToByte( tr.plane.normal );
				if (traceEnt->client)
				{
					tent->s.weapon = 1;
				}
			}
		}
		else // not rendering impact, must be a skybox or other similar thing?
		{
			break; // don't try anymore traces
		}

		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
	}
}


//---------------------------------------------------------
static void WP_FireDisruptor( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (!ent || !ent->client || ent->client->ps.zoomMode != 1)
	{ //do not ever let it do the alt fire when not zoomed
		altFire = qfalse;
	}

	if (ent && ent->s.eType == ET_NPC && !ent->client)
	{ //special case for animents
		WP_DisruptorAltFire( ent );
		return;
	}

	if ( altFire )
	{
		WP_DisruptorAltFire( ent );
	}
	else
	{
		WP_DisruptorMainFire( ent );
	}
}

/*
======================================================================

DISRUPTOR2

======================================================================
*/
//[DodgeSys]
extern qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod );
extern int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
//[/DodgeSys]
//---------------------------------------------------------
static void WP_Disruptor2MainFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_MAIN_DAMAGE*4/3;
	int velocity	= BLASTER_VELOCITY;
	gentity_t *missile;
	//[WeaponSys]

	//[/WeaponSys]



	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}

	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "bryar_proj";


	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}


//---------------------------------------------------------
void WP_Disruptor2AltFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
//---------------------------------------------------------
{
	int			damage = 0;
	int velocity	= BLASTER_VELOCITY;
	gentity_t *missile;

	damage = DISRUPTOR_ALT_DAMAGE*4/3;
	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );
	missile->classname = "bryar_proj";


	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}


//---------------------------------------------------------
static void WP_FireDisruptor2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (!ent || !ent->client || ent->client->ps.zoomMode != 1)
	{ //do not ever let it do the alt fire when not zoomed
		altFire = qfalse;
	}

	if (ent && ent->s.eType == ET_NPC && !ent->client)
	{ //special case for animents
		WP_Disruptor2AltFire(ent, muzzle, forward, altFire);
		return;
	}

	if ( altFire )
	{
		WP_Disruptor2AltFire(ent, muzzle, forward, altFire);
	}
	else
	{
		WP_Disruptor2MainFire(ent, muzzle, forward, altFire);
	}
}

/*
======================================================================

DISRUPTOR3

======================================================================
*/
//[DodgeSys]
extern qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod );
extern int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
//[/DodgeSys]
//---------------------------------------------------------
static void WP_Disruptor3MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_MAIN_DAMAGE*2/3;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192;
	int			ignore, traces;
	//[WeaponSys]
	vec3_t		shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t		shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]



	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	memset(&tr, 0, sizeof(tr)); //to shut the compiler up

	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight;//By eyes

	VectorMA( start, shotRange, forward, end );

	ignore = ent->s.number;
	traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT );
			//trap_Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				//BUGFIX12RAFIXME - ugh, can't seem to get the model index on the 
				//trap_G2Traces.  These probably need to be replaced with the more
				//indepth G2traces.  For now, just assume that the player model was hit.
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
				//[/BugFix12]
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}

		//[BoltBlockSys]
		//players can block or dodge disruptor shots.
		if(OJP_SaberCanBlock(traceEnt, ent, qfalse, tr.endpos, -1, -1) )
		{//saber can be used to block the shot.

			//broadcast shot blocked effect
			gentity_t *te = NULL;

			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
			tent->s.eFlags |= EF_WP_OPTION_3;
			VectorCopy( muzzle, tent->s.origin2 );
			tent->s.eventParm = ent->s.number;

			te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
			VectorCopy(tr.endpos, te->s.origin);
			VectorCopy(tr.plane.normal, te->s.angles);
			if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
			{
				te->s.angles[1] = 1;
			}
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			//reduce DP cost of the block
			//[ExpSys]
			G_DodgeDrain(traceEnt, ent, OJP_SaberBlockCost(traceEnt, ent, tr.endpos));
			//[/ExpSys]

			//force player into a projective block move.
			WP_SaberBlockNonRandom(traceEnt, tr.endpos, qtrue);
			return;
		}
		//[/BoltBlockSys]
		//[DodgeSys]
		else if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR))
		{//player physically dodged the damage.  Act like we didn't hit him.
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		/* basejka block/dodge code...redundent with dodgesys code in place.
		if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, G_GetHitLocation(traceEnt, tr.endpos) ) )
		{//act like we didn't even hit him
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		else if (traceEnt && traceEnt->client && traceEnt->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (WP_SaberCanBlock(traceEnt, tr.endpos, 0, MOD_DISRUPTOR, qtrue, 0))
			{ //broadcast and stop the shot because it was blocked
				gentity_t *te = NULL;

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
				VectorCopy( muzzle, tent->s.origin2 );
				tent->s.eventParm = ent->s.number;

				te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}
				te->s.eventParm = 0;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				return;
			}
		}
		*/
		//[/DodgeSys]
		else if ( (traceEnt->flags&FL_SHIELDED) )
		{//stopped cold
			return;
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->s.eFlags |= EF_WP_OPTION_3;
	VectorCopy( muzzle, tent->s.origin2 );
	tent->s.eventParm = ent->s.number;

	traceEnt = &g_entities[tr.entityNum];

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
			{
				ent->client->accuracy_hits++;
			} 

			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NORMAL, MOD_DISRUPTOR );
			
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			if (traceEnt->client)
			{
				tent->s.weapon = 1;
			}
		}
		else 
		{
			 // Hmmm, maybe don't make any marks on things that could break
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = 1;
		}
	}
}


//---------------------------------------------------------
void WP_Disruptor3AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = 0, skip;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192.0f;
	int			i;
	//[DodgeSys]
	int			count;
	//int			count, maxCount = 60;
	//[/DodgeSys]
	int			traces = DISRUPTOR_ALT_TRACES;
	qboolean	fullCharge = qfalse;

	//[WeaponSys]
	vec3_t	shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t	shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]

	damage = DISRUPTOR_ALT_DAMAGE*2/3;
	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	if (ent->client)
	{
		VectorCopy( ent->client->ps.origin, start );
		start[2] += ent->client->ps.viewheight;//By eyes
	}
	else
	{
		VectorCopy( ent->r.currentOrigin, start );
		start[2] += 24;
	}

	//[DodgeSys]
	//moved into DetermineDisruptorCharge so we can use it for Dodge cost calcs
	count = DetermineDisruptorCharge(ent);

	if(count >= DISRUPTOR_MAX_CHARGE)
	{
		fullCharge = qtrue;
	}

	// more powerful charges go through more things
	if ( count < 10 )
	{
		traces = 1;
	}
	else if ( count < 20 )
	{
		traces = 2;
	}

	//ent->s.generic1=count;
	ent->genericValue6=count;

	damage += count;
	
	skip = ent->s.number;

	for (i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forward, end );

		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR_SNIPER))
		{//player physically dodged the damage.  Act like we didn't hit him.
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		tent->s.eFlags |= EF_WP_OPTION_3;
		VectorCopy( muzzle, tent->s.origin2 );
		tent->s.shouldtarget = fullCharge;
		tent->s.eventParm = ent->s.number;

		// If the beam hits a skybox, etc. it would look foolish to add impact effects
		if ( render_impact ) 
		{
			if ( traceEnt->takedamage && traceEnt->client )
			{
				tent->s.otherEntityNum = traceEnt->s.number;

				// Create a simple impact type mark
				tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				tent->s.eFlags |= EF_ALT_FIRING;
	
				if ( LogAccuracyHit( traceEnt, ent )) 
				{
					if (ent->client)
					{
						ent->client->accuracy_hits++;
					}
				}
			} 
			else 
			{
				 if ( traceEnt->r.svFlags & SVF_GLASS_BRUSH 
						|| traceEnt->takedamage 
						|| traceEnt->s.eType == ET_MOVER )
				 {
					if ( traceEnt->takedamage )
					{
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 
								DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

						tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
						tent->s.eventParm = DirToByte( tr.plane.normal );
					}
				 }
				 else
				 {
					 // Hmmm, maybe don't make any marks on things that could break
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
					tent->s.eventParm = DirToByte( tr.plane.normal );
				 }
				break; // and don't try any more traces
			}

			if ( (traceEnt->flags&FL_SHIELDED) )
			{//stops us cold
				break;
			}

			if ( traceEnt->takedamage )
			{
				vec3_t preAng;
				int preHealth = traceEnt->health;
				int preLegs = 0;
				int preTorso = 0;


				if (traceEnt->client)
				{
					preLegs = traceEnt->client->ps.legsAnim;
					preTorso = traceEnt->client->ps.torsoAnim;
					VectorCopy(traceEnt->client->ps.viewangles, preAng);
				}

				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

				if (traceEnt->client && preHealth > 0 && traceEnt->health <= 0 && fullCharge &&
					G_CanDisruptify(traceEnt))
				{ //was killed by a fully charged sniper shot, so disintegrate
					VectorCopy(preAng, traceEnt->client->ps.viewangles);

					traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					VectorCopy(tr.endpos, traceEnt->client->ps.lastHitLoc);

					traceEnt->client->ps.legsAnim = preLegs;
					traceEnt->client->ps.torsoAnim = preTorso;

					traceEnt->r.contents = 0;

					VectorClear(traceEnt->client->ps.velocity);
				}

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
				tent->s.eventParm = DirToByte( tr.plane.normal );
				if (traceEnt->client)
				{
					tent->s.weapon = 1;
				}
			}
		}
		else // not rendering impact, must be a skybox or other similar thing?
		{
			break; // don't try anymore traces
		}

		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
	}
}


//---------------------------------------------------------
static void WP_FireDisruptor3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (!ent || !ent->client || ent->client->ps.zoomMode != 1)
	{ //do not ever let it do the alt fire when not zoomed
		altFire = qfalse;
	}

	if (ent && ent->s.eType == ET_NPC && !ent->client)
	{ //special case for animents
		WP_Disruptor3AltFire( ent );
		return;
	}

	if ( altFire )
	{
		WP_Disruptor3AltFire( ent );
	}
	else
	{
		WP_Disruptor3MainFire( ent );
	}
}

/*
======================================================================

DISRUPTOR4

======================================================================
*/
//[DodgeSys]
extern qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod );
extern int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
//[/DodgeSys]
//---------------------------------------------------------
static void WP_Disruptor4MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_MAIN_DAMAGE*7/6;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192;
	int			ignore, traces;
	//[WeaponSys]
	vec3_t		shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t		shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]



	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	memset(&tr, 0, sizeof(tr)); //to shut the compiler up

	VectorCopy( ent->client->ps.origin, start );
	start[2] += ent->client->ps.viewheight;//By eyes

	VectorMA( start, shotRange, forward, end );

	ignore = ent->s.number;
	traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, ignore, MASK_SHOT );
			//trap_Trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				//BUGFIX12RAFIXME - ugh, can't seem to get the model index on the 
				//trap_G2Traces.  These probably need to be replaced with the more
				//indepth G2traces.  For now, just assume that the player model was hit.
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
				//[/BugFix12]
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}

		//[BoltBlockSys]
		//players can block or dodge disruptor shots.
		if(OJP_SaberCanBlock(traceEnt, ent, qfalse, tr.endpos, -1, -1) )
		{//saber can be used to block the shot.

			//broadcast shot blocked effect
			gentity_t *te = NULL;

			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
			tent->s.eFlags |= EF_WP_OPTION_4;
			VectorCopy( muzzle, tent->s.origin2 );
			tent->s.eventParm = ent->s.number;

			te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
			VectorCopy(tr.endpos, te->s.origin);
			VectorCopy(tr.plane.normal, te->s.angles);
			if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
			{
				te->s.angles[1] = 1;
			}
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			//reduce DP cost of the block
			//[ExpSys]
			G_DodgeDrain(traceEnt, ent, OJP_SaberBlockCost(traceEnt, ent, tr.endpos));
			//[/ExpSys]

			//force player into a projective block move.
			WP_SaberBlockNonRandom(traceEnt, tr.endpos, qtrue);
			return;
		}
		//[/BoltBlockSys]
		//[DodgeSys]
		else if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR))
		{//player physically dodged the damage.  Act like we didn't hit him.
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		/* basejka block/dodge code...redundent with dodgesys code in place.
		if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, G_GetHitLocation(traceEnt, tr.endpos) ) )
		{//act like we didn't even hit him
			VectorCopy( tr.endpos, start );
			ignore = tr.entityNum;
			traces++;
			continue;
		}
		else if (traceEnt && traceEnt->client && traceEnt->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_3)
		{
			if (WP_SaberCanBlock(traceEnt, tr.endpos, 0, MOD_DISRUPTOR, qtrue, 0))
			{ //broadcast and stop the shot because it was blocked
				gentity_t *te = NULL;

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
				VectorCopy( muzzle, tent->s.origin2 );
				tent->s.eventParm = ent->s.number;

				te = G_TempEntity( tr.endpos, EV_SABER_BLOCK );
				VectorCopy(tr.endpos, te->s.origin);
				VectorCopy(tr.plane.normal, te->s.angles);
				if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
				{
					te->s.angles[1] = 1;
				}
				te->s.eventParm = 0;
				te->s.weapon = 0;//saberNum
				te->s.legsAnim = 0;//bladeNum

				return;
			}
		}
		*/
		//[/DodgeSys]
		else if ( (traceEnt->flags&FL_SHIELDED) )
		{//stopped cold
			return;
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->s.eFlags |= EF_WP_OPTION_4;
	VectorCopy( muzzle, tent->s.origin2 );
	tent->s.eventParm = ent->s.number;

	traceEnt = &g_entities[tr.entityNum];

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
			{
				ent->client->accuracy_hits++;
			} 

			G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NORMAL, MOD_DISRUPTOR );
			
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			if (traceEnt->client)
			{
				tent->s.weapon = 1;
			}
		}
		else 
		{
			 // Hmmm, maybe don't make any marks on things that could break
			tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = 1;
		}
	}
}


//---------------------------------------------------------
void WP_Disruptor4AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = 0, skip;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192.0f;
	int			i;
	//[DodgeSys]
	int			count;
	//int			count, maxCount = 60;
	//[/DodgeSys]
	int			traces = DISRUPTOR_ALT_TRACES;
	qboolean	fullCharge = qfalse;

	//[WeaponSys]
	vec3_t	shotMaxs = { DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE, DISRUPTOR_SHOT_SIZE };
	vec3_t	shotMins = { -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE, -DISRUPTOR_SHOT_SIZE };
	//[/WeaponSys]

	damage = DISRUPTOR_ALT_DAMAGE*7/6;
	if(ent->client->skillLevel[SK_DISRUPTOR] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	if (ent->client)
	{
		VectorCopy( ent->client->ps.origin, start );
		start[2] += ent->client->ps.viewheight;//By eyes
	}
	else
	{
		VectorCopy( ent->r.currentOrigin, start );
		start[2] += 24;
	}

	//[DodgeSys]
	//moved into DetermineDisruptorCharge so we can use it for Dodge cost calcs
	count = DetermineDisruptorCharge(ent);

	if(count >= DISRUPTOR_MAX_CHARGE)
	{
		fullCharge = qtrue;
	}

	// more powerful charges go through more things
	if ( count < 10 )
	{
		traces = 1;
	}
	else if ( count < 20 )
	{
		traces = 2;
	}

	//ent->s.generic1=count;
	ent->genericValue6=count;

	damage += count;
	
	skip = ent->s.number;

	for (i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forward, end );

		if (d_projectileGhoul2Collision.integer)
		{
			//[WeaponSys]
			trap_G2Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//trap_G2Trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
			//[/WeaponSys]
		}
		else
		{
			//[WeaponSys]
			trap_Trace( &tr, start, shotMins, shotMaxs, end, skip, MASK_SHOT );
			//[/WeaponSys]
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if (traceEnt && traceEnt->client && traceEnt->client->ps.duelInProgress &&
			traceEnt->client->ps.duelIndex != ent->s.number)
		{
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		if(G_DoDodge(traceEnt, ent, tr.endpos, -1, &damage, MOD_DISRUPTOR_SNIPER))
		{//player physically dodged the damage.  Act like we didn't hit him.
			skip = tr.entityNum;
			VectorCopy(tr.endpos, start);
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		tent->s.eFlags |= EF_WP_OPTION_4;
		VectorCopy( muzzle, tent->s.origin2 );
		tent->s.shouldtarget = fullCharge;
		tent->s.eventParm = ent->s.number;

		// If the beam hits a skybox, etc. it would look foolish to add impact effects
		if ( render_impact ) 
		{
			if ( traceEnt->takedamage && traceEnt->client )
			{
				tent->s.otherEntityNum = traceEnt->s.number;

				// Create a simple impact type mark
				tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				tent->s.eventParm = DirToByte(tr.plane.normal);
				tent->s.eFlags |= EF_ALT_FIRING;
	
				if ( LogAccuracyHit( traceEnt, ent )) 
				{
					if (ent->client)
					{
						ent->client->accuracy_hits++;
					}
				}
			} 
			else 
			{
				 if ( traceEnt->r.svFlags & SVF_GLASS_BRUSH 
						|| traceEnt->takedamage 
						|| traceEnt->s.eType == ET_MOVER )
				 {
					if ( traceEnt->takedamage )
					{
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 
								DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

						tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
						tent->s.eventParm = DirToByte( tr.plane.normal );
					}
				 }
				 else
				 {
					 // Hmmm, maybe don't make any marks on things that could break
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
					tent->s.eventParm = DirToByte( tr.plane.normal );
				 }
				break; // and don't try any more traces
			}

			if ( (traceEnt->flags&FL_SHIELDED) )
			{//stops us cold
				break;
			}

			if ( traceEnt->takedamage )
			{
				vec3_t preAng;
				int preHealth = traceEnt->health;
				int preLegs = 0;
				int preTorso = 0;


				if (traceEnt->client)
				{
					preLegs = traceEnt->client->ps.legsAnim;
					preTorso = traceEnt->client->ps.torsoAnim;
					VectorCopy(traceEnt->client->ps.viewangles, preAng);
				}

				G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_DISRUPTOR_SNIPER );

				if (traceEnt->client && preHealth > 0 && traceEnt->health <= 0 && fullCharge &&
					G_CanDisruptify(traceEnt))
				{ //was killed by a fully charged sniper shot, so disintegrate
					VectorCopy(preAng, traceEnt->client->ps.viewangles);

					traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
					VectorCopy(tr.endpos, traceEnt->client->ps.lastHitLoc);

					traceEnt->client->ps.legsAnim = preLegs;
					traceEnt->client->ps.torsoAnim = preTorso;

					traceEnt->r.contents = 0;

					VectorClear(traceEnt->client->ps.velocity);
				}

				tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_HIT );
				tent->s.eventParm = DirToByte( tr.plane.normal );
				if (traceEnt->client)
				{
					tent->s.weapon = 1;
				}
			}
		}
		else // not rendering impact, must be a skybox or other similar thing?
		{
			break; // don't try anymore traces
		}

		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
	}
}


//---------------------------------------------------------
static void WP_FireDisruptor4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (!ent || !ent->client || ent->client->ps.zoomMode != 1)
	{ //do not ever let it do the alt fire when not zoomed
		altFire = qfalse;
	}

	if (ent && ent->s.eType == ET_NPC && !ent->client)
	{ //special case for animents
		WP_Disruptor4AltFire( ent );
		return;
	}

	if ( altFire )
	{
		WP_Disruptor4AltFire( ent );
	}
	else
	{
		WP_Disruptor4MainFire( ent );
	}
}


/*
======================================================================

BOWCASTER

======================================================================
*/
#if 0
static void WP_BowcasterAltFire( gentity_t *ent )
{
	int	damage	= BOWCASTER_DAMAGE;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, BOWCASTER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;

	

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->damageDecreaseTime = level.time + 300;

	//missile->flags |= FL_BOUNCE; taken out because it was causing problems for sabers
	//missile->bounceCount = 3;
}
#endif

//[Bowcaster]
//---------------------------------------------------------
static void WP_BowcasterMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= BOWCASTER_DAMAGE, count=1,dp=0;
	float		vel;
	vec3_t		angs, dir;
	gentity_t	*missile;
	int i=0;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	dp = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;
		
		if ( dp < 1 )
		{
			dp = 1;
		}
		else if ( dp > BRYAR_MAX_CHARGE )
		{
			dp = BRYAR_MAX_CHARGE;
		}

		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forward, angs );

		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
		angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
		
		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( muzzle, dir, vel, 10000, ent, qtrue );

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;


		VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		missile->s.generic1 = dp;
		ent->client->ps.userInt2=dp;
		// we don't want it to bounce
		missile->bounceCount = 0;

		missile->damageDecreaseTime = level.time + 300;
}
//[/Bowcaster]

//---------------------------------------------------------
static void WP_FireBowcaster( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		WP_BowcasterAltFire( ent );
	}
	else
	{
		WP_BowcasterMainFire( ent );
	}
	*/
	WP_BowcasterMainFire( ent );
}

/*
======================================================================

BOWCASTER2

======================================================================
*/
#if 0
static void WP_Bowcaster2AltFire( gentity_t *ent )
{
	int	damage	= BOWCASTER_DAMAGE*6/12;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, BOWCASTER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;	
	missile->s.eFlags |= EF_WP_OPTION_2;
	

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->damageDecreaseTime = level.time + 300;

	//missile->flags |= FL_BOUNCE; taken out because it was causing problems for sabers
	//missile->bounceCount = 3;
}
#endif

//[Bowcaster]
//---------------------------------------------------------
static void WP_Bowcaster2MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= BOWCASTER_DAMAGE*6/12, count=1,dp=0;
	float		vel;
	vec3_t		angs, dir;
	gentity_t	*missile;
	int i=0;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	dp = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;
		
		if ( dp < 1 )
		{
			dp = 1;
		}
		else if ( dp > BRYAR_MAX_CHARGE )
		{
			dp = BRYAR_MAX_CHARGE;
		}

		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forward, angs );

		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
		angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
		
		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( muzzle, dir, vel, 10000, ent, qtrue );

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;	
		missile->s.eFlags |= EF_WP_OPTION_2;

		VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		missile->s.generic1 = dp;
		ent->client->ps.userInt2=dp;
		// we don't want it to bounce
		missile->bounceCount = 0;

		missile->damageDecreaseTime = level.time + 300;
}
//[/Bowcaster]

//---------------------------------------------------------
static void WP_FireBowcaster2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		WP_Bowcaster2AltFire( ent );
	}
	else
	{
		WP_Bowcaster2MainFire( ent );
	}
	*/
	WP_Bowcaster2MainFire( ent );
}

/*
======================================================================

BOWCASTER3

======================================================================
*/
#if 0
static void WP_Bowcaster3AltFire( gentity_t *ent )
{
	int	damage	= BOWCASTER_DAMAGE*10/12;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, BOWCASTER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER ;
	missile->s.eFlags |= EF_WP_OPTION_3;
	

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->damageDecreaseTime = level.time + 300;

	//missile->flags |= FL_BOUNCE; taken out because it was causing problems for sabers
	//missile->bounceCount = 3;
}
#endif

//[Bowcaster]
//---------------------------------------------------------
static void WP_Bowcaster3MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= BOWCASTER_DAMAGE*10/12, count=1,dp=0;
	float		vel;
	vec3_t		angs, dir;
	gentity_t	*missile;
	int i=0;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	dp = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;
		
		if ( dp < 1 )
		{
			dp = 1;
		}
		else if ( dp > BRYAR_MAX_CHARGE )
		{
			dp = BRYAR_MAX_CHARGE;
		}

		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forward, angs );

		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
		angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
		
		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( muzzle, dir, vel, 10000, ent, qtrue );

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;
		missile->s.eFlags |= EF_WP_OPTION_3;

		VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		missile->s.generic1 = dp;
		ent->client->ps.userInt2=dp;
		// we don't want it to bounce
		missile->bounceCount = 0;

		missile->damageDecreaseTime = level.time + 300;
}
//[/Bowcaster]

//---------------------------------------------------------
static void WP_FireBowcaster3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		WP_Bowcaster3AltFire( ent );
	}
	else
	{
		WP_Bowcaster3MainFire( ent );
	}
	*/
	WP_Bowcaster3MainFire( ent );
}

/*
======================================================================

BOWCASTER4

======================================================================
*/
#if 0
static void WP_Bowcaster4AltFire( gentity_t *ent )
{
	int	damage	= BOWCASTER_DAMAGE*8/12;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, BOWCASTER_VELOCITY, 10000, ent, qfalse);

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;
	missile->s.eFlags |= EF_WP_OPTION_4;
	

	VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->damageDecreaseTime = level.time + 300;

	//missile->flags |= FL_BOUNCE; taken out because it was causing problems for sabers
	//missile->bounceCount = 3;
}
#endif

//[Bowcaster]
//---------------------------------------------------------
static void WP_Bowcaster4MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= BOWCASTER_DAMAGE*8/12, count=1,dp=0;
	float		vel;
	vec3_t		angs, dir;
	gentity_t	*missile;
	int i=0;
	if(ent->client->skillLevel[SK_BOWCASTER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	dp = (level.time - ent->client->ps.weaponChargeTime) / BOWCASTER_CHARGE_UNIT;
		
		if ( dp < 1 )
		{
			dp = 1;
		}
		else if ( dp > BRYAR_MAX_CHARGE )
		{
			dp = BRYAR_MAX_CHARGE;
		}

		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forward, angs );

		// add some slop to the alt-fire direction
		angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
		angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
		
		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( muzzle, dir, vel, 10000, ent, qtrue );

		missile->classname = "bowcaster_alt_proj";
		missile->s.weapon = WP_BOWCASTER;
		missile->s.eFlags |= EF_WP_OPTION_4;

		VectorSet( missile->r.maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		missile->s.generic1 = dp;
		ent->client->ps.userInt2=dp;
		// we don't want it to bounce
		missile->bounceCount = 0;

		missile->damageDecreaseTime = level.time + 300;
}
//[/Bowcaster]

//---------------------------------------------------------
static void WP_FireBowcaster4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		WP_Bowcaster4AltFire( ent );
	}
	else
	{
		WP_Bowcaster4MainFire( ent );
	}
	*/
	WP_Bowcaster4MainFire( ent );
}



/*
======================================================================

REPEATER

======================================================================
*/

//---------------------------------------------------------
static void WP_RepeaterMainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = REPEATER_DAMAGE;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";

		missile->s.weapon = WP_REPEATER;


		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_RepeaterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	
	int	damage	= REPEATER_ALT_DAMAGE;

	gentity_t *missile = CreateMissile( muzzle, forward, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER;
	
	VectorSet( missile->r.maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->s.pos.trType = TR_GRAVITY;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
	missile->s.pos.trDelta[2] += 90.0f;
	}
	missile->s.pos.trDelta[2] += 45.0f; //give a slight boost in the upward direction
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT_SPLASH;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = REPEATER_ALT_SPLASH_DAMAGE;
	missile->splashRadius = REPEATER_ALT_SPLASH_RADIUS;
	

	// we don't want it to bounce forever
	missile->bounceCount = 8;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
	gentity_t *missile2 = CreateMissile( muzzle, forward, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );

	missile2->classname = "repeater_alt_proj";
	missile2->s.weapon = WP_REPEATER;
	
	VectorSet( missile2->r.maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile2->r.maxs, -1, missile->r.mins );
	missile2->s.pos.trType = TR_GRAVITY;
	missile2->s.pos.trDelta[1] -= 78.0f;
	missile2->s.pos.trDelta[2] -= 45.0f;
	missile2->s.pos.trDelta[2] += 45.0f; //give a slight boost in the upward direction
	missile2->damage = damage;
	missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile2->methodOfDeath = MOD_REPEATER_ALT;
	missile2->splashMethodOfDeath = MOD_REPEATER_ALT_SPLASH;
	missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile2->splashDamage = REPEATER_ALT_SPLASH_DAMAGE;
	missile2->splashRadius = REPEATER_ALT_SPLASH_RADIUS;


	// we don't want it to bounce forever
	missile2->bounceCount = 8;
	


	}
}

//---------------------------------------------------------
static void WP_FireRepeater( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire && ent->client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_0 )
	{
		WP_RepeaterAltFire( ent );
	}
	else
	{
		// add some slop to the alt-fire direction
		//angs[PITCH] += crandom() * REPEATER_SPREAD;
		//angs[YAW]	+= crandom() * REPEATER_SPREAD;

		AngleVectors( angs, dir, NULL, NULL );

		WP_RepeaterMainFire( ent, dir );
	}
}

/*
======================================================================

REPEATER2

======================================================================
*/

//---------------------------------------------------------
static void WP_Repeater2MainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = REPEATER_DAMAGE*2.0;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";
		missile->s.weapon = WP_REPEATER;	
		missile->s.eFlags |= EF_WP_OPTION_2;


		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_Repeater2AltFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = REPEATER_DAMAGE*2.0;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";
		missile->s.weapon = WP_REPEATER;	
		missile->s.eFlags |= EF_WP_OPTION_2;


		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_FireRepeater2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire && ent->client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_0 )
	{
		AngleVectors( angs, dir, NULL, NULL );
		
		WP_Repeater2AltFire( ent, dir );
	}
	else
	{
		// add some slop to the alt-fire direction
		//angs[PITCH] += crandom() * REPEATER_SPREAD;
		//angs[YAW]	+= crandom() * REPEATER_SPREAD;

		AngleVectors( angs, dir, NULL, NULL );

		WP_Repeater2MainFire( ent, dir );
	}
}

/*
======================================================================

REPEATER3

======================================================================
*/

//---------------------------------------------------------
static void WP_Repeater3MainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = REPEATER_DAMAGE * 3 / 2;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}
		shots = REPEATER_SHOTS;
		
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";
		missile->s.weapon = WP_REPEATER ;
		missile->s.eFlags |= EF_WP_OPTION_3;

		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_Repeater3AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	
	int	damage	= REPEATER_ALT_DAMAGE*4/5;

	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 3.0;
	}
	
	gentity_t *missile = CreateMissile( muzzle, forward, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER ;
	missile->s.eFlags |= EF_WP_OPTION_3;
	VectorSet( missile->r.maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->s.pos.trType = TR_GRAVITY;
	missile->s.pos.trDelta[2] += 45.0f; //give a slight boost in the upward direction
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT_SPLASH;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = REPEATER_ALT_SPLASH_DAMAGE;
	missile->splashRadius = REPEATER_ALT_SPLASH_RADIUS;


	// we don't want it to bounce forever
	missile->bounceCount = 8;

}

//---------------------------------------------------------
static void WP_FireRepeater3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire && ent->client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_0 )
	{
		WP_Repeater3AltFire( ent );
	}
	else
	{
		// add some slop to the alt-fire direction
		//angs[PITCH] += crandom() * REPEATER_SPREAD;
		//angs[YAW]	+= crandom() * REPEATER_SPREAD;

		AngleVectors( angs, dir, NULL, NULL );

		WP_Repeater3MainFire( ent, dir );
	}
}

/*
======================================================================

REPEATER4

======================================================================
*/

//---------------------------------------------------------
static void WP_Repeater4MainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = REPEATER_DAMAGE*3;
		shots = REPEATER_SHOTS;
	if(ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";
		missile->s.weapon = WP_REPEATER;
		missile->s.eFlags |= EF_WP_OPTION_4;

		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_Repeater4AltFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int damage = REPEATER_DAMAGE * 9 / 2;
	int shots;
	if (ent->client->skillLevel[SK_REPEATER] == FORCE_LEVEL_3)
	{
		shots = REPEATER_SHOTS + 1;
	}
	else 
	{
		shots = REPEATER_SHOTS;
	}	
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";
		missile->s.weapon = WP_REPEATER;
		missile->s.eFlags |= EF_WP_OPTION_4;

		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

//---------------------------------------------------------
static void WP_FireRepeater4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forward, angs );

	if ( altFire && ent->client->skillLevel[SK_REPEATER] >= FORCE_LEVEL_0 )
	{
		AngleVectors( angs, dir, NULL, NULL );

		WP_Repeater4AltFire( ent, dir );
	}
	else
	{
		// add some slop to the alt-fire direction
		//angs[PITCH] += crandom() * REPEATER_SPREAD;
		//angs[YAW]	+= crandom() * REPEATER_SPREAD;

		AngleVectors( angs, dir, NULL, NULL );

		WP_Repeater4MainFire( ent, dir );
	}
}


/*
======================================================================

DEMP2

======================================================================
*/

static void WP_DEMP2_MainFire( gentity_t *ent )
{
	int	damage	= DEMP2_DAMAGE;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;



	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//#if 0
static gentity_t *ent_list[MAX_GENTITIES];
//#endif

void DEMP2_AltRadiusDamage( gentity_t *ent )
{
	float		frac = ( level.time - ent->genericValue5 ) / 800.0f; // / 1600.0f; // synchronize with demp2 effect
	float		dist, radius, fact;
	gentity_t	*gent;
	int			iEntityList[MAX_GENTITIES];
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*myOwner = NULL;
	int			numListedEntities, i, e;
	vec3_t		mins, maxs;
	vec3_t		v, dir;

	if (ent->r.ownerNum >= 0 &&
		ent->r.ownerNum < /*MAX_CLIENTS ... let npc's/shooters use it*/MAX_GENTITIES)
	{
		myOwner = &g_entities[ent->r.ownerNum];
	}

	if (!myOwner || !myOwner->inuse || !myOwner->client)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end
	
	radius = frac * 200.0f; // 200 is max radius...the model is aprox. 100 units tall...the fx draw code mults. this by 2.

	fact = ent->count*0.6;

	if (fact < 1)
	{
		fact = 1;
	}

	radius *= fact;

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = ent->r.currentOrigin[i] - radius;
		maxs[i] = ent->r.currentOrigin[i] + radius;
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
		gent = entityList[ e ];

		if ( !gent || !gent->takedamage || !gent->r.contents )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( ent->r.currentOrigin[i] < gent->r.absmin[i] ) 
			{
				v[i] = gent->r.absmin[i] - ent->r.currentOrigin[i];
			} 
			else if ( ent->r.currentOrigin[i] > gent->r.absmax[i] ) 
			{
				v[i] = ent->r.currentOrigin[i] - gent->r.absmax[i];
			} 
			else 
			{
				v[i] = 0;
			}
		}

		// shape is an ellipsoid, so cut vertical distance in half`
		v[2] *= 0.5f;

		dist = VectorLength( v );

		if ( dist >= radius ) 
		{
			// shockwave hasn't hit them yet
			continue;
		}

		if (dist+(16*ent->count) < ent->genericValue6)
		{
			// shockwave has already hit this thing...
			continue;
		}

		VectorCopy( gent->r.currentOrigin, v );
		VectorSubtract( v, ent->r.currentOrigin, dir);

		// push the center of mass higher than the origin so players get knocked into the air more
		dir[2] += 12;

		if (gent != myOwner)
		{
			G_Damage( gent, myOwner, myOwner, dir, ent->r.currentOrigin, ent->damage, DAMAGE_DEATH_KNOCKBACK, ent->splashMethodOfDeath );
			if ( gent->takedamage 
				&& gent->client ) 
			{
				if ( gent->client->ps.electrifyTime < level.time )
				{//electrocution effect
					if (gent->s.eType == ET_NPC && gent->s.NPC_class == CLASS_VEHICLE &&
						gent->m_pVehicle && (gent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER || gent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER))
					{ //do some extra stuff to speeders/walkers
						gent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
					}
					else if ( gent->s.NPC_class != CLASS_VEHICLE 
						|| (gent->m_pVehicle && gent->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER) )
					{//don't do this to fighters
						gent->client->ps.electrifyTime = level.time + Q_irand( 300, 800 );
					}
				}
					if (gent->client->ps.powerups[PW_CLOAKED] )
					{//disable cloak temporarily
						Jedi_Decloak(gent);
						gent->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if (gent->client->ps.powerups[PW_SPHERESHIELDED] )
					{//disable cloak temporarily
						Sphereshield_Off(gent);
						gent->client->sphereshieldToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if (gent->client->ps.powerups[PW_OVERLOADED] )
					{//disable cloak temporarily
						Overload_Off(gent);
						gent->client->overloadToggleTime = level.time + Q_irand( 3000, 10000 );
					}
					if (gent->client->jetPackOn )
					{//disable cloak temporarily
						Jetpack_Off(gent);
						gent->client->jetPackToggleTime = level.time + Q_irand( 3000, 10000 );
					}

				//if ( gent->client->ps.powerups[PW_OVERLOADED] )
				//{//disable cloak temporarily
				//	Overload_Off( gent );
				//	gent->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				//}
			}
		}
	}

	// store the last fraction so that next time around we can test against those things that fall between that last point and where the current shockwave edge is
	ent->genericValue6 = radius;

	if ( frac < 1.0f )
	{
		// shock is still happening so continue letting it expand
		ent->nextthink = level.time + 50;
	}
	else
	{ //don't just leave the entity around
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
	}
}

//---------------------------------------------------------
void DEMP2_AltDetonate( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t *efEnt;

	G_SetOrigin( ent, ent->r.currentOrigin );
	if (!ent->pos1[0] && !ent->pos1[1] && !ent->pos1[2])
	{ //don't play effect with a 0'd out directional vector
		ent->pos1[1] = 1;
	}
	//Let's just save ourself some bandwidth and play both the effect and sphere spawn in 1 event
	efEnt = G_PlayEffect( EFFECT_EXPLOSION_DEMP2ALT, ent->r.currentOrigin, ent->pos1 );

	if (efEnt)
	{
		efEnt->s.weapon = ent->count*2;
	}

	ent->genericValue5 = level.time;
	ent->genericValue6 = 0;
	ent->nextthink = level.time + 50;
	ent->think = DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

//---------------------------------------------------------
static void WP_DEMP2_AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int		damage	= DEMP2_ALT_DAMAGE;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	int		count, origcount;
	float	fact;
	vec3_t	start, end;
	trace_t	tr;
	gentity_t *missile;

	VectorCopy( muzzle, start );

	VectorMA( start, DEMP2_ALT_RANGE, forward, end );

	count = ( level.time - ent->client->ps.weaponChargeTime ) / DEMP2_CHARGE_UNIT;

	origcount = count;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 3 )
	{
		count = 3;
	}

	fact = count*0.8;
	if (fact < 1)
	{
		fact = 1;
	}
	damage *= fact;

	if (!origcount)
	{ //this was just a tap-fire
		damage = 1;
	}

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT);

	missile = G_Spawn();
	G_SetOrigin(missile, tr.endpos);
	//In SP the impact actually travels as a missile based on the trace fraction, but we're
	//just going to be instant. -rww

	VectorCopy( tr.plane.normal, missile->pos1 );

	missile->count = count;

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;


	missile->think = DEMP2_AltDetonate;
	missile->nextthink = level.time;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2;
	missile->splashRadius = DEMP2_ALT_SPLASHRADIUS;

	missile->r.ownerNum = ent->s.number;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_FireDEMP2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if ( altFire )
	{
		WP_DEMP2_AltFire( ent );
	}
	else
	{
		WP_DEMP2_MainFire( ent );
	}
}

/*
======================================================================

DEMP22

======================================================================
*/

static void WP_DEMP22_MainFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
{
	int	damage	= DEMP2_DAMAGE/10;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY/5, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_2;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_INCINERATOR;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}




//---------------------------------------------------------
static void WP_DEMP22_AltFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
//---------------------------------------------------------
{
	int	damage	= DEMP2_DAMAGE/5;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY/5, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_2;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_INCINERATOR;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
	
}

//---------------------------------------------------------
static void WP_FireDEMP22( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if ( altFire )
	{
		WP_DEMP22_AltFire(ent, muzzle, forward, altFire);
	}
	else
	{
		WP_DEMP22_MainFire(ent, muzzle, forward, qfalse);
	}
}

/*
======================================================================

DEMP23

======================================================================
*/

static void WP_DEMP23_MainFire( gentity_t *ent )
{
	int	damage	= DEMP2_DAMAGE*7/10;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_3;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}



//---------------------------------------------------------
static void WP_DEMP23_AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int	damage	= DEMP2_DAMAGE*7/10;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_3;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_FireDEMP23( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if ( altFire )
	{
		WP_DEMP23_AltFire( ent );
	}
	else
	{
		WP_DEMP23_MainFire( ent );
	}
}

/*
======================================================================

DEMP24

======================================================================
*/

static void WP_DEMP24_MainFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
{
	int	damage	= DEMP2_DAMAGE/10;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY/5, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_4;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_FREEZER;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}


//---------------------------------------------------------
static void WP_DEMP24_AltFire(gentity_t* ent, vec3_t start, vec3_t dir, qboolean altFire)
//---------------------------------------------------------
{
	int	damage	= DEMP2_DAMAGE/5;
	if(ent->client->skillLevel[SK_DEMP2] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	gentity_t *missile = CreateMissile( muzzle, forward, DEMP2_VELOCITY/5, 10000, ent, qfalse);

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;
	missile->s.eFlags |= EF_WP_OPTION_4;


	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_FREEZER;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_FireDEMP24( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if ( altFire )
	{
		WP_DEMP24_AltFire(ent, muzzle, forward, altFire);
	}
	else
	{
		WP_DEMP24_MainFire(ent, muzzle, forward, altFire);
	}
}



/*
======================================================================

FLECHETTE

======================================================================
*/

//---------------------------------------------------------
static void WP_FlechetteMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = FLECHETTE_DAMAGE;
	if(ent->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	
		shots = FLECHETTE_SHOTS + 3;
	
		
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, ent, qfalse);

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;
		

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

  
//---------------------------------------------------------
void prox_mine_think( gentity_t *ent )
//---------------------------------------------------------
{
	int			count, i;
	qboolean	blow = qfalse;

	// if it isn't time to auto-explode, do a small proximity check
	if ( ent->delay > level.time )
	{

		count = G_RadiusList( ent->r.currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list );

		for ( i = 0; i < count; i++ )
		{
			if ( ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->activator->s.number )
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if ( blow )
	{
		ent->think = laserTrapExplode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}


//-----------------------------------------------------------------------------
static void WP_TraceSetStart( gentity_t *ent, vec3_t start, vec3_t mins, vec3_t maxs )
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


//[CoOp]
void WP_Explode( gentity_t *self )
{//make this entity explode
	gentity_t	*attacker = self;
	vec3_t		forward={0,0,1};

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

	if ( !self->client )
	{
		AngleVectors( self->s.angles, forward, NULL, NULL );
	}

	/* RAFIXME - need to figure this thingy out.
	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->r.currentOrigin, forward );
	}
	*/
	
	if ( self->s.owner )
	{
		attacker = &g_entities[self->s.owner];
	}
	else if ( self->activator )
	{
		attacker = self->activator;
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{ 
		G_RadiusDamage( self->r.currentOrigin, attacker, self->splashDamage, 
			self->splashRadius, self/*don't ignore attacker*/, self,
			/*RAFIXME - impliment this mod? MOD_EXPLOSIVE_SPLASH*/ MOD_ROCKET_SPLASH );
	}

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_SetOrigin( self, self->r.currentOrigin );

	self->nextthink = level.time + 50;
	self->think = G_FreeEntity;
}
//[/CoOp]


void WP_ExplosiveDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	laserTrapExplode(self);
}

//----------------------------------------------
void WP_flechette_alt_blow( gentity_t *ent )
//----------------------------------------------
{
	ent->s.pos.trDelta[0] = 1;
	ent->s.pos.trDelta[1] = 0;
	ent->s.pos.trDelta[2] = 0;

	laserTrapExplode(ent);
}

//#if 0
//------------------------------------------------------------------------------
static void WP_CreateFlechetteBouncyThing( vec3_t start, vec3_t fwd, gentity_t *self )
//------------------------------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( start, fwd, 700 + random() * 700, 1500 + random() * 2000, self, qtrue );
	int damage = FLECHETTE_ALT_DAMAGE;
	missile->think = WP_flechette_alt_blow;

	missile->activator = self;

	

	missile->s.weapon = WP_FLECHETTE;
	
	
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet( missile->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( missile->r.maxs, 3.0f, 3.0f, 3.0f );
	missile->clipmask = MASK_SHOT;

	missile->touch = touch_NULL;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->flags |= FL_BOUNCE_HALF;
	missile->s.eFlags |= EF_ALT_FIRING;

	missile->bounceCount = 50;

	missile->damage = damage;
	missile->dflags = 0;
	missile->splashDamage = FLECHETTE_ALT_SPLASH_DAM;
	missile->splashRadius = FLECHETTE_ALT_SPLASH_RAD;

	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT_SPLASH;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT_SPLASH;

	VectorCopy( start, missile->pos2 );
}


//---------------------------------------------------------
static void WP_FlechetteAltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t 	dir, fwd, start, angs;
					
	int i;

	vectoangles( forward, angs );
	VectorCopy( muzzle, start );
							   

	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall
															  
																			  
																		   
	int grenades;
	if(self->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
		{
		grenades = FLECHETTE_GRENADES + 3;
		}
	else
		{
		grenades = FLECHETTE_GRENADES+1;
		}
	for ( i = 0; i < grenades; i++ )
	{
		VectorCopy( angs, dir );

		dir[PITCH] -= random() * 4 + 8; // make it fly upwards
		dir[YAW] += crandom() * 2;
		AngleVectors( dir, fwd, NULL, NULL );

		WP_CreateFlechetteBouncyThing( start, fwd, self );

	}
		
	
}
	  

//#endif

//---------------------------------------------------------
static void WP_FireFlechette( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_FlechetteAltFire(ent);
	}
	else
	{
		WP_FlechetteMainFire( ent );
	}
	*/
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_FlechetteAltFire(ent);
	}
	else
	{
		WP_FlechetteMainFire( ent );
	}
}

/*
======================================================================

FLECHETTE2

======================================================================
*/

//---------------------------------------------------------
static void WP_Flechette2MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = FLECHETTE_DAMAGE*2/5;
	
	if(ent->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	
		shots = FLECHETTE_SHOTS + 5;

		
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, ent, qfalse);

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;	
		missile->s.eFlags |= EF_WP_OPTION_2;

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

  





//---------------------------------------------------------
static void WP_Flechette2AltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = FLECHETTE_DAMAGE*3/5;
	
	if(self->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
		shots = FLECHETTE_SHOTS + 5;

		
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, self, qfalse);

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;	
		missile->s.eFlags |= EF_WP_OPTION_2;

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
	  

//#endif

//---------------------------------------------------------
static void WP_FireFlechette2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette2AltFire(ent);
	}
	else
	{
		WP_Flechette2MainFire( ent );
	}
	*/
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette2AltFire(ent);
	}
	else
	{
		WP_Flechette2MainFire( ent );
	}
}
/*
======================================================================

FLECHETTE3

======================================================================
*/

//---------------------------------------------------------
static void WP_Flechette3MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int damage = FLECHETTE_DAMAGE*5;
	if(ent->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	int shots;

		shots = FLECHETTE_SHOTS + 1;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, ent, qfalse);

		missile->classname = "flech_proj";

		missile->s.weapon = WP_FLECHETTE;
		missile->s.eFlags |= EF_WP_OPTION_3;	

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

  



//---------------------------------------------------------
static void WP_Flechette3AltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;

	int damage = FLECHETTE_DAMAGE*15/8;
	if(self->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}
	int shots;
	if (self->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		shots = FLECHETTE_SHOTS + 3;
	}
	else
	{
		shots = FLECHETTE_SHOTS +1;
	}		
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, self, qfalse);

		missile->classname = "flech_proj";

		missile->s.weapon = WP_FLECHETTE;
		missile->s.eFlags |= EF_WP_OPTION_3;

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
	  

//#endif

//---------------------------------------------------------
static void WP_FireFlechette3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette4AltFire(ent);
	}
	else
	{
		WP_Flechette4MainFire( ent );
	}
	*/
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette3AltFire(ent);
	}
	else
	{
		WP_Flechette3MainFire( ent );
	}
}
/*
======================================================================

FLECHETTE4

======================================================================
*/

//---------------------------------------------------------
static void WP_Flechette4MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = FLECHETTE_DAMAGE * 3 / 2;
	if(ent->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}	
		shots = FLECHETTE_SHOTS + 1;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, FLECHETTE_VELOCITY, 10000, ent, qfalse);

		missile->classname = "flech_proj";

		missile->s.weapon = WP_FLECHETTE;
		missile->s.eFlags |= EF_WP_OPTION_4;

		VectorSet( missile->r.maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}

  


//#if 0
//------------------------------------------------------------------------------
static void WP_CreateFlechette4BouncyThing( vec3_t start, vec3_t fwd, gentity_t *self )
//------------------------------------------------------------------------------
{

	gentity_t	*missile = CreateMissile( start, fwd, 700 + random() * 700, 1500 + random() * 2000, self, qtrue );
	int damage = FLECHETTE_ALT_DAMAGE*4/5;
	if(self->client->skillLevel[SK_FLECHETTE] == FORCE_LEVEL_3)
	{
		damage	*= 2.0;
	}		
	missile->think = WP_flechette_alt_blow;

	missile->activator = self;

	


	missile->s.weapon = WP_FLECHETTE;
	missile->s.eFlags |= EF_WP_OPTION_4;
	
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet( missile->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( missile->r.maxs, 3.0f, 3.0f, 3.0f );
	missile->clipmask = MASK_SHOT;

	missile->touch = touch_NULL;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->flags |= FL_BOUNCE_HALF;
	missile->s.eFlags |= EF_ALT_FIRING;

	missile->bounceCount = 50;

	missile->damage = damage;
	missile->dflags = 0;
	missile->splashDamage = FLECHETTE_ALT_SPLASH_DAM;
	missile->splashRadius = FLECHETTE_ALT_SPLASH_RAD;

	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT_SPLASH;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT_SPLASH;

	VectorCopy( start, missile->pos2 );
}


//---------------------------------------------------------
static void WP_Flechette4AltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t 	dir, fwd, start, angs;
					
	int i;

	vectoangles( forward, angs );
	VectorCopy( muzzle, start );
							   

	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall
															  
																			  
																		   
	int grenades;

		grenades = FLECHETTE_GRENADES +1;

	for ( i = 0; i < grenades; i++ )
	{
		VectorCopy( angs, dir );

		dir[PITCH] -= random() * 4 + 8; // make it fly upwards
		dir[YAW] += crandom() * 2;
		AngleVectors( dir, fwd, NULL, NULL );

		WP_CreateFlechette4BouncyThing( start, fwd, self );

	}
		
	
}
	  

//#endif

//---------------------------------------------------------
static void WP_FireFlechette4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	/*
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette3AltFire(ent);
	}
	else
	{
		WP_Flechette3MainFire( ent );
	}
	*/
	if ( altFire )
	{
		//WP_FlechetteProxMine( ent );
		WP_Flechette4AltFire(ent);
	}
	else
	{
		WP_Flechette4MainFire( ent );
	}
}





/*
======================================================================

ROCKET LAUNCHER

======================================================================
*/

//---------------------------------------------------------
void rocketThink( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t newdir, targetdir, 
			up={0,0,1}, right; 
	vec3_t	org;
	float dot, dot2, dis;
	int i;
	float vel = (ent->spawnflags&1)?ent->speed:ROCKET_VELOCITY;

	if ( ent->genericValue1 && ent->genericValue1 < level.time )
	{//time's up, we're done, remove us
		if ( ent->genericValue2 )
		{//explode when die
			RocketDie( ent, &g_entities[ent->r.ownerNum], &g_entities[ent->r.ownerNum], 0, MOD_UNKNOWN );
		}
		else
		{//just remove when die
			G_FreeEntity( ent );
		}
		return;
	}
	if ( !ent->enemy 
		|| !ent->enemy->client 
		|| ent->enemy->health <= 0 
		|| ent->enemy->client->ps.powerups[PW_CLOAKED]		)
	{//no enemy or enemy not a client or enemy dead or enemy cloaked
		if ( !ent->genericValue1  )
		{//doesn't have its own self-kill time
			ent->nextthink = level.time + 10000;
			ent->think = G_FreeEntity;
		}
		return;
	}

	if ( (ent->spawnflags&1) )
	{//vehicle rocket
		if ( ent->enemy->client && ent->enemy->client->NPC_class == CLASS_VEHICLE )
		{//tracking another vehicle
			if ( ent->enemy->client->ps.speed+4000 > vel )
			{
				vel = ent->enemy->client->ps.speed+4000;
			}
		}
	}

	if ( ent->enemy && ent->enemy->inuse )
	{	
		float newDirMult = ent->angle?ent->angle*2.0f:1.0f;
		float oldDirMult = ent->angle?(1.0f-ent->angle)*2.0f:1.0f;

		VectorCopy( ent->enemy->r.currentOrigin, org );
		org[2] += (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5f;

		VectorSubtract( org, ent->r.currentOrigin, targetdir );
		VectorNormalize( targetdir );

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct( targetdir, ent->movedir );
		if ( (ent->spawnflags&1) )
		{//vehicle rocket
			if ( ent->radius > -1.0f )
			{//can lose the lock if DotProduct drops below this number
				if ( dot < ent->radius )
				{//lost the lock!!!
					//HMM... maybe can re-lock on if they come in front again?
					/*
					//OR: should it stop trying to lock altogether?
					if ( ent->genericValue1 )
					{//have a timelimit, set next think to that
						ent->nextthink = ent->genericValue1;
						if ( ent->genericValue2 )
						{//explode when die
							ent->think = G_ExplodeMissile;
						}
						else
						{
							ent->think = G_FreeEntity;
						}
					}
					else
					{
						ent->think = NULL;
						ent->nextthink = -1;
					}
					*/
					return;
				}
			}
		}


		// a dot of 1.0 means right-on-target.
		if ( dot < 0.0f )
		{	
			// Go in the direction opposite, start a 180.
			CrossProduct( ent->movedir, up, right );
			dot2 = DotProduct( targetdir, right );

			if ( dot2 > 0 )
			{	
				// Turn 45 degrees right.
				VectorMA( ent->movedir, 0.4f*newDirMult, right, newdir );
			}
			else
			{	
				// Turn 45 degrees left.
				VectorMA( ent->movedir, -0.4f*newDirMult, right, newdir );
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = ( (targetdir[2]*newDirMult) + (ent->movedir[2]*oldDirMult) ) * 0.5;

			// let's also slow down a lot
			vel *= 0.5f;
		}
		else if ( dot < 0.70f )
		{	
			// Still a bit off, so we turn a bit softer
			VectorMA( ent->movedir, 0.5f*newDirMult, targetdir, newdir );
		}
		else
		{	
			// getting close, so turn a bit harder
			VectorMA( ent->movedir, 0.9f*newDirMult, targetdir, newdir );
		}

		// add crazy drunkenness
		for (i = 0; i < 3; i++ )
		{
			newdir[i] += crandom() * ent->random * 0.25f;
		}

		// decay the randomness
		ent->random *= 0.9f;

		if ( ent->enemy->client
			&& ent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//tracking a client who's on the ground, aim at the floor...?
			// Try to crash into the ground if we get close enough to do splash damage
			dis = Distance( ent->r.currentOrigin, org );

			if ( dis < 128 )
			{
				// the closer we get, the more we push the rocket down, heh heh.
				newdir[2] -= (1.0f - (dis / 128.0f)) * 0.6f;
			}
		}

		VectorNormalize( newdir );

		VectorScale( newdir, vel * 0.5f, ent->s.pos.trDelta );
		VectorCopy( newdir, ent->movedir );
		SnapVector( ent->s.pos.trDelta );			// save net bandwidth
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
	}

	ent->nextthink = level.time + ROCKET_ALT_THINK_TIME;	// Nothing at all spectacular happened, continue.
	return;
}

extern void G_ExplodeMissile( gentity_t *ent );
void RocketDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	self->die = 0;
	self->r.contents = 0;

	G_ExplodeMissile( self );

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

//---------------------------------------------------------
static void WP_FireRocket( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	int	damage	= ROCKET_DAMAGE;
	int	vel = ROCKET_VELOCITY;
	int dif = 0;
	float rTime;
	gentity_t *missile;
if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3 && altFire)
{
	damage *= 3.0;
}
else if(altFire)
{
	damage *= 3 / 2;
}
else if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3)
{
	damage *= 2.0;
}
//	if ( altFire )
//	{
//		vel *= 0.5f;
//	}

	missile = CreateMissile( muzzle, forward, vel, 10000, ent, altFire );
	if (ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE)
	{
		float lockTimeInterval = ((g_gametype.integer==GT_SIEGE)?2400.0f:1200.0f)/16.0f;
		rTime = ent->client->ps.rocketLockTime;

		if (rTime == -1)
		{
			rTime = ent->client->ps.rocketLastValidTime;
		}
		dif = ( level.time - rTime ) / lockTimeInterval;

		if (dif < 0)
		{
			dif = 0;
		}

		//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
		if ( dif >= 10 && rTime != -1 )
		{
			missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

			if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
			{ //if enemy became invalid, died, or is on the same team, then don't seek it
				missile->angle = 0.5f;
				missile->think = rocketThink;
				missile->nextthink = level.time + ROCKET_ALT_THINK_TIME;
			}
		}

		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;


	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if (altFire)
	{
		missile->methodOfDeath = MOD_ROCKET_HOMING;
		missile->splashMethodOfDeath = MOD_ROCKET_HOMING_SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	}
//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
	
	

}

/*
======================================================================

ROCKET LAUNCHER2

======================================================================
*/
void grenadelauncherThinkStandard(gentity_t *ent);
//---------------------------------------------------------
void GrenadeLauncherExplode( gentity_t *ent )
//---------------------------------------------------------
{
	if ( !ent->count )
	{
		G_Sound( ent, CHAN_WEAPON, G_SoundIndex( "sound/weapons/thermal/warning.wav" ) );
		ent->count = 1;
		ent->genericValue5 = level.time + 500;
		ent->think = grenadelauncherThinkStandard;
		ent->nextthink = level.time;
		ent->r.svFlags |= SVF_BROADCAST;//so everyone hears/sees the explosion?
	}
	else
	{
		vec3_t	origin;
		vec3_t	dir={0,0,1};

		BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
		origin[2] += 8;
		SnapVector( origin );
		G_SetOrigin( ent, origin );

		ent->s.eType = ET_GENERAL;
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
		ent->freeAfterEvent = qtrue;

		if (G_RadiusDamage( ent->r.currentOrigin, ent->parent,  ent->splashDamage, ent->splashRadius, 
				ent, ent, ent->splashMethodOfDeath))
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}

		trap_LinkEntity( ent );
	}
}

void grenadelauncherThinkStandard(gentity_t *ent)
{
	if (ent->genericValue5 < level.time)
	{
		ent->think = GrenadeLauncherExplode;
		ent->nextthink = level.time;
		return;
	}

	G_RunObject(ent);
	ent->nextthink = level.time;
}

#define GLA_TIME				1500//6000
//---------------------------------------------------------
static void WP_FireRocket2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3 && altFire)
	{
	int damage = ROCKET_DAMAGE/2;
	
	gentity_t	*bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	
	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = grenadelauncherThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt->r.mins, bolt->r.maxs );//make sure our start point isn't on the other side of a wall

	if ( ent->client )
	{
		chargeAmount = 1.0f;
	}




	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + GLA_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt->s.pos.trDelta[1] -=30 ;
		bolt->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = damage;
	bolt->dflags = 0;
	bolt->splashDamage = ROCKET_SPLASH_DAMAGE;
	bolt->splashRadius = ROCKET_SPLASH_RADIUS;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->methodOfDeath = MOD_THERMAL;
	bolt->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	VectorCopy( start, bolt->pos2 );

	bolt->bounceCount = -5;

	//return bolt;
	
	
	

	
	gentity_t	*bolt2;
	

	bolt2 = G_Spawn();
	
	bolt2->physicsObject = qtrue;

	bolt2->classname = "thermal_detonator";
	bolt2->think = grenadelauncherThinkStandard;
	bolt2->nextthink = level.time;
	bolt2->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt2->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt2->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt2->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt2->r.mins, bolt2->r.maxs );//make sure our start point isn't on the other side of a wall


	// normal ones bounce, alt ones explode on impact
	bolt2->genericValue5 = level.time + GLA_TIME; // How long 'til she blows
	bolt2->s.pos.trType = TR_GRAVITY;
	bolt2->parent = ent;
	bolt2->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt2->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt2->s.pos.trDelta[1] +=30 ;
		bolt2->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt2->flags |= FL_BOUNCE_HALF;
	}

	bolt2->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt2->s.loopIsSoundset = qfalse;

	bolt2->damage = damage;
	bolt2->dflags = 0;
	bolt2->splashDamage = ROCKET_SPLASH_DAMAGE;
	bolt2->splashRadius = ROCKET_SPLASH_RADIUS;

	bolt2->s.eType = ET_MISSILE;
	bolt2->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt2->s.weapon = WP_THERMAL;
	bolt2->methodOfDeath = MOD_THERMAL;
	bolt2->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt2->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt2->s.pos.trBase );
	
	SnapVector( bolt2->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt2->r.currentOrigin);

	VectorCopy( start, bolt2->pos2 );

	bolt2->bounceCount = -5;
	

	//return bolt, bolt2;
	}
	else
	{
	gentity_t* bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	int damage = ROCKET_DAMAGE/3;
	if(altFire)
	{
	damage *= 3/2;	
	}
	if(ent->client->skillLevel[SK_ROCKET] >= FORCE_LEVEL_3)
	{
	damage *= 2;
	}
	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = grenadelauncherThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = 1.0f;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + GLA_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!altFire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = damage;
	bolt->dflags = 0;
	bolt->splashDamage = ROCKET_SPLASH_DAMAGE;
	bolt->splashRadius = ROCKET_SPLASH_RADIUS;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->methodOfDeath = MOD_THERMAL;
	bolt->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	//return bolt; 
	}
}



/*
======================================================================

ROCKET LAUNCHER3

======================================================================
*/

//---------------------------------------------------------
static void WP_FireRocket3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	int	damage	= ROCKET_DAMAGE*4/3;
	int	vel = ROCKET_VELOCITY;
	int dif = 0;
	float rTime;
	gentity_t *missile;

if(altFire)
{
	damage *= 3 / 2;
}
else if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3 && !altFire )
{
	damage *= 2.0;
}
//	if ( altFire )
//	{
//		vel *= 0.5f;
//	}

	missile = CreateMissile( muzzle, forward, vel, 10000, ent, altFire );


	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;	
	missile->s.eFlags |= EF_WP_OPTION_3;

	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if (altFire)
	{
		missile->methodOfDeath = MOD_ROCKET_HOMING;
		missile->splashMethodOfDeath = MOD_ROCKET_HOMING_SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	}
//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
	
	
	if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3 && altFire)
		{
	gentity_t *missile2;

//	if ( altFire )
//	{
//		vel *= 0.5f;
//	}

	missile2 = CreateMissile( muzzle2, forward, vel, 10000, ent, altFire );
	missile2->classname = "rocket_proj";
	missile2->s.weapon = WP_ROCKET_LAUNCHER;	
	missile2->s.eFlags |= EF_WP_OPTION_3;

	// Make it easier to hit things
	VectorSet( missile2->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile2->r.maxs, -1, missile2->r.mins );
	missile2->damage = damage;
	missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
	if (altFire)
	{
		missile2->methodOfDeath = MOD_ROCKET_HOMING;
		missile2->splashMethodOfDeath = MOD_ROCKET_HOMING_SPLASH;
	}
	else
	{
		missile2->methodOfDeath = MOD_ROCKET;
		missile2->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	}
//===testing being able to shoot rockets out of the air==================================
	missile2->health = 10;
	missile2->takedamage = qtrue;
	missile2->r.contents = MASK_SHOT;
	missile2->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
	missile2->clipmask = MASK_SHOT;
	missile2->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile2->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile2->bounceCount = 0;
		}
}




/*
======================================================================

ROCKET LAUNCHER4

======================================================================
*/


//---------------------------------------------------------
static void WP_FireRocket4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	int	damage	= ROCKET_DAMAGE*2/3;
	int	vel = ROCKET_VELOCITY;
	int dif = 0;
	float rTime;
	gentity_t *missile;
if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3 && altFire)
{
	damage *= 3.0;
}
else if(altFire)
{
	damage *= 3 / 2;
}
else if(ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3)
{
	damage *= 2.0;
}
//	if ( altFire )
//	{
//		vel *= 0.5f;
//	}

	missile = CreateMissile( muzzle, forward, vel, 10000, ent, altFire );
	if (ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE)
	{
		float lockTimeInterval = ((g_gametype.integer==GT_SIEGE)?2400.0f:1200.0f)/16.0f;
		rTime = ent->client->ps.rocketLockTime;

		if (rTime == -1)
		{
			rTime = ent->client->ps.rocketLastValidTime;
		}
		dif = ( level.time - rTime ) / lockTimeInterval;

		if (dif < 0)
		{
			dif = 0;
		}

		//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
		if ( dif >= 10 && rTime != -1 )
		{
			missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

			if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
			{ //if enemy became invalid, died, or is on the same team, then don't seek it
				missile->angle = 0.5f;
				missile->think = rocketThink;
				missile->nextthink = level.time + ROCKET_ALT_THINK_TIME;
			}
		}

		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;
	missile->s.eFlags |= EF_WP_OPTION_4;

	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if (altFire)
	{
		missile->methodOfDeath = MOD_ROCKET_HOMING;
		missile->splashMethodOfDeath = MOD_ROCKET_HOMING_SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	}
//===testing being able to shoot rockets out of the air==================================
	missile->health = 10;
	missile->takedamage = qtrue;
	missile->r.contents = MASK_SHOT;
	missile->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
	
	

}

/*
======================================================================

THERMAL DETONATOR

======================================================================
*/
//[WeaponSys]
#define TD_DAMAGE			300 //only on a direct impact
#define TD_SPLASH_DAM		150
#define TD_SPLASH_RAD		512
/*
#define TD_DAMAGE			300 //only do 70 on a direct impact
#define TD_SPLASH_RAD		512
#define TD_SPLASH_DAM		150
*/
//[/WeaponSys]
//[SnapThrow]
//moved
//#define TD_VELOCITY			900
//[/SnapThrow]
#define TD_MIN_CHARGE		0.15f
#define TD_TIME				1500//6000
#define TD_ALT_TIME			3000

#define TD_ALT_DAMAGE		300//100
#define TD_ALT_SPLASH_RAD	512
#define TD_ALT_SPLASH_DAM	150//90
#define TD_ALT_VELOCITY		600
#define TD_ALT_MIN_CHARGE	0.15f
#define TD_ALT_TIME			3000

void thermalThinkStandard(gentity_t *ent);

//---------------------------------------------------------
//---------------------------------------------------------
void thermalDetonatorExplode( gentity_t *ent )
//---------------------------------------------------------
{
	if ( !ent->count )
	{
		G_Sound( ent, CHAN_WEAPON, G_SoundIndex( "sound/weapons/thermal/warning.wav" ) );
		ent->count = 1;
		ent->genericValue5 = level.time + 500;
		ent->think = thermalThinkStandard;
		ent->nextthink = level.time;
		ent->r.svFlags |= SVF_BROADCAST;//so everyone hears/sees the explosion?
	}
	else
	{
		vec3_t	origin;
		vec3_t	dir={0,0,1};

		BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
		origin[2] += 8;
		SnapVector( origin );
		G_SetOrigin( ent, origin );

		ent->s.eType = ET_GENERAL;
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
		ent->freeAfterEvent = qtrue;

		if (G_RadiusDamage( ent->r.currentOrigin, ent->parent,  ent->splashDamage, ent->splashRadius, 
				ent, ent, ent->splashMethodOfDeath))
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}

		trap_LinkEntity( ent );
	}
}

void thermalThinkStandard(gentity_t *ent)
{
	if (ent->genericValue5 < level.time)
	{
		ent->think = thermalDetonatorExplode;
		ent->nextthink = level.time;
		return;
	}

	G_RunObject(ent);
	ent->nextthink = level.time;
}

//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (ent->client->skillLevel[SK_THERMAL] == FORCE_LEVEL_3)
	{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	
	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt->r.mins, bolt->r.maxs );//make sure our start point isn't on the other side of a wall

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt->s.pos.trDelta[1] -=30 ;
		bolt->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->methodOfDeath = MOD_THERMAL;
	bolt->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	VectorCopy( start, bolt->pos2 );

	bolt->bounceCount = -5;

	//return bolt;
	
	// THIS FUNCTION NEEDS SERIOUS FIXING cannot return bolt, bolt2 together
	

	
	gentity_t	*bolt2;
	

	bolt2 = G_Spawn();
	
	bolt2->physicsObject = qtrue;

	bolt2->classname = "thermal_detonator";
	bolt2->think = thermalThinkStandard;
	bolt2->nextthink = level.time;
	bolt2->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt2->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt2->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt2->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt2->r.mins, bolt2->r.maxs );//make sure our start point isn't on the other side of a wall


	// normal ones bounce, alt ones explode on impact
	bolt2->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt2->s.pos.trType = TR_GRAVITY;
	bolt2->parent = ent;
	bolt2->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt2->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt2->s.pos.trDelta[1] +=30 ;
		bolt2->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt2->flags |= FL_BOUNCE_HALF;
	}

	bolt2->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt2->s.loopIsSoundset = qfalse;

	bolt2->damage = TD_DAMAGE;
	bolt2->dflags = 0;
	bolt2->splashDamage = TD_SPLASH_DAM;
	bolt2->splashRadius = TD_SPLASH_RAD;

	bolt2->s.eType = ET_MISSILE;
	bolt2->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt2->s.weapon = WP_THERMAL;
	bolt2->methodOfDeath = MOD_THERMAL;
	bolt2->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt2->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt2->s.pos.trBase );
	
	SnapVector( bolt2->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt2->r.currentOrigin);

	VectorCopy( start, bolt2->pos2 );

	bolt2->bounceCount = -5;
	

	return bolt, bolt2; // which bolt are we using
	}
	else
	{
	gentity_t* bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if (chargeAmount > 1.0f)
	{
		chargeAmount = 1.0f;
	}
	else if (chargeAmount < TD_MIN_CHARGE)
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!altFire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->methodOfDeath = MOD_THERMAL;
	bolt->splashMethodOfDeath = MOD_THERMAL_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	return bolt; 
	}
}
//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (ent->client->skillLevel[SK_THERMAL] == FORCE_LEVEL_3)
	{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	
	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt->r.mins, bolt->r.maxs );//make sure our start point isn't on the other side of a wall

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt->s.pos.trDelta[1] -=30 ;
		bolt->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_2;
	bolt->methodOfDeath = MOD_INCINERATOR_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	VectorCopy( start, bolt->pos2 );

	bolt->bounceCount = -5;


	
	
	

	
	gentity_t	*bolt2;
	

	bolt2 = G_Spawn();
	
	bolt2->physicsObject = qtrue;

	bolt2->classname = "thermal_detonator";
	bolt2->think = thermalThinkStandard;
	bolt2->nextthink = level.time;
	bolt2->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt2->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt2->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt2->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt2->r.mins, bolt2->r.maxs );//make sure our start point isn't on the other side of a wall


	// normal ones bounce, alt ones explode on impact
	bolt2->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt2->s.pos.trType = TR_GRAVITY;
	bolt2->parent = ent;
	bolt2->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt2->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt2->s.pos.trDelta[1] +=30 ;
		bolt2->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt2->flags |= FL_BOUNCE_HALF;
	}

	bolt2->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt2->s.loopIsSoundset = qfalse;

	bolt2->damage = TD_DAMAGE;
	bolt2->dflags = 0;
	bolt2->splashDamage = TD_SPLASH_DAM;
	bolt2->splashRadius = TD_SPLASH_RAD;

	bolt2->s.eType = ET_MISSILE;
	bolt2->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt2->s.weapon = WP_THERMAL;
	bolt2->s.eFlags |= EF_WP_OPTION_2;
	bolt2->methodOfDeath = MOD_INCINERATOR_EXPLOSION;
	bolt2->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;

	bolt2->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt2->s.pos.trBase );
	
	SnapVector( bolt2->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt2->r.currentOrigin);

	VectorCopy( start, bolt2->pos2 );

	bolt2->bounceCount = -5;
	

	return bolt, bolt2;
	}
	else
	{
	gentity_t* bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if (chargeAmount > 1.0f)
	{
		chargeAmount = 1.0f;
	}
	else if (chargeAmount < TD_MIN_CHARGE)
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!altFire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_2;
	bolt->methodOfDeath = MOD_INCINERATOR_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	return bolt; 
	}
}
//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (ent->client->skillLevel[SK_THERMAL] == FORCE_LEVEL_3)
	{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	
	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt->r.mins, bolt->r.maxs );//make sure our start point isn't on the other side of a wall

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt->s.pos.trDelta[1] -=30 ;
		bolt->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_3;
	bolt->methodOfDeath = MOD_DIOXIS_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	VectorCopy( start, bolt->pos2 );

	bolt->bounceCount = -5;


	
	
	

	
	gentity_t	*bolt2;
	

	bolt2 = G_Spawn();
	
	bolt2->physicsObject = qtrue;

	bolt2->classname = "thermal_detonator";
	bolt2->think = thermalThinkStandard;
	bolt2->nextthink = level.time;
	bolt2->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt2->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt2->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt2->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt2->r.mins, bolt2->r.maxs );//make sure our start point isn't on the other side of a wall


	// normal ones bounce, alt ones explode on impact
	bolt2->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt2->s.pos.trType = TR_GRAVITY;
	bolt2->parent = ent;
	bolt2->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt2->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt2->s.pos.trDelta[1] +=30 ;
		bolt2->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt2->flags |= FL_BOUNCE_HALF;
	}

	bolt2->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt2->s.loopIsSoundset = qfalse;

	bolt2->damage = TD_DAMAGE;
	bolt2->dflags = 0;
	bolt2->splashDamage = TD_SPLASH_DAM;
	bolt2->splashRadius = TD_SPLASH_RAD;

	bolt2->s.eType = ET_MISSILE;
	bolt2->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt2->s.weapon = WP_THERMAL;
	bolt2->s.eFlags |= EF_WP_OPTION_3;
	bolt2->methodOfDeath = MOD_DIOXIS_EXPLOSION;
	bolt2->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;

	bolt2->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt2->s.pos.trBase );
	
	SnapVector( bolt2->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt2->r.currentOrigin);

	VectorCopy( start, bolt2->pos2 );

	bolt2->bounceCount = -5;
	

	return bolt, bolt2;
	}
	else
	{
	gentity_t* bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if (chargeAmount > 1.0f)
	{
		chargeAmount = 1.0f;
	}
	else if (chargeAmount < TD_MIN_CHARGE)
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!altFire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_3;
	bolt->methodOfDeath = MOD_DIOXIS_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	return bolt; 
	}
}
//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if (ent->client->skillLevel[SK_THERMAL] == FORCE_LEVEL_3)
	{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge
	
	VectorCopy( forward, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt->r.mins, bolt->r.maxs );//make sure our start point isn't on the other side of a wall

	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt->s.pos.trDelta[1] -=30 ;
		bolt->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_4;
	bolt->methodOfDeath = MOD_ION_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	VectorCopy( start, bolt->pos2 );

	bolt->bounceCount = -5;


	
	
	

	
	gentity_t	*bolt2;
	

	bolt2 = G_Spawn();
	
	bolt2->physicsObject = qtrue;

	bolt2->classname = "thermal_detonator";
	bolt2->think = thermalThinkStandard;
	bolt2->nextthink = level.time;
	bolt2->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet( bolt2->r.mins, -3.0f, -3.0f, -3.0f );
	VectorSet( bolt2->r.maxs, 3.0f, 3.0f, 3.0f );
	bolt2->clipmask = MASK_SHOT;

	W_TraceSetStart( ent, start, bolt2->r.mins, bolt2->r.maxs );//make sure our start point isn't on the other side of a wall


	// normal ones bounce, alt ones explode on impact
	bolt2->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt2->s.pos.trType = TR_GRAVITY;
	bolt2->parent = ent;
	bolt2->r.ownerNum = ent->s.number;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt2->s.pos.trDelta );

	if ( ent->health >= 0 )
	{
		bolt2->s.pos.trDelta[1] +=30 ;
		bolt2->s.pos.trDelta[2] += 120;
	}

	if ( !altFire )
	{
		bolt2->flags |= FL_BOUNCE_HALF;
	}

	bolt2->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );
	bolt2->s.loopIsSoundset = qfalse;

	bolt2->damage = TD_DAMAGE;
	bolt2->dflags = 0;
	bolt2->splashDamage = TD_SPLASH_DAM;
	bolt2->splashRadius = TD_SPLASH_RAD;

	bolt2->s.eType = ET_MISSILE;
	bolt2->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt2->s.weapon = WP_THERMAL;
	bolt2->s.eFlags |= EF_WP_OPTION_4;
	bolt2->methodOfDeath = MOD_ION_EXPLOSION;
	bolt2->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;

	bolt2->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt2->s.pos.trBase );
	
	SnapVector( bolt2->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt2->r.currentOrigin);

	VectorCopy( start, bolt2->pos2 );

	bolt2->bounceCount = -5;
	

	return bolt, bolt2;
	}
	else
	{
	gentity_t* bolt;
	vec3_t		dir, start;
	float chargeAmount = 1.0f; // default of full charge

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	bolt = G_Spawn();

	bolt->physicsObject = qtrue;

	bolt->classname = "thermal_detonator";
	bolt->think = thermalThinkStandard;
	bolt->nextthink = level.time;
	bolt->touch = touch_NULL;

	// How 'bout we give this thing a size...
	VectorSet(bolt->r.mins, -3.0f, -3.0f, -3.0f);
	VectorSet(bolt->r.maxs, 3.0f, 3.0f, 3.0f);
	bolt->clipmask = MASK_SHOT;

	W_TraceSetStart(ent, start, bolt->r.mins, bolt->r.maxs);//make sure our start point isn't on the other side of a wall

	if (ent->client)
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if (chargeAmount > 1.0f)
	{
		chargeAmount = 1.0f;
	}
	else if (chargeAmount < TD_MIN_CHARGE)
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	// normal ones bounce, alt ones explode on impact
	bolt->genericValue5 = level.time + TD_TIME; // How long 'til she blows
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->parent = ent;
	bolt->r.ownerNum = ent->s.number;
	VectorScale(dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta);

	if (ent->health >= 0)
	{
		bolt->s.pos.trDelta[2] += 120;
	}

	if (!altFire)
	{
		bolt->flags |= FL_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex("sound/weapons/thermal/thermloop.wav");
	bolt->s.loopIsSoundset = qfalse;

	bolt->damage = TD_DAMAGE;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;
	bolt->s.eFlags |= EF_WP_OPTION_4;
	bolt->methodOfDeath = MOD_ION_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	VectorCopy(start, bolt->pos2);

	bolt->bounceCount = -5;

	return bolt; 
	}
}
gentity_t *WP_DropThermal( gentity_t *ent )
{
	AngleVectors( ent->client->ps.viewangles, forward, vright, up );
	return (WP_FireThermalDetonator( ent, qfalse ));
}
gentity_t *WP_DropThermal2( gentity_t *ent )
{
	AngleVectors( ent->client->ps.viewangles, forward, vright, up );
	return (WP_FireThermalDetonator2( ent, qfalse ));
}
gentity_t *WP_DropThermal3( gentity_t *ent )
{
	AngleVectors( ent->client->ps.viewangles, forward, vright, up );
	return (WP_FireThermalDetonator3( ent, qfalse ));
}
gentity_t *WP_DropThermal4( gentity_t *ent )
{
	AngleVectors( ent->client->ps.viewangles, forward, vright, up );
	return (WP_FireThermalDetonator4( ent, qfalse ));
}
//---------------------------------------------------------
qboolean WP_LobFire( gentity_t *self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, int clipmask, 
				vec3_t velocity, qboolean tracePath, int ignoreEntNum, int enemyNum,
				float minSpeed, float maxSpeed, float idealSpeed, qboolean mustHit )
//---------------------------------------------------------
{ //for the galak mech NPC
	float	targetDist, shotSpeed, speedInc = 100, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed, 
	vec3_t	targetDir, shotVel, failCase; 
	trace_t	trace;
	trajectory_t	tr;
	qboolean	blocked;
	int		elapsedTime, skipNum, timeStep = 500, hitCount = 0, maxHits = 7;
	vec3_t	lastPos, testPos;
	gentity_t	*traceEnt;
	
	if ( !idealSpeed )
	{
		idealSpeed = 300;
	}
	else if ( idealSpeed < speedInc )
	{
		idealSpeed = speedInc;
	}
	shotSpeed = idealSpeed;
	skipNum = (idealSpeed-speedInc)/speedInc;
	if ( !minSpeed )
	{
		minSpeed = 100;
	}
	if ( !maxSpeed )
	{
		maxSpeed = 900;
	}
	while ( hitCount < maxHits )
	{
		VectorSubtract( target, start, targetDir );
		targetDist = VectorNormalize( targetDir );

		VectorScale( targetDir, shotSpeed, shotVel );
		travelTime = targetDist/shotSpeed;
		shotVel[2] += travelTime * 0.5 * g_gravity.value;

		if ( !hitCount )		
		{//save the first (ideal) one as the failCase (fallback value)
			if ( !mustHit )
			{//default is fine as a return value
				VectorCopy( shotVel, failCase );
			}
		}

		if ( tracePath )
		{//do a rough trace of the path
			blocked = qfalse;

			VectorCopy( start, tr.trBase );
			VectorCopy( shotVel, tr.trDelta );
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travelTime *= 1000.0f;
			VectorCopy( start, lastPos );
			
			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
			{
				if ( (float)elapsedTime > travelTime )
				{//cap it
					elapsedTime = floor( travelTime );
				}
				BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
				trap_Trace( &trace, lastPos, mins, maxs, testPos, ignoreEntNum, clipmask );

				if ( trace.allsolid || trace.startsolid )
				{
					blocked = qtrue;
					break;
				}
				if ( trace.fraction < 1.0f )
				{//hit something
					if ( trace.entityNum == enemyNum )
					{//hit the enemy, that's perfect!
						break;
					}
					else if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, target ) < 4096 )//hit within 64 of desired location, should be okay
					{//close enough!
						break;
					}
					else
					{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
						impactDist = DistanceSquared( trace.endpos, target );
						if ( impactDist < bestImpactDist )
						{
							bestImpactDist = impactDist;
							VectorCopy( shotVel, failCase );
						}
						blocked = qtrue;
						//see if we should store this as the failCase
						if ( trace.entityNum < ENTITYNUM_WORLD )
						{//hit an ent
							traceEnt = &g_entities[trace.entityNum];
							if ( traceEnt && traceEnt->takedamage && !OnSameTeam( self, traceEnt ) )
							{//hit something breakable, so that's okay
								//we haven't found a clear shot yet so use this as the failcase
								VectorCopy( shotVel, failCase );
							}
						}
						break;
					}
				}
				if ( elapsedTime == floor( travelTime ) )
				{//reached end, all clear
					break;
				}
				else
				{
					//all clear, try next slice
					VectorCopy( testPos, lastPos );
				}
			}
			if ( blocked )
			{//hit something, adjust speed (which will change arc)
				hitCount++;
				shotSpeed = idealSpeed + ((hitCount-skipNum) * speedInc);//from min to max (skipping ideal)
				if ( hitCount >= skipNum )
				{//skip ideal since that was the first value we tested
					shotSpeed += speedInc;
				}
			}
			else
			{//made it!
				break;
			}
		}
		else
		{//no need to check the path, go with first calc
			break;
		}
	}

	if ( hitCount >= maxHits )
	{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		VectorCopy( failCase, velocity );
		return qfalse;
	}
	VectorCopy( shotVel, velocity );
	return qtrue;
}

/*
======================================================================

LASER TRAP / TRIP MINE

======================================================================
*/
#define LT_DAMAGE			300
#define LT_SPLASH_RAD		512.0f
#define LT_SPLASH_DAM		150
#define LT_VELOCITY			900.0f
#define LT_SIZE				1.5f
#define LT_ALT_TIME			2000
#define	LT_ACTIVATION_DELAY	1000
#define	LT_DELAY_TIME		50

void laserTrapExplode( gentity_t *self )
{
	vec3_t v;
	self->takedamage = qfalse;

	if (self->activator)
	{
	if (self->s.weapon == WP_FLECHETTE)
	{
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, MOD_TRIP_MINE_SPLASH/*MOD_LT_SPLASH*/ );		
	}
	else
	{
	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, MOD_INCINERATOR_EXPLOSION_SPLASH/*MOD_LT_SPLASH*/ );
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, MOD_DIOXIS_EXPLOSION_SPLASH/*MOD_LT_SPLASH*/ );
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, MOD_DEMP2/*MOD_LT_SPLASH*/ );
	}
	else
	{
		G_RadiusDamage( self->r.currentOrigin, self->activator, self->splashDamage, self->splashRadius, self, self, MOD_TRIP_MINE_SPLASH/*MOD_LT_SPLASH*/ );		
	}
	}
	
	}

	if (self->s.weapon != WP_FLECHETTE)
	{
		G_AddEvent( self, EV_MISSILE_MISS, 0);
	}

	VectorCopy(self->s.pos.trDelta, v);
	//Explode outward from the surface

	if (self->s.time == -2)
	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}

	if (self->s.weapon == WP_FLECHETTE)
	{
		G_PlayEffect(EFFECT_EXPLOSION_FLECHETTE, self->r.currentOrigin, v);
	}
	else 
	{
		if(self->s.eFlags & EF_WP_OPTION_2)
		{
		G_PlayEffect(EFFECT_EXPLOSION_TRIPMINE2, self->r.currentOrigin, v);			
		}
		else if(self->s.eFlags & EF_WP_OPTION_3)
		{
		G_PlayEffect(EFFECT_EXPLOSION_TRIPMINE3, self->r.currentOrigin, v);			
		}		
		else if(self->s.eFlags & EF_WP_OPTION_4)
		{
		G_PlayEffect(EFFECT_EXPLOSION_TRIPMINE4, self->r.currentOrigin, v);			
		}
		else
		{
		G_PlayEffect(EFFECT_EXPLOSION_TRIPMINE, self->r.currentOrigin, v);	
		}		
	}


	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

void laserTrapDelayedExplode( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	self->enemy = attacker;
	self->think = laserTrapExplode;
	self->nextthink = level.time + FRAMETIME;
	self->takedamage = qfalse;
	if ( attacker && !attacker->s.number )
	{
		//less damage when shot by player
		self->splashDamage /= 3;
		self->splashRadius /= 3;
	}
}

void touchLaserTrap( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if (other && other->s.number < ENTITYNUM_WORLD)
	{ //just explode if we hit any entity. This way we don't have things happening like tripmines floating
	  //in the air after getting stuck to a moving door
		if ( ent->activator != other )
		{
			ent->touch = 0;
			ent->nextthink = level.time + FRAMETIME;
			ent->think = laserTrapExplode;
			VectorCopy(trace->plane.normal, ent->s.pos.trDelta);
		}
	}
	else
	{
		ent->touch = 0;
		if (trace->entityNum != ENTITYNUM_NONE)
		{
			ent->enemy = &g_entities[trace->entityNum];
		}
		laserTrapStick(ent, trace->endpos, trace->plane.normal);
	}
}

void proxMineThink(gentity_t *ent)
{
	int i = 0;
	gentity_t *cl;
	gentity_t *owner = NULL;

	if (ent->r.ownerNum < ENTITYNUM_WORLD)
	{
		owner = &g_entities[ent->r.ownerNum];
	}

	ent->nextthink = level.time;

	if (ent->genericValue15 < level.time ||
		!owner ||
		!owner->inuse ||
		!owner->client ||
		owner->client->pers.connected != CON_CONNECTED)
	{ //time to die!
		ent->think = laserTrapExplode;
		return;
	}

	while (i < MAX_CLIENTS)
	{ //eh, just check for clients, don't care about anyone else...
		cl = &g_entities[i];

		if (cl->inuse && cl->client && cl->client->pers.connected == CON_CONNECTED &&
			owner != cl && cl->client->sess.sessionTeam != TEAM_SPECTATOR &&
			cl->client->tempSpectate < level.time && cl->health > 0)
		{
			if (!OnSameTeam(owner, cl) || g_friendlyFire.integer)
			{ //not on the same team, or friendly fire is enabled
				vec3_t v;

				VectorSubtract(ent->r.currentOrigin, cl->client->ps.origin, v);
				if (VectorLength(v) < (ent->splashRadius/2.0f))
				{
					ent->think = laserTrapExplode;
					return;
				}
			}
		}
		i++;
	}
}

void laserTrapThink ( gentity_t *ent )
{
	gentity_t	*traceEnt;
	vec3_t		end;
	trace_t		tr;

	//just relink it every think
	trap_LinkEntity(ent);

	//turn on the beam effect
	if ( !(ent->s.eFlags&EF_FIRING) )
	{//arm me
		G_Sound( ent, CHAN_WEAPON, G_SoundIndex( "sound/weapons/laser_trap/warning.wav" ) );
		ent->s.eFlags |= EF_FIRING;
	}
	ent->think = laserTrapThink;
	ent->nextthink = level.time + FRAMETIME;

	// Find the main impact point
	VectorMA ( ent->s.pos.trBase, 1024, ent->movedir, end );
	trap_Trace ( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT);
	
	traceEnt = &g_entities[ tr.entityNum ];

	ent->s.time = -1; //let all clients know to draw a beam from this guy

	if ( traceEnt->client || tr.startsolid )
	{
		//go boom
		ent->touch = 0;
		ent->nextthink = level.time + LT_DELAY_TIME;
		ent->think = laserTrapExplode;
	}
}

void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal )
{
	G_SetOrigin( ent, endpos );
	VectorCopy( normal, ent->pos1 );

	VectorClear( ent->s.apos.trDelta );
	// This will orient the object to face in the direction of the normal
	VectorCopy( normal, ent->s.pos.trDelta );
	//VectorScale( normal, -1, ent->s.pos.trDelta );
	ent->s.pos.trTime = level.time;
	
	
	//This does nothing, cg_missile makes assumptions about direction of travel controlling angles
	vectoangles( normal, ent->s.apos.trBase );
	VectorClear( ent->s.apos.trDelta );
	ent->s.apos.trType = TR_STATIONARY;
	VectorCopy( ent->s.apos.trBase, ent->s.angles );
	VectorCopy( ent->s.angles, ent->r.currentAngles );
	

	G_Sound( ent, CHAN_WEAPON, G_SoundIndex( "sound/weapons/laser_trap/stick.wav" ) );
	if ( ent->count )
	{//a tripwire
		//add draw line flag
		VectorCopy( normal, ent->movedir );
		ent->think = laserTrapThink;
		ent->nextthink = level.time + LT_ACTIVATION_DELAY;//delay the activation
		ent->touch = touch_NULL;
		//make it shootable
		ent->takedamage = qtrue;
		ent->health = 5;
		ent->die = laserTrapDelayedExplode;

		//shove the box through the wall
		VectorSet( ent->r.mins, -LT_SIZE*2, -LT_SIZE*2, -LT_SIZE*2 );
		VectorSet( ent->r.maxs, LT_SIZE*2, LT_SIZE*2, LT_SIZE*2 );

		//so that the owner can blow it up with projectiles
		ent->r.svFlags |= SVF_OWNERNOTSHARED;
	}
	else
	{
		ent->touch = touchLaserTrap;
		ent->think = proxMineThink;//laserTrapExplode;
		ent->genericValue15 = level.time + 30000; //auto-explode after 30 seconds.
		ent->nextthink = level.time + LT_ALT_TIME; // How long 'til she blows

		//make it shootable
		ent->takedamage = qtrue;
		ent->health = 5;
		ent->die = laserTrapDelayedExplode;

		//shove the box through the wall
		VectorSet( ent->r.mins, -LT_SIZE*2, -LT_SIZE*2, -LT_SIZE*2 );
		VectorSet( ent->r.maxs, LT_SIZE*2, LT_SIZE*2, LT_SIZE*2 );

		//so that the owner can blow it up with projectiles
		ent->r.svFlags |= SVF_OWNERNOTSHARED;

		if ( !(ent->s.eFlags&EF_FIRING) )
		{//arm me
			G_Sound( ent, CHAN_WEAPON, G_SoundIndex( "sound/weapons/laser_trap/warning.wav" ) );
			ent->s.eFlags |= EF_FIRING;
			ent->s.time = -1;
			ent->s.bolt2 = 1;
		}
	}
}

void TrapThink(gentity_t *ent)
{ //laser trap think
	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void CreateLaserTrap( gentity_t *laserTrap, vec3_t start, gentity_t *owner )
{ //create a laser trap entity
	laserTrap->classname = "laserTrap";
	laserTrap->flags |= FL_BOUNCE_HALF;
	laserTrap->s.eFlags |= EF_MISSILE_STICK;
	laserTrap->splashRadius = LT_SPLASH_RAD;
	laserTrap->splashDamage = LT_SPLASH_DAM;
	if (owner->client && owner->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	laserTrap->damage = 2*LT_DAMAGE;
	}
	else
	{
	laserTrap->damage = LT_DAMAGE;
	}
	
	if (owner->client && owner->client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && owner->client->skillLevel[SK_TRIPMINEA] == FORCE_LEVEL_2 )
	{
		laserTrap->s.eFlags |= EF_WP_OPTION_2;
		laserTrap->methodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
		laserTrap->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
	}
	else if (owner->client && owner->client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && owner->client->skillLevel[SK_TRIPMINEA] == FORCE_LEVEL_3)
	{
		laserTrap->s.eFlags |= EF_WP_OPTION_3;
		laserTrap->methodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
		laserTrap->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
	}
	else if (owner->client && owner->client->skillLevel[SK_TRIPMINE] >= FORCE_LEVEL_1 && owner->client->skillLevel[SK_TRIPMINEB] == FORCE_LEVEL_1)
	{
		laserTrap->s.eFlags |= EF_WP_OPTION_4;
		laserTrap->methodOfDeath = MOD_ION_EXPLOSION;
		laserTrap->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;
	}
	else
	{
		laserTrap->methodOfDeath = MOD_TRIP_MINE_SPLASH;
		laserTrap->splashMethodOfDeath = MOD_TRIP_MINE_SPLASH;			
	}

	laserTrap->s.eType = ET_GENERAL;
	laserTrap->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	laserTrap->s.weapon = WP_TRIP_MINE;
	laserTrap->s.pos.trType = TR_GRAVITY;
	laserTrap->r.contents = MASK_SHOT;
	laserTrap->parent = owner;
	laserTrap->activator = owner;
	laserTrap->r.ownerNum = owner->s.number;
	VectorSet( laserTrap->r.mins, -LT_SIZE, -LT_SIZE, -LT_SIZE );
	VectorSet( laserTrap->r.maxs, LT_SIZE, LT_SIZE, LT_SIZE );
	laserTrap->clipmask = MASK_SHOT;
	laserTrap->s.solid = 2;
	laserTrap->s.modelindex = G_ModelIndex( "models/weapons2/laser_trap/laser_trap_w.glm" );
	laserTrap->s.modelGhoul2 = 1;
	laserTrap->s.g2radius = 40;

	laserTrap->s.genericenemyindex = owner->s.number+MAX_GENTITIES;

	laserTrap->health = 1;

	laserTrap->s.time = 0;

	laserTrap->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, laserTrap->s.pos.trBase );
	SnapVector( laserTrap->s.pos.trBase );			// save net bandwidth
	
	SnapVector( laserTrap->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, laserTrap->r.currentOrigin);

	laserTrap->s.apos.trType = TR_GRAVITY;
	laserTrap->s.apos.trTime = level.time;
	laserTrap->s.apos.trBase[YAW] = rand()%360;
	laserTrap->s.apos.trBase[PITCH] = rand()%360;
	laserTrap->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		laserTrap->s.apos.trBase[YAW] = -laserTrap->s.apos.trBase[YAW];
	}

	VectorCopy( start, laserTrap->pos2 );
	laserTrap->touch = touchLaserTrap;
	laserTrap->think = TrapThink;
	laserTrap->nextthink = level.time + 50;
}

void WP_PlaceLaserTrap(gentity_t* ent, qboolean alt_fire)
{
	gentity_t* laserTrap;
	//	gentity_t	*found = NULL;
	vec3_t		dir, start;
	//	int			trapcount = 0;
	int			foundLaserTraps[MAX_GENTITIES];

	foundLaserTraps[0] = ENTITYNUM_NONE;

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	laserTrap = G_Spawn();

	//limit to 10 placed at any one time
	//see how many there are now
	/*
	while ( (found = G_Find( found, FOFS(classname), "laserTrap" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundLaserTraps[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundLaserTraps[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundLaserTraps[i]];
			if ( laserTrap && found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundLaserTraps[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				G_FreeEntity( &g_entities[foundLaserTraps[removeMe]] );
			}
			foundLaserTraps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}
	*/
	//now make the new one
	CreateLaserTrap(laserTrap, start, ent);

	//set player-created-specific fields
	laserTrap->setTime = level.time;//remember when we placed it

	if (!alt_fire)
	{//tripwire
		laserTrap->count = 1;
	}

	//move it
	laserTrap->s.pos.trType = TR_GRAVITY;

	if (alt_fire)
	{
		VectorScale(dir, 512, laserTrap->s.pos.trDelta);
	}
	else
	{
		VectorScale(dir, 256, laserTrap->s.pos.trDelta);
	}

	trap_LinkEntity(laserTrap);

}

/*
======================================================================

LASER TRAP2 / TRIP MINE2

======================================================================
*/
void CreateLaserTrap2( gentity_t *laserTrap, vec3_t start, gentity_t *owner )
{ //create a laser trap entity
	laserTrap->classname = "laserTrap";
	laserTrap->flags |= FL_BOUNCE_HALF;
	laserTrap->s.eFlags |= EF_MISSILE_STICK;
	laserTrap->splashRadius = LT_SPLASH_RAD;
	laserTrap->splashDamage = LT_SPLASH_DAM;
	if (owner->client && owner->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	laserTrap->damage = 2*LT_DAMAGE;
	}
	else
	{
	laserTrap->damage = LT_DAMAGE;
	}
	

	
	laserTrap->s.eFlags |= EF_WP_OPTION_2;
	laserTrap->methodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
	laserTrap->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
	


	laserTrap->s.eType = ET_GENERAL;
	laserTrap->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	laserTrap->s.weapon = WP_TRIP_MINE;
	laserTrap->s.pos.trType = TR_GRAVITY;
	laserTrap->r.contents = MASK_SHOT;
	laserTrap->parent = owner;
	laserTrap->activator = owner;
	laserTrap->r.ownerNum = owner->s.number;
	VectorSet( laserTrap->r.mins, -LT_SIZE, -LT_SIZE, -LT_SIZE );
	VectorSet( laserTrap->r.maxs, LT_SIZE, LT_SIZE, LT_SIZE );
	laserTrap->clipmask = MASK_SHOT;
	laserTrap->s.solid = 2;
	laserTrap->s.modelindex = G_ModelIndex( "models/weapons2/laser_trap/laser_trap_w.glm" );
	laserTrap->s.modelGhoul2 = 1;
	laserTrap->s.g2radius = 40;

	laserTrap->s.genericenemyindex = owner->s.number+MAX_GENTITIES;

	laserTrap->health = 1;

	laserTrap->s.time = 0;

	laserTrap->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, laserTrap->s.pos.trBase );
	SnapVector( laserTrap->s.pos.trBase );			// save net bandwidth
	
	SnapVector( laserTrap->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, laserTrap->r.currentOrigin);

	laserTrap->s.apos.trType = TR_GRAVITY;
	laserTrap->s.apos.trTime = level.time;
	laserTrap->s.apos.trBase[YAW] = rand()%360;
	laserTrap->s.apos.trBase[PITCH] = rand()%360;
	laserTrap->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		laserTrap->s.apos.trBase[YAW] = -laserTrap->s.apos.trBase[YAW];
	}

	VectorCopy( start, laserTrap->pos2 );
	laserTrap->touch = touchLaserTrap;
	laserTrap->think = TrapThink;
	laserTrap->nextthink = level.time + 50;
}

void WP_PlaceLaserTrap2(gentity_t* ent, qboolean alt_fire)
{
	gentity_t* laserTrap;
	//	gentity_t	*found = NULL;
	vec3_t		dir, start;
	//	int			trapcount = 0;
	int			foundLaserTraps[MAX_GENTITIES];

	foundLaserTraps[0] = ENTITYNUM_NONE;

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	laserTrap = G_Spawn();

	//limit to 10 placed at any one time
	//see how many there are now
	/*
	while ( (found = G_Find( found, FOFS(classname), "laserTrap" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundLaserTraps[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundLaserTraps[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundLaserTraps[i]];
			if ( laserTrap && found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundLaserTraps[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				G_FreeEntity( &g_entities[foundLaserTraps[removeMe]] );
			}
			foundLaserTraps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}
	*/
	//now make the new one
	CreateLaserTrap2(laserTrap, start, ent);

	//set player-created-specific fields
	laserTrap->setTime = level.time;//remember when we placed it

	if (!alt_fire)
	{//tripwire
		laserTrap->count = 1;
	}

	//move it
	laserTrap->s.pos.trType = TR_GRAVITY;

	if (alt_fire)
	{
		VectorScale(dir, 512, laserTrap->s.pos.trDelta);
	}
	else
	{
		VectorScale(dir, 256, laserTrap->s.pos.trDelta);
	}

	trap_LinkEntity(laserTrap);

}
/*
======================================================================

LASER TRAP3 / TRIP MINE3

======================================================================
*/
void CreateLaserTrap3( gentity_t *laserTrap, vec3_t start, gentity_t *owner )
{ //create a laser trap entity
	laserTrap->classname = "laserTrap";
	laserTrap->flags |= FL_BOUNCE_HALF;
	laserTrap->s.eFlags |= EF_MISSILE_STICK;
	laserTrap->splashRadius = LT_SPLASH_RAD;
	laserTrap->splashDamage = LT_SPLASH_DAM;
	if (owner->client && owner->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	laserTrap->damage = 2*LT_DAMAGE;
	}
	else
	{
	laserTrap->damage = LT_DAMAGE;
	}
	

	
	laserTrap->s.eFlags |= EF_WP_OPTION_3;
	laserTrap->methodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
	laserTrap->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
	


	laserTrap->s.eType = ET_GENERAL;
	laserTrap->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	laserTrap->s.weapon = WP_TRIP_MINE;
	laserTrap->s.pos.trType = TR_GRAVITY;
	laserTrap->r.contents = MASK_SHOT;
	laserTrap->parent = owner;
	laserTrap->activator = owner;
	laserTrap->r.ownerNum = owner->s.number;
	VectorSet( laserTrap->r.mins, -LT_SIZE, -LT_SIZE, -LT_SIZE );
	VectorSet( laserTrap->r.maxs, LT_SIZE, LT_SIZE, LT_SIZE );
	laserTrap->clipmask = MASK_SHOT;
	laserTrap->s.solid = 2;
	laserTrap->s.modelindex = G_ModelIndex( "models/weapons2/laser_trap/laser_trap_w.glm" );
	laserTrap->s.modelGhoul2 = 1;
	laserTrap->s.g2radius = 40;

	laserTrap->s.genericenemyindex = owner->s.number+MAX_GENTITIES;

	laserTrap->health = 1;

	laserTrap->s.time = 0;

	laserTrap->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, laserTrap->s.pos.trBase );
	SnapVector( laserTrap->s.pos.trBase );			// save net bandwidth
	
	SnapVector( laserTrap->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, laserTrap->r.currentOrigin);

	laserTrap->s.apos.trType = TR_GRAVITY;
	laserTrap->s.apos.trTime = level.time;
	laserTrap->s.apos.trBase[YAW] = rand()%360;
	laserTrap->s.apos.trBase[PITCH] = rand()%360;
	laserTrap->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		laserTrap->s.apos.trBase[YAW] = -laserTrap->s.apos.trBase[YAW];
	}

	VectorCopy( start, laserTrap->pos2 );
	laserTrap->touch = touchLaserTrap;
	laserTrap->think = TrapThink;
	laserTrap->nextthink = level.time + 50;
}

void WP_PlaceLaserTrap3(gentity_t* ent, qboolean alt_fire)
{
	gentity_t* laserTrap;
	//	gentity_t	*found = NULL;
	vec3_t		dir, start;
	//	int			trapcount = 0;
	int			foundLaserTraps[MAX_GENTITIES];

	foundLaserTraps[0] = ENTITYNUM_NONE;

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	laserTrap = G_Spawn();

	//limit to 10 placed at any one time
	//see how many there are now
	/*
	while ( (found = G_Find( found, FOFS(classname), "laserTrap" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundLaserTraps[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundLaserTraps[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundLaserTraps[i]];
			if ( laserTrap && found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundLaserTraps[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				G_FreeEntity( &g_entities[foundLaserTraps[removeMe]] );
			}
			foundLaserTraps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}
	*/
	//now make the new one
	CreateLaserTrap3(laserTrap, start, ent);

	//set player-created-specific fields
	laserTrap->setTime = level.time;//remember when we placed it

	if (!alt_fire)
	{//tripwire
		laserTrap->count = 1;
	}

	//move it
	laserTrap->s.pos.trType = TR_GRAVITY;

	if (alt_fire)
	{
		VectorScale(dir, 512, laserTrap->s.pos.trDelta);
	}
	else
	{
		VectorScale(dir, 256, laserTrap->s.pos.trDelta);
	}

	trap_LinkEntity(laserTrap);

}
/*
======================================================================

LASER TRAP4 / TRIP MINE4

======================================================================
*/
void CreateLaserTrap4( gentity_t *laserTrap, vec3_t start, gentity_t *owner )
{ //create a laser trap entity
	laserTrap->classname = "laserTrap";
	laserTrap->flags |= FL_BOUNCE_HALF;
	laserTrap->s.eFlags |= EF_MISSILE_STICK;
	laserTrap->splashRadius = LT_SPLASH_RAD;
	laserTrap->splashDamage = LT_SPLASH_DAM;
	if (owner->client && owner->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	laserTrap->damage = 2*LT_DAMAGE;
	}
	else
	{
	laserTrap->damage = LT_DAMAGE;
	}
	
	
	laserTrap->s.eFlags |= EF_WP_OPTION_4;
	laserTrap->methodOfDeath = MOD_ION_EXPLOSION;
	laserTrap->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;
	


	laserTrap->s.eType = ET_GENERAL;
	laserTrap->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	laserTrap->s.weapon = WP_TRIP_MINE;
	laserTrap->s.pos.trType = TR_GRAVITY;
	laserTrap->r.contents = MASK_SHOT;
	laserTrap->parent = owner;
	laserTrap->activator = owner;
	laserTrap->r.ownerNum = owner->s.number;
	VectorSet( laserTrap->r.mins, -LT_SIZE, -LT_SIZE, -LT_SIZE );
	VectorSet( laserTrap->r.maxs, LT_SIZE, LT_SIZE, LT_SIZE );
	laserTrap->clipmask = MASK_SHOT;
	laserTrap->s.solid = 2;
	laserTrap->s.modelindex = G_ModelIndex( "models/weapons2/laser_trap/laser_trap_w.glm" );
	laserTrap->s.modelGhoul2 = 1;
	laserTrap->s.g2radius = 40;

	laserTrap->s.genericenemyindex = owner->s.number+MAX_GENTITIES;

	laserTrap->health = 1;

	laserTrap->s.time = 0;

	laserTrap->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, laserTrap->s.pos.trBase );
	SnapVector( laserTrap->s.pos.trBase );			// save net bandwidth
	
	SnapVector( laserTrap->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, laserTrap->r.currentOrigin);

	laserTrap->s.apos.trType = TR_GRAVITY;
	laserTrap->s.apos.trTime = level.time;
	laserTrap->s.apos.trBase[YAW] = rand()%360;
	laserTrap->s.apos.trBase[PITCH] = rand()%360;
	laserTrap->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		laserTrap->s.apos.trBase[YAW] = -laserTrap->s.apos.trBase[YAW];
	}

	VectorCopy( start, laserTrap->pos2 );
	laserTrap->touch = touchLaserTrap;
	laserTrap->think = TrapThink;
	laserTrap->nextthink = level.time + 50;
}

void WP_PlaceLaserTrap4(gentity_t* ent, qboolean alt_fire)
{
	gentity_t* laserTrap;
	//	gentity_t	*found = NULL;
	vec3_t		dir, start;
	//	int			trapcount = 0;
	int			foundLaserTraps[MAX_GENTITIES];

	foundLaserTraps[0] = ENTITYNUM_NONE;

	VectorCopy(forward, dir);
	VectorCopy(muzzle, start);

	laserTrap = G_Spawn();

	//limit to 10 placed at any one time
	//see how many there are now
	/*
	while ( (found = G_Find( found, FOFS(classname), "laserTrap" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundLaserTraps[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundLaserTraps[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundLaserTraps[i]];
			if ( laserTrap && found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundLaserTraps[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				G_FreeEntity( &g_entities[foundLaserTraps[removeMe]] );
			}
			foundLaserTraps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}
	*/
	//now make the new one
	CreateLaserTrap4(laserTrap, start, ent);

	//set player-created-specific fields
	laserTrap->setTime = level.time;//remember when we placed it

	if (!alt_fire)
	{//tripwire
		laserTrap->count = 1;
	}

	//move it
	laserTrap->s.pos.trType = TR_GRAVITY;

	if (alt_fire)
	{
		VectorScale(dir, 512, laserTrap->s.pos.trDelta);
	}
	else
	{
		VectorScale(dir, 256, laserTrap->s.pos.trDelta);
	}

	trap_LinkEntity(laserTrap);

}





/*
======================================================================

DET PACK

======================================================================
*/
void VectorNPos(vec3_t in, vec3_t out)
{
	if (in[0] < 0) { out[0] = -in[0]; } else { out[0] = in[0]; }
	if (in[1] < 0) { out[1] = -in[1]; } else { out[1] = in[1]; }
	if (in[2] < 0) { out[2] = -in[2]; } else { out[2] = in[2]; }
}

void DetPackBlow(gentity_t *self);

void charge_stick (gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t	*tent;

	if ( other 
		&& (other->flags&FL_BBRUSH)
		&& other->s.pos.trType == TR_STATIONARY
		&& other->s.apos.trType == TR_STATIONARY )
	{//a perfectly still breakable brush, let us attach directly to it!
		self->target_ent = other;//remember them when we blow up
	}
	else if ( other 
		&& other->s.number < ENTITYNUM_WORLD
		&& other->s.eType == ET_MOVER
		&& trace->plane.normal[2] > 0 )
	{//stick to it?
		self->s.groundEntityNum = other->s.number;
	}
	else if (other && other->s.number < ENTITYNUM_WORLD &&
		(other->client || !other->s.weapon))
	{ //hit another entity that is not stickable, "bounce" off
		self->target_ent = other;
		/*
		vec3_t vNor, tN;

		VectorCopy(trace->plane.normal, vNor);
		VectorNormalize(vNor);
		VectorNPos(self->s.pos.trDelta, tN);
		self->s.pos.trDelta[0] += vNor[0]*(tN[0]*(((float)Q_irand(1, 10))*0.1));
		self->s.pos.trDelta[1] += vNor[1]*(tN[1]*(((float)Q_irand(1, 10))*0.1));
		self->s.pos.trDelta[2] += vNor[1]*(tN[2]*(((float)Q_irand(1, 10))*0.1));

		vectoangles(vNor, self->s.angles);
		vectoangles(vNor, self->s.apos.trBase);
		self->touch = charge_stick;
		
		return;
		*/
	}
	else if (other && other->s.number < ENTITYNUM_WORLD)
	{ //hit an entity that we just want to explode on (probably another projectile or something)
		vec3_t v;

		self->touch = 0;
		self->think = 0;
		self->nextthink = 0;

		self->takedamage = qfalse;

		VectorClear(self->s.apos.trDelta);
		self->s.apos.trType = TR_STATIONARY;
	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_INCINERATOR_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_DIOXIS_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_ION_EXPLOSION_SPLASH );
	}
	else
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_DET_PACK_SPLASH );
	}

		VectorCopy(trace->plane.normal, v);
		VectorCopy(v, self->pos2);
		self->count = -1;
	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK2, self->r.currentOrigin, v);
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK3, self->r.currentOrigin, v);
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK4, self->r.currentOrigin, v);
	}
	else
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, v);
	}

		self->think = G_FreeEntity;
		self->nextthink = level.time;
		return;
	}

	//if we get here I guess we hit hte world so we can stick to it

	self->touch = 0;
	self->think = DetPackBlow;
	self->nextthink = level.time + 1800000;//make them last 30 mins?:eek:

	VectorClear(self->s.apos.trDelta);
	self->s.apos.trType = TR_STATIONARY;

	self->s.pos.trType = TR_STATIONARY;
	VectorCopy( self->r.currentOrigin, self->s.origin );
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	VectorClear( self->s.pos.trDelta );

	VectorClear( self->s.apos.trDelta );

	VectorNormalize(trace->plane.normal);

	vectoangles(trace->plane.normal, self->s.angles);
	VectorCopy(self->s.angles, self->r.currentAngles );
	VectorCopy(self->s.angles, self->s.apos.trBase);

	VectorCopy(trace->plane.normal, self->pos2);
	self->count = -1;

	G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/weapons/detpack/stick.wav"));
		
	tent = G_TempEntity( self->r.currentOrigin, EV_MISSILE_MISS );
	tent->s.weapon = 0;
	tent->parent = self;
	tent->r.ownerNum = self->s.number;

	//so that the owner can blow it up with projectiles
	self->r.svFlags |= SVF_OWNERNOTSHARED;
}

void DetPackBlow(gentity_t *self)
{
	vec3_t v;

	self->pain = 0;
	self->die = 0;
	self->takedamage = qfalse;

	if ( self->target_ent )
	{//we were attached to something, do *direct* damage to it!
	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_Damage( self->target_ent, self, &g_entities[self->r.ownerNum], v, self->r.currentOrigin, self->damage, 0, MOD_INCINERATOR_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_Damage( self->target_ent, self, &g_entities[self->r.ownerNum], v, self->r.currentOrigin, self->damage, 0, MOD_DIOXIS_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_Damage( self->target_ent, self, &g_entities[self->r.ownerNum], v, self->r.currentOrigin, self->damage, 0, MOD_ION_EXPLOSION_SPLASH );
	}
	else
	{
		G_Damage( self->target_ent, self, &g_entities[self->r.ownerNum], v, self->r.currentOrigin, self->damage, 0, MOD_DET_PACK_SPLASH );
	}

	}
	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_INCINERATOR_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_DIOXIS_EXPLOSION_SPLASH );
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_ION_EXPLOSION_SPLASH );
	}
	else
	{
		G_RadiusDamage( self->r.currentOrigin, self->parent, self->splashDamage, self->splashRadius, self, self, MOD_DET_PACK_SPLASH );
	}
	v[0] = 0;
	v[1] = 0;
	v[2] = 1;

	if (self->count == -1)
	{
		VectorCopy(self->pos2, v);
	}

	if (self->s.eFlags & EF_WP_OPTION_2)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK2, self->r.currentOrigin, v);
	}
	else if (self->s.eFlags & EF_WP_OPTION_3)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK3, self->r.currentOrigin, v);
	}
	else if (self->s.eFlags & EF_WP_OPTION_4)
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK4, self->r.currentOrigin, v);
	}
	else
	{
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, v);
	}

	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

void DetPackPain(gentity_t *self, gentity_t *attacker, int damage)
{
	self->think = DetPackBlow;
	self->nextthink = level.time + Q_irand(50, 100);
	self->takedamage = qfalse;
}

void DetPackDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	self->think = DetPackBlow;
	self->nextthink = level.time + Q_irand(50, 100);
	self->takedamage = qfalse;
}

void drop_charge (gentity_t *self, vec3_t start, vec3_t dir) 
{
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "detpack";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_RunObject;
	bolt->s.eType = ET_GENERAL;
	bolt->s.g2radius = 100;
	bolt->s.modelGhoul2 = 1;
	bolt->s.modelindex = G_ModelIndex("models/weapons2/detpack/det_pack_proj.glm");
	bolt->parent = self;
	bolt->r.ownerNum = self->s.number;
	if (self->client && self->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	bolt->damage = 600;	
	}
	else
	{
	bolt->damage = 300;	
	}
	bolt->splashDamage = 150;
	bolt->splashRadius = 512;
	

	
	bolt->methodOfDeath = MOD_DET_PACK_SPLASH;
	bolt->splashMethodOfDeath = MOD_DET_PACK_SPLASH;
	
	
	bolt->clipmask = MASK_SHOT;
	bolt->s.solid = 2;
	bolt->r.contents = MASK_SHOT;
	bolt->touch = charge_stick;

	bolt->physicsObject = qtrue;

	bolt->s.genericenemyindex = self->s.number+MAX_GENTITIES;
	//rww - so client prediction knows we own this and won't hit it

	VectorSet( bolt->r.mins, -2, -2, -2 );
	VectorSet( bolt->r.maxs, 2, 2, 2 );

	bolt->health = 1;
	bolt->takedamage = qtrue;
	bolt->pain = DetPackPain;
	bolt->die = DetPackDie;

	bolt->s.weapon = WP_DET_PACK;

	bolt->setTime = level.time;

	G_SetOrigin(bolt, start);
	bolt->s.pos.trType = TR_GRAVITY;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale(dir, 300, bolt->s.pos.trDelta );
	bolt->s.pos.trTime = level.time;

	bolt->s.apos.trType = TR_GRAVITY;
	bolt->s.apos.trTime = level.time;
	bolt->s.apos.trBase[YAW] = rand()%360;
	bolt->s.apos.trBase[PITCH] = rand()%360;
	bolt->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		bolt->s.apos.trBase[YAW] = -bolt->s.apos.trBase[YAW];
	}

	vectoangles(dir, bolt->s.angles);
	VectorCopy(bolt->s.angles, bolt->s.apos.trBase);
	VectorSet(bolt->s.apos.trDelta, 300, 0, 0 );
	bolt->s.apos.trTime = level.time;

	trap_LinkEntity(bolt);

	//[CoOp]
	//make some sight/sound events
	AddSoundEvent( NULL, bolt->r.currentOrigin, 128, AEL_MINOR, qtrue, qfalse );
	AddSightEvent( NULL, bolt->r.currentOrigin, 128, AEL_SUSPICIOUS, 10 );
	//[/CoOp]
}

void BlowDetpacks(gentity_t *ent)
{
	gentity_t *found = NULL;

	if ( ent->client->ps.hasDetPackPlanted )
	{
		while ( (found = G_Find( found, FOFS(classname), "detpack") ) != NULL )
		{//loop through all ents and blow the crap out of them!
			if ( found->parent == ent )
			{
				VectorCopy( found->r.currentOrigin, found->s.origin );
				found->think = DetPackBlow;
				found->nextthink = level.time + 100 + random() * 200;
				G_Sound( found, CHAN_BODY, G_SoundIndex("sound/weapons/detpack/warning.wav") );

				//[CoOp]
				// would be nice if this actually worked?
				AddSoundEvent( NULL, found->r.currentOrigin, found->splashRadius*2, AEL_DANGER, qfalse, qtrue );//FIXME: are we on ground or not?
				AddSightEvent( NULL, found->r.currentOrigin, found->splashRadius*2, AEL_DISCOVERED, 100 );
				//[/CoOp]
			}
		}
		ent->client->ps.hasDetPackPlanted = qfalse;
	}
}

qboolean CheatsOn(void) 
{
	if ( !g_cheats.integer )
	{
		return qfalse;
	}
	return qtrue;
}

void WP_DropDetPack( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*found = NULL;
	int			trapcount = 0;
	int			foundDetPacks[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			trapcount_org;
	int			lowestTimeStamp;
	int			removeMe;
	int			i;

	if ( !ent || !ent->client )
	{
		return;
	}

	//limit to 10 placed at any one time
	//see how many there are now
	while ( (found = G_Find( found, FOFS(classname), "detpack" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundDetPacks[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundDetPacks[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundDetPacks[i]];
			if ( found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundDetPacks[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				if (!CheatsOn())
				{ //Let them have unlimited if cheats are enabled
					G_FreeEntity( &g_entities[foundDetPacks[removeMe]] );
				}
			}
			foundDetPacks[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	if ( alt_fire  )
	{
		BlowDetpacks(ent);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, forward, vright, up );

		CalcMuzzlePoint( ent, forward, vright, up, muzzle );

		VectorNormalize( forward );
		VectorMA( muzzle, -4, forward, muzzle );
		drop_charge( ent, muzzle, forward );

		ent->client->ps.hasDetPackPlanted = qtrue;
	}
}
/*
======================================================================

DET PACK2

======================================================================
*/
void drop_charge2 (gentity_t *self, vec3_t start, vec3_t dir) 
{
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "detpack";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_RunObject;
	bolt->s.eType = ET_GENERAL;
	bolt->s.g2radius = 100;
	bolt->s.modelGhoul2 = 1;
	bolt->s.modelindex = G_ModelIndex("models/weapons2/detpack/det_pack_proj.glm");
	bolt->parent = self;
	bolt->r.ownerNum = self->s.number;
	if (self->client && self->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	bolt->damage = 600;	
	}
	else
	{
	bolt->damage = 300;	
	}
	bolt->splashDamage = 150;
	bolt->splashRadius = 512;
	

	
	bolt->s.eFlags |= EF_WP_OPTION_2;
	bolt->methodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
	bolt->splashMethodOfDeath = MOD_INCINERATOR_EXPLOSION_SPLASH;
	

	
	bolt->clipmask = MASK_SHOT;
	bolt->s.solid = 2;
	bolt->r.contents = MASK_SHOT;
	bolt->touch = charge_stick;

	bolt->physicsObject = qtrue;

	bolt->s.genericenemyindex = self->s.number+MAX_GENTITIES;
	//rww - so client prediction knows we own this and won't hit it

	VectorSet( bolt->r.mins, -2, -2, -2 );
	VectorSet( bolt->r.maxs, 2, 2, 2 );

	bolt->health = 1;
	bolt->takedamage = qtrue;
	bolt->pain = DetPackPain;
	bolt->die = DetPackDie;

	bolt->s.weapon = WP_DET_PACK;

	bolt->setTime = level.time;

	G_SetOrigin(bolt, start);
	bolt->s.pos.trType = TR_GRAVITY;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale(dir, 300, bolt->s.pos.trDelta );
	bolt->s.pos.trTime = level.time;

	bolt->s.apos.trType = TR_GRAVITY;
	bolt->s.apos.trTime = level.time;
	bolt->s.apos.trBase[YAW] = rand()%360;
	bolt->s.apos.trBase[PITCH] = rand()%360;
	bolt->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		bolt->s.apos.trBase[YAW] = -bolt->s.apos.trBase[YAW];
	}

	vectoangles(dir, bolt->s.angles);
	VectorCopy(bolt->s.angles, bolt->s.apos.trBase);
	VectorSet(bolt->s.apos.trDelta, 300, 0, 0 );
	bolt->s.apos.trTime = level.time;

	trap_LinkEntity(bolt);

	//[CoOp]
	//make some sight/sound events
	AddSoundEvent( NULL, bolt->r.currentOrigin, 128, AEL_MINOR, qtrue, qfalse );
	AddSightEvent( NULL, bolt->r.currentOrigin, 128, AEL_SUSPICIOUS, 10 );
	//[/CoOp]
}


void WP_DropDetPack2( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*found = NULL;
	int			trapcount = 0;
	int			foundDetPacks[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			trapcount_org;
	int			lowestTimeStamp;
	int			removeMe;
	int			i;

	if ( !ent || !ent->client )
	{
		return;
	}

	//limit to 10 placed at any one time
	//see how many there are now
	while ( (found = G_Find( found, FOFS(classname), "detpack" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundDetPacks[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundDetPacks[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundDetPacks[i]];
			if ( found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundDetPacks[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				if (!CheatsOn())
				{ //Let them have unlimited if cheats are enabled
					G_FreeEntity( &g_entities[foundDetPacks[removeMe]] );
				}
			}
			foundDetPacks[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	if ( alt_fire  )
	{
		BlowDetpacks(ent);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, forward, vright, up );

		CalcMuzzlePoint( ent, forward, vright, up, muzzle );

		VectorNormalize( forward );
		VectorMA( muzzle, -4, forward, muzzle );
		drop_charge2( ent, muzzle, forward );

		ent->client->ps.hasDetPackPlanted = qtrue;
	}
}
/*
======================================================================

DET PACK3

======================================================================
*/
void drop_charge3 (gentity_t *self, vec3_t start, vec3_t dir) 
{
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "detpack";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_RunObject;
	bolt->s.eType = ET_GENERAL;
	bolt->s.g2radius = 100;
	bolt->s.modelGhoul2 = 1;
	bolt->s.modelindex = G_ModelIndex("models/weapons2/detpack/det_pack_proj.glm");
	bolt->parent = self;
	bolt->r.ownerNum = self->s.number;
	if (self->client && self->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	bolt->damage = 600;	
	}
	else
	{
	bolt->damage = 300;	
	}
	bolt->splashDamage = 150;
	bolt->splashRadius = 512;
	


	
	bolt->s.eFlags |= EF_WP_OPTION_3;
	bolt->methodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
	bolt->splashMethodOfDeath = MOD_DIOXIS_EXPLOSION_SPLASH;
	

	
	bolt->clipmask = MASK_SHOT;
	bolt->s.solid = 2;
	bolt->r.contents = MASK_SHOT;
	bolt->touch = charge_stick;

	bolt->physicsObject = qtrue;

	bolt->s.genericenemyindex = self->s.number+MAX_GENTITIES;
	//rww - so client prediction knows we own this and won't hit it

	VectorSet( bolt->r.mins, -2, -2, -2 );
	VectorSet( bolt->r.maxs, 2, 2, 2 );

	bolt->health = 1;
	bolt->takedamage = qtrue;
	bolt->pain = DetPackPain;
	bolt->die = DetPackDie;

	bolt->s.weapon = WP_DET_PACK;

	bolt->setTime = level.time;

	G_SetOrigin(bolt, start);
	bolt->s.pos.trType = TR_GRAVITY;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale(dir, 300, bolt->s.pos.trDelta );
	bolt->s.pos.trTime = level.time;

	bolt->s.apos.trType = TR_GRAVITY;
	bolt->s.apos.trTime = level.time;
	bolt->s.apos.trBase[YAW] = rand()%360;
	bolt->s.apos.trBase[PITCH] = rand()%360;
	bolt->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		bolt->s.apos.trBase[YAW] = -bolt->s.apos.trBase[YAW];
	}

	vectoangles(dir, bolt->s.angles);
	VectorCopy(bolt->s.angles, bolt->s.apos.trBase);
	VectorSet(bolt->s.apos.trDelta, 300, 0, 0 );
	bolt->s.apos.trTime = level.time;

	trap_LinkEntity(bolt);

	//[CoOp]
	//make some sight/sound events
	AddSoundEvent( NULL, bolt->r.currentOrigin, 128, AEL_MINOR, qtrue, qfalse );
	AddSightEvent( NULL, bolt->r.currentOrigin, 128, AEL_SUSPICIOUS, 10 );
	//[/CoOp]
}


void WP_DropDetPack3( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*found = NULL;
	int			trapcount = 0;
	int			foundDetPacks[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			trapcount_org;
	int			lowestTimeStamp;
	int			removeMe;
	int			i;

	if ( !ent || !ent->client )
	{
		return;
	}

	//limit to 10 placed at any one time
	//see how many there are now
	while ( (found = G_Find( found, FOFS(classname), "detpack" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundDetPacks[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundDetPacks[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundDetPacks[i]];
			if ( found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundDetPacks[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				if (!CheatsOn())
				{ //Let them have unlimited if cheats are enabled
					G_FreeEntity( &g_entities[foundDetPacks[removeMe]] );
				}
			}
			foundDetPacks[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	if ( alt_fire  )
	{
		BlowDetpacks(ent);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, forward, vright, up );

		CalcMuzzlePoint( ent, forward, vright, up, muzzle );

		VectorNormalize( forward );
		VectorMA( muzzle, -4, forward, muzzle );
		drop_charge3( ent, muzzle, forward );

		ent->client->ps.hasDetPackPlanted = qtrue;
	}
}
/*
======================================================================

DET PACK4

======================================================================
*/
void drop_charge4 (gentity_t *self, vec3_t start, vec3_t dir) 
{
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "detpack";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_RunObject;
	bolt->s.eType = ET_GENERAL;
	bolt->s.g2radius = 100;
	bolt->s.modelGhoul2 = 1;
	bolt->s.modelindex = G_ModelIndex("models/weapons2/detpack/det_pack_proj.glm");
	bolt->parent = self;
	bolt->r.ownerNum = self->s.number;
	if (self->client && self->client->skillLevel[SK_DETPACK] == FORCE_LEVEL_3)
	{
	bolt->damage = 600;	
	}
	else
	{
	bolt->damage = 300;	
	}
	bolt->splashDamage = 150;
	bolt->splashRadius = 512;
	

	
	bolt->s.eFlags |= EF_WP_OPTION_4;
	bolt->methodOfDeath = MOD_ION_EXPLOSION;
	bolt->splashMethodOfDeath = MOD_ION_EXPLOSION_SPLASH;
	

	
	bolt->clipmask = MASK_SHOT;
	bolt->s.solid = 2;
	bolt->r.contents = MASK_SHOT;
	bolt->touch = charge_stick;

	bolt->physicsObject = qtrue;

	bolt->s.genericenemyindex = self->s.number+MAX_GENTITIES;
	//rww - so client prediction knows we own this and won't hit it

	VectorSet( bolt->r.mins, -2, -2, -2 );
	VectorSet( bolt->r.maxs, 2, 2, 2 );

	bolt->health = 1;
	bolt->takedamage = qtrue;
	bolt->pain = DetPackPain;
	bolt->die = DetPackDie;

	bolt->s.weapon = WP_DET_PACK;

	bolt->setTime = level.time;

	G_SetOrigin(bolt, start);
	bolt->s.pos.trType = TR_GRAVITY;
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale(dir, 300, bolt->s.pos.trDelta );
	bolt->s.pos.trTime = level.time;

	bolt->s.apos.trType = TR_GRAVITY;
	bolt->s.apos.trTime = level.time;
	bolt->s.apos.trBase[YAW] = rand()%360;
	bolt->s.apos.trBase[PITCH] = rand()%360;
	bolt->s.apos.trBase[ROLL] = rand()%360;

	if (rand()%10 < 5)
	{
		bolt->s.apos.trBase[YAW] = -bolt->s.apos.trBase[YAW];
	}

	vectoangles(dir, bolt->s.angles);
	VectorCopy(bolt->s.angles, bolt->s.apos.trBase);
	VectorSet(bolt->s.apos.trDelta, 300, 0, 0 );
	bolt->s.apos.trTime = level.time;

	trap_LinkEntity(bolt);

	//[CoOp]
	//make some sight/sound events
	AddSoundEvent( NULL, bolt->r.currentOrigin, 128, AEL_MINOR, qtrue, qfalse );
	AddSightEvent( NULL, bolt->r.currentOrigin, 128, AEL_SUSPICIOUS, 10 );
	//[/CoOp]
}


void WP_DropDetPack4( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*found = NULL;
	int			trapcount = 0;
	int			foundDetPacks[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			trapcount_org;
	int			lowestTimeStamp;
	int			removeMe;
	int			i;

	if ( !ent || !ent->client )
	{
		return;
	}

	//limit to 10 placed at any one time
	//see how many there are now
	while ( (found = G_Find( found, FOFS(classname), "detpack" )) != NULL )
	{
		if ( found->parent != ent )
		{
			continue;
		}
		foundDetPacks[trapcount++] = found->s.number;
	}
	//now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;
	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundDetPacks[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundDetPacks[i]];
			if ( found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundDetPacks[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				if (!CheatsOn())
				{ //Let them have unlimited if cheats are enabled
					G_FreeEntity( &g_entities[foundDetPacks[removeMe]] );
				}
			}
			foundDetPacks[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}

	if ( alt_fire  )
	{
		BlowDetpacks(ent);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, forward, vright, up );

		CalcMuzzlePoint( ent, forward, vright, up, muzzle );

		VectorNormalize( forward );
		VectorMA( muzzle, -4, forward, muzzle );
		drop_charge4( ent, muzzle, forward );

		ent->client->ps.hasDetPackPlanted = qtrue;
	}
}
/*
======================================================================

CONCUSSION

======================================================================
*/
//#pragma warning(disable : 4701) //local variable may be used without having been initialized
static void WP_FireConcussionAlt( gentity_t *ent )
{//a rail-gun-like beam
	int			damage = CONC_ALT_DAMAGE, skip, traces = DISRUPTOR_ALT_TRACES;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		shotRange = 8192.0f;
	qboolean	hitDodged = qfalse;
	vec3_t shot_mins, shot_maxs;
	int			i;
if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
{
	damage *= 2.0;
}
	//Shove us backwards for half a second
	VectorMA( ent->client->ps.velocity, -200, forward, ent->client->ps.velocity );
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
	if ( (ent->client->ps.pm_flags&PMF_DUCKED) )
	{//hunkered down
		ent->client->ps.pm_time = 100;
	}
	else
	{
		ent->client->ps.pm_time = 250;
	}
//	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION;
	//FIXME: only if on ground?  So no "rocket jump"?  Or: (see next FIXME)
	//FIXME: instead, set a forced ucmd backmove instead of this sliding

	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	skip = ent->s.number;

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}
	
	//Make it a little easier to hit guys at long range
	VectorSet( shot_mins, -1, -1, -1 );
	VectorSet( shot_maxs, 1, 1, 1 );

	for ( i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forward, end );

		//NOTE: if you want to be able to hit guys in emplaced guns, use "G2_COLLIDE, 10" instead of "G2_RETURNONHIT, 0"
		//alternately, if you end up hitting an emplaced_gun that has a sitter, just redo this one trace with the "G2_COLLIDE, 10" to see if we it the sitter
		//gi.trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );
		if (d_projectileGhoul2Collision.integer)
		{
			trap_G2Trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );
		}
		else
		{
			trap_Trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT );
		}

		traceEnt = &g_entities[tr.entityNum];

		if (d_projectileGhoul2Collision.integer && traceEnt->inuse && traceEnt->client)
		{ //g2 collision checks -rww
			if (traceEnt->inuse && traceEnt->client && traceEnt->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				traceEnt->client->g2LastSurfaceHit = tr.surfaceFlags;
				traceEnt->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				//BUGFIX12RAFIXME - ugh, can't seem to get the model index on the 
				//trap_G2Traces.  These probably need to be replaced with the more
				//indepth G2traces.  For now, just assume that the player model was hit.
				traceEnt->client->g2LastSurfaceModel = G2MODEL_PLAYER;
				//[/BugFix12]
			}

			if (traceEnt->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if ( tr.entityNum == ent->s.number )
		{
			// should never happen, but basically we don't want to consider a hit to ourselves?
			// Get ready for an attempt to trace through another person
			VectorCopy( tr.endpos, muzzle2 );
			VectorCopy( tr.endpos, start );
			skip = tr.entityNum;
#ifdef _DEBUG
			Com_Printf( "BAD! Concussion gun shot somehow traced back and hit the owner!\n" );			
#endif
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		//NOTE: let's just draw one beam at the end
		//tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
		//tent->svFlags |= SVF_BROADCAST;

		//VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		if ( traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC 
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) 
					|| !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
					|| traceEnt->s.eType == ET_MOVER )
				{
					qboolean noKnockBack;

					// Create a simple impact type mark that doesn't last long in the world
					//G_PlayEffectID( G_EffectIndex( "concussion/alt_hit" ), tr.endpos, tr.plane.normal );
					//no no no

					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
					{//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->accuracy_hits++;
					} 

					noKnockBack = (traceEnt->flags&FL_NO_KNOCKBACK);//will be set if they die, I want to know if it was on *before* they died
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
					{//hehe
						G_Damage( traceEnt, ent, ent, forward, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT );
						break;
					}
					G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT );

					//do knockback and knockdown manually
					if ( traceEnt->client )
					{//only if we hit a client
						vec3_t pushDir;
						VectorCopy( forward, pushDir );
						if ( pushDir[2] < 0.2f )
						{
							pushDir[2] = 0.2f;
						}//hmm, re-normalize?  nah...
						/*
						if ( !noKnockBack )
						{//knock-backable
							G_Throw( traceEnt, pushDir, 200 );
						}
						*/
						if ( traceEnt->health > 0 )
						{//alive
							//if ( G_HasKnockdownAnims( traceEnt ) )
							if (!noKnockBack && !traceEnt->localAnimIndex && traceEnt->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN &&
								BG_KnockDownable(&traceEnt->client->ps)) //just check for humanoids..
							{//knock-downable
								//G_Knockdown( traceEnt, ent, pushDir, 400, qtrue );
								vec3_t plPDif;
								float pStr;

								//cap it and stuff, base the strength and whether or not we can knockdown on the distance
								//from the shooter to the target
								VectorSubtract(traceEnt->client->ps.origin, ent->client->ps.origin, plPDif);
								pStr = 500.0f-VectorLength(plPDif);
								if (pStr < 150.0f)
								{
									pStr = 150.0f;
								}
								if (pStr > 200.0f)
								{
									traceEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
									traceEnt->client->ps.forceHandExtendTime = level.time + 1100;
									traceEnt->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
								}
								traceEnt->client->ps.otherKiller = ent->s.number;
								traceEnt->client->ps.otherKillerTime = level.time + 5000;
								traceEnt->client->ps.otherKillerDebounceTime = level.time + 100;
								//[Asteroids]
								traceEnt->client->otherKillerMOD = MOD_UNKNOWN;
								traceEnt->client->otherKillerVehWeapon = 0;
								traceEnt->client->otherKillerWeaponType = WP_NONE;
								//[/Asteroids]

								traceEnt->client->ps.velocity[0] += pushDir[0]*pStr;
								traceEnt->client->ps.velocity[1] += pushDir[1]*pStr;
								traceEnt->client->ps.velocity[2] = pStr;
							}
						}
					}

					if ( traceEnt->s.eType == ET_MOVER )
					{//stop the traces on any mover
						break;
					}
				}
				else 
				{
					 // we only make this mark on things that can't break or move
				//	tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
				//	tent->s.eventParm = DirToByte(tr.plane.normal);
				//	tent->s.eFlags |= EF_ALT_FIRING;

					//tent->svFlags |= SVF_BROADCAST;
					//eh? why broadcast?
				//	VectorCopy( tr.plane.normal, tent->pos1 );

					//mmm..no..don't do this more than once for no reason whatsoever.
					break; // hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle2 );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
		hitDodged = qfalse;
	}
	//just draw one beam all the way to the end
//	tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
//	tent->svFlags |= SVF_BROADCAST;
	//again, why broadcast?

//	tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
//	tent->s.eventParm = DirToByte(tr.plane.normal);
//	tent->s.eFlags |= EF_ALT_FIRING;
//	VectorCopy( muzzle, tent->s.origin2 );

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, muzzle, dir );

//	shotDist = VectorNormalize( dir );

	//let's pack all this junk into a single tempent, and send it off.
	tent = G_TempEntity(tr.endpos, EV_CONC_ALT_IMPACT);
	tent->s.eventParm = DirToByte(tr.plane.normal);
	tent->s.owner = ent->s.number;
	VectorCopy(dir, tent->s.angles);
	VectorCopy(muzzle, tent->s.origin2);
	VectorCopy(forward, tent->s.angles2);

#if 0 //yuck
	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( muzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
		//FIXME: creates *way* too many effects, make it one effect somehow?
		G_PlayEffectID( G_EffectIndex( "concussion/alt_ring" ), spot, actualAngles );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, forward, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );

	G_PlayEffectID( G_EffectIndex( "concussion/altmuzzle_flash" ), muzzle, forward );
#endif
}
//#pragma warning(default : 4701) //local variable may be used without having been initialized

static void WP_FireConcussionMain( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t	start;
	int		damage	= CONC_DAMAGE;
	float	vel = CONC_VELOCITY;
	gentity_t *missile;
if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
{
	damage *= 2.0;
}
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

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	missile = CreateMissile( start, forward, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;

	missile->mass = 10;

	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = CONC_SPLASH_DAMAGE;
	missile->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

}
//----------------------------------------------
static void WP_FireConcussion( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireConcussionAlt(ent);
	else
		WP_FireConcussionMain(ent);
}

/*
======================================================================

CONCUSSION2

======================================================================
*/
//#pragma warning(disable : 4701) //local variable may be used without having been initialized
static void WP_FireConcussion2Alt( gentity_t *ent )
{//a rail-gun-like beam
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = CONC_ALT_DAMAGE*2/75;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";

		missile->s.weapon = WP_BRYAR_PISTOL;


		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
//#pragma warning(default : 4701) //local variable may be used without having been initialized

static void WP_FireConcussion2Main( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = CONC_DAMAGE*2/75;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";

		missile->s.weapon = WP_BRYAR_PISTOL;


		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
//----------------------------------------------
static void WP_FireConcussion2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireConcussion2Alt(ent);
	else
		WP_FireConcussion2Main(ent);
}

/*
======================================================================

CONCUSSION3

======================================================================
*/
//#pragma warning(disable : 4701) //local variable may be used without having been initialized
static void WP_FireConcussion3Alt( gentity_t *ent )
{//a rail-gun-like beam
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = CONC_ALT_DAMAGE*1/50;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";

		missile->s.weapon = WP_REPEATER;
		missile->s.eFlags |= EF_WP_OPTION_3;

		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
//#pragma warning(default : 4701) //local variable may be used without having been initialized

static void WP_FireConcussion3Main( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t		fwd, angs;
	gentity_t	*missile;
	int i;
	int shots;
	int damage = CONC_DAMAGE*1/50;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
		shots = REPEATER_SHOTS;
			
	for (i = 0; i < shots; i++ )
	{
		vectoangles( forward, angs );

		if (i != 0)
		{ //do nothing on the first shot, it will hit the crosshairs
			angs[PITCH] += crandom() * REPEATER_SPREAD;
			angs[YAW]	+= crandom() * REPEATER_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( muzzle, fwd, REPEATER_VELOCITY, 10000, ent, qfalse);

		missile->classname = "repeater_proj";

		missile->s.weapon = WP_REPEATER;
		missile->s.eFlags |= EF_WP_OPTION_3;

		VectorSet( missile->r.maxs, REPEATER_SIZE, REPEATER_SIZE, REPEATER_SIZE );
		VectorScale( missile->r.maxs, -1, missile->r.mins );

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_REPEATER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(5,8);
									  
													 

		missile->flags |= FL_BOUNCE_SHRAPNEL;
	}
}
//----------------------------------------------
static void WP_FireConcussion3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireConcussion3Alt(ent);
	else
		WP_FireConcussion3Main(ent);
}

/*
======================================================================

CONCUSSION4

======================================================================
*/
//#pragma warning(disable : 4701) //local variable may be used without having been initialized
static void WP_FireConcussion4Alt( gentity_t *ent )
{//a rail-gun-like beam
	vec3_t	start;
	int		damage	= CONC_ALT_DAMAGE*4/3;
	float	vel = CONC_VELOCITY;
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

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	missile = CreateMissile( start, forward, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;	
	missile->s.eFlags |= EF_WP_OPTION_4;

	missile->mass = 10;

	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = CONC_SPLASH_DAMAGE;
	missile->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
	gentity_t *missile2;

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

	VectorCopy( muzzle2, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	missile2 = CreateMissile( start, forward, vel, 10000, ent, qfalse );

	missile2->classname = "conc_proj";

	missile2->s.weapon = WP_CONCUSSION;	
	missile2->s.eFlags |= EF_WP_OPTION_4;
	missile2->mass = 10;

	// Make it easier to hit things
	VectorSet( missile2->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile2->r.maxs, -1, missile2->r.mins );

	missile2->damage = damage;
	missile2->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile2->methodOfDeath = MOD_CONC;
	missile2->splashMethodOfDeath = MOD_CONC;

	missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile2->splashDamage = CONC_SPLASH_DAMAGE;
	missile2->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile2->bounceCount = 0;
	}
}
//#pragma warning(default : 4701) //local variable may be used without having been initialized

static void WP_FireConcussion4Main( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t	start;
	int		damage	= CONC_DAMAGE*4/3;
	float	vel = CONC_VELOCITY;
	gentity_t *missile;
	if(ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
	{
		damage *= 2.0;
	}	
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

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	missile = CreateMissile( start, forward, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;	
	missile->s.eFlags |= EF_WP_OPTION_4;

	missile->mass = 10;

	// Make it easier to hit things
	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = CONC_SPLASH_DAMAGE;
	missile->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

}
//----------------------------------------------
static void WP_FireConcussion4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireConcussion4Alt(ent);
	else
		WP_FireConcussion4Main(ent);
}




/*
======================================================================

BRYAR OLD

======================================================================
*/

static void WP_FireBryarOldMain(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_OLD_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
		missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	}
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;

	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;


	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_OLD;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarOldAlt(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE;
	int count;

	gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qtrue );
	gentity_t   *missile2;
	float boxSize = 0;

	//[DualPistols]
	if ((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2 = CreateMissile(muzzle2, forward, BRYAR_OLD_VEL, 10000, ent, qtrue);

	}
	//[/DualPistols]

	//if(ent->client->skillLevel[SK_PISTOL] != FORCE_LEVEL_3)
	//{
		//return;
	//}


	//[/DualPistols]
	//[/DualPistols]

//	else if(ent->client->skillLevel[SK_OLD] != FORCE_LEVEL_3)
	//{
		//return;
	//}

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;


	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	missile2->classname = "bryar_proj";
	missile2->s.weapon = WP_BRYAR_OLD;
	
	}
	//[/DualPistols]
	count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	//[BryarSecondary]
	else if ( count > BRYAR_MAX_CHARGE )
	{
		count = BRYAR_MAX_CHARGE;
	}

	damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count/BRYAR_MAX_CHARGE*(BRYAR_PISTOL_ALT_DPMAXDAMAGE-BRYAR_PISTOL_ALT_DPDAMAGE);

	//[/BryarSecondary]

	missile->s.generic1 = count; // The missile will then render according to the charge level.

	boxSize = BRYAR_OLD_ALT_SIZE*(count*0.5);

	VectorSet( missile->r.maxs, boxSize, boxSize, boxSize );
	VectorSet( missile->r.mins, -boxSize, -boxSize, -boxSize );

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->s.generic1 = count;
		VectorSet( missile2->r.maxs, boxSize, boxSize, boxSize );
		VectorSet( missile2->r.mins, -boxSize, -boxSize, -boxSize );
	}
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarOld( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarOldAlt(ent);
	else
		WP_FireBryarOldMain(ent);
}

/*
======================================================================

BRYAR OLD2

======================================================================
*/

static void WP_FireBryarOld2Main(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*6/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_OLD_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
		missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	}
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;

	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;
	missile->s.eFlags |= EF_WP_OPTION_2;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_OLD;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarOld2Alt(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*5/7;
	int count;

	gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qtrue );
	gentity_t   *missile2;
	float boxSize = 0;

	//[DualPistols]
	if ((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2 = CreateMissile(muzzle2, forward, BRYAR_OLD_VEL, 10000, ent, qtrue);

	}
	//[/DualPistols]

	//if(ent->client->skillLevel[SK_PISTOL] != FORCE_LEVEL_3)
	//{
		//return;
	//}


	//[/DualPistols]
	//[/DualPistols]

//	else if(ent->client->skillLevel[SK_OLD] != FORCE_LEVEL_3)
	//{
		//return;
	//}

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;	
	missile->s.eFlags |= EF_WP_OPTION_2;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	missile2->classname = "bryar_proj";
	missile2->s.weapon = WP_BRYAR_OLD;	
	missile2->s.eFlags |= EF_WP_OPTION_2;
	}
	//[/DualPistols]
	count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	//[BryarSecondary]
	else if ( count > BRYAR_MAX_CHARGE )
	{
		count = BRYAR_MAX_CHARGE;
	}

	damage = BRYAR_PISTOL_ALT_DPDAMAGE + (float)count/BRYAR_MAX_CHARGE*(BRYAR_PISTOL_ALT_DPMAXDAMAGE-BRYAR_PISTOL_ALT_DPDAMAGE);

	//[/BryarSecondary]

	missile->s.generic1 = count; // The missile will then render according to the charge level.

	boxSize = BRYAR_OLD_ALT_SIZE*(count*0.5);

	VectorSet( missile->r.maxs, boxSize, boxSize, boxSize );
	VectorSet( missile->r.mins, -boxSize, -boxSize, -boxSize );

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->s.generic1 = count;
		VectorSet( missile2->r.maxs, boxSize, boxSize, boxSize );
		VectorSet( missile2->r.mins, -boxSize, -boxSize, -boxSize );
	}
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL_ALT;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarOld2( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarOld2Alt(ent);
	else
		WP_FireBryarOld2Main(ent);
}

/*
======================================================================

BRYAR OLD3

======================================================================
*/

static void WP_FireBryarOld3Main(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*4/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_OLD_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
		missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	}
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;

	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;
	missile->s.eFlags |= EF_WP_OPTION_3;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarOld3Alt(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*6/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_OLD_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
		missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	}
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;

	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]
	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;



	VectorSet( missile->r.maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//----------------------------------------------
static void WP_FireBryarOld3( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarOld3Alt(ent);
	else
		WP_FireBryarOld3Main(ent);
}

/*
======================================================================

BRYAR OLD4

======================================================================
*/

static void WP_FireBryarOld4Main(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*2/7;
	gentity_t	*missile;

	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		if(ent->client->leftPistol)
			missile=CreateMissile(muzzle2,forward,BRYAR_OLD_VEL,10000,ent,qfalse);
		else
			missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
		
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
		missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	}
	//gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_PISTOL_VEL, 10000, ent, altFire );
	//gentity_t   *missile2;

	//[DualPistols]
	//if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	//	missile2 = CreateMissile(muzzle2,forward,BRYAR_PISTOL_VEL,10000,ent,altFire);
	//[/DualPistols]

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_OLD;
	missile->s.eFlags |= EF_WP_OPTION_4;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "bryar_proj";
		missile2->s.weapon = WP_BRYAR_PISTOL;
	}*/
	//[/DualPistols]

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BRYAR_PISTOL;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//[DualPistols]
	/*
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_BRYAR_PISTOL;
		missile2->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile2->bounceCount = 8;
	}*/
	//[/DualPistols]
}

static void WP_FireBryarOld4Alt(gentity_t*ent)
{
	int damage = BRYAR_OLD_DAMAGE*45/7;
	int count;

	gentity_t	*missile = CreateMissile( muzzle, forward, BRYAR_OLD_VEL, 10000, ent, qfalse );
	gentity_t   *missile2;
	float boxSize = 0;

	//[DualPistols]
	if ((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2 = CreateMissile(muzzle2, forward, BRYAR_OLD_VEL, 10000, ent, qfalse);
	}
	//[/DualPistols]

	//if(ent->client->skillLevel[SK_PISTOL] != FORCE_LEVEL_3)
	//{
		//return;
	//}

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->classname = "rocket_proj";
		missile2->s.weapon = WP_ROCKET_LAUNCHER;
	}
	//[/DualPistols]
	//[/DualPistols]
//[/DualPistols]


	//[/BryarSecondary]



	VectorSet( missile->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	VectorSet( missile2->r.maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile2->r.maxs, -1, missile2->r.mins );
	}
	//[/DualPistols]
//	else if(ent->client->skillLevel[SK_OLD] != FORCE_LEVEL_3)
	//{
		//return;
	//}




	//[DualPistols]

	//[/DualPistols]

	missile->damage = damage;
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
	
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;
	// we don't want it to bounce forever
	missile->bounceCount = 0;

	//[DualPistols]
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
		missile2->damage = damage;
		missile2->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile2->methodOfDeath = MOD_ROCKET;
		missile2->splashMethodOfDeath = MOD_ROCKET_SPLASH;
		missile2->clipmask = MASK_SHOT;
//===testing being able to shoot rockets out of the air==================================
		missile2->health = 10;
		missile2->takedamage = qtrue;
		missile2->r.contents = MASK_SHOT;
		missile2->die = RocketDie;
//===testing being able to shoot rockets out of the air==================================
	
		missile2->splashDamage = ROCKET_SPLASH_DAMAGE;
		missile2->splashRadius = ROCKET_SPLASH_RADIUS;
	// we don't want it to bounce forever
		missile2->bounceCount = 0;
	}
	//[/DualPistols]
}

//----------------------------------------------
static void WP_FireBryarOld4( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	if(altFire)
		WP_FireBryarOld4Alt(ent);
	else
		WP_FireBryarOld4Main(ent);
}




//---------------------------------------------------------
// FireStunBaton
//---------------------------------------------------------
void WP_FireStunBaton( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	vec3_t		muzzleStun;
	if((ent->client->ps.eFlags & EF_DUAL_WEAPONS))
	{
	if(ent->client->leftPistol)
	{
	if (!ent->client)
	{
		VectorCopy(ent->r.currentOrigin, muzzleStun);
		muzzleStun[2] += 8;
	}
	else
	{
		VectorCopy(ent->client->ps.origin, muzzleStun);
		muzzleStun[2] += ent->client->ps.viewheight-6;
	}

	VectorMA(muzzleStun, 20.0f, forward, muzzleStun);
	VectorMA(muzzleStun, -4.0f, vright, muzzleStun);

	VectorMA( muzzleStun, STUN_BATON_RANGE, forward, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	trap_Trace ( &tr, muzzleStun, mins, maxs, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if (tr_ent && tr_ent->takedamage && tr_ent->client)
	{ //see if either party is involved in a duel
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
	}

	if ( tr_ent && tr_ent->takedamage )
	{
		G_PlayEffect( EFFECT_STUNHIT, tr.endpos, tr.plane.normal );

		G_Sound( tr_ent, CHAN_WEAPON, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, forward, tr.endpos, STUN_BATON_DAMAGE, (DAMAGE_NO_KNOCKBACK|DAMAGE_HALF_ABSORB), MOD_STUN_BATON );

		if (tr_ent->client)
		{ //if it's a player then use the shock effect
			if ( tr_ent->client->NPC_class == CLASS_VEHICLE )
			{//not on vehicles
				if ( !tr_ent->m_pVehicle
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL 
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_FLIER )
				{//can zap animals
					tr_ent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
				}
			}
			else
			{
				tr_ent->client->ps.electrifyTime = level.time + 700;
			}
		}
	}
	}
		else
		{
	if (!ent->client)
	{
		VectorCopy(ent->r.currentOrigin, muzzleStun);
		muzzleStun[2] += 8;
	}
	else
	{
		VectorCopy(ent->client->ps.origin, muzzleStun);
		muzzleStun[2] += ent->client->ps.viewheight-6;
	}

	VectorMA(muzzleStun, 20.0f, forward, muzzleStun);
	VectorMA(muzzleStun, 4.0f, vright, muzzleStun);

	VectorMA( muzzleStun, STUN_BATON_RANGE, forward, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	trap_Trace ( &tr, muzzleStun, mins, maxs, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if (tr_ent && tr_ent->takedamage && tr_ent->client)
	{ //see if either party is involved in a duel
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
	}

	if ( tr_ent && tr_ent->takedamage )
	{
		G_PlayEffect( EFFECT_STUNHIT, tr.endpos, tr.plane.normal );

		G_Sound( tr_ent, CHAN_WEAPON, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, forward, tr.endpos, STUN_BATON_DAMAGE, (DAMAGE_NO_KNOCKBACK|DAMAGE_HALF_ABSORB), MOD_STUN_BATON );

		if (tr_ent->client)
		{ //if it's a player then use the shock effect
			if ( tr_ent->client->NPC_class == CLASS_VEHICLE )
			{//not on vehicles
				if ( !tr_ent->m_pVehicle
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL 
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_FLIER )
				{//can zap animals
					tr_ent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
				}
			}
			else
			{
				tr_ent->client->ps.electrifyTime = level.time + 700;
			}
		}
	}
		}
		ent->client->leftPistol = !ent->client->leftPistol;
	}
	else
	{
	if (!ent->client)
	{
		VectorCopy(ent->r.currentOrigin, muzzleStun);
		muzzleStun[2] += 8;
	}
	else
	{
		VectorCopy(ent->client->ps.origin, muzzleStun);
		muzzleStun[2] += ent->client->ps.viewheight-6;
	}

	VectorMA(muzzleStun, 20.0f, forward, muzzleStun);
	VectorMA(muzzleStun, 4.0f, vright, muzzleStun);

	VectorMA( muzzleStun, STUN_BATON_RANGE, forward, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	trap_Trace ( &tr, muzzleStun, mins, maxs, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if (tr_ent && tr_ent->takedamage && tr_ent->client)
	{ //see if either party is involved in a duel
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
	}

	if ( tr_ent && tr_ent->takedamage )
	{
		G_PlayEffect( EFFECT_STUNHIT, tr.endpos, tr.plane.normal );

		G_Sound( tr_ent, CHAN_WEAPON, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, forward, tr.endpos, STUN_BATON_DAMAGE, (DAMAGE_NO_KNOCKBACK|DAMAGE_HALF_ABSORB), MOD_STUN_BATON );

		if (tr_ent->client)
		{ //if it's a player then use the shock effect
			if ( tr_ent->client->NPC_class == CLASS_VEHICLE )
			{//not on vehicles
				if ( !tr_ent->m_pVehicle
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL 
					|| tr_ent->m_pVehicle->m_pVehicleInfo->type == VH_FLIER )
				{//can zap animals
					tr_ent->client->ps.electrifyTime = level.time + Q_irand( 3000, 4000 );
				}
			}
			else
			{
				tr_ent->client->ps.electrifyTime = level.time + 700;
			}
		}
	}
	}
	



}


//---------------------------------------------------------
// FireMelee
//---------------------------------------------------------
void WP_FireMelee( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	vec3_t		muzzlePunch;

	if (ent->client && ent->client->ps.torsoAnim == BOTH_MELEE2)
	{ //right
		if (ent->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM))
		{
			return;
		}
	}
	else
	{ //left
		if (ent->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM))
		{
			return;
		}
	}

	if (!ent->client)
	{
		VectorCopy(ent->r.currentOrigin, muzzlePunch);
		muzzlePunch[2] += 8;
	}
	else
	{
		VectorCopy(ent->client->ps.origin, muzzlePunch);
		muzzlePunch[2] += ent->client->ps.viewheight-6;
	}

	VectorMA(muzzlePunch, 20.0f, forward, muzzlePunch);
	VectorMA(muzzlePunch, 4.0f, vright, muzzlePunch);

	VectorMA( muzzlePunch, MELEE_RANGE, forward, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	trap_Trace ( &tr, muzzlePunch, mins, maxs, end, ent->s.number, MASK_SHOT );

	if (tr.entityNum != ENTITYNUM_NONE)
	{ //hit something
		tr_ent = &g_entities[tr.entityNum];

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
				dmg *= 10;
			}

			G_Damage( tr_ent, ent, ent, forward, tr.endpos, dmg, DAMAGE_NO_ARMOR, MOD_MELEE );
		}
	}
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir);
void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
		vec3_t forward, right, up;

		AngleVectors (ent->client->ps.viewangles, forward, right, up);
		CalcMuzzlePoint ( ent, forward, right, up, muzzle );
		if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);
		ent->client->fireHeld = qtrue;
}

void Weapon_HookFree (gentity_t *ent)
{
	ent->parent->client->fireHeld = qfalse;
	ent->parent->client->hookhasbeenfired = qfalse;
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}

void Weapon_HookThink (gentity_t *ent)
{
		if (ent->enemy) {
			vec3_t v, oldorigin;

			VectorCopy(ent->r.currentOrigin, oldorigin);
			v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
			v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
			v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
			SnapVectorTowards( v, oldorigin );	// save net bandwidth

			G_SetOrigin( ent, v );
			ent->nextthink = level.time + 50; //lmo continue to think if attached to an enemy!
		}

		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.lastHitLoc);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}


//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if (!attacker)
	{
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
rwwFIXMEFIXME: Since ghoul2 models are on server and properly updated now,
it may be reasonable to base muzzle point off actual weapon bolt point.
The down side would be that it does not necessarily look alright from a
first person perspective.
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) 
{
	int weapontype;
	vec3_t muzzleOffPoint;

	weapontype = ent->s.weapon;
	VectorCopy( ent->s.pos.trBase, muzzlePoint );

	VectorCopy(WP_MuzzlePoint[weapontype], muzzleOffPoint);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{	// Use the table to generate the muzzlepoint;
		{	// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzleOffPoint[0], forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
			muzzlePoint[2] += ent->client->ps.viewheight + muzzleOffPoint[2];
		}
	}

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

//[DualPistols]
void CalcMuzzlePoint2 ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) 
{
	int weapontype;
	vec3_t muzzleOffPoint;

	weapontype = ent->s.weapon;
	VectorCopy( ent->s.pos.trBase, muzzlePoint );

	VectorCopy(WP_MuzzlePoint2[weapontype], muzzleOffPoint);

	if (weapontype > WP_NONE && weapontype < WP_NUM_WEAPONS)
	{	// Use the table to generate the muzzlepoint;
		{	// Crouching.  Use the add-to-Z method to adjust vertically.
			VectorMA(muzzlePoint, muzzleOffPoint[0], forward, muzzlePoint);
			VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
			muzzlePoint[2] += ent->client->ps.viewheight + muzzleOffPoint[2];
		}
	}

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}
//[/DualPistols]

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

extern void G_MissileImpact( gentity_t *ent, trace_t *trace );
void WP_TouchVehMissile( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	trace_t	myTrace;
	memcpy( (void *)&myTrace, (void *)trace, sizeof(myTrace) );
	if ( other )
	{
		myTrace.entityNum = other->s.number;
	}
	G_MissileImpact( ent, &myTrace );
}

void WP_CalcVehMuzzle(gentity_t *ent, int muzzleNum)
{
	Vehicle_t *pVeh = ent->m_pVehicle;
	mdxaBone_t boltMatrix;
	vec3_t	vehAngles;

	assert(pVeh);

	if (pVeh->m_iMuzzleTime[muzzleNum] == level.time)
	{ //already done for this frame, don't need to do it again
		return;
	}
	//Uh... how about we set this, hunh...?  :)
	pVeh->m_iMuzzleTime[muzzleNum] = level.time;
	
	VectorCopy( ent->client->ps.viewangles, vehAngles );
	if ( pVeh->m_pVehicleInfo
		&& (pVeh->m_pVehicleInfo->type == VH_ANIMAL
			 ||pVeh->m_pVehicleInfo->type == VH_WALKER
			 ||pVeh->m_pVehicleInfo->type == VH_SPEEDER) )
	{
		vehAngles[PITCH] = vehAngles[ROLL] = 0;
	}

	trap_G2API_GetBoltMatrix_NoRecNoRot(ent->ghoul2, 0, pVeh->m_iMuzzleTag[muzzleNum], &boltMatrix, vehAngles,
		ent->client->ps.origin, level.time, NULL, ent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, pVeh->m_vMuzzlePos[muzzleNum]);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, pVeh->m_vMuzzleDir[muzzleNum]);
}

void WP_VehWeapSetSolidToOwner( gentity_t *self )
{
	self->r.svFlags |= SVF_OWNERNOTSHARED;
	if ( self->genericValue1 )
	{//expire after a time
		if ( self->genericValue2 )
		{//blow up when your lifetime is up
			self->think = G_ExplodeMissile;//FIXME: custom func?
		}
		else
		{//just remove yourself
			self->think = G_FreeEntity;//FIXME: custom func?
		}
		self->nextthink = level.time + self->genericValue1;
	}
}

#define VEH_HOMING_MISSILE_THINK_TIME		100
gentity_t *WP_FireVehicleWeapon( gentity_t *ent, vec3_t start, vec3_t dir, vehWeaponInfo_t *vehWeapon, qboolean alt_fire, qboolean isTurretWeap )
{
	gentity_t	*missile = NULL;

	//FIXME: add some randomness...?  Inherent inaccuracy stat of weapon?  Pilot skill?
	if ( !vehWeapon )
	{//invalid vehicle weapon
		return NULL;
	}
	else if ( vehWeapon->bIsProjectile )
	{//projectile entity
		vec3_t		mins, maxs;

		VectorSet( maxs, vehWeapon->fWidth/2.0f,vehWeapon->fWidth/2.0f,vehWeapon->fHeight/2.0f );
		VectorScale( maxs, -1, mins );

		//make sure our start point isn't on the other side of a wall
		WP_TraceSetStart( ent, start, mins, maxs );
		
		//FIXME: CUSTOM MODEL?
		//QUERY: alt_fire true or not?  Does it matter?
		missile = CreateMissile( start, dir, vehWeapon->fSpeed, 10000, ent, qfalse );

		missile->classname = "vehicle_proj";
		
		missile->s.genericenemyindex = ent->s.number+MAX_GENTITIES;
		missile->damage = vehWeapon->iDamage;
		missile->splashDamage = vehWeapon->iSplashDamage;
		missile->splashRadius = vehWeapon->fSplashRadius;

		//FIXME: externalize some of these properties?
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->clipmask = MASK_SHOT;
		//Maybe by checking flags...?
		if ( vehWeapon->bSaberBlockable )
		{
			missile->clipmask |= CONTENTS_LIGHTSABER;
		}
		/*
		if ( (vehWeapon->iFlags&VWF_KNOCKBACK) )
		{
			missile->dflags &= ~DAMAGE_DEATH_KNOCKBACK;
		}
		if ( (vehWeapon->iFlags&VWF_RADAR) )
		{
			missile->s.eFlags |= EF_RADAROBJECT;
		}
		*/
		// Make it easier to hit things
		VectorCopy( mins, missile->r.mins );
		VectorCopy( maxs, missile->r.maxs );
		//some slightly different stuff for things with bboxes
		if ( vehWeapon->fWidth || vehWeapon->fHeight )
		{//we assume it's a rocket-like thing
			missile->s.weapon = WP_ROCKET_LAUNCHER;//does this really matter?
			missile->methodOfDeath = MOD_VEHICLE;//MOD_ROCKET;
			missile->splashMethodOfDeath = MOD_VEHICLE;//MOD_ROCKET;// ?SPLASH;

			// we don't want it to ever bounce
			missile->bounceCount = 0;

			missile->mass = 10;
		}
		else
		{//a blaster-laser-like thing
			missile->s.weapon = WP_BLASTER;//does this really matter?
			missile->methodOfDeath = MOD_VEHICLE; //count as a heavy weap
			missile->splashMethodOfDeath = MOD_VEHICLE;// ?SPLASH;
			// we don't want it to bounce forever
			missile->bounceCount = 8;
		}
		
		if ( vehWeapon->bHasGravity )
		{//TESTME: is this all we need to do?
			missile->s.weapon = WP_THERMAL;//does this really matter?
			missile->s.pos.trType = TR_GRAVITY;
		}
		
		if ( vehWeapon->bIonWeapon )
		{//so it disables ship shields and sends them out of control
			missile->s.weapon = WP_DEMP2;
		}

		if ( vehWeapon->iHealth )
		{//the missile can take damage
			//[Asteroids]
			/*
			//don't do this - ships hit them first and have no trace.plane.normal to bounce off it at and end up in the middle of the asteroid...
			missile->health = vehWeapon->iHealth;
			missile->takedamage = qtrue;
			missile->r.contents = MASK_SHOT;
			missile->die = RocketDie;
			*/
			//[/Asteroids]
		}

		//pilot should own this projectile on server if we have a pilot
		if (ent->m_pVehicle && ent->m_pVehicle->m_pPilot)
		{//owned by vehicle pilot
			missile->r.ownerNum = ent->m_pVehicle->m_pPilot->s.number;
		}
		else
		{//owned by vehicle?
			missile->r.ownerNum = ent->s.number;
		}

		//set veh as cgame side owner for purpose of fx overrides
		missile->s.owner = ent->s.number;
		if ( alt_fire )
		{//use the second weapon's iShotFX
			missile->s.eFlags |= EF_ALT_FIRING;
		}
		if ( isTurretWeap )
		{//look for the turret weapon info on cgame side, not vehicle weapon info
			missile->s.weapon = WP_TURRET;
		}
		if ( vehWeapon->iLifeTime )
		{//expire after a time
			if ( vehWeapon->bExplodeOnExpire )
			{//blow up when your lifetime is up
				missile->think = G_ExplodeMissile;//FIXME: custom func?
			}
			else
			{//just remove yourself
				missile->think = G_FreeEntity;//FIXME: custom func?
			}
			missile->nextthink = level.time + vehWeapon->iLifeTime;
		}
		missile->s.otherEntityNum2 = (vehWeapon-&g_vehWeaponInfo[0]);
		missile->s.eFlags |= EF_JETPACK_ACTIVE;
		//homing
		if ( vehWeapon->fHoming )
		{//homing missile
			if ( ent->client && ent->client->ps.rocketLockIndex != ENTITYNUM_NONE )
			{
				int dif = 0;
				float rTime;
				rTime = ent->client->ps.rocketLockTime;

				if (rTime == -1)
				{
					rTime = ent->client->ps.rocketLastValidTime;
				}

				if ( !vehWeapon->iLockOnTime )
				{//no minimum lock-on time
					dif = 10;//guaranteed lock-on
				}
				else
				{
					float lockTimeInterval = vehWeapon->iLockOnTime/16.0f;
					dif = ( level.time - rTime ) / lockTimeInterval;
				}

				if (dif < 0)
				{
					dif = 0;
				}

				//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
				if ( dif >= 10 && rTime != -1 )
				{
					missile->enemy = &g_entities[ent->client->ps.rocketLockIndex];

					if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
					{ //if enemy became invalid, died, or is on the same team, then don't seek it
						missile->spawnflags |= 1;//just to let it know it should be faster...
						missile->speed = vehWeapon->fSpeed;
						missile->angle = vehWeapon->fHoming;
						missile->radius = vehWeapon->fHomingFOV;
						//crap, if we have a lifetime, need to store that somewhere else on ent and have rocketThink func check it every frame...
						if ( vehWeapon->iLifeTime )
						{//expire after a time
							missile->genericValue1 = level.time + vehWeapon->iLifeTime;
							missile->genericValue2 = (int)(vehWeapon->bExplodeOnExpire);
						}
						//now go ahead and use the rocketThink func
						missile->think = rocketThink;//FIXME: custom func?
						missile->nextthink = level.time + VEH_HOMING_MISSILE_THINK_TIME;
						missile->s.eFlags |= EF_RADAROBJECT;//FIXME: externalize
						if ( missile->enemy->s.NPC_class == CLASS_VEHICLE )
						{//let vehicle know we've locked on to them
							missile->s.otherEntityNum = missile->enemy->s.number;
						}
					}
				}

				VectorCopy( dir, missile->movedir );
				missile->random = 1.0f;//FIXME: externalize?
			}
		}
		if ( !vehWeapon->fSpeed )
		{//a mine or something?
			//[Asteroids]
			if ( vehWeapon->iHealth )
			{//the missile can take damage
				missile->health = vehWeapon->iHealth;
				missile->takedamage = qtrue;
				missile->r.contents = MASK_SHOT;
				missile->die = RocketDie;
			}
			//[/Asteroids]
			//only do damage when someone touches us
			missile->s.weapon = WP_THERMAL;//does this really matter?
			G_SetOrigin( missile, start );
			missile->touch = WP_TouchVehMissile;
			missile->s.eFlags |= EF_RADAROBJECT;//FIXME: externalize
			//crap, if we have a lifetime, need to store that somewhere else on ent and have rocketThink func check it every frame...
			if ( vehWeapon->iLifeTime )
			{//expire after a time
				missile->genericValue1 = vehWeapon->iLifeTime;
				missile->genericValue2 = (int)(vehWeapon->bExplodeOnExpire);
			}
			//now go ahead and use the setsolidtoowner func
			missile->think = WP_VehWeapSetSolidToOwner;
			missile->nextthink = level.time + 3000;
		}
	}
	else
	{//traceline
		//FIXME: implement
	}

	return missile;
}

//custom routine to not waste tempents horribly -rww
void G_VehMuzzleFireFX( gentity_t *ent, gentity_t *broadcaster, int muzzlesFired )
{
	Vehicle_t *pVeh = ent->m_pVehicle;
	gentity_t *b;

	if (!pVeh)
	{
		return;
	}

	if (!broadcaster)
	{ //oh well. We will WASTE A TEMPENT.
		b = G_TempEntity( ent->client->ps.origin, EV_VEH_FIRE );
	}
	else
	{ //joy
		b = broadcaster;
	}

	//this guy owns it
	b->s.owner = ent->s.number;

	//this is the bitfield of all muzzles fired this time
	//NOTE: just need MAX_VEHICLE_MUZZLES bits for this... should be cool since it's currently 12 and we're sending it in 16 bits
	b->s.trickedentindex = muzzlesFired;

	if ( broadcaster )
	{ //add the event
		G_AddEvent( b, EV_VEH_FIRE, 0 );
	}
}

void G_EstimateCamPos( vec3_t viewAngles, vec3_t cameraFocusLoc, float viewheight, float thirdPersonRange, 
					  float thirdPersonHorzOffset, float vertOffset, float pitchOffset, 
					  int ignoreEntNum, vec3_t camPos )
{
	int		MASK_CAMERACLIP = (MASK_SOLID|CONTENTS_PLAYERCLIP);
	float	CAMERA_SIZE = 4;
	vec3_t	cameramins;
	vec3_t	cameramaxs;
	vec3_t	cameraFocusAngles, camerafwd, cameraup;
	vec3_t	cameraIdealTarget, cameraCurTarget;
	vec3_t	cameraIdealLoc, cameraCurLoc;
	vec3_t	diff;
	vec3_t	camAngles;
	vec3_t	viewaxis[3];
	trace_t	trace;

	VectorSet( cameramins, -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE );
	VectorSet( cameramaxs, CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE );

	VectorCopy( viewAngles, cameraFocusAngles );
	cameraFocusAngles[PITCH] += pitchOffset;
	if ( !bg_fighterAltControl.integer )
	{//clamp view pitch
		cameraFocusAngles[PITCH] = AngleNormalize180( cameraFocusAngles[PITCH] );
		if (cameraFocusAngles[PITCH] > 80.0)
		{
			cameraFocusAngles[PITCH] = 80.0;
		}
		else if (cameraFocusAngles[PITCH] < -80.0)
		{
			cameraFocusAngles[PITCH] = -80.0;
		}
	}
	AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

	cameraFocusLoc[2] += viewheight;

	VectorCopy( cameraFocusLoc, cameraIdealTarget );
	cameraIdealTarget[2] += vertOffset;

	//NOTE: on cgame, this uses the thirdpersontargetdamp value, we ignore that here
	VectorCopy( cameraIdealTarget, cameraCurTarget );
	trap_Trace( &trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, ignoreEntNum, MASK_CAMERACLIP );
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	VectorMA(cameraIdealTarget, -(thirdPersonRange), camerafwd, cameraIdealLoc);
	//NOTE: on cgame, this uses the thirdpersoncameradamp value, we ignore that here
	VectorCopy( cameraIdealLoc, cameraCurLoc );
	trap_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, ignoreEntNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0)
	{
		VectorCopy( trace.endpos, cameraCurLoc );
	}

	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	{
		float dist = VectorNormalize(diff);
		//under normal circumstances, should never be 0.00000 and so on.
		if ( !dist || (diff[0] == 0 || diff[1] == 0) )
		{//must be hitting something, need some value to calc angles, so use cam forward
			VectorCopy( camerafwd, diff );
		}
	}

	vectoangles(diff, camAngles);

	if ( thirdPersonHorzOffset != 0.0f )
	{
		AnglesToAxis( camAngles, viewaxis );
		VectorMA( cameraCurLoc, thirdPersonHorzOffset, viewaxis[1], cameraCurLoc );
	}

	VectorCopy(cameraCurLoc, camPos);
}

void WP_GetVehicleCamPos( gentity_t *ent, gentity_t *pilot, vec3_t camPos )
{
	float thirdPersonHorzOffset = ent->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
	float thirdPersonRange = ent->m_pVehicle->m_pVehicleInfo->cameraRange;
	float pitchOffset = ent->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
	float vertOffset = ent->m_pVehicle->m_pVehicleInfo->cameraVertOffset;

	if ( ent->client->ps.hackingTime )
	{
		thirdPersonHorzOffset += (((float)ent->client->ps.hackingTime)/MAX_STRAFE_TIME) * -80.0f;
		thirdPersonRange += fabs(((float)ent->client->ps.hackingTime)/MAX_STRAFE_TIME) * 100.0f;
	}

	if ( ent->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset )
	{
		if ( pilot->client->ps.viewangles[PITCH] > 0 )
		{
			vertOffset = 130+pilot->client->ps.viewangles[PITCH]*-10;
			if ( vertOffset < -170 )
			{
				vertOffset = -170;
			}
		}
		else if ( pilot->client->ps.viewangles[PITCH] < 0 )
		{
			vertOffset = 130+pilot->client->ps.viewangles[PITCH]*-5;
			if ( vertOffset > 130 )
			{
				vertOffset = 130;
			}
		}
		else
		{
			vertOffset = 30;
		}
		if ( pilot->client->ps.viewangles[PITCH] > 0 )
		{
			pitchOffset = pilot->client->ps.viewangles[PITCH]*-0.75;
		}
		else if ( pilot->client->ps.viewangles[PITCH] < 0 )
		{
			pitchOffset = pilot->client->ps.viewangles[PITCH]*-0.75;
		}
		else
		{
			pitchOffset = 0;
		}
	}

	//Control Scheme 3 Method:
	G_EstimateCamPos( ent->client->ps.viewangles, pilot->client->ps.origin, pilot->client->ps.viewheight, thirdPersonRange, 
		thirdPersonHorzOffset, vertOffset, pitchOffset, 
		pilot->s.number, camPos );
	/*
	//Control Scheme 2 Method:
	G_EstimateCamPos( ent->m_pVehicle->m_vOrientation, ent->r.currentOrigin, pilot->client->ps.viewheight, thirdPersonRange, 
		thirdPersonHorzOffset, vertOffset, pitchOffset, 
		pilot->s.number, camPos );
	*/
}

void WP_VehLeadCrosshairVeh( gentity_t *camTraceEnt, vec3_t newEnd, const vec3_t dir, const vec3_t shotStart, vec3_t shotDir )
{
	//[Asteroids]
	if ( g_vehAutoAimLead.integer )
	{
		if ( camTraceEnt 
			&& camTraceEnt->client
			&& camTraceEnt->client->NPC_class == CLASS_VEHICLE )
		{//if the crosshair is on a vehicle, lead it
			float dot, distAdjust = DotProduct( camTraceEnt->client->ps.velocity, dir );
			vec3_t	predPos, predShotDir;
			if ( distAdjust > 500 || DistanceSquared( camTraceEnt->client->ps.origin, shotStart ) > 7000000 )
			{//moving away from me at a decent speed and/or more than @2600 units away from me
				VectorMA( newEnd, distAdjust, dir, predPos );
				VectorSubtract( predPos, shotStart, predShotDir );
				VectorNormalize( predShotDir );
				dot = DotProduct( predShotDir, shotDir );
				if ( dot >= 0.75f )
				{//if the new aim vector is no more than 23 degrees off the original one, go ahead and adjust the aim
					VectorCopy( predPos, newEnd );
				}
			}
		}
	//[/Asteroids]
	}
	VectorSubtract( newEnd, shotStart, shotDir );
	VectorNormalize( shotDir );
}

#define MAX_XHAIR_DIST_ACCURACY	20000.0f
extern float g_cullDistance;
extern int BG_VehTraceFromCamPos( trace_t *camTrace, bgEntity_t *bgEnt, const vec3_t entOrg, const vec3_t shotStart, const vec3_t end, vec3_t newEnd, vec3_t shotDir, float bestDist );
qboolean WP_VehCheckTraceFromCamPos( gentity_t *ent, const vec3_t shotStart, vec3_t shotDir )
{
	//FIXME: only if dynamicCrosshair and dynamicCrosshairPrecision is on!
	if ( !ent 
		|| !ent->m_pVehicle 
		|| !ent->m_pVehicle->m_pVehicleInfo 
		|| !ent->m_pVehicle->m_pPilot//not being driven
		|| !((gentity_t*)ent->m_pVehicle->m_pPilot)->client//not being driven by a client...?!!!
		|| (ent->m_pVehicle->m_pPilot->s.number >= MAX_CLIENTS) )//being driven, but not by a real client, no need to worry about crosshair
	{
		return qfalse;
	}
	if ( (ent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && g_cullDistance > MAX_XHAIR_DIST_ACCURACY )
		|| ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
	{
		//FIRST: simulate the normal crosshair trace from the center of the veh straight forward
		trace_t trace;
		vec3_t	dir, start, end;
		if ( ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
		{//for some reason, the walker always draws the crosshair out from from the first muzzle point
			AngleVectors( ent->client->ps.viewangles, dir, NULL, NULL );
			VectorCopy( ent->r.currentOrigin, start );
			start[2] += ent->m_pVehicle->m_pVehicleInfo->height-DEFAULT_MINS_2-48;
		}
		else
		{
			vec3_t ang;
			if (ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{
				VectorSet(ang, 0.0f, ent->m_pVehicle->m_vOrientation[1], 0.0f);
			}
			else
			{
				VectorCopy(ent->m_pVehicle->m_vOrientation, ang);
			}
			AngleVectors( ang, dir, NULL, NULL );
			VectorCopy( ent->r.currentOrigin, start );
		}
		VectorMA( start, g_cullDistance, dir, end );
		trap_Trace( &trace, start, vec3_origin, vec3_origin, end, 
			ent->s.number, CONTENTS_SOLID|CONTENTS_BODY );

		if ( ent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
		{//just use the result of that one trace since walkers don't do the extra trace
			VectorSubtract( trace.endpos, shotStart, shotDir );
			VectorNormalize( shotDir );
			return qtrue;
		}
		else
		{//NOW do the trace from the camPos and compare with above trace
			trace_t	extraTrace;
			vec3_t	newEnd;
			int camTraceEntNum = BG_VehTraceFromCamPos( &extraTrace, (bgEntity_t *)ent, ent->r.currentOrigin, shotStart, end, newEnd, shotDir, (trace.fraction*g_cullDistance) );
			if ( camTraceEntNum )
			{
				WP_VehLeadCrosshairVeh( &g_entities[camTraceEntNum-1], newEnd, dir, shotStart, shotDir );
				return qtrue;
			}
		}
	}
	return qfalse;
}

//---------------------------------------------------------
void FireVehicleWeapon( gentity_t *ent, qboolean alt_fire ) 
//---------------------------------------------------------
{
	Vehicle_t *pVeh = ent->m_pVehicle;
	int muzzlesFired = 0;
	gentity_t *missile = NULL;
	vehWeaponInfo_t *vehWeapon = NULL;
	qboolean	clearRocketLockEntity = qfalse;
	
	//ROP VEHICLE_IMP START
	int nStopPrimary;
	int	nStopAlt;

	if ( !pVeh )
	{
		return;
	}

	if(pVeh->bTransAnimFlag)
	{
		//Can never fire during transition anims
		return;
	}

	//Check whether we can fire
	nStopPrimary = pVeh->m_pVehicleInfo->MBFstopprimaryfiring;
	nStopAlt = pVeh->m_pVehicleInfo->MBFstopaltfiring;

	if((pVeh->eCurrentMode & pVeh->m_pVehicleInfo->MBFstopprimaryfiring) && !alt_fire)
	{
		//We can't primary fire in current mode
		return;
	}

	if((pVeh->eCurrentMode & pVeh->m_pVehicleInfo->MBFstopaltfiring) && alt_fire)
	{
		//We can't alt fire in current mode
		return;
	}
	//ROP VEHICLE_IMP END

	if (pVeh->m_iRemovedSurfaces)
	{ //can't fire when the thing is breaking apart
		return;
	}

	if (pVeh->m_pVehicleInfo->type == VH_WALKER &&
		ent->client->ps.electrifyTime > level.time)
	{ //don't fire while being electrocuted
		return;
	}

	// TODO?: If possible (probably not enough time), it would be nice if secondary fire was actually a mode switch/toggle
	// so that, for instance, an x-wing can have 4-gun fire, or individual muzzle fire. If you wanted a different weapon, you
	// would actually have to press the 2 key or something like that (I doubt I'd get a graphic for it anyways though). -AReis

	// If this is not the alternate fire, fire a normal blaster shot...
	if ( pVeh->m_pVehicleInfo &&
		(pVeh->m_pVehicleInfo->type != VH_FIGHTER || (pVeh->m_ulFlags&VEH_WINGSOPEN)) ) // NOTE: Wings open also denotes that it has already launched.
	{//fighters can only fire when wings are open
		int	weaponNum = 0, vehWeaponIndex = VEH_WEAPON_NONE;
		int	delay = 1000;
		qboolean aimCorrect = qfalse;
		qboolean linkedFiring = qfalse;

		if ( !alt_fire )
		{
			weaponNum = 0;
		}
		else
		{
			weaponNum = 1;
		}

		vehWeaponIndex = pVeh->m_pVehicleInfo->weapon[weaponNum].ID;

		if ( pVeh->weaponStatus[weaponNum].ammo <= 0 )
		{//no ammo for this weapon
			if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
			{// let the client know he's out of ammo
				int i;
				//but only if one of the vehicle muzzles is actually ready to fire this weapon
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
					{//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					if ( pVeh->m_iMuzzleTag[i] != -1 
						&& pVeh->m_iMuzzleWait[i] < level.time )
					{//this one would have fired, send the no ammo message
						G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
						break;
					}
				}
			}
			return;
		}

		delay = pVeh->m_pVehicleInfo->weapon[weaponNum].delay;
		aimCorrect = pVeh->m_pVehicleInfo->weapon[weaponNum].aimCorrect;
		if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 2//always linked
			|| ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 1//optionally linkable
				 && pVeh->weaponStatus[weaponNum].linked ) )//linked
		{//we're linking the primary or alternate weapons, so we'll do *all* the muzzles
			linkedFiring = qtrue;
		}

		if ( vehWeaponIndex <= VEH_WEAPON_BASE || vehWeaponIndex >= MAX_VEH_WEAPONS )
		{//invalid vehicle weapon
			return;
		}
		else
		{
			int i, numMuzzles = 0, numMuzzlesReady = 0, cumulativeDelay = 0, cumulativeAmmo = 0;
			qboolean sentAmmoWarning = qfalse;

			vehWeapon = &g_vehWeaponInfo[vehWeaponIndex];

			if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 2 )
			{//always linked weapons don't accumulate delay, just use specified delay
				cumulativeDelay = delay;
			}
			//find out how many we've got for this weapon
			for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
			{
				if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
				{//this muzzle doesn't match the weapon we're trying to use
					continue;
				}
				if ( pVeh->m_iMuzzleTag[i] != -1 && pVeh->m_iMuzzleWait[i] < level.time )
				{
					numMuzzlesReady++;
				}
				if ( pVeh->m_pVehicleInfo->weapMuzzle[pVeh->weaponStatus[weaponNum].nextMuzzle] != vehWeaponIndex )
				{//Our designated next muzzle for this weapon isn't valid for this weapon (happens when ships fire for the first time)
					//set the next to this one
					pVeh->weaponStatus[weaponNum].nextMuzzle = i;
				}
				if ( linkedFiring )
				{
					cumulativeAmmo += vehWeapon->iAmmoPerShot;
					if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable != 2 )
					{//always linked weapons don't accumulate delay, just use specified delay
						cumulativeDelay += delay;
					}
				}
				numMuzzles++;
			}

			if ( linkedFiring )
			{//firing all muzzles at once
				if ( numMuzzlesReady != numMuzzles )
				{//can't fire all linked muzzles yet
					return;
				}
				else 
				{//can fire all linked muzzles, check ammo
					if ( pVeh->weaponStatus[weaponNum].ammo < cumulativeAmmo )
					{//can't fire, not enough ammo
						if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
						{// let the client know he's out of ammo
							G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
						}
						return;
					}
				}
			}

			for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
			{
				if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
				{//this muzzle doesn't match the weapon we're trying to use
					continue;
				}
				if ( !linkedFiring
					&& i != pVeh->weaponStatus[weaponNum].nextMuzzle )
				{//we're only firing one muzzle and this isn't it
					continue;
				}

				// Fire this muzzle.
				if ( pVeh->m_iMuzzleTag[i] != -1 && pVeh->m_iMuzzleWait[i] < level.time )
				{
					vec3_t	start, dir;
					
					if ( pVeh->weaponStatus[weaponNum].ammo < vehWeapon->iAmmoPerShot )
					{//out of ammo!
						if ( !sentAmmoWarning )
						{
							sentAmmoWarning = qtrue;
							if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
							{// let the client know he's out of ammo
								G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
							}
						}
					}
					else
					{//have enough ammo to shoot
						//do the firing
						WP_CalcVehMuzzle(ent, i);
						VectorCopy( pVeh->m_vMuzzlePos[i], start );
						VectorCopy( pVeh->m_vMuzzleDir[i], dir );
						if ( WP_VehCheckTraceFromCamPos( ent, start, dir ) )
						{//auto-aim at whatever crosshair would be over from camera's point of view (if closer)
						}
						else if ( aimCorrect )
						{//auto-aim the missile at the crosshair if there's anything there
							trace_t trace;
							vec3_t	end;
							vec3_t	ang;
							vec3_t	fixedDir;

							if (pVeh->m_pVehicleInfo->type == VH_SPEEDER)
							{
								VectorSet(ang, 0.0f, pVeh->m_vOrientation[1], 0.0f);
							}
							else
							{
								VectorCopy(pVeh->m_vOrientation, ang);
							}
							AngleVectors( ang, fixedDir, NULL, NULL );
							VectorMA( ent->r.currentOrigin, 32768, fixedDir, end );
							//VectorMA( ent->r.currentOrigin, 8192, dir, end );
							trap_Trace( &trace, ent->r.currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, MASK_SHOT );
							if ( trace.fraction < 1.0f && !trace.allsolid && !trace.startsolid )
							{
								vec3_t newEnd;
								VectorCopy( trace.endpos, newEnd );
								WP_VehLeadCrosshairVeh( &g_entities[trace.entityNum], newEnd, fixedDir, start, dir );
							}
						}

						//play the weapon's muzzle effect if we have one
						//NOTE: just need MAX_VEHICLE_MUZZLES bits for this... should be cool since it's currently 12 and we're sending it in 16 bits
						muzzlesFired |= (1<<i);
												
						missile = WP_FireVehicleWeapon( ent, start, dir, vehWeapon, alt_fire, qfalse );
						if ( vehWeapon->fHoming )
						{//clear the rocket lock entity *after* all muzzles have fired
							clearRocketLockEntity = qtrue;
						}
					}

					if ( linkedFiring )
					{//we're linking the weapon, so continue on and fire all appropriate muzzles
						continue;
					}
					//else just firing one
					//take the ammo, set the next muzzle and set the delay on it
					if ( numMuzzles > 1 )
					{//more than one, look for it
						int nextMuzzle = pVeh->weaponStatus[weaponNum].nextMuzzle;
						while ( 1 )
						{
							nextMuzzle++;
							if ( nextMuzzle >= MAX_VEHICLE_MUZZLES )
							{
								nextMuzzle = 0;
							}
							if ( nextMuzzle == pVeh->weaponStatus[weaponNum].nextMuzzle )
							{//WTF?  Wrapped without finding another valid one!
								break;
							}
							if ( pVeh->m_pVehicleInfo->weapMuzzle[nextMuzzle] == vehWeaponIndex )
							{//this is the next muzzle for this weapon
								pVeh->weaponStatus[weaponNum].nextMuzzle = nextMuzzle;
								break;
							}
						}
					}//else, just stay on the one we just fired
					//set the delay on the next muzzle
					pVeh->m_iMuzzleWait[pVeh->weaponStatus[weaponNum].nextMuzzle] = level.time + delay;
					//take away the ammo
					pVeh->weaponStatus[weaponNum].ammo -= vehWeapon->iAmmoPerShot;
					//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
					if ( pVeh->m_pParentEntity && ((gentity_t*)(pVeh->m_pParentEntity))->client )
					{
						((gentity_t*)(pVeh->m_pParentEntity))->client->ps.ammo[weaponNum] = pVeh->weaponStatus[weaponNum].ammo;
					}
					//done!
					//we'll get in here again next frame and try the next muzzle...
					//return;
					goto tryFire;
				}
			}
			//we went through all the muzzles, so apply the cumulative delay and ammo cost
			if ( cumulativeAmmo )
			{//taking ammo one shot at a time
				//take the ammo
				pVeh->weaponStatus[weaponNum].ammo -= cumulativeAmmo;
				//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
				if ( pVeh->m_pParentEntity && ((gentity_t*)(pVeh->m_pParentEntity))->client )
				{
					((gentity_t*)(pVeh->m_pParentEntity))->client->ps.ammo[weaponNum] = pVeh->weaponStatus[weaponNum].ammo;
				}
			}
			if ( cumulativeDelay )
			{//we linked muzzles so we need to apply the cumulative delay now, to each of the linked muzzles
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
					{//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					//apply the cumulative delay
					pVeh->m_iMuzzleWait[i] = level.time + cumulativeDelay;
				}
			}
		}
	}

tryFire:
	if ( clearRocketLockEntity )
	{//hmm, should probably clear that anytime any weapon fires?
		ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
		ent->client->ps.rocketLockTime = 0;
		ent->client->ps.rocketTargetTime = 0;
	}

	if ( vehWeapon && muzzlesFired > 0 )
	{
		G_VehMuzzleFireFX(ent, missile, muzzlesFired );
	}
}

//[WeapAccuracy]
int SkillLevelforWeapon(gentity_t *ent, int weapon)
{
	if(!ent || !ent->inuse || !ent->client)
	{
		return 0;
	}

	switch(weapon)
	{
		case WP_REPEATER:
			return ent->client->skillLevel[SK_REPEATER];
			break;
		case WP_ROCKET_LAUNCHER:
			return ent->client->skillLevel[SK_ROCKET];
			break;
		case WP_BOWCASTER:
			return ent->client->skillLevel[SK_BOWCASTER];
			break;
		case WP_DISRUPTOR:
			return ent->client->skillLevel[SK_DISRUPTOR];
			break;
		case WP_THERMAL:
			return ent->client->skillLevel[SK_THERMAL];
			break;
	case WP_STUN_BATON:
			return ent->client->skillLevel[SK_WRIST];
			break;
		case WP_DEMP2:
			return ent->client->skillLevel[SK_DEMP2];
			break;
		case WP_BRYAR_PISTOL:
			return ent->client->skillLevel[SK_PISTOL];
			break;
		case WP_BRYAR_OLD:
			return ent->client->skillLevel[SK_OLD];
			break;
		case WP_CONCUSSION:
			return ent->client->skillLevel[SK_CONCUSSION];
			break;
		case WP_FLECHETTE:
			return ent->client->skillLevel[SK_FLECHETTE];
			break;
		case WP_TRIP_MINE:
			return ent->client->skillLevel[SK_TRIPMINE];
			break;
		case WP_DET_PACK:
			return ent->client->skillLevel[SK_DETPACK];
			break;
		default:
			return ent->client->skillLevel[SK_BLASTER];
			break;
	};
}
//[/WeapAccuracy]


/*
===============
FireWeapon
===============
*/
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint);

//[WeapAccuracy]
extern void G_AddMercBalance(gentity_t *self, int amount);
//[/WeapAccuracy]
void FireWeapon( gentity_t *ent, qboolean altFire ) 
{
	//[CoOp]
	float alert = 256;  //alert level for weapon alter events
	//[/CoOp]

		s_quadFactor = 1;
	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( ent->s.weapon != WP_SABER && ent->s.weapon != WP_STUN_BATON && ent->s.weapon != WP_MELEE ) 
	{
		if( ent->s.weapon == WP_FLECHETTE ) {
			ent->client->accuracy_shots += FLECHETTE_SHOTS;
		} else {
			ent->client->accuracy_shots++;
		}
	}

	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		FireVehicleWeapon( ent, altFire );
		return;
	}
	else
	{
		// set aiming directions
		if (ent->s.weapon == WP_EMPLACED_GUN &&
			ent->client->ps.emplacedIndex)
		{ //if using emplaced then base muzzle point off of gun position/angles
			gentity_t *emp = &g_entities[ent->client->ps.emplacedIndex];

			if (emp->inuse)
			{
				float yaw;
				vec3_t viewAngCap;
				int override;

				VectorCopy(ent->client->ps.viewangles, viewAngCap);
				if (viewAngCap[PITCH] > 40)
				{
					viewAngCap[PITCH] = 40;
				}

				override = BG_EmplacedView(ent->client->ps.viewangles, emp->s.angles, &yaw,
					emp->s.origin2[0]);
				
				if (override)
				{
					viewAngCap[YAW] = yaw;
				}

				AngleVectors( viewAngCap, forward, vright, up );
			}
			else
			{
				AngleVectors( ent->client->ps.viewangles, forward, vright, up );
			}
		}
		else if (ent->s.number < MAX_CLIENTS &&
			ent->client->ps.m_iVehicleNum && ent->s.weapon == WP_BLASTER)
		{ //riding a vehicle...with blaster selected
			vec3_t vehTurnAngles;
			gentity_t *vehEnt = &g_entities[ent->client->ps.m_iVehicleNum];

			if (vehEnt->inuse && vehEnt->client && vehEnt->m_pVehicle)
			{
				VectorCopy(vehEnt->m_pVehicle->m_vOrientation, vehTurnAngles);
				vehTurnAngles[PITCH] = ent->client->ps.viewangles[PITCH];
			}
			else
			{
				VectorCopy(ent->client->ps.viewangles, vehTurnAngles);
			}
			if (ent->client->pers.cmd.rightmove > 0)
			{ //shooting to right
				vehTurnAngles[YAW] -= 90.0f;
			}
			else if (ent->client->pers.cmd.rightmove < 0)
			{ //shooting to left
				vehTurnAngles[YAW] += 90.0f;
			}

			AngleVectors( vehTurnAngles, forward, vright, up );
		}
		else
		{
			AngleVectors( ent->client->ps.viewangles, forward, vright, up );
		}

		CalcMuzzlePoint ( ent, forward, vright, up, muzzle );
		//[DualPistols]
		if (ent->client->ps.eFlags & EF_DUAL_WEAPONS)
			CalcMuzzlePoint2 ( ent, forward, vright, up, muzzle2 );
		//[/DualPistols]
		else if (ent->client->skillLevel[SK_CONCUSSION] == FORCE_LEVEL_3)
			CalcMuzzlePoint2 ( ent, forward, vright, up, muzzle2 );
		else if (ent->client->skillLevel[SK_ROCKET] == FORCE_LEVEL_3)
			CalcMuzzlePoint2 ( ent, forward, vright, up, muzzle2 );
	//[WeapAccuracy]
		//bump accuracy based on MP level.
		if(ent && ent->client)
		{
			vec3_t angs; //used for adding in mishap inaccuracy.
			float slopFactor = MISHAP_MAXINACCURACY * (1 - (ent->client->ps.MISHAP_VARIABLE/(float)MISHAPLEVEL_LIGHT));
			slopFactor = Com_Clamp(0, MISHAP_MAXINACCURACY, slopFactor);

			vectoangles( forward, angs );
			angs[PITCH] += flrand(-slopFactor, slopFactor);
			angs[YAW] += flrand(-slopFactor, slopFactor);
			AngleVectors( angs, forward, NULL, NULL );

			//increase mishap level
			if(!Q_irand(0, SkillLevelforWeapon(ent, ent->s.weapon)-1) && ent->s.weapon != WP_EMPLACED_GUN )//Sorry but the mishap meter needs to go up more that before.
			{//failed skill roll, add mishap.
				if(ent->s.weapon == WP_DISRUPTOR && ent->client->ps.zoomMode == 0)
					G_AddMercBalance(ent, Q_irand(2, 3));// 1 was not enough
				else if(ent->s.weapon == WP_FLECHETTE)
					G_AddMercBalance(ent,1);
				else if(ent->s.weapon == WP_BRYAR_PISTOL)
					G_AddMercBalance(ent,Q_irand(2,3));
				else if(ent->s.weapon == WP_REPEATER)
				{
					ent->client->cloneFired++;
					if(ent->client->cloneFired == 2)
					{
						if(ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove ==0)
							G_AddMercBalance(ent,1);
						else
							G_AddMercBalance(ent,2);

						ent->client->cloneFired=0;
					}
				}
				else
					G_AddMercBalance(ent, Q_irand(1, 2));// 1 was not enough
			}
		}
		//[/WeapAccuracy]

		// fire the specific weapon
		switch( ent->s.weapon ) {
		case WP_STUN_BATON:
			WP_FireStunBaton(ent, altFire);
			break;

		case WP_MELEE:
			//[CoOp]
			alert = 0;
			//[/CoOp]
			WP_FireMelee(ent, altFire);
			break;

		case WP_SABER:
			break;

		case WP_BRYAR_PISTOL:
			//[WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireBryarPistol2( ent, altFire );
				}
				else
				{
					WP_FireBryarPistol2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireBryarPistol3( ent, altFire );
				}
				else
				{
					WP_FireBryarPistol3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireBryarPistol4( ent, altFire );
				}
				else
				{
					WP_FireBryarPistol4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireBryarPistol( ent, altFire );
				}
				else
				{
					WP_FireBryarPistol( ent, qfalse );
				}
			}
			break;

		case WP_CONCUSSION:
						//[/WeaponSys]

			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireConcussion2( ent, altFire );
				}
				else
				{
					WP_FireConcussion2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireConcussion3( ent, altFire );
				}
				else
				{
					WP_FireConcussion3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireConcussion4( ent, altFire);
				}
				else
				{
					WP_FireConcussion4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireConcussion( ent, altFire );
				}
				else
				{
					WP_FireConcussion( ent, qfalse );
				}
			}	
			break;

		case WP_BRYAR_OLD:
			//[WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireBryarOld2( ent, altFire );
				}
				else
				{
					WP_FireBryarOld2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireBryarOld3( ent, altFire );
				}
				else
				{
					WP_FireBryarOld3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireBryarOld4( ent, altFire );
				}
				else
				{
					WP_FireBryarOld4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireBryarOld( ent, altFire );
				}
				else
				{
					WP_FireBryarOld( ent, qfalse );
				}
			}
			break;
			//[/WeaponSys]

		case WP_BLASTER:
			//[/WeaponSys]

			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireBlaster2( ent, qfalse );
				}
				else
				{
					WP_FireBlaster2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireBlaster3( ent, qfalse );
				}
				else
				{
					WP_FireBlaster3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireBlaster4( ent, qfalse );
				}
				else
				{
					WP_FireBlaster4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireBlaster( ent, qfalse );
				}
				else
				{
					WP_FireBlaster( ent, qfalse );
				}
			}			
			break;

		case WP_DISRUPTOR:
			//[CoOp]
			alert = 50;
			//[/CoOp]
			//[WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireDisruptor2( ent, qfalse );
				}
				else
				{
					WP_FireDisruptor2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireDisruptor3( ent, qfalse );
				}
				else
				{
					WP_FireDisruptor3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireDisruptor4( ent, altFire );
				}
				else
				{
					WP_FireDisruptor4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireDisruptor( ent, altFire );
				}
				else
				{
					WP_FireDisruptor( ent, qfalse );
				}
			}
			break;

		case WP_BOWCASTER:
			//[/WeaponSys]

			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					return;
				}
				else
				{
					WP_FireBowcaster2( ent, altFire );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					return;
				}
				else
				{
					WP_FireBowcaster3( ent, altFire );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					return;
				}
				else
				{
					WP_FireBowcaster4( ent, altFire );
				}
			}
			else 
			{	
				if ( altFire )
				{
					return;
				}
				else
				{
					WP_FireBowcaster( ent, altFire );
				}
			}	
			break;

		case WP_REPEATER:
						//[/WeaponSys]

			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireRepeater2( ent, altFire );
				}
				else
				{
					WP_FireRepeater2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireRepeater3( ent, altFire );
				}
				else
				{
					WP_FireRepeater3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireRepeater4( ent, altFire);
				}
				else
				{
					WP_FireRepeater4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireRepeater( ent, altFire );
				}
				else
				{
					WP_FireRepeater( ent, qfalse );
				}
			}	
			break;

		case WP_DEMP2:
						//[WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireDEMP22( ent, altFire );
				}
				else
				{
					WP_FireDEMP22( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireDEMP23( ent, altFire );
				}
				else
				{
					WP_FireDEMP23( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireDEMP24( ent, altFire );
				}
				else
				{
					WP_FireDEMP24( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireDEMP2( ent, altFire );
				}
				else
				{
					WP_FireDEMP2( ent, qfalse );
				}
			}
			break;

		case WP_FLECHETTE:
			//[/WeaponSys]

			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireFlechette2( ent, altFire );
				}
				else
				{
					WP_FireFlechette2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireFlechette3( ent, altFire );
				}
				else
				{
					WP_FireFlechette3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireFlechette4( ent, altFire);
				}
				else
				{
					WP_FireFlechette4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireFlechette( ent, altFire );
				}
				else
				{
					WP_FireFlechette( ent, qfalse );
				}
			}	
			break;

		case WP_ROCKET_LAUNCHER:
			//[/WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
				if ( altFire )
				{
					WP_FireRocket2( ent, altFire );
				}
				else
				{
					WP_FireRocket2( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
				if ( altFire )
				{
					WP_FireRocket3( ent, altFire );
				}
				else
				{
					WP_FireRocket3( ent, qfalse );
				}
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
				if ( altFire )
				{
					WP_FireRocket4( ent, altFire);
				}
				else
				{
					WP_FireRocket4( ent, qfalse );
				}
			}
			else 
			{	
				if ( altFire )
				{
					WP_FireRocket( ent, altFire );
				}
				else
				{
					WP_FireRocket( ent, qfalse );
				}
			}	
			break;

		case WP_THERMAL:
			//[WeaponSys]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
			WP_FireThermalDetonator2( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
			WP_FireThermalDetonator3( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
			WP_FireThermalDetonator4( ent, altFire );
			//[/WeaponSys]
			}
			else 
			{	
			WP_FireThermalDetonator( ent, altFire );
			//[/WeaponSys]
			}	
			break;

		case WP_TRIP_MINE:
			//[CoOp]
			alert = 0;
			//[/CoOp]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
			WP_PlaceLaserTrap2( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
			WP_PlaceLaserTrap3( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
			WP_PlaceLaserTrap4( ent, altFire );
			//[/WeaponSys]
			}
			else 
			{	
			WP_PlaceLaserTrap( ent, altFire );
			//[/WeaponSys]
			}
			break;

		case WP_DET_PACK:
			//[CoOp]
			alert = 0;
			//[/CoOp]
			if(ent->client->ps.eFlags & EF_WP_OPTION_2)
			{	
			WP_DropDetPack2( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_3)
			{	
			WP_DropDetPack3( ent, altFire );
			//[/WeaponSys]
			}
			else if(ent->client->ps.eFlags & EF_WP_OPTION_4)
			{	
			WP_DropDetPack4( ent, altFire );
			//[/WeaponSys]
			}
			else 
			{	
			WP_DropDetPack( ent, altFire );
			//[/WeaponSys]
			}
			break;

		case WP_EMPLACED_GUN:
			if (ent->client && ent->client->ewebIndex)
			{ //specially handled by the e-web itself
				break;
			}
			WP_FireEmplaced( ent, altFire );
			break;
		default:
//			assert(!"unknown weapon fire");
			break;
		}
	}

	//[CoOp] SP code
	//alert events for NPCs
	// We should probably just use this as a default behavior, in special cases, just set alert to false.
	if ( alert > 0 )
	{
		if ( ent->client->ps.groundEntityNum == ENTITYNUM_WORLD//FIXME: check for sand contents type?
			&& ent->s.weapon != WP_MELEE
			//&& ent->s.weapon != WP_TUSKEN_STAFF //RAFIXME - Impliment?
			&& ent->s.weapon != WP_THERMAL
			&& ent->s.weapon != WP_TRIP_MINE
			&& ent->s.weapon != WP_DET_PACK )
		{//the vibration of the shot carries through your feet into the ground
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED, qfalse, qtrue );
		}
		else
		{//an in-air alert
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED, qfalse, qfalse );
		}
		AddSightEvent( ent, muzzle, alert*2, AEL_DISCOVERED, 20 );
	}
	//[/CoOp]

	G_LogWeaponFire(ent->s.number, ent->s.weapon);
}

//---------------------------------------------------------
static void WP_FireEmplaced( gentity_t *ent, qboolean altFire )
//---------------------------------------------------------
{
	vec3_t	dir, angs, gunpoint;
	vec3_t	right;
	gentity_t *gun;
	int side;

	if (!ent->client)
	{
		return;
	}

	if (!ent->client->ps.emplacedIndex)
	{ //shouldn't be using WP_EMPLACED_GUN if we aren't on an emplaced weapon
		return;
	}

	gun = &g_entities[ent->client->ps.emplacedIndex];

	if (!gun->inuse || gun->health <= 0)
	{ //gun was removed or killed, although we should never hit this check because we should have been forced off it already
		return;
	}

	VectorCopy(gun->s.origin, gunpoint);
	gunpoint[2] += 46;

	AngleVectors(ent->client->ps.viewangles, NULL, right, NULL);

	if (gun->genericValue10)
	{ //fire out of the right cannon side
		VectorMA(gunpoint, 10.0f, right, gunpoint);
		side = 0;
	}
	else
	{ //the left
		VectorMA(gunpoint, -10.0f, right, gunpoint);
		side = 1;
	}

	gun->genericValue10 = side;
	G_AddEvent(gun, EV_FIRE_WEAPON, side);

	vectoangles( forward, angs );

	AngleVectors( angs, dir, NULL, NULL );

	WP_FireEmplacedMissile( gun, gunpoint, dir, altFire, ent );
}


#define EMPLACED_CANRESPAWN 1

//----------------------------------------------------------

/*QUAKED emplaced_gun (0 0 1) (-30 -20 8) (30 20 60) CANRESPAWN

 count - if CANRESPAWN spawnflag, decides how long it is before gun respawns (in ms)
 constraint - number of degrees gun is constrained from base angles on each side (default 60.0)

 showhealth - set to 1 to show health bar on this entity when crosshair is over it
  
  teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

  alliedTeam - team that can use this
	0 - any
	1 - red
	2 - blue

  teamnodmg - team that turret does not take damage from or do damage to
	0 - none
	1 - red
	2 - blue
*/
 
//----------------------------------------------------------
extern qboolean TryHeal(gentity_t *ent, gentity_t *target); //g_utils.c
void emplaced_gun_use( gentity_t *self, gentity_t *other, trace_t *trace )
{
	vec3_t fwd1, fwd2;
	float dot;
	int oldWeapon;
	gentity_t *activator = other;
	float zoffset = 50;
	vec3_t anglesToOwner;
	vec3_t vLen;
	float ownLen;

	if ( self->health <= 0 )
	{ //gun is destroyed
		return;
	}

	if (self->activator)
	{ //someone is already using me
		return;
	}

	if (!activator->client)
	{
		return;
	}

	if (activator->client->ps.emplacedTime > level.time)
	{ //last use attempt still too recent
		return;
	}

	if (activator->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //don't use if busy doing something else
		return;
	}

	if (activator->client->ps.origin[2] > self->s.origin[2]+zoffset-8)
	{ //can't use it from the top
		return;
	}

	if (activator->client->ps.pm_flags & PMF_DUCKED)
	{ //must be standing
		return;
	}

	if (activator->client->ps.isJediMaster)
	{ //jm can't use weapons
		return;
	}

	VectorSubtract(self->s.origin, activator->client->ps.origin, vLen);
	ownLen = VectorLength(vLen);

	if (ownLen > 64.0f)
	{ //must be within 64 units of the gun to use at all
		return;
	}

	// Let's get some direction vectors for the user
	AngleVectors( activator->client->ps.viewangles, fwd1, NULL, NULL );

	// Get the guns direction vector
	AngleVectors( self->pos1, fwd2, NULL, NULL );

	dot = DotProduct( fwd1, fwd2 );

	// Must be reasonably facing the way the gun points ( 110 degrees or so ), otherwise we don't allow to use it.
	if ( dot < -0.2f )
	{
		goto tryHeal;
	}

	VectorSubtract(self->s.origin, activator->client->ps.origin, fwd1);
	VectorNormalize(fwd1);

	dot = DotProduct( fwd1, fwd2 );

	//check the positioning in relation to the gun as well
	if ( dot < 0.6f )
	{
		goto tryHeal;
	}

	self->genericValue1 = 1;

	oldWeapon = activator->s.weapon;

	// swap the users weapon with the emplaced gun
	activator->client->ps.weapon = self->s.weapon;
	activator->client->ps.weaponstate = WEAPON_READY;
	activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

	activator->client->ps.emplacedIndex = self->s.number;

	self->s.emplacedOwner = activator->s.number;
	self->s.activeForcePass = NUM_FORCE_POWERS+1;

	// the gun will track which weapon we used to have
	self->s.weapon = oldWeapon;

	//user's new owner becomes the gun ent
	activator->r.ownerNum = self->s.number;
	self->activator = activator;

	VectorSubtract(self->r.currentOrigin, activator->client->ps.origin, anglesToOwner);
	vectoangles(anglesToOwner, anglesToOwner);
	return;

tryHeal: //well, not in the right dir, try healing it instead...
	TryHeal(activator, self);
}

void emplaced_gun_realuse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	emplaced_gun_use(self, other, NULL);
}

//----------------------------------------------------------
void emplaced_gun_pain( gentity_t *self, gentity_t *attacker, int damage )
{
	self->s.health = self->health;

	if ( self->health <= 0 )
	{
		//death effect.. for now taken care of on cgame
	}
	else
	{
		//if we have a pain behavior set then use it I guess
		G_ActivateBehavior( self, BSET_PAIN );
	}
}

#define EMPLACED_GUN_HEALTH 800

//----------------------------------------------------------
void emplaced_gun_update(gentity_t *self)
{
	vec3_t	smokeOrg, puffAngle;
	int oldWeap;
	float ownLen = 0;

	if (self->health < 1 && !self->genericValue5)
	{ //we are dead, set our respawn delay if we have one
		if (self->spawnflags & EMPLACED_CANRESPAWN)
		{
			self->genericValue5 = level.time + 4000 + self->count;
		}
	}
	else if (self->health < 1 && self->genericValue5 < level.time)
	{ //we are dead, see if it's time to respawn
		self->s.time = 0;
		self->genericValue4 = 0;
		self->genericValue3 = 0;
		self->health = EMPLACED_GUN_HEALTH*0.4;
		self->s.health = self->health;
	}

	if (self->genericValue4 && self->genericValue4 < 2 && self->s.time < level.time)
	{ //we have finished our warning (red flashing) effect, it's time to finish dying
		vec3_t explOrg;

		VectorSet( puffAngle, 0, 0, 1 );

		VectorCopy(self->r.currentOrigin, explOrg);
		explOrg[2] += 16;

		//just use the detpack explosion effect
		G_PlayEffect(EFFECT_EXPLOSION_DETPACK, explOrg, puffAngle);

		self->genericValue3 = level.time + Q_irand(2500, 3500);

		G_RadiusDamage(self->r.currentOrigin, self, self->splashDamage, self->splashRadius, self, NULL, MOD_UNKNOWN);

		self->s.time = -1;

		self->genericValue4 = 2;
	}

	if (self->genericValue3 > level.time)
	{ //see if we are freshly dead and should be smoking
		if (self->genericValue2 < level.time)
		{ //is it time yet to spawn another smoke puff?
			VectorSet( puffAngle, 0, 0, 1 );
			VectorCopy(self->r.currentOrigin, smokeOrg);

			smokeOrg[2] += 60;

			G_PlayEffect(EFFECT_SMOKE, smokeOrg, puffAngle);
			self->genericValue2 = level.time + Q_irand(250, 400);
		}
	}

	if (self->activator && self->activator->client && self->activator->inuse)
	{ //handle updating current user
		vec3_t vLen;
		VectorSubtract(self->s.origin, self->activator->client->ps.origin, vLen);
		ownLen = VectorLength(vLen);

		if (!(self->activator->client->pers.cmd.buttons & BUTTON_USE) && self->genericValue1)
		{//RACC - finished attaching the player to the turret.
			self->genericValue1 = 0;
		}

		if ((self->activator->client->pers.cmd.buttons & BUTTON_USE) && !self->genericValue1)
		{//RACC - trigger start to get off the emplaced turret
			self->activator->client->ps.emplacedIndex = 0;
			self->activator->client->ps.saberHolstered = 0;
			self->nextthink = level.time + 50;
			return;
		}
	}

	if ((self->activator && self->activator->client) &&
		(!self->activator->inuse || self->activator->client->ps.emplacedIndex != self->s.number || self->genericValue4 || ownLen > 64))
	{ //get the user off of me then
		self->activator->client->ps.stats[STAT_WEAPONS] &= ~(1<<WP_EMPLACED_GUN);
		//[CoOp]
		//SP Turrets have ammo, remove the ammo from the player
		self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] = 0;
		//[/CoOp]

		oldWeap = self->activator->client->ps.weapon;
		self->activator->client->ps.weapon = self->s.weapon;
		self->s.weapon = oldWeap;
		self->activator->r.ownerNum = ENTITYNUM_NONE;
		self->activator->client->ps.emplacedTime = level.time + 1000;
		self->activator->client->ps.emplacedIndex = 0;
		self->activator->client->ps.saberHolstered = 0;
		self->activator = NULL;

		self->s.activeForcePass = 0;
	}
	else if (self->activator && self->activator->client)
	{ //make sure the user is using the emplaced gun weapon
		self->activator->client->ps.weapon = WP_EMPLACED_GUN;
		self->activator->client->ps.weaponstate = WEAPON_READY;
		//[CoOp]
		//SP Turrets have ammo.  increase the amount of ammo the player has for this turret.
		if(self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] < ammoData[weaponData[WP_EMPLACED_GUN].ammoIndex].max)
		{
			self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex]++;
		}
		//[/CoOp]
	}
	self->nextthink = level.time + 50;
}

//----------------------------------------------------------
void emplaced_gun_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{ //set us up to flash and then explode
	if (self->genericValue4)
	{
		return;
	}

	self->genericValue4 = 1;

	self->s.time = level.time + 3000;

	self->genericValue5 = 0;
}

void SP_emplaced_gun( gentity_t *ent )
{
	const char *name = "models/map_objects/mp/turret_chair.glm";
	vec3_t down;
	trace_t tr;

	//make sure our assets are precached
	RegisterItem( BG_FindItemForWeapon(WP_EMPLACED_GUN) );

	ent->r.contents = CONTENTS_SOLID;
	ent->s.solid = SOLID_BBOX;

	ent->genericValue5 = 0;

	VectorSet( ent->r.mins, -30, -20, 8 );
	VectorSet( ent->r.maxs, 30, 20, 60 );

	VectorCopy(ent->s.origin, down);

	down[2] -= 1024;

	trap_Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_SOLID);

	if (tr.fraction != 1 && !tr.allsolid && !tr.startsolid)
	{
		VectorCopy(tr.endpos, ent->s.origin);
	}

	ent->spawnflags |= 4; // deadsolid

	ent->health = EMPLACED_GUN_HEALTH;

	if (ent->spawnflags & EMPLACED_CANRESPAWN)
	{ //make it somewhat easier to kill if it can respawn
		ent->health *= 0.4;
	}

	ent->maxHealth = ent->health;
	G_ScaleNetHealth(ent);

	ent->genericValue4 = 0;

	ent->takedamage = qtrue;
	ent->pain = emplaced_gun_pain;
	ent->die = emplaced_gun_die;

	// being caught in this thing when it blows would be really bad.
	ent->splashDamage = 80;
	ent->splashRadius = 128;

	// amount of ammo that this little poochie has
	G_SpawnInt( "count", "600", &ent->count );

	G_SpawnFloat( "constraint", "60", &ent->s.origin2[0] );

	ent->s.modelindex = G_ModelIndex( (char *)name );
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 110;

	//so the cgame knows for sure that we're an emplaced weapon
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin( ent, ent->s.origin );
	
	// store base angles for later
	VectorCopy( ent->s.angles, ent->pos1 );
	VectorCopy( ent->s.angles, ent->r.currentAngles );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	ent->think = emplaced_gun_update;
	ent->nextthink = level.time + 50;

	ent->use = emplaced_gun_realuse;

	ent->r.svFlags |= SVF_PLAYER_USABLE;

	ent->s.pos.trType = TR_STATIONARY;

	ent->s.owner = MAX_CLIENTS+1;
	ent->s.shouldtarget = qtrue;
	//ent->s.teamowner = 0;

	trap_LinkEntity(ent);
}

//[CoOp]
/*QUAKED emplaced_eweb (0 0 1) (-12 -12 -24) (12 12 24) INACTIVE FACING INVULNERABLE PLAYERUSE

 INACTIVE cannot be used until used by a target_activate
 FACING - player must be facing relatively in the same direction as the gun in order to use it
 VULNERABLE - allow the gun to take damage
 PLAYERUSE - only the player makes it run its usescript

 count - how much ammo to give this gun ( default 999 )
 health - how much damage the gun can take before it blows ( default 250 )
 delay - ONLY AFFECTS NPCs - time between shots ( default 200 on hardest setting )
 wait - ONLY AFFECTS NPCs - time between bursts ( default 800 on hardest setting )
 splashdamage - how much damage a blowing up gun deals ( default 80 )
 splashradius - radius for exploding damage ( default 128 )

 scripts:
	will run usescript, painscript and deathscript
*/
void SP_emplaced_eweb( gentity_t *ent )
{
	//temp replacing the eweb with an emplaced gun so we can at least get the AI/scripting
	//running for it first.
	SP_emplaced_gun(ent);
}
//[/CoOp]


