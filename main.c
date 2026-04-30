#include <dirent.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "./raylib-6.0_linux_amd64/include/raylib.h"
#include "assert.h"
#include "croma.h"
#include "flag.h"

#define rand_between(x, y) ((x) + (rand() % ((y) - (x))))
#define rand_choose(x) ((x)[rand() % (sizeof(x) / sizeof(*x))])

enum Color_Type {
        C_UNKNOWN = DT_UNKNOWN,
        C_DEFAULT = C_UNKNOWN,
        C_FIFO = DT_FIFO,
        C_CHR = DT_CHR,
        C_DIR = DT_DIR,
        C_BLK = DT_BLK,
        C_REG = DT_REG,
        C_LNK = DT_LNK,
        C_SOCK = DT_SOCK,
        C_WHT = DT_WHT,
        C_BG,
        C_ROOT,
        C_LINE,
};


#define GRUV_BG ((Color) { \
.a = 0xFF,                 \
.r = 0x1d,                 \
.g = 0x20,                 \
.b = 0x21,                 \
})

#define GRUV_FG ((Color) { \
.a = 0xFF,                 \
.r = 0xd4,                 \
.g = 0xbe,                 \
.b = 0x98,                 \
})

#define GRUV_BLACK ((Color) { \
.a = 0xFF,                    \
.r = 0x92,                    \
.g = 0x83,                    \
.b = 0x74,                    \
})

#define GRUV_RED ((Color) { \
.a = 0xFF,                  \
.r = 0xea,                  \
.g = 0x69,                  \
.b = 0x62,                  \
})

#define GRUV_GREEN ((Color) { \
.a = 0xFF,                    \
.r = 0xa9,                    \
.g = 0xb6,                    \
.b = 0x65,                    \
})

#define GRUV_YELLOW ((Color) { \
.a = 0xFF,                     \
.r = 0xe7,                     \
.g = 0x8a,                     \
.b = 0x4e,                     \
})

#define GRUV_BLUE ((Color) { \
.a = 0xFF,                   \
.r = 0x7d,                   \
.g = 0xae,                   \
.b = 0xa3,                   \
})

#define GRUV_MAGENTA ((Color) { \
.a = 0xFF,                      \
.r = 0xd3,                      \
.g = 0x86,                      \
.b = 0x9b,                      \
})

#define GRUV_CYAN ((Color) { \
.a = 0xFF,                   \
.r = 0x89,                   \
.g = 0xb4,                   \
.b = 0x82,                   \
})

#define GRUV_WHITE ((Color) { \
.a = 0xFF,                    \
.r = 0xdd,                    \
.g = 0xc7,                    \
.b = 0xa1,                    \
})

#define COLOR_TEXT GRUV_FG
#define COLOR_BG GRUV_BG

struct Color_Pair {
        Color fg, bg;
};

static const struct Color_Pair colors_lut[] = {
        [C_DEFAULT] = { COLOR_TEXT, COLOR_BG },
        [C_FIFO] = { COLOR_TEXT, COLOR_BG },
        [C_CHR] = { COLOR_TEXT, COLOR_BG },
        [C_DIR] = { COLOR_TEXT, GRUV_BLUE },
        [C_BLK] = { COLOR_TEXT, COLOR_BG },
        [C_REG] = { COLOR_TEXT, COLOR_BG },
        [C_LNK] = { COLOR_TEXT, COLOR_BG },
        [C_SOCK] = { COLOR_TEXT, COLOR_BG },
        [C_WHT] = { COLOR_TEXT, COLOR_BG },
        [C_BG] = { COLOR_BG },
        [C_ROOT] = { GRUV_RED, GRUV_RED },
        [C_LINE] = { GRUV_BLACK },
};

int WIDTH = 800;
int HEIGHT = 600;
int RADIUS = 40;

Vector2 DISPL = (Vector2) { 0, 0 };
float SCALE = 1.;

bool need_update = true;

typedef struct FS_Node FS_Node;

struct FS_Node {
        struct {
                int capacity;
                int count;
                FS_Node **items;
        } childs;

        char *path; // fs path
        char *name; // fs basename

        // graphical representation
        Vector2 pos;
        unsigned int custom_pos : 1;
        struct Color_Pair color;
        bool fill;
};

