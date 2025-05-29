#include "game.h"

#define LABEL_LIMIT 3
#define OPTID_PAUSED 7 /* index in strings:1... */
#define OPTID_RESUME 8
#define OPTID_MENU 9

struct popup {
  struct label {
    int texid;
    int x,y,w,h;
    int optid;
    int selectable;
  } labelv[LABEL_LIMIT];
  int labelc;
  int labelp;
  int optpending;
};

void popup_del(struct popup *popup) {
  if (!popup) return;
  struct label *label=popup->labelv;
  int i=popup->labelc;
  for (;i-->0;label++) {
    egg_texture_del(label->texid);
  }
  free(popup);
}

static int popup_add_label(struct popup *popup,int optid,int selectable) {
  if (popup->labelc>=LABEL_LIMIT) return -1;
  struct label *label=popup->labelv+popup->labelc++;
  label->texid=font_texres_oneline(g.font,1,optid,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  label->optid=optid;
  label->selectable=selectable;
  return 0;
}

static void popup_pack(struct popup *popup) {
  popup->labelp=-1;
  struct label *label=popup->labelv;
  int i=popup->labelc;
  int hsum=0;
  for (;i-->0;label++) {
    label->x=(FBW>>1)-(label->w>>1);
    if (label->selectable&&(popup->labelp<0)) popup->labelp=label-popup->labelv;
    hsum+=label->h;
  }
  int y=(FBH>>1)-(hsum>>1);
  for (i=popup->labelc,label=popup->labelv;i-->0;label++) {
    label->y=y;
    y+=label->h;
  }
}

struct popup *popup_new() {
  struct popup *popup=calloc(1,sizeof(struct popup));
  if (!popup) return 0;
  popup_add_label(popup,OPTID_PAUSED,0);
  popup_add_label(popup,OPTID_RESUME,1);
  popup_add_label(popup,OPTID_MENU,2);
  popup_pack(popup);
  return popup;
}

static void popup_move(struct popup *popup,int d) {
  egg_play_sound(RID_sound_ui_motion);
  int panic=popup->labelc;
  while (panic-->0) {
    popup->labelp+=d;
    if (popup->labelp<0) popup->labelp=popup->labelc-1;
    else if (popup->labelp>=popup->labelc) popup->labelp=0;
    struct label *label=popup->labelv+popup->labelp;
    if (label->selectable) return;
  }
  popup->labelp=-1;
}

void popup_update(struct popup *popup,double elapsed) {
  // We can also dismiss with AUX1; that is managed by main.c.
  if ((g.input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) popup_move(popup,-1);
  if ((g.input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) popup_move(popup,1);
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    if ((popup->labelp>=0)&&(popup->labelp<popup->labelc)) {
      struct label *label=popup->labelv+popup->labelp;
      popup->optpending=label->optid; // Wait for SOUTH to release before committing it; easier than having to blackout in the game.
    }
  }
  if (!(g.input&EGG_BTN_SOUTH)&&(g.pvinput&EGG_BTN_SOUTH)) {
    switch (popup->optpending) {
      case OPTID_RESUME: {
          toggle_popup(); // deletes us
        } return;
      case OPTID_MENU: {
          toggle_popup(); // deletes us
          begin_hello();
        } return;
    }
  }
}

void popup_render(struct popup *popup) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000c0);
  if ((popup->labelp>=0)&&(popup->labelp<popup->labelc)) {
    struct label *label=popup->labelv+popup->labelp;
    graf_draw_rect(&g.graf,label->x-1,label->y-1,label->w+1,label->h+1,0x0000ffff);
  }
  struct label *label=popup->labelv;
  int i=popup->labelc;
  for (;i-->0;label++) {
    graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
  }
}
