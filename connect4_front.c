#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "connect4.h"

#define WINDOW_SIZE_X_MIN 200
#define WINDOW_SIZE_Y_MIN 200
#define GRID_LINE_WID 1
#define BOARD_ROW_NUM 6
#define BOARD_COL_NUM 7
#define DEFAULT_SIZE_X 400
#define DEFAULT_SIZE_Y 400
#define BUF_MAX 128
static int DEFAULT_PORT_NO = 20000;
static char *FONT_NAME = "fixed";
static char *WINDOW_NAME = "Report 1";


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
    int selected_row, selected_col;
    int size_x, size_y; // including invisible cells
    int pos_x, pos_y;   // including invisible cells
    int gap_size;
} Grid_t;

typedef struct X11othello {
    Connect4_t game;
    Game_state_t my_move;
    int sock_fd;
    Display *disp;
    Window  win;
    Grid_t grid;
    GCs_t   GCs;
    XFontStruct *font;
    Color_pixel_t color_pixel;
    Atom wm_delete_window;
} X11Connect4_t;

typedef enum {
    CONNECT4_SERVER_ROLE,
    CONNECT4_CLIENT_ROLE
} Connect4_role_t;

// X11Connect4_t othello;

static unsigned long _alloc_named_color (X11Connect4_t *cnct4, const char *color_name);
void alloc_named_colors (X11Connect4_t *cnct4);
void create_GCs (X11Connect4_t *cnct4);
void set_foregrounds (X11Connect4_t *cnct4);
static int init_sock (char *host_name, int port_no, Connect4_role_t role);

void init (X11Connect4_t *cnct4, char **argv, int argc,
            int col_num, int row_num, int grid_gap,
            Connect4_role_t role, char *host_name, int port_no);

int min (int a, int b);
int max (int a, int b);

bool is_on_grid (X11Connect4_t *cnct4, int cursor_x, int cursor_y, int *row, int *col);
void optimize_grid_pos (X11Connect4_t *cnct4, int win_width, int win_height);
void draw_string (X11Connect4_t *cnct4, const char *str, int xpos, int ypos);
void draw_cell (X11Connect4_t *cnct4, int row, int col);
void draw_grid (X11Connect4_t *cnct4);
void select_cell (X11Connect4_t *cnct4, int row, int col);
void highlight_cell_mouse_on (X11Connect4_t *cnct4);
void mouse_click (X11Connect4_t *cnct4);

void loop (X11Connect4_t *cnct4);



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
_alloc_named_color (X11Connect4_t *cnct4, const char *color_name)
{
    Colormap cmap = DefaultColormap(
        cnct4->disp,
        DefaultScreen(cnct4->disp)
    );
    XColor color, useless;
    int rslt;
    rslt = XAllocNamedColor(
        cnct4->disp,
        cmap,
        color_name,
        &color,
        &useless
    );
    // if (rslt == 0)
    //     xerror("XAllocNamedColor()");

    return color.pixel;
}

void alloc_named_colors (X11Connect4_t *cnct4)
{
    cnct4->color_pixel = (Color_pixel_t){
        .black = BlackPixel(cnct4->disp, DefaultScreen(cnct4->disp)),
        .white = WhitePixel(cnct4->disp, DefaultScreen(cnct4->disp)),
        .board = _alloc_named_color(cnct4, "ForestGreen"),
        .black_highlight = _alloc_named_color(cnct4, "DarkSlateGray"),
        .white_highlight = _alloc_named_color(cnct4, "Honeydew"),
        .board_highlight = _alloc_named_color(cnct4, "Green"),
    };

}

