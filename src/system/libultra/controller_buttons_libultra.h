#ifndef __CONTROLLER_BUTTONS_LIBULTRA_H__
#define __CONTROLLER_BUTTONS_LIBULTRA_H__

enum ControllerButtons {
    ControllerButtonNone    = 0,
    ControllerButtonA       = (1 << 15),
    ControllerButtonB       = (1 << 14),
    ControllerButtonZ       = (1 << 13),
    ControllerButtonStart   = (1 << 12),
    ControllerButtonUp      = (1 << 11),
    ControllerButtonDown    = (1 << 10),
    ControllerButtonLeft    = (1 << 9),
    ControllerButtonRight   = (1 << 8),
    ControllerButtonL       = (1 << 5),
    ControllerButtonR       = (1 << 4),
    ControllerButtonCUp     = (1 << 3),
    ControllerButtonCDown   = (1 << 2),
    ControllerButtonCLeft   = (1 << 1),
    ControllerButtonCRight  = (1 << 0)
};

#endif
