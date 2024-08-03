#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include "config.h"

#define NUM_ICONS 10
#define BUFFER_SIZE 1024
#define ASCII_WIDTH 40
#define MAX_OUTPUT_LENGTH 4096

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char *label;
    char *command;
} Command;

char output_buffer[MAX_OUTPUT_LENGTH];
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
int offset = 0;
char os_name[BUFFER_SIZE];
char kernel_version[BUFFER_SIZE];
char hostname[BUFFER_SIZE];
char shell[BUFFER_SIZE];

char *get_executable_path(char *buffer, size_t bufsize) {
    ssize_t len = readlink("/proc/self/exe", buffer, bufsize - 1);
    if (len == -1) {
        perror("readlink");
        return NULL;
    }
    buffer[len] = '\0';
    return buffer;
}

const char **read_ascii_art(const char *file_path, int *num_lines) {
    char exe_path[PATH_MAX];
    if (!get_executable_path(exe_path, sizeof(exe_path))) {
        perror("Failed to get executable path");
        return NULL;
    }

    char ascii_file_path[PATH_MAX];
    snprintf(ascii_file_path, sizeof(ascii_file_path), "%s/%s", dirname(exe_path), file_path);

    FILE *file = fopen(ascii_file_path, "r");
    if (!file) {
        perror("fopen");
        return NULL;
    }

    const char **lines = malloc(TOTAL_HEIGHT * sizeof(char *));
    if (!lines) {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    char line[BUFFER_SIZE];
    int count = 0;
    while (fgets(line, sizeof(line), file) && count < TOTAL_HEIGHT) {
        line[strcspn(line, "\n")] = '\0';
        lines[count] = strdup(line);
        if (!lines[count]) {
            perror("strdup");
            for (int i = 0; i < count; i++) free((void *)lines[i]);
            free(lines);
            fclose(file);
            return NULL;
        }
        count++;
    }

    fclose(file);
    *num_lines = count;
    return lines;
}

void init_static_info() {
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp) {
        while (fgets(os_name, sizeof(os_name), fp)) {
            if (strncmp(os_name, "PRETTY_NAME=", 12) == 0) {
                char *start = strchr(os_name, '=') + 1;
                if (start[0] == '"') start++;
                char *end = strchr(start, '"');
                if (end) *end = '\0';
                strcpy(os_name, start);
                break;
            }
        }
        fclose(fp);
    } else {
        perror("Failed to open /etc/os-release");
        strcpy(os_name, "Unknown");
    }

    struct utsname uname_data;
    if (uname(&uname_data) == 0) {
        strcpy(kernel_version, uname_data.release);
    } else {
        perror("uname failed");
        strcpy(kernel_version, "Unknown");
    }

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname failed");
        strcpy(hostname, "Unknown");
    }

    const char *shell_env = getenv("SHELL");
    strcpy(shell, shell_env ? shell_env : "Unknown");
}

void append_to_output(const char *label, const char *data) {
    int required_space = snprintf(NULL, 0, "%-*s%s\n", INFO_WIDTH, label, data) + 1;
    if (offset + required_space < MAX_OUTPUT_LENGTH) {
        pthread_mutex_lock(&buffer_mutex);
        offset += snprintf(output_buffer + offset, MAX_OUTPUT_LENGTH - offset, "%-*s%s\n", INFO_WIDTH, label, data);
        pthread_mutex_unlock(&buffer_mutex);
    }
}

void *fetch_and_append(void *arg) {
    Command *cmd = (Command *)arg;
    char buffer[BUFFER_SIZE];
    FILE *fp = popen(cmd->command, "r");
    if (!fp) {
        perror("popen");
        pthread_exit(NULL);
    }

    if (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        append_to_output(cmd->label, buffer);
    }

    pclose(fp);
    pthread_exit(NULL);
}

void append_static_info() {
    append_to_output(" OS", os_name);
    append_to_output(" Kernel", kernel_version);
    append_to_output(" Hostname", hostname);
    append_to_output(" Shell", shell);
}

