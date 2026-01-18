#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define PORT "22"
#define RATE "300000"
#define OUTFILE "bios.txt"

#define GREEN  "\033[92m"
#define YELLOW "\033[93m"
#define WHITE  "\033[97m"
#define RESET  "\033[0m"

pid_t progress_pid = 0;

void progress_loop(int x) {
    for (int a = 0; a <= 255; a++) {
        for (int b = 0; b <= 255; b++) {
            printf(GREEN "[Portscan] - " WHITE "%d.%d.%d.*" RESET "\n", x, a, b);
            fflush(stdout);
            usleep(4000);
        }
    }
}

void stop_progress() {
    if (progress_pid > 0) {
        kill(progress_pid, SIGKILL);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <primer_octeto>\n", argv[0]);
        return 1;
    }

    int x = atoi(argv[1]);
    if (x < 0 || x > 255) {
        printf("Octeto inválido\n");
        return 1;
    }

    FILE *out = fopen(OUTFILE, "w");
    if (!out) {
        perror("bios.txt");
        return 1;
    }

    signal(SIGINT, stop_progress);

    progress_pid = fork();
    if (progress_pid == 0) {
        progress_loop(x);
        exit(0);
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "masscan %d.0.0.0/8 -p%s --rate %s -oL -",
        x, PORT, RATE
    );

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("masscan");
        return 1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;

        char proto[8];
        int port;
        char status[16];
        char ip[64];

        if (sscanf(line, "%7s %d %15s %63s", proto, &port, status, ip) == 4) {
            printf(YELLOW "%s" RESET "\n", ip);
            fprintf(out, "%s\n", ip);
            fflush(out);
        }
    }

    pclose(fp);
    fclose(out);
    stop_progress();

    printf(GREEN "[✔] Scan finalizado\n" RESET);
    printf(GREEN "[✔] IPs guardadas en " WHITE "%s\n" RESET, OUTFILE);

    return 0;
}
