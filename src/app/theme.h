#ifndef APP_THEME_H
#define APP_THEME_H

#include "cg/geometry.h"

#define THEME_MAP_RADIUS_X 3.0f
#define THEME_MAP_RADIUS_Z 3.6f
#define THEME_BACK_WALL_HEIGHT 4.4f
#define THEME_BACK_WALL_THICKNESS .16f
enum { THEME_BACK_WALL_SEGMENTS = 12 };

/* Application-specific assets, built with the reusable Mesh interface.
 * Each factory returns a new owned geometry, or NULL on failure. */
typedef enum {
    THEME_FLOOR,
    THEME_BACK_WALL,
    THEME_ROOF,
    THEME_BASE,
    THEME_FIN_LEFT,
    THEME_FIN_RIGHT
} ThemePart;
Geometry *theme_cabinet_create(ThemePart part);
Geometry *theme_alien_create(Color uniform);

#endif