// </Color initializer>
// --------------------------------------------------
// <GC intializer and applier>
void create_GCs (X11Connect4_t *cnct4)
{
    cnct4->GCs = (GCs_t){
        .black = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
        .white = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
        .board = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
        .black_highlight = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
        .white_highlight = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
        .board_highlight = XCreateGC(cnct4->disp, cnct4->win, 0, NULL),
    };
}
void set_foregrounds (X11Connect4_t *cnct4)
{
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.black,
        cnct4->color_pixel.black
    );
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.white,
        cnct4->color_pixel.white
    );
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.black_highlight,
        cnct4->color_pixel.black_highlight
    );
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.white_highlight,
        cnct4->color_pixel.white_highlight
    );
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.board,
        cnct4->color_pixel.board
    );
    XSetForeground(
        cnct4->disp,
        cnct4->GCs.board_highlight,
        cnct4->color_pixel.board_highlight
    );
}
// </GC initializer and applier>
// --------------------------------------------------
// <Network initializer>
static int
init_sock (char *host_name, int port_no, Connect4_role_t role)
{
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(port_no)
    };
    // strcpy((char*)&addr.sin_addr, gethostbyname(host_name)->h_addr);
    memcpy((char*)&addr.sin_addr, gethostbyname(host_name)->h_addr, gethostbyname(host_name)->h_length);

    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (role == CONNECT4_SERVER_ROLE) {
        if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        listen(sock_fd, 1);

        puts("Waiting a client connecting...");
        int temp_fd = sock_fd;
        sock_fd = accept(temp_fd, NULL, NULL);
        close(temp_fd);
        if (sock_fd < 0) {
            perror("accept");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        puts("Established connects successfully!!");
    }
    else {
        // role == CONNECT4_CLIENT_ROLE
        puts("Connecting to the server...");
        if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        puts("Connected!!");
    }

    return sock_fd;
}

