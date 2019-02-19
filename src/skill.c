/* Crimson2 Mud Server
 * All source written/copyright Ryan Haksi 1995 *
 * This source code is proprietary. Use in whole or in part without
 * explicity permission by the author is strictly prohibited
 *
 * Current email address(es): cryogen@infoserve.net
 * Phone number: (604) 591-5295
 *
 * C4 Script Language written/copyright Cam Lesiuk 1995
 * Email: clesiuk@engr.uvic.ca
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "crimson2.h"
#include "macro.h"
#include "log.h"
#include "str.h"
#include "send.h"
#include "queue.h"
#include "thing.h"
#include "base.h"
#include "char.h"
#include "player.h"
#include "cmd_inv.h"
#include "skill.h"

/* types of skills */
BYTE *skillFlagList[] = {
  "*WEAPON*",
  "*MELEE*",
  "*LASER*",
  "*SLUG*",
  "*BLASTER*",
  "*ION*",
  "*SUPPORT*",
  "*PLASMA*",
  "*OFFENSE*",
  "*DEFENSE*",
  "*PSYCHIC*",
  "*BODY*",
  "*TELEKINETIC*",
  "*APPORTATION*",
  "*SPIRIT*",
  "*PYROKINETIC*",
  "*TELEPATHY*",
  "*GENERAL*",
  "*CLASS*",
  "*ALL*",
  "*GOOD*",
  "*EVIL*",
  ""
};

/* Understanding how skills work:
 * skill values range from 0-250
 * in general a skill value denotes:
 * percentage (0-250%) ie lockpicking, spells (% affects duration/damage/cost etc)
 * bonus ie 0.0-25.0 generally (weapon skills bonus to hit/damage)
 * the levels built into the skillist table are only the defaults, they can be overridden
 * to any specific values by the class/race skills (and they are deceptive because unless

 * skill at default level+100 (max player level is 200)
 * if the skill is flagged as bad, then its maximums etc are worse
 * if the skill is flagged as good, then its gained @ lower level etc 
*/

/* The following is all the skill constants, I got sick of updating 
 * a huge number of constants so instead I changed them to variables
 * and copy the offsets in to the variables so I dont have to worry 
 * about where they are in the table when I edit it
 */
WORD SKILL_MELEE_ACCURACY=0;
WORD SKILL_MELEE_DAMAGE=0;
WORD SKILL_MELEE_SPEED=0;

WORD SKILL_PISTOL_ACCURACY=0;
WORD SKILL_PISTOL_DAMAGE=0;
WORD SKILL_PISTOL_SPEED=0;

WORD SKILL_RIFLE_ACCURACY=0;
WORD SKILL_RIFLE_DAMAGE=0;
WORD SKILL_RIFLE_SPEED=0;

WORD SKILL_CANNON_ACCURACY=0;
WORD SKILL_CANNON_DAMAGE=0;
WORD SKILL_CANNON_SPEED=0;

WORD SKILL_MELEE_KNIFE=0;
WORD SKILL_MELEE_BLADE=0;
WORD SKILL_MELEE_BLUDGEON=0;
WORD SKILL_MELEE_AXE=0;

WORD SKILL_LASER_PISTOL=0;
WORD SKILL_SLUG_PISTOL=0;
WORD SKILL_SLUG_MACHINE=0;
WORD SKILL_BLASTER_PISTOL=0;

WORD SKILL_LASER_RIFLE=0;
WORD SKILL_SLUG_RIFLE=0;
WORD SKILL_BLASTER_RIFLE=0;
WORD SKILL_ION_RIFLE=0;
WORD SKILL_GRENADE_RIFLE=0;
WORD SKILL_PLASMA_RIFLE=0;

WORD SKILL_BLASTER_CANNON=0;
WORD SKILL_ION_CANNON=0;
WORD SKILL_MISSILE_CANNON=0;
WORD SKILL_PLASMA_CANNON=0;

/* metabolic skills */
WORD SKILL_CELL_REPAIR;
WORD SKILL_REFRESH=0;
WORD SKILL_ENDURANCE=0;
WORD SKILL_BREATHWATER=0;
WORD SKILL_STRENGTH=0;
WORD SKILL_DARKVISION=0;
WORD SKILL_SLOW_POISON=0;
WORD SKILL_CURE_POISON=0;
WORD SKILL_HEAL_MINOR=0;
WORD SKILL_REGENERATION=0;
WORD SKILL_HEAL_MAJOR=0;
WORD SKILL_DEXTERITY=0;
WORD SKILL_CONSTITUTION=0;
WORD SKILL_HASTE=0;
WORD SKILL_QUENCH=0;
WORD SKILL_SUSTENANCE=0;
WORD SKILL_ACIDTOUCH=0;
WORD SKILL_POISONTOUCH=0;

/* telekinetic skills */
WORD SKILL_CRUSH=0;
WORD SKILL_FORCESTORM=0;
WORD SKILL_GHOSTFIST=0;
WORD SKILL_KINETIC_SHIELD=0;
WORD SKILL_IMMOBILIZE=0;
WORD SKILL_HEARTSTOP=0;
WORD SKILL_ASPHYXIATE=0;
WORD SKILL_INVISIBILITY=0;
WORD SKILL_SLOW=0;        
WORD SKILL_IMPROVEDINVIS=0;
WORD SKILL_VACUUMWALK=0;
/* WORD SKILL_KINETICWEAPON - weapon does concussive damage too */

/* telepathic skills */
WORD SKILL_PHANTOMEAR=0;
WORD SKILL_PHANTOMEYE=0;
WORD SKILL_MINDLINK=0;
WORD SKILL_DOMINATION=0;
WORD SKILL_THOUGHTBLADE=0;
WORD SKILL_MINDCRUSH=0;
WORD SKILL_DEATHDREAM=0;
WORD SKILL_MINDSHIELD=0;
WORD SKILL_SLEEP=0;
WORD SKILL_BERSERK=0;
WORD SKILL_MINDCLEAR=0;
/* WORD SKILL_IRONWILL        SKILL_SLEEP+1  increase willpower for awhile */
/* WORD SKILL_FORCE           SKILL_IRONWILL+1 order command */
/* WORD SKILL_FEEBLEMIND      SKILL_FORCE+1  harder to work psi powers */
/* WORD SKILL_CONFUSE         SKILL_FEEBLEMIND+1  reduce perception/guard */
/* WORD SKILL_COUNTERSTRIKE   attacker feels our pain (mind damage) */
/* WORD SKILL_MESSAGE         talk to sleeping people even */
/* WORD SKILL_SLEEPWATCH      talk/watch while "sleeping" */

/* apportation skills */
WORD SKILL_TELEPORT=0;      /* go somewhere */
WORD SKILL_SUMMON=0;        /* bring someone here */
WORD SKILL_SUCCOR=0;        /* go to someone */
WORD SKILL_BANISH=0;        /* bounce someone a few rooms away */
WORD SKILL_MASSTELEPORT=0;  /* everyone with me to dest */
WORD SKILL_MASSSUCCOR=0;    /* everyone to leader */
WORD SKILL_MASSSUMMON=0;    /* everyone to me */
WORD SKILL_DISRUPTDOOR=0;   /* destroy a teleportal */
WORD SKILL_PHASEWALK=0;     /* walk out of phase, no terrain costs */
WORD SKILL_PHANTOMPOCKET=0; /* can carry stuff in this doesnt weigh anything */
WORD SKILL_WATERWALK=0;     /* dont need a boat on water */
WORD SKILL_TELETRACK=0;     /* go to someone nearby */
WORD SKILL_RECALL=0;        /* go to marked location */
WORD SKILL_MARK=0;          /* mark this location */
WORD SKILL_TRANSLOCATE=0;   /* Swap goto marked location but mark this one */
WORD SKILL_PHASEDOOR=0;      /* Walk through closed door */
/* WORD SKILL_PHANTOMDOOR=0;   -* create a portal you can enter */
/* WORD SKILL_BLIT  go a few rooms in a direction */
/* WORD SKILL_TELESHIELD more than nosummon prevent people from coming */
/* WORD SKILL_FLICKER flicker in and out reduce damage by 25% */

