#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define WINDOW_SIZE_X_MIN 200
#define WINDOW_SIZE_Y_MIN 200
#define GRID_LINE_WID 1
#define BOARD_ROW_SIZE 6
#define BOARD_COL_SIZE 7
#define DEFAULT_SIZE_X 400
#define DEFAULT_SIZE_Y 400
static char *FONT_NAME = "fixed";
static char *WINDOW_NAME = "Report 1";

typedef enum {
    BLACK_MOVE,
    WHITE_MOVE,
    GAME_OVER
} Game_state_t;

typedef enum {
    CELL_BLACK,
    CELL_WHITE,
    CELL_EMPTY
} Cell_state_t;

typedef struct othello {
    u_int64_t black;
    u_int64_t white;
    Game_state_t state;
    int size_x, size_y;
} Othello_t;

typedef struct Color_pixel {
    unsigned long black, black_highlight;
    unsigned long white, white_highlight;
    unsigned long board, board_highlight;
} Color_pixel_t;

typedef struct GCs {
    GC black, black_highlight;
    GC white, white_highlight;
    GC board, board_highlight;
} GCs_t;

typedef struct Grid {
    int row_num, col_num; // num of visible cells
    int cellsize_x, cellsize_y;
    int selected_x, selected_y;
    int size_x, size_y; // including invisible cells
    int pos_x, pos_y;   // including invisible cells
    int gap_size;
} Grid_t;

typedef struct X11othello {
    Othello_t game;
    Display *disp;
    Window  win;
    Grid_t grid;
    GCs_t   GCs;
    XFontStruct *font;
    Color_pixel_t color_pixel;
    Atom wm_delete_window;
} X11othello_t;

// X11othello_t othello;

static unsigned long _alloc_named_color (X11othello_t *othl, const char *color_name);
void alloc_named_colors (X11othello_t *othl);
void create_GCs (X11othello_t *othl);
void set_foregrounds (X11othello_t *othl);

void new_game (Othello_t *game, int row, int col);
void init (X11othello_t *othl, char **argv, int argc, int col_num, int row_num, int grid_gap);

int min (int a, int b);
int max (int a, int b);

bool is_on_grid (X11othello_t *othl, int cursor_x, int cursor_y, int *row, int *col);
void optimize_grid_pos (X11othello_t *othl, int win_width, int win_height);
void draw_string (X11othello_t *othl, const char *str, int xpos, int ypos);
void draw_cell (X11othello_t *othl, int row, int col);
void draw_grid (X11othello_t *othl);
void select_cell (X11othello_t *othl, int row, int col);
void highlight_cell_mouse_on (X11othello_t *othl);
void mouse_click (X11othello_t *othl);

void loop (X11othello_t *othl);



// XErrorHandler
// err_handler (Display *err_disp, XErrorEvent *err_event)
// {
//     char err_msg[256];
//     XGetErrorText(err_disp, err_event->error_code, err_msg, sizeof(err_msg));
//     printf("Error: %s\n", err_msg);
//     exit(EXIT_FAILURE);
//     return 0;
// }

void xerror (const char *msg)
{
    // printf()
}

// --------------------------------------------------
// <Color initializer>

static unsigned long
_alloc_named_color (X11othello_t *othl, const char *color_name)
{
    Colormap cmap = DefaultColormap(
        othl->disp,
        DefaultScreen(othl->disp)
    );
    XColor color, useless;
    int rslt;
    rslt = XAllocNamedColor(
        othl->disp,
        cmap,
        color_name,
        &color,
        &useless
    );
    // if (rslt == 0)
    //     xerror("XAllocNamedColor()");

    return color.pixel;
}

void alloc_named_colors (X11othello_t *othl)
{
    othl->color_pixel = (Color_pixel_t){
        .black = BlackPixel(othl->disp, DefaultScreen(othl->disp)),
        .white = WhitePixel(othl->disp, DefaultScreen(othl->disp)),
        .board = _alloc_named_color(othl, "ForestGreen"),
        .black_highlight = _alloc_named_color(othl, "DarkSlateGray"),
        .white_highlight = _alloc_named_color(othl, "Honeydew"),
        .board_highlight = _alloc_named_color(othl, "Green"),
    };

}

