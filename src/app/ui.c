#include "app/ui.h"
#include "cg/render.h"

#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const Color INK = {.028f, .035f, .105f};
static const Color TEXT = {1, .96f, .83f}, MUTED = {.57f, .65f, .83f};
static const Color ACCENT = {.69f, .91f, .24f}, GOLD = {1, .81f, .18f};
static const Color RETRO_PINK = {.96f, .23f, .69f}, RETRO_CYAN = {.22f, .79f, .79f};
static const Color RETRO_PANEL = {.035f, .014f, .105f};
static void retro_dots(float x, float y, float w, float h, int step, float size, Color a,
                       Color b);
static void retro_panel(float x, float y, float w, float h) {
    if (w < 16 || h < 16)
        return;
    render_rect(x + 5, y + 5, w, h, (Color){.012f, .008f, .055f});
    render_rect(x, y, w, h, RETRO_PANEL);
    retro_dots(x + 7, y + 7, w - 14, h - 14, 14, 2, (Color){.085f, .021f, .17f},
               (Color){.13f, .03f, .23f});
    render_rect(x, y, w, 3, RETRO_PINK);
    render_rect(x, y + h - 3, w, 3, RETRO_PINK);
    render_rect(x, y, 3, h, RETRO_CYAN);
    render_rect(x + w - 3, y, 3, h, RETRO_CYAN);
}
static void retro_bar(float x, float y, float w, float h) {
    render_rect(x, y, w, h, RETRO_PANEL);
    if (h > 14)
        retro_dots(x + 6, y + 5, w - 12, h - 10, 14, 2, (Color){.075f, .019f, .15f},
                   (Color){.12f, .027f, .21f});
    render_rect(x, y, w, 3, RETRO_PINK);
    render_rect(x, y + h - 3, w, 3, RETRO_CYAN);
}
static void retro_chip(float x, float y, float w, float h, Color accent) {
    render_rect(x + 3, y + 3, w, h, (Color){.012f, .008f, .055f});
    render_rect(x, y, w, h, RETRO_PANEL);
    render_rect(x, y, w, 2, accent);
    render_rect(x, y, 3, h, RETRO_CYAN);
    render_rect(x + w - 2, y, 2, h, (Color){.30f, .13f, .39f});
}

