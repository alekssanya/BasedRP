// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"
#include "g_db.h"

//[SVN]
//rearraigned repository to make it easier to initially compile.
#include "../../ojpenhanced/ui/jamp/menudef.h"
//#include "../../ui/menudef.h"			// for the voice chats
//[/SVN]

//[CoOp]
extern	qboolean		in_camera;
//[/CoOp]

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

void Cmd_NPC_f( gentity_t *ent );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);

//[AdminSys]
//to allow the /vote command to work for both team votes and normal votes.
void Cmd_TeamVote_f( gentity_t *ent );
//[/AdminSys]

// Required for holocron edits.
//[HolocronFiles]
extern vmCvar_t bot_wp_edit;
//[/HolocronFiles]

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		//[BotTweaks] 
		//[ClientNumFix]
		} else if ( g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT )
		//} else if ( g_entities[cl->ps.clientNum]r.svFlags & SVF_BOT )
		//[/ClientNumFix]
		{//make fake pings for bots.
			ping = Q_irand(50, 150);
		//[/BotTweaks]
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		Com_sprintf (entry, sizeof(entry),
			//[ExpSys]
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i ", level.sortedClients[i],
			//" %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			//[/ExpSys]
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			//[ExpSys]
			cl->ps.persistant[PERS_CAPTURES],
			//cl->ps.persistant[PERS_CAPTURES]);
			(int) cl->sess.skillPoints);
			//[/ExpSys]
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}

int roll_dice(max_value) {
	int result = rand() % (max_value + 1);
	while (result == 0) {
		result = rand() % (max_value + 1);
	}

	return result;
}

#define MAX_EMOTE_CATEGORIES 5

const char anim_headers[MAX_EMOTE_CATEGORIES][50] = {
	"Body",
	"Movement",
	"Blaster",
	"Saber",
	"Force"
};

#define BROADCAST_DISTANCE 999999999
#define VOICE_DISTANCE 600
#define VOICE_DISTANCE_LONG 2000
#define VOICE_DISTANCE_LOW 65
#define SHOUT_DISTANCE 1500
#define ACTION_DISTANCE 1200
#define ACTION_DISTANCE_LOW 200
#define ACTION_DISTANCE_LONG 2000


qboolean StringIsInteger(const char* s) {
	int			i = 0, len = 0;
	qboolean	foundDigit = qfalse;

	for (i = 0, len = strlen(s); i < len; i++)
	{
		if (!isdigit(s[i]))
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
}

void Cmd_Roll_f(gentity_t* ent) {
	char arg1[MAX_STRING_CHARS];

	if (trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"^1�������������: ^2/roll ^3<����. �����>.\n^1������: ^2/roll ^320\n\"");
		return;
	}

	trap_Argv(1, arg1, sizeof(arg1));

	if (StringIsInteger(arg1) == qfalse) {
		trap_SendServerCommand(ent - g_entities, "print \"�������� ������ ���� ����� ������.\n\"");
		return;
	}

	int max_value = atoi(arg1);

	if (max_value < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"������������ �������� ������ ���� �� ����� 2.\n\"");
		return;
	}

	int result = roll_dice(max_value);

	trap_SendServerCommand(-1, va("chat \"^3<������ �����> %s^2 �������� ^3%d^2 �� ^3%d\n\"",
		ent->client->pers.netname, result, max_value));

	return;
}



#define MAX_WORDED_EMOTES 52 //132 ��������� 52 � ������
//alex: type for storing worde animations wo use with the emote system
typedef struct worded_animation_s {
	const char* animation_name;
	int			animation_code;
	const char* animation_category;
} worded_animation_t;

const worded_animation_t animations[MAX_WORDED_EMOTES] = {
	{"aim",				BOTH_STAND4TOATTACK2,		"Blaster"	},
	{"aim2",			BOTH_STAND5TOAIM,			"Blaster"	},
	{"aim3",			BOTH_ATTACK2,				"Blaster"	},
	{"aim4",			TORSO_WEAPONIDLE4,			"Blaster"	},
	{"aim5",			TORSO_WEAPONIDLE3,			"Blaster"	},
	{"beg",				BOTH_KNEES1,				"Body"		},
	{"beg2",			BOTH_DEATH14_SITUP,			"Body"		},
	{"beg3",			BOTH_CHOKE2,				"Body"		},
	{"bow",				BOTH_BOW,					"Body"		},
	{"choked",			BOTH_CHOKE3,				"Body"		},
	{"commlinkdown",	BOTH_TALKCOMM1STOP,			"Body"		},
	{"commlinkup",		BOTH_TALKCOMM1START,		"Body"		},
	{"cover",			BOTH_DODGE_HOLD_FL,			"Movement"	},
	{"cuffed",			BOTH_STAND4,				"Body"		},
	{"die",				BOTH_DEATH14_UNGRIP,		"Body"		},
	{"drainloop",		BOTH_FORCE_DRAIN_GRAB_HOLD,	"Force"		},
	{"fear",			BOTH_SONICPAIN_HOLD,		"Body"		},
	{"flourish",		BOTH_SHOWOFF_FAST,			"Saber"		},
	{"flourish2",		BOTH_SHOWOFF_MEDIUM,		"Saber"		},
	{"flourish3",		BOTH_SHOWOFF_STRONG,		"Saber"		},
	{"flourish4",		BOTH_SHOWOFF_DUAL,			"Saber"		},
	{"flourish5",		BOTH_SHOWOFF_STAFF,			"Saber"		},
	{"forcechoke",		BOTH_FORCEGRIP3,			"Force"		},
	{"forcelightning",	BOTH_FORCELIGHTNING_HOLD,	"Force"		},
	{"heroic",			BOTH_STAND5TOSTAND8,		"Body"		},
	{"holddetonator",	TORSO_WEAPONREADY10,		"Body"		},
	{"hug",				BOTH_HUGGER1,				"Body"		},
	{"kneel",			BOTH_CROUCH3,				"Body"		},
	{"lean",			BOTH_STAND10,				"Body"		},
	{"leantable",		BOTH_STAND7TOSTAND8,		"Body"		},
	{"meditate",		BOTH_STAND5TOSIT2,			"Body"		},
	{"mindtrick",		BOTH_MINDTRICK1,			"Force"		},
	{"point",			BOTH_STAND5TOAIM,			"Body"		},
	{"pressbutton",		BOTH_BUTTON_HOLD,			"Body"		},
	{"saberthrow",		BOTH_SABERTHROW1START,		"Saber"		},
	{"sit",				BOTH_SIT2,					"Body"		},
	{"sit2",			BOTH_SIT3,					"Body"		},
	{"sit3",			BOTH_SIT6,					"Body"		},
	{"sitpilot",		BOTH_GUNSIT1,				"Body"		},
	{"sleep",			BOTH_SLEEP1,				"Body"		},
	{"sneak",			BOTH_CROUCH1,				"Movement"	},
	{"spreadlegs",		BOTH_KNEEL_TO_STAND,		"Body"		},
	{"surrender",		TORSO_SURRENDER_START,		"Body"		},
	{"tossleft",		BOTH_TOSS1,					"Force"		},
	{"tossright",		BOTH_TOSS2,					"Force"		},
	{"type",			BOTH_CONSOLE1,				"Body"		},
	{"victory",			BOTH_WINGS_CLOSE,			"Saber"		},
	{"victory2",		BOTH_DEATH14_UNGRIP,		"Saber"		},
	{"victory3",		BOTH_DEATH14_SITUP,			"Saber"		},
	{"victory4",		BOTH_VICTORY_STAFF,			"Saber"		},
	{"wave",			BOTH_SILENCEGESTURE1,		"Body"		},
	{"windy",			BOTH_WIND,					"Movement"	},
/*	{"force",			BOTH_USEFORCE,				"Force"},
	{"forcecasual",		BOTH_FORCECASUAL,			"Force"		},
	{"anikata2",		BOTH_ANAKINKATA2,			"Saber"		},
	{"anikata3",		BOTH_ANAKINKATA3,			"Saber"		},
	{"ataru",			BOTH_ATARU,					"Saber"		},
	{"bump",			BOTH_FISTBUMP,				"Body"		},
	{"carry",			BOTH_CARRY,					"Movement"	},
	{"cross",			BOTH_ARMSCROSSED,			"Body"		},
	{"cufffront",		BOTH_CUFFEDFRONT,			"Body"		},
	{"cuffknees",		BOTH_CUFFEDKNEES,			"Body"		},
	{"cup",				BOTH_COFFEE_IDLE,			"Body"		},
	{"cupsip",			BOTH_COFFEE_SIP,			"Body"		},
	{"datapad",			BOTH_DATAPAD,				"Body"		},
	{"datapad2",		BOTH_DATAPAD2,				"Body"		},
	{"djemso",			BOTH_DJEMSO,				"Saber"		},
	{"djemso2",			BOTH_DJEMSO2,				"Saber"		},
	{"drink",			BOTH_COFFEE_SIP,			"Body"		},
	{"facepalm",		BOTH_FACEPALM,				"Body"		},
	{"facepalm2",		BOTH_FACEPALM2,				"Body"		},
	{"guard",			BOTH_GUARD,					"Saber"		},
	{"gunspin1",		BOTH_GUNSPINB,				"Blaster"	},
	{"gunspin2",		BOTH_GUNSPINF,				"Blaster"	},
	{"gunspin3",		BOTH_GUNSPINS,				"Blaster"	},
	{"handsback",		BOTH_HANDSBACK,				"Body"		},
	{"handsfront",		BOTH_HANDSFRONT,			"Body"		},
	{"handstand",		BOTH_HANDSTAND,				"Body"		},
	{"handstand2",		BOTH_HANDSTAND2,			"Body"		},
	{"headhold",		BOTH_HANDSHEAD,				"Body"		},
	{"helpedup",		BOTH_HELPEDUP,				"Body"		},
	{"helpup",			BOTH_HELPUP,				"Body"		},
	{"hips",			BOTH_HANDSHIPS,				"Body"		},
	{"hips2",			BOTH_HANDSHIPS2,			"Body"		},
	{"holdobject",		BOTH_OBJECT,				"Body"		},
	{"hurt",			BOTH_HURT,					"Body"		},
	{"hurt2",			BOTH_HURT2,					"Body"		},
	{"idle",			BOTH_SABERIDLE,				"Saber"		},
	{"jarkai",			BOTH_JARKAI,				"Saber"		},
	{"jarkai2",			BOTH_JARKAIREVERSE,			"Saber"		},
	{"juyo",			BOTH_JUYO,					"Saber"		},
	{"leanback",		BOTH_LEANWALL,				"Body"		},
	{"leanfront",		BOTH_LEANFRONT,				"Body"		},
	{"makashi",			BOTH_MAKASHI,				"Saber"		},
	{"meditate2",		BOTH_MEDITATION2,			"Force"		},
	{"meditate3",		BOTH_MEDITATEFORCE,			"Force"		},
	{"niman",			BOTH_NIMAN,					"Saber"		},
	{"pistol",			BOTH_PISTOLREADY,			"Blaster"	},
	{"ponder",			BOTH_PONDER,				"Body"		},
	{"ponder2",			BOTH_PONDER2,				"Body"		},
	{"pushup",			BOTH_PUSHUP,				"Body"		},
	{"quickdraw",		BOTH_QUICKDRAW,				"Blaster"	},
	{"quickdraw2",		BOTH_QUICKDRAW2,			"Blaster"	},
	{"read",			BOTH_READ,					"Body"		},
	{"relax",			BOTH_LAYDOWN,				"Body"		},
	{"saberdraw1",		BOTH_SABERDRAW1,			"Saber"		},
	{"saberdraw2",		BOTH_SABERDRAW2,			"Saber"		},
	{"saberdraw3",		BOTH_SABERDRAW3,			"Saber"		},
	{"saberdraw4",		BOTH_SABERDRAW4,			"Saber"		},
	{"saberdraw5",		BOTH_SABERDRAW5,			"Saber"		},
	{"saberpoint",		BOTH_SABERPOINT,			"Saber"		},
	{"saberpoint2",		BOTH_SABERPOINT2,			"Saber"		},
	{"salute",			BOTH_SALUTE,				"Body"		},
	{"scratch",			BOTH_HEADSCRATCH,			"Body"		},
	{"shien",			BOTH_SHIEN,					"Saber"		},
	{"shien2",			BOTH_SHIEN2,				"Saber"		},
	{"shien3",			BOTH_SHIEN3,				"Saber"		},
	{"shiicho",			BOTH_SHIICHO,				"Saber"		},
	{"sit4",			BOTH_SITARMS,				"Body"		},
	{"sit5",			BOTH_SITCROSS,				"Body"		},
	{"sit6",			BOTH_SITCROSS2,				"Body"		},
	{"sit7",			BOTH_SITLEAN,				"Body"		},
	{"sitfeet",			BOTH_SITFEET,				"Body"		},
	{"sitpalm",			BOTH_SITPALM,				"Body"		},
	{"sitpalm2",		BOTH_SITPALM2,				"Body"		},
	{"situp",			BOTH_SITUP,					"Body"		},
	{"soresu",			BOTH_SORESU,				"Saber"		},
	{"soresu2",			BOTH_SORESU2,				"Saber"		},
	{"stance",			BOTH_SABERSTANCE,			"Saber"		},
	{"stance2",			BOTH_SABERSTANCE2,			"Saber"		},
	{"stance3",			BOTH_SABERSTANCE3,			"Saber"		},
	{"anikata",			BOTH_ANAKINKATA,			"Saber"		},*/
};

void print_table_horizontal_line(gentity_t* ent) {
	trap_SendServerCommand(ent - g_entities, "print \" ^9======================================\n\"");
	return;
}

void print_heading_text_row(gentity_t* ent, char header_text[MAX_STRING_CHARS]) {
	//34 characters left for space and text
	int length = strlen(header_text);

	char emote_row[MAX_STRING_CHARS] = " ";

	strcat(emote_row, "^9||^3");

	qboolean can_be_centered;

	if (length % 2 == 0) {
		can_be_centered = qtrue;
	}
	else {
		can_be_centered = qfalse;
	}

	int number_of_spaces = 34 - length;
	int left_spaces;
	int right_spaces;

	if (can_be_centered) {
		left_spaces = number_of_spaces / 2;
		right_spaces = left_spaces;
	}
	else {
		//it will be rounded down
		left_spaces = number_of_spaces / 2;
		right_spaces = left_spaces + 1;
	}

	//take care of the space to the left of the writing
	for (int i = 0; i < left_spaces; i++) {
		strcat(emote_row, " ");
	}

	strcat(emote_row, header_text);

	//take care of the space to the right of the writing
	for (int i = 0; i < right_spaces; i++) {
		strcat(emote_row, " ");
	}

	strcat(emote_row, "^9||\n");

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", emote_row));

	return;
}

void print_header(gentity_t* ent, char text[MAX_STRING_CHARS]) {
	print_table_horizontal_line(ent);
	print_heading_text_row(ent, text);
	print_table_horizontal_line(ent);
}

int get_max_spaces_right(char text[MAX_STRING_CHARS]) {
	return 33 - strlen(text);
}