// </Color initializer>
// --------------------------------------------------
// <GC intializer and applier>
void create_GCs (X11othello_t *othl)
{
    othl->GCs = (GCs_t){
        .black = XCreateGC(othl->disp, othl->win, 0, NULL),
        .white = XCreateGC(othl->disp, othl->win, 0, NULL),
        .board = XCreateGC(othl->disp, othl->win, 0, NULL),
        .black_highlight = XCreateGC(othl->disp, othl->win, 0, NULL),
        .white_highlight = XCreateGC(othl->disp, othl->win, 0, NULL),
        .board_highlight = XCreateGC(othl->disp, othl->win, 0, NULL),
    };
}
void set_foregrounds (X11othello_t *othl)
{
    XSetForeground(
        othl->disp,
        othl->GCs.black,
        othl->color_pixel.black
    );
    XSetForeground(
        othl->disp,
        othl->GCs.white,
        othl->color_pixel.white
    );
    XSetForeground(
        othl->disp,
        othl->GCs.black_highlight,
        othl->color_pixel.black_highlight
    );
    XSetForeground(
        othl->disp,
        othl->GCs.white_highlight,
        othl->color_pixel.white_highlight
    );
    XSetForeground(
        othl->disp,
        othl->GCs.board,
        othl->color_pixel.board
    );
    XSetForeground(
        othl->disp,
        othl->GCs.board_highlight,
        othl->color_pixel.board_highlight
    );
}
// </GC initializer and applier>
// --------------------------------------------------
void new_game (Othello_t *game, int row, int col)
{
    assert(0<row && 0<col);
    assert(row*col <= sizeof(u_int64_t)*CHAR_BIT);

    game->black = 0;
    game->white = 0;
    game->state = BLACK_MOVE;

    game->size_x = row;
    game->size_y = col;
}

void
init (X11othello_t *othl, char **argv, int argc, int col_num, int row_num, int grid_gap)
{

    assert(0 <= row_num && row_num < 10);
    assert(0 <= col_num && col_num < 10);

    new_game(&othl->game, col_num, row_num);
    othl->grid.col_num = col_num;
    othl->grid.row_num = row_num;
    othl->grid.gap_size = grid_gap;
    // X11othello_t *othl = &othello;
    // XSetErrorHandler(err_handler);
    othl->disp = XOpenDisplay(NULL);
    // if (othl->disp == NULL)
    //     xerror("XOpenDisplay()");

    othl->win = XCreateSimpleWindow(
        othl->disp, RootWindow(othl->disp, DefaultScreen(othl->disp)),
        0, 0, DEFAULT_SIZE_X, DEFAULT_SIZE_Y, GRID_LINE_WID,
        BlackPixel(othl->disp, DefaultScreen(othl->disp)),
        WhitePixel(othl->disp, DefaultScreen(othl->disp))
    );
    // XMapWindow(othl->disp, othl->win);
    alloc_named_colors(othl);
    create_GCs(othl);
    set_foregrounds(othl);

    // Set up the font
    othl->font = XLoadQueryFont(othl->disp, FONT_NAME);
    if (othl->font == NULL)
        puts("font not found");
    XSetFont(othl->disp, othl->GCs.black, othl->font->fid);


    // puts("Good");

    // Set up the Event Mask
    XSelectInput(
        othl->disp, othl->win,
        ExposureMask | StructureNotifyMask |
        ButtonPressMask |
        PointerMotionMask | PointerMotionHintMask
    );

    // Set window name
    XTextProperty text_prop;
    XStringListToTextProperty(
        &WINDOW_NAME, 1, &text_prop
    );

    // Set window minimum size
    XSizeHints *size_hits = XAllocSizeHints();
    size_hits->flags = PMinSize;
    size_hits->min_width    = WINDOW_SIZE_X_MIN;
    size_hits->min_height   = WINDOW_SIZE_Y_MIN;

    XSetWMProperties(
        othl->disp, othl->win,
        &text_prop, &text_prop,
        argv, argc, size_hits, NULL, NULL
    );

    othl->wm_delete_window = XInternAtom(
        othl->disp, "WM_DELETE_WINDOW", False
    );
    XSetWMProtocols(
        othl->disp, othl->win,
        &othl->wm_delete_window, 1
    );

    XFree(text_prop.value);
    XFree(size_hits);

    XMapWindow(othl->disp, othl->win);
    XFlush(othl->disp);
}


