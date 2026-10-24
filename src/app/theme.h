#ifndef APP_THEME_H
#define APP_THEME_H

#include "cg/geometry.h"

/* Application-specific assets, built with the reusable Mesh interface.
 * Each factory returns a new owned geometry, or NULL on failure. */
typedef enum {
    THEME_FLOOR,
    THEME_ROOF,
    THEME_BASE,
    THEME_FIN_LEFT,
    THEME_FIN_RIGHT,
    THEME_DETAILS
} ThemePart;
Geometry *theme_cabinet_create(ThemePart part);
Geometry *theme_alien_create(Color uniform);

#endif
