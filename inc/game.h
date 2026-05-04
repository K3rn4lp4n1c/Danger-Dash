#ifndef GAME_H
#define GAME_H

#define _XOPEN_SOURCE_EXTENDED 1
#define MA_NO_NULL

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <locale.h>
#include <wchar.h>
#include <ncursesw/curses.h>

#if defined(DISABLE_AUDIO)
typedef struct { int dummy; } Audio;
typedef struct { int dummy; } ma_decoder;
typedef int ma_result;
#define MA_SUCCESS 0
static inline int __audio_init__(Audio *audio, const char *music_path) { return 0; }
static inline void __audio_play_sfx__(Audio *audio, const char *path) {}
static inline void __audio_start_music__(Audio *audio) {}
static inline void __audio_stop_music__(Audio *audio) {}
static inline void __audio_shutdown__(Audio *audio) {}
static inline ma_result ma_decoder_init_file(const char *filename, void *config, ma_decoder *decoder) { ma_result a; return a; }
static inline void ma_decoder_uninit(ma_decoder *decoder) {}
static inline char* ma_result_description(ma_result result) { return "Audio disabled"; }
#else
#include "miniaudio.h"
typedef struct {
    ma_engine engine;
    ma_sound music;
    int music_loaded;
} Audio;

int __audio_init__(Audio *audio, const char *music_path);
void __audio_play_sfx__(Audio *audio, const char *path);
void __audio_start_music__(Audio *audio);
void __audio_stop_music__(Audio *audio);
void __audio_shutdown__(Audio *audio);
#endif

extern const char GAME_TITLE[];
extern const char GAME_VERSION[];
extern const char *OBSTACLES[4];
extern const char *MUSIC[4];
extern const char *SOUND_EFFECTS[4];
extern const char RANKING_FILE[];

#define MAX_PLAYERS 4
#define MAX_PREDECESSORS 5
#define MAX_NAME_LENGTH 20
#define INITIAL_FRAME_RATE 50000 // ~20 FPS

#ifndef NASM_FUNCTIONS
#define NASM_FUNCTIONS
int check_for_collision(int, int);
void move_player(int *, int, int, int, int, int);
#endif

typedef enum { Benjamin, Ethan, Muhammad, Youssef, } Characters;
typedef enum { INACTIVE, ACTIVE, IDLE, BUSY, } States;

const double OBSTACLE_ODDS = 0.01; // 1% chance of new obstacle each frame

typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
    Characters character;
} Record;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int x, y;
    Characters character;
    States state;
    pthread_mutex_t lock;
    pthread_t thread;
} Player;

typedef struct {
    Player *player;
    int key;
    unsigned long frame_rate;
} Input;

typedef struct {
    WINDOW *wstatus;
    WINDOW *wgame;
    WINDOW *winfo;
    char **map;
    unsigned long frame_rate;
} Environment;

/* n00b5_d0_n0t_t0uch! Delicate memory layout of struct used in `game.asm` */
typedef struct {
    int player_count;
    int score;
    Environment *env;
    States state;
    pthread_t input;
    pthread_mutex_t lock;
    Player *players[MAX_PLAYERS];
    Audio audio;
    Record *predecessors[MAX_PREDECESSORS];
} Game;

Game* init(int, char **, Characters *);

void helloWorld(), showConfig();
void update(Game *), run(Game *), end(Game *), deinit(Game *);

void __refresh_all_windows__(Game *), __initialize_curses__();
void __initial_screen__(Game *, int, int), __adjust_map__(Game *, int, int);
void __erase_all_windows__(Environment *);
void* __keypress__(void *), *__player_effect__(void *);

const wchar_t* __resolve_character__(Characters*);
#endif