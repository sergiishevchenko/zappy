#include "gfx.hpp"
#include "raylib.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>

static const int TILE = 48;
static const char *RES_NAMES[] = {
    "food", "linemate", "deraumere", "sibur",
    "mendiane", "phiras", "thystame"
};

static Color res_color(int r)
{
    static const Color cols[] = {
        {120, 200, 80, 255}, {180, 180, 180, 255}, {200, 120, 80, 255},
        {80, 160, 220, 255}, {160, 80, 200, 255}, {220, 200, 80, 255},
        {200, 80, 160, 255}
    };
    if (r < 0 || r > 6)
        return WHITE;
    return cols[r];
}

static Color team_color(int i)
{
    static const Color cols[] = {
        {220, 80, 80, 255}, {80, 180, 220, 255}, {80, 220, 120, 255},
        {220, 180, 80, 255}, {180, 80, 220, 255}, {220, 120, 180, 255}
    };
    return cols[i % 6];
}

static int team_index(GameState &gs, const std::string &team)
{
    for (size_t i = 0; i < gs.teams.size(); i++)
        if (gs.teams[i] == team)
            return (int)i;
    return 0;
}

static void draw_orient_arrow(int px, int py, int orient, Color col)
{
    int cx = px + TILE / 2;
    int cy = py + TILE / 2;
    int dx = 0;
    int dy = 0;
    if (orient == 1) dy = -12;
    else if (orient == 2) dx = 12;
    else if (orient == 3) dy = 12;
    else if (orient == 4) dx = -12;
    DrawLine(cx, cy, cx + dx, cy + dy, col);
    DrawCircle(cx + dx, cy + dy, 4, col);
}

static void draw_hud(GameState &gs, int sel_x, int sel_y, bool has_sel,
    bool mode3d, bool music_on)
{
    DrawRectangle(0, 0, 260, GetScreenHeight(), {20, 20, 28, 255});
    DrawText("ZAPPY", 10, 10, 20, {255, 220, 100, 255});
    char buf[256];
    snprintf(buf, sizeof(buf), "Map: %dx%d  Freq: %d", gs.width, gs.height, gs.freq);
    DrawText(buf, 10, 40, 14, LIGHTGRAY);
    snprintf(buf, sizeof(buf), "Mode: %s  TAB toggle", mode3d ? "3D" : "2D");
    DrawText(buf, 10, 58, 12, {180, 200, 255, 255});
    snprintf(buf, sizeof(buf), "Music: GnG %s  M toggle", music_on ? "ON" : "OFF");
    DrawText(buf, 10, 74, 12, {180, 200, 255, 255});
    DrawText("+/- change time unit", 10, 90, 12, GRAY);
    int y = 110;
    for (auto &team : gs.teams) {
        int cnt = 0;
        int l8 = 0;
        for (auto &kv : gs.players) {
            if (kv.second.team == team && kv.second.alive) {
                cnt++;
                if (kv.second.level >= 8)
                    l8++;
            }
        }
        snprintf(buf, sizeof(buf), "%s: %d pl, L8:%d", team.c_str(), cnt, l8);
        DrawText(buf, 10, y, 14, {180, 220, 180, 255});
        y += 22;
    }
    if (has_sel && sel_x >= 0 && sel_y >= 0
        && sel_x < gs.width && sel_y < gs.height) {
        y += 8;
        snprintf(buf, sizeof(buf), "Tile (%d,%d):", sel_x, sel_y);
        DrawText(buf, 10, y, 14, {255, 220, 100, 255});
        y += 20;
        Tile &t = gs.map[sel_y][sel_x];
        for (int r = 0; r < 7; r++) {
            if (t.res[r] > 0) {
                snprintf(buf, sizeof(buf), "  %s: %d", RES_NAMES[r], t.res[r]);
                DrawText(buf, 10, y, 12, {180, 180, 200, 255});
                y += 16;
            }
        }
        for (auto &kv : gs.players) {
            Player &p = kv.second;
            if (p.alive && p.x == sel_x && p.y == sel_y) {
                snprintf(buf, sizeof(buf), "  #%d %s L%d o%d",
                    p.id, p.team.c_str(), p.level, p.orient);
                DrawText(buf, 10, y, 12, {200, 255, 200, 255});
                y += 16;
            }
        }
    }
}

static float tile_height(const Tile &t)
{
    float h = 0.2f;
    for (int r = 0; r < 7; r++)
        h += t.res[r] * 0.15f;
    return h;
}

