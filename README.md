# Goblin Kabobblin'

Requires [Egg](https://github.com/aksommerville/egg) to build.

Code For A Cause Mini Jam, 23 May 2025, theme "YOU ONLY GET ONE".

The basic idea:
- 2d single-screen platformer. Win each level by killing all the goblins with your bow.
- Arrows keep moving until they reach the wall, possibly collecting goblins along the way.
- You only get one arrow.
- So arrange for the goblins to line themselves up, then fire.

## Agenda

- Friday
- - [x] Basic app features: Modal, sprite, map, etc.
- - [x] Provisional graphics: Terrain, hero, goblin, arrow
- - - Let's try 8-pixel tiles instead of 16, go for a real lo-fi Atari kind of look.
- - [x] Core gameplay
- - - [x] Walk, jump, etc
- - - [x] Fire arrows (yes, plural, while developing)
- - - [x] Goblins
- - - [x] Hazards: Goblins, pits, fires, timed things like flamethrowers or hammers
- - - - Got goblins and pits, i'm not sure we actually need more. Keeping it limited gives it a kind of canonical feel.
- - - [x] Victory and failure conditions
- - - [x] Moveable blocks, for influencing goblins' movement. ...DECLINE. As with hazards, let's keep it stupid simple.
- - [x] Coyote time.
- - [x] Hero animation.
- - [x] By EOD: Game should be basically playable. No audio or menus, and temporary layouts. ...missed
- Saturday
- - [x] Treasure.
- - [x] Maps. 10..20 would be good. ...got 12
- - [x] Select reasonable top and bottom times for the bonus.
- - - With 12 maps, 1:27.987 with all treasure. 1:08.612 without.
- - - 2:30..4:00? ...2:00
- - [x] Observed a goblin flung into the wall, when we were struggling close to each other.
- - - Easy enough to reproduce if you stub eating. Get attacked and guide him into a wall, doesn't happen all the time, but pretty reliable.
- - [x] Real graphics.
- - - ...I actually like what we have, maybe don't change anything.
- - [x] Visual fireworks for treasure.
- - [x] Sound effects.
- - [x] Music.
- - - [x] Play
- - - [x] Hello
- - - [x] Denouement ...same as hello, i think it's ok
- - [x] Modals.
- - - [x] Hello. Do a combined hello/gameover like Dot's Wicked Garden, I liked that.
- - - [x] Scene Denouement, and find a more concise name for that.
- - [x] By EOD: Try to be completely finished with the game. So we can see it fresh tomorrow for second-guessing.
- Sunday
- - [x] Finish game by 1700. That's plenty of overflow time if needed.
- - [x] Finish Itch page by 2200, test, etc.
- - [x] Jam closes at midnight.

Feedback from TechHead, 26 May:
- [x] Introduce climbing with less danger.
- [x] Visible "hungry" state for goblins before they attack.
- [x] Consider widening attack radius (but might not be necessary with Hungry). ...Hungry takes care of it.
- [x] Increase pitch with increasing kabobblification.
- [x] Pick up stuck arrow, only if unkabobbled.

More jam feedback:
- [x] Some option to reset during play.
- [x] Consider upping the gravity a little, someone mentioned it feels slow.
- [ ] It's possible for goblins to land directly upon each other, then they go inert. Let them escape somehow.
- [ ] Can we stop the clock during fade-outs? That might trim times enough to make <1:00 reachable. (it's probably reachable today technically but I can't do it).
