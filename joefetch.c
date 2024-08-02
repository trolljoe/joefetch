#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#define BUFFER_SIZE 1024
#define ASCII_WIDTH 40
#define MAX_OUTPUT_LENGTH 4096
#define INFO_WIDTH 50
#define TOTAL_HEIGHT 25

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

void init_static_info() {
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp) {
        while (fgets(os_name, sizeof(os_name), fp)) {
            if (strncmp(os_name, "PRETTY_NAME=", 12) == 0) {
                strcpy(os_name, strchr(os_name, '=') + 1);
                os_name[strcspn(os_name, "\"")] = '\0';
                break;
            }
        }
        fclose(fp);
    }

    struct utsname uname_data;
    if (uname(&uname_data) == 0) {
        strcpy(kernel_version, uname_data.release);
    }

    gethostname(hostname, sizeof(hostname));
    strcpy(shell, getenv("SHELL") ? getenv("SHELL") : "Unknown");
}

void append_to_output(const char *label, const char *data) {
    pthread_mutex_lock(&buffer_mutex);
    int required_space = snprintf(NULL, 0, "%-*s%s\n", INFO_WIDTH, label, data) + 1;
    if (offset + required_space < MAX_OUTPUT_LENGTH) {
        offset += snprintf(output_buffer + offset, MAX_OUTPUT_LENGTH - offset, "%-*s%s\n", INFO_WIDTH, label, data);
    }
    pthread_mutex_unlock(&buffer_mutex);
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
    append_to_output("OS", os_name);
    append_to_output("Kernel", kernel_version);
    append_to_output("Hostname", hostname);
    append_to_output("Shell", shell);
}

void prepare_info_lines(char *info_lines[], int *num_lines) {
    memset(info_lines, 0, sizeof(char *) * TOTAL_HEIGHT);
    char *line = strtok(output_buffer, "\n");
    int index = 0;
    while (line && index < TOTAL_HEIGHT) {
        info_lines[index++] = line;
        line = strtok(NULL, "\n");
    }
    *num_lines = index;
}

void print_ascii_and_info(const char *ascii_logo[], int num_ascii_lines, char *info_lines[], int num_info_lines) {
    int max_lines = num_ascii_lines > num_info_lines ? num_ascii_lines : num_info_lines;
    int info_start_line = (TOTAL_HEIGHT - max_lines) / 2 + num_ascii_lines;

    for (int i = 0; i < num_ascii_lines; i++) {
        printf("%s\n", ascii_logo[i]);
    }

    for (int i = num_ascii_lines; i < info_start_line; i++) {
        printf("\n");
    }

    for (int i = 0; i < max_lines; i++) {
        printf("%-*s\n", INFO_WIDTH, i < num_info_lines ? info_lines[i] : "");
    }
}

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    init_static_info();

    const Command commands[] = {
        {"Uptime", "awk '{print int($1/3600) \" hours, \" int(($1%3600)/60) \" minutes\"}' /proc/uptime"},
        {"Memory", "grep 'MemTotal:' /proc/meminfo | awk '{print $2/1024 \" MB\"}'"},
        {"Used Memory", "grep 'MemAvailable:' /proc/meminfo | awk '{printf \"%.2f MB\", $2/1024}'"},
        {"CPU", "grep 'model name' /proc/cpuinfo | head -1 | cut -d ':' -f 2- | sed 's/^ //'"}, 
        {"CPU Cores", "grep -c '^processor' /proc/cpuinfo"},
        {"Disk", "df -h / | tail -1 | awk '{print $3 \"/\" $2 \" used (\" $5 \")\"}'"},
        {"GPU", "lspci | grep -i 'vga\\|3d\\|2d' | cut -d ':' -f 3 | sed 's/^ //'"}, 
        {"Resolution", "xdpyinfo | grep dimensions | awk '{print $2}'"},
        {"Processes", "ps ax | wc -l | awk '{print $1 \" running\"}'"},
        {"IP Address", "ip addr show | grep 'inet ' | awk '{print $2}' | head -1 | cut -d '/' -f 1"},
        {"MAC Address", "ip link show | grep link/ether | awk '{print $2}'"}
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

    const char *ascii_logo[] = {
        "    .--.",
        "   |o_o |",
        "   |:_/ |",
        "  //   \\ \\",
        " (|     | )",
        "/'\\_   _/`\\",
        "\\___)=(___/"
    };
    int num_ascii_lines = sizeof(ascii_logo) / sizeof(ascii_logo[0]);

    char *info_lines[TOTAL_HEIGHT] = {NULL};
    int num_info_lines = 0;
    prepare_info_lines(info_lines, &num_info_lines);

    print_ascii_and_info(ascii_logo, num_ascii_lines, info_lines, num_info_lines);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
    printf("\nExecution Time: %.6f seconds\n", time_spent);

    return 0;
}