void print_row(gentity_t* ent, char text[MAX_STRING_CHARS]) {
	char emote_row[MAX_STRING_CHARS] = " ";

	strcat(emote_row, "^9|| ^3");

	strcat(emote_row, text);

	for (int i = 0; i < get_max_spaces_right(text); i++) {
		strcat(emote_row, " ");
	}

	strcat(emote_row, "^9||\n");

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", emote_row));

	return;
}


void show_animation_list(gentity_t* ent, int beginning_index, int end_index) {
	for (int i = beginning_index; i < end_index; i++) {
		print_header(ent, anim_headers[i]);
		for (int j = 0; j < MAX_WORDED_EMOTES; j++) {
			//alex: if animation is in that category
			if (Q_stricmp(animations[j].animation_category, anim_headers[i]) == 0) {
				print_row(ent, animations[j].animation_name);
			}
		}
	}
	//alex: end the table
	print_table_horizontal_line(ent);

	return;
}

// alex: plays an animation from anims.h by id OR a word (look for animation_t)
void Cmd_Emote_f(gentity_t* ent)
{
	char arg[MAX_TOKEN_CHARS] = { 0 };
	char anim_id[100] = { 0 };

	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent - g_entities, va(
			"print \"�������������: /anim <id �������� �� 0 �� %d>\n\"", MAX_ANIMATIONS - 1));
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	Q_strncpyz(anim_id, arg, sizeof(anim_id));

	// �������� ������ �������
	if (Q_stricmp(anim_id, "list") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv(2, arg, sizeof(arg));
			int page = atoi(arg);

			if (page > -1) {
				show_animation_list(ent, page, MAX_EMOTE_CATEGORIES);
				return;
			}
			else {
				trap_SendServerCommand(ent - g_entities, "print \"����� �������� ������� �� ����������.\n\"");
				return;
			}
		}
		else {
			show_animation_list(ent, 0, 3);
			trap_SendServerCommand(ent - g_entities, "print \"^3�������� 1/2. ����� ���������� ��������� ��������, �������� /anim list 2\n\"");
			return;
		}
	}
	else if (Q_stricmp(anim_id, "stop") == 0 && !(ent->client->pers.player_statuses & (1 << 6))) {
		ent->client->pers.player_statuses &= ~(1 << 1); // ������ ���� "������"
		ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
		ent->client->ps.forceHandExtendTime = 0;
		ent->client->ps.forceDodgeAnim = 0;
		return;
	}

	int anim_id_int = atoi(anim_id);

	if (anim_id_int < 0 || anim_id_int >= MAX_ANIMATIONS) {
		trap_SendServerCommand(ent - g_entities, va(
			"print \"ID �������� ������ ���� �� 0 �� %d\n\"", MAX_ANIMATIONS - 1));
		return;
	}

	if (ent->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN) {
		trap_SendServerCommand(ent->s.number, "print \"������ ������������ ������, ����� �� ����� � ���.\n\"");
		return;
	}
	
	if (StringIsInteger(anim_id))
	{
		// �������� �� ID
		if (anim_id_int > 0 && anim_id_int < MAX_ANIMATIONS) {
			ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
			ent->client->ps.forceDodgeAnim = anim_id_int;
			ent->client->ps.forceHandExtendTime = level.time + 1000000;
			ent->client->pers.player_statuses |= (1 << 1);
			return;
		}
	}
	else {
		// ����� �� �������� ��������
		for (int i = 0; i < MAX_WORDED_EMOTES; i++) {
			if (Q_stricmp(anim_id, animations[i].animation_name) == 0) {
				ent->client->ps.forceHandExtend = HANDEXTEND_TAUNT;
				ent->client->ps.forceDodgeAnim = animations[i].animation_code;
				ent->client->ps.forceHandExtendTime = level.time + 1000000;
				ent->client->pers.player_statuses |= (1 << 1); // ���������� ���� "� ������"
				return;
			}
		}
	}
	return;
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if (!HasAdminRights(ent)) {
		SendPrint(ent, "������������ ���� ��� ���������� �������");
		return qfalse;
	}
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEALIVE")));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( (unsigned char) *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
//[VisualWeapons]
extern qboolean OJP_AllPlayersHaveClientPlugin(void);
//[/VisualWeapons]
void Cmd_Give_f (gentity_t *cmdent, int baseArg)
{
	char		name[MAX_TOKEN_CHARS];
	gentity_t	*ent;
	//gitem_t		*it; // ensiform - removed
	int			i;
	qboolean	give_all;
	//gentity_t		*it_ent; // ensiform - removed
	//trace_t		trace; // ensiform - removed
	char		arg[MAX_TOKEN_CHARS];

	if ( !CheatsOk( cmdent ) ) {
		return;
	}

	if (baseArg)
	{
		char otherindex[MAX_TOKEN_CHARS];

		trap_Argv( 1, otherindex, sizeof( otherindex ) );

		if (!otherindex[0])
		{
			Com_Printf("giveother requires that the second argument be a client index number.\n");
			return;
		}

		i = atoi(otherindex);

		if (i < 0 || i >= MAX_CLIENTS)
		{
			Com_Printf("%i is not a client index\n", i);
			return;
		}

		ent = &g_entities[i];

		if (!ent->inuse || !ent->client)
		{
			Com_Printf("%i is not an active client\n", i);
			return;
		}
	}
	else
	{
		ent = cmdent;
	}

	trap_Argv( 1+baseArg, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	//[CoOp]
	/*if (give_all)
	{
		i = 0;
		while (i < HI_NUM_HOLDABLE)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			i++;
		}
		i = 0;
	}*/
	//[/CoOp]

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	//[CoOp]
	if (give_all || Q_stricmp( name, "inventory") == 0)
	{
		i = 0;
		for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ ) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
		}
	}

	if (give_all || Q_stricmp( name, "force") == 0)
	{
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			ent->client->ps.fd.forcePower = atoi(arg);
			if (ent->client->ps.fd.forcePower > 250) {
				ent->client->ps.fd.forcePower = 250;
			}
		}
		else {

			if(ent->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3)
			{
			ent->client->ps.fd.forcePower=250;
			ent->client->ps.fd.forcePowerMax=250;
			ent->client->ps.stats[STAT_MAX_DODGE] = 250;
			}
			else if(ent->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2)
			{
			ent->client->ps.fd.forcePower=150;
			ent->client->ps.fd.forcePowerMax=150;
			ent->client->ps.stats[STAT_MAX_DODGE] = 150;

			}
			else if(ent->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1)
			{
			ent->client->ps.fd.forcePower=100;
			ent->client->ps.fd.forcePowerMax=100;
			ent->client->ps.stats[STAT_MAX_DODGE] = 100;
			}
			else if (ent->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
			{ //recharge cloak
			ent->client->ps.fd.forcePower=100;
			ent->client->ps.fd.forcePowerMax=100;
			ent->client->ps.stats[STAT_MAX_DODGE] = 100;
			}
			else 
			{
			ent->client->ps.fd.forcePower=25;
			ent->client->ps.fd.forcePowerMax=25;
			ent->client->ps.stats[STAT_MAX_DODGE] = 25;
			}
			
		}
		if (!give_all)
			return;
	}
	//[/CoOp]

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON+1))  - ( 1 << WP_NONE );
		//[VisualWeapons]
		//update the weapon stats for this player since they have changed.
		if(OJP_AllPlayersHaveClientPlugin())
		{//don't send the weapon updates if someone isn't able to process this new event type (IE anyone without
			//the OJP client plugin)
			G_AddEvent(ent, EV_WEAPINVCHANGE, ent->client->ps.stats[STAT_WEAPONS]);
		}
		//[/VisualWeapons]
		if (!give_all)
			return;
	}
	
	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 )
	{
		trap_Argv( 2+baseArg, arg, sizeof( arg ) );
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi(arg));

		//[VisualWeapons]
		//update the weapon stats for this player since they have changed.
		if(OJP_AllPlayersHaveClientPlugin())
		{//don't send the weapon updates if someone isn't able to process this new event type (IE anyone without
			//the OJP client plugin)
			G_AddEvent(ent, EV_WEAPINVCHANGE, ent->client->ps.stats[STAT_WEAPONS]);
		}
		//[/VisualWeapons]
		return;
	}

	//[CoOp]
	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = AMMO_BLASTER ; i < AMMO_MAX ; i++ ) {
			if ( num > ammoData[i].max )
				num = ammoData[i].max;
			Add_Ammo( ent, i, num );
		}
		if (!give_all)
			return;
	}

	/*if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}*/
	//[/CoOp]

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3+baseArg) {
			trap_Argv( 2+baseArg, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
			return;
	}

	/*
	// ensiform - Not used in basejka or OJP so why keep?
	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}*/
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	/*
	if ( !CheatsOk( ent ) ) {
		return;
	}
	*/
	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	//[CoOp]
	if (in_camera)
		return;
	//[/CoOp]

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_TeamTask_f

From TA.
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}

//[AdminSys]
extern void AddIP( char *str );
extern vmCvar_t	g_autoKickTKSpammers;
extern vmCvar_t	g_autoBanTKSpammers;
void G_CheckTKAutoKickBan( gentity_t *ent ) 
{
	if ( !ent || !ent->client || ent->s.number >= MAX_CLIENTS )
	{
		return;
	}

	if ( g_autoKickTKSpammers.integer > 0
		|| g_autoBanTKSpammers.integer > 0 )
	{
		ent->client->sess.TKCount++;
		if ( g_autoBanTKSpammers.integer > 0
			&& ent->client->sess.TKCount >= g_autoBanTKSpammers.integer )
		{
			if ( ent->client->sess.IPstring[0] )
			{//ban their IP
				AddIP( ent->client->sess.IPstring );
			}

			trap_SendServerCommand( -1, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME_ADMIN", "TKBAN")) );
			//Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", ent->s.number );
			//Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", ent->client->pers.netname );
			//trap_SendConsoleCommand( EXEC_INSERT, va( "banClient %d\n", ent->s.number ) );
			trap_SendConsoleCommand( EXEC_INSERT, va( "clientkick %d\n", ent->s.number ) );
			return;
		}
		if ( g_autoKickTKSpammers.integer > 0
			&& ent->client->sess.TKCount >= g_autoKickTKSpammers.integer )
		{
			trap_SendServerCommand( -1, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME_ADMIN", "TKKICK")) );
			//Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", ent->s.number );
			//Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick \"%s\"\n", ent->client->pers.netname );
			trap_SendConsoleCommand( EXEC_INSERT, va( "clientkick %d\n", ent->s.number ) );
			return;
		}
		//okay, not gone (yet), but warn them...
		if ( g_autoBanTKSpammers.integer > 0
			&& (g_autoKickTKSpammers.integer <= 0 || g_autoBanTKSpammers.integer < g_autoKickTKSpammers.integer) )
		{//warn about ban
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME_ADMIN", "WARNINGTKBAN")) );
		}
		else if ( g_autoKickTKSpammers.integer > 0 )
		{//warn about kick
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME_ADMIN", "WARNINGTKKICK")) );
		}
	}
}
//[/AdminSys]


/*
=================
Cmd_Kill_f
=================
*/
//[AdminSys]
extern vmCvar_t	g_autoKickKillSpammers;
extern vmCvar_t	g_autoBanKillSpammers;
//[/AdminSys]
void Cmd_Kill_f( gentity_t *ent ) {
	//[BugFix41]
	//if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->tempSpectate >= level.time ) {
	//[/BugFix41]
		return;
	}
	if (ent->health <= 0)
		return;
	//[CoOp]
	if (in_camera)
		return;
	//[/CoOp]

	if ((g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

//[AdminSys]
	if ( g_autoKickKillSpammers.integer > 0
		|| g_autoBanKillSpammers.integer > 0 )
	{
		ent->client->sess.killCount++;
		if ( g_autoBanKillSpammers.integer > 0
			&& ent->client->sess.killCount >= g_autoBanKillSpammers.integer )
		{
			if ( ent->client->sess.IPstring[0] != '\0' )
			{//ban their IP
				AddIP( ent->client->sess.IPstring );
			}

			trap_SendServerCommand( -1, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME_ADMIN", "SUICIDEBAN")) );
			//Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", ent->s.number );
			//Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", ent->client->pers.netname );
			//trap_SendConsoleCommand( EXEC_INSERT, va( "banClient %d\n", ent->s.number ) );
			trap_SendConsoleCommand( EXEC_INSERT, va( "clientkick %d\n", ent->s.number ) );
			return;
		}
		if ( g_autoKickKillSpammers.integer > 0
			&& ent->client->sess.killCount >= g_autoKickKillSpammers.integer )
		{
			trap_SendServerCommand( -1, va("print \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME_ADMIN", "SUICIDEKICK")) );
			//Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", ent->s.number );
			//Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", ent->client->pers.netname );
			trap_SendConsoleCommand( EXEC_INSERT, va( "clientkick %d\n", ent->s.number ) );
			return;
		}
		//okay, not gone (yet), but warn them...
		if ( g_autoBanKillSpammers.integer > 0
			&& (g_autoKickKillSpammers.integer <= 0 || g_autoBanKillSpammers.integer < g_autoKickKillSpammers.integer) )
		{//warn about ban
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME_ADMIN", "WARNINGSUICIDEBAN")) );
		}
		else if ( g_autoKickKillSpammers.integer > 0 )
		{//warn about kick
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME_ADMIN", "WARNINGSUICIDEKICK")) );
		}
	}
//[/AdminSys]
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

//[ClientNumFix]
gentity_t *G_GetDuelWinner(gclient_t *client)
{
	int i;
	gentity_t *wEnt;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wEnt = &g_entities[i];
		
		if (wEnt->client && wEnt->client != client && wEnt->client->pers.connected == CON_CONNECTED && wEnt->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return wEnt;
		}
	}

	return NULL;
}
#if 0
gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}
#endif
//[/ClientNumFix]

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if (g_gametype.integer == GT_SIEGE)
	{ //don't announce these things in siege
		return;
	}

	//[CoOp]
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
		} else if ( client->sess.sessionTeam == TEAM_FREE ) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	} else {
		if ( client->sess.sessionTeam == TEAM_RED ) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEREDTEAM")) );
		} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBLUETEAM")));
		} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
		} else if ( client->sess.sessionTeam == TEAM_FREE ) {
			if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
			{
				/*
				gentity_t *currentWinner = G_GetDuelWinner(client);

				if (currentWinner && currentWinner->client)
				{
					trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
					currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), client->pers.netname));
				}
				else
				{
					trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
					client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
				}
				*/
				//NOTE: Just doing a vs. once it counts two players up
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
			}
		}
	}
	//[/CoOp]

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  client - &level.clients[0],
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}