float
POSX(float x)
{
        return (x + DISPL.x) * SCALE;
}

float
POSY(float y)
{
        return (y + DISPL.y) * SCALE;
}

Vector2
POS(Vector2 pos)
{
        Vector2 v = (Vector2) { .x = POSX(pos.x), .y = POSY(pos.y) };
        return v;
}

float
SOPX(float x)
{
        return x / SCALE - DISPL.x;
}

float
SOPY(float y)
{
        return y / SCALE - DISPL.y;
}

Vector2
SOP(Vector2 pos)
{
        Vector2 v = (Vector2) { .x = SOPX(pos.x), .y = SOPY(pos.y) };
        return v;
}

FS_Node *
get_grabbed_node(FS_Node *root, Vector2 raw_pos)
{
        if (CheckCollisionPointCircle(raw_pos, root->pos, RADIUS)) {
                return root;
        }

        for_da_each(n, &root->childs)
        {
                FS_Node *node = get_grabbed_node(*n, raw_pos);
                if (node) return node;
        }
        return NULL;
}

void fsnode_recalc_child_positions(FS_Node *parent);
void fsnode_populate(FS_Node *node);

const double DB_CLICK_LAPSE = 0.3;
const double NO_GRAB_LAPSE = 0.1;

void
mouse_event(FS_Node *root)
{
        static FS_Node *grabbed_node = NULL;
        static double last_click = 0;
        static double grab_start = 0;
        static Vector2 left_down;
        static bool left_down_updated = false;
        Vector2 pos = (Vector2) { -1, -1 };
        bool pos_updated = false;
        double t = 0;

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (!pos_updated) {
                        pos = SOP(GetMousePosition());
                        pos_updated = true;
                }
                if (t == 0) t = GetTime();
                if (grab_start == 0) grab_start = t;
                if (!grabbed_node) {
                        grabbed_node = get_grabbed_node(root, pos);
                }

                if (grabbed_node) {
                        if (t - grab_start > NO_GRAB_LAPSE) {
                                grabbed_node->pos = pos;
                                grabbed_node->custom_pos = 1;
                                fsnode_recalc_child_positions(grabbed_node);
                                need_update = 1;
                        }
                        left_down_updated = false;
                }

                if (!grabbed_node && left_down_updated) {
                        Vector2 pos = GetMousePosition();
                        DISPL.x += (-left_down.x + pos.x) / SCALE;
                        DISPL.y += (-left_down.y + pos.y) / SCALE;
                        left_down_updated = false;
                }

                if (!left_down_updated) {
                        left_down = GetMousePosition();
                        left_down_updated = true;
                }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                if (!pos_updated) {
                        pos = SOP(GetMousePosition());
                        pos_updated = true;
                }
                if (t == 0) t = GetTime();
                grabbed_node = NULL;
                grab_start = 0;
                left_down_updated = false;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (!pos_updated) {
                        pos = SOP(GetMousePosition());
                        pos_updated = true;
                }
                if (t == 0) t = GetTime();
                if (t - last_click <= DB_CLICK_LAPSE) {
                        if (grabbed_node) fsnode_populate(grabbed_node);
                }
                last_click = t;
                left_down_updated = false;
        }

        float scroll;
        const float SCROLL_FACTOR = 5.;
        float SCALE_PERCENT = 100 * SCALE / 1.;
        if ((scroll = GetMouseWheelMove())) {
                SCALE_PERCENT += scroll * SCROLL_FACTOR;
                SCALE = SCALE_PERCENT / 100;
                printf("SCALE: %f (%f%%)\n", SCALE, SCALE_PERCENT);
                grabbed_node = NULL;
                grab_start = 0;
                left_down_updated = false;
        }
}

FS_Node *
fsnode_alloc()
{
        return malloc(sizeof(FS_Node));
}

void
fsnode_recalc_child_positions(FS_Node *parent)
{
        int n = parent->childs.count;
        float rad = M_PI * 2 / n;
        double sep = (RADIUS) / sin(M_PI / n) + RADIUS + 20;

        if (n == 0) return;

        for (int i = 0; i < n; i++) {
                FS_Node *child = parent->childs.items[i];
                if (child->custom_pos) continue;
                int qq = rad / M_PI / 2;
                int sqq = qq == 0 || qq == 1 ? 1 : -1;
                int cqq = qq == 0 || qq == 3 ? 1 : -1;
                child->pos.x = parent->pos.x + cos(rad * i) * sep * sqq;
                child->pos.y = parent->pos.y + sin(rad * i) * sep * cqq;
                need_update = 1;
        }

        for_da_each(c, &parent->childs)
        {
                fsnode_recalc_child_positions(*c);
        }
}

