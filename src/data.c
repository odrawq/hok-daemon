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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <cjson/cJSON.h>
#include "log.h"
#include "data.h"

static void load_users(void);
static void save_users(void);
static cJSON *get_or_create_user(const int_fast64_t chat_id, const int save_on_creation);

static cJSON *users_cache;
static pthread_mutex_t users_cache_mutex;

void init_data_module(void)
{
    load_users();
}

int get_state(const int_fast64_t chat_id, const char *state_name)
{
    pthread_mutex_lock(&users_cache_mutex);
    const cJSON *state = cJSON_GetObjectItem(get_or_create_user(chat_id, 1), state_name);
    pthread_mutex_unlock(&users_cache_mutex);

    return state->valueint;
}

void set_state(const int_fast64_t chat_id, const char *state_name, const int state_value)
{
    pthread_mutex_lock(&users_cache_mutex);

    cJSON_SetIntValue(cJSON_GetObjectItem(get_or_create_user(chat_id, 0), state_name), state_value);
    save_users();

    pthread_mutex_unlock(&users_cache_mutex);
}

int has_problem(const int_fast64_t chat_id)
{
    pthread_mutex_lock(&users_cache_mutex);
    const int state = cJSON_GetObjectItem(get_or_create_user(chat_id, 1), "problem") ? 1 : 0;
    pthread_mutex_unlock(&users_cache_mutex);

    return state;
}

void set_problem(const int_fast64_t chat_id, const char *problem_text)
{
    cJSON *problem = cJSON_CreateObject();
    cJSON_AddNumberToObject(problem, "time", time(NULL));
    cJSON_AddStringToObject(problem, "text", problem_text);

    pthread_mutex_lock(&users_cache_mutex);

    cJSON_AddItemToObject(get_or_create_user(chat_id, 0), "problem", problem);
    save_users();

    pthread_mutex_unlock(&users_cache_mutex);
}

void unset_problem(const int_fast64_t chat_id)
{
    pthread_mutex_lock(&users_cache_mutex);

    cJSON_DeleteItemFromObject(get_or_create_user(chat_id, 0), "problem");
    save_users();

    pthread_mutex_unlock(&users_cache_mutex);
}

cJSON *get_problems(const int include_chat_ids, const int banned_problems, const int pending_problems)
{
    cJSON *problems = cJSON_CreateArray();

    pthread_mutex_lock(&users_cache_mutex);
    cJSON *user = users_cache->child;

    while (user)
    {
        const int_fast64_t chat_id = strtoll(user->string, NULL, 10);

        const int account_ban_state = cJSON_GetNumberValue(cJSON_GetObjectItem(get_or_create_user(chat_id, 0), "account_ban_state"));
        const int problem_pending_state = cJSON_GetNumberValue(cJSON_GetObjectItem(get_or_create_user(chat_id, 0), "problem_pending_state"));

        if ((banned_problems && !pending_problems && !(account_ban_state && !problem_pending_state)) ||
            (!banned_problems && pending_problems && !(!account_ban_state && problem_pending_state)) ||
            (!banned_problems && !pending_problems && !(!account_ban_state && !problem_pending_state)) ||
            (banned_problems && pending_problems && !(account_ban_state && problem_pending_state)))
            goto next;

        const cJSON *problem = cJSON_GetObjectItem(user, "problem");

        if (problem)
        {
            cJSON *text = cJSON_GetObjectItem(problem, "text");

            if (!include_chat_ids)
                cJSON_AddItemToArray(problems, cJSON_CreateString(text->valuestring));
            else
            {
                char chat_id_with_problem[MAX_CHAT_ID_SIZE + MAX_USERNAME_SIZE + MAX_PROBLEM_SIZE + 7];
                sprintf(chat_id_with_problem,
                        "(%s) %s",
                        user->string,
                        text->valuestring);

                cJSON_AddItemToArray(problems, cJSON_CreateString(chat_id_with_problem));
            }
        }

    next:
        user = user->next;
    }

    pthread_mutex_unlock(&users_cache_mutex);
    return problems;
}

cJSON *get_outdated_problems_chat_ids(void)
{
    cJSON *outdated_problems_chat_ids = cJSON_CreateArray();

    pthread_mutex_lock(&users_cache_mutex);
    cJSON *user = users_cache->child;

    while (user)
    {
        const int_fast64_t chat_id = strtoll(user->string, NULL, 10);

        if (cJSON_GetNumberValue(cJSON_GetObjectItem(get_or_create_user(chat_id, 0), "account_ban_state")) ||
            cJSON_GetNumberValue(cJSON_GetObjectItem(get_or_create_user(chat_id, 0), "problem_pending_state")))
            goto next;

        const cJSON *problem = cJSON_GetObjectItem(user, "problem");

        if (problem && difftime(time(NULL), cJSON_GetNumberValue(cJSON_GetObjectItem(problem, "time"))) > MAX_PROBLEM_SECONDS)
            cJSON_AddItemToArray(outdated_problems_chat_ids, cJSON_CreateString(user->string));

    next:
        user = user->next;
    }

    pthread_mutex_unlock(&users_cache_mutex);
    return outdated_problems_chat_ids;
}

/*
 * Loads data from FILE_USERS into users_cache.
 */
static void load_users(void)
{
    FILE *users_file = fopen(FILE_USERS, "r");

    if (!users_file)
        die("%s: %s: failed to open %s",
            __BASE_FILE__,
            __func__,
            FILE_USERS);

    fseek(users_file, 0, SEEK_END);
    const size_t users_file_size = ftell(users_file);
    rewind(users_file);

    char *users_string = malloc(users_file_size + 1);

    if (!users_string)
        die("%s: %s: failed to allocate memory for users_string",
            __BASE_FILE__,
            __func__);

    if (fread(users_string,
              1,
              users_file_size,
              users_file) != users_file_size)
        die("%s: %s: failed to read data from %s",
            __BASE_FILE__,
            __func__,
            FILE_USERS);

    fclose(users_file);

    users_string[users_file_size] = 0;
    cJSON *users = cJSON_Parse(users_string);

    if (!users)
        die("%s: %s: failed to parse users_string",
            __BASE_FILE__,
            __func__);

    free(users_string);
    users_cache = users;
}

/*
 * Saves data from users_cache into FILE_USERS.
 */
static void save_users(void)
{
    char *users_string = cJSON_Print(users_cache);

    if (!users_string)
        die("%s: %s: failed to print users_cache");

    FILE *users_file = fopen(FILE_USERS, "w");

    if (!users_file)
        die("%s: %s: failed to open %s",
            __BASE_FILE__,
            __func__,
            FILE_USERS);

    fprintf(users_file, "%s", users_string);

    fclose(users_file);
    free(users_string);
}

static cJSON *get_or_create_user(const int_fast64_t chat_id, const int save_on_creation)
{
    char chat_id_string[MAX_CHAT_ID_SIZE + 1];
    sprintf(chat_id_string,
            "%" PRIdFAST64,
            chat_id);

    cJSON *user = cJSON_GetObjectItem(users_cache, chat_id_string);

    if (!user)
    {
        user = cJSON_CreateObject();
        cJSON_AddNumberToObject(user, "account_ban_state", 0);
        cJSON_AddNumberToObject(user, "problem_pending_state", 1);
        cJSON_AddNumberToObject(user, "problem_description_state", 0);

        cJSON_AddItemToObject(users_cache, chat_id_string, user);

        if (save_on_creation)
            save_users();
    }

    return user;
}
