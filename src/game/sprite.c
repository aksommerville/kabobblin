#include "game.h"

/* Primitive del and alloc; only used here.
 * These do not call init, and do not interact with the global list.
 */
 
static void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (g.hero==sprite) g.hero=0; // It goes null at the moment of defunct too, but let's be extra careful.
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

static struct sprite *sprite_alloc(const struct sprite_type *type) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->phl=-0.5;
  sprite->pht=-0.5;
  sprite->phr=0.5;
  sprite->phb=0.5;
  return sprite;
}

static int sprite_install(struct sprite *sprite) {
  if (!sprite) return -1;
  if (g.spritec>=g.spritea) {
    int na=g.spritea+32;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) return -1;
    g.spritev=nv;
    g.spritea=na;
  }
  g.spritev[g.spritec++]=sprite;
  return 0;
}

/* Drop defunct sprites.
 */
 
void drop_defunct_sprites() {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (!sprite->defunct) continue;
    g.spritec--;
    memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
    sprite_del(sprite);
  }
}

/* Kill.
 */
 
void kill_all_sprites() {
  g.hero=0;
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    sprite->defunct=1;
  }
}

void kill_sprite(struct sprite *sprite) {
  if (!sprite) return;
  sprite->defunct=1;
  if (sprite==g.hero) g.hero=0;
}

/* Commands.
 */
 
static const struct sprite_type *sprite_type_from_commands(const void *src,int srcc) {
  struct rom_command_reader reader={.v=src,.c=srcc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode!=CMD_sprite_sprtype) continue;
    return sprite_type_by_id((cmd.argv[0]<<8)|cmd.argv[1]);
  }
  return &sprite_type_dummy;
}

static void sprite_apply_command(struct sprite *sprite,uint8_t opcode,const uint8_t *v,int c) {
  switch (opcode) {
    case CMD_sprite_tile: {
        sprite->tileid=v[0];
        sprite->xform=v[1];
      } break;
  }
}

/* Spawn.
 */
 
struct sprite *spawn_sprite(int spriteid,const struct sprite_type *type,double x,double y) {
  if (spriteid&&type) return 0; // ID or type, pick one.
  struct sprite *sprite=0;
  if (spriteid) {
    const void *serial=0;
    int serialc=res_get(&serial,EGG_TID_sprite,spriteid);
    struct rom_sprite rspr;
    if (rom_sprite_decode(&rspr,serial,serialc)<0) return 0;
    if (!(type=sprite_type_from_commands(rspr.cmdv,rspr.cmdc))) return 0;
    if (!(sprite=sprite_alloc(type))) return 0;
    sprite->x=x;
    sprite->y=y;
    struct rom_command_reader reader={.v=rspr.cmdv,.c=rspr.cmdc};
    struct rom_command cmd;
    while (rom_command_reader_next(&cmd,&reader)>0) {
      sprite_apply_command(sprite,cmd.opcode,cmd.argv,cmd.argc);
    }
    if (type->init&&(type->init(sprite)<0)) {
      sprite_del(sprite);
      return 0;
    }
  } else if (type) {
    if (!(sprite=sprite_alloc(type))) return 0;
    sprite->x=x;
    sprite->y=y;
    if (type->init&&(type->init(sprite)<0)) {
      sprite_del(sprite);
      return 0;
    }
  }
  if (sprite_install(sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  return sprite;
}

/* Type by id.
 */
 
const struct sprite_type *sprite_type_by_id(int sprtype) {
  switch (sprtype) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    SPRTYPE_FOR_EACH
    #undef _
  }
  return 0;
}
