#include "app/ui.h"
#include "cg/render.h"

#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const Color INK = {.028f, .035f, .105f}, PANEL = {.045f, .055f, .15f};
static const Color TEXT = {1, .96f, .83f}, MUTED = {.57f, .65f, .83f};
static const Color ACCENT = {.69f, .91f, .24f}, GOLD = {1, .81f, .18f};

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
    render_rect(x, y, w, 34, accent ? (Color){.67f, .075f, .12f} : (Color){.10f, .14f, .28f});
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
        render_rect(screen.x - 38, screen.y + UI_TOP - 16, 92, 22, INK);
        render_text(screen.x - 32, screen.y + UI_TOP, label,
                    m->selected == i ? ACCENT : MACHINE_CONFIG.prize_colors[i], false);
    }
    Vec3 screen;
    AABB tray = scene_bounds(m->solids[m->tray_solid].node);
    Vec3 label = vec3((tray.min.x + tray.max.x) * .5f, tray.max.y + .04f, tray.max.z - .1f);
    if (camera_project(&ui->camera, label, ui_view_width(ui), ui_view_height(ui), &screen) &&
        screen.x > 50 && screen.x < ui_view_width(ui) - 60 && screen.y < ui_view_height(ui) - 12) {
        render_rect(screen.x - 44, screen.y + UI_TOP - 15, 90, 22, INK);
        render_text(screen.x - 38, screen.y + UI_TOP, "ENTREGA", ACCENT, false);
    }
}
static void panel(const UserInterface *ui, const PrizeMachine *m) {
    float x = (float)ui_view_width(ui), left = x + 22;
    char text[128];
    render_rect(x, UI_TOP, UI_PANEL_WIDTH, (float)(ui->height - UI_TOP - UI_BOTTOM), PANEL);
    render_rect(x, UI_TOP, 1, (float)(ui->height - UI_TOP - UI_BOTTOM), (Color){.15f, .21f, .27f});
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
            render_rect(left - 7, y - 1, 278, 28, (Color){.10f, .19f, .23f});
        alien_icon(left + 8, y + 11, 8);
        render_text(left + 30, y + 15, m->prizes[i].name, MACHINE_CONFIG.prize_colors[i], false);
        render_text(left + 139, y + 15, prize_state_name(m->prizes[i].state),
                    m->prizes[i].state == PRIZE_DELIVERED ? ACCENT : MUTED, false);
    }
    render_rect(left, 262, 272, 1, (Color){.15f, .21f, .27f});
    render_text(left, 287, "COMANDO DA GARRA   /   1 a 4", MUTED, false);
    for (int i = 0; i < JOINT_COUNT; ++i) {
        float y = 300 + (float)i * 57;
        bool active = ui->active_joint == i;
        render_rect(left - 6, y, 282, 51,
                    active ? (Color){.10f, .21f, .25f} : (Color){.07f, .10f, .14f});
        if (active)
            render_rect(left - 6, y, 3, 51, ACCENT);
        snprintf(text, sizeof(text), "%d  %s", i + 1, MACHINE_CONFIG.joints[i].name);
        render_text(left + 6, y + 18, text, active ? ACCENT : TEXT, false);
        snprintf(text, sizeof(text), "[%g, %g]", (double)MACHINE_CONFIG.joints[i].min,
                 (double)MACHINE_CONFIG.joints[i].max);
        render_text(left + 6, y + 39, text, MUTED, false);
        snprintf(text, sizeof(text), "%6.1f", (double)m->arm.angle[i]);
        render_text(left + 112, y + 30, text, TEXT, true);
        render_rect(left + 199, y + 9, 31, 32, (Color){.14f, .22f, .28f});
        render_text(left + 210, y + 30, "-", TEXT, true);
        render_rect(left + 238, y + 9, 31, 32, (Color){.14f, .22f, .28f});
        render_text(left + 248, y + 30, "+", TEXT, true);
    }
    render_text(left, 553, "GARRA", MUTED, false);
    snprintf(text, sizeof(text), "%4.1f / 40 graus", (double)m->arm.opening);
    render_text(left + 139, 553, text, TEXT, false);
    render_rect(left, 565, 270, 4, (Color){.14f, .20f, .26f});
    render_rect(left, 565, 270 * m->arm.opening / 40, 4, ACCENT);
    const char *status = m->delivered == PRIZE_COUNT               ? "Colecao completa!"
                         : m->held >= 0                            ? "Garra ocupada"
                         : machine_capture_candidate(m, NULL) >= 0 ? "Captura possivel"
                                                                   : "Aproxime a garra aberta";
    render_text(left, 593, status,
                m->held >= 0 || machine_capture_candidate(m, NULL) >= 0 ? ACCENT : MUTED, false);
    button(left, 610, 130, "C  FECHAR", true);
    button(left + 140, 610, 130, "O  ABRIR", false);
    render_text(left, 668, "Setas: mover  |  +/-: 5 graus", MUTED, false);
    render_text(left, 687, "[ e ]: ajuste de 1 grau", MUTED, false);
    glPopMatrix();
    button(left, ui->height - 105, 130, "H  AJUDA", false);
    button(left + 140, ui->height - 105, 130, "R  REINICIAR", false);
}
static void help(const UserInterface *ui) {
    float x = 32, y = 166, w = (float)ui_view_width(ui) - 64;
    render_rect(x, y, w, 386, (Color){.045f, .075f, .11f});
    render_rect(x, y, 4, 386, ACCENT);
    render_text(x + 24, y + 37, "COMO JOGAR", ACCENT, true);
    const char *lines[] = {"1. Clique em um premio para destaca-lo.",
                           "2. Escolha a junta com 1, 2, 3 ou 4.",
                           "3. Use as setas esquerda/direita para mover.",
                           "   +/- muda o alvo em 5 graus; [ e ] em 1 grau.",
                           "4. Aproxime a garra aberta. C fecha; O abre e solta.",
                           "5. Transporte e solte na bandeja verde.",
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
UIRect ui_mode_rect(const UserInterface *ui, int mode) {
    float w = fminf(660, (float)ui->width - 48);
    float h = fminf(76, ((float)ui->height - 230) / 4);
    return (UIRect){((float)ui->width - w) * .5f, 178 + (float)mode * (h + 12), w, h};
}
int ui_mode_at(const UserInterface *ui, int x, int y) {
    if (!ui->menu)
        return -1;
    for (int i = 0; i < MODE_COUNT; ++i) {
        UIRect r = ui_mode_rect(ui, i);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            return i;
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
    render_rect(x, y, first.w + 48, 330, PANEL);
    render_rect(x, y, 4, 330, GOLD);
    render_text(first.x, y + 39, "PARTIDA PAUSADA", GOLD, true);
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
static void menu(const UserInterface *ui) {
    UserInterface background = *ui;
    background.width += UI_PANEL_WIDTH;
    space_background(&background);
    UIRect first = ui_mode_rect(ui, 0);
    render_text(first.x, 112, "ESCOLHA SUA MISSAO", GOLD, true);
    render_text(first.x, 143, "Resgate os 3 aliens. Clique em um modo ou pressione 1-4.", TEXT,
                false);
    const char *descriptions[] = {
        "Sem limite. Bandeja ampla; pouca variacao; quiques suaves.",
        "4 min. Bandeja menor; pouca variacao; quiques leves.",
        "3 min. Bandeja reduzida; distancias variadas; quiques moderados.",
        "2 min. Menor bandeja; maior variacao; quiques fortes."};
    const Color colors[] = {
        {.69f, .91f, .24f}, {.24f, .80f, .74f}, {1, .81f, .18f}, {1, .32f, .30f}};
    for (int i = 0; i < MODE_COUNT; ++i) {
        UIRect r = ui_mode_rect(ui, i);
        render_rect(r.x, r.y, r.w, r.h, PANEL);
        render_rect(r.x, r.y, 4, r.h, colors[i]);
        char title[48];
        snprintf(title, sizeof(title), "%d  %s", i + 1, GAME_MODES[i].name);
        render_text(r.x + 22, r.y + 29, title, colors[i], true);
        render_text(r.x + 22, r.y + r.h - 16, descriptions[i], TEXT, false);
    }
    UIRect last = ui_mode_rect(ui, MODE_COUNT - 1);
    render_text(first.x, last.y + last.h + 36,
                "Nova partida e reset sorteiam as posicoes. Esc: sair.", MUTED, false);
}
static void round_result(const UserInterface *ui, const PrizeMachine *m) {
    float w = fminf(640, (float)ui_view_width(ui) - 40);
    float x = ((float)ui_view_width(ui) - w) * .5f, y = (float)ui->height * .43f;
    render_rect(x, y, w, 145, PANEL);
    render_rect(x, y, 4, 145, m->round == ROUND_WON ? ACCENT : GOLD);
    render_text(x + 24, y + 38, m->round == ROUND_WON ? "MISSAO CUMPRIDA!" : "TEMPO ESGOTADO", GOLD,
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
        return;
    }
    render_overlay_begin(ui->width, ui->height);
    space_background(ui);
    render_overlay_end();
    draw_world(ui, m);
    render_overlay_begin(ui->width, ui->height);
    labels(ui, m);
    render_rect(0, 0, (float)ui->width, UI_TOP, INK);
    planet_icon(43, 37, 15);
    render_text(76, 36, "PIZZA PLANET", TEXT, true);
    render_text(76, 56, "A GARRA!   /   LACLAW LAB", GOLD, false);
    render_rect(0, UI_TOP - 3, (float)ui->width, 3, (Color){.82f, .08f, .13f});
    char clock[80];
    if (GAME_MODES[m->mode].time_limit > 0) {
        int seconds = (int)ceilf(m->time_remaining);
        snprintf(clock, sizeof(clock), "%s / %02d:%02d", GAME_MODES[m->mode].name, seconds / 60,
                 seconds % 60);
    } else
        snprintf(clock, sizeof(clock), "SANDBOX / SEM TEMPO LIMITE");
    render_text((float)ui->width - 480, 43, clock,
                m->mode != MODE_SANDBOX && m->time_remaining < 30 ? (Color){1, .32f, .30f} : GOLD,
                false);
    button((float)ui->width - 155, 23, 130, ui->paused ? "P  CONTINUAR" : "P  PAUSA", false);
    render_text(25, 105, ui->debug ? "DEPURACAO  /  D para ocultar" : "RESGATE A TRIPULACAO!",
                ui->debug ? (Color){1, .6f, .2f} : MUTED, false);
    panel(ui, m);
    render_rect(0, (float)ui->height - UI_BOTTOM, (float)ui_view_width(ui), UI_BOTTOM, INK);
    const char *status = m->message_time > 0 ? m->message
                         : m->delivered == PRIZE_COUNT
                             ? "Colecao completa! Pressione R para jogar de novo."
                         : m->held >= 0 ? "Leve o premio ate a bandeja verde e abra a garra."
                         : machine_capture_candidate(m, NULL) >= 0
                             ? "Captura possivel. Pressione C para fechar."
                             : "Clique em um premio. Use 1-4 e as setas para controlar o braco.";
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
        render_rect(20, 117, (float)ui_view_width(ui) - 40, 29, INK);
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
