#include "game/game.h"

/* Instance definition.
 */
 
struct sprite_hero {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  if (!g.hero) g.hero=sprite;
  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  //TODO
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
