
#include "clips.h"

#include "codegen/assets/audio/clips.h"

unsigned short soundsSkippable[16] = {
    SOUNDS_PORTAL_ENTER1,
    SOUNDS_PORTAL_ENTER2,
    SOUNDS_PORTAL_EXIT1,
    SOUNDS_PORTAL_EXIT2,
    SOUNDS_CONCRETE1, //left foot
    SOUNDS_CONCRETE2, //right foot
    SOUNDS_CONCRETE3, //land
    SOUNDS_CONCRETE4, //jump

    // Turret
    SOUNDS_DEPLOY,
    SOUNDS_RETRACT,
    SOUNDS_PING,
    SOUNDS_FLESH_IMPACT_BULLET1,
    SOUNDS_FLESH_IMPACT_BULLET2,
    SOUNDS_FLESH_IMPACT_BULLET3,
    SOUNDS_FLESH_IMPACT_BULLET4,
    SOUNDS_FLESH_IMPACT_BULLET5
};

unsigned short soundsPortalEnter[2] = {
    SOUNDS_PORTAL_ENTER1,
    SOUNDS_PORTAL_ENTER2,
};

unsigned short soundsPortalExit[2] = {
    SOUNDS_PORTAL_EXIT1,
    SOUNDS_PORTAL_EXIT2,
};


unsigned short soundsPortalgunShoot[2] = {
    SOUNDS_PORTALGUN_SHOOT_RED1,
    SOUNDS_PORTALGUN_SHOOT_BLUE1,
};

unsigned short soundsConcreteFootstep[4] = {
    SOUNDS_CONCRETE1, //left foot
    SOUNDS_CONCRETE2, //right foot
    SOUNDS_CONCRETE3, //land
    SOUNDS_CONCRETE4, //jump
};

unsigned short soundsSelecting[2] = {
    SOUNDS_WPN_DENYSELECT,
    SOUNDS_WPN_SELECT,
};

unsigned short soundsIntercom[2] = {
    SOUNDS_DING_ON,
    SOUNDS_DING_OFF,
};
unsigned short soundsButton = SOUNDS_BUTTON3;
unsigned short soundsButtonRelease = SOUNDS_BUTTON10;
unsigned short soundsReleaseCube = SOUNDS_DOORMOVE2;
unsigned short soundsPedestalShooting = SOUNDS_CHARGING;
unsigned short soundsDoor= SOUNDS_DOOR_METAL_THIN_CLOSE2;

unsigned short soundsPedestalMoving = SOUNDS_PORTALGUN_ROTATE1;
unsigned short soundsFastFalling = SOUNDS_FAST_WINDLOOP1;
unsigned short soundsBallCatcher = SOUNDS_ALYX_STUNNER1;

unsigned short soundsPortalOpen2 = SOUNDS_PORTAL_OPEN2;
unsigned short soundsTickTock = SOUNDS_TICKTOCK1;

unsigned short soundsPortalFizzle = SOUNDS_PORTAL_FIZZLE2;

unsigned short soundsElevatorMoving = SOUNDS_WALL_MOVE5;
unsigned short soundsElevatorDoor = SOUNDS_DOORMOVE1;
unsigned short soundsElevatorChime = SOUNDS_PORTAL_ELEVATOR_CHIME;

unsigned short soundsBallLoop = SOUNDS_ENERGY_SING_LOOP4;
unsigned short soundsBallLaunch = SOUNDS_ENERGY_SING_FLYBY1;
unsigned short soundsBallBounce = SOUNDS_ENERGY_BOUNCE1;
unsigned short soundsBallKill = SOUNDS_ENERGY_DISINTEGRATE4;
unsigned short soundsBallExplode = SOUNDS_ENERGY_SING_EXPLOSION2;

int clipsCheckSoundSkippable(unsigned short soundID){
    int arrayLength = sizeof(soundsSkippable)/sizeof(soundsSkippable[0]);
    for(int i=0; i<arrayLength; i++){
        if (soundID == soundsSkippable[i]){
            return 1;
        }
    }
    return 0;
}