/* Spirit skills */
WORD SKILL_SPIRITWALK=0;    /* set spirit free */
WORD SKILL_SENSELIFE=0;     
WORD SKILL_SEEINVISIBLE=0;  
WORD SKILL_LUCKSHIELD=0;    
WORD SKILL_IDENTIFY=0;      /* identify an object */
WORD SKILL_STAT=0;          /* stat a creature resist, hitroll etc */
WORD SKILL_LUCKYHITS=0;     /* hit bonus */
WORD SKILL_LUCKYDAMAGE=0;   /* damage bonus */
/* WORD SKILL_SECURE - make things hard to find/picklock */
/* WORD SKILL_PSI_RESIST - disrupt psionics */
/* WORD SKILL_PSI_IMMUNE - disrupt psionics completely */
/* WORD SKILL_LUCKYDODGE - improve dodge chances */
/* WORD SKILL_ESP - identify what someone is concentrating on */
/* WORD SKILL_LOCATE_OBJECT - locate some objects */
/* WORD SKILL_POWERSTEAL - steal power points */
/* WORD SKILL_TIMESTOP - decrement tWait, POWERFULL!! */

/* Pyrokinesis */
WORD SKILL_BURNINGFIST=0;   
WORD SKILL_FLAMESTRIKE=0;   
WORD SKILL_INCINERATE=0;    
WORD SKILL_IGNITE=0;        /* set off grenades */
WORD SKILL_HEATSHIELD=0;    
WORD SKILL_FIRE_ACCURACY=0;
WORD SKILL_FIRE_DAMAGE=0;   
WORD SKILL_FIRE_SPEED=0;    
WORD SKILL_FIREBLADE=0;     /* make a burning sword we can wield */
WORD SKILL_FIRESHIELD=0;    /* make a burning shield we can wield */
WORD SKILL_FIREARMOR=0;     /* make burning armor we can wield */
/* WORD SKILL_FLAMEWEAPON=0    weapon is burning does heat damage too */
/* WORD SKILL_BURNINGAURA=0    every round attacker take heat damage */

WORD SKILL_FLEE=0;
WORD SKILL_PURSUE=0;
WORD SKILL_RESCUE=0;
WORD SKILL_PICKLOCK=0;
WORD SKILL_HACKING=0;
WORD SKILL_SEARCH=0;
WORD SKILL_CONCEAL=0;
WORD SKILL_HIDE=0;
WORD SKILL_DODGE=0;
WORD SKILL_WILLPOWER=0;
WORD SKILL_TRACK=0;
WORD SKILL_SNEAK=0;
WORD SKILL_DISARM=0;
WORD SKILL_AMBUSH=0;
WORD SKILL_SPEED=0;
WORD SKILL_PERCEPTION=0;
WORD SKILL_PICKPOCKET=0;
WORD SKILL_PEEK=0;
WORD SKILL_STEAL=0;
WORD SKILL_GUARD=0;

/* the master list of all skills in play */
  /* Name */               /* Skill flags */                            /* Level */    /* Max */          /* Gain Divisor*/
struct SkillListType skillList[] = {
  { "<Undefined>",        0,                                            {254,254,254,254}, {0,0,0,0},    {99,99,99,99}, NULL },

  { "Melee-Accuracy",     SF_WEAPON|SF_MELEE,                           {10,25,55,80}, {20,100,125,200}, {10,15,40,70}, &SKILL_MELEE_ACCURACY },
  { "Melee-Damage",       SF_WEAPON|SF_MELEE,                           {15,30,60,85}, {20, 90,125,200}, {15,25,55,75}, &SKILL_MELEE_DAMAGE },
  { "Melee-Speed",        SF_WEAPON|SF_MELEE,                           {20,35,65,99}, {20,100,150,200}, {20,35,60,99}, &SKILL_MELEE_SPEED },

  { "Pistol-Accuracy",    SF_WEAPON,                                    {10,25,55,75}, {20,100,150,200}, {10,15,40,70}, &SKILL_PISTOL_ACCURACY },
  { "Pistol-Damage",      SF_WEAPON,                                    {15,30,60,80}, {20, 90,125,150}, {15,25,55,75}, &SKILL_PISTOL_DAMAGE },
  { "Pistol-Speed",       SF_WEAPON,                                    {20,35,65,99}, {20,100,150,200}, {20,35,60,99}, &SKILL_PISTOL_SPEED },

  { "Rifle-Accuracy",     SF_WEAPON,                                    {20,35,60,85}, {20,100,150,200}, {10,15,40,70}, &SKILL_RIFLE_ACCURACY },
  { "Rifle-Damage",       SF_WEAPON,                                    {25,40,65,90}, {20, 90,125,150}, {15,25,55,75}, &SKILL_RIFLE_DAMAGE },
  { "Rifle-Speed",        SF_WEAPON,                                    {30,45,70,99}, {20,100,150,200}, {20,35,60,99}, &SKILL_RIFLE_SPEED },

  { "Cannon-Accuracy",    SF_WEAPON,                                    {50,60,75,99}, {20,100,150,200}, {10,20,40,70}, &SKILL_CANNON_ACCURACY },
  { "Cannon-Damage",      SF_WEAPON,                                    {55,65,80,99}, {20, 90,125,150}, {15,25,55,75}, &SKILL_CANNON_DAMAGE },
  { "Cannon-Speed",       SF_WEAPON,                                    {60,70,85,99}, {20,100,150,200}, {20,35,60,99}, &SKILL_CANNON_SPEED },

