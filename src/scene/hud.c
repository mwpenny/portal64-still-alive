#include "hud.h"

#include "../../build/assets/materials/hud.h"
#include "../menu/controls.h"
#include "../graphics/graphics.h"
#include "../util/time.h"
#include "../levels/levels.h"
#include "./scene.h"
#include "../savefile/savefile.h"

#define HUD_CENTER_WIDTH 6
#define HUD_CENTER_HEIGHT 8

#define HUD_CENTER_S 101
#define HUD_CENTER_T 12

#define HUD_OUTER_WIDTH  24
#define HUD_OUTER_HEIGHT 32

#define HUD_OUTER_OFFSET_X 3
#define HUD_OUTER_OFFSET_Y 5

#define HUD_UPPER_X ((SCREEN_WD - HUD_OUTER_WIDTH - (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_UPPER_Y ((SCREEN_HT - HUD_OUTER_HEIGHT - (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define HUD_LOWER_X ((SCREEN_WD - HUD_OUTER_WIDTH + (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_LOWER_Y ((SCREEN_HT - HUD_OUTER_HEIGHT + (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define RETICLE_XMIN ((SCREEN_WD - (8 << 1)) << 1)
#define RETICLE_YMIN ((SCREEN_HT - (8 << 1)) << 1)
#define RETICLE_WIDTH 16
#define RETICLE_HEIGHT 16

#define PROMPT_FADE_TIME        2.0f

void hudInit(struct Hud* hud) {
    hud->promptType = CutscenePromptTypeNone;
    hud->promptOpacity = 0.0f;
    hud->subtitleOpacity = 0.0f;

    hud->flags = 0;
    hud->resolvedPrompts = 0;
    hud->lastPortalIndexShot = -1;

    if (gCurrentLevelIndex == 0) {
        hud->fadeInTimer = INTRO_TOTAL_TIME;
    } else {
        hud->fadeInTimer = 0.0f;
    }
}


void hudUpdate(struct Hud* hud) {
    if (hud->fadeInTimer > 0.0f) {
        hud->fadeInTimer -= FIXED_DELTA_TIME;

        if (hud->fadeInTimer < 0.0f) {
            hud->fadeInTimer = 0.0f;
        }
    }

    float targetPromptOpacity = (hud->flags & HudFlagsShowingPrompt) ? 1.0 : 0.0f;
    float targetSubtitleOpacity = (hud->flags & HudFlagsShowingSubtitle) ? 0.7: 0.0f;

    if (targetPromptOpacity != hud->promptOpacity) {
        hud->promptOpacity = mathfMoveTowards(hud->promptOpacity, targetPromptOpacity, FIXED_DELTA_TIME / PROMPT_FADE_TIME);
    }

    if (targetSubtitleOpacity != hud->subtitleOpacity) {
        hud->subtitleOpacity = mathfMoveTowards(hud->subtitleOpacity, targetSubtitleOpacity, FIXED_DELTA_TIME / PROMPT_FADE_TIME);
    }

    if (targetPromptOpacity && (hud->resolvedPrompts & (1 << hud->promptType)) != 0) {
        hudShowActionPrompt(hud, CutscenePromptTypeNone);
    }

}

void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp) { 
    hud->flags &= ~(HudFlagsLookedPortalable0 | HudFlagsLookedPortalable1);

    if (gScene.player.flags & PlayerHasFirstPortalGun){
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 0, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable0;
        }
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 1, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable1;
        }
    }
}

void hudPortalFired(struct Hud* hud, int index) {
    hud->lastPortalIndexShot = index;

    if (index == 0) {
        hudResolvePrompt(hud, CutscenePromptTypePortal0);
    }
 
    if (index == 1) {
        hudResolvePrompt(hud, CutscenePromptTypePortal1);
    }
}

u8 gPromptActions[] = {
    ControllerActionNone,
    ControllerActionOpenPortal1,
    ControllerActionOpenPortal0,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionDuck,
    ControllerActionMove,
    ControllerActionJump,
};

char* gPromptText[] = {
    NULL,
    "TO PLACE THE ORANGE PORTAL",
    "TO PLACE THE BLUE PORTAL",
    "TO PICKUP AND OBJECT",
    "TO DROP AND OBJECT",
    "TO USE",
    "TO CROUCH",
    "TO MOVE",
    "TO JUMP",
};

