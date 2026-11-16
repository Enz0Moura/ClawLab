#ifndef APP_THEME_H
#define APP_THEME_H

#include "cg/geometry.h"

#include <stddef.h>

#define THEME_MAP_RADIUS_X 3.0f
#define THEME_MAP_RADIUS_Z 3.6f
#define THEME_BACK_WALL_HEIGHT 4.4f
#define THEME_BACK_WALL_THICKNESS .16f
enum {
    THEME_BACK_WALL_SEGMENTS = 12,
    THEME_MAX_BASE_INDICATORS = 7,
    THEME_MAX_FIN_POINTS = 32
};

typedef struct {
    Vec3 center, size;
    Color color;
} ThemeEllipsoid;
typedef struct {
    Vec3 center;
    float radius_x, radius_y, thickness;
    Color color;
} ThemeRing;
typedef struct {
    float x, y;
} ThemePoint2;
typedef struct {
    /* Profile height and fraction of the lathe's X/Z radii. */
    float height, radius_fraction;
} ThemeProfilePoint;
typedef struct {
    Vec3 center;
    float radius_x, radius_z;
    const ThemeProfilePoint *profile;
    size_t profile_count;
    Color color;
} ThemeLatheDesign;

/* Individual alien parts are data, so positions, proportions, and colors can
 * be changed without editing the mesh-generation loops. Array order is left to
 * right when viewed from the front. */
typedef struct {
    ThemeEllipsoid lower_body, head;
    ThemeEllipsoid ears[2], arms[2], legs[2];
    ThemeEllipsoid antenna_stem, antenna_tip;
    ThemeEllipsoid eyes[3], pupils[3];
    ThemeEllipsoid mouth, badge;
    ThemeRing badge_ring;
    float facing_degrees;
    AABB bounds;
} ThemeAlienDesign;

typedef struct {
    float center_x, radius_x, radius_z, skirt_height;
    unsigned segments;
    Color skirt_color, top_color;
} ThemeFloorDesign;
typedef struct {
    float center_x, radius_x, radius_z, height, thickness;
    unsigned mesh_segments, collision_segments;
    Color color;
} ThemeBackWallDesign;
typedef struct {
    ThemeLatheDesign dome, cap, trim;
} ThemeRoofDesign;
typedef struct {
    ThemeLatheDesign shell, belt;
    ThemeEllipsoid porthole;
    ThemeRing porthole_ring;
    ThemeEllipsoid indicators[THEME_MAX_BASE_INDICATORS];
    size_t indicator_count;
} ThemeBaseDesign;
typedef struct {
    /* Convex outline in the XY plane; mirrored around center_x on each side. */
    const ThemePoint2 *outline;
    size_t outline_count;
    float center_x, side_offset, center_z, half_depth;
    Color front_color, back_color, side_color;
} ThemeFinDesign;
typedef struct {
    ThemeFloorDesign floor;
    ThemeBackWallDesign back_wall;
    ThemeRoofDesign roof;
    ThemeBaseDesign base;
    ThemeFinDesign fin;
} ThemeCabinetDesign;


typedef enum {
    THEME_FLOOR,
    THEME_BACK_WALL,
    THEME_ROOF,
    THEME_BASE,
    THEME_FIN_LEFT,
    THEME_FIN_RIGHT
} ThemePart;

ThemeAlienDesign theme_alien_default_design(Color lower_body_color);
Geometry *theme_alien_create_design(const ThemeAlienDesign *design);
Geometry *theme_alien_create(Color lower_body_color);

ThemeCabinetDesign theme_cabinet_default_design(void);
Geometry *theme_cabinet_create_design(ThemePart part, const ThemeCabinetDesign *design);
Geometry *theme_cabinet_create(ThemePart part);

#endif
