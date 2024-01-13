# Development Progress

One major goal of this fork is to complete development of the game where James left off. This will involve understanding and documenting the tools and processes he used.

Progress is tracked below.

## Game content

- [x] Chamber 00
- [x] Chamber 01
- [x] Chamber 02
- [x] Chamber 03
- [x] Chamber 04
- [x] Chamber 05
- [x] Chamber 06
- [x] Chamber 07
- [x] Chamber 08
- [x] Chamber 09
- [x] Chamber 10
- [x] Chamber 11
- [x] Chamber 12
- [x] Chamber 13
- [ ] Chamber 14
    - [ ] Rising staircase
- [ ] Chamber 15
- [ ] Chamber 16
    - [ ] Turrets
- [ ] Chamber 17
    - [ ] Companion Cube
    - [ ] Incinerator
- [ ] Chamber 18
- [ ] Chamber 19
    - [ ] Fire
    - [ ] Misc. escape models
- [ ] Escape 00
- [ ] Escape 01
    - [ ] Rocket sentry
- [ ] Escape 02
    - [ ] GLaDOS
    - [ ] Personality cores
- [ ] Credits

## New feature TODO list
- [ ] check if display list is long enough
- [ ] pausing while glados is speaking can end her speech early
- [x] test chamber 10 without jumping
- [x] check collider flags when filtering contacts
- [x] gun flicker between levels
- [x] fizzler player sound effect
- [x] clear z buffer instead of partitioning it
- [X] add translations to menus
- [x] jump animation
- [x] optimize static culling
- [x] figure out why portals sometimes are in front of window
- [x] portal hole cutting problems
- [x] crashed when dying in test chamber 05 when hit by pellet in mid air while touching a portal
- [x] rumble pak support
- [x] valve intro
- [x] polish up subtitles
- [x] more sound settings
- [x] add desk chairs and monitors
- [x] Add auto save checkpoints
- [x] Correct elevator timing

## New sounds TODO list
- [ ] Box collision sounds
- [x] Ambient background loop
- [x] Unstationary scaffolding moving sound

## Bug TODO list (hardware verified) (high -> low priority)
- [ ] Two wall portals next to eachother can be used to clip any object out of any level by pushing it into corner, then dropping. 
- [x] Passing into a ceiling portal can sometimes mess with the player rotation
- [x] player can clip through back of elevator by jumping and strafeing at the back corners while inside.
