#ifndef GAME_H
#define GAME_H

#define FBW 160
#define FBH 88
#define COLC 20 /* All maps same size, exactly one screenful. */
#define ROWC 11

struct sprite;
struct sprite_type;

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int texid_tiles; // Going to try to fit all the tiles in one sheet.
  
  int mapid;
  uint8_t map[COLC*ROWC];
  uint8_t physics[256];
  
  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK. We need this often enough that it's worth pointing to special.
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

#define _(tag) extern const struct sprite_type sprite_type_##tag;
SPRTYPE_FOR_EACH
#undef _

#endif
