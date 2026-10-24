#ifndef APP_UI_H
#define APP_UI_H

#include "app/machine.h"
#include "cg/camera.h"

enum { UI_PANEL_WIDTH = 316, UI_TOP = 76, UI_BOTTOM = 70 };
typedef struct {
    int width, height, active_joint;
    bool help, debug, free_camera, menu, paused;
    Camera camera;
} UserInterface;
typedef struct {
    float x, y, w, h;
} UIRect;
UIRect ui_mode_rect(const UserInterface *ui, int mode);
int ui_mode_at(const UserInterface *ui, int x, int y);
UIRect ui_pause_rect(const UserInterface *ui, int action);
unsigned char ui_pause_at(const UserInterface *ui, int x, int y);
void ui_draw(const UserInterface *ui, const PrizeMachine *machine);
/* Returns an ASCII command matching the keyboard; 0 if no button was hit. */
unsigned char ui_click(const UserInterface *ui, int x, int y);
int ui_view_width(const UserInterface *ui);
int ui_view_height(const UserInterface *ui);

#endif