qboolean G_PowerDuelCheckFail(gentity_t *ent)
{
	int			loners = 0;
	int			doubles = 0;

	if (!ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE)
	{
		return qtrue;
	}

	G_PowerDuelCount(&loners, &doubles, qfalse);

	if (ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
	{
		return qtrue;
	}

	if (ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
SetTeam
=================
*/
//[AdminSys]
int G_CountHumanPlayers( int team );
int G_CountBotPlayers( int team );
extern int OJP_PointSpread(void);
//[/AdminSys]
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
void SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	//[CoOp]
	} else if ( g_gametype.integer == GT_SINGLE_PLAYER ) 
	{//players spawn on NPCTEAM_PLAYER
		team = NPCTEAM_PLAYER;
	//[/CoOp]
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
				if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					team = TEAM_BLUE;
				}
				else
				{
					team = TEAM_RED;
				}
			}
			else
			{
			*/
				//[AdminSys]
				team = PickTeam( clientNum, (ent->r.svFlags & SVF_BOT) );
				//team = PickTeam( clientNum );
				//[/AdminSys]
			//}
		}

		if ( g_teamForceBalance.integer>1 && !g_trueJedi.integer ) 
		{//racc - override player's choice if the team balancer is in effect.
			int		counts[TEAM_NUM_TEAMS];

			//[ClientNumFix]
			counts[TEAM_BLUE] = TeamCount( ent-g_entities, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent-g_entities, TEAM_RED );
			//counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			//counts[TEAM_RED] = TeamCount( ent->client->ps.clientNUm, TEAM_RED );
			//[/ClientNumFix]

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
				}
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/
				{
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
				}
				return; // ignore the request
			}
			//[AdminSys]
			//balance based on team score
			if(g_teamForceBalance.integer >= 3 && g_gametype.integer != GT_SIEGE)
			{//check the scores 
				if(level.teamScores[TEAM_BLUE] - OJP_PointSpread() >= level.teamScores[TEAM_RED] 
					&& counts[TEAM_BLUE] >= counts[TEAM_RED] && team == TEAM_BLUE)
				{//blue team is ahead, don't add more players to that team
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
					return;
				}
				else if(level.teamScores[TEAM_RED] - OJP_PointSpread() >= level.teamScores[TEAM_BLUE] 
					&& counts[TEAM_RED] > counts[TEAM_BLUE] && team == TEAM_RED)
				{//red team is ahead, don't add more players to that team
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
					return;
				}
			}

			//teams have to be balanced in this situation, check for human/bot team balance. 
			if(g_teamForceBalance.integer == 4)
			{//check for human/bot 
				int BotCount[TEAM_NUM_TEAMS];
				int HumanCount = G_CountHumanPlayers( -1 );

				BotCount[TEAM_BLUE] = G_CountBotPlayers( TEAM_BLUE );
				BotCount[TEAM_RED] = G_CountBotPlayers( TEAM_RED );

				if(HumanCount < 2)
				{//don't worry about this check then since there's not enough humans to care.
				}
				else if(BotCount[TEAM_RED] - BotCount[TEAM_BLUE] > 1 
					&& !(ent->r.svFlags & SVF_BOT) && team == TEAM_BLUE)
				{//red team has too many bots, humans can't join blue
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
					return;
				}
				else if(BotCount[TEAM_BLUE] - BotCount[TEAM_RED] > 1
					&& !(ent->r.svFlags & SVF_BOT) && team == TEAM_RED)
				{//blue team has too many bots, humans can't join red
					//[ClientNumFix]
					trap_SendServerCommand( ent-g_entities, 
					//trap_SendServerCommand( ent->client->ps.clientNum, 
					//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
					return;
				}
			}
			//[/AdminSys]

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
			if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBELIGHT")) );
				return;
			}
			if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEDARK")) );
				return;
			}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}
	
	//[BugFix41]
	oldTeam = client->sess.sessionTeam;
	//[/BugFix41]

	if (g_gametype.integer == GT_SIEGE)
	{
		if (client->tempSpectate >= level.time &&
			team == TEAM_SPECTATOR)
		{ //sorry, can't do that.
			return;
		}
		
		//[BugFix41]
		if ( team == oldTeam && team != TEAM_SPECTATOR ) {
			return;
		}
		//[/BugFix41]

		client->sess.siegeDesiredTeam = team;
		//oh well, just let them go.
		/*
		if (team != TEAM_SPECTATOR)
		{ //can't switch to anything in siege unless you want to switch to being a fulltime spectator
			//fill them in on their objectives for this team now
			trap_SendServerCommand(ent-g_entities, va("sb %i", client->sess.siegeDesiredTeam));

			trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time the round begins.\n\"") );
			return;
		}
		*/
		if (client->sess.sessionTeam != TEAM_SPECTATOR &&
			team != TEAM_SPECTATOR)
		{ //not a spectator now, and not switching to spec, so you have to wait til you die.
			//trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time you respawn.\n\"") );
			qboolean doBegin;
			if (ent->client->tempSpectate >= level.time)
			{
				doBegin = qfalse;
			}
			else
			{
				doBegin = qtrue;
			}

			if (doBegin)
			{
				// Kill them so they automatically respawn in the team they wanted.
				if (ent->health > 0)
				{
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die( ent, ent, ent, 100000, MOD_TEAM_CHANGE ); 
				}
			}

			if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
			{
				SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qfalse);
			}

			return;
		}
	}

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_DUEL)
		&& level.numNonSpectatorClients >= 2 )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( (g_gametype.integer == GT_POWERDUEL)
		&& (level.numPlayingClients >= 3 || G_PowerDuelCheckFail(ent)) )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer )
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	//[BugFix41]
	// moved this up above the siege check
	//oldTeam = client->sess.sessionTeam;
	//[/BugFix41]
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	//If it's siege then show the mission briefing for the team you just joined.
//	if (g_gametype.integer == GT_SIEGE && team != TEAM_SPECTATOR)
//	{
//		trap_SendServerCommand(clientNum, va("sb %i", team));
//	}

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		if ( (g_gametype.integer != GT_DUEL) || (oldTeam != TEAM_SPECTATOR) )	{//so you don't get dropped to the bottom of the queue for changing skins, etc.
			client->sess.spectatorTime = level.time;
		}
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (oldTeam != TEAM_SPECTATOR)
	{
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	if (!g_preventTeamBegin)
	{
		ClientBegin( clientNum, qfalse );
	}
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
//[BugFix38]
extern void G_LeaveVehicle( gentity_t *ent, qboolean ConCheck );
//[/BugFix38]
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	//[BugFix38]
	G_LeaveVehicle( ent, qfalse ); // clears m_iVehicleNum as well
	//ent->client->ps.m_iVehicleNum = 0;
	//[/BugFix38]
	ent->client->ps.viewangles[ROLL] = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = 0;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
	//[DuelSys]
	ent->client->ps.duelInProgress = qfalse; // MJN - added to clean it up a bit.
	//[/DuelSys]
	//[BugFix38]
	//[OLDGAMETYPES]
	ent->client->ps.isJediMaster = qfalse; // major exploit if you are spectating somebody and they are JM and you reconnect
	//[/OLDGAMETYPES]

		if(ent->client->skillLevel[SK_JETPACK] == FORCE_LEVEL_3 || ent->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_3 )
		{
			ent->client->ps.jetpackFuel = 250;
		}
		else if(ent->client->skillLevel[SK_JETPACK] == FORCE_LEVEL_2 || ent->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_2 )		
		{
			ent->client->ps.jetpackFuel = 150;
		}
		else if(ent->client->skillLevel[SK_JETPACK] == FORCE_LEVEL_1 || ent->client->skillLevel[SK_FLAMETHROWER] == FORCE_LEVEL_1 )
		{
			ent->client->ps.jetpackFuel = 100;
		}
		else if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK) || ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_FLAMETHROWER))
		{	
			ent->client->ps.jetpackFuel = 100;			
		}
		else
		{
			ent->client->ps.jetpackFuel = 0;
		}



		if(ent->client->skillLevel[SK_CLOAK] == FORCE_LEVEL_3 || ent->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_3 || ent->client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_3 || ent->client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_3)
		{
			ent->client->ps.cloakFuel = 250;
		}	
		else if(ent->client->skillLevel[SK_CLOAK] == FORCE_LEVEL_2 || ent->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_2 || ent->client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_2 || ent->client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_2)
		{
			ent->client->ps.cloakFuel = 150;
		}		
		else if(ent->client->skillLevel[SK_CLOAK] == FORCE_LEVEL_1 || ent->client->skillLevel[SK_ELECTROSHOCKER] == FORCE_LEVEL_1 || ent->client->skillLevel[SK_SPHERESHIELD] == FORCE_LEVEL_1 || ent->client->skillLevel[SK_OVERLOAD] == FORCE_LEVEL_1)
		{
			ent->client->ps.cloakFuel = 100;
		}
		else if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_CLOAK) || ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_ELECTROSHOCKER) || ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SPHERESHIELD) || ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_OVERLOAD))
		{	
			ent->client->ps.cloakFuel = 100;			
		}		
		else
		{
			ent->client->ps.cloakFuel = 0;
		}

		

		if(ent->client->skillLevel[SK_HEALTH] == FORCE_LEVEL_3)
		{
		ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 999;
		}
		else if(ent->client->skillLevel[SK_HEALTH] == FORCE_LEVEL_2)
		{
		ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 500;

		}
		else if(ent->client->skillLevel[SK_HEALTH] == FORCE_LEVEL_1)
		{
		ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 250;
		}
		else 
		{
		if (g_gametype.integer == GT_SIEGE && ent->client->siegeClass != -1)
		{
			siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];
			ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 100;

			if (scl->maxhealth)
			{
				ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = scl->maxhealth;
			}
		}
		else
		{
		ent->health = ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 100;
		}
		}
	
	
		
 // so that you don't keep dead angles if you were spectating a dead person
	//[/BugFix38]
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	//[ExpSys]
	//changed this so that we can link to this function thru the "forcechanged" behavior with its new design.
	if ( trap_Argc() < 2 ) {
	//if ( trap_Argc() != 2 ) {
	//[/ExpSys]
		oldTeam = ent->client->sess.sessionTeam;
		//[CoOp]
		if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
			switch ( oldTeam ) {
			case NPCTEAM_PLAYER:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
				break;
			case TEAM_SPECTATOR:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
				break;
			}
		} else {
			switch ( oldTeam ) {
			case TEAM_BLUE:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")) );
				break;
			case TEAM_RED:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")) );
				break;
			case TEAM_FREE:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
				break;
			case TEAM_SPECTATOR:
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
				break;
			}
		}
		//[/CoOp]
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	if (gEscaping)
		return;

	//[CoOp]
	if (in_camera)
		return;
	//[/CoOp]

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_DUEL
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
		//disallow changing teams
		trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Duel\n\"" );
		return;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	if (g_gametype.integer == GT_POWERDUEL)
	{ //don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	ent->client->switchTeamTime = level.time + 5000;

}

/*
=================
Cmd_DuelTeam_f
=================
*/
void Cmd_DuelTeam_f(gentity_t *ent)
{
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if (g_gametype.integer != GT_POWERDUEL)
	{ //don't bother doing anything if this is not power duel
		return;
	}

	/*
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"You cannot change your duel team unless you are a spectator.\n\""));
		return;
	}
	*/

	if ( trap_Argc() != 2 )
	{ //No arg so tell what team we're currently on.
		oldTeam = ent->client->sess.duelTeam;
		switch ( oldTeam )
		{
		case DUELTEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"None\n\"") );
			break;
		case DUELTEAM_LONE:
			trap_SendServerCommand( ent-g_entities, va("print \"Single\n\"") );
			break;
		case DUELTEAM_DOUBLE:
			trap_SendServerCommand( ent-g_entities, va("print \"Double\n\"") );
			break;
		default:
			break;
		}
		return;
	}

	if ( ent->client->switchDuelTeamTime > level.time )
	{ //debounce for changing
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	oldTeam = ent->client->sess.duelTeam;

	if (!Q_stricmp(s, "free"))
	{
		ent->client->sess.duelTeam = DUELTEAM_FREE;
	}
	else if (!Q_stricmp(s, "single"))
	{
		ent->client->sess.duelTeam = DUELTEAM_LONE;
	}
	else if (!Q_stricmp(s, "double"))
	{
		ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"'%s' not a valid duel team.\n\"", s) );
	}

	if (oldTeam == ent->client->sess.duelTeam)
	{ //didn't actually change, so don't care.
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //ok..die
		int curTeam = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = oldTeam;
		G_Damage(ent, ent, ent, NULL, ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		ent->client->sess.duelTeam = curTeam;
	}
	//reset wins and losses
	ent->client->sess.wins = 0;
	ent->client->sess.losses = 0;

	//get and distribute relevent paramters
	ClientUserinfoChanged( ent->s.number );

	ent->client->switchDuelTeamTime = level.time + 5000;
}

int G_TeamForSiegeClass(const char *clName)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(team);
	siegeClass_t *scl;

	if (!stm)
	{
		return 0;
	}

	while (team <= SIEGETEAM_TEAM2)
	{
		scl = stm->classes[i];

		if (scl && scl->name[0])
		{
			if (!Q_stricmp(clName, scl->name))
			{
				return team;
			}
		}

		i++;
		if (i >= MAX_SIEGE_CLASSES || i >= stm->numClasses)
		{
			if (team == SIEGETEAM_TEAM2)
			{
				break;
			}
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam(team);
			i = 0;
		}
	}

	return 0;
}

/*
=================
Cmd_SiegeClass_f
=================
*/
void Cmd_SiegeClass_f( gentity_t *ent )
{
	char className[64];
	int team = 0;
	int preScore;
	qboolean startedAsSpec = qfalse;

	if (g_gametype.integer != GT_SIEGE)
	{ //classes are only valid for this gametype
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (trap_Argc() < 1)
	{
		return;
	}

	if ( ent->client->switchClassTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSSWITCH")) );
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		startedAsSpec = qtrue;
	}

	trap_Argv( 1, className, sizeof( className ) );

	team = G_TeamForSiegeClass(className);

	if (!team)
	{ //not a valid class name
		return;
	}

	if (ent->client->sess.sessionTeam != team)
	{ //try changing it then
		g_preventTeamBegin = qtrue;
		if (team == TEAM_RED)
		{
			SetTeam(ent, "red");
		}
		else if (team == TEAM_BLUE)
		{
			SetTeam(ent, "blue");
		}
		g_preventTeamBegin = qfalse;

		if (ent->client->sess.sessionTeam != team)
		{ //failed, oh well
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR ||
				ent->client->sess.siegeDesiredTeam != team)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSTEAM")) );
				return;
			}
		}
	}

	//preserve 'is score
	preScore = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality(team, className);

	//Set the session data
	strcpy(ent->client->sess.siegeClass, className);

	// get and distribute relevent paramters
	ClientUserinfoChanged( ent->s.number );

	if (ent->client->tempSpectate < level.time)
	{
		// Kill him (makes sure he loses flags, etc)
		if (ent->health > 0 && !startedAsSpec)
		{
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		}

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec)
		{ //respawn them instantly.
			ClientBegin( ent->s.number, qfalse );
		}
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = preScore;

	ent->client->switchClassTime = level.time + 5000;
}

