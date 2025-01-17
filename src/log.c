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
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "requests.h"
#include "data.h"
#include "log.h"

static char *get_current_timestamp(void);

/*
 * Prints a formatted string to the FILE_INFOLOG with timestamp.
 */
void report(const char *fmt, ...)
{
    FILE *file = fopen(FILE_INFOLOG, "a");

    if (!file)
        die("Failed to open %s", FILE_INFOLOG);

    fprintf(file, "%s ", get_current_timestamp());
    va_list argp;
    va_start(argp, fmt);
    vfprintf(file, fmt, argp);
    va_end(argp);
    fputc('\n', file);

    fclose(file);
}

/*
 * Prints a formatted string to the FILE_ERRORLOG with timestamp and
 * exits the daemon with EXIT_FAILURE status.
 */
void die(const char *fmt, ...)
{
    FILE *file = fopen(FILE_ERRORLOG, "a");

    if (file)
    {
        fprintf(file, "%s ", get_current_timestamp());
        va_list argp;
        va_start(argp, fmt);
        vfprintf(file, fmt, argp);
        va_end(argp);
        fputc('\n', file);

        fclose(file);
    }

    exit(EXIT_FAILURE);
}

static char *get_current_timestamp(void)
{
    const time_t current_time = time(NULL);
    static char timestamp[MAX_TIMESTAMP_SIZE + 1];

    strftime(timestamp,
             sizeof timestamp,
             "[%Y-%m-%d %H:%M:%S]",
             localtime(&current_time));

    return timestamp;
}
