//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:  Heads-up displays
//


#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"

#include "z_zone.h"

#include "deh_main.h"
#include "i_input.h"
#include "i_swap.h"
#include "i_video.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "m_controls.h"
#include "m_misc.h"
#include "w_wad.h"
#include "m_argv.h" // [crispy] M_ParmExists()
#include "st_stuff.h" // [crispy] ST_HEIGHT
#include "p_setup.h" // maplumpinfo

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "v_video.h" // [crispy] V_DrawPatch() et al.

#include <emscripten.h>

//
// Locally used constants, shortcuts.
//
#define HU_TITLE	(mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2	(mapnames_commercial[gamemap-1])
#define HU_TITLEP	(mapnames_commercial[gamemap-1 + 32])
#define HU_TITLET	(mapnames_commercial[gamemap-1 + 64])
#define HU_TITLEN	(mapnames_commercial[gamemap-1 + 96 + 3])
#define HU_TITLEM	(mapnames_commercial[gamemap-1 + 105 + 3])
#define HU_TITLE_CHEX   (mapnames_chex[(gameepisode-1)*9+gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE	't'
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1

#define HU_COORDX	(ORIGWIDTH - 7 * hu_font['A'-HU_FONTSTART]->width)

char *chat_macros[10] =
{
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char*	player_names[] =
{
    HUSTR_PLRGREEN,
    HUSTR_PLRINDIGO,
    HUSTR_PLRBROWN,
    HUSTR_PLRRED
};

char			chat_char; // remove later.
static player_t*	plr;
patch_t*		hu_font[HU_FONTSIZE];
static hu_textline_t	w_title;
boolean			chat_on;
static hu_itext_t	w_chat;
static boolean		always_off = false;
static char		chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static boolean		message_on;
boolean			message_dontfuckwithme;
static boolean		message_nottobefuckedwith;

static hu_stext_t	w_message;
static int		message_counter;

extern int		showMessages;

static boolean		headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char*	mapnames[] =	// DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

char*   mapnames_chex[] =   // Chex Quest names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

// List of names for levels in commercial IWADs
// (doom2.wad, plutonia.wad, tnt.wad).  These are stored in a
// single large array; WADs like pl2.wad have a MAP33, and rely on
// the layout in the Vanilla executable, where it is possible to
// overflow the end of one array into the next.

char *mapnames_commercial[] =
{
    // DOOM 2 map names.

    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,
    HUSTR_10,
    HUSTR_11,
	
    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,
    HUSTR_20,
	
    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,
    HUSTR_30,
    HUSTR_31,
    HUSTR_32,

    // Plutonia WAD map names.

    PHUSTR_1,
    PHUSTR_2,
    PHUSTR_3,
    PHUSTR_4,
    PHUSTR_5,
    PHUSTR_6,
    PHUSTR_7,
    PHUSTR_8,
    PHUSTR_9,
    PHUSTR_10,
    PHUSTR_11,
	
    PHUSTR_12,
    PHUSTR_13,
    PHUSTR_14,
    PHUSTR_15,
    PHUSTR_16,
    PHUSTR_17,
    PHUSTR_18,
    PHUSTR_19,
    PHUSTR_20,
	
    PHUSTR_21,
    PHUSTR_22,
    PHUSTR_23,
    PHUSTR_24,
    PHUSTR_25,
    PHUSTR_26,
    PHUSTR_27,
    PHUSTR_28,
    PHUSTR_29,
    PHUSTR_30,
    PHUSTR_31,
    PHUSTR_32,
    
    // TNT WAD map names.

    THUSTR_1,
    THUSTR_2,
    THUSTR_3,
    THUSTR_4,
    THUSTR_5,
    THUSTR_6,
    THUSTR_7,
    THUSTR_8,
    THUSTR_9,
    THUSTR_10,
    THUSTR_11,
	
    THUSTR_12,
    THUSTR_13,
    THUSTR_14,
    THUSTR_15,
    THUSTR_16,
    THUSTR_17,
    THUSTR_18,
    THUSTR_19,
    THUSTR_20,
	
    THUSTR_21,
    THUSTR_22,
    THUSTR_23,
    THUSTR_24,
    THUSTR_25,
    THUSTR_26,
    THUSTR_27,
    THUSTR_28,
    THUSTR_29,
    THUSTR_30,
    THUSTR_31,
    THUSTR_32,

    // Emulation: TNT maps 33-35 can be warped to and played if they exist
    // so include blank names instead of spilling over
    "",
    "",
    ""
    ,
    NHUSTR_1,
    NHUSTR_2,
    NHUSTR_3,
    NHUSTR_4,
    NHUSTR_5,
    NHUSTR_6,
    NHUSTR_7,
    NHUSTR_8,
    NHUSTR_9,

    MHUSTR_1,
    MHUSTR_2,
    MHUSTR_3,
    MHUSTR_4,
    MHUSTR_5,
    MHUSTR_6,
    MHUSTR_7,
    MHUSTR_8,
    MHUSTR_9,
    MHUSTR_10,
    MHUSTR_11,
    MHUSTR_12,
    MHUSTR_13,
    MHUSTR_14,
    MHUSTR_15,
    MHUSTR_16,
    MHUSTR_17,
    MHUSTR_18,
    MHUSTR_19,
    MHUSTR_20,
    MHUSTR_21
};

// [crispy] display names of single special levels in Automap
// These are single, non-consecutive, (semi-)official levels
// without their own music or par times and thus do not need
// to be handled as distinct pack_* game missions.
typedef struct
{
    GameMission_t mission;
    int episode;
    int map;
    const char *wad;
    const char *name;
} speciallevel_t;

static const speciallevel_t speciallevels[] = {
    // [crispy] ExM0
    {doom, 1, 0, NULL, NULL},
    {doom, 2, 0, NULL, NULL},
    {doom, 3, 0, NULL, NULL},
    {doom, 4, 0, NULL, NULL},
    // [crispy] Romero's latest E1 additions
    {doom, 1, 8, "e1m8b.wad", HUSTR_E1M8B},
    {doom, 1, 4, "e1m4b.wad", HUSTR_E1M4B},
    // [crispy] E1M10 "Sewers" (Xbox Doom)
    {doom, 1, 10, NULL, HUSTR_E1M10},
    // [crispy] The Master Levels for Doom 2
    {doom2, 0, 1, "attack.wad", MHUSTR_1},
    {doom2, 0, 1, "canyon.wad", MHUSTR_2},
    {doom2, 0, 1, "catwalk.wad", MHUSTR_3},
    {doom2, 0, 1, "combine.wad", MHUSTR_4},
    {doom2, 0, 1, "fistula.wad", MHUSTR_5},
    {doom2, 0, 1, "garrison.wad", MHUSTR_6},
    {doom2, 0, 1, "manor.wad", MHUSTR_7},
    {doom2, 0, 1, "paradox.wad", MHUSTR_8},
    {doom2, 0, 1, "subspace.wad", MHUSTR_9},
    {doom2, 0, 1, "subterra.wad", MHUSTR_10},
    {doom2, 0, 1, "ttrap.wad", MHUSTR_11},
    {doom2, 0, 3, "virgil.wad", MHUSTR_12},
    {doom2, 0, 5, "minos.wad", MHUSTR_13},
    {doom2, 0, 7, "bloodsea.wad", MHUSTR_14},
    {doom2, 0, 7, "mephisto.wad", MHUSTR_15},
    {doom2, 0, 7, "nessus.wad", MHUSTR_16},
    {doom2, 0, 8, "geryon.wad", MHUSTR_17},
    {doom2, 0, 9, "vesperas.wad", MHUSTR_18},
    {doom2, 0, 25, "blacktwr.wad", MHUSTR_19},
    {doom2, 0, 31, "teeth.wad", MHUSTR_20},
    {doom2, 0, 32, "teeth.wad", MHUSTR_21},
};

static void HU_SetSpecialLevelName (const char *wad, const char **name)
{
    int i;

    for (i = 0; i < arrlen(speciallevels); i++)
    {
	const speciallevel_t speciallevel = speciallevels[i];

	if (logical_gamemission == speciallevel.mission &&
	    (!speciallevel.episode || gameepisode == speciallevel.episode) &&
	    gamemap == speciallevel.map &&
	    (!speciallevel.wad || !strcasecmp(wad, speciallevel.wad)))
	{
	    *name = speciallevel.name ? speciallevel.name : maplumpinfo->name;
	    break;
	}
    }
}

void HU_Init(void)
{

    int		i;
    int		j;
    char	buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
	DEH_snprintf(buffer, 9, "STCFN%.3d", j++);
	hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

}

void HU_Stop(void)
{
    headsupactive = false;
}

EMSCRIPTEN_KEEPALIVE
const char *HU_GetMapName(){
    const char *s;
    // [crispy] string buffers for map title and WAD file name
    char	*ptr;

    switch ( logical_gamemission )
    {
      case doom:
	s = HU_TITLE;
	break;
      case doom2:
	 s = HU_TITLE2;
         // Pre-Final Doom compatibility: map33-map35 names don't spill over
         if (gameversion <= exe_doom_1_9 && gamemap >= 33)
         {
             s = "";
         }
	 break;
      case pack_plut:
	s = HU_TITLEP;
	break;
      case pack_tnt:
	s = HU_TITLET;
	break;
      default:
         s = "Unknown level";
         break;
    }

    if (logical_gamemission == doom && gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }

    // [crispy] display names of single special levels in Automap
    HU_SetSpecialLevelName(W_WadNameForLump(maplumpinfo), &s);

    // [crispy] explicitely display (episode and) map if the
    // map is from a PWAD or if the map title string has been dehacked
    if (strcmp(s, DEH_String(s)) || (!W_IsIWADLump(maplumpinfo) && (!nervewadfile || gamemission != pack_nerve)))
    {
	char *m;

	ptr = M_StringJoin(W_WadNameForLump(maplumpinfo), ": ", maplumpinfo->name, NULL);
	m = ptr;

	free(ptr);
    }

    // dehacked substitution to get modified level name

    s = DEH_String(s);

    return s;
}

void HU_Start(void)
{

    int		i;
    const char *s;
    // [crispy] string buffers for map title and WAD file name
    char	buf[8], *ptr;

    if (headsupactive)
	HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    chat_on = false;

    // create the message widget
    HUlib_initSText(&w_message,
		    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
		    hu_font,
		    HU_FONTSTART, &message_on);

    // create the map title widget
    HUlib_initTextLine(&w_title,
		       HU_TITLEX, HU_TITLEY,
		       hu_font,
		       HU_FONTSTART);
    
    switch ( logical_gamemission )
    {
      case doom:
	s = HU_TITLE;
	break;
      case doom2:
	 s = HU_TITLE2;
         // Pre-Final Doom compatibility: map33-map35 names don't spill over
         if (gameversion <= exe_doom_1_9 && gamemap >= 33)
         {
             s = "";
         }
	 break;
      case pack_plut:
	s = HU_TITLEP;
	break;
      case pack_tnt:
	s = HU_TITLET;
	break;
      case pack_nerve:
	if (gamemap <= 9)
	  s = HU_TITLEN;
	else
	  s = HU_TITLE2;
	break;
      case pack_master:
	if (gamemap <= 21)
	  s = HU_TITLEM;
	else
	  s = HU_TITLE2;
	break;
      default:
         s = "Unknown level";
         break;
    }

    if (logical_gamemission == doom && gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }

    // [crispy] display names of single special levels in Automap
    HU_SetSpecialLevelName(W_WadNameForLump(maplumpinfo), &s);

    // [crispy] explicitely display (episode and) map if the
    // map is from a PWAD or if the map title string has been dehacked
    if (strcmp(s, DEH_String(s)) || (!W_IsIWADLump(maplumpinfo) && (!nervewadfile || gamemission != pack_nerve)))
    {
	char *m;

	ptr = M_StringJoin(W_WadNameForLump(maplumpinfo), ": ", maplumpinfo->name, NULL);
	m = ptr;

	free(ptr);
    }

    // dehacked substitution to get modified level name

    s = DEH_String(s);
    // [crispy] print the map title in white from the first colon onward
    M_snprintf(buf, sizeof(buf), "%s", ":");
    ptr = M_StringReplace(s, ":", buf);
    s = ptr;
    
    while (*s)
	HUlib_addCharToTextLine(&w_title, *(s++));

    // create the chat widget
    HUlib_initIText(&w_chat,
		    HU_INPUTX, HU_INPUTY,
		    hu_font,
		    HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
	HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;

}

void HU_Drawer(void)
{

    HUlib_drawSText(&w_message);
    HUlib_drawIText(&w_chat);
    if (automapactive)
	HUlib_drawTextLine(&w_title, false);

}

void HU_Erase(void)
{

    HUlib_eraseSText(&w_message);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);

}

void HU_Ticker(void)
{

    int i, rc;
    char c;

    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
	message_on = false;
	message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme)
    {

	// display message if necessary
	if ((plr->message && !message_nottobefuckedwith)
	    || (plr->message && message_dontfuckwithme))
	{
	    HUlib_addMessageToSText(&w_message, 0, plr->message);
	    plr->message = 0;
	    message_on = true;
	    message_counter = HU_MSGTIMEOUT;
	    message_nottobefuckedwith = message_dontfuckwithme;
	    message_dontfuckwithme = 0;
	}

    }

    // check for incoming chat characters
    if (netgame)
    {
	for (i=0 ; i<MAXPLAYERS; i++)
	{
	    if (!playeringame[i])
		continue;
	    if (i != consoleplayer
		&& (c = players[i].cmd.chatchar))
	    {
		if (c <= HU_BROADCAST)
		    chat_dest[i] = c;
		else
		{
		    rc = HUlib_keyInIText(&w_inputbuffer[i], c);
		    if (rc && c == KEY_ENTER)
		    {
			if (w_inputbuffer[i].l.len
			    && (chat_dest[i] == consoleplayer+1
				|| chat_dest[i] == HU_BROADCAST))
			{
			    HUlib_addMessageToSText(&w_message,
						    DEH_String(player_names[i]),
						    w_inputbuffer[i].l.l);
			    
			    message_nottobefuckedwith = true;
			    message_on = true;
			    message_counter = HU_MSGTIMEOUT;
			    if ( gamemode == commercial )
			      S_StartSound(0, sfx_radio);
			    else
			      S_StartSound(0, sfx_tink);
			}
			HUlib_resetIText(&w_inputbuffer[i]);
		    }
		}
		players[i].cmd.chatchar = 0;
	    }
	}
    }

}