/*
=================
Cmd_ForceChanged_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	//[ExpSys]
	/* //racc - don't do this stuff anymore since forcepowers are now applied as soon as the client's userinfo updates.
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;

argCheck:

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}
	*/
	//[/ExpSys]

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];
		char	userinfo[MAX_INFO_STRING];

		trap_Argv( 2, arg, sizeof( arg ) );
		if (arg[0] && ent->client)
		{//new force power string, update the forcepower string.
			trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
			Info_SetValueForKey( userinfo, "forcepowers", arg );
			trap_SetUserinfo( ent->s.number, userinfo );	

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{ //if it's a spec, just make the changes now
				//No longer print it, as the UI calls this a lot.
				WP_InitForcePowers( ent );
			}
			else
			{//wait til respawn and tell the player that.
				trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED")) );

				ent->client->ps.fd.forceDoInit = 1;
			}
		}

		trap_Argv( 1, arg, sizeof( arg ) );
		if (arg[0] && arg[0] != 'x' && g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL)
		{ //if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

//[StanceSelection]
qboolean G_ValidSaberStyle(gentity_t *ent, int saberStyle);
//extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
//[/StanceSelection]
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride)
{
	char truncSaberName[64];
	int i = 0;

	if (!siegeOverride &&
		g_gametype.integer == GT_SIEGE &&
		ent->client->siegeClass != -1 &&
		(
		 bgSiegeClasses[ent->client->siegeClass].saberStance ||
		 bgSiegeClasses[ent->client->siegeClass].saber1[0] ||
		 bgSiegeClasses[ent->client->siegeClass].saber2[0]
		))
	{ //don't let it be changed if the siege class has forced any saber-related things
        return qfalse;
	}

	while (saberName[i] && i < 64-1)
	{
        truncSaberName[i] = saberName[i];
		i++;
	}
	truncSaberName[i] = 0;

	if ( saberNum == 0 && (Q_stricmp( "none", truncSaberName ) == 0 || Q_stricmp( "remove", truncSaberName ) == 0) )
	{ //can't remove saber 0 like this
        strcpy(truncSaberName, DEFAULT_SABER);
	}

	//Set the saber with the arg given. If the arg is
	//not a valid sabername defaults will be used.
	WP_SetSaber(ent->s.number, ent->client->saber, saberNum, truncSaberName);

	if (!ent->client->saber[0].model[0])
	{
		assert(0); //should never happen!
		strcpy(ent->client->sess.saberType, DEFAULT_SABER);
	}
	else
	{
		strcpy(ent->client->sess.saberType, ent->client->saber[0].name);
	}

	if (!ent->client->saber[1].model[0])
	{
		strcpy(ent->client->sess.saber2Type, "none");
	}
	else
	{
		strcpy(ent->client->sess.saber2Type, ent->client->saber[1].name);
	}

	//[StanceSelection]
	if ( !G_ValidSaberStyle(ent, ent->client->ps.fd.saberAnimLevel) )
	{//had an illegal style, revert to default
		ent->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	/*
	if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
	{
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}
	*/
	//[/StanceSelection]

	return qtrue;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	
	//[BugFix38]
	// can't follow another spectator
	if ( level.clients[ i ].tempSpectate >= level.time ) {
		return;
	}
	//[/BugFix38]

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	qboolean	looped = qfalse; //OpenRP - Avoid /team follow1 crash - Thanks to Raz0r
	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) 
	//[OpenRP - Avoid /team follow1 crash - Thanks to Raz0r]
		{
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = 0;
				looped = qtrue;
			}
	///[OpenRP - Avoid /team follow1 crash - Thanks to Raz0r]
		}
		if ( clientnum < 0 ) {
	//[OpenRP - Avoid /team follow1 crash - Thanks to Raz0r]
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = level.maxclients - 1;
				looped = qtrue;
			}
	//[/OpenRP - Avoid /team follow1 crash - Thanks to Raz0r]										  
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		//[BugFix38]
		// can't follow another spectator
		if ( level.clients[ clientnum ].tempSpectate >= level.time ) {
			return;
		}
		//[/BugFix38]

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

static void G_SayTo(gentity_t* ent, gentity_t* other, int mode, int color, const char* name, const char* message, char* locMsg)
{
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if (other->client->pers.connected != CON_CONNECTED) {
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
		return;
	}
	/*
	// no chatting to players in tournaments
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}
	*/
	//They've requested I take this out.

	

	if (locMsg)
	{
		trap_SendServerCommand(other - g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\" %i",
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, locMsg, color, message, ent->s.number));
	}
	else
	{
		trap_SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\" %i",
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message, ent->s.number));
	}
}

void delete_chat_command(char* original_text, int no_of_chars) {
	char text[MAX_SAY_TEXT] = "";
	for (int i = no_of_chars; i < strlen(original_text); i++) {
		strncat(text, &original_text[i], 1);
	}
	strcpy(original_text, text);
}



#define EC		"\x19"

//[TABBot]
extern void TAB_BotOrder( gentity_t *orderer, gentity_t *orderee, int order, gentity_t *objective);
//This badboy of a function scans the say command for possible bot orders and then does them
void BotOrderParser(gentity_t *ent, gentity_t *target, int mode, const char *chatText)
{
	int i;
	//int x;
	char tempname[36];
	gclient_t	*cl;
	char *ordereeloc;
	gentity_t *orderee = NULL;
	char *temp;
	char text[MAX_SAY_TEXT];
	int order;
	gentity_t *objective = NULL;

	if(ent->r.svFlags & SVF_BOT)
	{//bots shouldn't give orders.  They were accidently giving orders to each other with some
		//of their taunt chats.
		return;
	}

	Q_strncpyz( text, chatText, sizeof(text) );
	Q_CleanStr(text);
	Q_strlwr(text);

	//place marker at end of chattext
	ordereeloc = text;
	ordereeloc += 8*MAX_SAY_TEXT;
	//ordereeloc = Q_strrchr(text, "\0");

	//ok, first look for a orderee
	for ( i=0 ; i< g_maxclients.integer ; i++ )
	{
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}
		//[ClientNumFix]
		if ( !(g_entities[i].r.svFlags & SVF_BOT) )
		//if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) )
		//[/ClientNumFix]
		{
			continue;
		}
		strcpy(tempname, cl->pers.netname);
		Q_CleanStr(tempname);
		Q_strlwr(tempname);

		temp = strstr( text, tempname );	

		if(temp)
		{
			if(temp < ordereeloc)
			{
				ordereeloc = temp;
				//[ClientNumFix]
				orderee = &g_entities[i];
				//orderee = &g_entities[cl->ps.clientNum];
				//[/ClientNumFix]
			}
		}
	}
	
	if(!orderee)
	{//Couldn't find a bot to order
		return;
	}

	if(!OnSameTeam(ent, orderee))
	{//don't take orders from a guy on the other team.
		return;
	}

	G_Printf("%s\n", orderee->client->pers.netname);

	//ok, now determine the order given
	if(strstr(text, "kneel") || strstr(text, "bow"))
	{//BOTORDER_KNEELBEFOREZOD
		order = BOTORDER_KNEELBEFOREZOD;
	}
	else if(strstr(text, "attack") || strstr(text, "destroy"))
	{
		order = BOTORDER_SEARCHANDDESTROY;
	}
	else
	{//no order given.
		return;
	}

	//determine the target entity
	if(!objective)
	{
		if(strstr(text, "me"))
		{
			objective = ent;
		}
		else
		{//troll thru the player names for a possible objective entity.
			temp = NULL;
			for ( i=0 ; i< g_maxclients.integer ; i++ )
			{
				cl = level.clients + i;
				if ( cl->pers.connected != CON_CONNECTED )
				{
					continue;
				}
				//[ClientNumFix]
				if ( i == orderee-g_entities )
				//if ( cl->ps.clientNum == orderee->client->ps.clientNum )
				//[ClientNumFix]
				{//Don't want the orderee to be the target
					continue;
				}
				strcpy(tempname, cl->pers.netname);
				Q_CleanStr(tempname);
				Q_strlwr(tempname);

				temp = strstr( text, tempname );	

				if(temp)
				{
					if(temp > ordereeloc)
					{//Don't parse the orderee again
						//[ClientNumFix]
						objective = &g_entities[i];
						//objective = &g_entities[cl->ps.clientNum];
						//[ClientNumFix]
					}
				}
			}
		}
	}

	TAB_BotOrder(ent, orderee, order, objective);
}
//[/TABBot]


void paralyze_player(int client_id) {
	if (client_id == -1) {
		return;
	}

	gentity_t* ent = &g_entities[client_id];

	if (!(ent->flags & FL_NOTARGET)) {
		ent->flags ^= FL_NOTARGET;
	}

	// GalaxyRP (Alex): [Death System] Paralyze the target player.
	ent->client->pers.player_statuses |= (1 << 6);

	// ��������� ������� �������� ���
	ent->client->pers.downedAnim = ent->client->ps.forceDodgeAnim;

	// ��������� ���� ���������
	ent->client->ps.pm_type = PM_FREEZE;

	ent->client->ps.eFlags |= EF_INVULNERABLE;
	ent->client->invulnerableTimer = level.time + 10000;

	ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
	ent->client->ps.forceHandExtendTime = level.time + 9999999;
	ent->client->ps.velocity[2] += 150;
	ent->client->ps.forceDodgeAnim = 0;
	ent->client->ps.quickerGetup = qtrue;

	// GalaxyRP (Alex): [Death System] Set their HP to 50 so they don't die the old way instantly.
	ent->client->ps.stats[STAT_HEALTH] = 50;
	ent->health = 50;
}

qboolean can_player_get_up(gentity_t* ent, gentity_t* target) {

	if (!(target->client->pers.player_statuses & (1 << 6))) {
		trap_SendServerCommand(ent - g_entities, "print \"^1You cannot help them because they're not downed!\n\"");
		trap_SendServerCommand(ent - g_entities, "cp \"^1You cannot help them because they're not downed!\n\"");

		return qfalse;
	}

	//GalaxyRP (Alex): [Death System] Ent has permission, they can revive anyone.
	/*
	if (check_admin_command(ent, ADM_GETUP, qfalse)) {
		trap_SendServerCommand(ent - g_entities, va("cp \"^2You helped %s up.\"", target->client->pers.netname));
		trap_SendServerCommand(ent - g_entities, va("print \"^2You helped %s up.\"", target->client->pers.netname));

		return qtrue;
	}*/

	//GalaxyRP (Alex): [Death System] Ent and target are the same, player tries to get up by themselves.
	if (ent->client->ps.clientNum == target->client->ps.clientNum) {
		//GalaxyRP (Alex): [Death System] If player's timer is done, allow them to get up.
		if (ent->client->downedTime == 0) {
			trap_SendServerCommand(ent - g_entities, "print \"^2You got up!\n\"");
			trap_SendServerCommand(ent - g_entities, "cp \"^2You got up!\n\"");
			return qtrue;
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"^1You cannot get up until the timer is finished!\n\"");
			trap_SendServerCommand(ent - g_entities, "cp \"^1You cannot get up until the timer is finished!\n\"");
			return qfalse;
		}
	}
	//GalaxyRP (Alex): [Death System] Ent and target are different.
	else {
		//GalaxyRP (Alex): [Death System] Can't help someone else get up if you're also down.
		if (ent->client->pers.player_statuses & (1 << 6)) {
			trap_SendServerCommand(ent - g_entities, "print \"^1You cannot help someone else while you're downed!\n\"");
			trap_SendServerCommand(ent - g_entities, "cp \"^1You cannot help someone else while you're downed!\n\"");

			return qfalse;
		}
		else {
			trap_SendServerCommand(ent - g_entities, va("cp \"^2You helped %s up.\"", target->client->pers.netname));
			trap_SendServerCommand(ent - g_entities, va("print \"^2You helped %s up.\"", target->client->pers.netname));
			trap_SendServerCommand(target->client->ps.clientNum, va("cp \"^2 %s helped you up!.\"", ent->client->pers.netname));
			trap_SendServerCommand(target->client->ps.clientNum, va("print \"^2 %s helped you up!.\"", ent->client->pers.netname));

			return qtrue;
		}
	}

	//GalaxyRP (Alex): [Death System] If player is close enough or has admin permission, allow them to help someone up.
	if (Distance(ent->client->ps.origin, target->client->ps.origin) <= 120/* || check_admin_command(ent, ADM_GETUP, qfalse) */ ) {
		trap_SendServerCommand(ent - g_entities, va("cp \"^2You helped %s up.\"", target->client->pers.netname));
		trap_SendServerCommand(ent - g_entities, va("print \"^2You helped %s up.\"", target->client->pers.netname));
		trap_SendServerCommand(target->client->ps.clientNum, va("cp \"^2 %s helped you up!.\"", ent->client->pers.netname));
		trap_SendServerCommand(target->client->ps.clientNum, va("print \"^2 %s helped you up!.\"", ent->client->pers.netname));

		return qtrue;
	}
	else {
		trap_SendServerCommand(ent - g_entities, va("cp \"^1You are too far away to help them up!\"", target->client->pers.netname));
		trap_SendServerCommand(ent - g_entities, va("print \"^1You are too far away to help them up!\"", target->client->pers.netname));
		return qfalse;
	}
}

void help_up(gentity_t* ent, gentity_t* target) {
	if (can_player_get_up(ent, target)) {
		//GalaxyRP (Alex): [Death System] No longer paralyzed.

		target->client->pers.player_statuses &= ~(1 << 6);

		if (target->flags & FL_NOTARGET) {
			target->flags ^= FL_NOTARGET;
		}

		if (rp_downed_invulnerability_timer.integer) {
			target->client->invulnerableTimer = 0; // ����� �������
			target->client->ps.eFlags &= ~EF_INVULNERABLE; // ������ ���� ������������
		}

		target->client->invulnerableTimer = 0; // ����� �������
		target->client->ps.eFlags &= ~EF_INVULNERABLE; // ������ ���� ������������
		target->client->ps.forceHandExtend = HANDEXTEND_NONE;
		target->client->ps.forceHandExtendTime = level.time;
		target->client->downedTime = 0;
	}

	return;
}

/*
=================
Cmd_Kill_f
=================
*/

void Cmd_Helpup_f(gentity_t* ent) {
	char targetIndex[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: helpup <player>\n\"");
		return;
	}

	trap_Argv(1, targetIndex, sizeof(targetIndex));
	int i = ClientNumberFromString(ent, targetIndex, qfalse);
	if (i == -1) {
		return;
	}

	gentity_t* target;

	target = &g_entities[i];

	help_up(ent, target);

	return;
}

void Cmd_Getup_f(gentity_t* ent) {
	char otherindex[MAX_TOKEN_CHARS];

	if (trap_Argc() < 1) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /getup\n\"");
		return;
	}

	//GalaxyRP (Alex): [Death System] If player's timer is done or he is an admin, allow them to get up.
	if (ent->client->downedTime == 0 /* || check_admin_command(ent, ADM_GETUP, qfalse)*/) {
		help_up(ent, ent);
		return;
	}

	return;
}


#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )

typedef struct chat_modifiers_s {
	const char* chat_modifier;
	const char* chat_format;
	int            distance;
} chat_modifiers_t;

