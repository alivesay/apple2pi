/*
 * Copyright 2013, David Schmenk
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define BAUDRATE B115200
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define die(str, args...) do { \
        prlog(str); \
        exit(-1); \
    } while(0)
/*
 * Apple II request entry
 */
#define MAX_XFER        32

struct a2request {
        int  fd;
        int  type;
        int  addr;
        int  count;
        int  xfer;
        char *buffer;
        struct a2request *next;
} *a2reqlist = NULL, *a2reqfree = NULL;

/*
 * ASCII to scancode conversion
 */
#define MOD_FN          0x80
#define MOD_CTRL        0x8000
#define MOD_ALT         0x4000
#define MOD_SHIFT       0x2000
#define KEY_CODE        0x03FF
#define KEY_PRESS       0x80
#define KEY_ASCII       0x7F

int keycode[256] = {
        /*
         * normal scancode
         */
        MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
        MOD_CTRL |             KEY_A,           // CTRL-A code 01
        MOD_CTRL |             KEY_B,           // CTRL-B code 02
        MOD_CTRL |             KEY_C,           // CTRL-C code 03
        MOD_CTRL |             KEY_D,           // CTRL-D code 04
        MOD_CTRL |             KEY_E,           // CTRL-E code 05
        MOD_CTRL |             KEY_F,           // CTRL-F code 06
        MOD_CTRL |             KEY_G,           // CTRL-G code 07
                               KEY_LEFT,        // CTRL-H code 08
                               KEY_TAB,         // CTRL-I code 09
                               KEY_DOWN,        // CTRL-J code 0A
                               KEY_UP,          // CTRL-K code 0B
        MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                               KEY_ENTER,       // CTRL-M code 0D
        MOD_CTRL |             KEY_N,           // CTRL-N code 0E
        MOD_CTRL |             KEY_O,           // CTRL-O code 0F
        MOD_CTRL |             KEY_P,           // CTRL-P code 10
        MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
        MOD_CTRL |             KEY_R,           // CTRL-R code 12
        MOD_CTRL |             KEY_S,           // CTRL-S code 13
        MOD_CTRL |             KEY_T,           // CTRL-T code 14
                               KEY_RIGHT,       // CTRL-U code 15
        MOD_CTRL |             KEY_V,           // CTRL-V code 16
        MOD_CTRL |             KEY_W,           // CTRL-W code 17
        MOD_CTRL |             KEY_X,           // CTRL-X code 18
        MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
        MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
                               KEY_ESC,         // ESCAPE code 1B
        MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
        MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
        MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
        MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                               KEY_SPACE,       // ' '    code 20
                   MOD_SHIFT | KEY_1,           // !      code 21
                   MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
                   MOD_SHIFT | KEY_3,           // #      code 23
                   MOD_SHIFT | KEY_4,           // $      code 24
                   MOD_SHIFT | KEY_5,           // %      code 25
                   MOD_SHIFT | KEY_7,           // &      code 26
                               KEY_APOSTROPHE,  // '      code 27
                   MOD_SHIFT | KEY_9,           // (      code 28
                   MOD_SHIFT | KEY_0,           // )      code 29
                   MOD_SHIFT | KEY_8,           // *      code 2A
                   MOD_SHIFT | KEY_EQUAL,       // +      code 2B
                               KEY_COMMA,       // ,      code 2C
                               KEY_MINUS,       // -      code 2D
                               KEY_DOT,         // .      code 2E
                               KEY_SLASH,       // /      code 2F
                               KEY_0,           // 0      code 30
                               KEY_1,           // 1      code 31
                               KEY_2,           // 2      code 32
                               KEY_3,           // 3      code 33
                               KEY_4,           // 4      code 34
                               KEY_5,           // 5      code 35
                               KEY_6,           // 6      code 36
                               KEY_7,           // 7      code 37
                               KEY_8,           // 8      code 38
                               KEY_9,           // 9      code 39
                   MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                               KEY_SEMICOLON,   // ;      code 3B
                   MOD_SHIFT | KEY_COMMA,       // <      code 3C
                               KEY_EQUAL,       // =      code 3D
                   MOD_SHIFT | KEY_DOT,         // >      code 3E
                   MOD_SHIFT | KEY_SLASH,       // ?      code 3F
                   MOD_SHIFT | KEY_2,           // @      code 40
                   MOD_SHIFT | KEY_A,           // A      code 41
                   MOD_SHIFT | KEY_B,           // B      code 42
                   MOD_SHIFT | KEY_C,           // C      code 43
                   MOD_SHIFT | KEY_D,           // D      code 44
                   MOD_SHIFT | KEY_E,           // E      code 45
                   MOD_SHIFT | KEY_F,           // F      code 46
                   MOD_SHIFT | KEY_G,           // G      code 47
                   MOD_SHIFT | KEY_H,           // H      code 48
                   MOD_SHIFT | KEY_I,           // I      code 49
                   MOD_SHIFT | KEY_J,           // J      code 4A
                   MOD_SHIFT | KEY_K,           // K      code 4B
                   MOD_SHIFT | KEY_L,           // L      code 4C
                   MOD_SHIFT | KEY_M,           // M      code 4D
                   MOD_SHIFT | KEY_N,           // N      code 4E
                   MOD_SHIFT | KEY_O,           // O      code 4F
                   MOD_SHIFT | KEY_P,           // P      code 50
                   MOD_SHIFT | KEY_Q,           // Q      code 51
                   MOD_SHIFT | KEY_R,           // R      code 52
                   MOD_SHIFT | KEY_S,           // S      code 53
                   MOD_SHIFT | KEY_T,           // T      code 54
                   MOD_SHIFT | KEY_U,           // U      code 55
                   MOD_SHIFT | KEY_V,           // V      code 56
                   MOD_SHIFT | KEY_W,           // W      code 57
                   MOD_SHIFT | KEY_X,           // X      code 58
                   MOD_SHIFT | KEY_Y,           // Y      code 59
                   MOD_SHIFT | KEY_Z,           // Z      code 5A
                               KEY_LEFTBRACE,   // [      code 5B
                               KEY_BACKSLASH,   // \      code 5C
                               KEY_RIGHTBRACE,  // ]      code 5D
                   MOD_SHIFT | KEY_6,           // ^      code 5E
                   MOD_SHIFT | KEY_MINUS,       // _      code 5F
                               KEY_GRAVE,       // `      code 60
                               KEY_A,           // a      code 61
                               KEY_B,           // b      code 62
                               KEY_C,           // c      code 63
                               KEY_D,           // d      code 64
                               KEY_E,           // e      code 65
                               KEY_F,           // f      code 66
                               KEY_G,           // g      code 67
                               KEY_H,           // h      code 68
                               KEY_I,           // i      code 69
                               KEY_J,           // j      code 6A
                               KEY_K,           // k      code 6B
                               KEY_L,           // l      code 6C
                               KEY_M,           // m      code 6D
                               KEY_N,           // n      code 6E
                               KEY_O,           // o      code 6F
                               KEY_P,           // p      code 70
                               KEY_Q,           // q      code 71
                               KEY_R,           // r      code 72
                               KEY_S,           // s      code 73
                               KEY_T,           // t      code 74
                               KEY_U,           // u      code 75
                               KEY_V,           // v      code 76
                               KEY_W,           // w      code 77
                               KEY_X,           // x      code 78
                               KEY_Y,           // y      code 79
                               KEY_Z,           // z      code 7A
                   MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
                   MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
                   MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
                   MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                               KEY_BACKSPACE,   // BS     code 7F                   
        /*
         * w/ closed apple scancodes
         */
        MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
        MOD_CTRL |             KEY_A,           // CTRL-A code 01
        MOD_CTRL |             KEY_B,           // CTRL-B code 02
        MOD_CTRL |             KEY_C,           // CTRL-C code 03
        MOD_CTRL |             KEY_D,           // CTRL-D code 04
        MOD_CTRL |             KEY_E,           // CTRL-E code 05
        MOD_CTRL |             KEY_F,           // CTRL-F code 06
        MOD_CTRL |             KEY_G,           // CTRL-G code 07
                               KEY_HOME,        // CTRL-H code 08
                               KEY_INSERT,      // CTRL-I code 09
                               KEY_PAGEDOWN,    // CTRL-J code 0A
                               KEY_PAGEUP,      // CTRL-K code 0B
        MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                               KEY_LINEFEED,    // CTRL-M code 0D
        MOD_CTRL |             KEY_N,           // CTRL-N code 0E
        MOD_CTRL |             KEY_O,           // CTRL-O code 0F
        MOD_CTRL |             KEY_P,           // CTRL-P code 10
        MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
        MOD_CTRL |             KEY_R,           // CTRL-R code 12
        MOD_CTRL |             KEY_S,           // CTRL-S code 13
        MOD_CTRL |             KEY_T,           // CTRL-T code 14
                               KEY_END,         // CTRL-U code 15
        MOD_CTRL |             KEY_V,           // CTRL-V code 16
        MOD_CTRL |             KEY_W,           // CTRL-W code 17
        MOD_CTRL |             KEY_X,           // CTRL-X code 18
        MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
        MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
                               KEY_ESC,         // ESCAPE code 1B
        MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
        MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
        MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
        MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                               KEY_SPACE,       // ' '    code 20
		                       KEY_F11,         // !      code 21
                   MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
			                   KEY_F13,         // #      code 23
                               KEY_F14,         // $      code 24
							   KEY_F15,         // %      code 25
							   KEY_F17,         // &      code 26
                               KEY_APOSTROPHE,  // '      code 27
                               KEY_F19,         // (      code 28
                               KEY_F20,         // )      code 29
                               KEY_F18,         // *      code 2A
                   MOD_SHIFT | KEY_EQUAL,       // +      code 2B
                               KEY_COMMA,       // ,      code 2C
                               KEY_MINUS,       // -      code 2D
                               KEY_DOT,         // .      code 2E
                               KEY_SLASH,       // /      code 2F
                               KEY_F10,         // 0      code 30
                               KEY_F1,          // 1      code 31
                               KEY_F2,          // 2      code 32
                               KEY_F3,          // 3      code 33
                               KEY_F4,          // 4      code 34
                               KEY_F5,          // 5      code 35
                               KEY_F6,          // 6      code 36
                               KEY_F7,          // 7      code 37
                               KEY_F8,          // 8      code 38
                               KEY_F9,          // 9      code 39
                   MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                               KEY_SEMICOLON,   // ;      code 3B
                   MOD_SHIFT | KEY_COMMA,       // <      code 3C
                               KEY_EQUAL,       // =      code 3D
                   MOD_SHIFT | KEY_DOT,         // >      code 3E
                   MOD_SHIFT | KEY_SLASH,       // ?      code 3F
                               KEY_F12,         // @      code 40
                   MOD_SHIFT | KEY_A,           // A      code 41
                   MOD_SHIFT | KEY_B,           // B      code 42
                   MOD_SHIFT | KEY_C,           // C      code 43
                   MOD_SHIFT | KEY_D,           // D      code 44
                   MOD_SHIFT | KEY_E,           // E      code 45
                   MOD_SHIFT | KEY_F,           // F      code 46
                   MOD_SHIFT | KEY_G,           // G      code 47
                   MOD_SHIFT | KEY_H,           // H      code 48
                   MOD_SHIFT | KEY_I,           // I      code 49
                   MOD_SHIFT | KEY_J,           // J      code 4A
                   MOD_SHIFT | KEY_K,           // K      code 4B
                   MOD_SHIFT | KEY_L,           // L      code 4C
                   MOD_SHIFT | KEY_M,           // M      code 4D
                   MOD_SHIFT | KEY_N,           // N      code 4E
                   MOD_SHIFT | KEY_O,           // O      code 4F
                   MOD_SHIFT | KEY_P,           // P      code 50
                   MOD_SHIFT | KEY_Q,           // Q      code 51
                   MOD_SHIFT | KEY_R,           // R      code 52
                   MOD_SHIFT | KEY_S,           // S      code 53
                   MOD_SHIFT | KEY_T,           // T      code 54
                   MOD_SHIFT | KEY_U,           // U      code 55
                   MOD_SHIFT | KEY_V,           // V      code 56
                   MOD_SHIFT | KEY_W,           // W      code 57
                   MOD_SHIFT | KEY_X,           // X      code 58
                   MOD_SHIFT | KEY_Y,           // Y      code 59
                   MOD_SHIFT | KEY_Z,           // Z      code 5A
                               KEY_LEFTBRACE,   // [      code 5B
                               KEY_BACKSLASH,   // \      code 5C
                               KEY_RIGHTBRACE,  // ]      code 5D
							   KEY_F16,         // ^      code 5E
                   MOD_SHIFT | KEY_MINUS,       // _      code 5F
                               KEY_GRAVE,       // `      code 60
                               KEY_A,           // a      code 61
                               KEY_B,           // b      code 62
                               KEY_C,           // c      code 63
                               KEY_D,           // d      code 64
                               KEY_E,           // e      code 65
                               KEY_F,           // f      code 66
                               KEY_G,           // g      code 67
                               KEY_H,           // h      code 68
                               KEY_I,           // i      code 69
                               KEY_J,           // j      code 6A
                               KEY_K,           // k      code 6B
                               KEY_L,           // l      code 6C
                               KEY_M,           // m      code 6D
                               KEY_N,           // n      code 6E
                               KEY_O,           // o      code 6F
                               KEY_P,           // p      code 70
                               KEY_Q,           // q      code 71
                               KEY_R,           // r      code 72
                               KEY_S,           // s      code 73
                               KEY_T,           // t      code 74
                               KEY_U,           // u      code 75
                               KEY_V,           // v      code 76
                               KEY_W,           // w      code 77
                               KEY_X,           // x      code 78
                               KEY_Y,           // y      code 79
                               KEY_Z,           // z      code 7A
                   MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
                   MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
                   MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
                   MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                               KEY_DELETE       // DELETE code 7F
};
int accel[32] = { 0,  1,  4,  8,  9,  10,  11,  12, 13, 14, 15, 16, 17, 18, 19, 20
                 -21, -20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10, -9, -8, -4, -1}; 