// </Network initializer>
// --------------------------------------------------
void
init (X11Connect4_t *cnct4, char **argv, int argc,
        int col_num, int row_num, int grid_gap,
        Connect4_role_t role, char *host_name, int port_no)
{

    assert(0 <= row_num && row_num < 10);
    assert(0 <= col_num && col_num < 10);

    cnct4->sock_fd = init_sock(host_name, port_no, role);
    cnct4->my_move = (role == CONNECT4_SERVER_ROLE) ? BLACK_MOVE : WHITE_MOVE;

    new_game(&cnct4->game, col_num, row_num);
    cnct4->grid.col_num = col_num;
    cnct4->grid.row_num = row_num;
    cnct4->grid.gap_size = grid_gap;
    // X11Connect4_t *cnct4 = &othello;
    // XSetErrorHandler(err_handler);
    cnct4->disp = XOpenDisplay(NULL);
    // if (cnct4->disp == NULL)
    //     xerror("XOpenDisplay()");

    cnct4->win = XCreateSimpleWindow(
        cnct4->disp, RootWindow(cnct4->disp, DefaultScreen(cnct4->disp)),
        0, 0, DEFAULT_SIZE_X, DEFAULT_SIZE_Y, GRID_LINE_WID,
        BlackPixel(cnct4->disp, DefaultScreen(cnct4->disp)),
        WhitePixel(cnct4->disp, DefaultScreen(cnct4->disp))
    );
    // XMapWindow(cnct4->disp, cnct4->win);
    alloc_named_colors(cnct4);
    create_GCs(cnct4);
    set_foregrounds(cnct4);

    // Set up the font
    cnct4->font = XLoadQueryFont(cnct4->disp, FONT_NAME);
    if (cnct4->font == NULL)
        puts("font not found");
    XSetFont(cnct4->disp, cnct4->GCs.black, cnct4->font->fid);


    // puts("Good");

    // Set up the Event Mask
    XSelectInput(
        cnct4->disp, cnct4->win,
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
        cnct4->disp, cnct4->win,
        &text_prop, &text_prop,
        argv, argc, size_hits, NULL, NULL
    );

    cnct4->wm_delete_window = XInternAtom(
        cnct4->disp, "WM_DELETE_WINDOW", False
    );
    XSetWMProtocols(
        cnct4->disp, cnct4->win,
        &cnct4->wm_delete_window, 1
    );

    XFree(text_prop.value);
    XFree(size_hits);

    XMapWindow(cnct4->disp, cnct4->win);
    XFlush(cnct4->disp);
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
is_on_grid (X11Connect4_t *cnct4, int cursor_x, int cursor_y, int *row, int *col)
{
    Grid_t *grid = &cnct4->grid;

    *row = -1 + (cursor_y - grid->pos_y)/(grid->cellsize_y + grid->gap_size);
    *col = -1 + (cursor_x - grid->pos_x)/(grid->cellsize_x + grid->gap_size);

    if (0 <= *row && *row < grid->row_num &&
        0 <= *col && *col < grid->col_num)
        return true;

    return false;


}

void
optimize_grid_pos (X11Connect4_t *cnct4, int win_width, int win_height)
{
    // (col_num)x(row_num) for the game board
    // top row left column for labels
    // bottom row for status text
    // right column is empty

    int nCellx = cnct4->grid.col_num;
    int cellsize_x =
        (win_width - (nCellx+1)*cnct4->grid.gap_size)/(nCellx+2);

    int nCelly = cnct4->grid.row_num;
    int cellsize_y =
        (win_height - (nCelly+1)*cnct4->grid.gap_size)/(nCelly+2);

    if (cellsize_x < cellsize_y)
        cnct4->grid.cellsize_x = cnct4->grid.cellsize_y = cellsize_x;
    else
        cnct4->grid.cellsize_x = cnct4->grid.cellsize_y = cellsize_y;

    cnct4->grid.size_x = (nCellx+2)*cnct4->grid.cellsize_x + (nCellx+2);
    cnct4->grid.size_y = (nCelly+2)*cnct4->grid.cellsize_y + (nCelly+2);

    cnct4->grid.pos_x = win_width/2 - cnct4->grid.size_x/2;
    cnct4->grid.pos_y = win_height/2 - cnct4->grid.size_y/2;
}


void draw_string (X11Connect4_t *cnct4, const char *str, int xpos, int ypos)
{
    int width = XTextWidth(cnct4->font, str, strlen(str));
    int height = cnct4->font->ascent + cnct4->font->descent;

    XDrawString(
        cnct4->disp, cnct4->win,
        cnct4->GCs.black,
        xpos - width/2, ypos + height/2,
        str, strlen(str)
    );
}

void draw_cell (X11Connect4_t *cnct4, int row, int col)
{
    Grid_t *grid = &cnct4->grid;

    // 一番上の行と右の列はラベル用のセルなので飛ばす
    int x = grid->pos_x + (col+1)*(grid->cellsize_x + grid->gap_size);
    int y = grid->pos_y + (row+1)*(grid->cellsize_y + grid->gap_size);

    bool is_highlight = (
        row == grid->selected_row &&
        col == grid->selected_col
    );

    XFillRectangle(
        cnct4->disp, cnct4->win,
        is_highlight ? cnct4->GCs.board_highlight : cnct4->GCs.board,
        x, y, cnct4->grid.cellsize_x, cnct4->grid.cellsize_y
    );

    Cell_state_t cs;

    if ((cs = connect4_get_cell_state(&cnct4->game, row, col)) != CELL_EMPTY) {
        XFillArc(cnct4->disp, cnct4->win,
            cs == BLACK_MOVE ? cnct4->GCs.black : cnct4->GCs.white,
            x, y, grid->cellsize_x, grid->cellsize_y,
            0, 360 * 64
        );
    }
}

void draw_grid (X11Connect4_t *cnct4)
{
    XClearWindow(cnct4->disp, cnct4->win);

    Grid_t *grid = &cnct4->grid;
    int nCellx = grid->col_num;
    int nCelly = grid->row_num;

    XFillRectangle(
        cnct4->disp, cnct4->win,
        cnct4->GCs.black,
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
        draw_string(cnct4, label, x, y);
    }

    for (int col = 0; col < grid->col_num; col++) {
        int x = grid->pos_x
                + (col + 1)*(grid->cellsize_x+grid->gap_size)
                + grid->cellsize_x/2;
        int y = grid->pos_y + grid->cellsize_y/2;

        char label[] = {"ABCDEFGHIJ"[col], '\0'};
        draw_string(cnct4, label, x, y);
    }

    for (int row = 0; row < grid->row_num; row++)
        for (int col = 0; col < grid->col_num; col++)
            draw_cell(cnct4, row, col);
}

void select_cell (X11Connect4_t *cnct4, int row, int col)
{
    int old_col = cnct4->grid.selected_col;
    int old_row = cnct4->grid.selected_row;

    // すでに選択済み
    if (row == old_row && col == old_col)
        return;

    cnct4->grid.selected_col = col;
    cnct4->grid.selected_row = row;

    // ハイライトを消すため
    if (0 <= old_row)
        draw_cell(cnct4, old_row, old_col);

    if (0 <= row)
        draw_cell(cnct4, row, col);
}

void highlight_cell_mouse_on (X11Connect4_t *cnct4)
{
    Window root, child;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int mask;

    int rslt = XQueryPointer(
        cnct4->disp,
        cnct4->win,
        &root, &child,
        &root_x, &root_y,
        &win_x, &win_y,
        &mask
    );
    if (!rslt)
        return;

    int row, col;
    if (is_on_grid(cnct4, win_x, win_y, &row, &col))
        select_cell(cnct4, row, col);
    else
        select_cell(cnct4, -1, -1); // 以前のハイライトを消すため


}

void mouse_click (X11Connect4_t *cnct4)
{
    if (connect4_get_game_state(&cnct4->game) != cnct4->my_move)
        return;

    Grid_t *grid = &cnct4->grid;
    if (!is_valid_move(&cnct4->game, grid->selected_row, grid->selected_col))
        return;

    connect4_make_move(&cnct4->game, cnct4->grid.selected_row, cnct4->grid.selected_col);

    char buf[BUF_MAX];
    snprintf(buf, sizeof(buf), "PLACE-%d%d", grid->selected_col, grid->selected_row);
    if (write(cnct4->sock_fd, buf, strlen(buf)) < 0) {
        perror("write");
        return;
    }
}

int finalize (X11Connect4_t *cnct4)
{
    XFreeGC(cnct4->disp, cnct4->GCs.black);
    XFreeGC(cnct4->disp, cnct4->GCs.white);
    XFreeGC(cnct4->disp, cnct4->GCs.board);
    XFreeGC(cnct4->disp, cnct4->GCs.black_highlight);
    XFreeGC(cnct4->disp, cnct4->GCs.white_highlight);
    XFreeGC(cnct4->disp, cnct4->GCs.board_highlight);

    XFreeFont(cnct4->disp, cnct4->font);

    XCloseDisplay(cnct4->disp);

    close(cnct4->sock_fd);
}

void loop (X11Connect4_t *cnct4)
{
    char buf[BUF_MAX];
    fd_set fd_mask;
    XEvent event;
    bool redraw_flg;
    char *ERROR_MSG = "ERROR";
    char *YOUWIN_MSG = "YOU-WIN";
    for (;;) {
        FD_ZERO(&fd_mask);
        FD_SET(cnct4->sock_fd, &fd_mask);
        int select_ret = select(cnct4->sock_fd + 1,
                    &fd_mask, NULL, NULL,
                    &(struct timeval){.tv_usec = 1}
        );
        if (select_ret < 0) {
            perror("select");
            return;
        }

        if (FD_ISSET(cnct4->sock_fd, &fd_mask)) {
            redraw_flg = true;
            ssize_t len = read(cnct4->sock_fd, buf, sizeof(buf));
            if (len < 0) {
                perror("read");
                return;
            }
            if (strstr(buf, "PLACE-")) {
                // my move, not opposit's move
                if (connect4_get_game_state(&cnct4->game) == cnct4->my_move) {
                    puts("Error: it is your turn, but the oppsit made move");
                    if (write(cnct4->sock_fd, ERROR_MSG, strlen(ERROR_MSG)) < 0) {
                        perror("write");
                        return;
                    }
                    return;
                }
                char xc, yc;
                sscanf(buf, "PLACE-%c%c", &xc, &yc);
                int col = xc - '0';
                int row = yc - '0';
                if (!is_valid_move(&cnct4->game, row, col)) {
                    if (write(cnct4->sock_fd, ERROR_MSG, strlen(ERROR_MSG)) < 0) {
                        perror("write");
                        return;
                    }
                    puts("Error: Invalid move by the opposit");
                    // printf("%d:%d\n", row, col);
                    return;
                }
                connect4_make_move(&cnct4->game, row, col);
                if (connect4_get_game_state(&cnct4->game) == GAME_OVER)
                    if (connect4_get_game_result(&cnct4->game) != connect4_get_my_win_result_value(cnct4->my_move)) {
                        write(cnct4->sock_fd, YOUWIN_MSG, strlen(YOUWIN_MSG));
                        puts("You Lose");
                        return;
                    }
            }
            else if (strstr(buf, ERROR_MSG)) {
                if (connect4_get_game_result(&cnct4->game) == GAME_DRAW) {
                    puts("Game: Draw");
                }
                else {
                    puts("Some error occured!!");
                    return;
                }
            }
            else if (strstr(buf, YOUWIN_MSG)) {
                puts("congratulations!! You win!!");
                return;
            }
            else {
                puts("Recieved invalid messeage");
                if (write(cnct4->sock_fd, ERROR_MSG, strlen(ERROR_MSG)) < 0) {
                    perror("write");
                    return;
                }
                return;
            }
        }
        if (redraw_flg) {
            draw_grid(cnct4);
            redraw_flg = false;
        }
        if (!XPending(cnct4->disp))
            continue;

        XNextEvent(cnct4->disp, &event);
        // XClearWindow(cnct4->disp, cnct4->win);
        // XDrawString(cnct4->disp, cnct4->win, cnct4->GCs.board,
        // 20, 20, "WE", 2);
        // draw_string(cnct4, &"123456789"[0], 0, 0);

        switch (event.type)
        {
            case ConfigureNotify:
                optimize_grid_pos(cnct4,
                    event.xconfigure.width,
                    event.xconfigure.height
                );
                break;
            case Expose:
                if (event.xexpose.count == 0)
                    redraw_flg = true;
                break;
            case MotionNotify:
                highlight_cell_mouse_on(cnct4);
                break;
            case ButtonPress:
                highlight_cell_mouse_on(cnct4);
                mouse_click(cnct4);
                redraw_flg = true;

                break;
            case ClientMessage:
                if (event.xclient.data.l[0] == cnct4->wm_delete_window) {
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
    X11Connect4_t cnct4;
    Connect4_role_t role;
    char buf[BUF_MAX];

    // Select role
    do {
        printf("Select your role\n");
        printf("%d: Server\n", CONNECT4_SERVER_ROLE);
        printf("%d: Client\n", CONNECT4_CLIENT_ROLE);
        fgets(buf, sizeof(buf), stdin);
        role = (Connect4_role_t)strtol(buf, NULL, 10);
    } while (role != CONNECT4_SERVER_ROLE && role != CONNECT4_CLIENT_ROLE);

    // Host name
    if (role == CONNECT4_CLIENT_ROLE) {
        do {
            printf("Input server's host name: ");
            fgets(buf, sizeof(buf), stdin);
        } while (strlen(buf) == 0 || buf[0] == '\n');

        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
    }
    else {
        int ret = gethostname(buf, sizeof(buf));
        if (ret < 0) {
            perror("gethostname");
            return 1;
        }
        printf("This server name: %s\n", buf);
    }

    init(&cnct4, argv, argc,
            BOARD_COL_NUM, BOARD_ROW_NUM, 1,
            role, buf, DEFAULT_PORT_NO);

    loop(&cnct4);
    // getchar();

    finalize(&cnct4);

    return 0;
}