int min (int a, int b)
{
    return(a<b) ? a : b;
}

int max (int a, int b)
{
    return (a>b) ? a : b;
}

bool
is_on_grid (X11othello_t *othl, int cursor_x, int cursor_y, int *row, int *col)
{
    Grid_t *grid = &othl->grid;

    *row = -1 + (cursor_y - grid->pos_y)/(grid->cellsize_y + grid->gap_size);
    *col = -1 + (cursor_x - grid->pos_x)/(grid->cellsize_x + grid->gap_size);

    if (0 <= *row && *row < grid->row_num &&
        0 <= *col && *col < grid->col_num)
        return true;

    return false;


}

void
optimize_grid_pos (X11othello_t *othl, int win_width, int win_height)
{
    // (col_num)x(row_num) for the game board
    // top row left column for labels
    // bottom row for status text
    // right column is empty

    int nCellx = othl->grid.col_num;
    int cellsize_x =
        (win_width - (nCellx+1)*othl->grid.gap_size)/(nCellx+2);

    int nCelly = othl->grid.row_num;
    int cellsize_y =
        (win_height - (nCelly+1)*othl->grid.gap_size)/(nCelly+2);

    if (cellsize_x < cellsize_y)
        othl->grid.cellsize_x = othl->grid.cellsize_y = cellsize_x;
    else
        othl->grid.cellsize_x = othl->grid.cellsize_y = cellsize_y;

    othl->grid.size_x = (nCellx+2)*othl->grid.cellsize_x + (nCellx+2);
    othl->grid.size_y = (nCelly+2)*othl->grid.cellsize_y + (nCelly+2);

    othl->grid.pos_x = win_width/2 - othl->grid.size_x/2;
    othl->grid.pos_y = win_height/2 - othl->grid.size_y/2;
}


void draw_string (X11othello_t *othl, const char *str, int xpos, int ypos)
{
    int width = XTextWidth(othl->font, str, strlen(str));
    int height = othl->font->ascent + othl->font->descent;

    XDrawString(
        othl->disp, othl->win,
        othl->GCs.black,
        xpos - width/2, ypos + height/2,
        str, strlen(str)
    );
}

void draw_cell (X11othello_t *othl, int row, int col)
{
    Grid_t *grid = &othl->grid;

    // 一番上の行と右の列はラベル用のセルなので飛ばす
    int x = grid->pos_x + (col+1)*(grid->cellsize_x + grid->gap_size);
    int y = grid->pos_y + (row+1)*(grid->cellsize_y + grid->gap_size);

    bool is_highlight = (
        row == grid->selected_x &&
        col == grid->selected_y
    );

    XFillRectangle(
        othl->disp, othl->win,
        is_highlight ? othl->GCs.board_highlight : othl->GCs.board,
        x, y, othl->grid.cellsize_x, othl->grid.cellsize_y
    );
}