char* gSubtitleText[] = {
    "",
    "Hello and, again, welcome to the Aperture Science \ncomputer-aided enrichment center.",
    "We hope your brief detention in the relaxation \nvault has been a pleasant one.",
    "Your specimen has been processed and we are now\nready to begin the test proper.",
    "Before we start, however, keep in mind that \nalthough fun and learning are the primary goals of\nall enrichment center activities, serious\ninjuries may occur.", 
    "For your own safety and the safety of others, \nplease refrain from t-*bzzzzzt*",
    "Por favor bordón de fallar. Muchos gracias de \nfallar gra-*bzzt*",
    "Stand back. The portal will open in three, two, one.",
    "Excellent. Please proceed into the chamberlock\nafter completing each test.",
    "First, however, note the incandescent particle\nfield across the exit.",
    "This Aperture Science Material Emancipation Grid\nwill vaporize any unauthorized equipment that\npasses through it - for instance, the Aperture\nScience Weighted Storage Cube.", 
    "Please place the Weighted Storage Cube on the \nFifteen Hundred Megawatt Aperture Science Heavy \nDuty Super-Colliding Super Button.", 
    "Perfect. Please move quickly to the chamberlock,\nas the effects of prolonged exposure to the \nbutton are not part of this test.",
    "You're doing very well!",
    "Please be advised that a noticeable taste of blood\nis not part of any test protocol but is an \nunintended side effect of the Aperture Science \nMaterial Emancipation Grill, which may,\nin semi-rare cases, emancipate dental \nfillings, crowns, tooth enamel and teeth.",
    "Very good! You are now in possession of the \nAperture Science Handheld Portal Device.",
    "With it, you can create your own portals.",
    "These intra dimensional gates have proven to \nbe completely safe.",
    "The device, however, has not.",
    "Do not touch the operational end of the device.",
    "Do not look directly at the operational end of the\ndevice.",
    "Do not submerge the device in liquid, even partially.",
    "Most importantly, under no circumstances should \nyou-*bzzzpt*",
    "Please proceed to the chamberlock. Mind the gap.",
    "Well done! Remember: The Aperture Science Bring \nYour Daughter to Work Day is the perfect time to\nhave her tested.",
    "Welcome to test chamber four.",
    "You're doing quite well.",
    "Once again, excellent work.",
    "As part of a required test protocol, we will not \nmonitor the next test chamber. You will be \nentirely on your own.Good luck.",
    "You're not a good person. You know that, right?",
    "As part of a required test protocol, our previous\nstatement suggesting that we would not monitor \nthis chamber was an outright fabrication.",
    "Good job! As part of a required test protocol, we\nwill stop enhancing the truth in three, two, o-\n*bzzt*",
    "Warning devices are required on all mobile \nequipment. However, alarms and flashing hazard lights \nhave been found to agitate the high energy pellet\nand have therefore been disabled for \nyour safety.",
    "Good. Now use the Aperture Science Unstationary\nScaffold to reach the chamberlock.",
    "While safety is one of many Enrichment Center goals,\nthe Aperture Science High Energy Pellet, seen \nto the left of the chamber, can and has \ncaused permanent disabilities such as \nvaporization.",
    "Please be careful.",
    "Unbelievable! You, <B>Subject Name Here<B>, must be\nthe pride of <B>Subject Hometown Here<B>.",
    "Very impressive. Please note that any appearance of\ndanger is merely a device to enhance your \ntesting experience.",
    "Please note that we have added a consequence for \nfailure. Any contact with the chamber floor will \nresult in an 'unsatisfactory' mark on your \nofficial testing record followed by \ndeath. Good luck!",
    "The Enrichment Center regrets to inform you that\nthis next test is impossible.",
    "Make no attempt to solve it.",
    "Fantastic! You remained resolute and resourceful in\nan atmosphere of extreme pessimism.",
    "The Enrichment Center apologizes for this clearly \nbroken test chamber.",
    "Once again, the Enrichment Center offers its most \nsincere apologies on the occasion of this \nunsolvable test environment.",
    "Frankly, this chamber was a mistake. If we were you,\nwe would quit now.",
    "No one will blame you for giving up. In fact, quitting\nat this point is a perfectly reasonable \nresponse.",
    "Quit now and cake will be served immediately.",
    "Hello again. To reiterate our previous warning: This\ntest (garbled) -ard momentum.",
    "Momentum, a function of mass and velocity, is\nconserved between portals. In layman's terms: speedy\nthing goes in, speedy thing comes out.",
    "Spectacular. You appear to understand how a portal\naffects forward momentum, or to be more precise,\nhow it does not.",
    "The Enrichment Center promises to always provide a\nsafe testing environment.",
    "In dangerous testing environments, the Enrichment \nCenter promises to always provide useful advice.",
    "For instance, the floor here will kill you - try to \navoid it.",
    "Get ot ydaer f-f-fling yourself. F-Fling into sp\n-*bzzt*",
    "Weeeeeeeeeeeeeeeeeeeeee-*bzzt*",
    "The device has been modified so that it can now \nmanufacture two linked portals at once.",
    "As part of an optional test protocol, we are pleased\nto present an amusing fact:",
    "The device is now more valuable than the organs \nand combined incomes of everyone in <B>Subject \nHometown Here<B>.",
    "Through no fault of the Enrichment Center, you \nhave managed to trap yourself in this room.",
    "An escape hatch will open in three, two, one."
};



void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    if (promptType == CutscenePromptTypeNone) {
        hud->flags &= ~HudFlagsShowingPrompt;
        return;
    }

    hud->flags |= HudFlagsShowingPrompt;
    hud->promptType = promptType;
}

