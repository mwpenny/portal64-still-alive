#ifndef __CONTROLLER_BUTTONS_LIBDRAGON_H__
#define __CONTROLLER_BUTTONS_LIBDRAGON_H__

enum ControllerButtons {
    ControllerButtonNone    = 0,
    ControllerButtonA       = (1 << 0),
    ControllerButtonB       = (1 << 1),
    ControllerButtonZ       = (1 << 2),
    ControllerButtonStart   = (1 << 3),
    ControllerButtonUp      = (1 << 4),
    ControllerButtonDown    = (1 << 5),
    ControllerButtonLeft    = (1 << 6),
    ControllerButtonRight   = (1 << 7),
    ControllerButtonL       = (1 << 10),
    ControllerButtonR       = (1 << 11),
    ControllerButtonCUp     = (1 << 12),
    ControllerButtonCDown   = (1 << 13),
    ControllerButtonCLeft   = (1 << 14),
    ControllerButtonCRight  = (1 << 15)
};

#endif