const chat_modifiers_t chat_modifiers[] = {
	{"/low",		"chat \"%s^9 lowers their voice:%s\n\"",			VOICE_DISTANCE_LOW	},
	{"/long",		"chat \"%s:%s\n\"",									VOICE_DISTANCE_LONG	},
	{"/all",		"chat \"%s:^2%s\n\"",								BROADCAST_DISTANCE	},
	{"/melow",		"chat \"%s^3%s\n\"",								ACTION_DISTANCE_LOW	},
	{"/meall",		"chat \"%s^3%s\n\"",								BROADCAST_DISTANCE	},
	{"/melong",		"chat \"%s^3%s\n\"",								ACTION_DISTANCE_LONG},
	{"/me",			"chat \"%s^3%s\n\"",								ACTION_DISTANCE		},
	{"/shoutlong",	"chat \"%s ^3shouts:^2%s\n\"",						VOICE_DISTANCE_LONG	},
	{"/shoutall",	"chat \"%s ^3shouts:^2%s\n\"",						BROADCAST_DISTANCE	},
	{"/shout",		"chat \"%s ^3shouts:^2%s\n\"",						SHOUT_DISTANCE		},
	{"/dolow",		"chat \"^3(%s^3)%s\n\"",							ACTION_DISTANCE_LOW	},
	{"/dolong",		"chat \"^3(%s^3)%s\n\"",							ACTION_DISTANCE_LONG},
	{"/doall",		"chat \"^3(%s^3)%s\n\"",							BROADCAST_DISTANCE	},
	{"/do",			"chat \"^3(%s^3)%s\n\"",							ACTION_DISTANCE		},
	{"/forcelow",	"chat \"%s^5 uses the Force to%s\n\"",				ACTION_DISTANCE_LOW	},
	{"/forcelong",	"chat \"%s^5 uses the Force to%s\n\"",				ACTION_DISTANCE_LONG},
	{"/forceall",	"chat \"%s^5 uses the Force to%s\n\"",				BROADCAST_DISTANCE	},
	{"/force",		"chat \"%s^5 uses the Force to%s\n\"",				ACTION_DISTANCE		},
	{"/mylow",		"chat \"%s^3's %s\n\"",								ACTION_DISTANCE_LOW	},
	{"/myall",		"chat \"%s^3's %s\n\"",								BROADCAST_DISTANCE	},
	{"/mylong",		"chat \"%s^3's %s\n\"",								ACTION_DISTANCE_LONG},
	{"/my",			"chat \"%s^3's %s\n\"",								ACTION_DISTANCE		},
	{"/ryl2",		"chat \"%s ^3(Ryl - Lekku only):^2%s\n\"",			VOICE_DISTANCE		},
	{"/ryl",		"chat \"%s ^3(Ryl):^2%s\n\"",						VOICE_DISTANCE		},
	{"/rodian",		"chat \"%s ^3(Rodian):^2%s\n\"",					VOICE_DISTANCE		},
	{"/huttese",	"chat \"%s ^3(Huttese):^2%s\n\"",					VOICE_DISTANCE		},
	{"/catharese",	"chat \"%s ^3(Catharese):^2%s\n\"",					VOICE_DISTANCE		},
	{"/mando",		"chat \"%s ^3(Mando'a):^2%s\n\"",					VOICE_DISTANCE		},
	{"/npc",		"chat \"^3(%s^3) NPC:^4%s\n\"",						VOICE_DISTANCE		},
	{"/npclow",		"chat \"^3(%s^3) NPC Lowers their voice:^4%s\n\"",	VOICE_DISTANCE_LOW	},
	{"/npcall",		"chat \"^3(%s^3) NPC:^4%s\n\"",						BROADCAST_DISTANCE	},
	{"/comm",		"chat \"^6<%s^6>^3 -C-^2%s\n\"",					BROADCAST_DISTANCE	},
	{"/c",			"chat \"^6<%s^6>^3 -C-^2%s\n\"",					BROADCAST_DISTANCE	},
	{"/thought",	"chat \"%s ^7is thinking: %s\n\"",					BROADCAST_DISTANCE	},
};

void G_Say(gentity_t* ent, gentity_t* target, int mode, const char* chatText) {
	int			j;
	gentity_t* other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons. Or let it be VERY long for RP purposes ;)
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char* locMsg = NULL;
	//distance for distance-based chat
	int distance = 999999999;
	//This is the limit where the chat text will stop appearing at
	int max_voice_distance = 600;
	//variable used for OOC chat (or team chat)
	int ooc_flag = 0;
	char ooc_text[700] = "";
	int broadcast_distance = 999999999;

	if (mode == SAY_TEAM) {
		ooc_flag = 1;
		mode = SAY_ALL;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	switch (mode) {
	default:
	case SAY_ALL:
		// zyk: if player is silenced by an admin, he cannot say anything
		if (ent->client->pers.player_statuses & (1 << 0))
			return;

		//ooc chat case
		if (ooc_flag == 1)
		{
			//add paranthesis for OOC chat (I know it's a workaround and it should be done better but it works)
			char beginning[] = "((";
			char end[] = "^1))";

			strcat(ooc_text, beginning);
			strcat(ooc_text, text);
			strcat(ooc_text, end);

			ooc_text;

			G_LogPrintf("ooc: %s: %s\n", ent->client->pers.netname, text);
			Com_sprintf(name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
			color = COLOR_RED;

			break;
		}

		char* output = NULL;

		// these two have to be in the same order, one if the distance to the modifiers, so the order has to match
		// in chat_modifiers, shorter strings have to be AFTER the longer string (e.g. /me HAS to be AFTER /melong, otherwise it'll pick /me instead)

		char slash = '/';

		const char* ptr = strchr(text, slash);
		int index_of_slash = -1;
		if (ptr) {
			index_of_slash = ptr - text;
		}

		for (int i = 0; i < ARRAY_LEN(chat_modifiers); i++) {
			output = strstr(text, chat_modifiers[i].chat_modifier);


			if (output && index_of_slash == 0) {
				delete_chat_command(text, strlen(chat_modifiers[i].chat_modifier));
				G_LogPrintf(va("%s: %s: %s\n"), chat_modifiers[i].chat_modifier, ent->client->pers.netname, text);

				for (j = 0; j < level.numConnectedClients; j++) {

					other = &g_entities[j];
					if (Distance(ent->client->ps.origin, other->client->ps.origin) <= chat_modifiers[i].distance || other->client->sess.sessionTeam == TEAM_SPECTATOR)
					{
						trap_SendServerCommand(other->client->ps.clientNum, va(chat_modifiers[i].chat_format, ent->client->pers.netname, text));
					}
					else
						continue;
				}

				return;
			}
		}

		G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, text);
		Com_sprintf(name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		//set the desired distance here
		distance = 700;
		break;
	case SAY_TEAM:
		// zyk: if player is silenced by an admin, he cannot say anything
		if (ent->client->pers.player_statuses & (1 << 0))
			return;

		//This should be visible at all times
		G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, ooc_text);
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
			locMsg = location;
		}
		else
		{
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:

		if (target && target->inuse && target->client &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
			locMsg = location;
		}
		else
		{
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_MAGENTA;
		break;
	case SAY_ALLY: // zyk: say to allies
		// zyk: if player is silenced by an admin, he cannot say anything
		if (ent->client->pers.player_statuses & (1 << 0))
			return;

		G_LogPrintf("sayally: %s: %s\n", ent->client->pers.netname, text);
		Com_sprintf(name, sizeof(name), EC"{%s%c%c"EC"}"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);

		color = COLOR_WHITE;
		break;
	}

	if (target) {
		G_SayTo(ent, target, mode, color, name, text, locMsg);
		return;
	}

	// send it to all the appropriate clients, within the desired distance
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		//I know there can be a switch here, but i'm too lazy
		if (mode == SAY_ALL || mode == SAY_TEAM)
		{
			if (mode == SAY_ALL) {

				if (Distance(ent->client->ps.origin, other->client->ps.origin) <= distance)
				{
					if (ooc_flag == 1) {
						G_SayTo(ent, other, mode, color, name, ooc_text, locMsg);
					}
					else
						G_SayTo(ent, other, mode, color, name, text, locMsg);
				}
				else
					continue;
			}
			else
				G_SayTo(ent, other, mode, color, name, ooc_text, locMsg);
		}
		else
			G_SayTo(ent, other, mode, color, name, text, locMsg);
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(gentity_t* ent) {
	char* p = NULL;

	if (trap_Argc() < 2)
		return;

	p = ConcatArgs(1);

	G_Say(ent, NULL, SAY_ALL, p);
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f(gentity_t* ent) {
	char* p = NULL;

	if (trap_Argc() < 2)
		return;

	p = ConcatArgs(1);

}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f(gentity_t* ent) {
	int			targetNum;
	gentity_t* target;
	char* p;
	char		arg[MAX_TOKEN_CHARS];

	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: tell <player id or name> <message>\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = ClientNumberFromString(ent, arg, qfalse); // zyk: changed this. Now it will use new function
	if (targetNum == -1) {
		return;
	}

	target = &g_entities[targetNum];

	p = ConcatArgs(2);

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p);
	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, p);
	}
}

//siege voice command
static void Cmd_VoiceCommand_f(gentity_t *ent)
{
	gentity_t *te;
	char arg[MAX_TOKEN_CHARS];
	char *s;
	int i = 0;

	if (g_gametype.integer < GT_TEAM)
	{
		return;
	}

	if (trap_Argc() < 2)
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		ent->client->tempSpectate >= level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOICECHATASSPEC")) );
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (arg[0] == '*')
	{ //hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
		return;
	}

	s = va("*%s", arg);

	//now, make sure it's a valid sound to be playing like this.. so people can't go around
	//screaming out death sounds or whatever.
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (!bg_customSiegeSoundNames[i])
		{
			break;
		}
		if (!Q_stricmp(bg_customSiegeSoundNames[i], s))
		{ //it matches this one, so it's ok
			break;
		}
		i++;
	}

	if (i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i])
	{ //didn't find it in the list
		return;
	}

	te = G_TempEntity(vec3_origin, EV_VOICECMD_SOUND);
	te->s.groundEntityNum = ent->s.number;
	te->s.eventParm = G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
	te->r.svFlags |= SVF_BROADCAST;
}


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	//[BugFix31]
	//This wasn't working for non-spectators since s.origin doesn't update for active players.
	if(ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR )
	{//active players use currentOrigin
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->r.currentOrigin ) ) );
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
	}
	//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
	//[/BugFix31]
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Power Duel",
	"Single Player",
	"Team FFA",
	"Siege",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
G_ClientNumberFromName

Finds the client number of the client with the given name
==================
*/
int G_ClientNumberFromName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

/*
==================
G_ClientNumberFromStrippedName

Same as above, but strips special characters out of the names before comparing.
==================
*/
int G_ClientNumberFromStrippedName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString2( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString2( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
Cmd_CallVote_f
==================
*/

//[AdminSys]
void Cmd_CallTeamVote_f( gentity_t *ent );
//[/AdminSys]
extern void SiegeClearSwitchData(void); //g_saga.c
const char *G_GetArenaInfoByMap( const char *map );
void Cmd_CallVote_f( gentity_t *ent ) {
	int		i;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];
//	int		n = 0;
//	char*	type = NULL;
	char*		mapName = 0;
	const char*	arenaInfo;

	if ( !g_allowVote.integer ) {
		//[AdminSys]
		//try teamvote if available.
		trap_Argv( 1, arg1, sizeof( arg1 ) );
		if(g_allowTeamVote.integer && !Q_stricmp( arg1, "kick" ))
		{
			Cmd_CallTeamVote_f( ent );
		}
		else
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		}
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		//[/AdminSys]
		return;
	}

	if ( level.voteTime || level.voteExecuteTime >= level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")) );
		return;
	}
	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MAXVOTES")) );
		return;
	}

//	if (g_gametype.integer != GT_DUEL &&
//		g_gametype.integer != GT_POWERDUEL)
//	{
//		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
//			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")) );
//			return;
//		}
//	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
	} else if ( !Q_stricmp( arg1, "timelimit" ) ) {
	} else if ( !Q_stricmp( arg1, "fraglimit" ) ) {
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"" );
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( arg1, "g_gametype" ) )
	{
		//[AdminSys]
		if(!g_allowGametypeVote.integer)
		{
			trap_SendServerCommand( ent-g_entities, "print \"Gametype voting is disabled.\n\"" );
			return;
		}
		//[/AdminSys]

		i = atoi( arg2 );
		if( i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			trap_SendServerCommand( ent-g_entities, "print \"Invalid gametype.\n\"" );
			return;
		}

		level.votingGametype = qtrue;
		level.votingGametypeTo = i;

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[i] );
	}
	else if ( !Q_stricmp( arg1, "map" ) ) 
	{
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];

		//[AdminSys]
		if(g_AllowMapVote.integer != 2)
		{
			if(g_AllowMapVote.integer == 1)
			{
				trap_SendServerCommand( ent-g_entities, "print \"You can only do map restart and nextmap votes while in restricting mode voting mode.\n\"" );
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "print \"Map voting is disabled.\n\"" );
			}
			return;
		}
		//[/AdminSys]

		if (!G_DoesMapSupportGametype(arg2, trap_Cvar_VariableIntegerValue("g_gametype")))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}
		
		arenaInfo	= G_GetArenaInfoByMap(arg2);
		if (arenaInfo)
		{
			mapName = Info_ValueForKey(arenaInfo, "longname");
		}

		if (!mapName || !mapName[0])
		{
			mapName = "ERROR";
		}

		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s", mapName);
	}
	else if ( !Q_stricmp ( arg1, "clientkick" ) )
	{
		int n = atoi ( arg2 );

		//[AdminSys]
		if(!g_AllowKickVote.integer)
		{
			trap_SendServerCommand( ent-g_entities, "print \"Kick voting is disabled.\n\"" );
			return;
		}
		//[/AdminSys]

		if ( n < 0 || n >= MAX_CLIENTS )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", n ) );
			return;
		}

		if ( g_entities[n].client->pers.connected == CON_DISCONNECTED )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", n ) );
			return;
		}
			
		Com_sprintf ( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[n].client->pers.netname );
	}
	else if ( !Q_stricmp ( arg1, "kick" ) )
	{
		int clientid = G_ClientNumberFromName ( arg2 );

		//[AdminSys]
		if(!g_AllowKickVote.integer)
		{
			trap_SendServerCommand( ent-g_entities, "print \"Kick voting is disabled.\n\"" );
			return;
		}
		//[/AdminSys]

		if ( clientid == -1 )
		{
			clientid = G_ClientNumberFromStrippedName(arg2);

			if (clientid == -1)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"there is no client named '%s' currently on the server.\n\"", arg2 ) );
				return;
			}
		}

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", clientid );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[clientid].client->pers.netname );
	}
	else if ( !Q_stricmp( arg1, "nextmap" ) ) 
	{
		char	s[MAX_STRING_CHARS];

		//[AdminSys]
		if(!g_AllowMapVote.integer)
		{
			trap_SendServerCommand( ent-g_entities, "print \"Map voting is disabled.\n\"" );
			return;
		}
		//[/AdminSys]

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		SiegeClearSwitchData();
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} 
	else
	{
		//[AdminSys]
		if(!g_AllowMapVote.integer && !Q_stricmp( arg1, "map_restart" ))
		{
			trap_SendServerCommand( ent-g_entities, "print \"Map voting is disabled.\n\"" );
			return;
		}
		//[/AdminSys]

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}

	trap_SendServerCommand( -1, va("print \"%s^7 %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE") ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
	}
	ent->client->mGameFlags |= PSG_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );	
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	//[AdminSys]
	//make it so that the vote command applies to team votes first before normal votes.
	int team = ent->client->sess.sessionTeam;
	int cs_offset = -1;
		
	team = ent->client->sess.sessionTeam;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;

	if( cs_offset != -1)
	{//we're on a team
		if ( level.teamVoteTime[cs_offset] && !(ent->client->mGameFlags & PSG_TEAMVOTED) )
		{//team vote is in progress and we haven't voted for it.  Vote for it instead of
			//for the normal vote.
			Cmd_TeamVote_f(ent);
			return;
		}
	}
	//[/AdminSys]

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
		return;
	}

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")) );
		return;
	}