void draw_grid (X11othello_t *othl)
{
    XClearWindow(othl->disp, othl->win);

    Grid_t *grid = &othl->grid;
    int nCellx = grid->col_num;
    int nCelly = grid->row_num;

    XFillRectangle(
        othl->disp, othl->win,
        othl->GCs.black,
        grid->pos_x + grid->cellsize_x,
        grid->pos_y + grid->cellsize_y,
        nCellx*grid->cellsize_x + (nCellx+1)*grid->gap_size,
        nCelly*grid->cellsize_y + (nCelly+1)*grid->gap_size
    );

    for (int row = 0; row < grid->row_num; row++) {
        int x = grid->pos_x + grid->cellsize_x/2;
        int y = grid->pos_y
                + (row + 1)*(grid->cellsize_y+grid->gap_size)
                + grid->cellsize_y/2;

        char label[] = {"123456789"[row], '\0'};
        draw_string(othl, label, x, y);
    }

    for (int col = 0; col < grid->col_num; col++) {
        int x = grid->pos_x
                + (col + 1)*(grid->cellsize_x+grid->gap_size)
                + grid->cellsize_x/2;
        int y = grid->pos_y + grid->cellsize_y/2;

        char label[] = {"ABCDEFGHIJ"[col], '\0'};
        draw_string(othl, label, x, y);
    }

    for (int row = 0; row < grid->row_num; row++)
        for (int col = 0; col < grid->col_num; col++)
            draw_cell(othl, row, col);
}

void select_cell (X11othello_t *othl, int row, int col)
{
    int old_row = othl->grid.selected_x;
    int old_col = othl->grid.selected_y;

    // すでに選択済み
    if (row == old_row && col == old_col)
        return;

    othl->grid.selected_x = row;
    othl->grid.selected_y = col;

    // ハイライトを消すため
    if (0 <= old_row)
        draw_cell(othl, old_row, old_col);

    if (0 <= row)
        draw_cell(othl, row, col);
}

void highlight_cell_mouse_on (X11othello_t *othl)
{
    Window root, child;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask;

    int rslt = XQueryPointer(
        othl->disp,
        othl->win,
        &root, &child,
        &root_x, &root_y,
        &win_x, &win_y,
        &mask
    );
    if (!rslt)
        return;

    int row, col;
    if (is_on_grid(othl, win_x, win_y, &row, &col))
        select_cell(othl, row, col);
    else
        select_cell(othl, -1, -1); // 以前のハイライトを消すため


}

void mouse_click (X11othello_t *othl)
{
    if (othl->game.state == GAME_OVER) {
        new_game(&othl->game, othl->game.size_x, othl->game.size_y);
        return;
    }
    if (othl->game.state == BLACK_MOVE) {

    }
    else if (othl->game.state == WHITE_MOVE) {

    }
        
}

int finalize (X11othello_t *othl)
{
    XFreeGC(othl->disp, othl->GCs.black);
    XFreeGC(othl->disp, othl->GCs.white);
    XFreeGC(othl->disp, othl->GCs.board);
    XFreeGC(othl->disp, othl->GCs.black_highlight);
    XFreeGC(othl->disp, othl->GCs.white_highlight);
    XFreeGC(othl->disp, othl->GCs.board_highlight);

    XFreeFont(othl->disp, othl->font);

    XCloseDisplay(othl->disp);
}

void loop (X11othello_t *othl)
{
    XEvent event;
    bool redraw_flg;
    for (;;) {
        if (redraw_flg) {
            draw_grid(othl);
            redraw_flg = false;
        }
        if (!XPending(othl->disp))
            continue;

        XNextEvent(othl->disp, &event);
        // XClearWindow(othl->disp, othl->win);
        // XDrawString(othl->disp, othl->win, othl->GCs.board,
        // 20, 20, "WE", 2);
        // draw_string(othl, &"123456789"[0], 0, 0);

        switch (event.type)
        {
            case ConfigureNotify:
                optimize_grid_pos(othl,
                    event.xconfigure.width,
                    event.xconfigure.height
                );
                break;
            case Expose:
                if (event.xexpose.count == 0)
                    redraw_flg = true;
                break;
            case MotionNotify:
                highlight_cell_mouse_on(othl);
                break;
            case ButtonPress:
                highlight_cell_mouse_on(othl);
                mouse_click(othl);
                redraw_flg = true;

                break;
            case ClientMessage:
                if (event.xclient.data.l[0] == othl->wm_delete_window) {
                    puts("Catch quit flag");
                    return;
                }

            default:
                break;
        }
    }
}

int main (int argc, char *argv[])
{
    X11othello_t othl;
    init(&othl, argv, argc, 7, 3, 1);

    loop(&othl);
    // getchar();

    finalize(&othl);

    return 0;
}