static void draw_map_2d(GameState &gs, int cam_x, int cam_y,
    int sel_x, int sel_y, bool has_sel)
{
    if (gs.width <= 0 || gs.height <= 0)
        return;
    int cols = (GetScreenWidth() - 270) / TILE;
    int rows = (GetScreenHeight() - 100) / TILE;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int tx = (cam_x + col) % gs.width;
            int ty = (cam_y + row) % gs.height;
            if (tx < 0) tx += gs.width;
            if (ty < 0) ty += gs.height;
            int px = 270 + col * TILE;
            int py = 50 + row * TILE;
            DrawRectangle(px, py, TILE - 1, TILE - 1, {45, 45, 55, 255});
            DrawRectangleLines(px, py, TILE - 1, TILE - 1, {60, 60, 80, 255});
            if (has_sel && tx == sel_x && ty == sel_y) {
                DrawRectangleLinesEx(
                    {(float)px, (float)py, (float)TILE - 1, (float)TILE - 1},
                    3, {255, 220, 80, 255});
            }
            Tile &t = gs.map[ty][tx];
            int dot = 0;
            for (int r = 0; r < 7; r++) {
                int lim = t.res[r] < 3 ? t.res[r] : 3;
                for (int n = 0; n < lim; n++) {
                    DrawCircle(px + 8 + dot * 10, py + 8, 4, res_color(r));
                    dot++;
                }
            }
            for (auto &kv : gs.eggs) {
                Egg &e = kv.second;
                if (e.x == tx && e.y == ty)
                    DrawCircle(px + TILE / 2, py + TILE - 10, 6, {255, 255, 200, 255});
            }
            for (auto &inc : gs.incants) {
                if (inc.x == tx && inc.y == ty)
                    DrawCircleLines(px + TILE / 2, py + TILE / 2, 18, {255, 100, 255, 255});
            }
            int pi = 0;
            for (auto &kv : gs.players) {
                Player &p = kv.second;
                if (!p.alive || p.x != tx || p.y != ty)
                    continue;
                Color c = team_color(team_index(gs, p.team));
                DrawRectangle(px + 14, py + 20, 20, 20, c);
                char lbl[8];
                snprintf(lbl, sizeof(lbl), "%d", p.level);
                DrawText(lbl, px + 18, py + 22, 12, BLACK);
                draw_orient_arrow(px, py, p.orient, c);
                pi++;
                (void)pi;
            }
        }
    }
}

static void draw_map_3d(GameState &gs, int cam_x, int cam_y, Camera3D &camera)
{
    if (gs.width <= 0 || gs.height <= 0)
        return;
    int cols = 16;
    int rows = 12;
    BeginMode3D(camera);
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int tx = (cam_x + col) % gs.width;
            int ty = (cam_y + row) % gs.height;
            if (tx < 0) tx += gs.width;
            if (ty < 0) ty += gs.height;
            Tile &t = gs.map[ty][tx];
            float h = tile_height(t);
            Vector3 pos = {(float)col * 1.2f, h / 2.0f, (float)row * 1.2f};
            Color base = {55, 55, 70, 255};
            int total = 0;
            for (int r = 0; r < 7; r++)
                total += t.res[r];
            if (total > 0)
                base = res_color(total % 7);
            base.a = 220;
            DrawCube(pos, 1.0f, h, 1.0f, base);
            DrawCubeWires(pos, 1.0f, h, 1.0f, {30, 30, 40, 255});
        }
    }
    for (auto &kv : gs.players) {
        Player &p = kv.second;
        if (!p.alive)
            continue;
        int col = p.x - cam_x;
        int row = p.y - cam_y;
        if (col < 0 || col >= cols || row < 0 || row >= rows)
            continue;
        Vector3 pos = {(float)col * 1.2f, 1.5f + p.level * 0.1f, (float)row * 1.2f};
        Color c = team_color(team_index(gs, p.team));
        DrawCube(pos, 0.6f, 1.2f + p.level * 0.08f, 0.6f, c);
    }
    for (auto &kv : gs.eggs) {
        Egg &e = kv.second;
        int col = e.x - cam_x;
        int row = e.y - cam_y;
        if (col < 0 || col >= cols || row < 0 || row >= rows)
            continue;
        Vector3 pos = {(float)col * 1.2f, 0.4f, (float)row * 1.2f};
        DrawSphere(pos, 0.3f, {255, 255, 180, 255});
    }
    EndMode3D();
}

static void rotate_local(int orient, int lx, int ly, int &wx, int &wy)
{
    wx = lx;
    wy = ly;
    for (int i = 1; i < orient; i++) {
        int nx = wy;
        int ny = -wx;
        wx = nx;
        wy = ny;
    }
}

