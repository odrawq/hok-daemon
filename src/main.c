/******************************************************************************
 *                                                                            *
 *   _             _                _                                         *
 *  | |__    ___  | | __         __| |  __ _   ___  _ __ ___    ___   _ __    *
 *  | '_ \  / _ \ | |/ / _____  / _` | / _` | / _ \| '_ ` _ \  / _ \ | '_ \   *
 *  | | | || (_) ||   < |_____|| (_| || (_| ||  __/| | | | | || (_) || | | |  *
 *  |_| |_| \___/ |_|\_\        \__,_| \__,_| \___||_| |_| |_| \___/ |_| |_|  *
 *                                                                            *
 * Copyright (C) 2024-2025 qwardo <odrawq.qwardo@gmail.com>                   *
 *                                                                            *
 * This file is part of hok-daemon.                                           *
 *                                                                            *
 * hok-daemon is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * hok-daemon is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with hok-daemon. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                            *
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>

#include "version.h"
#include "log.h"
#include "requests.h"
#include "data.h"
#include "bot.h"

#define ERRORSTAMP "\e[0;31;1mError:\e[0m"

static void handle_args(int argc, char **argv);
static void check_user(void);
static void daemonize(void);
static void init_signals(void);
static void init_modules(void);
static void init_info(void);
static void handle_signal(const int signal);

static int maintenance_mode = 0;

static pid_t pid;
static char *mode;

int main(int argc, char **argv)
{
    handle_args(argc, argv);

    check_user();
    daemonize();

    init_signals();
    init_modules();
    init_info();

    report("hok-daemon %d.%d.%d started (PID: %d; Mode: %s)",
           MAJOR_VERSION,
           MINOR_VERSION,
           PATCH_VERSION,
           pid,
           mode);

    start_bot(maintenance_mode);
}

static void handle_args(int argc, char **argv)
{
    if (argc == 1)
        return;

    opterr = 0;

    static const struct option long_options[] =
    {
        {"help",        no_argument, 0, 'h'},
        {"version",     no_argument, 0, 'v'},
        {"maintenance", no_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;

    while ((opt = getopt_long(argc,
                              argv,
                              "+hvm",
                              long_options,
                              NULL)) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("Usage: hok-daemon [option]\n"
                       "Daemon for controlling the 'Hands of Kindness' Telegram bot.\n\n"
                       "Options:\n"
                       "  -h, --help           print this help and exit\n"
                       "  -v, --version        print the hok-daemon version and exit\n"
                       "  -m, --maintenance    run the hok-daemon in maintenance mode\n"
                       "\nTo run the hok-daemon, run it as the hok-daemon user."
                       "\nPlease send bug reports to <odrawq.qwardo@gmail.com>\n");
                exit(EXIT_SUCCESS);

            case 'v':
                printf("hok-daemon %d.%d.%d\n",
                       MAJOR_VERSION,
                       MINOR_VERSION,
                       PATCH_VERSION);
                exit(EXIT_SUCCESS);

            case 'm':
                maintenance_mode = 1;
                break;

            case '?':
                if (optopt)
                    fprintf(stderr,
                            ERRORSTAMP " unknown option '-%c'\n"
                            "Try 'hok-daemon -h' for more information.\n",
                            optopt);
                else
                    fprintf(stderr,
                            ERRORSTAMP " unknown option '%s'\n"
                            "Try 'hok-daemon -h' for more information.\n",
                            argv[optind - 1]);

                // Fall through.

            default:
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
    {
        fprintf(stderr,
                ERRORSTAMP " expecting options only\n"
                "Try 'hok-daemon -h' for more information.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Checks the user running the process.
 * If the user is not hok-daemon, terminates the process.
 */
static void check_user(void)
{
    const struct passwd *pw = getpwuid(getuid());

    if (!pw)
    {
        fprintf(stderr,
                ERRORSTAMP " failed to get user data\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(pw->pw_name, "hok-daemon"))
    {
        fprintf(stderr,
                ERRORSTAMP " hok-daemon must be start under the hok-daemon user\n");
        exit(EXIT_FAILURE);
    }
}

static void daemonize(void)
{
    if (daemon(0, 0) < 0)
    {
        fprintf(stderr,
                ERRORSTAMP " failed to daemonize\n");
        exit(EXIT_FAILURE);
    }
}

static void init_signals(void)
{
    signal(SIGTERM, handle_signal);
    signal(SIGSEGV, handle_signal);
}

static void init_modules(void)
{
    init_requests_module();

    if (!maintenance_mode)
        init_data_module();
}

static void init_info(void)
{
    pid = getpid();
    mode = maintenance_mode ? "Maintenance" : "Default";
}

static void handle_signal(const int signal)
{
    switch (signal)
    {
        case SIGTERM:
            report("hok-daemon %d.%d.%d terminated (PID: %d; Mode: %s)",
                   MAJOR_VERSION,
                   MINOR_VERSION,
                   PATCH_VERSION,
                   pid,
                   mode);
            exit(EXIT_SUCCESS);

        case SIGSEGV:
            die("Segmentation fault");
    }
}
