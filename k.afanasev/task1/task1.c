#define __EXTENSIONS__ 1
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define MAX_OPTIONS 100
#define MAX_ARGUMENT 1024
#define MAX_NUMBER 32
#define MAX_NAME 63
#define MAX_VALUE 255
#define MAX_ENV 100
#define MAX_TEXT_OUTPUT 65536

struct command { int option; const char *argument; long number; };
extern char **environ;
static size_t output_left = MAX_TEXT_OUTPUT;


static int valid_text(const char *text, size_t maximum)
{
    size_t i = 0;
    while (text[i]) {
        unsigned char c = (unsigned char)text[i];
        if (i >= maximum) return 0;
        if (c >= 32 && c <= 126) { i++; continue; }
        if (c >= 0xD0 && c <= 0xD3 && i + 1 < maximum &&
            (unsigned char)text[i + 1] >= 0x80 &&
            (unsigned char)text[i + 1] <= 0xBF) { i += 2; continue; }
        return 0;
    }
    return 1;
}

static int parse_number(const char *text, long *number)
{
    size_t i;
    char *end;
    if (!text[0]) return -1;
    for (i = 0; text[i]; i++)
        if (i >= MAX_NUMBER || text[i] < '0' || text[i] > '9') return -1;
    errno = 0;
    *number = strtol(text, &end, 10);
    if (errno == ERANGE || *end || *number < 0 ||
        (long)(rlim_t)*number != *number || (rlim_t)*number == RLIM_INFINITY)
        return -1;
#ifdef RLIM_SAVED_CUR
    if ((rlim_t)*number == RLIM_SAVED_CUR) return -1;
#endif
#ifdef RLIM_SAVED_MAX
    if ((rlim_t)*number == RLIM_SAVED_MAX) return -1;
#endif
    return 0;
}

static int name_letter(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int valid_assignment(const char *text)
{
    const char *equal = strchr(text, '=');
    size_t i, length;
    if (!equal || !name_letter((unsigned char)text[0])) return 0;
    length = (size_t)(equal - text);
    if (length > MAX_NAME) return 0;
    for (i = 1; i < length; i++)
        if (!name_letter((unsigned char)text[i]) && (text[i] < '0' || text[i] > '9'))
            return 0;
    return valid_text(equal + 1, MAX_VALUE);
}

static int safe_byte(int c)
{
    if (output_left == 0) {
        fputs("Text output limit reached.\n", stderr);
        return -1;
    }
    if (putchar(c) == EOF) return -1;
    output_left--;
    return 0;
}

static int safe_literal(const char *text)
{
    while (*text)
        if (safe_byte((unsigned char)*text++) < 0) return -1;
    return 0;
}

static int print_safe(const char *text)
{
    const char hex[] = "0123456789ABCDEF";
    size_t i;
    for (i = 0; i < MAX_ARGUMENT && text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 32 && c <= 126) {
            if (safe_byte(c) < 0) return 1;
        }
        else if (c >= 0xD0 && c <= 0xD3 && i + 1 < MAX_ARGUMENT &&
                 (unsigned char)text[i + 1] >= 0x80 &&
                 (unsigned char)text[i + 1] <= 0xBF) {
            if (safe_byte(c) < 0 || safe_byte((unsigned char)text[++i]) < 0) return 1;
        }
        else if (safe_literal("\\x") < 0 || safe_byte(hex[c >> 4]) < 0 ||
                 safe_byte(hex[c & 15]) < 0) return 1;
    }
    if (text[i] && safe_literal(" [truncated]") < 0) return 1;
    return safe_byte('\n') < 0 ? 1 : 0;
}