#define QUEUESIZE		128

static char	chatchars[QUEUESIZE];
static int	head = 0;
static int	tail = 0;


void HU_queueChatChar(char c)
{
    if (((head + 1) & (QUEUESIZE-1)) == tail)
    {
	plr->message = DEH_String(HUSTR_MSGU);
    }
    else
    {
	chatchars[head] = c;
	head = (head + 1) & (QUEUESIZE-1);
    }
}

char HU_dequeueChatChar(void)
{
    char c;

    if (head != tail)
    {
	c = chatchars[tail];
	tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
	c = 0;
    }

    return c;
}

static void StartChatInput(int dest)
{
    chat_on = true;
    HUlib_resetIText(&w_chat);
    HU_queueChatChar(HU_BROADCAST);

    I_StartTextInput(0, 8, SCREENWIDTH, 16);
}

static void StopChatInput(void)
{
    chat_on = false;
    I_StopTextInput();
}

boolean HU_Responder(event_t *ev)
{

    static char		lastmessage[HU_MAXLINELENGTH+1];
    char*		macromessage;
    boolean		eatkey = false;
    static boolean	altdown = false;
    unsigned char 	c;
    int			i;
    int			numplayers;
    
    static int		num_nobrainers = 0;

    numplayers = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
	numplayers += playeringame[i];

    if (ev->data1 == KEY_RSHIFT)
    {
	return false;
    }
    else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
    {
	altdown = ev->type == ev_keydown;
	return false;
    }

    if (ev->type != ev_keydown)
	return false;

    if (!chat_on)
    {
	if (ev->data1 == key_message_refresh)
	{
	    message_on = true;
	    message_counter = HU_MSGTIMEOUT;
	    eatkey = true;
	}
	else if (netgame && ev->data2 == key_multi_msg)
	{
	    eatkey = true;
            StartChatInput(HU_BROADCAST);
	}
	else if (netgame && numplayers > 2)
	{
	    for (i=0; i<MAXPLAYERS ; i++)
	    {
		if (ev->data2 == key_multi_msgplayer[i])
		{
		    if (playeringame[i] && i!=consoleplayer)
		    {
			eatkey = true;
                        StartChatInput(i + 1);
			break;
		    }
		    else if (i == consoleplayer)
		    {
			num_nobrainers++;
			if (num_nobrainers < 3)
			    plr->message = DEH_String(HUSTR_TALKTOSELF1);
			else if (num_nobrainers < 6)
			    plr->message = DEH_String(HUSTR_TALKTOSELF2);
			else if (num_nobrainers < 9)
			    plr->message = DEH_String(HUSTR_TALKTOSELF3);
			else if (num_nobrainers < 32)
			    plr->message = DEH_String(HUSTR_TALKTOSELF4);
			else
			    plr->message = DEH_String(HUSTR_TALKTOSELF5);
		    }
		}
	    }
	}
    }
    else
    {
	// send a macro
	if (altdown)
	{
	    c = ev->data1 - '0';
	    if (c > 9)
		return false;
	    // fprintf(stderr, "got here\n");
	    macromessage = chat_macros[c];

	    // kill last message with a '\n'
	    HU_queueChatChar(KEY_ENTER); // DEBUG!!!

	    // send the macro message
	    while (*macromessage)
		HU_queueChatChar(*macromessage++);
	    HU_queueChatChar(KEY_ENTER);

            // leave chat mode and notify that it was sent
            StopChatInput();
            M_StringCopy(lastmessage, chat_macros[c], sizeof(lastmessage));
            plr->message = lastmessage;
            eatkey = true;
	}
	else
	{
            c = ev->data3;

	    eatkey = HUlib_keyInIText(&w_chat, c);
	    if (eatkey)
	    {
		HU_queueChatChar(c);
	    }
	    if (c == KEY_ENTER)
	    {
		StopChatInput();
                if (w_chat.l.len)
                {
                    M_StringCopy(lastmessage, w_chat.l.l, sizeof(lastmessage));
                    plr->message = lastmessage;
                }
	    }
	    else if (c == KEY_ESCAPE)
	    {
                StopChatInput();
            }
	}
    }

    return eatkey;
}