//	if (g_gametype.integer != GT_DUEL &&
//		g_gametype.integer != GT_POWERDUEL)
//	{
//		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
//			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
//			return;
//		}
//	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")) );

	ent->client->mGameFlags |= PSG_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	//[AdminSys]
	//int		i, team, cs_offset;	
	int		i, targetClientNum=ENTITYNUM_NONE, team, cs_offset;
	//[/AdminSys]
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	//[AdminSys]
	if ( g_gametype.integer < GT_TEAM )
	{
		trap_SendServerCommand( ent-g_entities, "print \"Cannot call a team vote in a non-team gametype!\n\"" );
		return;
	}
	//[/AdminSys]
	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
	//[AdminSys]
	{
		trap_SendServerCommand( ent-g_entities, "print \"Cannot call a team vote if not on a team!\n\"" );
		return;
	}
	

	//if ( !g_allowVote.integer ) {
	if ( !g_allowTeamVote.integer ) {
	//[/AdminSys]
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADY")) );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MAXTEAMVOTES")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	//[AdminSys]
	//if ( !Q_stricmp( arg1, "leader" ) ) {
	//	char netname[MAX_NETNAME], leader[MAX_NETNAME];
	if ( !Q_stricmp( arg1, "leader" )
		|| !Q_stricmp( arg1, "kick" ) ) {
		char netname[MAX_NETNAME], target[MAX_NETNAME];

		if ( !arg2[0] ) {
			//i = ent->client->ps.clientNum;
			targetClientNum = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				//i = atoi( arg2 );
				//if ( i < 0 || i >= level.maxclients ) {
				//	trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
				targetClientNum = atoi( arg2 );
				if ( targetClientNum < 0 || targetClientNum >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", targetClientNum) );
					return;
				}

				//if ( !g_entities[i].inuse ) {
				//	trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
				if ( !g_entities[targetClientNum].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", targetClientNum) );
					return;
				}
			}
			else {
				Q_strncpyz(target, arg2, sizeof(target));
				Q_CleanStr(target);
				//Q_strncpyz(leader, arg2, sizeof(leader));
				//Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					//if ( !Q_stricmp(netname, leader) ) {
					if ( !Q_stricmp(netname, target) ) 
					{
						targetClientNum = i;
						break;
					}
				}
				if ( targetClientNum >= level.maxclients ) {
				//if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		if ( targetClientNum >= MAX_CLIENTS )
		{//wtf?
			trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
			return;
		}
		if ( level.clients[targetClientNum].sess.sessionTeam != ent->client->sess.sessionTeam )
		{//can't call a team vote on someone not on your team!
			trap_SendServerCommand( ent-g_entities, va("print \"Cannot call a team vote on someone not on your team (%s).\n\"", level.clients[targetClientNum].pers.netname) );
			return;
		}
		//just use the client number
		Com_sprintf(arg2, sizeof(arg2), "%d", targetClientNum);
		//Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player on your team> OR kick <player on your team>.\n\"" );
		//trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	if ( !Q_stricmp( "kick", arg1 ) )
	{//use clientkick and number (so they can't change their name)
		Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "clientkick %s", arg2 );
	}
	else
	{//just a number
		Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );
	}
	//[/AdminSys]

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].mGameFlags &= ~PSG_TEAMVOTED;
	}
	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLTEAMVOTECAST")) );

	ent->client->mGameFlags |= PSG_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCHEATS")));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}

//[BugFix38]
void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck ) {



	if (ent->client->ps.m_iVehicleNum)
	{ //tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			
			if ( ConCheck ) { // check connection
				int pCon = ent->client->pers.connected;
				ent->client->pers.connected = 0;
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
				ent->client->pers.connected = pCon;
			} else { // or not.
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
			}
		}
	}

	ent->client->ps.m_iVehicleNum = 0;
}
//[/BugFix38]

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->m_iVehicleNum)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(ps, forcedUse))
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SHIELDBOOSTER:
		if (ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_ARMOR])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		//[SentryGun]
		/*
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}
		*/
		//[/SentryGun]

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		trap_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_JETPACK: //do something?
		return 1;
	case HI_SQUADTEAM:
		return 1;
	case HI_VEHICLEMOUNT:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK:
		return 1;
	case HI_SPHERESHIELD:
		return 1; 
	case HI_OVERLOAD:
		return 1;
	case HI_GRAPPLE:
		return 1;
	default:
		return 1;
	}
}

void saberKnockDown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	//[TAUNTFIX]
	if (ent->client->ps.weapon != WP_SABER) {
		return;
	}

	if (level.intermissiontime) { // not during intermission
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ) { // not when spec
		return;
	}

	if (ent->client->tempSpectate >= level.time ) { // not when tempSpec
		return;
	}

	if (ent->client->ps.emplacedIndex) { //on an emplaced gun
		return;
	}

	if (ent->client->ps.m_iVehicleNum) { //in a vehicle like at-st
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if ( veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
			return;

		if ( veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
			return;
	}
	//[/TAUNTFIX]

	if (ent->client->ps.fd.forceGripCripple)
	{ //if they are being gripped, don't let them unholster their saber
		if (ent->client->ps.saberHolstered)
		{
			return;
		}
	}

	
	if (ent->client->ps.saberInFlight)
	{
		//[SaberThrowSys]
		if(!ent->client->ps.saberEntityNum)
		{//our saber is dead, Try pulling it back.
			ent->client->ps.forceHandExtend = HANDEXTEND_SABERPULL;
			ent->client->ps.forceHandExtendTime = level.time + 300;			
		}
		//Can't use the Force to turn off the saber in midair anymore 
		/* basejka code
		if (ent->client->ps.saberEntityNum)
		{ //turn it off in midair
			saberKnockDown(&g_entities[ent->client->ps.saberEntityNum], ent, ent);
		}
		*/
		//[/SaberThrowSys]
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	//[TAUNTFIX]
	/* ensiform - moved this up to the top of function
	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}
	*/
	//[/TAUNTFIX]

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered == 2)
		{
			ent->client->ps.saberHolstered = 0;

			if (ent->client->saber[0].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			}
			if (ent->client->saber[1].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
			}
		}
		else
		{
			ent->client->ps.saberHolstered = 2;
			if (ent->client->saber[0].soundOff)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
			}
			if (ent->client->saber[1].soundOff &&
				ent->client->saber[1].model[0])
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
			}
			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}


qboolean G_ValidSaberStyle(gentity_t *ent, int saberStyle)
{	
	if(saberStyle == SS_MEDIUM)
	{//SS_YELLOW is the default and always valid
		return qtrue;
	}
	//otherwise, check to see if the player has the skill to use this style
	switch (saberStyle)
	{
		case SS_FAST:
			if(ent->client->skillLevel[SK_BLUESTYLE] > 0)
			{
				return qtrue;
			}
			break;

		default:
			if(ent->client->skillLevel[saberStyle+SK_REDSTYLE-SS_STRONG] > 0)
			{//valid style
				return qtrue;
			}
			break;
	};		
	



	return qfalse;
}

extern vmCvar_t		d_saberStanceDebug;

extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;
	qboolean usingSiegeStyle = qfalse;
	
	//[BugFix15]
	// MJN - Saber Cycle Fix - Thanks Wudan!!
	if ( ent->client->ps.weapon != WP_SABER )
	{
        return;
	}

	/*
	if ( !ent || !ent->client )
	{
		return;
	}
	*/
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/	
	//[/BugFix15]

	//[TAUNTFIX]
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //not for spectators
		return;
	}

	if (ent->client->tempSpectate >= level.time)
	{ //not for spectators
		return;
	}

	if (level.intermissiontime)
	{ //not during intermission
		return;
	}

	if (ent->client->ps.m_iVehicleNum)
	{ //in a vehicle like at-st
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if ( veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
			return;

		if ( veh->m_pVehicle && veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
			return;
	}
	//[/TAUNTFIX]

	/* basejka code
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{ //no cycling for akimbo
		if ( WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) )
		{//can turn second saber off 
			//[SaberThrowSys]
			//can't toggle the other saber while the other saber is in flight.
			if ( ent->client->ps.saberHolstered == 1 && !ent->client->ps.saberInFlight)
			//if ( ent->client->ps.saberHolstered == 1 )
			//[/SaberThrowSys]
			{//have one holstered
				//unholster it
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;
				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if ( ent->client->ps.saberHolstered == 0 )
			{//have none holstered
				if ( (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
				{//can't turn it off manually
				}
				else if ( ent->client->saber[1].bladeStyle2Start > 0
					&& (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
				{//can't turn it off manually
				}
				else
				{
					//turn it off
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if (d_saberStanceDebug.integer)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\"") );
			}
			return;
		}
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) )
	{ //use staff stance then.
		if ( ent->client->ps.saberHolstered == 1 )
		{//second blade off
			if ( ent->client->ps.saberInFlight )
			{//can't turn second blade back on if it's in the air, you naughty boy!
				if (d_saberStanceDebug.integer)
				{
					trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\"") );
				}
				return;
			}
			//turn it on
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if ( ent->client->saber[0].stylesForbidden )
			{//have a style we have to use
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
				if ( ent->client->ps.weaponTime <= 0 )
				{ //not busy, set it now
					ent->client->ps.fd.saberAnimLevel = selectLevel;
				}
				else
				{ //can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = selectLevel;
				}
			}
		}
		else if ( ent->client->ps.saberHolstered == 0 )
		{//both blades on
			if ( (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
			{//can't turn it off manually
			}
			else if ( ent->client->saber[0].bladeStyle2Start > 0
				&& (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
			{//can't turn it off manually
			}
			else
			{
				//turn second one off
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if ( ent->client->saber[0].singleBladeStyle != SS_NONE )
				{
					if ( ent->client->ps.weaponTime <= 0 )
					{ //not busy, set it now
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					}
					else
					{ //can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
					}
				}
			}
		}
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\"") );
		}
		return;
	}
	*/

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	if (g_gametype.integer == GT_SIEGE &&
		ent->client->siegeClass != -1 &&
		bgSiegeClasses[ent->client->siegeClass].saberStance)
	{ //we have a flag of useable stances so cycle through it instead
		int i = selectLevel+1;

		usingSiegeStyle = qtrue;

		while (i != selectLevel)
		{ //cycle around upward til we hit the next style or end up back on this one
			if (i >= SS_NUM_SABER_STYLES)
			{ //loop back around to the first valid
				i = SS_FAST;
			}

			if (bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i))
			{ //we can use this one, select it and break out.
				selectLevel = i;
				break;
			}
			i++;
		}

		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\"") );
		}
	}
	else
	{//normal style selection
		int attempts;
		selectLevel++;

		for(attempts = 0; attempts < SS_STAFF; attempts++)
		{
			if(selectLevel > SS_STAFF)
			{
				selectLevel = SS_FAST;
			}

			if(G_ValidSaberStyle(ent, selectLevel))
			{
				break;
			}

			//no dice, keep looking
			selectLevel++;
		}

		//handle saber activation/deactivation based on the style transition
		if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0]
			&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) )
		{//using dual sabers
			if(selectLevel != SS_DUAL && ent->client->ps.saberHolstered == 0 && !ent->client->ps.saberInFlight)
			{//not using dual style, turn off the other blade
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
				ent->client->ps.saberHolstered = 1;
			}
			else if(selectLevel == SS_DUAL && ent->client->ps.saberHolstered == 1 && !ent->client->ps.saberInFlight)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;
			}
		}
		else if (ent->client->saber[0].numBlades > 1
			&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) )
		{ //use staff stance then.
			if(selectLevel != SS_STAFF && ent->client->ps.saberHolstered == 0 && !ent->client->ps.saberInFlight)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
			}
			else if(selectLevel == SS_STAFF && ent->client->ps.saberHolstered == 1 && !ent->client->ps.saberInFlight)
			{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
					ent->client->ps.saberHolstered = 0;
			}
		}
		/*
		//[HiddenStances]
		if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] 
		&& ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_3
			|| selectLevel > SS_TAVION)
		//if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
		//[/HiddenStances]
		{
			selectLevel = FORCE_LEVEL_1;
		}
		*/
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\"") );
		}
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	/*
	if ( !usingSiegeStyle )
	{
		//make sure it's valid, change it if not
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
	}
	*/

	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}


//[DuelSys]
extern vmCvar_t g_multiDuel;
//[/DuelSys]
//[TABBots]
extern void TAB_BotSaberDuelChallenged(gentity_t *bot, gentity_t *player);
extern int FindBotType(int clientNum);
//[/TABBots]

void Cmd_EngageDuel_f(gentity_t *ent)
{
	trace_t tr;
	vec3_t forward, fwdOrg;

	if (!g_privateDuel.integer)
	{
		return;
	}

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{ //rather pointless in this mode..
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	//[DuelSys] 
	//Allow dueling in team games.
	/* basejka code
	//if (g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_SIEGE)
	if (g_gametype.integer >= GT_TEAM)
	{ //no private dueling in team modes
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE")) );
		return;
	}
	*/
	//[/DuelSys]

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	/*
	if (!ent->client->ps.saberHolstered)
	{ //must have saber holstered at the start of the duel
		return;
	}
	*/
	//NOTE: No longer doing this..

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.duelInProgress)
	{
		return;
	}

	// New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	//[DuelSys]
	// Update - MJN - This uses the new duelTimer cvar to get time, in seconds, before next duel is allowed.
	//[/DuelSys]
	if (ent->client->ps.fd.privateDuelTime > level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "CANTDUEL_JUSTDID")) );
		return;
	}
	//[DuelSys]
	// MJN - cvar g_multiDuel allows more than 1 private duel at a time.
	if (!g_multiDuel.integer && G_OtherPlayersDueling())
	//if (G_OtherPlayersDueling())
	//[/DuelSys]
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "CANTDUEL_BUSY")) );
		return;
	}

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	trap_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t *challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			challenged->client->ps.weapon != WP_SABER || challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight)
		{
			return;
		}
		//[DuelSys]
		// MJN - added friendly fire check. Allows private duels in team games where friendly fire is on.
		if (!g_friendlyFire.integer && (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged)))
		//if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
		//[/DuelSys]
		{
			return;
		}

		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
		{//racc - our duel target has already challenged us, start the duel.
			//[DuelSys]
			// MJN - added ^7 to clear the color on following text
			trap_SendServerCommand( /*challenged-g_entities*/-1, va("print \"%s ^7%s %s!\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
			//trap_SendServerCommand( /*challenged-g_entities*/-1, va("print \"%s %s %s!\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
			//[/DuelSys]

			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;

			ent->client->ps.duelTime = level.time + 2000;
			challenged->client->ps.duelTime = level.time + 2000;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
			G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

			//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)

			if (!ent->client->ps.saberHolstered)
			{
				if (ent->client->saber[0].soundOff)
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				}
				if (ent->client->saber[1].soundOff &&
					ent->client->saber[1].model[0])
				{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
				}
				ent->client->ps.weaponTime = 400;
				ent->client->ps.saberHolstered = 2;
			}
			if (!challenged->client->ps.saberHolstered)
			{
				if (challenged->client->saber[0].soundOff)
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[0].soundOff);
				}
				if (challenged->client->saber[1].soundOff &&
					challenged->client->saber[1].model[0])
				{
					G_Sound(challenged, CHAN_AUTO, challenged->client->saber[1].soundOff);
				}
				challenged->client->ps.weaponTime = 400;
				challenged->client->ps.saberHolstered = 2;
			}
		}
		else
		{
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			//[DuelSys]
			// MJN - added "\n ^7" to properly align text on screen
			trap_SendServerCommand( challenged-g_entities, va("cp \"%s\n ^7%s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")) );
			trap_SendServerCommand( ent-g_entities, va("cp \"%s\n ^7%s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
			//trap_SendServerCommand( challenged-g_entities, va("cp \"%s %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")) );
			//trap_SendServerCommand( ent-g_entities, va("cp \"%s %s\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
			//[/DuelSys]
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		//[TABBots]
		if((challenged->r.svFlags & SVF_BOT) && FindBotType(challenged->s.number) == BOT_TAB)
		{//we just tried to challenge a TABBot, check to see if it's wishes to go for it.			
			TAB_BotSaberDuelChallenged(challenged, ent);
		}
		//[/TABBots]

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 5000;
	}
}

