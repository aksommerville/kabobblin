#include "game.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch header=%d,%d actual=%d,%d\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  
  // We're using just one tilesheet. Load it here, and then it's constant.
  if (egg_texture_load_image(g.texid_tiles=egg_texture_new(),RID_image_tiles)<0) return -1;
  {
    const void *serial=0;
    int serialc=res_get(&serial,EGG_TID_tilesheet,RID_tilesheet_tiles);
    struct rom_tilesheet_reader reader;
    if (rom_tilesheet_reader_init(&reader,serial,serialc)<0) return -1;
    struct rom_tilesheet_entry entry;
    while (rom_tilesheet_reader_next(&entry,&reader)>0) {
      if (entry.tableid!=NS_tilesheet_physics) continue;
      memcpy(g.physics+entry.tileid,entry.v,entry.c);
    }
  }
  
  srand_auto();
  
  if (begin_level(1)<0) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {
  // TODO poll input
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
  drop_defunct_sprites();
}

void egg_client_render() {
  graf_reset(&g.graf);
  graf_draw_tile_buffer(&g.graf,g.texid_tiles,NS_sys_tilesize>>1,NS_sys_tilesize>>1,g.map,COLC,ROWC,COLC);
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    int x=(int)(sprite->x*NS_sys_tilesize+0.5);
    int y=(int)(sprite->y*NS_sys_tilesize+0.5);
    if (sprite->type->render) sprite->type->render(sprite,x,y);
    else graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
  }
  graf_flush(&g.graf);
}

int res_get(void *dstpp,int tid,int rid) {
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return 0;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid>tid) return 0;
    if (res->tid<tid) continue;
    if (res->rid>rid) return 0;
    if (res->rid<rid) continue;
    *(const void**)dstpp=res->v;
    return res->c;
  }
  return 0;
}
