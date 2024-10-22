// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomstat.h 26369 2010-05-29 08:28:29Z jethead71 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_net.h"

// We need the playr data structure as well.
#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif


// ------------------------
// Command line parameters.
//
extern  boolean nomonsters; // checkparm of -nomonsters
extern  boolean respawnparm; // checkparm of -respawn
extern  boolean fastparm; // checkparm of -fast

extern  boolean devparm; // DEBUG: launched with -devparm

enum {
   am_active = 1,  // currently shown
   am_overlay= 2,  // covers the screen, i.e. not overlay mode
   am_rotate = 4,  // rotates to the player facing direction
   am_follow = 8,  // keep the player centred
   am_grid   =16,  // show grid
};
typedef unsigned automapmode_e;
extern automapmode_e automapmode; // Mode that the automap is in

// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t gamemode;
extern GameMission_t gamemission;

// Set if homebrew PWAD stuff has been added.
extern  boolean modifiedgame;

// CPhipps - new compatibility handling
extern complevel_t compatibility_level, default_compatibility_level;

// CPhipps - old compatibility testing flags aliased to new handling
#define compatibility (compatibility_level<=boom_compatibility_compatibility)
#define demo_compatibility (compatibility_level <= doom_demo_compatibility)
#define mbf_features (compatibility_level>=mbf_compatibility)

extern int default_translucency;
extern boolean general_translucency; // true if translucency is ok // phares

extern int demo_insurance, default_demo_insurance;      // killough 4/5/98

// -------------------------------------------
// killough 10/98: compatibility vector

enum {
   comp_telefrag,
   comp_dropoff,
   comp_vile,
   comp_pain,
   comp_skull,
   comp_blazing,
   comp_doorlight,
   comp_model,
   comp_god,
   comp_falloff,
   comp_floors,
   comp_skymap,
   comp_pursuit,
   comp_doorstuck,
   comp_staylift,
   comp_zombie,
   comp_stairs,
   comp_infcheat,
   comp_zerotags,
   comp_moveblock,
   comp_respawn,  /* cph - this is the inverse of comp_respawnfix from eternity */
   comp_sound,
   COMP_NUM,      /* cph - should be last in sequence */
   COMP_TOTAL=32  // Some extra room for additional variables
};

extern int comp[COMP_TOTAL], default_comp[COMP_TOTAL];

extern boolean enable_sound;
extern boolean rockblock;

// -------------------------------------------
// Language.
extern  Language_t   language;


// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t  startskill;
extern  int             startepisode;
extern  int  startmap;

extern  boolean  autostart;

// Selected by user.
extern  skill_t         gameskill;
extern  int  gameepisode;
extern  int  gamemap;

// Nightmare mode flag, single player.
extern  boolean         respawnmonsters;

// Netgame? Only true if >1 player.
extern  boolean netgame;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern  boolean deathmatch;

// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.

// These are not used, but should be (menu).
// From m_menu.c:
//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
extern int snd_SfxVolume;      // maximum volume for sound
extern int snd_MusicVolume;    // maximum volume for music

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  boolean statusbaractive;

extern  boolean automapactive; // In AutoMap mode?
extern  boolean menuactive; // Menu overlayed?
extern  boolean paused;  // Game Pause?


extern  boolean  viewactive;

extern  boolean  nodrawers;
extern  boolean  noblit;

extern int  viewwindowx;
extern int  viewwindowy;
extern int  viewheight;
extern int  viewwidth;
extern int  scaledviewwidth;

// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int viewangleoffset;

// Player taking events, and displaying.
extern  int consoleplayer;
extern  int displayplayer;


// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int totalkills, totallive;
extern int totalitems;
extern int totalsecret;

// Timer, for scores.
extern  int levelstarttic; // gametic at level start
extern  int basetic;    /* killough 9/29/98: levelstarttic, adjusted */
extern  int leveltime; // tics in game play for par



// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern  boolean usergame;

//?
extern  boolean demoplayback;
extern  boolean demorecording;

// Quit after playing a demo from cmdline.
extern  boolean  singledemo;




//?
extern  gamestate_t     gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern int  gametic;

// Bookkeeping on players - state.
extern player_t players[MAXPLAYERS];

// Alive? Disconnected?
extern  boolean  playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
#define MAX_DM_STARTS   10
//extern  mapthing_t      deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t *deathmatchstarts;     // killough
extern  size_t     num_deathmatchstarts; // killough
extern  mapthing_t* deathmatch_p;

// Player spawn spots.
extern  mapthing_t      playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t  wminfo;


// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern  int  maxammo[NUMAMMO];

//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern char  basedefault[];
extern  int  debugfile;

// if true, load all graphics at level load
extern  boolean         precache;


// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t     wipegamestate;

extern  int             mouseSensitivity;
//?
// debug flag to cancel adaptiveness
extern  boolean         singletics;

extern  int             bodyqueslot;



// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern int  skyflatnum;

// Netgame stuff (buffers and pointers, i.e. indices).
extern  doomcom_t* doomcom;
extern  doomdata_t* netbuffer;   // This points inside doomcom.

extern int  rndindex;

extern int  maketic;

extern  ticcmd_t        netcmds[][BACKUPTICS];

extern int  ticdup;

extern  int             nettics[MAXNETNODES];

//-----------------------------------------------------------------------------

// v1.1-like pitched sounds
extern int pitched_sounds;     // killough 2/21/98

extern int allow_pushers;         // MT_PUSH Things    // phares 3/10/98
extern int default_allow_pushers;

extern int variable_friction;  // ice & mud            // phares 3/10/98
extern int default_variable_friction;

extern int monsters_remember;                          // killough 3/1/98
extern int default_monsters_remember;

extern int weapon_recoil;          // weapon recoil    // phares
extern int default_weapon_recoil;

extern int player_bobbing;  // whether player bobs or not   // phares 2/25/98
extern int default_player_bobbing;  // killough 3/1/98: make local to each game

#ifdef DOGS
extern int dogs, default_dogs;     // killough 7/19/98: Marine's best friend :)
extern int dog_jumping, default_dog_jumping;   // killough 10/98
#endif

/* killough 8/8/98: distance friendly monsters tend to stay from player */
extern int distfriend, default_distfriend;

/* killough 9/8/98: whether monsters are allowed to strafe or retreat */
extern int monster_backing, default_monster_backing;

/* killough 9/9/98: whether monsters intelligently avoid hazards */
extern int monster_avoid_hazards, default_monster_avoid_hazards;

/* killough 10/98: whether monsters are affected by friction */
extern int monster_friction, default_monster_friction;

/* killough 9/9/98: whether monsters help friends */
extern int help_friends, default_help_friends;

extern int flashing_hom; // killough 10/98

extern int doom_weapon_toggles;   // killough 10/98

/* killough 7/19/98: whether monsters should fight against each other */
extern int monster_infighting, default_monster_infighting;

extern int monkeys, default_monkeys;


#endif
