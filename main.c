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

int WIDTH = 800;
int HEIGHT = 600;
int RADIUS = 40;

/* Todo: convert to moved scale to allow displacement and zoom - See POS() */
Vector2 DISPL = (Vector2) { 0, 0 };
float SCALE = 0.5;

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
        Color color;
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
                .color = BLUE,
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
        node->color = RED;
        return node;
}


void
fsnode_draw(FS_Node *node)
{
        int fontsize = 20;
        for_da_each(n, &node->childs)
        {
                DrawLineV(POS(node->pos), POS((*n)->pos), WHITE);
        }
        DrawCircleV(POS(node->pos), RADIUS * SCALE, node->color);
        DrawText(node->name, POSX(node->pos.x - (RADIUS * SCALE) / 2.0), POSY(node->pos.y - fontsize / 2.0), fontsize * SCALE, WHITE);
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
                switch (entry->d_type) {
                case DT_DIR: n->color = ORANGE; break;
                case DT_LNK: n->color = MAGENTA; break;
                }
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
                        ClearBackground(BLACK);
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