static bool world_to_screen(GameState &gs, int cam_x, int cam_y,
    int tx, int ty, int &px, int &py)
{
    int cols = (GetScreenWidth() - 270) / TILE;
    int rows = (GetScreenHeight() - 100) / TILE;
    int dx = tx - cam_x;
    int dy = ty - cam_y;

    while (dx < 0)
        dx += gs.width;
    while (dy < 0)
        dy += gs.height;
    dx %= gs.width;
    dy %= gs.height;
    if (dx >= cols || dy >= rows)
        return false;
    px = 270 + dx * TILE + TILE / 2;
    py = 50 + dy * TILE + TILE / 2;
    return true;
}

static void draw_sound_waves(GameState &gs, int cam_x, int cam_y, int64_t t)
{
    static const int LX[9] = {0, 0, -1, -1, -1, 0, 1, 1, 1};
    static const int LY[9] = {0, -1, -1, 0, 1, 1, 1, 0, -1};

    for (auto &b : gs.broadcasts) {
        int cx;
        int cy;
        if (!world_to_screen(gs, cam_x, cam_y, b.x, b.y, cx, cy))
            continue;
        float age = (float)(t - b.start_ms) / 3000.0f;
        if (age < 0.0f)
            age = 0.0f;
        if (age > 1.0f)
            continue;
        unsigned char alpha = (unsigned char)(220 * (1.0f - age));
        float ring = age * TILE * 2.5f;
        DrawCircleLines(cx, cy, ring, {255, 200, 80, alpha});
        DrawCircleLines(cx, cy, ring * 0.6f, {255, 240, 120, alpha});
        for (int k = 1; k <= 8; k++) {
            int lx = LX[k];
            int ly = LY[k];
            int wx;
            int wy;
            rotate_local(b.orient, lx, ly, wx, wy);
            int sx = cx + wx * TILE / 2;
            int sy = cy + wy * TILE / 2;
            Color c = {255, 180, 60, alpha};
            DrawLine(cx, cy, sx, sy, c);
            DrawCircle(sx, sy, 10 + (int)(age * 8), c);
            char lbl[4];
            snprintf(lbl, sizeof(lbl), "%d", k);
            DrawText(lbl, sx - 4, sy - 6, 12, {255, 255, 200, 255});
        }
        DrawCircle(cx, cy, 8, {255, 120, 40, 255});
    }
}

static Sound make_beep()
{
    Wave beep;
    memset(&beep, 0, sizeof(beep));
    beep.frameCount = 2205;
    beep.sampleRate = 22050;
    beep.sampleSize = 16;
    beep.channels = 1;
    short *data = (short *)MemAlloc(2205 * sizeof(short));
    for (int i = 0; i < 2205; i++)
        data[i] = (short)(32767 * 0.2 * (i % 50 < 25 ? 1 : -1));
    beep.data = data;
    Sound snd = LoadSoundFromWave(beep);
    UnloadWave(beep);
    return snd;
}

struct GngNote {
    int freq;
    int ms;
};