static void ellipse(float x, float y, float rx, float ry, Color color) {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; ++i) {
        float a = cg_radians(360.0f * i / 32);
        glVertex2f(x + rx * cosf(a), y + ry * sinf(a));
    }
    glEnd();
}
static void planet_icon(float x, float y, float radius) {
    ellipse(x, y, radius, radius, GOLD);
    glColor3f(.98f, .21f, .17f);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        float a = cg_radians(360.0f * i / 48), px = radius * 1.55f * cosf(a),
              py = radius * .38f * sinf(a);
        glVertex2f(x + px, y + py - px * .25f);
    }
    glEnd();
    glLineWidth(1);
}
static void alien_icon(float x, float y, float radius) {
    const Color green = {.64f, .88f, .14f};
    ellipse(x - radius, y, radius * .40f, radius * .22f, green);
    ellipse(x + radius, y, radius * .40f, radius * .22f, green);
    ellipse(x, y, radius, radius * .62f, green);
    render_rect(x - 1, y - radius, 2, radius * .45f, green);
    ellipse(x, y - radius, 2.3f, 2.3f, green);
    for (int i = -1; i <= 1; ++i) {
        ellipse(x + i * radius * .48f, y - 1, radius * .22f, radius * .25f, TEXT);
        ellipse(x + i * radius * .48f, y - 1, radius * .08f, radius * .13f, INK);
    }
}
static void space_background(const UserInterface *ui) {
    float w = (float)ui_view_width(ui), top = UI_TOP, bottom = (float)ui->height - UI_BOTTOM;
    glBegin(GL_QUADS);
    glColor3f(.035f, .045f, .14f);
    glVertex2f(0, top);
    glVertex2f(w, top);
    glColor3f(.085f, .105f, .25f);
    glVertex2f(w, bottom);
    glVertex2f(0, bottom);
    glEnd();
    retro_dots(8, top + 8, w - 16, (bottom - top) * .55f, 15, 2,
               (Color){.075f, .06f, .20f}, (Color){.11f, .07f, .25f});
    float horizon = top + (bottom - top) * .70f;
    glLineWidth(1);
    glColor3f(.24f, .055f, .22f);
    glBegin(GL_LINES);
    for (int i = 0; i <= 24; ++i) {
        glVertex2f(w * .5f, horizon);
        glVertex2f(w * (float)i / 24.0f, bottom);
    }
    for (int i = 1; i <= 8; ++i) {
        float t = (float)i / 8.0f;
        float y = horizon + (bottom - horizon) * t * t;
        glVertex2f(0, y);
        glVertex2f(w, y);
    }
    glEnd();
    for (int i = 0; i < 65; ++i) {
        float x = 18 + (float)((i * 137 + 31) % 947) / 947 * (w - 36);
        float y = top + 44 + (float)((i * 83 + 17) % 601) / 601 * (bottom - top - 68);
        float size = i % 9 == 0 ? 2.2f : 1.0f;
        Color color = i % 9 == 0 ? GOLD : (Color){.36f, .43f, .65f};
        render_rect(x, y, size, size, color);
        if (i % 9 == 0) {
            render_rect(x - 3, y + .6f, 8, 1, color);
            render_rect(x + .6f, y - 3, 1, 8, color);
        }
    }
    planet_icon(68, 159, 18);
    render_text(103, 155, "PIZZA PLANET", GOLD, false);
}
static float panel_scale(const UserInterface *ui) {
    return cg_clamp((float)ui_view_height(ui) / 654, .65f, 1);
}
int ui_view_width(const UserInterface *ui) {
    int w = ui->width - UI_PANEL_WIDTH;
    return w > 1 ? w : 1;
}
int ui_view_height(const UserInterface *ui) {
    int h = ui->height - UI_TOP - UI_BOTTOM;
    return h > 1 ? h : 1;
}
static void button(float x, float y, float w, const char *label, bool accent) {
    Color edge = accent ? RETRO_PINK : RETRO_CYAN;
    render_rect(x + 3, y + 3, w, 34, (Color){.012f, .008f, .055f});
    render_rect(x, y, w, 34, accent ? (Color){.22f, .035f, .20f} : RETRO_PANEL);
    retro_dots(x + 5, y + 5, w - 10, 24, 12, 2, (Color){.09f, .025f, .17f},
               (Color){.14f, .035f, .24f});
    render_rect(x, y, w, 3, edge);
    render_rect(x, y, 3, 34, accent ? GOLD : RETRO_CYAN);
    render_text(x + 12, y + 22, label, accent ? GOLD : TEXT, false);
}
static void draw_world(const UserInterface *ui, const PrizeMachine *m) {
    render_camera(&ui->camera, 0, UI_BOTTOM, ui_view_width(ui), ui_view_height(ui));
    scene_draw(m->root);
    AABB tray = scene_bounds(m->solids[m->tray_solid].node);
    for (int i = 1; i < PRIZE_COUNT; ++i) {
        float x = tray.min.x + (tray.max.x - tray.min.x) * (float)i / PRIZE_COUNT;
        render_line(vec3(x, tray.max.y + .005f, tray.min.z),
                    vec3(x, tray.max.y + .005f, tray.max.z), ACCENT);
    }
    if (m->selected >= 0) {
        AABB box = scene_bounds(m->prizes[m->selected].node);
        box.min = v3_sub(box.min, vec3(.035f, .035f, .035f));
        box.max = v3_add(box.max, vec3(.035f, .035f, .035f));
        glLineWidth(2);
        render_aabb(box, ACCENT);
        glLineWidth(1);
    }
    if (ui->ik_mode && m->ik.target_active) {
        Vec3 p = m->ik.target;
        float r = .12f;
        bool ik_reached = m->ik.error <= MACHINE_IK_POSITION_TOLERANCE &&
                          (!m->ik.orientation_active ||
                           m->ik.orientation_error <= MACHINE_IK_ORIENTATION_TOLERANCE);
        Color target_color = ik_reached ? ACCENT : GOLD;
        render_line(vec3(p.x - r, p.y, p.z), vec3(p.x + r, p.y, p.z), target_color);
        render_line(vec3(p.x, p.y - r, p.z), vec3(p.x, p.y + r, p.z), target_color);
        render_line(vec3(p.x, p.y, p.z - r), vec3(p.x, p.y, p.z + r), target_color);
        render_line(machine_grip_position(m), p, target_color);
    }
    if (ui->debug) {
        render_aabb(MACHINE_CONFIG.workspace, (Color){.3f, .4f, .6f});
        for (int i = 0; i < m->solid_count; ++i)
            render_aabb(scene_bounds(m->solids[i].node), (Color){.4f, .6f, .8f});
        Color arm = m->message_time > 0 && strstr(m->message, "bloqueado") ? (Color){1, .2f, .2f}
                                                                           : (Color){1, .68f, .2f};
        for (int i = 0; i < ARM_PART_COUNT; ++i)
            render_aabb(scene_bounds(m->arm.parts[i]), arm);
        for (int i = 0; i < PRIZE_COUNT; ++i)
            render_aabb(scene_bounds(m->prizes[i].node), ACCENT);
        for (int i = 0; i < JOINT_COUNT; ++i)
            render_axes(m->arm.joint[i]->world, .6f);
        render_axes(m->arm.grip->world, .45f);
        glPushMatrix();
        glMultMatrixf(m->arm.grip->world.m);
        render_aabb(
            (AABB){
                {-MACHINE_CONFIG.capture_x, -MACHINE_CONFIG.capture_y, -MACHINE_CONFIG.capture_z},
                {MACHINE_CONFIG.capture_x, MACHINE_CONFIG.capture_y, MACHINE_CONFIG.capture_z}},
            (Color){1, .3f, .8f});
        glPopMatrix();
        render_aabb(machine_delivery_region(m), ACCENT);
    }
}
static void labels(const UserInterface *ui, const PrizeMachine *m) {
    for (int i = 0; i < PRIZE_COUNT; ++i) {
        if (m->prizes[i].state == PRIZE_DELIVERED)
            continue;
        Vec3 position = mat4_point(m->prizes[i].node->world, vec3(0, .5f, 0));
        position.y += .24f;
        Vec3 screen;
        if (!camera_project(&ui->camera, position, ui_view_width(ui), ui_view_height(ui), &screen))
            continue;
        if (screen.x < 40 || screen.x > ui_view_width(ui) - 45 || screen.y < 10 ||
            screen.y > ui_view_height(ui) - 15)
            continue;
        char label[32];
        snprintf(label, sizeof(label), "%02d %s", i + 1, m->prizes[i].name);
        retro_chip(screen.x - 38, screen.y + UI_TOP - 16, 92, 22,
                   m->selected == i ? RETRO_PINK : RETRO_CYAN);
        render_text(screen.x - 32, screen.y + UI_TOP, label,
                    m->selected == i ? ACCENT : MACHINE_CONFIG.prize_colors[i], false);
    }
    Vec3 screen;
    AABB tray = scene_bounds(m->solids[m->tray_solid].node);
    Vec3 label = vec3((tray.min.x + tray.max.x) * .5f, tray.max.y + .04f, tray.max.z - .1f);
    if (camera_project(&ui->camera, label, ui_view_width(ui), ui_view_height(ui), &screen) &&
        screen.x > 50 && screen.x < ui_view_width(ui) - 60 && screen.y < ui_view_height(ui) - 12) {
        retro_chip(screen.x - 44, screen.y + UI_TOP - 15, 90, 22, RETRO_CYAN);
        render_text(screen.x - 38, screen.y + UI_TOP, "ENTREGA", ACCENT, false);
    }
}
static void panel(const UserInterface *ui, const PrizeMachine *m) {
    float x = (float)ui_view_width(ui), left = x + 22;
    char text[128];
    retro_panel(x, UI_TOP, UI_PANEL_WIDTH, (float)(ui->height - UI_TOP - UI_BOTTOM));
    float scale = panel_scale(ui);
    glPushMatrix();
    glTranslatef(0, UI_TOP * (1 - scale), 0);
    glScalef(1, scale, 1);
    render_text(left, 108, "TRIPULACAO ALIENIGENA", GOLD, false);
    snprintf(text, sizeof(text), "%d / 3  aliens resgatados", m->delivered);
    render_text(left, 137, text, TEXT, true);
    for (int i = 0; i < PRIZE_COUNT; ++i) {
        float y = 155 + (float)i * 31;
        if (m->selected == i)
            render_rect(left - 7, y - 1, 278, 28, RETRO_PINK);
        else
            render_rect(left - 7, y - 1, 278, 28, (Color){.045f, .018f, .115f});
        if (m->selected == i)
            render_rect(left - 7, y - 1, 4, 28, GOLD);
        alien_icon(left + 8, y + 11, 8);
        render_text(left + 30, y + 15, m->prizes[i].name,
                    m->selected == i ? TEXT : MACHINE_CONFIG.prize_colors[i], false);
        render_text(left + 139, y + 15, prize_state_name(m->prizes[i].state),
                    m->selected == i ? TEXT
                                     : (m->prizes[i].state == PRIZE_DELIVERED ? ACCENT : MUTED),
                    false);
    }
    render_rect(left, 262, 272, 2, RETRO_PINK);
    render_text(left, 287, ui->ik_mode ? "IK ATIVO   /   CURSOR + CAPTURA"
                                       : "JUNTAS   /   CONTROLE FK 1 a 4",
                ui->ik_mode ? ACCENT : MUTED, false);
    for (int i = 0; i < JOINT_COUNT; ++i) {
        float y = 300 + (float)i * 57;
        bool active = ui->active_joint == i;
        render_rect(left - 6, y, 282, 51,
                    active ? (Color){.30f, .035f, .23f} : (Color){.045f, .018f, .115f});
        if (active)
            render_rect(left - 6, y, 4, 51, GOLD);
        render_rect(left - 6, y, 282, 2, active ? RETRO_PINK : (Color){.22f, .08f, .31f});
        snprintf(text, sizeof(text), "%d  %s", i + 1, MACHINE_CONFIG.joints[i].name);
        render_text(left + 6, y + 18, text, active ? GOLD : TEXT, false);
        snprintf(text, sizeof(text), "[%g, %g]", (double)MACHINE_CONFIG.joints[i].min,
                 (double)MACHINE_CONFIG.joints[i].max);
        render_text(left + 6, y + 39, text, MUTED, false);
        snprintf(text, sizeof(text), "%6.1f", (double)m->arm.angles.current[i]);
        render_text(left + 112, y + 30, text, TEXT, true);
        retro_chip(left + 199, y + 9, 31, 32, RETRO_CYAN);
        render_text(left + 210, y + 30, "-", TEXT, true);
        retro_chip(left + 238, y + 9, 31, 32, RETRO_PINK);
        render_text(left + 248, y + 30, "+", TEXT, true);
    }
    render_text(left, 553, "GARRA", MUTED, false);
    snprintf(text, sizeof(text), "%4.1f / 40 graus", (double)m->arm.opening);
    render_text(left + 139, 553, text, TEXT, false);
    render_rect(left, 565, 270, 5, (Color){.055f, .025f, .13f});
    render_rect(left, 565, 270 * m->arm.opening / 40, 5, RETRO_PINK);
    const char *status = m->delivered == PRIZE_COUNT               ? "Colecao completa!"
                         : m->held >= 0                            ? "Garra ocupada"
                         : machine_capture_candidate(m, NULL) >= 0 ? "Captura possivel"
                                                                   : "Aproxime a garra aberta";
    Color status_color =
        m->held >= 0 || machine_capture_candidate(m, NULL) >= 0 ? ACCENT : MUTED;
    if (ui->ik_mode && m->ik.target_active) {
        if (m->ik.orientation_active)
            snprintf(text, sizeof(text), "IK: %.3f m / %.1f deg", (double)m->ik.error,
                     (double)m->ik.orientation_error);
        else
            snprintf(text, sizeof(text), "Erro IK: %.3f", (double)m->ik.error);
        status = text;
        status_color = m->ik.error <= MACHINE_IK_POSITION_TOLERANCE &&
                               (!m->ik.orientation_active ||
                                m->ik.orientation_error <= MACHINE_IK_ORIENTATION_TOLERANCE)
                           ? ACCENT
                           : GOLD;
    }
    Color status_edge = status_color.g > .8f && status_color.r < .9f ? RETRO_CYAN : RETRO_PINK;
    retro_chip(left - 4, 576, 278, 25, status_edge);
    render_text(left + 5, 593, status, status_color, false);
    button(left, 610, 130, "C  FECHAR", true);
    button(left + 140, 610, 130, "O  ABRIR", false);
    render_text(left, 668, "Setas: junta  |  +/-: 5 graus  |  [ ]: 1 grau", MUTED, false);
    render_text(left, 687, ui->ik_mode ? "I: cursor  |  clique alien: pegar"
                                       : "I: mover garra com cursor",
                ui->ik_mode ? ACCENT : MUTED, false);
    glPopMatrix();
    button(left, ui->height - 105, 130, "H  AJUDA", false);
    button(left + 140, ui->height - 105, 130, "R  REINICIAR", false);
}
static void help(const UserInterface *ui) {
    float x = 32, y = 166, w = (float)ui_view_width(ui) - 64;
    retro_panel(x, y, w, 450);
    render_text(x + 24, y + 37, "COMO JOGAR", RETRO_CYAN, true);
    const char *lines[] = {"1. Clique em um premio para destaca-lo.",
                           "2. Escolha a junta com 1, 2, 3 ou 4.",
                           "3. Use as setas esquerda/direita para mover.",
                           "   +/- muda o alvo em 5 graus; [ e ] em 1 grau.",
                           "4. Aproxime a garra aberta. C fecha; O abre e solta.",
                           "5. Transporte e solte na bandeja verde.",
                           "I: cursor guia a garra; clique no alien para pegar.",
                           "No IK, clique no piso para mover sem fechar a garra.",
                           "A captura só fecha alinhada e com rota sem colisão.",
                           "",
                           "Camera: direito gira; roda pressionada desloca.",
                           "F: camera livre (direito olha; roda avanca/recua).",
                           "Home: restaurar camera. Espaco: alternar garra.",
                           "R: reiniciar. P ou Esc: abrir a pausa.",
                           "H ou F1: fechar ajuda. D: depuracao.",
                           "Continuar na pausa preserva a partida e o tempo."};
    size_t count = sizeof(lines) / sizeof(lines[0]);
    for (size_t i = 0; i < count; ++i)
        render_text(x + 24, y + 72 + (float)i * 24, lines[i], i + 1 == count ? ACCENT : TEXT,
                    false);
}
static bool menu_columns(const UserInterface *ui) {
    return ui->width >= 960;
}
UIRect ui_mode_rect(const UserInterface *ui, int mode) {
    float width = (float)ui->width;
    float height = (float)ui->height;
    if (menu_columns(ui)) {
        float frame_w = fminf(1120, width - 48);
        float frame_x = (width - frame_w) * .5f;
        float art_w = fminf(350, frame_w * .34f);
        float art_x = frame_x + frame_w - art_w - 18;
        float list_x = frame_x + 18;
        float x = list_x + 22;
        float w = art_x - list_x - 62;
        float h = cg_clamp((height - 400) / 4, 48, 62);
        return (UIRect){x, 250 + (float)mode * (h + 12), w, h};
    }
    float w = fmaxf(1, width - 48);
    float h = cg_clamp((height - 350) / 4, 40, 66);
    return (UIRect){24, 262 + (float)mode * (h + 10), w, h};
}
int ui_mode_at(const UserInterface *ui, int x, int y) {
    if (!ui->menu || !ui->mode_select)
        return -1;
    for (int i = 0; i < MODE_COUNT; ++i) {
        UIRect r = ui_mode_rect(ui, i);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return i;
    }
    return -1;
}
UIRect ui_home_rect(const UserInterface *ui, int item) {
    if (menu_columns(ui)) {
        float width = (float)ui->width;
        float frame_w = fminf(1120, width - 48);
        float frame_x = (width - frame_w) * .5f;
        float art_w = fminf(350, frame_w * .34f);
        float art_x = frame_x + frame_w - art_w - 18;
        float list_x = frame_x + 40;
        float x = list_x;
        float w = art_x - frame_x - 80;
        return (UIRect){x, 300 + (float)item * 100, w, 70};
    }
    return (UIRect){24, 284 + (float)item * 88, fmaxf(1, (float)ui->width - 48), 64};
}
int ui_home_at(const UserInterface *ui, int x, int y) {
    if (!ui->menu || ui->mode_select)
        return -1;
    for (int item = 0; item < 2; ++item) {
        UIRect r = ui_home_rect(ui, item);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return item;
    }
    return -1;
}
UIRect ui_pause_rect(const UserInterface *ui, int action) {
    float w = fminf(560, (float)ui->width - 48);
    return (UIRect){((float)ui->width - w) * .5f + 24,
                    ((float)ui->height - 330) * .5f + 112 + (float)action * 52, w - 48, 34};
}
unsigned char ui_pause_at(const UserInterface *ui, int x, int y) {
    if (!ui->paused || ui->menu)
        return 0;
    const unsigned char commands[] = {'p', 'r', 'm'};
    for (int i = 0; i < 3; ++i) {
        UIRect r = ui_pause_rect(ui, i);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return commands[i];
    }
    return 0;
}
static void pause_menu(const UserInterface *ui, const PrizeMachine *m) {
    UIRect first = ui_pause_rect(ui, 0);
    float x = first.x - 24, y = first.y - 112;
    retro_panel(x, y, first.w + 48, 330);
    render_text(first.x, y + 39, "PARTIDA PAUSADA", RETRO_PINK, true);
    char text[96];
    if (m->mode == MODE_SANDBOX)
        snprintf(text, sizeof(text), "Sandbox / %d de 3 aliens / sem limite de tempo",
                 m->delivered);
    else {
        int seconds = (int)ceilf(m->time_remaining);
        snprintf(text, sizeof(text), "%s / %d de 3 aliens / %02d:%02d restantes",
                 GAME_MODES[m->mode].name, m->delivered, seconds / 60, seconds % 60);
    }
    render_text(first.x, y + 68, text, TEXT, false);
    render_text(first.x, y + 91, "Tempo e movimentos congelados. Continue de onde parou.", MUTED,
                false);
    const char *labels[] = {"P  CONTINUAR", "R  RESETAR PARTIDA", "M  VOLTAR AOS MODOS"};
    for (int i = 0; i < 3; ++i) {
        UIRect r = ui_pause_rect(ui, i);
        button(r.x, r.y, r.w, labels[i], i == 0);
    }
    render_text(first.x, y + 290, "Resetar comeca novamente no mesmo modo.", MUTED, false);
    render_text(first.x, y + 312, "Voltar aos modos encerra a partida atual.", MUTED, false);
}
static void retro_dots(float x, float y, float w, float h, int step, float size, Color a,
                       Color b) {
    glBegin(GL_QUADS);
    int row = 0;
    for (float py = y; py < y + h; py += (float)step, ++row) {
        int column = 0;
        for (float px = x; px < x + w; px += (float)step, ++column) {
            Color color = (row + column) % 5 == 0 ? b : a;
            glColor3f(color.r, color.g, color.b);
            glVertex2f(px, py);
            glVertex2f(px + size, py);
            glVertex2f(px + size, py + size);
            glVertex2f(px, py + size);
        }
    }
    glEnd();
}
static void menu_background(const UserInterface *ui) {
    static const Color night = {.035f, .018f, .12f};
    float width = (float)ui->width, height = (float)ui->height;
    float horizon = height * .68f;
    render_rect(0, 0, width, height, night);
    render_rect(0, 0, width, horizon, (Color){.055f, .018f, .16f});
    render_rect(0, horizon, width, height - horizon, (Color){.12f, .025f, .24f});
    retro_dots(8, 32, width - 16, horizon - 42, 10, 3, (Color){.13f, .035f, .28f},
               (Color){.22f, .055f, .36f});
    for (int i = 0; i < 65; ++i) {
        float x = 18 + (float)((i * 137 + 31) % 947) / 947 * (width - 36);
        float y = 28 + (float)((i * 83 + 17) % 601) / 601 * (height - 56);
        float size = i % 9 == 0 ? 3 : 2;
        Color star = i % 9 == 0 ? GOLD : (Color){.25f, .30f, .53f};
        render_rect(x, y, size, size, star);
        if (i % 9 == 0) {
            render_rect(x - 3, y + 1, 9, 1, star);
            render_rect(x + 1, y - 3, 1, 9, star);
        }
    }
    /* Perspective grid floor below the neon horizon. */
    glColor3f(.91f, .10f, .52f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (int i = 0; i <= 28; ++i) {
        float end_x = width * (float)i / 28.0f;
        glVertex2f(width * .5f, horizon);
        glVertex2f(end_x, height - 22);
    }
    for (int i = 1; i <= 12; ++i) {
        float t = (float)i / 12.0f;
        float y = horizon + (height - 22 - horizon) * t * t;
        glColor3f(.55f + .32f * t, .07f, .38f + .22f * t);
        glVertex2f(0, y);
        glVertex2f(width, y);
    }
    glEnd();
    glLineWidth(1);
    render_rect(0, horizon - 2, width, 4, (Color){.98f, .12f, .56f});

    /* Keep a clean edge around the retrowave sky and perspective floor. */
    for (int y = 28; y < ui->height - 28; y += 6)
        render_rect(0, (float)y, width, 1, (Color){.035f, .02f, .105f});
}
static void claw_screen_background(float x, float y, float w, float h, bool framed) {
    if (framed) {
        render_rect(x, y, w, h, (Color){.045f, .08f, .17f});
        render_rect(x, y, w, 3, (Color){.22f, .79f, .79f});
        render_rect(x, y + h - 3, w, 3, (Color){.22f, .79f, .79f});
        render_rect(x, y, 3, h, (Color){.22f, .79f, .79f});
        render_rect(x + w - 3, y, 3, h, (Color){.22f, .79f, .79f});
    } else {
        render_rect(x, y, w, h, (Color){.045f, .08f, .17f});
        render_rect(x, y, w, 2, (Color){.22f, .79f, .79f});
        render_rect(x, y + h - 2, w, 2, (Color){.22f, .79f, .79f});
    }
    for (float scan = y + 8; scan < y + h - 6; scan += 8)
        render_rect(x + 5, scan, w - 10, 1, (Color){.065f, .095f, .18f});

}
static void claw_cabinet(float x, float y, float w, float h) {
    render_rect(x + 5, y + 5, w, h, (Color){.015f, .018f, .07f});
    render_rect(x, y, w, h, (Color){.12f, .11f, .26f});
    render_rect(x, y, w, 5, (Color){.99f, .31f, .23f});
    render_rect(x, y + 5, 5, h - 10, (Color){.22f, .79f, .79f});
    render_rect(x + w - 5, y + 5, 5, h - 10, (Color){.22f, .79f, .79f});
    render_text(x + 16, y + 27, "RESGATE ESPACIAL", GOLD, true);
    claw_screen_background(x + 14, y + 39, w - 28, h - 94, true);
    render_rect(x + 14, y + h - 44, w - 28, 27, (Color){.07f, .075f, .17f});
    render_text(x + 22, y + h - 25, "3 ALIENS  /  1 GARRA", (Color){.22f, .84f, .83f},
                false);
}
static UIRect menu_preview_rect(const UserInterface *ui) {
    if (menu_columns(ui)) {
        float frame_w = fminf(1120, (float)ui->width - 48);
        float frame_x = ((float)ui->width - frame_w) * .5f;
        float art_w = fminf(350, frame_w * .34f);
        float art_x = frame_x + frame_w - art_w - 18;
        float art_h = fmaxf(160, fminf(448, (float)ui->height - 225));
        return (UIRect){art_x + 17, 208, art_w - 34, art_h - 100};
    }
    float art_w = (float)ui->width - 48;
    float view_w = fminf(280, art_w - 24);
    if (view_w < 1)
        view_w = art_w;
    return (UIRect){24 + (art_w - view_w) * .5f, 114, view_w, 112};
}
static void menu_orbit_trails(Vec3 grip) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT |
                 GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glLineWidth(1.5f);
    const Color cyan = {.17f, .83f, .91f};
    const Color pink = {.94f, .20f, .72f};
    for (int ring = 0; ring < 2; ++ring) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 72; ++i) {
            float a = cg_radians((float)i * 5);
            float radius_x = ring == 0 ? .72f : .92f;
            float radius_y = ring == 0 ? .58f : .43f;
            float radius_z = ring == 0 ? .10f : .28f;
            glColor4f(ring == 0 ? cyan.r : pink.r, ring == 0 ? cyan.g : pink.g,
                      ring == 0 ? cyan.b : pink.b, ring == 0 ? .58f : .38f);
            glVertex3f(grip.x + radius_x * cosf(a), grip.y + radius_y * sinf(a),
                       grip.z + radius_z * sinf(a));
        }
        glEnd();
    }
    glPopAttrib();
}
static void draw_menu_preview(const UserInterface *ui, const PrizeMachine *m) {
    if (!m)
        return;
    UIRect view = menu_preview_rect(ui);
    int width = (int)view.w, height = (int)view.h;
    if (width <= 0 || height <= 0)
        return;
    int x = (int)view.x;
    int y = ui->height - (int)(view.y + view.h);
    float time = (float)glutGet(GLUT_ELAPSED_TIME) * .001f;
    float yaw = 28.0f + time * 7.0f;
    bool wide = menu_columns(ui);
    Vec3 grip = machine_grip_position(m);
    Camera preview = {vec3(grip.x, grip.y + (wide ? -.35f : -.62f), grip.z), yaw, 14,
                      wide ? 4.8f : 2.65f, 42, .08f, 30};

    render_camera(&preview, x, y, width, height);
    menu_orbit_trails(grip);
    if (wide) {
        /* Reuse the same forearm, palm, fingers, and wrist joint drawn in play. */
        static const int end_effector[] = {1, 2, 3, 4, 7};
        for (size_t i = 0; i < sizeof(end_effector) / sizeof(end_effector[0]); ++i)
            scene_draw(m->arm.parts[end_effector[i]]);
    } else {
        static const int end_effector[] = {2, 3, 4, 7};
        for (size_t i = 0; i < sizeof(end_effector) / sizeof(end_effector[0]); ++i)
            scene_draw(m->arm.parts[end_effector[i]]);
    }
    /* Stage the game's actual alien meshes below the gripper for the preview. */
    for (int i = 0; i < PRIZE_COUNT; ++i) {
        glPushMatrix();
        glTranslatef(grip.x + ((float)i - 1.0f) * .64f, grip.y - 1.08f, grip.z + .20f);
        glRotatef((float)(i - 1) * 12.0f, 0, 1, 0);
        m->prizes[i].node->geometry->draw(m->prizes[i].node->geometry);
        glPopMatrix();
    }
}
static void menu_preview_frame(const UserInterface *ui) {
    UIRect r = menu_preview_rect(ui);
    Color cyan = {.22f, .79f, .79f};
    Color pink = {.96f, .23f, .69f};
    render_rect(r.x - 2, r.y - 2, r.w + 4, 2, cyan);
    render_rect(r.x - 2, r.y + r.h, r.w + 4, 2, cyan);
    render_rect(r.x - 2, r.y - 2, 2, r.h + 4, pink);
    render_rect(r.x + r.w, r.y - 2, 2, r.h + 4, pink);
}
static void menu(const UserInterface *ui) {
    static const Color frame = {.98f, .31f, .23f};
    static const Color cyan = {.22f, .79f, .79f};
    static const Color mode_colors[] = {
        {.69f, .91f, .24f}, {.24f, .80f, .74f}, {1, .81f, .18f}, {1, .32f, .30f}};
    const char *descriptions[] = {
        "Sem limite. Bandeja ampla; pouca variacao; quiques suaves.",
        "4 min. Bandeja menor; pouca variacao; quiques leves.",
        "3 min. Bandeja reduzida; distancias variadas; quiques moderados.",
        "2 min. Menor bandeja; maior variacao; quiques fortes."};
    menu_background(ui);
    UIRect first = ui_mode_rect(ui, 0);

    if (menu_columns(ui)) {
        float frame_w = fminf(1120, (float)ui->width - 48);
        float frame_x = ((float)ui->width - frame_w) * .5f;
        float art_w = fminf(350, frame_w * .34f);
        float art_x = frame_x + frame_w - art_w - 18;
        float list_x = frame_x + 18;
        float list_w = art_x - list_x - 28;
        float list_h = fminf(402, (float)ui->height - 230);
        Color pink = {.96f, .23f, .69f};
        render_rect(frame_x, 39, frame_w, 3, frame);
        render_rect(frame_x, 42, frame_w, 1, (Color){.30f, .19f, .34f});
        render_text(frame_x + 4, 79, "A GARRA!", GOLD, true);
        render_text(frame_x + 134, 78, "RESGATE ESPACIAL  /  EDICAO ARCADE", cyan, false);
        render_text(frame_x + frame_w - 228, 78, "SETAS + ENTER  /  1-4", TEXT, false);
        claw_cabinet(art_x, 166, art_w, fminf(448, (float)ui->height - 225));
        render_rect(list_x + 8, 198, list_w, list_h, (Color){.012f, .008f, .055f});
        render_rect(list_x, 190, list_w, list_h, (Color){.045f, .018f, .12f});
        retro_dots(list_x + 8, 199, list_w - 16, list_h - 18, 12, 3,
                   (Color){.105f, .025f, .21f}, (Color){.16f, .035f, .29f});
        render_rect(list_x, 190, list_w, 3, pink);
        render_rect(list_x, 190, 3, list_h, cyan);
        render_rect(list_x + list_w - 3, 190, 3, list_h, cyan);
        render_rect(list_x, 190 + list_h - 3, list_w, 3, pink);
        render_rect(list_x + 8, 199, list_w - 16, 1, (Color){.41f, .13f, .43f});
        render_text(list_x + 24, 229, ui->mode_select ? "ESCOLHA A MISSAO" : "MENU PRINCIPAL",
                    GOLD, true);
        render_text(list_x + 24, 245,
                    ui->mode_select ? "RESGATE OS 3 ALIENS / USE AS SETAS"
                                    : "NOVO JOGO / SETAS + ENTER",
                    MUTED, false);
    } else {
        float art_y = 112;
        float art_h = 116;
        float art_x = 24;
        float art_w = (float)ui->width - 48;
        render_rect(24, 38, (float)ui->width - 48, 3, frame);
        render_text(25, 76, "A GARRA!", GOLD, true);
        render_text(154, 75, "RESGATE ESPACIAL  /  EDICAO ARCADE", cyan, false);
        claw_screen_background(art_x, art_y, art_w, art_h, false);
        render_text(first.x, 249, ui->mode_select ? "ESCOLHA SUA MISSAO" : "MENU PRINCIPAL",
                    GOLD, true);
    }

    int item_count = ui->mode_select ? MODE_COUNT : 2;
    for (int i = 0; i < item_count; ++i) {
        UIRect r = ui->mode_select ? ui_mode_rect(ui, i) : ui_home_rect(ui, i);
        Color accent = ui->mode_select ? mode_colors[i] : cyan;
        bool selected = i == ui->menu_selection;
        if (selected) {
            Color pink = {.96f, .23f, .69f};
            render_rect(r.x - 11, r.y + 4, r.w + 15, r.h, (Color){.012f, .018f, .055f});
            render_rect(r.x - 14, r.y, r.w + 15, r.h, pink);
            render_rect(r.x - 14, r.y, 4, r.h, GOLD);
            /* Blocky yellow pointer, echoing the selector in the reference. */
            render_rect(r.x + 2, r.y + r.h * .5f - 8, 7, 16, GOLD);
            render_rect(r.x + 9, r.y + r.h * .5f - 4, 6, 8, GOLD);
        }
        char title[48];
        if (ui->mode_select)
            snprintf(title, sizeof(title), "%d  %s", i + 1, GAME_MODES[i].name);
        else
            snprintf(title, sizeof(title), "%s", i == 0 ? "NOVO JOGO" : "SAIR");
        render_text(r.x + 28, r.y + 25, title, selected ? TEXT : accent, true);
        if (ui->mode_select) {
            if (r.h >= 58)
                render_text(r.x + 28, r.y + r.h - 10, descriptions[i],
                            selected ? (Color){1, .82f, .90f} : MUTED, false);
            else
                render_text(r.x + 28, r.y + r.h - 8, "PRESSIONE A TECLA INDICADA",
                            selected ? TEXT : MUTED, false);
        } else {
            const char *description = i == 0 ? "Escolha um modo e comece o resgate."
                                             : "Fechar o jogo.";
            render_text(r.x + 28, r.y + r.h - 12, description,
                        selected ? (Color){1, .82f, .90f} : MUTED, false);
        }
    }
    float footer_y = (float)ui->height - 40;
    render_text(26, footer_y,
                ui->mode_select ? "ENTER OU CLIQUE: INICIAR  /  ESC: VOLTAR AO MENU"
                                : "ENTER OU CLIQUE: NOVO JOGO  /  ESC: SAIR",
                MUTED, false);
}

