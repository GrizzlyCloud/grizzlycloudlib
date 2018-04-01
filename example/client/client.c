#include <gc.h>

static void callback_account_set(struct gc_s *gc, sn error)
{
    hm_log(LOG_DEBUG, &gc->log, "Account set: [%.*s]", sn_p(error));
}

static void callback_account_exists(struct gc_s *gc, sn error)
{
    hm_log(LOG_DEBUG, &gc->log, "Account exists: [%.*s]", sn_p(error));
}

static void callback_traffic(struct gc_s *gc, sn error, sn type, sn cloud,
                             sn device, sn upload, sn download)
{
    hm_log(LOG_DEBUG, &gc->log, "Traffic get: [%.*s:%.*s:%.*s:%.*s:%.*s:%.*s]",
                                sn_p(error),
                                sn_p(type),
                                sn_p(cloud),
                                sn_p(device),
                                sn_p(upload),
                                sn_p(download));
}

static void callback_login(struct gc_s *gc, sn error)
{
    hm_log(LOG_DEBUG, &gc->log, "Login error: [%.*s]", sn_p(error));
}

static void client_account_create(struct gc_s *gc)
{
    struct proto_s as = { .type = ACCOUNT_SET };
    sn_set(as.u.account_set.email,    gc->config.username);
    sn_set(as.u.account_set.password, gc->config.password);

    hm_log(LOG_DEBUG, &gc->log, "Sending account set for username [%.*s]",
                                sn_p(gc->config.username));

    int ret;
    ret = gc_packet_send(gc, &as);
    if(ret != GC_OK) CALLBACK_ERROR(&gc->log, "client_account_set");
}

static void client_account_exists(struct gc_s *gc)
{
    struct proto_s as = { .type = ACCOUNT_EXISTS };
    sn_set(as.u.account_exists.email,    gc->config.username);
    sn_set(as.u.account_exists.password, gc->config.password);

    hm_log(LOG_DEBUG, &gc->log, "Sending account exists for username [%.*s]",
                                sn_p(gc->config.username));

    int ret;
    ret = gc_packet_send(gc, &as);
    if(ret != GC_OK) CALLBACK_ERROR(&gc->log, "client_account_exists");
}

static void client_login(struct gc_s *gc)
{
    struct proto_s as = { .type = ACCOUNT_LOGIN };
    sn_set(as.u.account_login.email,    gc->config.username);
    sn_set(as.u.account_login.password, gc->config.password);
    sn_set(as.u.account_login.devname,  gc->config.device);

    hm_log(LOG_DEBUG, &gc->log, "Sending login request for username [%.*s]",
                                sn_p(gc->config.username));

    int ret;
    ret = gc_packet_send(gc, &as);
    if(ret != GC_OK) CALLBACK_ERROR(&gc->log, "client_login");
}

static void callback_state_changed(struct gc_s *gc, enum gc_state_e state)
{
    switch(state) {
        case GC_HANDSHAKE_SUCCESS: {
            hm_log(LOG_TRACE, &gc->log, "Connected to upstream");
            sn_initz(account_create, "account_create");
            sn_initz(account_exists, "account_exists");
            if(sn_cmps(gc->config.action, account_create)) {
                client_account_create(gc);
            } else if(sn_cmps(gc->config.action, account_exists)) {
                client_account_exists(gc);
            } else {
                client_login(gc);
            }
        }
        break;

        default: {
            printf("Upstream changed: %d\n", state);
        }
        break;
    }
}

static void do_daemon()
{
    pid_t pid = fork();
    if(pid < 0) {
        printf("Unable to daemonize: fork failed: %s\n", strerror(errno));
        exit(1);
    }

    if(pid != 0) {
        printf("Daemonized as pid %d.\n", pid);
        exit(0);
    }

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

#define NULL_DEV	"/dev/null"

    stdin = fopen(NULL_DEV, "r");
    if(stdin == NULL) {
        printf("Unable to reopen stdin to %s: %s\n", NULL_DEV, strerror(errno));
        exit(1);
    }

    stdout = fopen(NULL_DEV, "w");
    if(stdout == NULL) {
        printf("Unable to reopen stdout to %s: %s\n", NULL_DEV, strerror(errno));
        exit(1);
    }

    stderr = fopen(NULL_DEV, "w");
    if(stderr == NULL) {
        printf("Unable to reopen stderr to %s: %s\n", NULL_DEV, strerror(errno));
        exit(1);
    }

    pid_t s = setsid();
    if(s < 0) {
        printf("Unable to create new session, setsid(2) failed: %s :: %d\n", strerror(errno), s);
        exit(1);
    }

    printf("Successfully daemonized as pid %d.\n", getpid());
}

int main(int argc, char **argv)
{
    struct gc_init_s gci;
    struct gc_s *gc;

    if(argc == 1) {
        printf("\n");
        printf("  GrizzlyCloud - Simplified VPN alternative for IoT\n");
        printf("\n");
        printf("  --config <file> - Set configuration file.\n");
        printf("  --log <file>    - Set log file.\n");
        printf("  --nolog         - Redirect all log messages to stdout.\n");
        printf("  --daemonize     - Daemonize client.\n");
        printf("  --traffic       - Get current traffic (Administration only).\n");
        printf("\n");
        exit(1);
    }

    const char *config_file = NULL;
    const char *log_file    = NULL;
    int nolog = 0;
    int daemonize = 0;
    int traffic   = 0;

    int i;
    for(i = 0; i < argc; i++) {
        if(strcmp(argv[i], "--config") == 0 && (i + 1) < argc)
            config_file = argv[i + 1];
        else if(strcmp(argv[i], "--log") == 0 && (i + 1) < argc)
            log_file = argv[i + 1];
        else if(strcmp(argv[i], "--nolog") == 0)
            nolog = 1;
        else if(strcmp(argv[i], "--daemonize") == 0)
            daemonize = 1;
        else if(strcmp(argv[i], "--traffic") == 0)
            traffic = 1;
    }

    if(config_file == NULL) {
        printf("Configuration file required.\n");
        exit(1);
    }

    if(log_file == NULL && nolog == 0) {
        printf("Either --log <file> or --nolog must be set.\n");
        exit(1);
    }

    if(daemonize == 1 && log_file == NULL) {
        printf("When daemonized, --log <file> must be specified.\n");
        exit(1);
    }

    if(daemonize == 1) do_daemon();

    memset(&gci, 0, sizeof(gci));

    gci.loop                   = ev_default_loop(0);
    gci.cfgfile                = config_file;
    gci.logfile                = log_file;
    gci.loglevel               = LOG_TRACE;
    gci.callback.state_changed = callback_state_changed;
    gci.callback.login         = callback_login;
    gci.callback.traffic       = callback_traffic;
    gci.callback.account_set   = callback_account_set;
    gci.callback.account_exists= callback_account_exists;
    gci.traffic                = traffic == 1 ? 1 : 0;

    gc = gc_init(&gci);
    if(gc == NULL) {
        return 1;
    }

    ev_run(gci.loop, 0);
    gc_deinit(gc);

    return 0;
}