volatile int stop = FALSE, isdaemon = FALSE;
struct input_event evkey, evrelx, evrely, evsync;

void sendkeycodedown(int fd, int code)
{
        /*
         * press keys
         */
        evkey.value = 1;
        if (code & MOD_ALT)
        {
                evkey.code = KEY_LEFTALT;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_CTRL)
        {
                evkey.code = KEY_LEFTCTRL;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_SHIFT)
        {
                evkey.code = KEY_LEFTSHIFT;
                write(fd, &evkey, sizeof(evkey));
        }
        evkey.code = code & KEY_CODE;
        write(fd, &evkey,  sizeof(evkey));
        write(fd, &evsync, sizeof(evsync));
}
void sendkeycodeup(int fd, int code)
{
        /*
         * release keys
         */
        evkey.code  = code & KEY_CODE;
        evkey.value = 0;
        write(fd, &evkey, sizeof(evkey));
        if (code & MOD_SHIFT)
        {
                evkey.code  = KEY_LEFTSHIFT;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_CTRL)
        {
                evkey.code  = KEY_LEFTCTRL;
                write(fd, &evkey, sizeof(evkey));
        }        
        if (code & MOD_ALT)
        {
                evkey.code  = KEY_LEFTALT;
                write(fd, &evkey, sizeof(evkey));
        }
        write(fd, &evsync, sizeof(evsync));
}
void sendkey(int fd, int mod, int key)
{
        static int prevcode = -1;
        int code = keycode[(mod & MOD_FN) | (key & KEY_ASCII)]
                  | ((mod << 8) & MOD_ALT);
        
        if (prevcode >= 0)
        {
                sendkeycodeup(fd, prevcode);
                if (!(key & KEY_PRESS) && ((code & KEY_CODE) != (prevcode & KEY_CODE)))
                        /*
                         * missed a key down event
                         * synthesize one
                         */
                        sendkeycodedown(fd, code);
                (key & KEY_PRESS) ? sendkeycodedown(fd, code) : sendkeycodeup(fd, code);
        }
        else
        {
                sendkeycodedown(fd, code);
                if (!(key & KEY_PRESS))
                        /*
                         * missed a key down event
                         * already synthesized one
                         */
                        sendkeycodeup(fd, code);
        }
        prevcode = (key & KEY_PRESS) ? code : -1;
}
void sendbttn(int fd, int mod, int bttn)
{
        static int lastbtn = 0;
        
        if (bttn)
        {
                lastbtn        =
                evkey.code  = (mod == 0)   ? BTN_LEFT 
                            : (mod & 0x40) ? BTN_RIGHT
                                           : BTN_MIDDLE;
                evkey.value = 1;
        }
        else
        {
                evkey.code  = lastbtn;
                evkey.value = 0;
        }
        write(fd, &evkey, sizeof(evkey));
        write(fd, &evsync, sizeof(evsync));
}
void sendrelxy(int fd, int x, int y)
{
		if (x > 4 || x < -4) x = x *4;
		else x = accel[x & 0x1F];
		if (y > 4 || y < -4) y = y * 4;
		else y = accel[y & 0x1F];
        evrelx.value = x;
        evrely.value = y;
        write(fd, &evrelx, sizeof(evrelx));
        write(fd, &evrely, sizeof(evrely));
        write(fd, &evsync, sizeof(evsync));
}
int writeword(int fd, int word, char ack)
{
        char rwchar;
        
        rwchar = word;  /* send low byte of word */
        write(fd, &rwchar, 1);
        if ((read(fd, &rwchar, 1) == 1) && (rwchar == ack))      /* receive ack */
        {
                rwchar = word >> 8;  /* send high byte of word */
                write(fd, &rwchar, 1);
                if ((read(fd, &rwchar, 1) == 1) && (rwchar == ack))      /* receive ack */
                        return TRUE;
        }
        return FALSE;
}
void prlog(char *str)
{
        if (!isdaemon)
                puts(str);
}
struct a2request *addreq(int a2fd, int reqfd, int type, int addr, int count, char *buffer)
{
        char rwchar;
        struct a2request *a2req = a2reqfree;
        if (a2req == NULL)
                a2req = malloc(sizeof(struct a2request));
        else
                a2reqfree = a2reqfree->next;
        a2req->fd     = reqfd;
        a2req->type   = type;
        a2req->addr   = addr;
        a2req->count  = count;
        a2req->xfer   = 0;
        a2req->buffer = buffer;
        a2req->next   = NULL;
        if (a2reqlist == NULL)
        {
                /*
                 * Initiate request.
                 */
                a2reqlist = a2req;
                rwchar    = a2req->type;
                write(a2fd, &rwchar, 1);
        }
        else
        {
                /*
                 * Add to end of request list.
                 */
                struct a2request *a2reqnext = a2reqlist;
                while (a2reqnext->next != NULL)
                        a2reqnext = a2reqnext->next;
                a2reqnext->next = a2req;
        }
        return a2req;
}
void finreq(int a2fd, int status, int result)
{
        char finbuf[2];
        struct a2request *a2req = a2reqlist;
        if (a2req->next)
        {
                /*
                 * Initiate next request.
                 */
                finbuf[0] = a2req->next->type;
                write(a2fd, finbuf, 1);
        }
        /*
         * Send result to socket.
         */
        if (a2req->fd)
        {
                if (a2req->type == 0x90) /* read bytes */
                        write(a2req->fd, a2req->buffer, a2req->count);
                finbuf[0] = status;
                finbuf[1] = result;
                write(a2req->fd, finbuf, 2);
        }
        if (a2req->buffer)
        {
                free(a2req->buffer);
                a2req->buffer = NULL;
        }
        /*
         * Update lists.
         */
        a2reqlist   = a2req->next;
        a2req->next = a2reqfree;
        a2reqfree   = a2req;
}
void main(int argc, char **argv)
{
        struct uinput_user_dev uidev;
        struct termios oldtio,newtio;
        unsigned char iopkt[16];
        int i, c, lastbtn;
        int a2fd, kbdfd, moufd, srvfd, reqfd, maxfd;
        struct sockaddr_in servaddr;
        fd_set readset, openset;
        char *devtty = "/dev/ttyAMA0"; /* default for Raspberry Pi */

        /*
         * Parse arguments
         */
        if (argc > 1)
        {
                /*
                 * Are we running as a daemon?
                 */
                if (strcmp(argv[1], "--daemon") == 0)
                {
                        pid_t pid, sid; /* our process ID and Session ID */
                
                        pid = fork();   /* fork off the parent process */
                        if (pid < 0)
                                die("a2pid: fork() failure");
                        /*
                         * If we got a good PID, then
                         * we can exit the parent process
                         */
                        if (pid > 0)
                                exit(EXIT_SUCCESS);
                        umask(0);       /* change the file mode mask */
                        /*
                         * Open any logs here
                         */
                        sid = setsid(); /* create a new SID for the child process */
                        if (sid < 0)
                                die("a2pid: setsid() failure");
                        if ((chdir("/")) < 0)   /* change the current working directory */
                                die("a2pid: chdir() failure");
                        /*
                         * Close out the standard file descriptors
                         */
                        close(STDIN_FILENO);
                        close(STDOUT_FILENO);
                        close(STDERR_FILENO);
                        isdaemon = TRUE;
                        /*
                         * Another argument must be tty device
                         */
                        if (argc > 2)
                                devtty = argv[2];
                }
                else
                        /*
                         * Must be tty device
                         */
                        devtty = argv[1];
        }
        /*
         * Create keyboard input device
         */
        prlog("a2pid: Create keyboard input device\n");
        kbdfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (kbdfd < 0)
                die("error: uinput open");
        if (ioctl(kbdfd, UI_SET_EVBIT, EV_KEY) < 0)
                die("error: uinput ioctl EV_KEY");
        if (ioctl(kbdfd, UI_SET_EVBIT, EV_REP) < 0)
                die("error: uinput ioctl EV_REP");
        for (i = KEY_ESC; i <= KEY_F10; i++)
                if (ioctl(kbdfd, UI_SET_KEYBIT, i) < 0)
                        die("error: uinput ioctl SET_KEYBITs");        
        for (i = KEY_HOME; i <= KEY_DELETE; i++)
                if (ioctl(kbdfd, UI_SET_KEYBIT, i) < 0)
                        die("error: uinput ioctl SET_KEYBITs");
        if (ioctl(kbdfd, UI_SET_EVBIT, EV_SYN) < 0)
                die("error: ioctl EV_SYN");
        bzero(&uidev, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Apple2 Pi Keyboard");
        uidev.id.bustype = BUS_RS232;
        uidev.id.vendor  = 0x05ac;      /* apple */
        uidev.id.product = 0x2e;
        uidev.id.version = 1;
        write(kbdfd, &uidev, sizeof(uidev));
        if (ioctl(kbdfd, UI_DEV_CREATE) < 0)
                die("error: ioctl DEV_CREATE");
        /*
         * Set repeat delay values that make sense.
         */
        bzero(&evkey,  sizeof(evkey));
        evkey.type  = EV_REP;
        evkey.code  = REP_DELAY;
        evkey.value = 500;      /* 0.5 sec delay */
        if (write(kbdfd, &evkey, sizeof(evkey)) < 0)
                die("error: REP_DELAY");
        evkey.type  = EV_REP;
        evkey.code  = REP_PERIOD;
        evkey.value = 67;      /* 15 reps/sec */
        if (write(kbdfd, &evkey, sizeof(evkey)) < 0)
                die("error: REP_PERIOD");
        /*
         * Create mouse input device
         */
        prlog("a2pid: Create mouse input device\n");
        moufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (moufd < 0)
                die("error: uinput open");
        if (ioctl(moufd, UI_SET_EVBIT, EV_KEY) < 0)
                die("error: uinput ioctl EV_KEY");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_LEFT) < 0)
                die("error: uinput ioctl BTN_LEFT");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_RIGHT) < 0)
                die("error: uinput ioctl BTN_RIGHT");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_MIDDLE) < 0)
                die("error: uinput ioctl BTN_MIDDLE");
        if (ioctl(moufd, UI_SET_EVBIT, EV_REL) < 0)
                die("error: ioctl EV_REL");
        if (ioctl(moufd, UI_SET_RELBIT, REL_X) < 0)
                die("error: ioctl REL_X");
        if (ioctl(moufd, UI_SET_RELBIT, REL_Y) < 0)
                die("error: ioctl REL_Y");
        if (ioctl(moufd, UI_SET_EVBIT, EV_SYN) < 0)
                die("error: ioctl EV_SYN");
        bzero(&uidev, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Apple2 Pi Mouse");
        uidev.id.bustype = BUS_RS232;
        uidev.id.vendor  = 0x05ac;      /* apple */
        uidev.id.product = 0x2e;
        uidev.id.version = 1;
        write(moufd, &uidev, sizeof(uidev));
        if (ioctl(moufd, UI_DEV_CREATE) < 0)
                die("error: ioctl DEV_CREATE");
        /*
         * Initialize event structures.
         */
        bzero(&evkey,  sizeof(evkey));
        bzero(&evsync, sizeof(evsync));
        bzero(&evrelx, sizeof(evrelx));
        bzero(&evrely, sizeof(evrely));
        evkey.type  = EV_KEY;
        evrelx.type = EV_REL;
        evrelx.code = REL_X;
        evrely.type = EV_REL;
        evrely.code = REL_Y;
        evsync.type = EV_SYN;
        /*
         * Open serial port.
         */
        prlog("a2pid: Open serial port\n");
        a2fd = open(devtty, O_RDWR | O_NOCTTY);
        if (a2fd < 0)
                die("error: serial port open");
        tcflush(a2fd, TCIFLUSH);
        tcgetattr(a2fd, &oldtio); /* save current port settings */
        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag     = BAUDRATE /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
        newtio.c_iflag     = IGNPAR;
        newtio.c_oflag     = 0;
        newtio.c_lflag     = 0; /* set input mode (non-canonical, no echo,...) */
        newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
        newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
        tcsetattr(a2fd, TCSANOW, &newtio);
        prlog("a2pid: Waiting...\n");
        iopkt[0] = 0x80;  /* request re-sync if Apple II already running */
        write(a2fd, iopkt, 1);        
        if (read(a2fd, iopkt, 1) == 1)
        {
                if (iopkt[0] == 0x80)   /* receive sync */
                {
                        prlog("a2pid: Connected.\n");
                        iopkt[0] = 0x81;  /* acknowledge */
                        write(a2fd, iopkt, 1);
                        tcflush(a2fd, TCIFLUSH);
                }
                else if (iopkt[0] == 0x9F)     /* bad request from Apple II */
                {
                        prlog("a2pi: Bad Connect Request.\n");
                        tcflush(a2fd, TCIFLUSH);
                }
                else
                {
                        prlog("a2pi: Bad Sync ACK\n");
                        stop = TRUE;
                }
        }
        newtio.c_cc[VMIN] = 3; /* blocking read until 3 chars received */
        tcsetattr(a2fd, TCSANOW, &newtio);
        /*
         * Open socket.
         */
        prlog("a2pid: Open server socket\n");
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_port        = htons(6502);
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        srvfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (srvfd < 0)
                die("error: socket create");
        if (bind(srvfd,(struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                die("error: bind socket");
        if (listen(srvfd, 1) < 0)
                die("error: listen socket");
        reqfd = 0;
        FD_ZERO(&openset);
        FD_SET(a2fd,  &openset);
        FD_SET(srvfd, &openset);
        maxfd = a2fd > srvfd ? a2fd : srvfd;
        /*
         * Event loop
         */
        prlog("a2pid: Enter event loop\n");
        while (!stop)
        {
                memcpy(&readset, &openset, sizeof(openset));
                if (select(maxfd + 1, &readset, NULL, NULL, NULL) > 0)
                {
                        /*
                         * Serial port to Apple II transaction.
                         */
                        if (FD_ISSET(a2fd, &readset))
                        {
                                if (read(a2fd, iopkt, 3) == 3)
                                {
                                        // printf("a2pi: Event [0x%02X] [0x%02X] [0x%02X]\n", iopkt[0], iopkt[1], iopkt[2]);
                                        switch (iopkt[0])
                                        {
                                                case 0x80: /* sync */
                                                        prlog("a2pid: Re-Connected.\n");
                                                        iopkt[0] = 0x81;  /* acknowledge */
                                                        write(a2fd, iopkt, 1);
                                                        tcflush(a2fd, TCIFLUSH);
                                                        break;                                
                                                case 0x82: /* keyboard event */
                                                        // printf("Keyboard Event: 0x%02X:%c\n", iopkt[1], iopkt[2] & 0x7F);
                                                        sendkey(kbdfd, iopkt[1], iopkt[2]);
                                                        if (iopkt[2] == 0x9B && iopkt[1] == 0xC0)
                                                                stop = TRUE;
                                                        break;
                                                case 0x84: /* mouse move event */
                                                        // printf("Mouse XY Event: %d,%d\n", (signed char)iopkt[1], (signed char)iopkt[2]);
                                                        sendrelxy(moufd, (signed char)iopkt[1], (signed char)iopkt[2]);
                                                        break;
                                                case 0x86: /* mouse button event */
                                                        // printf("Mouse Button %s Event 0x%02X\n", iopkt[2] ? "[PRESS]" : "[RELEASE]", iopkt[1]);
                                                        sendbttn(moufd, iopkt[1], iopkt[2]);
                                                        break;
                                                case 0x90: /* acknowledge read bytes request*/
                                                        if (a2reqlist) /* better have an outstanding request */
                                                        {
                                                                //printf("a2pid: read %d of %d bytes from 0x%04X\n", a2reqlist->xfer, a2reqlist->count, a2reqlist->addr);
                                                                newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                                c = a2reqlist->count - a2reqlist->xfer > MAX_XFER
                                                                  ? MAX_XFER
                                                                  : a2reqlist->count - a2reqlist->xfer;
                                                                if (writeword(a2fd, a2reqlist->addr + a2reqlist->xfer, 0x91) && writeword(a2fd, c, 0x91))
                                                                {
                                                                        for (i = 0; i < c; i++)
                                                                        {
                                                                                if (read(a2fd, iopkt, 1) == 1)
                                                                                        a2reqlist->buffer[a2reqlist->xfer++] = iopkt[0];
                                                                                else
                                                                                {
                                                                                        stop = TRUE;
                                                                                        break;
                                                                                }
                                                                        }
                                                                }
                                                                else
                                                                        stop = TRUE;
                                                                newtio.c_cc[VMIN]  = 3; /* blocking read until 3 chars received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                        }
                                                        else
                                                                stop = TRUE;
                                                        break;
                                                case 0x92: /* acknowledge write bytes */
                                                        if (a2reqlist) /* better have an outstanding request */
                                                        {
                                                                //printf("a2pid: wrote %d of %d bytes to 0x%04X\n", a2reqlist->xfer, a2reqlist->count, a2reqlist->addr);
                                                                newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                                c = a2reqlist->count - a2reqlist->xfer > MAX_XFER
                                                                  ? MAX_XFER
                                                                  : a2reqlist->count - a2reqlist->xfer;
                                                                if (writeword(a2fd, a2reqlist->addr + a2reqlist->xfer, 0x93) && writeword(a2fd, c, 0x93))
                                                                {
                                                                        if (write(a2fd, a2reqlist->buffer + a2reqlist->xfer, c) == c)
                                                                                a2reqlist->xfer += c;
                                                                        else
                                                                                stop = TRUE;
                                                                }
                                                                else
                                                                        stop = TRUE;
                                                                newtio.c_cc[VMIN]  = 3; /* blocking read until 3 chars received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                        }
                                                        else
                                                                stop = TRUE;
                                                        break;
                                                case 0x94: /* acknowledge call */
                                                        if (a2reqlist) /* better have an outstanding request */
                                                        {
                                                                //printf("a2pid: call address 0x%04X\n", a2reqlist->addr);
                                                                newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                                if (!writeword(a2fd, a2reqlist->addr, 0x95))
                                                                        stop = TRUE;
                                                                newtio.c_cc[VMIN]  = 3; /* blocking read until 3 chars received */
                                                                tcsetattr(a2fd, TCSANOW, &newtio);
                                                        }
                                                        else
                                                                stop = TRUE;
                                                        break;
                                                case 0x9E: /* request complete ok */
                                                case 0x9F: /* request complete error */
                                                        if (a2reqlist) /* better have an outstanding request */
                                                        {
                                                                //printf("a2pid: complete request 0x%02X:0x%02X\n", (unsigned char)iopkt[0], (unsigned char)iopkt[1]);
                                                                if ((a2reqlist->type == 0x90 || a2reqlist->type == 0x92)
                                                                 && (a2reqlist->count > a2reqlist->xfer))
                                                                {
                                                                        iopkt[0] = a2reqlist->type;
                                                                        write(a2fd, iopkt, 1);
                                                                }
                                                                else
                                                                {
                                                                        //printf("a2pid: finish request 0x%02X:0x%02X\n", (unsigned char)iopkt[0], (unsigned char)iopkt[1]);
                                                                        finreq(a2fd, (unsigned char)iopkt[0], (unsigned char)iopkt[1]);
                                                                }
                                                        }
                                                        else
                                                                stop = TRUE;
                                                        break;
                                                default:
                                                        prlog("a2pid: Unknown Event\n");
                                                        tcflush(a2fd, TCIFLUSH);
                                                        //stop = TRUE;
                                        }
                                }
                                else
                                {
                                        prlog("a2pid: error read serial port ????\n");
                                        stop = TRUE;
                                }
                        }
                        /*
                         * Socket server connection.
                         */
                        if (FD_ISSET(srvfd, &readset))
                        {
                                int len;
                                struct sockaddr inaddr;
                                len   = sizeof(inaddr);
                                reqfd = accept(srvfd, NULL, NULL);
                                if (reqfd > 0)
                                        FD_SET(reqfd, &openset);
                                else
                                        prlog("a2pid: error accept");
                                maxfd = reqfd > maxfd ? reqfd : maxfd;
                                prlog("a2pi: Client Connect\n");
                        }
                        /*
                         * Socket client request.
                         */
                        if (reqfd > 0 && FD_ISSET(reqfd, &readset))
                        {
                                int addr, count;
                                char *databuf;
                                if (read(reqfd, iopkt, 1) == 1)
                                {
                                        // printf("a2pi: Request [0x%02X]\n", iopkt[0]);
                                        switch (iopkt[0])
                                        {
                                                case 0x90: /* read bytes */
                                                        if (read(reqfd, iopkt, 4) == 4)
                                                        {
                                                                addr  = (unsigned char)iopkt[0] | ((unsigned char)iopkt[1] << 8);
                                                                count = (unsigned char)iopkt[2] | ((unsigned char)iopkt[3] << 8);
                                                                if (count)
                                                                {
                                                                        databuf = malloc(count);
                                                                        addreq(a2fd, reqfd, 0x90, addr, count, databuf);
                                                                }
                                                                else
                                                                {
                                                                        iopkt[0] = 0x9E;
                                                                        iopkt[1] = 0x00;
                                                                        write(reqfd, iopkt, 2);
                                                                }
                                                        }
                                                        break;
                                                case 0x92: /* write bytes */
                                                        if (read(reqfd, iopkt, 4) == 4)
                                                        {
                                                                addr  = (unsigned char)iopkt[0] | ((unsigned char)iopkt[1] << 8);
                                                                count = (unsigned char)iopkt[2] | ((unsigned char)iopkt[3] << 8);
                                                                if (count)
                                                                {
                                                                        databuf = malloc(count);
                                                                        if (read(reqfd, databuf, count) == count)
                                                                                addreq(a2fd, reqfd, 0x92, addr, count, databuf);
                                                                }
                                                                else
                                                                {
                                                                        iopkt[0] = 0x9E;
                                                                        iopkt[1] = 0x00;
                                                                        write(reqfd, iopkt, 2);
                                                                }
                                                        }
                                                        break;
                                                case 0x94: /* call */
                                                        if (read(reqfd, iopkt, 2) == 2)
                                                        {
                                                                addr  = (unsigned char)iopkt[0] | ((unsigned char)iopkt[1] << 8);
                                                                addreq(a2fd, reqfd, 0x94, addr, 0, NULL);
                                                        }
                                                        break;
                                                case 0xFF: /* close */
                                                        FD_CLR(reqfd, &openset);
                                                        close(reqfd);
                                                        reqfd = 0;
                                                        maxfd = a2fd > srvfd ? a2fd : srvfd;
                                                        break;
                                                default:
                                                        prlog("a2pid: Unknown Request\n");
                                                        stop = TRUE;
                                        }
                                }
                                else
                                {
                                        prlog("a2pid: error read socket ????");
                                        stop = TRUE;
                                }
                        }
                }
        }
        if (reqfd > 0)
                close(reqfd);
        shutdown(srvfd, SHUT_RDWR);
        close(srvfd);
        tcsetattr(a2fd, TCSANOW, &oldtio);
        tcflush(a2fd, TCIFLUSH);
        close(a2fd);
        ioctl(moufd, UI_DEV_DESTROY);
        ioctl(kbdfd, UI_DEV_DESTROY);
        close(moufd);
        close(kbdfd);
}