  { "Melee-Knife",        SF_WEAPON|SF_MELEE,                           { 1,15,40,70}, {20,100,150,200}, {10,15,30,60}, &SKILL_MELEE_KNIFE },
  { "Melee-Blade",        SF_WEAPON|SF_MELEE,                           {10,20,45,75}, {20,100,150,200}, {10,15,30,60}, &SKILL_MELEE_BLADE },
  { "Melee-Bludgeon",     SF_WEAPON|SF_MELEE,                           {15,25,50,80}, {20,100,150,200}, {10,15,30,60}, &SKILL_MELEE_BLUDGEON },
  { "Melee-Axe",          SF_WEAPON|SF_MELEE,                           {20,35,55,85}, {20,100,150,200}, {10,15,30,60}, &SKILL_MELEE_AXE },
                                                                                                                                       
  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Laser-Pistol",       SF_WEAPON|SF_LASER,                           { 1,15,35,65}, {20,100,150,200}, {10,15,30,60}, &SKILL_LASER_PISTOL },
  { "Slug-Pistol",        SF_WEAPON|SF_SLUG,                            {10,20,40,70}, {20,100,150,200}, {10,15,30,60}, &SKILL_SLUG_PISTOL },
  { "Slug-MachinePistol", SF_WEAPON|SF_SLUG,                            {15,25,45,75}, {20,100,150,200}, {10,15,30,60}, &SKILL_SLUG_MACHINE },
  { "Blaster-Pistol",     SF_WEAPON|SF_BLASTER,                         {25,30,50,80}, {20,100,150,200}, {10,15,30,60}, &SKILL_BLASTER_PISTOL },
                                                                                                                                       
  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Laser-Rifle",        SF_WEAPON|SF_LASER,                           {15,25,45,75}, {30,100,150,200}, {10,15,30,60}, &SKILL_LASER_RIFLE },
  { "Slug-Rifle",         SF_WEAPON|SF_SLUG,                            {20,30,50,75}, {30,100,150,200}, {10,15,30,60}, &SKILL_SLUG_RIFLE },
  { "Blaster-Rifle",      SF_WEAPON|SF_BLASTER,                         {25,30,50,80}, {30,100,150,200}, {10,15,30,60}, &SKILL_BLASTER_RIFLE },
  { "Ion-Rifle",          SF_WEAPON|SF_ION,                             {30,40,60,90}, {30,100,150,200}, {10,15,30,60}, &SKILL_ION_RIFLE },
  { "Grenade-Rifle",      SF_WEAPON|SF_SUPPORT,                         {40,50,70,95}, {30,100,150,200}, {10,15,30,60}, &SKILL_GRENADE_RIFLE },
  { "Plasma-Rifle",       SF_WEAPON|SF_PLASMA,                          {50,60,70,99}, {30,100,150,200}, {10,15,30,60}, &SKILL_PLASMA_RIFLE },
                                                                                                                                       
  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Blaster-Cannon",     SF_WEAPON|SF_BLASTER,                         {45,50,70,99}, {30,100,150,200}, {10,17,35,65}, &SKILL_BLASTER_CANNON },
  { "Ion-Cannon",         SF_WEAPON|SF_ION,                             {60,65,75,99}, {30,100,150,200}, {10,17,35,65}, &SKILL_ION_CANNON },
  { "Missile-Cannon",     SF_WEAPON|SF_SUPPORT,                         {70,75,85,99}, {30,100,150,200}, {10,17,35,65}, &SKILL_MISSILE_CANNON },
  { "Plasma-Cannon",      SF_WEAPON|SF_PLASMA,                          {80,85,90,99}, {30,100,150,200}, {10,17,35,65}, &SKILL_PLASMA_CANNON },
                                                                                                                                       
  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Cell-Repair",        SF_PSYCHIC|SF_BODY|SF_GOOD,                   { 8,18,33,53}, {30,150,200,250}, {10,17,35,65}, &SKILL_CELL_REPAIR },
  { "Refresh",            SF_PSYCHIC|SF_BODY|SF_GOOD,                   { 5,15,30,60}, {30,150,200,250}, {10,17,35,65}, &SKILL_REFRESH },
  { "Endurance",          SF_PSYCHIC|SF_BODY,                           { 7,17,27,57}, {30,150,200,250}, {10,17,45,75}, &SKILL_ENDURANCE },
  { "Quench",             SF_PSYCHIC|SF_BODY,                           { 9,19,29,59}, {30,150,200,250}, {10,17,45,75}, &SKILL_QUENCH },
  { "Sustenance",         SF_PSYCHIC|SF_BODY,                           { 9,19,29,59}, {30,150,200,250}, {10,17,45,75}, &SKILL_SUSTENANCE },
  { "Breathwater",        SF_PSYCHIC|SF_BODY,                           {10,20,30,60}, {30,150,200,250}, {10,17,45,75}, &SKILL_BREATHWATER },
  { "Acidtouch",          SF_PSYCHIC|SF_BODY|SF_OFFENSE|SF_EVIL,        {12,22,42,62}, {20,125,150,175}, {10,17,45,75}, &SKILL_ACIDTOUCH },
  { "Strength",           SF_PSYCHIC|SF_BODY,                           {15,25,35,65}, {30,150,200,250}, {10,17,45,75}, &SKILL_STRENGTH },
  { "Constitution",       SF_PSYCHIC|SF_BODY,                           {17,27,37,67}, {30,150,200,250}, {10,17,45,75}, &SKILL_CONSTITUTION },
  { "Darkvision",         SF_PSYCHIC|SF_BODY,                           {20,30,50,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_DARKVISION },
  { "Slow-Poison",        SF_PSYCHIC|SF_BODY,                           {22,24,27,33}, {30,150,200,250}, {10,17,45,75}, &SKILL_SLOW_POISON },
  { "Poisontouch",        SF_PSYCHIC|SF_BODY|SF_OFFENSE|SF_EVIL,        {22,32,52,72}, {20,125,150,175}, {10,17,45,75}, &SKILL_POISONTOUCH },
  { "Haste",              SF_PSYCHIC|SF_BODY,                           {25,35,45,75}, {30,150,200,250}, {15,40,75,99}, &SKILL_HASTE },
  { "Dexterity",          SF_PSYCHIC|SF_BODY,                           {27,37,47,87}, {30,150,200,250}, {10,17,45,75}, &SKILL_DEXTERITY },
  { "Cure-Poison",        SF_PSYCHIC|SF_BODY|SF_GOOD,                   {35,45,55,85}, {30,150,200,250}, {10,17,45,75}, &SKILL_CURE_POISON },
  { "Heal-Minor",         SF_PSYCHIC|SF_BODY|SF_GOOD,                   {40,50,60,90}, {30,150,200,250}, {10,17,45,75}, &SKILL_HEAL_MINOR },
  { "Regeneration",       SF_PSYCHIC|SF_BODY,                           {45,55,75,95}, {30,150,200,250}, {10,17,45,75}, &SKILL_REGENERATION },
  { "Heal-Major",         SF_PSYCHIC|SF_BODY|SF_GOOD,                   {75,80,90,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_HEAL_MAJOR },

  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Ghostfist",          SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE|SF_EVIL, { 5,20,40,60}, {30,150,200,250}, {10,17,45,75}, &SKILL_GHOSTFIST },
  { "Invisibility",       SF_PSYCHIC|SF_TELEKINETIC,                    {10,20,30,60}, {30,150,200,250}, {10,17,45,75}, &SKILL_INVISIBILITY },
  { "Slow",               SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE,         {12,22,32,62}, {30,150,200,250}, {10,17,45,75}, &SKILL_SLOW },
  { "Vacuumwalk",         SF_PSYCHIC|SF_TELEKINETIC,                    {15,25,35,65}, {30,150,200,250}, {10,17,45,75}, &SKILL_VACUUMWALK },
  { "Kinetic-Shield",     SF_PSYCHIC|SF_TELEKINETIC|SF_DEFENSE,         {17,25,35,65}, {30,150,200,250}, {10,17,45,75}, &SKILL_KINETIC_SHIELD },
  { "Forcestorm",         SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE|SF_EVIL, {20,30,55,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_FORCESTORM },
  { "Immobilize",         SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE,         {22,32,52,72}, {30,150,200,250}, {10,17,45,75}, &SKILL_IMMOBILIZE },
  { "ImprovedInvis",      SF_PSYCHIC|SF_TELEKINETIC,                    {25,35,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_IMPROVEDINVIS },
  { "Crush",              SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE|SF_EVIL, {40,50,70,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_CRUSH },
  { "Asphyxiate",         SF_PSYCHIC|SF_TELEKINETIC|SF_EVIL,            {45,55,75,95}, {30,150,200,250}, {10,17,45,75}, &SKILL_ASPHYXIATE },
  { "Heartstop",          SF_PSYCHIC|SF_TELEKINETIC|SF_OFFENSE|SF_EVIL, {75,80,90,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_HEARTSTOP },
                                                                                                                                       
  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "PhantomEar",         SF_PSYCHIC|SF_TELEPATHY,                      { 7,17,27,57}, {30,150,200,250}, {10,17,45,75}, &SKILL_PHANTOMEAR },
  { "Thoughtblade",       SF_PSYCHIC|SF_TELEPATHY|SF_OFFENSE|SF_EVIL,   { 5,20,40,60}, {30,150,200,250}, {10,17,45,75}, &SKILL_THOUGHTBLADE },
  { "PhantomEye",         SF_PSYCHIC|SF_TELEPATHY,                      {12,22,42,72}, {30,150,200,250}, {10,17,45,75}, &SKILL_PHANTOMEYE },
  { "Mindshield",         SF_PSYCHIC|SF_TELEPATHY|SF_DEFENSE,           {15,25,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_MINDSHIELD },
  { "Sleep",              SF_PSYCHIC|SF_TELEPATHY,                      {17,27,37,67}, {30,150,200,250}, {10,17,45,75}, &SKILL_SLEEP },
  { "Mindclear",          SF_PSYCHIC|SF_TELEPATHY,                      {19,29,39,69}, {30,150,200,250}, {10,17,45,75}, &SKILL_MINDCLEAR },
  { "Mindcrush",          SF_PSYCHIC|SF_TELEPATHY|SF_OFFENSE|SF_EVIL,   {21,30,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_MINDCRUSH },
  { "Domination",         SF_PSYCHIC|SF_TELEPATHY|SF_OFFENSE|SF_EVIL,   {23,32,52,82}, {30,150,200,250}, {10,17,45,75}, &SKILL_DOMINATION },
  { "Mindlink",           SF_PSYCHIC|SF_TELEPATHY,                      {25,35,55,85}, {30,150,200,250}, {10,17,45,75}, &SKILL_MINDLINK },
  { "Berserk",            SF_PSYCHIC|SF_TELEPATHY,                      {30,40,50,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_BERSERK },
  { "Deathdream",         SF_PSYCHIC|SF_TELEPATHY|SF_OFFENSE|SF_EVIL,   {40,50,70,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_DEATHDREAM },

  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Phantompocket",      SF_PSYCHIC|SF_APPORTATION,                    {10,20,40,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_PHANTOMPOCKET },
  { "Recall",             SF_PSYCHIC|SF_APPORTATION,                    {13,23,43,73}, {30,150,200,250}, {10,17,45,75}, &SKILL_RECALL },
  { "Mark",               SF_PSYCHIC|SF_APPORTATION,                    {13,23,43,73}, {30,150,200,250}, {10,17,45,75}, &SKILL_MARK },
  { "Waterwalk",          SF_PSYCHIC|SF_APPORTATION,                    {14,24,44,74}, {30,150,200,250}, {10,17,45,75}, &SKILL_WATERWALK },
  { "Phasewalk",          SF_PSYCHIC|SF_APPORTATION,                    {10,17,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_PHASEWALK },
  { "Teletrack",          SF_PSYCHIC|SF_APPORTATION,                    {18,27,47,77}, {30,150,200,250}, {10,17,45,75}, &SKILL_TELETRACK },
  { "Translocate",        SF_PSYCHIC|SF_APPORTATION,                    {20,29,49,79}, {30,150,200,250}, {10,17,45,75}, &SKILL_TRANSLOCATE },
  { "Succor",             SF_PSYCHIC|SF_APPORTATION,                    {22,30,50,80}, {30,150,200,250}, {10,17,45,75}, &SKILL_SUCCOR },
  { "DisruptDoor",        SF_PSYCHIC|SF_APPORTATION,                    {24,32,42,82}, {30,150,200,250}, {10,17,45,75}, &SKILL_DISRUPTDOOR },
  { "Summon",             SF_PSYCHIC|SF_APPORTATION,                    {25,35,55,85}, {30,150,200,250}, {10,17,45,75}, &SKILL_SUMMON },
  { "Banish",             SF_PSYCHIC|SF_APPORTATION,                    {30,40,60,90}, {30,150,200,250}, {10,17,45,75}, &SKILL_BANISH },
  { "Teleport",           SF_PSYCHIC|SF_APPORTATION,                    {35,45,65,95}, {30,150,200,250}, {10,17,45,75}, &SKILL_TELEPORT },
  { "MassSuccor",         SF_PSYCHIC|SF_APPORTATION,                    {35,45,65,95}, {30,150,200,250}, {10,17,45,75}, &SKILL_MASSSUCCOR },
  { "MassSummon",         SF_PSYCHIC|SF_APPORTATION,                    {40,50,70,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_MASSSUMMON },
  { "MassTeleport",       SF_PSYCHIC|SF_APPORTATION,                    {40,50,70,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_MASSTELEPORT },
  { "PhaseDoor",          SF_PSYCHIC|SF_APPORTATION,                    {27,37,47,87}, {30,150,200,250}, {10,17,45,75}, &SKILL_PHASEDOOR },

  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "SeeInvisible",       SF_PSYCHIC|SF_SPIRIT,                         {10,20,40,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_SEEINVISIBLE },
  { "Luckyhits",          SF_PSYCHIC|SF_SPIRIT,                         {12,32,62,92}, {30,150,200,250}, {10,17,45,75}, &SKILL_LUCKYHITS },
  { "SenseLife",          SF_PSYCHIC|SF_SPIRIT,                         {15,25,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_SENSELIFE },
  { "Luckydamage",        SF_PSYCHIC|SF_SPIRIT,                         {17,36,66,96}, {30,150,200,250}, {10,17,45,75}, &SKILL_LUCKYDAMAGE },
  { "SpiritWalk",         SF_PSYCHIC|SF_SPIRIT,                         {20,30,50,80}, {30,150,200,250}, {10,17,45,75}, &SKILL_SPIRITWALK },
  { "Luckshield",         SF_PSYCHIC|SF_SPIRIT,                         {22,32,42,82}, {30,150,200,250}, {10,17,45,75}, &SKILL_LUCKSHIELD },
  { "Identify",           SF_PSYCHIC|SF_SPIRIT,                         {25,35,45,85}, {30,75, 100,125}, {10,17,45,75}, &SKILL_IDENTIFY },
  { "Stat",               SF_PSYCHIC|SF_SPIRIT,                         {27,37,67,97}, {30,150,200,250}, {10,17,45,75}, &SKILL_STAT },

  /* Name */              /* Skill flags */                             /* Level */    /* Max */          /* Gain Divisor*/
  { "Burningfist",        SF_PSYCHIC|SF_PYROKINETIC|SF_OFFENSE|SF_EVIL, { 8,16,32,64}, {30,150,200,250}, {10,17,45,75}, &SKILL_BURNINGFIST },
  { "Fireshield",         SF_PSYCHIC|SF_PYROKINETIC|SF_DEFENSE,         {13,22,42,72}, {30,150,200,250}, {10,17,45,75}, &SKILL_FIRESHIELD },
  { "Fireblade",          SF_PSYCHIC|SF_PYROKINETIC,                    {15,24,44,74}, {30,150,200,250}, {10,17,45,75}, &SKILL_FIREBLADE },
  { "Heatshield",         SF_PSYCHIC|SF_PYROKINETIC|SF_DEFENSE,         {17,25,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_HEATSHIELD },
  { "Firearmor",          SF_PSYCHIC|SF_PYROKINETIC|SF_DEFENSE,         {19,27,47,77}, {30,150,200,250}, {10,17,45,75}, &SKILL_FIREARMOR },
  { "Flamestrike",        SF_PSYCHIC|SF_PYROKINETIC|SF_OFFENSE|SF_EVIL, {21,30,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_FLAMESTRIKE },
/*
  { "Ignite",             SF_PSYCHIC|SF_PYROKINETIC|SF_OFFENSE|SF_EVIL, {35,45,65,95}, {30,150,200,250}, {10,17,45,75}, &SKILL_IGNITE },
*/
  { "Incinerate",         SF_PSYCHIC|SF_PYROKINETIC|SF_OFFENSE|SF_EVIL, {40,50,70,99}, {30,150,200,250}, {10,17,45,75}, &SKILL_INCINERATE },
  /* Fire Weapon group - keep Together! */
  { "Fire-Accuracy",      SF_PSYCHIC|SF_PYROKINETIC|SF_WEAPON,          {14,24,44,74}, {30,150,200,250}, {10,15,40,70}, &SKILL_FIRE_ACCURACY },
  { "Fire-Damage",        SF_PSYCHIC|SF_PYROKINETIC|SF_WEAPON,          {19,34,54,84}, {30,100,150,200}, {15,25,55,75}, &SKILL_FIRE_DAMAGE },
  { "Fire-Speed",         SF_PSYCHIC|SF_PYROKINETIC|SF_WEAPON,          {30,40,60,90}, {30,150,200,250}, {20,35,60,99}, &SKILL_FIRE_SPEED },

  /* Misc. skills */
  { "Flee",               SF_GENERAL,                                   { 1,15,35,65}, {30,150,200,250}, {10,20,40,75}, &SKILL_FLEE },
/*
  { "Search",             SF_GENERAL,                                   { 1,20,40,70}, {20, 75,100,125}, {10,17,45,75}, &SKILL_SEARCH },
*/
  { "Hide",               SF_CLASS,                                     { 4,24,44,74}, {20, 75,100,150}, {10,17,45,75}, &SKILL_HIDE },
  { "Track",              SF_CLASS,                                     { 4,24,44,74}, {30,150,200,250}, {10,17,45,75}, &SKILL_TRACK },
  { "Ambush",             SF_CLASS,                                     { 4,24,44,74}, {40,200,300,400}, {10,17,45,75}, &SKILL_AMBUSH },
  { "Dodge",              SF_GENERAL,                                   { 5,25,45,75}, {30,150,200,250}, {10,17,45,75}, &SKILL_DODGE },
/*
  { "Conceal",            SF_GENERAL,                                   { 8,18,48,78}, {30,150,200,250}, {10,17,45,75}, &SKILL_CONCEAL },
*/
  { "Pursue",             SF_GENERAL,                                   {10,20,40,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_PURSUE },
  { "Rescue",             SF_CLASS,                                     {10,20,40,70}, {10, 50, 65, 75}, {10,17,45,75}, &SKILL_RESCUE },
  { "Picklock",           SF_CLASS,                                     {10,20,40,70}, {20, 75,100,125}, {10,17,45,75}, &SKILL_PICKLOCK },
  { "Hacking",            SF_CLASS,                                     {10,20,40,70}, {20, 75,100,125}, {10,17,45,75}, &SKILL_HACKING },
  { "WillPower",          SF_GENERAL,                                   {10,20,40,70}, {30,150,200,250}, {10,17,45,75}, &SKILL_WILLPOWER },
  { "Sneak",              SF_CLASS,                                     {12,22,42,72}, {30,150,200,250}, {10,17,45,75}, &SKILL_SNEAK },
/*
  { "Disarm",             SF_CLASS,                                     {15,25,45,75}, {30,150,200,250}, {20,30,50,75}, &SKILL_DISARM },
*/
  { "Perception",         SF_GENERAL,                                   {15,25,45,75}, {20,100,150,175}, {10,17,45,75}, &SKILL_PERCEPTION },
  { "Pickpocket",         SF_CLASS,                                     {15,25,45,75}, {20,100,150,175}, {10,17,45,75}, &SKILL_PICKPOCKET },
  { "Peek",               SF_CLASS,                                     {20,30,50,80}, {20, 75,100,125}, {10,17,45,75}, &SKILL_PEEK },
  { "Steal",              SF_CLASS,                                     {20,30,50,80}, {20, 75,100,125}, {10,17,45,75}, &SKILL_STEAL },
  { "Guard",              SF_GENERAL,                                   {25,35,65,95}, {20, 75,100,125}, {10,17,45,75}, &SKILL_GUARD },
  { "Speed",              SF_GENERAL|SF_WEAPON,                         {25,35,65,95}, {20,100,150,175}, {35,45,70,99}, &SKILL_SPEED },

  /* Terminating entry */
  { "",0, {0,0,0,0},{0,0,0,0},{0,0,0,0},0 }
};

WORD skillNum;

SKILLDETAIL humanDetail[] = {
  { NULL,              0,   0              }
};

SKILLDETAIL artificerDetail[] = {
  { &SKILL_PERCEPTION +1,  SD_MODIFYSKILL },
  { &SKILL_WILLPOWER, +1,  SD_MODIFYSKILL },
  { NULL,              0,   0              }
};

SKILLDETAIL siliconoidDetail[] = {
  { &SKILL_PERCEPTION,-1,  SD_MODIFYSKILL },
  { &SKILL_DODGE,     -1,  SD_MODIFYSKILL },
  { &SKILL_SPEED,     -1,  SD_MODIFYSKILL },
  { &SKILL_WILLPOWER, +2,  SD_MODIFYSKILL },
  { &SKILL_AMBUSH,    +1,  SD_MODIFYSKILL },
  { &SKILL_GUARD,     +1,  SD_MODIFYSKILL },
  { NULL,             0,   0              }
};

SKILLDETAIL salamanderDetail[] = {
  { &SKILL_DODGE,     +1,  SD_MODIFYSKILL },
  { &SKILL_SPEED,     +1,  SD_MODIFYSKILL },
  { &SKILL_AMBUSH,    +1,  SD_MODIFYSKILL },
  { &SKILL_FLEE,      -2,  SD_MODIFYSKILL },
  { &SKILL_PURSUE,    +1,  SD_MODIFYSKILL },
  { &SKILL_TRACK,     +1,  SD_MODIFYSKILL },
  { &SKILL_HIDE,      -2,  SD_MODIFYSKILL },
  { NULL,             0,   0              }
};

struct RaceListType raceList[] = {
  {
    "Human",
    {0,  0,  0,  0,  0,  0,  0,  0,  0}, /* damage resist */
    0, /* cant use skills */
    SF_SLUG,  /* good at skills */
    SF_LASER, /* bad at skills */
    100,100,100, /* max Hunger, Thirst, Intox */
    4,  4,  4,  /* gain Hunger, Thirst, Intox (per tick) */
    100,100,100, /* max Hit, Move, Power points - % class max */
    100,100,100, /* gain Hit, Move, Power points - % class amount */
    {40, 40, 40, 40, 40 },  /* Min Str/Dex/Con/Wis/Int */
    {100,100,100,100,100 }, /* Max */
    {0,  0,  0,  0,  0 },   /* Bonus */
    -1, /* normalize abilities to this */
    100, /* percentage of normal xp needed */
    humanDetail,
    EQU_HEAD|EQU_NECK_1|EQU_NECK_2|EQU_TORSO|EQU_ARM_R|EQU_ARM_L|EQU_WRIST_R|EQU_WRIST_L|EQU_HAND_R
      |EQU_HAND_L|EQU_HELD_R|EQU_HELD_L|EQU_FINGER_R|EQU_FINGER_L|EQU_WAIST|EQU_LEGS|EQU_FEET|EQU_BODY

  },{
    "Artificer",
    {-10, -10, -10,  -5,  +5,  0,  30,  0,  +5}, /* damage resist */
    SF_PLASMA|SF_SUPPORT|SF_SLUG, /* cant use skills */
    SF_TELEPATHY|SF_TELEKINETIC, /* good at skills */
    SF_WEAPON|SF_MELEE, /* bad at skills */
    60, 50, 50, /* max Hunger, Thirst, Intox */
    2,  2,  1,  /* gain Hunger, Thirst, Intox (per tick) */
    90,100,120, /* max Hit, Move, Power points - % class max */
    90,100,120, /* gain Hit, Move, Power points - % class amount */
    {25, 40, 25, 50, 75 },  /* Min Str/Dex/Con/Wis/Int */
    {75, 100,75, 120,150 }, /* Max */
    {0,  0,  0,  5,  10 },  /* Bonus */
    -1, /* normalize abilities to this */
    105, /* percentage of normal xp needed */
    artificerDetail,
    EQU_HEAD|EQU_NECK_1|EQU_NECK_2|EQU_TORSO|EQU_ARM_R|EQU_ARM_L|EQU_WRIST_R|EQU_WRIST_L|EQU_HAND_R
      |EQU_HAND_L|EQU_HELD_R|EQU_HELD_L|EQU_FINGER_R|EQU_FINGER_L|EQU_WAIST|EQU_LEGS|EQU_FEET|EQU_BODY

  },{
    "Siliconoid",
    {20, 20,  0,  30,  0,  -30,  0, -20,  0}, /* damage resist */
    SF_TELEPATHY, /* cant use skills */
    SF_SUPPORT|SF_PLASMA|SF_BODY, /* good at skills */
    SF_SPIRIT|SF_PSYCHIC, /* bad at skills */
    300, 75, 200, /* max Hunger, Thirst, Intox */
    7,  3,  10,  /* gain Hunger, Thirst, Intox (per tick) */
    120,100, 80, /* max Hit, Move, Power points - % class max */
    100,100, 75, /* gain Hit, Move, Power points - % class amount */
    {50, 25, 75, 40, 25 },  /* Min Str/Dex/Con/Wis/Int */
    {150,75,150,100, 90 }, /* Max */
    {10,  0, 10,  0,  0 },  /* Bonus */
    -1, /* normalize abilities to this */
    100, /* percentage of normal xp needed */
    /* Skill Details */
    siliconoidDetail,
    EQU_NECK_1|EQU_TORSO|EQU_ARM_R|EQU_ARM_L|EQU_WRIST_R|EQU_WRIST_L|EQU_HAND_R
      |EQU_HAND_L|EQU_HELD_R|EQU_HELD_L|EQU_FINGER_R|EQU_FINGER_L|EQU_WAIST|EQU_LEGS|EQU_FEET|EQU_BODY

  },{
    "Salamander",
    {10, -30, -10, 30,  -10,  0,  0, 10, 10}, /* damage resist */
    SF_ION, /* cant use skills */
    SF_PYROKINETIC, /* good at skills */
    SF_SPIRIT, /* bad at skills */
    150, 50, 200, /* max Hunger, Thirst, Intox */
    6,   1,  1,  /* gain Hunger, Thirst, Intox (per tick) */
    100, 80,100, /* max Hit, Move, Power points - % class max */
    100,150,100, /* gain Hit, Move, Power points - % class amount */
    {35, 75, 30, 25,  30 },  /* Min Str/Dex/Con/Wis/Int */
    {90,150,100, 75, 120 }, /* Max */
    {0,  10,  5,  0,   0 },  /* Bonus */
    -1, /* normalize abilities to this */
    105, /* percentage of normal xp needed */
    /* Skill Details */
    salamanderDetail,
    EQU_HEAD|EQU_NECK_1|EQU_NECK_2|EQU_TORSO|EQU_ARM_R|EQU_ARM_L|EQU_WRIST_R|EQU_WRIST_L|EQU_HAND_R
      |EQU_HAND_L|EQU_HELD_R|EQU_HELD_L|EQU_FINGER_R|EQU_FINGER_L|EQU_WAIST|EQU_TAIL|EQU_TAILGRIP|EQU_BODY

  },{
    ""
 }
};

SKILLDETAIL soldierDetail[] = {
  { &SKILL_RESCUE,    0,   SD_GETSKILL },
  { &SKILL_TRACK,     0,   SD_GETSKILL },
/*
  { &SKILL_DISARM,    0,   SD_GETSKILL },
*/
  { &SKILL_WILLPOWER, +1,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};                           

SKILLDETAIL medicDetail[] = {
  { &SKILL_RESCUE,    -2,  SD_GETSKILL },
  { &SKILL_WILLPOWER, +1,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};

SKILLDETAIL techDetail[] = {
  { &SKILL_TRACK,     -1,  SD_GETSKILL },
  { &SKILL_HACKING,   -1,  SD_GETSKILL },
  { NULL,             0,   0           }
};

SKILLDETAIL researcherDetail[] = {
  { &SKILL_DODGE,     -1,  SD_MODIFYSKILL },
  { &SKILL_WILLPOWER, +2,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};

SKILLDETAIL mysticDetail[] = {
  { &SKILL_WILLPOWER, +1,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};

SKILLDETAIL labtechDetail[] = {
  { &SKILL_DODGE,     -1,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};

SKILLDETAIL rogueDetail[] = {
  { &SKILL_SNEAK,     0,   SD_GETSKILL },
  { &SKILL_HIDE,      0,   SD_GETSKILL },
  { &SKILL_PICKLOCK,  0,   SD_GETSKILL },
  { &SKILL_HACKING,   0,   SD_GETSKILL },
  { &SKILL_AMBUSH,    0,   SD_GETSKILL },
  { &SKILL_PICKPOCKET,0,   SD_GETSKILL },
  { &SKILL_PEEK,      0,   SD_GETSKILL },
  { &SKILL_STEAL,     0,   SD_GETSKILL },
  { &SKILL_PERCEPTION,+1,  SD_MODIFYSKILL },
  { &SKILL_GUARD,     +1,  SD_MODIFYSKILL },
  { &SKILL_DODGE,     +1,  SD_MODIFYSKILL },
  { &SKILL_FLEE,      +1,  SD_MODIFYSKILL },
  { NULL,             0,   0           }
};

struct ClassListType classList[] = {
  {
    "Soldier",
    SF_PYROKINETIC, /* Skill Cant */
    SF_WEAPON|SF_LASER, /* Skill Good */
    SF_PSYCHIC, /* Skill Bad */
    SF_WEAPON|SF_BODY|SF_GENERAL, /* default level skills */
    15,12,5, /* max Hit, Move, Power points - per level */
    10,10,5, /* gain Hit, Move, Power points - per tick */
    15,100,0, /* starting Hit, Move, Power points */
    100,
    soldierDetail,
    {15, 5, 15, -15, -20 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Medic",
    SF_PLASMA, /* Skill Cant */
    SF_BODY|SF_DEFENSE, /* Skill Good */
    SF_PYROKINETIC|SF_APPORTATION|SF_OFFENSE, /* Skill Bad */
    SF_WEAPON|SF_BODY|SF_TELEKINETIC|SF_GENERAL, /* default level skills */
    11,11,15, /* max Hit, Move, Power points - per level */
    7,10,15, /* gain Hit, Move, Power points - per tick */
    20,100,100, /* starting Hit, Move, Power points */
    80,
    medicDetail,
    {-15, -5, -10, 20, 10 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Tech",
    0, /* Skill Cant */
    SF_TELEKINETIC, /* Skill Good */
    SF_TELEPATHY|SF_PLASMA|SF_SUPPORT|SF_ION, /* Skill Bad */
    SF_WEAPON|SF_TELEKINETIC|SF_APPORTATION|SF_GENERAL, /* default level skills */
    10,11,10, /* max Hit, Move, Power points - per level */
    10,10,10, /* gain Hit, Move, Power points - per tick */
    15,100,50, /* starting Hit, Move, Power points */
    100,
    techDetail,
    {-5, 5, -5, 10, -5 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Researcher",
    SF_PLASMA|SF_SUPPORT|SF_ION, /* Skill Cant */
    SF_PYROKINETIC|SF_OFFENSE, /* Skill Good */
    SF_WEAPON|SF_BODY|SF_MELEE|SF_SPIRIT|SF_DEFENSE, /* Skill Bad */
    SF_WEAPON|SF_TELEPATHY|SF_PYROKINETIC|SF_SPIRIT|SF_GENERAL, /* default level skills */
    7,10,15, /* max Hit, Move, Power points - per level */
    5, 5,12, /* gain Hit, Move, Power points - per tick */
    15,80,100, /* starting Hit, Move, Power points */
    110,
    researcherDetail,
    {-20, -5, -10, 10, 25 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Rogue",
    0, /* Skill Cant */
    SF_MELEE, /* Skill Good */
    SF_PYROKINETIC|SF_SPIRIT, /* Skill Bad */
    SF_WEAPON|SF_APPORTATION|SF_SPIRIT|SF_GENERAL, /* default level skills */
    12,15,7, /* max Hit, Move, Power points - per level */
    10,10,5, /* gain Hit, Move, Power points - per tick */
    15,120,25, /* starting Hit, Move, Power points */
    85,
    rogueDetail,
    {-5, 20, -10, 5, -10 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Mystic",
    SF_PLASMA|SF_SUPPORT|SF_ION, /* Skill Cant */
    SF_TELEPATHY, /* Skill Good */
    SF_MELEE|SF_PYROKINETIC|SF_TELEKINETIC, /* Skill Bad */
    SF_WEAPON|SF_TELEPATHY|SF_SPIRIT|SF_TELEKINETIC|SF_GENERAL, /* default level skills */
    9,10,16, /* max Hit, Move, Power points - per level */
    6, 6,13, /* gain Hit, Move, Power points - per tick */
    18,90,100, /* starting Hit, Move, Power points */
    90,
    mysticDetail,
    {-10, -5, -10, 15, 10 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "Labtech",
    SF_PLASMA|SF_SUPPORT|SF_ION, /* Skill Cant */
    SF_TELEKINETIC|SF_OFFENSE, /* Skill Good */
    SF_WEAPON|SF_BODY|SF_MELEE|SF_SPIRIT|SF_PYROKINETIC|SF_DEFENSE, /* Skill Bad */
    SF_WEAPON|SF_TELEPATHY|SF_PYROKINETIC|SF_APPORTATION|SF_GENERAL, /* default level skills */
    7,10,15, /* max Hit, Move, Power points - per level */
    5, 5,12, /* gain Hit, Move, Power points - per tick */
    15,80,100, /* starting Hit, Move, Power points */
    110,
    labtechDetail,
    {-20, -5, -10, 10, 25 }  /* Bonus Str/Dex/Con/Wis/Int */

  }, {
    "" /*terminating entry*/
  }
};

void SkillInit(void) {
  LWORD i;
  BYTE  buf[256];
  LWORD error = FALSE;

  /* make sure constant and string match */
  for (i=0; *skillList[i].sName; i++) {
    BITSET(skillList[i].sFlag, SF_ALL);
    if (skillList[i].sCheck)
      *(skillList[i].sCheck)=i;
  }

  sprintf(buf, "Total Skills in play: %ld\n", i);
  Log(LOG_BOOT, buf);
  skillNum = i;
  if (error) exit(1);
}

SKILLDETAIL *SkillDetailFind(SKILLDETAIL *sDetail, LWORD skill) {
  LWORD i;

  i=0;
  if (sDetail[i].sNum == NULL) return NULL;
  while (*(sDetail[i].sNum) != skill) {
    i++;
    if (sDetail[i].sNum == NULL) return NULL;
  }
  return sDetail + i;
}

/* will use maxLevel or Character level, whichever is lower */
LWORD SkillMaximum(THING *thing, LWORD skill, LWORD maxLevel, LWORD *gain) {
#define MAX_DIVISOR   5   /* 20% increase/descrease each time */
#define LEVEL_DIVISOR 7   /* ~15% increase/decrease each time */
#define GAIN_DIVISOR  7   /* ~15% increase/decrease each time */

  LWORD        i;
  LWORD        numSet;
  LWORD        level;
  LWORD        maxFactor; /* a factor of 100% would give normal skill advancement more is better */
  LWORD        levelFactor;
  LWORD        gainFactor;
  SKILLDETAIL *raceDetail;
  SKILLDETAIL *classDetail;
  LWORD        max;

  /* Check the obvious reasons why we cant do this at all */
  if(gain) *gain=0;
  if (thing->tType != TTYPE_PLR)
    return 0;
  if BITANY(skillList[skill].sFlag, raceList[Plr(thing)->pRace].rSkillCant) return 0;
  if BITANY(skillList[skill].sFlag, classList[Plr(thing)->pClass].cSkillCant) return 0;

  numSet = 0; /* Modifier */
  maxFactor = 100;
  levelFactor = 100;
  gainFactor = 100;
  classDetail = SkillDetailFind(raceList[Plr(thing)->pRace].rSkill,   skill);
  raceDetail  = SkillDetailFind(classList[Plr(thing)->pClass].cSkill, skill);

  /* see what levels we are restricted to */
  level = Character(thing)->cLevel;
  if (maxLevel>0) MAXSET(level, maxLevel);
  /* if we dont get this skill by default then we must wait until we are over 100th level */
  if ( !BITANY(skillList[skill].sFlag, classList[Plr(thing)->pClass].cSkillDefault) 
    && !(classDetail && classDetail->sGetSkill)
    && !(raceDetail && raceDetail->sGetSkill)
      ) {
    level -= 100; 
    if (level<0) return 0;
  }
  
  /* check to see if they good or bad at this sort of thing */
  numSet += FlagSetNum(BITANY(skillList[skill].sFlag, raceList[Plr(thing)->pRace].rSkillGood));
  numSet -= FlagSetNum(BITANY(skillList[skill].sFlag, raceList[Plr(thing)->pRace].rSkillBad));
  numSet += FlagSetNum(BITANY(skillList[skill].sFlag, classList[Plr(thing)->pClass].cSkillGood));
  numSet -= FlagSetNum(BITANY(skillList[skill].sFlag, classList[Plr(thing)->pClass].cSkillBad));
  if (raceDetail) {
    numSet += raceDetail->sModifier;
  }
  if (classDetail) {
    numSet += classDetail->sModifier;
  }

  /* Increment how good they are at skill accordingly */
  for(i=0; i<numSet; i++){
    maxFactor   += maxFactor/(MAX_DIVISOR+i*3);
    levelFactor -= levelFactor/(LEVEL_DIVISOR+i*3);
    gainFactor  -= gainFactor/(GAIN_DIVISOR+i*3);
  }
  /* Decrement how good they are at skill accordingly */
  for(i=0; i>numSet; i--){
    maxFactor   -= maxFactor/(MAX_DIVISOR+i*2);
    levelFactor += levelFactor/(LEVEL_DIVISOR+i*2);
    gainFactor  += gainFactor/(GAIN_DIVISOR+i*2);
  }
  
  /* find which of the 4 max's, gains etc to use */  
  for (i=0; i<4 && level>=(skillList[skill].sLevel[i]*levelFactor/100); i++);
  i--;
  if (i==-1) return 0; /* cant do this yet */

  /* calc base max */
  max = skillList[skill].sMax[i];
  /* interpolate between 2 table entries */
  if (i<3) {
    LWORD diffMax;
    LWORD diffLevel;
    LWORD prevLevel;
    LWORD nextLevel;
    LWORD percentLevel;

    prevLevel=( skillList[skill].sLevel[i]*levelFactor/100 );
    nextLevel=( skillList[skill].sLevel[i+1]*levelFactor/100 );
    diffLevel = nextLevel - prevLevel;
    percentLevel = (level - prevLevel)*100 / diffLevel;

    /* interpolate up max */
    diffMax = ( skillList[skill].sMax[i+1] - skillList[skill].sMax[i] );
    max += diffMax * percentLevel / 100;
  }

  if(gain) {
    LWORD        theirSkill;
    LWORD        j;

    theirSkill = Plr(thing)->pSkill[skill];

    /* Base gain */
    for (j=0; j<3; j++) {
      *gain = skillList[skill].sGain[j];
      if (theirSkill < (skillList[skill].sMax[j]*maxFactor/100) ) break;
    }
    /* interpolate gain */
    if (j<3) {
      LWORD diffGain;
      LWORD prevGain;
      LWORD nextGain;

      LWORD prevMax;
      LWORD nextMax;
      LWORD diffMax;
      LWORD percentMax;

      prevGain=( skillList[skill].sGain[j] );
      nextGain=( skillList[skill].sGain[j+1] );
      diffGain = nextGain - prevGain;
      
      if (j>0)
        prevMax = skillList[skill].sMax[j-1] * maxFactor / 100;
      else
        prevMax = 0;
      nextMax = skillList[skill].sMax[j] * maxFactor / 100;
      diffMax = nextMax - prevMax;
      percentMax = (theirSkill - prevMax) * 100 / diffMax;

      *gain += diffGain * percentMax / 100;
    }

    *gain = (*gain) * gainFactor/100;
    *gain = ( Plr(thing)->pInt*10/17 + 50 ) / *gain + 1;
  }

  max = max*maxFactor/100;
  return max;
}

/* 
 * Call from within script to allow mob/obj whatever to train Characters 
 * (the script must intercept Practice commands and replace with a call to this routine)
 * The canPractice flag must be satisfied partially, that is to say a canPractice of
 * SF_OFFENCE|SF_TELEKINETIC will only let you practice skills that have either flags set
 * maxLevel is the maximum level that this can train you to! ie if the levels for a skill
 * are 10/20/30/40 and maxLevel is 35, then you will only be able to practice the skill
 * to the max set for the char as they were at 35th level.
 */
void SkillPractice(THING *thing, BYTE *cmd, FLAG canPractice, FLAG cantPractice, LWORD maxLevel) {
  LWORD i;
  LWORD max;
  LWORD maxHere;
  LWORD gain;
  WORD  oldValue;
  BYTE  buf[256];
  BYTE *originalCmd;
  FLAG  categoryBit = 0;

  originalCmd = cmd;

  /* throwaway practice */
  cmd = StrOneWord(cmd, NULL);

  /* get the next word - ie category to show */
  cmd = StrOneWord(cmd, buf);

  if (*buf) {
    /* Okay have a category?, skill?, validate it */
    i = TYPEFIND(buf, skillFlagList);
    if (i<0) {
      i = TYPEFIND(buf, skillList);
      if (i<0) {
        SendThing("^wOh, good one... that isnt a skill and it isnt a category either\n", thing);
        return;
      }
    } else 
      categoryBit = 1<<i;
  }

  /* just show them a category */
  if (!*buf || categoryBit) {
    SkillShow(thing, originalCmd, canPractice, cantPractice, maxLevel);
    SendHint("^;HINT: Try ^<PRACTICE <CATEGORY>^; or ^<PRACTICE <SKILL>^; (see ^<help practice^;)\n", thing);
    return;
  }

  if (Plr(thing)->pPractice <= 0) {
    SendThing("^wI'm afraid you are out of practices\n", thing);
    return;
  }

  maxHere = SkillMaximum(thing, i, maxLevel, &gain);
  max     = SkillMaximum(thing, i, 0, NULL);
  if (max <= 0) {
    SendThing("^wI'm afraid you cannot learn ^rthat ^wskill\n", thing);
    return;
  }
  if ((maxHere <= 0)
      ||!BITANY(canPractice, skillList[i].sFlag)
      || BITANY(skillList[i].sFlag, cantPractice)){
    SendThing("^wI'm afraid you cannot learn that skill ^gHERE!\n", thing);
    return;
  }

  oldValue = Plr(thing)->pSkill[i];
  if (oldValue >= max) {
    sprintf(buf, "^wYou are as good at ^r%s ^was you can be\n", skillList[i].sName);
    SendThing(buf, thing);
    return;
  } else if (oldValue >= maxHere) {
    sprintf(buf, "^yYou are as good at ^r%s ^was you can be, at least for here and now\n", skillList[i].sName);
    SendThing(buf, thing);
    return;
  } 

  Plr(thing)->pSkill[i] += gain;
  Plr(thing)->pPractice -= 1;
  MAXSET(Plr(thing)->pSkill[i], maxHere);
  sprintf(buf, "^cYou improve %s from %hd to %hd\n", skillList[i].sName, oldValue, Plr(thing)->pSkill[i]);
  SendThing(buf, thing);
}

/* 
 * Show the player what they can practice.
 * Separate skills up into categories so it isnt so overwhelming
 */
void SkillShow(THING *thing, BYTE *cmd, FLAG canPractice, FLAG cantPractice, LWORD maxLevel) {
  LWORD i;
  LWORD j;
  LWORD numShown;
  LWORD max;
  LWORD maxHere;
  LWORD gain;
  BYTE  line[256];
  BYTE  buf[256];
  FLAG  categoryBit;

  /* throwaway practice */
  cmd = StrOneWord(cmd, NULL);
  /* get the next word - ie category to show */
  cmd = StrOneWord(cmd, buf);

  /* If nothing is specified show the categories */
  if (!*buf && Character(thing)->cLevel<10)
    strcpy(buf, "*all");
  if (!*buf) {
    if (maxLevel)
      SendThing("^yYou can practice skills in the following categories:\n^g", thing);
    else
      SendThing("^yYou posess skills in the following categories:\n^g", thing);
    numShown=0;
    for (i=0; *skillFlagList[i]; i++) {
      for(j=0; *skillList[j].sName; j++) {
        if (BITANY(skillList[j].sFlag, 1<<i)) {
          max = SkillMaximum(thing, j, 0, NULL);
          /* Limit the display to what we have unless we can actually pr here */
          if (max > 0 && (maxLevel || Plr(thing)->pSkill[j]>0)) { /* if we can practice this at all */
            sprintf(buf, "  %-30s", skillFlagList[i]);
            SendThing(buf, thing);
            numShown++;
            if (numShown%2 == 0) 
              SendThing("\n", thing);
            break;
          }
        }
      }
    }
    if (numShown%2) 
      SendThing("\n", thing);
    SendHint("^;HINT: To display skills in a category type ^<practice *category^;\n", thing);
    if (maxLevel) {
      SendHint("^;HINT: e.g. ^<PRACTICE *ALL^; shows every skill you can practice\n", thing);
    } else {
      SendHint("^;HINT: e.g. ^<PRACTICE *ALL^; shows every skill you posess\n", thing);
      SendHint("^;HINT: Find a trainer and see what skills you can ^<practice\n", thing);
    }
    return;
  }

  i = TYPEFIND(buf, skillFlagList);
  if (i<0) {
    SendThing("^wOh, good one... I've never even heard of that category\n", thing);
    return;
  }
  categoryBit = 1<<i;
                     
  if (maxLevel) {
    if (!strcmp(skillFlagList[i], "*ALL*")) {
      SendThing("^wYou can practice the following skills\n", thing);
    } else {
      SendThing("^wYou can practice the following ", thing);
      SendThing(skillFlagList[i], thing);
      SendThing(" skills:\n", thing);
    }
  } else {
    if (!strcmp(skillFlagList[i], "*ALL*")) {
      SendThing("^wYou currently posess the following skills\n", thing);
    } else {
      SendThing("^wYou currently posess the following ", thing);
      SendThing(skillFlagList[i], thing);
      SendThing(" skills:\n", thing);
    }
  }

  numShown = 0;
  for(i=0; *skillList[i].sName; i++) {
    if (BITANY(skillList[i].sFlag, categoryBit)) {
      max = SkillMaximum(thing, i, 0, NULL);

      /* Limit the display to what we have unless we can actually pr here */
      if (max > 0 && (maxLevel || Plr(thing)->pSkill[i]>0)) { /* if we can practice this at all */
        maxHere = SkillMaximum(thing, i, maxLevel, &gain);

        /* are at max allready */
        if (max<=Plr(thing)->pSkill[i]) { 
          sprintf(line,"*%s(^Y%hd^y)*", skillList[i].sName, Plr(thing)->pSkill[i]);
          sprintf(buf,"^y%-30s", line);
  
        /* can practice it here to full potential */
        } else if ( (maxHere>=max) 
          && BITANY(canPractice, skillList[i].sFlag)
          &&!BITANY(cantPractice, skillList[i].sFlag)){
          sprintf(line,"%s(^w%hd/%ld^g)[%ld]", skillList[i].sName, Plr(thing)->pSkill[i], maxHere, gain);
          sprintf(buf,"^g%-30s", line);
  
        /* cant practice to full potential */
        } else if (BITANY(canPractice, skillList[i].sFlag) 
          &&!BITANY(cantPractice, skillList[i].sFlag)){
          sprintf(line,"%s(^p%hd/*%ld^C)[%ld]", skillList[i].sName, Plr(thing)->pSkill[i], maxHere, gain);
          sprintf(buf,"^C%-30s", line);
  
        /* can practice but not here */
        } else { 
          sprintf(line,"*%s(%hd/%ld)*", skillList[i].sName, Plr(thing)->pSkill[i], max);
          sprintf(buf,"^r%-26s", line);
        }
        SendThing(buf, thing);
        numShown++;
        if (numShown%3 == 0) 
          SendThing("\n", thing);
      }
    }
  }
  if (numShown%3 != 0)
    SendThing("\n", thing);
  sprintf(buf, "\n^w%ld ^cSkills shown. ^g(And you have %ld Practices left)\n", numShown, Plr(thing)->pPractice);
  SendThing(buf, thing);
  if (!maxLevel) {
    SendHint("^;HINT: You can only practice your skills in the presence of a trainer!\n", thing);
    SendHint("^;HINT: The first trainer in the game is ^<Duah^;. Find him!\n", thing);
  }
  return;
}








