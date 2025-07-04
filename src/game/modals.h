/* modals.h
 * Hello and Game Over.
 * No generic management; each modal has its own API.
 * When a modal is present it captures all input except the global quit key, and renders the entire framebuffer.
 * The main game is not a modal; main.c manages that separately.
 */
 
#ifndef MODALS_H
#define MODALS_H

struct hello;
struct gameover;
struct popup;

void hello_del(struct hello *hello);
struct hello *hello_new();
void hello_update(struct hello *hello,double elapsed);
void hello_render(struct hello *hello);

void gameover_del(struct gameover *gameover);
struct gameover *gameover_new();
void gameover_update(struct gameover *gameover,double elapsed);
void gameover_render(struct gameover *gameover);

/* (popup) is a little different -- Owner MUST render the game first, then the popup.
 * Do not update the game while popped up.
 */
void popup_del(struct popup *popup);
struct popup *popup_new();
void popup_update(struct popup *popup,double elapsed);
void popup_render(struct popup *popup);

#endif
