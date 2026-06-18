/* context_behavior.c
 * Example C implementation that mirrors the conceptual functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

#define LOG_PATH_DEFAULT "context_behavior.log"

static FILE *g_logfile = NULL;

int LoggingWrapper_init(const char *path) {
    if (!path) path = LOG_PATH_DEFAULT;
    g_logfile = fopen(path, "a");
    if (!g_logfile) return -1;
    return 0;
}

void LoggingWrapper_close(void) {
    if (g_logfile) {
        fclose(g_logfile);
        g_logfile = NULL;
    }
}

void LoggingWrapper_log(const char *level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (g_logfile) {
        fprintf(g_logfile, "%s: ", level);
        vfprintf(g_logfile, fmt, ap);
        fprintf(g_logfile, "\n");
        fflush(g_logfile);
    } else {
        fprintf(stderr, "%s: ", level);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
    }
    va_end(ap);
}

static volatile sig_atomic_t g_last_signal = 0;

void CustomExceptionHandler_signal(int signum) {
    g_last_signal = signum;
    LoggingWrapper_log("ERROR", "Caught signal %d", signum);
    exit(128 + signum);
}

int FileValidationLibrary_is_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

int FileValidationLibrary_size_within(const char *path, size_t max_bytes) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (size_t)st.st_size <= max_bytes;
}

int FileValidationLibrary_has_allowed_extension(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;
    if (strcasecmp(ext, ".txt") == 0) return 1;
    if (strcasecmp(ext, ".md") == 0) return 1;
    if (strcasecmp(ext, ".csv") == 0) return 1;
    return 0;
}

typedef struct ContextBehavior {
    char *tmp_path;
    int owns_tmp;
} ContextBehavior;

ContextBehavior *ContextBehavior_create(const char *tmp_path) {
    ContextBehavior *ctx = (ContextBehavior *)calloc(1, sizeof(ContextBehavior));
    if (!ctx) return NULL;
    ctx->tmp_path = strdup(tmp_path ? tmp_path : "/tmp/context_behavior_c");
    ctx->owns_tmp = 1;

    if (mkdir(ctx->tmp_path, 0700) != 0 && errno != EEXIST) {
        LoggingWrapper_log("WARN", "Could not create tmp_path %s", ctx->tmp_path);
    }

    LoggingWrapper_log("INFO", "ContextBehavior created with tmp_path=%s", ctx->tmp_path);
    return ctx;
}

void ContextBehavior_destroy(ContextBehavior *ctx) {
    if (!ctx) return;
    LoggingWrapper_log("INFO", "Destroying ContextBehavior for tmp_path=%s", ctx->tmp_path);
    if (ctx->owns_tmp && ctx->tmp_path) free(ctx->tmp_path);
    free(ctx);
}

int main(int argc, char **argv) {
    LoggingWrapper_init(LOG_PATH_DEFAULT);

    signal(SIGINT, CustomExceptionHandler_signal);
    signal(SIGTERM, CustomExceptionHandler_signal);

    ContextBehavior *ctx = ContextBehavior_create(NULL);
    if (!ctx) {
        LoggingWrapper_log("ERROR", "Failed to create ContextBehavior");
        return 1;
    }

    if (argc > 1) {
        const char *candidate = argv[1];
        LoggingWrapper_log("INFO", "Validating candidate file: %s", candidate);
        if (!FileValidationLibrary_is_regular_file(candidate)) {
            LoggingWrapper_log("ERROR", "Not a regular file: %s", candidate);
        } else if (!FileValidationLibrary_has_allowed_extension(candidate)) {
            LoggingWrapper_log("ERROR", "Disallowed extension: %s", candidate);
        } else if (!FileValidationLibrary_size_within(candidate, 10 * 1024 * 1024)) {
            LoggingWrapper_log("ERROR", "File too large: %s", candidate);
        } else {
            LoggingWrapper_log("INFO", "File appears valid: %s", candidate);
        }
    } else {
        LoggingWrapper_log("INFO", "No file provided to validate.");
    }

    ContextBehavior_destroy(ctx);
    LoggingWrapper_close();
    return 0;
}
