
#include "player.h"
#include "../controls/controller.h"
#include "../util/time.h"
#include "../defs.h"

void playerInit(struct Player* player) {
    transformInitIdentity(&player->transform);
}

#define PLAYER_SPEED    (SCENE_SCALE * 2.0f)

#define ROTATE_RATE     (M_PI * 0.25f)

void playerUpdate(struct Player* player, struct Transform* cameraTransform) {
    struct Vector3 forward;
    struct Vector3 right;

    quatMultVector(&cameraTransform->rotation, &gForward, &forward);
    quatMultVector(&cameraTransform->rotation, &gRight, &right);

    gForward.y = 0.0f;
    gRight.y = 0.0f;

    vector3Normalize(&gForward, &gForward);
    vector3Normalize(&gRight, &gRight);

    OSContPad* controllerInput = controllersGetControllerData(0);

    vector3AddScaled(&player->transform.position, &forward, -controllerInput->stick_y * gTimeDelta * PLAYER_SPEED / 80.0f, &player->transform.position);
    vector3AddScaled(&player->transform.position, &right, controllerInput->stick_x * gTimeDelta * PLAYER_SPEED / 80.0f, &player->transform.position);

    float rotate = 0.0f;

    if (controllerGetButton(0, L_CBUTTONS)) {
        rotate += ROTATE_RATE * gTimeDelta;
    } else if (controllerGetButton(0, R_CBUTTONS)) {
        rotate -= ROTATE_RATE * gTimeDelta;
    }

    if (rotate) {
        struct Quaternion rotateBy;
        quatAxisAngle(&gUp, rotate, &rotateBy);
        struct Quaternion finalRotation;

        quatMultiply(&player->transform.rotation, &rotateBy, &finalRotation);
        player->transform.rotation = finalRotation;
    }
}