static Sound make_gng_music()
{
    static const GngNote theme[] = {
        {392, 140}, {466, 140}, {523, 140}, {587, 140},
        {622, 220}, {587, 140}, {523, 140}, {466, 140},
        {392, 140}, {523, 140}, {622, 140}, {784, 260},
        {622, 140}, {587, 140}, {523, 140}, {466, 140},
        {392, 360}, {0, 200},
        {523, 140}, {622, 140}, {784, 140}, {932, 260},
        {784, 140}, {622, 140}, {523, 140}, {466, 140},
        {392, 500}, {0, 0}
    };
    const int rate = 22050;
    int total_ms = 0;

    for (int i = 0; theme[i].freq || theme[i].ms; i++)
        total_ms += theme[i].ms;
    int frames = rate * (total_ms + 400) / 1000;
    Wave w;
    memset(&w, 0, sizeof(w));
    w.frameCount = frames;
    w.sampleRate = rate;
    w.sampleSize = 16;
    w.channels = 1;
    short *data = (short *)MemAlloc(frames * sizeof(short));
    memset(data, 0, frames * sizeof(short));
    int pos = 0;
    for (int i = 0; theme[i].freq || theme[i].ms; i++) {
        int note_frames = theme[i].ms * rate / 1000;
        float freq = (float)theme[i].freq;
        for (int f = 0; f < note_frames && pos < frames; f++, pos++) {
            if (freq <= 0) {
                data[pos] = 0;
                continue;
            }
            float phase = fmodf(freq * (float)f / rate, 1.0f);
            data[pos] = (phase < 0.5f) ? 10000 : -10000;
        }
    }
    w.data = data;
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

int run_gfx(const char *host, int port)
{
    NetClient net(host, port);
    if (!net.connect())
        return 84;
    const int sw = 1280;
    const int sh = 800;
    InitWindow(sw, sh, "Zappy GFX");
    SetTargetFPS(60);
    InitAudioDevice();
    Sound beep = make_beep();
    Sound music = make_gng_music();
    SetSoundVolume(music, 0.25f);
    bool music_on = true;
    PlaySound(music);
    int cam_x = 0;
    int cam_y = 0;
    int sel_x = -1;
    int sel_y = -1;
    bool has_sel = false;
    bool mode3d = false;
    size_t prev_bcasts = 0;
    Camera3D camera = {};
    camera.position = {8.0f, 14.0f, 18.0f};
    camera.target = {8.0f, 0.0f, 8.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    while (!WindowShouldClose() && net.connected()) {
        if (music_on && !IsSoundPlaying(music))
            PlaySound(music);
        if (IsKeyPressed(KEY_TAB))
            mode3d = !mode3d;
        if (IsKeyPressed(KEY_M)) {
            music_on = !music_on;
            if (music_on)
                PlaySound(music);
            else
                StopSound(music);
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            int f;
            {
                std::lock_guard<std::mutex> lk(net.state().mtx);
                f = net.state().freq + 50;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "sst %d", f);
            net.send_cmd(buf);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            int f;
            {
                std::lock_guard<std::mutex> lk(net.state().mtx);
                f = net.state().freq - 50;
                if (f < 50)
                    f = 50;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "sst %d", f);
            net.send_cmd(buf);
        }
        if (IsKeyPressed(KEY_LEFT)) cam_x--;
        if (IsKeyPressed(KEY_RIGHT)) cam_x++;
        if (IsKeyPressed(KEY_UP)) cam_y--;
        if (IsKeyPressed(KEY_DOWN)) cam_y++;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !mode3d) {
            Vector2 m = GetMousePosition();
            if (m.x > 260 && m.y > 40) {
                sel_x = cam_x + (int)(m.x - 270) / TILE;
                sel_y = cam_y + (int)(m.y - 50) / TILE;
                has_sel = true;
            }
        }
        BeginDrawing();
        ClearBackground({30, 30, 40, 255});
        {
            std::lock_guard<std::mutex> lk(net.state().mtx);
            GameState &gs = net.state();
            int64_t t = now_ms();
            while (!gs.broadcasts.empty() && gs.broadcasts.front().until_ms < t)
                gs.broadcasts.pop_front();
            while (!gs.server_msgs.empty() && gs.server_msgs.front().until_ms < t)
                gs.server_msgs.pop_front();
            while (!gs.incants.empty() && gs.incants.front().until_ms < t)
                gs.incants.pop_front();
            if (gs.broadcasts.size() > prev_bcasts)
                PlaySound(beep);
            prev_bcasts = gs.broadcasts.size();
            if (mode3d)
                draw_map_3d(gs, cam_x, cam_y, camera);
            else {
                draw_map_2d(gs, cam_x, cam_y, sel_x, sel_y, has_sel);
                draw_sound_waves(gs, cam_x, cam_y, t);
            }
            draw_hud(gs, sel_x, sel_y, has_sel, mode3d, music_on);
            int by = sh - 90;
            for (auto &s : gs.server_msgs) {
                char buf[256];
                snprintf(buf, sizeof(buf), "[srv] %s", s.text.c_str());
                DrawText(buf, 270, by, 14, {150, 220, 255, 255});
                by += 18;
            }
            for (auto &b : gs.broadcasts) {
                char buf[256];
                snprintf(buf, sizeof(buf), ">> #%d: %s",
                    b.player_id, b.text.c_str());
                DrawText(buf, 270, by, 14, {255, 200, 100, 255});
                by += 18;
            }
            if (!gs.winner.empty()) {
                DrawRectangle(sw / 2 - 150, sh / 2 - 40, 300, 80,
                    {40, 120, 60, 230});
                std::string msg = "Winner: " + gs.winner;
                DrawText(msg.c_str(), sw / 2 - 80, sh / 2 - 10, 20, WHITE);
            }
        }
        EndDrawing();
    }
    UnloadSound(beep);
    UnloadSound(music);
    CloseAudioDevice();
    net.disconnect();
    CloseWindow();
    return 0;
}
