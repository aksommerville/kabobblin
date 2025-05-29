#include "game.h"

struct g g={0};

static void hiscore_load() {
  char tmp[16];
  int tmpc=egg_store_get(tmp,sizeof(tmp),"hiscore",7);
  if ((tmpc>0)&&(tmpc<sizeof(tmp))) {
    g.hiscore=0;
    int i=0; for (;i<tmpc;i++) {
      if ((tmp[i]<'0')||(tmp[i]>'9')) {
        g.hiscore=0;
        break;
      } else {
        g.hiscore*=10;
        g.hiscore+=tmp[i]-'0';
        if (g.hiscore<0) {
          g.hiscore=0;
          break;
        }
      }
    }
  }
}

void hiscore_save() {
  char tmp[8];
  int v=g.hiscore;
  if (v<0) v=0;
  int i=sizeof(tmp);
  for (;i-->0;v/=10) tmp[i]='0'+v%10;
  egg_store_set("hiscore",7,tmp,sizeof(tmp));
}

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
  hiscore_load();
  
  begin_hello();
  
  return 0;
}

static void check_completion(double elapsed) {
  if (g.victory) return; // Already established.
  int status=0;
  int have_goblin=0,arrow_in_flight=0;;
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_goblin) {
      have_goblin=1;
    } else if (sprite->type==&sprite_type_arrow) {
      if (!arrow_finished(sprite)) arrow_in_flight=1;
    }
  }
  if (arrow_in_flight) {
    status=0;
  } else if (!have_goblin) {
    status=1;
  }
  // If victory was not established, dying ends the level.
  // You *are* allowed to win when dead. Your life is not as important as The Mission.
  if (!status) {
    if (!g.hero) status=-1;
    else return;
  }
  if (status!=g.deferred_victory) {
    g.deferred_victory=status;
    g.deferred_victory_clock=FADE_OUT_TIME;
  } else if ((g.deferred_victory_clock-=elapsed)<=0.0) {
    g.victory=g.deferred_victory;
  }
}

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0); // we only get one
  if (g.input!=g.pvinput) {
    if ((g.input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) {
      egg_terminate(0);
      return;
    }
    if (!g.hello&&!g.gameover&&(g.input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) {
      toggle_popup();
    }
  }
  if (g.hello) {
    hello_update(g.hello,elapsed);
  } else if (g.gameover) {
    gameover_update(g.gameover,elapsed);
  } else if (g.popup) {
    popup_update(g.popup,elapsed);
  } else {
  
    g.playtime+=elapsed;
    int i=g.spritec;
    while (i-->0) {
      struct sprite *sprite=g.spritev[i];
      if (sprite->defunct) continue;
      if (sprite->type->update) sprite->type->update(sprite,elapsed);
    }
  
    // Level termination.
    check_completion(elapsed);
    if (g.victory<0) {
      g.deathc++; // "death" or any other failure condition
      if (begin_level(g.mapid)<0) {
        egg_terminate(1);
        return;
      }
    } else if (g.victory>0) {
      if (g.got_treasure) g.treasurec++;
      if (begin_level(g.mapid+1)<0) {
        begin_gameover();
      }
    }
    
  }
  drop_defunct_sprites(); // Even if not playing. Otherwise there can be one frame with old sprites still present, when the game restarts.
}

void egg_client_render() {
  graf_reset(&g.graf);
  if (g.hello) {
    hello_render(g.hello);
  } else if (g.gameover) {
    gameover_render(g.gameover);
  } else {
    // Play or play+popup.
    graf_draw_tile_buffer(&g.graf,g.texid_tiles,NS_sys_tilesize>>1,NS_sys_tilesize>>1,g.map,COLC,ROWC,COLC);
    int i=g.spritec;
    while (i-->0) {
      struct sprite *sprite=g.spritev[i];
      int x=(int)(sprite->x*NS_sys_tilesize+0.5);
      int y=(int)(sprite->y*NS_sys_tilesize+0.5);
      if (sprite->type->render) sprite->type->render(sprite,x,y);
      else graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
    }
    if (g.deferred_victory) {
      int alpha=(int)((1.0-(g.deferred_victory_clock/FADE_OUT_TIME))*255.0);
      if (alpha>0) {
        if (alpha>0xff) alpha=0xff;
        graf_draw_rect(&g.graf,0,0,FBW,FBH,0x00000000|alpha);
      }
    }
    if (g.popup) {
      popup_render(g.popup);
    }
  }
  graf_flush(&g.graf);
}

/* ROM.
 */

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

/* Mode changes.
 */
 
static void drop_modals() {
  if (g.hello) {
    hello_del(g.hello);
    g.hello=0;
  }
  if (g.gameover) {
    gameover_del(g.gameover);
    g.gameover=0;
  }
}
 
void begin_hello() {
  drop_modals();
  egg_play_song(RID_song_takes_one_to_tango,0,1);
  if (!(g.hello=hello_new())) {
    egg_terminate(1);
    return;
  }
}

void begin_gameover() {
  drop_modals();
  egg_play_song(RID_song_takes_one_to_tango,0,1);
  if (!(g.gameover=gameover_new())) {
    egg_terminate(1);
    return;
  }
}

void begin_play() {
  drop_modals();

  // Reset session globals.
  g.treasurec=0;
  g.deathc=0;
  g.playtime=0.0;
  
  // Music is the same for all levels; it plays until gameover.
  egg_play_song(RID_song_all_in_a_row,0,1);

  if (begin_level(1)<0) {
    egg_terminate(1);
    return;
  }
}

void toggle_popup() {
  if (g.popup) {
    popup_del(g.popup);
    g.popup=0;
  } else {
    if (!(g.popup=popup_new())) return;
  }
  egg_play_sound(RID_sound_popup_toggle);
}