void prepare_info_lines(const char *info_lines[], int *num_lines) {
    char *line = strtok(output_buffer, "\n");
    int index = 0;
    while (line && index < TOTAL_HEIGHT) {
        info_lines[index++] = line;
        line = strtok(NULL, "\n");
    }
    *num_lines = index;
}

void print_ascii_and_info(const char *info_lines[], int num_info_lines) {
    const char *colors[] = {RED_COLOR, GREEN_COLOR, YELLOW_COLOR, BLUE_COLOR, MAGENTA_COLOR, CYAN_COLOR, WHITE_COLOR};
    const char *icons[NUM_ICONS] = {
        "", "", "", "", "", "", "", "", "", ""
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    int ascii_logo_lines;
    const char **ascii_logo = read_ascii_art(ASCII_FILE_PATH, &ascii_logo_lines);

    if (!ascii_logo) {
        printf("Error reading ASCII art from %s\n", ASCII_FILE_PATH);
        return;
    }

    for (int i = 0; i < ascii_logo_lines; i++) {
        printf("%s%s\n", GREEN_COLOR, ascii_logo[i]);
        free((void *)ascii_logo[i]);
    }
    free(ascii_logo);

    int max_lines = ascii_logo_lines > num_info_lines ? ascii_logo_lines : num_info_lines;
    int info_start_line = (TOTAL_HEIGHT - max_lines) / 2 + ascii_logo_lines;

    for (int i = ascii_logo_lines; i < info_start_line; i++) {
        printf("\n");
    }

    int color_index = 0;
    for (int i = 0; i < max_lines; i++) {
        if (i < num_info_lines) {
            int icon_found = 0;
            for (int j = 0; j < NUM_ICONS; j++) {
                if (strstr(info_lines[i], icons[j])) {
                    printf("%s%-*s%s\n", colors[color_index % num_colors], INFO_WIDTH, info_lines[i], RESET_COLOR);
                    color_index++;
                    icon_found = 1;
                    break;
                }
            }
            if (!icon_found) {
                printf("%s%-*s%s\n", GREEN_COLOR, INFO_WIDTH, info_lines[i], RESET_COLOR);
            }
        } else {
            printf("%-*s\n", INFO_WIDTH, "");
        }
    }
}

int main() {
    clock_t start, end;
    double time_spent;

    start = clock();

    init_static_info();

    const Command commands[] = {
        {" Uptime", "awk '{print int($1/3600) \" hours, \" int(($1%3600)/60) \" minutes\"}' /proc/uptime"},
        {" Memory", "grep 'MemTotal:' /proc/meminfo | awk '{print $2/1024 \" MB\"}'"},
        {" Used Memory", "grep 'MemAvailable:' /proc/meminfo | awk '{printf \"%.2f MB\", $2/1024}'"},
        {" CPU", "grep 'model name' /proc/cpuinfo | head -1 | cut -d ':' -f 2- | sed 's/^ //'"}, 
        {" CPU Cores", "grep -c '^processor' /proc/cpuinfo"},
        {" Disk", "df -h / | tail -1 | awk '{print $3 \"/\" $2 \" used (\" $5 \")\"}'"},
        {" GPU", "lspci | grep -i 'vga\\|3d\\|2d' | cut -d ':' -f 3 | sed 's/^ //'"}, 
        {" Resolution", "xdpyinfo | grep dimensions | awk '{print $2}'"},
        {" Processes", "ps ax | wc -l | awk '{print $1 \" running\"}'"}
    };

    int num_commands = sizeof(commands) / sizeof(commands[0]);
    pthread_t threads[num_commands];
    for (int i = 0; i < num_commands; ++i) {
        pthread_create(&threads[i], NULL, fetch_and_append, (void *)&commands[i]);
    }

    append_static_info();
    for (int i = 0; i < num_commands; ++i) {
        pthread_join(threads[i], NULL);
    }

    const char *info_lines[TOTAL_HEIGHT] = {NULL};
    int num_info_lines = 0;
    prepare_info_lines(info_lines, &num_info_lines);
    print_ascii_and_info(info_lines, num_info_lines);

    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nExecution Time: %.6f seconds\n", time_spent);

    return 0;
}
