#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_LENGTH 128
#define POS_LENGTH 2
#define X_POS_ID "0035"
#define Y_POS_ID "0036"

#define LANDSCAPE 1     /* 横屏模式 */ 
#if LANDSCAPE == 1
    #define SCREEN_HEIGHT 1080
#endif

#define DEBUG
#ifdef DEBUG
    #define dbg_printf(fmt, ...) printf(fmt, __VA_ARGS__)
#else
    #define dbg_printf(fmt, ...) 
#endif

enum {
    READ_MODE,
    WRITE_MODE,
};

typedef struct {
    char *type;
    int value;
    float pts;
} pos_stamp;

pos_stamp g_pos[2] = {
    {X_POS_ID, -1, 0},
    {Y_POS_ID, -1, 0}
};

int check_system_environment(void)
{
    int ret;

    ret = system("adb.exe version");
    if (ret != 0) {
       printf("Please install adb\n");
        return -1;
    }

    printf("Please connect the phone and turn on the USB debugging mode\n");
    system("adb.exe devices");

    return 0;
}

void recode_tap_coordinates(char *filename)
{
    char adb_cmd[BUFFER_LENGTH] = {0};
    int cnt = 1;

    printf("Recode tap coordinates to %s\n", filename);
    printf("Please tap the phone screen\n");
    printf("Press Ctrl+c to exit\n");

    sprintf(adb_cmd, "rm %s", filename);
    system(adb_cmd);
    memset(adb_cmd, 0, BUFFER_LENGTH);

    sprintf(adb_cmd, "adb.exe shell getevent -c 14 -t | grep -e \"0035\" -e \"0036\" >> %s", filename);
    dbg_printf("%s\n", adb_cmd);

    while (1) {
        system(adb_cmd);
        printf("recode coordinates %d\n", cnt++);
    }
}

int get_position_info(char *line)
{
    char *type;
    char *num;
    char *time;
    int i;

    line[16] = '\0';
    time = &line[1];
    line[46] = '\0';
    type = &line[42];
    line[55] = '\0';
    num = &line[47];

    for (i = 0; i < POS_LENGTH; i++) {
        if (!strcmp(type, g_pos[i].type)) {
            break;
        }
    }

    if (i == POS_LENGTH) {
        printf("error coordinates type\n");
        return -1;
    }

    g_pos[i].value = strtol(num, NULL, 16);
    g_pos[i].pts = strtof(time, NULL);

    return 0;
}

#define FIND_PAIRED_POSITION (g_pos[0].value != -1 && g_pos[1].value != -1 && g_pos[0].pts == g_pos[1].pts)

int simulate_screen_tap(char *filename)
{
    FILE *pos_file;
    char line[BUFFER_LENGTH] = {0};
    char adb_cmd[100] = {0};
    float last_time = 0;

    pos_file = fopen(filename, "r");
    if (pos_file == NULL) {
        perror("fopen failed");
        return errno;
    }

    while(fgets(line, BUFFER_LENGTH, pos_file) != NULL) {
        long long interval;

        if (get_position_info(line) < 0) {
            printf("get position failed\n");
            return -1;
        }
        
        if (!FIND_PAIRED_POSITION) {
            memset(line, 0, BUFFER_LENGTH);
            continue;
        }

        if (last_time != 0) {
            interval = (g_pos[0].pts - last_time) * 1000000;
            printf("sleep %lld microseconds\n", interval);
            usleep(interval);
        }

#if LANDSCAPE == 1
        sprintf(adb_cmd, "adb.exe shell input tap %d %d", g_pos[1].value, SCREEN_HEIGHT - g_pos[0].value);
#else
        sprintf(adb_cmd, "adb.exe shell input tap %d %d", g_pos[0].value, g_pos[1].value);
#endif
        dbg_printf("%d %d\n", g_pos[0].value, g_pos[1].value);
        dbg_printf("%s\n", adb_cmd);

        system(adb_cmd);
        g_pos[0].value = -1;
        g_pos[1].value = -1;

        last_time = g_pos[0].pts;
        memset(line, 0, BUFFER_LENGTH);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int times;
    int mode;
    char *filename;
    int ret = 0;

    if (argc == 2) {
        mode = READ_MODE;
        filename = argv[1];
        times = 1;
    } else if (argc == 3 && !strcmp(argv[2], "-r")) {
        mode = WRITE_MODE;
        filename = argv[1];
    } else if (argc == 4 && !strcmp(argv[2], "-t")) {
        mode = READ_MODE;
        filename = argv[1];
        times = atoi(argv[3]);
    } else {
        printf("Usage:\n");
        printf("%s <filename> [option]...\n", argv[0]);
        printf("    -r: recode\n");
        printf("    -t <times>: repeat <times>\n");
        return -1;
    }

    ret = check_system_environment();
    if (ret != 0) {
        printf("Check system environment failed\n");
        return ret;
    }

#if LANDSCAPE == 1
    printf("Landscape mode: please rotate your phone 90 degrees COUNTERCLOCKWISE\n");
#endif

    if (mode == WRITE_MODE) {
        recode_tap_coordinates(filename);
    } else if (mode == READ_MODE) {
        for (int i = 0; i < times; i++) {
            ret = simulate_screen_tap(filename);
            if (ret < 0) {
                printf("simulate_screen_tap error\n");
                break;
            }
            sleep(1);
        }
    } else {
        printf("error mode\n");
        ret = -1;
    }

    return ret;
}