#ifndef FINAL_BUILD
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}


//[SaberSys]
void Cmd_DebugSetSaberBlock_f(gentity_t *self)
{//This is a simple debugging function for debugging the saberblocked code.
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	//self->client->ps.saberMove = atoi(arg);
	//self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
	self->client->ps.saberBlocked = atoi(arg);

	if (self->client->ps.saberBlocked > BLOCKED_TOP_PROJ)
	{
		self->client->ps.saberBlocked = BLOCKED_TOP_PROJ;
	}
}
//[/SaberSys]


void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	G_SetAnim(self, NULL, SETANIM_BOTH, i, flags, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	G_SetAnim(self, NULL, SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t *self);

void Bot_SetForcedMovement(int bot, int forward, int right, int up);

#ifndef FINAL_BUILD
extern void DismembermentByNum(gentity_t *self, int num);
extern void G_SetVehDamageFlags( gentity_t *veh, int shipSurf, int damageLevel );
#endif

static int G_ClientNumFromNetname(char *name)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client &&
			!Q_stricmp(ent->client->pers.netname, name))
		{
			return ent->s.number;
		}
		i++;
	}

	return -1;
}

qboolean TryGrapple(gentity_t *ent)
{
	if (ent->client->ps.weaponTime > 0)
	{ //weapon busy
		return qfalse;
	}
	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //force power or knockdown or something
		return qfalse;
	}
	if (ent->client->grappleState)
	{ //already grappling? but weapontime should be > 0 then..
		return qfalse;
	}

	if (ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
	{
		Cmd_ToggleSaber_f(ent);
		if (!ent->client->ps.saberHolstered)
		{ //must have saber holstered
			return qfalse;
		}
	}

	//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_PA_1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	if (ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{ //providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if (ent->client->ps.legsAnim == ent->client->ps.torsoAnim)
		{
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

		//[BugFix35]
		ent->client->dangerTime = level.time;
		//[/BugFix35]
		return qtrue;
	}

	return qfalse;
}

qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);

//[ROQFILES]
extern qboolean inGameCinematic;
//[/ROQFILES]

/*
=================
ClientCommand
=================
*/
//[CoOpEditor]
extern void Create_Autosave( vec3_t origin, int size, qboolean teleportPlayers );
extern void Save_Autosaves(void);
extern void Delete_Autosaves(gentity_t* ent);
//[/CoOpEditor]
//[KnockdownSys]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//[/KnockdownSys]
void ClientCommand( int clientNum ) {
	gentity_t *ent;
//	gentity_t *targetplayer;
	char	cmd[MAX_TOKEN_CHARS];
	char	cmd2[MAX_TOKEN_CHARS];
	//char	cmd3[MAX_TOKEN_CHARS];
//	float		bounty;
//	int clientid = 0;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww
	if(!Q_stricmp(cmd,"modelscale"))
	{
		int size;
//		int temp,temp2;
		if(!ojp_modelscaleEnabled.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Modelscale is disabled!\n\"") );
			return;
		}
		if(trap_Argc()!=2)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Current modelscale is %i.\n\"", (ent->client->ps.iModelScale ? ent->client->ps.iModelScale : 100)) );
			return;
		}
		trap_Argv(1,cmd2,sizeof(cmd2));
		size=atoi(cmd2);
		ent->client->ps.iModelScale=size;
		//ent->client->ps.stats[STAT_MAX_DODGE] = ((100-size)*(ent->client->ps.stats[STAT_DODGE]/100))/2;
 
		return;
	}

	if (Q_stricmp(cmd, "register") == 0) {
		Cmd_Register_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "login") == 0) {
		Cmd_Login_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "logout") == 0) {
		Cmd_Logout_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "addsp") == 0) {
		Cmd_AddSkillPoint_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "mdscl") == 0) {
		Cmd_SetModelScale_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "roll") == 0) {
		Cmd_Roll_f(ent);
		return;
	}
	if(Q_stricmp(cmd, "anim") == 0) {
		Cmd_Emote_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "getup") == 0) {
		Cmd_Getup_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "helpup") == 0) {
		Cmd_Helpup_f(ent);
		return;
	}

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}

	if (Q_stricmp (cmd, "say_team") == 0) {
		if (g_gametype.integer < GT_TEAM)
		{ //not a team game, just refer to regular say.
			Cmd_Say_f (ent, SAY_ALL, qfalse);
		}
		else
		{
			Cmd_Say_f (ent, SAY_TEAM, qfalse);
		}
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}

	//note: these voice_cmds come from the ui/jamp/ingame_voicechat.menu menu file...
	//		the strings are in strings/English/menus.str and all start with "VC_"
	if (Q_stricmp(cmd, "voice_cmd") == 0)
	{
		Cmd_VoiceCommand_f(ent);
		return;
	}

	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}

	//[ROQFILES]
	if (Q_stricmp (cmd, "EndCinematic") == 0)
	{//one of the clients just finished their cutscene, start rendering server frames again.
		inGameCinematic = qfalse;
		return;
	}
	//[/ROQFILES]
	

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		qboolean giveError = qfalse;
		//rwwFIXMEFIXME: This is terrible, write it differently

		if (!Q_stricmp(cmd, "give"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "giveother"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "god"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "notarget"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "noclip"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "kill"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamtask"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "levelshot"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follow"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follownext"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "followprev"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "team"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "duelteam"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "siegeclass"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "forcechanged"))
		{ //special case: still update force change
			Cmd_ForceChanged_f (ent);
			return;
		}
		else if (!Q_stricmp(cmd, "where"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "vote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callteamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "gc"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "setviewpos"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "stats"))
		{
			giveError = qtrue;
		}

		if (giveError)
		{
			trap_SendServerCommand( clientNum, va("print \"%s (%s) \n\"", G_GetStringEdString("MP_SVGAME", "CANNOT_TASK_INTERMISSION"), cmd ) );
		}
		else
		{
			Cmd_Say_f (ent, qfalse, qtrue);
		}
		return;
	}

	if (Q_stricmp (cmd, "give") == 0)
	{
		Cmd_Give_f (ent, 0);
	}
	else if (Q_stricmp (cmd, "giveother") == 0)
	{ //for debugging pretty much
		Cmd_Give_f (ent, 1);
	}
	else if (Q_stricmp (cmd, "t_use") == 0 && CheatsOk(ent))
	{ //debug use map object
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			gentity_t *targ;

			trap_Argv( 1, sArg, sizeof( sArg ) );
			targ = G_Find( NULL, FOFS(targetname), sArg );

			while (targ)
			{
				if (targ->use)
				{
					targ->use(targ, ent, ent);
				}
				targ = G_Find( targ, FOFS(targetname), sArg );
			}
		}
	}
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if ( Q_stricmp( cmd, "NPC" ) == 0 && CheatsOk(ent) && !in_camera)
	{
		Cmd_NPC_f( ent );
	}
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "duelteam") == 0)
		Cmd_DuelTeam_f (ent);
	else if (Q_stricmp (cmd, "siegeclass") == 0)
		Cmd_SiegeClass_f (ent);
	else if (Q_stricmp (cmd, "forcechanged") == 0)
		Cmd_ForceChanged_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f (ent);
	else if (Q_stricmp (cmd, "teamvote") == 0)
		Cmd_TeamVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else if (Q_stricmp (cmd, "stats") == 0)
		Cmd_Stats_f( ent );
	//for convenient powerduel testing in release
	else if (Q_stricmp(cmd, "killother") == 0 && CheatsOk( ent ))
	{
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			int entNum = 0;

			trap_Argv( 1, sArg, sizeof( sArg ) );

			entNum = G_ClientNumFromNetname(sArg);

			if (entNum >= 0 && entNum < MAX_GENTITIES)
			{
				gentity_t *kEnt = &g_entities[entNum];

				if (kEnt->inuse && kEnt->client)
				{
					kEnt->flags &= ~FL_GODMODE;
					kEnt->client->ps.stats[STAT_HEALTH] = kEnt->health = -999;
					player_die (kEnt, kEnt, kEnt, 100000, MOD_SUICIDE);
				}
			}
		}
	}
	//[Test]
		else if (Q_stricmp(cmd, "dropsaber") == 0)
	{		
		vec3_t vecnorm;

		if (g_allowDropSaber.integer == 0)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Drop is disabled on this server!\n\"" ) );
			return;
		}
				
		if (ent->client->ps.weapon == WP_SABER &&
			ent->client->ps.saberEntityNum &&
			ent->client->ps.saberInFlight)
		{
			return;
		}
		if (ent->client->ps.weapon == WP_SABER &&
				ent->client->ps.saberEntityNum &&
				!ent->client->ps.saberInFlight)
		{
			saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
		}
		else {
			TossClientWeapon(ent, vecnorm, 500);
		}
	}
#ifndef FINAL_BUILD
	else if (Q_stricmp(cmd, "testtrace") == 0)
	{
		trace_t tr;
		vec3_t traceTo, traceFrom, traceDir;

		AngleVectors(ent->client->ps.viewangles, traceDir, 0, 0);
		VectorCopy(ent->client->ps.origin, traceFrom);
		VectorMA( traceFrom, 30, traceDir, traceTo );

		trap_Trace( &tr, traceFrom, NULL, NULL, traceTo, ent->s.number, MASK_SHOT );

		if(tr.fraction < 1.0f)
		{
			G_Printf("%i", tr.entityNum);
		}
	}
#endif
	//[/Test]
	else if (Q_stricmp(cmd, "lamercheck") == 0)
	{
		trap_SendServerCommand( -1, va("cp \"This mod is based on code taken from the\nOpen Jedi Project. If the supposed author doesn't\ngive proper credit to OJP,\nplease contact us and we\n will deal with it.\nEmail: razorace@hotmail.com\n\""));
	}
	//[HolocronFiles]
	else if (Q_stricmp(cmd, "!addholocron") == 0 && bot_wp_edit.integer >= 1)
	{// Add a new holocron point. Unique1 added.
		AOTCTC_Holocron_Add ( ent );
	}
	else if (Q_stricmp(cmd, "!saveholocrons") == 0 && bot_wp_edit.integer >= 1)
	{// Save holocron position table. Unique1 added.
		AOTCTC_Holocron_Savepositions();		
	}
	else if (Q_stricmp(cmd, "!spawnholocron") == 0 && bot_wp_edit.integer >= 1)
	{// Spawn a holocron... Unique1 added.
		AOTCTC_Create_Holocron( rand()%18, ent->r.currentOrigin );
	}
	//[/HolocronFiles]
	//[CoOpEditor]
	else if (Q_stricmp (cmd, "autosave_add") == 0 && bot_wp_edit.integer)
	{
		int args = trap_Argc();
		char arg1[MAX_STRING_CHARS];
		char arg2[MAX_STRING_CHARS];

		
		if(args < 1)
		{//no args, use defaults
			Create_Autosave(ent->r.currentOrigin, 0, qfalse);
		}
		else
		{
			trap_Argv(1, arg1, sizeof(arg1));
			if(arg1[0] == 't')
			{//use default size with teleport flag
				Create_Autosave(ent->r.currentOrigin, 0, qtrue);
			}
			else if(args > 1)
			{//size and teleport flag
				trap_Argv(2, arg2, sizeof(arg2));
				Create_Autosave(ent->r.currentOrigin, atoi(arg1), arg2[0] == 't' ? qtrue:qfalse );
			}
			else
			{//just size
				Create_Autosave(ent->r.currentOrigin, atoi(arg1), qfalse );
			}
		}
	}
	else if (Q_stricmp (cmd, "autosave_save") == 0 && bot_wp_edit.integer)
	{
		Save_Autosaves();
	}
	else if (Q_stricmp (cmd, "autosave_delete") == 0 && bot_wp_edit.integer)
	{
		Delete_Autosaves(ent);
	}
	//[/CoOpEditor]