void
fsnode_append_child(FS_Node *parent, FS_Node *child)
{
        da_append(&parent->childs, child);
        fsnode_recalc_child_positions(parent);
}

FS_Node *
fsnode_init(FS_Node *node, FS_Node *parent, char *path)
{
        printf("Adding node for: %s\n", path);
        need_update = 1;

        *node = (FS_Node) {
                .path = strdup(path),
                .color = colors_lut[C_DEFAULT],
        };
        node->name = basename(node->path);

        if (parent) {
                // position is calculated here
                fsnode_append_child(parent, node);
        } else {
                node->pos = (Vector2) { WIDTH / 2., HEIGHT / 2. };
        }

        return node;
}

FS_Node *
fsnode_root(char *path)
{
        FS_Node *node = fsnode_init(fsnode_alloc(), NULL, path);
        node->color = colors_lut[C_ROOT];
        node->fill = false;
        return node;
}


void
fsnode_draw(FS_Node *node)
{
        int fontsize = 20;
        for_da_each(n, &node->childs)
        {
                Vector2 P = POS(node->pos);
                Vector2 Q = POS((*n)->pos);
                float m = (P.x - Q.x) / (P.y - Q.y);
                float y = (RADIUS * SCALE) / sqrtf(1 + m * m);
                float s = P.y < Q.y ? 1 : -1;
                Vector2 P2 = (Vector2) { P.x + m * y * s, P.y + y * s };
                Vector2 Q2 = (Vector2) { Q.x - m * y * s, Q.y - y * s };
                DrawLineV(P2, Q2, colors_lut[C_LINE].fg);
        }

        if (node->fill) {
                DrawCircleV(POS(node->pos), RADIUS * SCALE, node->color.bg);
        } else {
                DrawCircleLinesV(POS(node->pos), RADIUS * SCALE, node->color.bg);
        }

        DrawText(node->name, POSX(node->pos.x - (RADIUS * SCALE) / 2.0), POSY(node->pos.y - fontsize / 2.0), fontsize * SCALE, node->color.fg);
        for_da_each(n, &node->childs)
        {
                fsnode_draw(*n);
        }
}

char *
path_cat(const char *a, const char *b)
{
        static char buf[1024] = { 0 };
        snprintf(buf, sizeof buf - 1, "%s/%s", a, b);
        return buf;
}

void
fsnode_populate(FS_Node *node)
{
        if (node->childs.count != 0) {
                node->childs.count = 0;
                return;
        }

        DIR *dir = opendir(node->path);
        if (dir == NULL) return;
        struct dirent *entry;
        while ((entry = readdir(dir))) {
                if (!strcmp(entry->d_name, ".")
                    // || !strcmp(entry->d_name, "..")
                    ) continue;
                FS_Node *n = fsnode_init(fsnode_alloc(), node, path_cat(node->path, entry->d_name));
                n->color = colors_lut[entry->d_type];
        }
        closedir(dir);
}

int
main(int argc, char **argv)
{
        char *initial_path = NULL;

        flag_program(.help = "QCFS - by Hugo Coto, credits to Adrián Quiroga");
        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        }

        SetTraceLogLevel(LOG_WARNING);
        InitWindow(WIDTH, HEIGHT, "QCFS interface");
        SetTargetFPS(60);
        SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

        initial_path = "/home/hugo/code/qcfs/test";
        assert(initial_path);
        printf("Initial path: %s\n", initial_path);


        FS_Node *root = fsnode_root(initial_path);

        assert(IsWindowReady());
        while (!WindowShouldClose()) {
                BeginDrawing();
                if (need_update) {
                        ClearBackground(colors_lut[C_BG].fg);
                        fsnode_draw(root);
                        need_update = false | 1; // dont use this variable
                }
                mouse_event(root);
                EndDrawing();
        }

        CloseWindow();
        flag_free();

        return 0;
}
