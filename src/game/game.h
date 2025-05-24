#ifndef GAME_H
#define GAME_H

#define FBW 160
#define FBH 88
#define COLC 20 /* All maps same size, exactly one screenful. */
#define ROWC 11

#define FADE_OUT_TIME 0.500

// Scoring. Current maximum possible score is 2300, and that is achievable.
#define PARTICIPATION_AWARD 100
#define TIME_BONUS_MAX 1000 /* What you get for completing below the minimum time. */
#define TIME_MINIMUM   120.0
#define TIME_MAXIMUM   240.0
#define TREASURE_BONUS 100 /* Per treasure. */
#define DEATH_BONUS -20 /* Per death. */

struct sprite;
struct sprite_type;

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"
#include "modals.h"

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int texid_tiles; // Going to try to fit all the tiles in one sheet.
  int input,pvinput; // Refreshed by main, before any modals or sprites see it. Read-only to them.
  
  // No generalized modals. There's a Hello and a Game Over, both entirely bespoke.
  struct hello *hello;
  struct gameover *gameover;
  
  int mapid;
  uint8_t map[COLC*ROWC];
  uint8_t physics[256];
  
  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK. We need this often enough that it's worth pointing to special.
  int victory; // <0=fail, >0=succeed, 0=playing
  int arrows_remaining;
  int deferred_victory;
  double deferred_victory_clock;
  int treasurec; // Total treasure collected for session. Updates only as we win the level.
  int got_treasure; // Nonzero if we got the treasure this time. It only counts after you win the level.
  int deathc; // Per session.
  double playtime; // Per session, modals don't count.
  int hiscore;
} g;

int res_get(void *dstpp,int tid,int rid);

/* world.c
 */
int begin_level(int mapid);

/* sprite.c
 * "spawn" is the only way to create a sprite instance, and "defunct" the only way to delete one.
 */
struct sprite {
  const struct sprite_type *type;
  double x,y; // center, in meters
  uint8_t tileid,xform;
  int defunct;
  double phl,phr,pht,phb; // physical bounds relative to center. (phl,pht) should usually be negative.
};
struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  void (*render)(struct sprite *sprite,int x,int y);
};
void drop_defunct_sprites();
void kill_all_sprites();
void kill_sprite(struct sprite *sprite);
struct sprite *spawn_sprite(int spriteid,const struct sprite_type *type,double x,double y); // (spriteid) or (type); both is an error
const struct sprite_type *sprite_type_by_id(int sprtype);

/* physics.c
 * We'll manage collisions and whatnot ad-hoc in sprite controllers.
 * Call out to physics as positions change.
 */
int physics_rectify_sprite(struct sprite *sprite,double corrx,double corry);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
SPRTYPE_FOR_EACH
#undef _

void arrow_setup(struct sprite *sprite,double dx);
int arrow_finished(const struct sprite *sprite);

/* Change mode. (main.c)
 */
void begin_hello();
void begin_gameover();
void begin_play();

void hiscore_save();

#endif