#ifdef _DEBUG
	else if (Q_stricmp(cmd, "relax") == 0 && CheatsOk( ent ))
	{
		if (ent->client->ps.eFlags & EF_RAG)
		{
			ent->client->ps.eFlags &= ~EF_RAG;
		}
		else
		{
			ent->client->ps.eFlags |= EF_RAG;
		}
	}
	else if (Q_stricmp(cmd, "holdme") == 0 && CheatsOk( ent ))
	{
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			int entNum = 0;

			trap_Argv( 1, sArg, sizeof( sArg ) );

			entNum = atoi(sArg);

			if (entNum >= 0 &&
				entNum < MAX_GENTITIES)
			{
				gentity_t *grabber = &g_entities[entNum];

				if (grabber->inuse && grabber->client && grabber->ghoul2)
				{
					if (!grabber->s.number)
					{ //switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
						ent->client->ps.ragAttach = ENTITYNUM_NONE;
					}
					else
					{
						ent->client->ps.ragAttach = grabber->s.number;
					}
				}
			}
		}
		else
		{
			ent->client->ps.ragAttach = 0;
		}
	}
	else if (Q_stricmp(cmd, "limb_break") == 0 && CheatsOk( ent ))
	{
		if (trap_Argc() > 1)
		{
			char sArg[MAX_STRING_CHARS];
			int breakLimb = 0;

			trap_Argv( 1, sArg, sizeof( sArg ) );
			if (!Q_stricmp(sArg, "right"))
			{
				breakLimb = BROKENLIMB_RARM;
			}
			else if (!Q_stricmp(sArg, "left"))
			{
				breakLimb = BROKENLIMB_LARM;
			}

			G_BreakArm(ent, breakLimb);
		}
	}
	else if (Q_stricmp(cmd, "headexplodey") == 0 && CheatsOk( ent ))
	{
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			DismembermentTest(ent);
		}
	}
	else if (Q_stricmp(cmd, "debugstupidthing") == 0 && CheatsOk( ent ))
	{
		int i = 0;
		gentity_t *blah;
		while (i < MAX_GENTITIES)
		{
			blah = &g_entities[i];
			if (blah->inuse && blah->classname && blah->classname[0] && !Q_stricmp(blah->classname, "NPC_Vehicle"))
			{
				Com_Printf("Found it.\n");
			}
			i++;
		}
	}
	else if (Q_stricmp(cmd, "arbitraryprint") == 0 && CheatsOk( ent ))
	{
		trap_SendServerCommand( -1, va("cp \"Blah blah blah\n\""));
	}
	else if (Q_stricmp(cmd, "handcut") == 0 && CheatsOk( ent ))
	{
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		if (trap_Argc() > 1)
		{
			trap_Argv( 1, sarg, sizeof( sarg ) );

			if (sarg[0])
			{
				bCl = atoi(sarg);

				if (bCl >= 0 && bCl < MAX_GENTITIES)
				{
					gentity_t *hEnt = &g_entities[bCl];

					if (hEnt->client)
					{
						if (hEnt->health > 0)
						{
							gGAvoidDismember = 1;
							hEnt->flags &= ~FL_GODMODE;
							hEnt->client->ps.stats[STAT_HEALTH] = hEnt->health = -999;
							player_die (hEnt, hEnt, hEnt, 100000, MOD_SUICIDE);
						}
						gGAvoidDismember = 2;
						G_CheckForDismemberment(hEnt, ent, hEnt->client->ps.origin, 999, hEnt->client->ps.legsAnim, qfalse);
						gGAvoidDismember = 0;
					}
				}
			}
		}
	}
	else if (Q_stricmp(cmd, "loveandpeace") == 0 && CheatsOk( ent ))
	{
		trace_t tr;
		vec3_t fPos;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

		fPos[0] = ent->client->ps.origin[0] + fPos[0]*40;
		fPos[1] = ent->client->ps.origin[1] + fPos[1]*40;
		fPos[2] = ent->client->ps.origin[2] + fPos[2]*40;

		trap_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

		if (tr.entityNum < MAX_CLIENTS && tr.entityNum != ent->s.number)
		{
			gentity_t *other = &g_entities[tr.entityNum];

			if (other && other->inuse && other->client)
			{
				vec3_t entDir;
				vec3_t otherDir;
				vec3_t entAngles;
				vec3_t otherAngles;

				if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(ent);
				}

				if (other->client->ps.weapon == WP_SABER && !other->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(other);
				}

				if ((ent->client->ps.weapon != WP_SABER || ent->client->ps.saberHolstered) &&
					(other->client->ps.weapon != WP_SABER || other->client->ps.saberHolstered))
				{
					VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
					VectorCopy( ent->client->ps.viewangles, entAngles );
					entAngles[YAW] = vectoyaw( otherDir );
					SetClientViewAngle( ent, entAngles );

					StandardSetBodyAnim(ent, /*BOTH_KISSER1LOOP*/BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
					ent->client->ps.saberMove = LS_NONE;
					ent->client->ps.saberBlocked = 0;
					ent->client->ps.saberBlocking = 0;

					VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
					VectorCopy( other->client->ps.viewangles, otherAngles );
					otherAngles[YAW] = vectoyaw( entDir );
					SetClientViewAngle( other, otherAngles );

					StandardSetBodyAnim(other, /*BOTH_KISSEE1LOOP*/BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
					other->client->ps.saberMove = LS_NONE;
					other->client->ps.saberBlocked = 0;
					other->client->ps.saberBlocking = 0;
				}
			}
		}
	}
#endif
	//[MELEE]
	else if (Q_stricmp(cmd, "togglesaber") == 0)
	{
		Cmd_ToggleSaber_f(ent);
	}
	/* racc - This cheat code isn't used anymore.
	else if (Q_stricmp(cmd, "thedestroyer") == 0 && CheatsOk( ent ) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
		Cmd_ToggleSaber_f(ent);

		if (!ent->client->ps.saberHolstered)
		{
		}
	}
	*/
	//begin bot debug cmds
	else if (Q_stricmp(cmd, "debugBMove_Forward") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, arg, -1, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Back") == 0 && CheatsOk(ent))
	{
		int arg = -4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, arg, -1, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Right") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, arg, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Left") == 0 && CheatsOk(ent))
	{
		int arg = -4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, arg, -1);
	}
	else if (Q_stricmp(cmd, "debugBMove_Up") == 0 && CheatsOk(ent))
	{
		int arg = 4000;
		int bCl = 0;
		char sarg[MAX_STRING_CHARS];

		assert(trap_Argc() > 1);
		trap_Argv( 1, sarg, sizeof( sarg ) );

		assert(sarg[0]);
		bCl = atoi(sarg);
		Bot_SetForcedMovement(bCl, -1, -1, arg);
	}
	//end bot debug cmds
#ifndef FINAL_BUILD
	else if (Q_stricmp(cmd, "debugSetSaberMove") == 0)
	{
		Cmd_DebugSetSaberMove_f(ent);
	}
	//[SaberSys]
	//This command forces the player into a given saberblocked state which is determined by the inputed numberical value.
	else if (Q_stricmp(cmd, "debugSetSaberBlock") == 0)
	{
		Cmd_DebugSetSaberBlock_f(ent);
	}
	//[/SaberSys]
	else if (Q_stricmp(cmd, "debugSetBodyAnim") == 0)
	{
		Cmd_DebugSetBodyAnim_f(ent, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	}
	else if (Q_stricmp(cmd, "debugDismemberment") == 0)
	{
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			char	arg[MAX_STRING_CHARS];
			int		iArg = 0;

			if (trap_Argc() > 1)
			{
				trap_Argv( 1, arg, sizeof( arg ) );

				if (arg[0])
				{
					iArg = atoi(arg);
				}
			}

			DismembermentByNum(ent, iArg);
		}
	}
	else if (Q_stricmp(cmd, "debugDropSaber") == 0)
	{
		if (ent->client->ps.weapon == WP_SABER &&
			ent->client->ps.saberEntityNum &&
			!ent->client->ps.saberInFlight)
		{
			saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
		}
	}
	else if (Q_stricmp(cmd, "debugKnockMeDown") == 0)
	{
		//[KnockdownSys]
		G_Knockdown(ent, NULL, vec3_origin, 300, qtrue);
		/*
		if (BG_KnockDownable(&ent->client->ps))
		{
			ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
			ent->client->ps.forceDodgeAnim = 0;
			if (trap_Argc() > 1)
			{
				ent->client->ps.forceHandExtendTime = level.time + 1100;
				ent->client->ps.quickerGetup = qfalse;
			}
			else
			{
				ent->client->ps.forceHandExtendTime = level.time + 700;
				ent->client->ps.quickerGetup = qtrue;
			}
		}
		*/
		//[/KnockdownSys]
	}
	else if (Q_stricmp(cmd, "debugSaberSwitch") == 0)
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);
				
				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client)
		{
			Cmd_ToggleSaber_f(targ);
		}
	}
	else if (Q_stricmp(cmd, "debugIKGrab") == 0)
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);
				
				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client && ent->s.number != targ->s.number)
		{
			targ->client->ps.heldByClient = ent->s.number+1;
		}
	}
	else if (Q_stricmp(cmd, "debugIKBeGrabbedBy") == 0)
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);
				
				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client && ent->s.number != targ->s.number)
		{
			ent->client->ps.heldByClient = targ->s.number+1;
		}
	}
	else if (Q_stricmp(cmd, "debugIKRelease") == 0)
	{
		gentity_t *targ = NULL;

		if (trap_Argc() > 1)
		{
			char	arg[MAX_STRING_CHARS];

			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				int x = atoi(arg);
				
				if (x >= 0 && x < MAX_CLIENTS)
				{
					targ = &g_entities[x];
				}
			}
		}

		if (targ && targ->inuse && targ->client)
		{
			targ->client->ps.heldByClient = 0;
		}
	}
	else if (Q_stricmp(cmd, "debugThrow") == 0)
	{
		trace_t tr;
		vec3_t tTo, fwd;

		if (ent->client->ps.weaponTime > 0 || ent->client->ps.forceHandExtend != HANDEXTEND_NONE ||
			ent->client->ps.groundEntityNum == ENTITYNUM_NONE || ent->health < 1)
		{
			return;
		}

		AngleVectors(ent->client->ps.viewangles, fwd, 0, 0);
		tTo[0] = ent->client->ps.origin[0] + fwd[0]*32;
		tTo[1] = ent->client->ps.origin[1] + fwd[1]*32;
		tTo[2] = ent->client->ps.origin[2] + fwd[2]*32;

		trap_Trace(&tr, ent->client->ps.origin, 0, 0, tTo, ent->s.number, MASK_PLAYERSOLID);

		if (tr.fraction != 1)
		{
			gentity_t *other = &g_entities[tr.entityNum];

			if (other->inuse && other->client && other->client->ps.forceHandExtend == HANDEXTEND_NONE &&
				other->client->ps.groundEntityNum != ENTITYNUM_NONE && other->health > 0 &&
				(int)ent->client->ps.origin[2] == (int)other->client->ps.origin[2])
			{
				float pDif = 40.0f;
				vec3_t entAngles, entDir;
				vec3_t otherAngles, otherDir;
				vec3_t intendedOrigin;
				vec3_t boltOrg, pBoltOrg;
				vec3_t tAngles, vDif;
				vec3_t fwd, right;
				trace_t tr;
				trace_t tr2;

				VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
				VectorCopy( ent->client->ps.viewangles, entAngles );
				entAngles[YAW] = vectoyaw( otherDir );
				SetClientViewAngle( ent, entAngles );

				ent->client->ps.forceHandExtend = HANDEXTEND_PRETHROW;
				ent->client->ps.forceHandExtendTime = level.time + 5000;

				ent->client->throwingIndex = other->s.number;
				ent->client->doingThrow = level.time + 5000;
				ent->client->beingThrown = 0;

				VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
				VectorCopy( other->client->ps.viewangles, otherAngles );
				otherAngles[YAW] = vectoyaw( entDir );
				SetClientViewAngle( other, otherAngles );

				other->client->ps.forceHandExtend = HANDEXTEND_PRETHROWN;
				other->client->ps.forceHandExtendTime = level.time + 5000;

				other->client->throwingIndex = ent->s.number;
				other->client->beingThrown = level.time + 5000;
				other->client->doingThrow = 0;

				//Doing this now at a stage in the throw, isntead of initially.
				//other->client->ps.heldByClient = ent->s.number+1;

				G_EntitySound( other, CHAN_VOICE, G_SoundIndex("*pain100.wav") );
				G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*jump1.wav") );
				G_Sound(other, CHAN_AUTO, G_SoundIndex( "sound/movers/objects/objectHit.wav" ));

				//see if we can move to be next to the hand.. if it's not clear, break the throw.
				VectorClear(tAngles);
				tAngles[YAW] = ent->client->ps.viewangles[YAW];
				VectorCopy(ent->client->ps.origin, pBoltOrg);
				AngleVectors(tAngles, fwd, right, 0);
				boltOrg[0] = pBoltOrg[0] + fwd[0]*8 + right[0]*pDif;
				boltOrg[1] = pBoltOrg[1] + fwd[1]*8 + right[1]*pDif;
				boltOrg[2] = pBoltOrg[2];

				VectorSubtract(boltOrg, pBoltOrg, vDif);
				VectorNormalize(vDif);

				VectorClear(other->client->ps.velocity);
				intendedOrigin[0] = pBoltOrg[0] + vDif[0]*pDif;
				intendedOrigin[1] = pBoltOrg[1] + vDif[1]*pDif;
				intendedOrigin[2] = other->client->ps.origin[2];

				trap_Trace(&tr, intendedOrigin, other->r.mins, other->r.maxs, intendedOrigin, other->s.number, other->clipmask);
				trap_Trace(&tr2, ent->client->ps.origin, ent->r.mins, ent->r.maxs, intendedOrigin, ent->s.number, CONTENTS_SOLID);

				if (tr.fraction == 1.0 && !tr.startsolid && tr2.fraction == 1.0 && !tr2.startsolid)
				{
					VectorCopy(intendedOrigin, other->client->ps.origin);
				}
				else
				{ //if the guy can't be put here then it's time to break the throw off.
					vec3_t oppDir;
					int strength = 4;

					other->client->ps.heldByClient = 0;
					other->client->beingThrown = 0;
					ent->client->doingThrow = 0;

					ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
					G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain25.wav") );

					other->client->ps.forceHandExtend = HANDEXTEND_NONE;
					VectorSubtract(other->client->ps.origin, ent->client->ps.origin, oppDir);
					VectorNormalize(oppDir);
					other->client->ps.velocity[0] = oppDir[0]*(strength*40);
					other->client->ps.velocity[1] = oppDir[1]*(strength*40);
					other->client->ps.velocity[2] = 150;

					VectorSubtract(ent->client->ps.origin, other->client->ps.origin, oppDir);
					VectorNormalize(oppDir);
					ent->client->ps.velocity[0] = oppDir[0]*(strength*40);
					ent->client->ps.velocity[1] = oppDir[1]*(strength*40);
					ent->client->ps.velocity[2] = 150;
				}
			}
		}
	}
#endif
#ifdef VM_MEMALLOC_DEBUG
	else if (Q_stricmp(cmd, "debugTestAlloc") == 0)
	{ //rww - small routine to stress the malloc trap stuff and make sure nothing bad is happening.
		char *blah;
		int i = 1;
		int x;

		//stress it. Yes, this will take a while. If it doesn't explode miserably in the process.
		while (i < 32768)
		{
			x = 0;

			trap_TrueMalloc((void **)&blah, i);
			if (!blah)
			{ //pointer is returned null if allocation failed
				trap_SendServerCommand( -1, va("print \"Failed to alloc at %i!\n\"", i));
				break;
			}
			while (x < i)
			{ //fill the allocated memory up to the edge
				if (x+1 == i)
				{
					blah[x] = 0;
				}
				else
				{
					blah[x] = 'A';
				}
				x++;
			}
			trap_TrueFree((void **)&blah);
			if (blah)
			{ //should be nullified in the engine after being freed
				trap_SendServerCommand( -1, va("print \"Failed to free at %i!\n\"", i));
				break;
			}

			i++;
		}

		trap_SendServerCommand( -1, "print \"Finished allocation test\n\"");
	}
#endif
#ifndef FINAL_BUILD
	else if (Q_stricmp(cmd, "debugShipDamage") == 0)
	{
		char	arg[MAX_STRING_CHARS];
		char	arg2[MAX_STRING_CHARS];
		int		shipSurf, damageLevel;

		trap_Argv( 1, arg, sizeof( arg ) );
		trap_Argv( 2, arg2, sizeof( arg2 ) );
		shipSurf = SHIPSURF_FRONT+atoi(arg);
		damageLevel = atoi(arg2);

		G_SetVehDamageFlags( &g_entities[ent->s.m_iVehicleNum], shipSurf, damageLevel );
	}
#endif
	else
	{
		if (Q_stricmp(cmd, "addbot") == 0)
		{ //because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
//			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
			trap_SendServerCommand( clientNum, va("print \"%s.\n\"", G_GetStringEdString("MP_SVGAME", "ONLY_ADD_BOTS_AS_SERVER")));
		}
		else
		{
			trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		}
	}
}