static void print_limit(rlim_t value)
{
    char digits[3 * sizeof(rlim_t) + 1];
    size_t used = 0;
    if (value == RLIM_INFINITY) { fputs("unlimited", stdout); return; }
#ifdef RLIM_SAVED_CUR
    if (value == RLIM_SAVED_CUR) { fputs("saved current limit", stdout); return; }
#endif
#ifdef RLIM_SAVED_MAX
    if (value == RLIM_SAVED_MAX) { fputs("saved maximum limit", stdout); return; }
#endif
    do { digits[used++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (used) putchar(digits[--used]);
}

static int show_resource(int resource, const char *label)
{
    struct rlimit limit;
    if (getrlimit(resource, &limit) < 0) { perror("getrlimit"); return 1; }
    printf("%s: ", label); print_limit(limit.rlim_cur); putchar('\n');
    return 0;
}

static int set_resource(int resource, long number, const char *label)
{
    struct rlimit limit;
    if (getrlimit(resource, &limit) < 0) { perror("getrlimit"); return 1; }
    if (limit.rlim_max != RLIM_INFINITY && (rlim_t)number > limit.rlim_max) {
        fputs("Requested value exceeds the hard limit.\n", stderr); return 1;
    }
    limit.rlim_cur = (rlim_t)number;
    if (setrlimit(resource, &limit) < 0) { perror("setrlimit"); return 1; }
    printf("%s set to %ld\n", label, number);
    return 0;
}

static int max_processes(int change, long number)
{
#ifdef RLIMIT_NPROC
    return change ? set_resource(RLIMIT_NPROC, number, "max user processes") :
                    show_resource(RLIMIT_NPROC, "max user processes (-u)");
#else
    long value;
    (void)number;
    if (change) {
        fputs("Changing max user processes is not supported on this system.\n", stderr);
        return 1;
    }
    errno = 0;
    value = sysconf(_SC_CHILD_MAX);
    if (value == -1 && errno) { perror("sysconf"); return 1; }
    if (value == -1) puts("max user processes (-u): unlimited");
    else printf("max user processes (-u): %ld\n", value);
    return 0;
#endif
}

static void print_ids(void)
{
    printf("PID: %ld\nPPID: %ld\nPGRP: %ld\ngroup leader: %s\n",
           (long)getpid(), (long)getppid(), (long)getpgrp(),
           getpid() == getpgrp() ? "yes" : "no");
}

static int execute(const struct command *item)
{
    char name[MAX_NAME + 1], *directory;
    const char *equal;
    size_t capacity, length, count;
    int saved_error, result;
    switch (item->option) {
    case 'i':
        printf("ruid=%lu euid=%lu\nrgid=%lu egid=%lu\n",
               (unsigned long)getuid(), (unsigned long)geteuid(),
               (unsigned long)getgid(), (unsigned long)getegid()); break;
    case 's':
        puts("before:"); print_ids();
        if (setpgid(0, 0) < 0 && getpid() != getpgrp()) { perror("setpgid"); return 1; }
        puts("after:"); print_ids(); break;
    case 'p': print_ids(); break;
    case 'u': return max_processes(0, 0);
    case 'U': return max_processes(1, item->number);
    case 'c': return show_resource(RLIMIT_CORE, "core limit (bytes)");
    case 'C': return set_resource(RLIMIT_CORE, item->number, "core limit (bytes)");
    case 'd':
        for (capacity = 256; capacity <= 1048576; capacity *= 2) {
            directory = malloc(capacity);
            if (!directory) { perror("malloc"); return 1; }
            if (getcwd(directory, capacity)) {
                result = print_safe(directory);
                free(directory);
                return result;
            }
            saved_error = errno; free(directory); errno = saved_error;
            if (errno != ERANGE) { perror("getcwd"); return 1; }
        }
        fputs("Directory path too long.\n", stderr); return 1;
    case 'v':
        for (count = 0; environ && environ[count]; count++)
            if (count == MAX_ENV) { fputs("Too many environment variables (max 100).\n", stderr); return 1; }
        for (count = 0; environ && environ[count]; count++)
            if (print_safe(environ[count])) return 1;
        break;
    case 'V':
        equal = strchr(item->argument, '=');
        length = (size_t)(equal - item->argument);
        memcpy(name, item->argument, length); name[length] = '\0';
        if (setenv(name, equal + 1, 1) < 0) { perror("setenv"); return 1; }
        fputs("Environment updated: ", stdout);
        if (print_safe(item->argument)) return 1;
        break;
    default: return 1;
    }
    return ferror(stdout) ? 1 : 0;
}

static void print_help(void)
{
    puts("Usage: ./task1 [options]; options execute from right to left.\n"
         "-h             help\n-i             UID/GID\n-s             become group leader\n"
         "-p             PID/PPID/PGRP\n-u             max user processes\n"
         "-U NUMBER      set max user processes if supported\n-c             core limit in bytes\n"
         "-C NUMBER      set core limit in bytes\n-d             current directory\n"
         "-v             environment\n-V NAME=VALUE  change environment\n"
         "Limits: 100 options; 32 digits; name 63 bytes; value 255 bytes.");
}

int main(int argc, char *argv[])
{
    struct command commands[MAX_OPTIONS];
    int option, count = 0, i, status = 0, help = 0;
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) { perror("signal"); return 1; }
    if (argc == 1) { print_help(); return fflush(stdout) == EOF || ferror(stdout) ? 1 : 0; }
    if (argc > 2 * MAX_OPTIONS + 1) { fputs("Too many arguments.\n", stderr); return 1; }
    for (i = 1; i < argc; i++)
        if (!valid_text(argv[i], MAX_ARGUMENT)) {
            fputs("Argument too long or contains forbidden characters.\n", stderr); return 1;
        }
    opterr = 0;
    while ((option = getopt(argc, argv, ":hispuU:cC:dvV:")) != -1) {
        if (option == '?' || option == ':') {
            if (option == ':') fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else fputs("Unknown or malformed option.\n", stderr);
            return 1;
        }
        if (count == MAX_OPTIONS) { fputs("Too many options (max 100).\n", stderr); return 1; }
        commands[count].option = option;
        commands[count].argument = optarg;
        commands[count].number = 0;
        if ((option == 'U' || option == 'C') && parse_number(optarg, &commands[count].number) < 0) {
            fprintf(stderr, "Invalid value for -%c: use at most 32 digits, within range.\n", option);
            return 1;
        }
        if (option == 'V' && !valid_assignment(optarg)) {
            fputs("Use NAME=VALUE: name [A-Za-z_][A-Za-z0-9_]* up to 63 bytes, value up to 255.\n", stderr);
            return 1;
        }
        if (option == 'h') help = 1;
        count++;
    }
    if (optind != argc || count == 0) { fputs("Unexpected operand or no options.\n", stderr); return 1; }
    if (help) print_help();
    else for (i = count - 1; i >= 0; i--) {
        if (execute(&commands[i])) status = 1;
        if (ferror(stdout)) { status = 1; break; }
        if (output_left == 0 && i > 0) {
            fputs("Text output limit reached.\n", stderr);
            status = 1;
            break;
        }
    }
    if (fflush(stdout) == EOF || ferror(stdout)) { perror("stdout"); status = 1; }
    return status;
}