static void round_result(const UserInterface *ui, const PrizeMachine *m) {
    float w = fminf(640, (float)ui_view_width(ui) - 40);
    float x = ((float)ui_view_width(ui) - w) * .5f, y = (float)ui->height * .43f;
    retro_panel(x, y, w, 145);
    render_text(x + 24, y + 38, m->round == ROUND_WON ? "MISSAO CUMPRIDA!" : "TEMPO ESGOTADO",
                m->round == ROUND_WON ? RETRO_CYAN : RETRO_PINK,
                true);
    char text[80];
    snprintf(text, sizeof(text), "%d de 3 aliens resgatados / %s", m->delivered,
             GAME_MODES[m->mode].name);
    render_text(x + 24, y + 75, text, TEXT, false);
    render_text(x + 24, y + 112, "R: tentar novamente   /   P: pausa e menu", MUTED, false);
}
void ui_draw(const UserInterface *ui, const PrizeMachine *m) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (ui->menu) {
        render_overlay_begin(ui->width, ui->height);
        menu(ui);
        render_overlay_end();
        draw_menu_preview(ui, m);
        render_overlay_begin(ui->width, ui->height);
        menu_preview_frame(ui);
        render_overlay_end();
        return;
    }
    render_overlay_begin(ui->width, ui->height);
    space_background(ui);
    render_overlay_end();
    draw_world(ui, m);
    render_overlay_begin(ui->width, ui->height);
    labels(ui, m);
    retro_bar(0, 0, (float)ui->width, UI_TOP);
    planet_icon(43, 37, 15);
    render_text(76, 36, "PIZZA PLANET", TEXT, true);
    render_text(76, 56, "A GARRA!   /   LACLAW LAB", GOLD, false);
    char clock[80];
    if (GAME_MODES[m->mode].time_limit > 0) {
        int seconds = (int)ceilf(m->time_remaining);
        snprintf(clock, sizeof(clock), "%s / %02d:%02d", GAME_MODES[m->mode].name, seconds / 60,
                 seconds % 60);
    } else
        snprintf(clock, sizeof(clock), "SANDBOX / SEM TEMPO LIMITE");
    retro_chip((float)ui->width - 500, 23, 300, 31,
               m->mode != MODE_SANDBOX && m->time_remaining < 30 ? RETRO_PINK : GOLD);
    render_text((float)ui->width - 480, 43, clock,
                m->mode != MODE_SANDBOX && m->time_remaining < 30 ? (Color){1, .32f, .30f} : GOLD,
                false);
    button((float)ui->width - 155, 23, 130, ui->paused ? "P  CONTINUAR" : "P  PAUSA", false);
    render_text(25, 105, ui->debug ? "DEPURACAO  /  D para ocultar" : "RESGATE A TRIPULACAO!",
                ui->debug ? (Color){1, .6f, .2f} : MUTED, false);
    panel(ui, m);
    float footer_y = (float)ui->height - UI_BOTTOM;
    retro_bar(0, footer_y, (float)ui_view_width(ui), UI_BOTTOM);
    retro_chip(18, footer_y + 9, (float)ui_view_width(ui) - 36, 52,
               ui->ik_mode ? RETRO_CYAN : RETRO_PINK);
    char ik_status[128];
    const char *status;
    if (m->message_time > 0)
        status = m->message;
    else if (m->delivered == PRIZE_COUNT)
        status = "Colecao completa! Pressione R para jogar de novo.";
    else if (m->held >= 0)
        status = "Leve o premio ate a bandeja verde e abra a garra.";
    else if (machine_capture_candidate(m, NULL) >= 0)
        status = "Captura possivel. Pressione C para fechar.";
    else if (ui->ik_mode && m->ik.target_active) {
        if (m->ik.orientation_active)
            snprintf(ik_status, sizeof(ik_status), "IK: %.3f m / %.1f deg",
                     (double)m->ik.error, (double)m->ik.orientation_error);
        else
            snprintf(ik_status, sizeof(ik_status),
                     "IK: erro %.3f; mova cursor ou clique em alien.", (double)m->ik.error);
        status = ik_status;
    } else if (ui->ik_mode)
        status = "IK ativo: mova o cursor; clique no alien para captura automatica.";
    else
        status = "Clique em um premio. Use 1-4 e as setas para controlar o braco.";
    render_text(26, (float)ui->height - 40, status, TEXT, true);
    render_text(26, (float)ui->height - 17,
                ui->free_camera ? "LIVRE / Direito: olhar / Meio: deslocar / Roda: mover / F: "
                                  "orbital / P: pausa / H: ajuda"
                                : "ORBITAL / Direito: girar / Meio: deslocar / Roda: zoom / F: "
                                  "livre / P: pausa / H: ajuda",
                MUTED, false);
    if (ui->debug) {
        Vec3 p = machine_grip_position(m);
        char text[120];
        snprintf(text, sizeof(text), "Garra: (%.3f, %.3f, %.3f)  |  movimentos rejeitados: %u",
                 (double)p.x, (double)p.y, (double)p.z, m->blocked_moves);
        retro_chip(20, 117, (float)ui_view_width(ui) - 40, 29, GOLD);
        render_text(28, 136, text, (Color){1, .7f, .3f}, false);
    }
    if (m->round != ROUND_PLAYING)
        round_result(ui, m);
    if (ui->help)
        help(ui);
    if (ui->paused)
        pause_menu(ui, m);
    render_overlay_end();
}
unsigned char ui_click(const UserInterface *ui, int x, int y) {
    int left = ui_view_width(ui) + 22;
    if (x < left - 6 || x > left + 276)
        return 0;
    if (y >= ui->height - 105 && y <= ui->height - 71)
        return x < left + 135 ? 'H' : 'R';
    y = UI_TOP + (int)((float)(y - UI_TOP) / panel_scale(ui));
    if (y >= 300 && y < 528) {
        int joint = (y - 300) / 57, offset = (y - 300) % 57;
        if (offset > 51)
            return 0;
        if (x >= left + 199 && x <= left + 230)
            return (unsigned char)('a' + joint); /* private button codes */
        if (x >= left + 238 && x <= left + 269)
            return (unsigned char)('e' + joint);
        return (unsigned char)('1' + joint);
    }
    if (y >= 610 && y <= 644)
        return x < left + 135 ? 'C' : 'O';
    return 0;
}