void hudShowSubtitle(struct Hud* hud, enum CutsceneSubtitleType subtitleType) {
    if (subtitleType == CutsceneSubtitleTypeNone) {
        hud->flags &= ~HudFlagsShowingSubtitle;
        return;
    }

    hud->flags |= HudFlagsShowingSubtitle;
    hud->subtitleType = subtitleType;
}

void hudResolvePrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    hud->resolvedPrompts |= (1 << promptType);
}

void hudResolveSubtitle(struct Hud* hud) {
    hud->flags &= ~HudFlagsShowingSubtitle;
}

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    if (player->flags & PlayerIsDead) {
        gSPDisplayList(renderState->dl++, hud_death_overlay);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
    }

    if (hud->fadeInTimer > 0.0f) {
        float alpha = 0.0f;

        if (hud->fadeInTimer > INTRO_FADE_TIME) {
            alpha = 1.0f;
        } else {
            alpha = hud->fadeInTimer * (1.0f / INTRO_FADE_TIME);
        }

        if (alpha >= 0.0f) {
            gSPDisplayList(renderState->dl++, hud_death_overlay);
            gDPSetPrimColor(renderState->dl++, 0, 0, 0, 0, 0, (u8)(255.0f * alpha));
            gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
            gSPDisplayList(renderState->dl++, hud_death_overlay_revert);
        }
    }

    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    int position_of_left_asset = 0; // position from original image to clip out left hud asset
    int position_of_right_asset = HUD_OUTER_WIDTH; // position from original image to clip out right hud asset
    int position_of_portal_indicator = HUD_OUTER_WIDTH*4;

    // white hud because player is grabbing
    if ((playerIsGrabbing(player)) && (player->flags & PlayerHasFirstPortalGun) ){
        gDPSetPrimColor(renderState->dl++, 255, 255, 255, 255, 255, 255);
        gSPTextureRectangle(renderState->dl++, 
            HUD_UPPER_X, HUD_UPPER_Y,
            HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
            G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
    }
    // else blue and orange logic
    else{
        //blue drawing
        if (player->flags & PlayerHasFirstPortalGun){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            if (hud->flags & HudFlagsLookedPortalable1){
                position_of_right_asset = (HUD_OUTER_WIDTH*3);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X, HUD_LOWER_Y,
                HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_right_asset << 5, 0 << 5, 1 << 10, 1 << 10);
            
            // if the player has the first gun but not second both left and right are blue
            if (!(player->flags & PlayerHasSecondPortalGun)){
                if (hud->flags & HudFlagsLookedPortalable1){
                    position_of_left_asset = (HUD_OUTER_WIDTH*2);
                }   
                gSPTextureRectangle(renderState->dl++, 
                    HUD_UPPER_X, HUD_UPPER_Y,
                    HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2),
                    G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            
        }
        //orange drawing
        if (player->flags & PlayerHasSecondPortalGun){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            if (hud->flags & HudFlagsLookedPortalable0){
                position_of_left_asset = (HUD_OUTER_WIDTH*2);
            }
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X, HUD_UPPER_Y,
                HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_left_asset << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }
    

    // both portal guns owned is only time when the last shot portal indicator appears
    if ((player->flags & PlayerHasSecondPortalGun) && (player->flags & PlayerHasFirstPortalGun) && hud->lastPortalIndexShot != -1){
        //orange indicator
        if (hud->lastPortalIndexShot == 0){
            gDPSetPrimColor(renderState->dl++, 255, 255, 255, 128, 0, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_LOWER_X - 68, HUD_LOWER_Y,
                HUD_LOWER_X - 68 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
        //blue indicator
        else if (hud->lastPortalIndexShot == 1){
            gDPSetPrimColor(renderState->dl++, 255, 255, 0, 128, 255, 255);
            gSPTextureRectangle(renderState->dl++, 
                HUD_UPPER_X + 100, HUD_LOWER_Y,
                HUD_UPPER_X + 100 + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2), 
                G_TX_RENDERTILE, position_of_portal_indicator << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }

    if (hud->fadeInTimer <= 0.0f) {
        // center reticle is drawn over top everything
        gSPDisplayList(renderState->dl++, hud_material_list[CENTER_RETICLE_INDEX]);
        gDPSetPrimColor(renderState->dl++, 255, 255, 210, 210, 210, 255);
        gSPTextureRectangle(renderState->dl++, 
                RETICLE_XMIN, RETICLE_YMIN,
                RETICLE_XMIN + (RETICLE_WIDTH << 2), RETICLE_YMIN + (RETICLE_HEIGHT << 2), 
                G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
    }

    if (hud->promptOpacity > 0.0f && hud->promptType != CutscenePromptTypeNone) {
        controlsRenderPrompt(gPromptActions[hud->promptType], gPromptText[hud->promptType], hud->promptOpacity, renderState);
    }

    if (hud->subtitleOpacity > 0.0f && gSaveData.controls.flags & ControlSaveSubtitlesEnabled) {
        controlsRenderSubtitle(gSubtitleText[hud->subtitleType], hud->subtitleOpacity, renderState);
    }
}