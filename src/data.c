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

static void load_users_data(void);
static void save_users_data(void);
static cJSON *get_or_create_user_data(const int_fast64_t chat_id, const int save_on_creation);

static cJSON *users_data_cache;
static pthread_mutex_t users_data_cache_mutex;
static pthread_mutexattr_t users_data_cache_mutex_attr;

void init_data_module(void)
{
    pthread_mutexattr_init(&users_data_cache_mutex_attr);
    pthread_mutexattr_settype(&users_data_cache_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&users_data_cache_mutex, &users_data_cache_mutex_attr);
    load_users_data();
}

int get_state(const int_fast64_t chat_id, const char *state_name)
{
    pthread_mutex_lock(&users_data_cache_mutex);
    const cJSON *state = cJSON_GetObjectItem(get_or_create_user_data(chat_id, 1), state_name);
    pthread_mutex_unlock(&users_data_cache_mutex);

    return state->valueint;
}

void set_state(const int_fast64_t chat_id, const char *state_name, const int state_value)
{
    pthread_mutex_lock(&users_data_cache_mutex);

    cJSON_SetIntValue(cJSON_GetObjectItem(get_or_create_user_data(chat_id, 0), state_name), state_value);
    save_users_data();

    pthread_mutex_unlock(&users_data_cache_mutex);
}

int has_problem(const int_fast64_t chat_id)
{
    pthread_mutex_lock(&users_data_cache_mutex);
    const int state = cJSON_GetObjectItem(get_or_create_user_data(chat_id, 1), "problem") ? 1 : 0;
    pthread_mutex_unlock(&users_data_cache_mutex);

    return state;
}

void set_problem(const int_fast64_t chat_id, const char *problem)
{
    pthread_mutex_lock(&users_data_cache_mutex);

    const time_t current_time = time(NULL);
    cJSON *user_problem = cJSON_CreateObject();

    cJSON_AddNumberToObject(user_problem, "time", current_time);
    cJSON_AddStringToObject(user_problem, "text", problem);
    cJSON_AddItemToObject(get_or_create_user_data(chat_id, 0), "problem", user_problem);

    save_users_data();

    pthread_mutex_unlock(&users_data_cache_mutex);
}

void unset_problem(const int_fast64_t chat_id)
{
    pthread_mutex_lock(&users_data_cache_mutex);

    cJSON_DeleteItemFromObject(get_or_create_user_data(chat_id, 0), "problem");
    save_users_data();

    pthread_mutex_unlock(&users_data_cache_mutex);
}

cJSON *get_problems(const int include_chat_ids, const int banned_problems, const int pending_problems)
{
    pthread_mutex_lock(&users_data_cache_mutex);

    cJSON *problems = cJSON_CreateArray();
    cJSON *user_data = users_data_cache->child;

    while (user_data)
    {
        const int_fast64_t chat_id = strtoll(user_data->string, NULL, 10);

        const int account_ban_state = get_state(chat_id, "account_ban_state");
        const int problem_pending_state = get_state(chat_id, "problem_pending_state");

        if ((banned_problems && !pending_problems && !(account_ban_state && !problem_pending_state)) ||
            (!banned_problems && pending_problems && !(!account_ban_state && problem_pending_state)) ||
            (!banned_problems && !pending_problems && !(!account_ban_state && !problem_pending_state)) ||
            (banned_problems && pending_problems && !(account_ban_state && problem_pending_state)))
        {
            user_data = user_data->next;
            continue;
        }

        const cJSON *user_problem = cJSON_GetObjectItem(user_data, "problem");

        if (user_problem)
        {
            const char *user_problem_string = cJSON_GetStringValue(cJSON_GetObjectItem(user_problem, "text"));

            if (!include_chat_ids)
                cJSON_AddItemToArray(problems, cJSON_CreateString(user_problem_string));
            else
            {
                char chat_id_with_problem[MAX_CHAT_ID_SIZE + MAX_USERNAME_SIZE + MAX_PROBLEM_SIZE + 7];
                sprintf(chat_id_with_problem,
                        "(%s) %s",
                        user_data->string,
                        user_problem_string);

                cJSON_AddItemToArray(problems, cJSON_CreateString(chat_id_with_problem));
            }
        }

        user_data = user_data->next;
    }

    pthread_mutex_unlock(&users_data_cache_mutex);
    return problems;
}

cJSON *get_outdated_problems_chat_ids(void)
{
    pthread_mutex_lock(&users_data_cache_mutex);

    cJSON *outdated_problems_chat_ids = cJSON_CreateArray();
    cJSON *user_data = users_data_cache->child;

    while (user_data)
    {
        const int_fast64_t chat_id = strtoll(user_data->string, NULL, 10);

        if (get_state(chat_id, "account_ban_state") ||
            get_state(chat_id, "problem_pending_state"))
        {
            user_data = user_data->next;
            continue;
        }

        const cJSON *user_problem = cJSON_GetObjectItem(user_data, "problem");

        if (user_problem)
        {
            const time_t current_time = time(NULL);
            const time_t user_problem_time = cJSON_GetNumberValue(cJSON_GetObjectItem(user_problem, "time"));

            if (difftime(current_time, user_problem_time) > MAX_PROBLEM_SECONDS)
                cJSON_AddItemToArray(outdated_problems_chat_ids, cJSON_CreateString(user_data->string));
        }

        user_data = user_data->next;
    }

    pthread_mutex_unlock(&users_data_cache_mutex);
    return outdated_problems_chat_ids;
}

/*
 * Loads data from FILE_USERSDATA into users_data.
 */
static void load_users_data(void)
{
    FILE *users_data_file = fopen(FILE_USERSDATA, "r");

    if (!users_data_file)
        die("Failed to open %s", FILE_USERSDATA);

    fseek(users_data_file, 0, SEEK_END);
    const size_t users_data_file_size = ftell(users_data_file);
    rewind(users_data_file);

    char *users_data_string = malloc(users_data_file_size + 1);

    if (!users_data_string)
        die("Failed to allocate memory for users data");

    if (fread(users_data_string,
              1,
              users_data_file_size,
              users_data_file) != users_data_file_size)
        die("Failed to read data from %s", FILE_USERSDATA);

    fclose(users_data_file);

    users_data_string[users_data_file_size] = 0;
    cJSON *users_data = cJSON_Parse(users_data_string);

    if (!users_data)
        die("Failed to parse users data");

    free(users_data_string);
    users_data_cache = users_data;
}

/*
 * Saves data from users_data into FILE_USERSDATA.
 */
static void save_users_data(void)
{
    char *users_data_string = cJSON_Print(users_data_cache);

    if (!users_data_string)
        die("Failed to get users data cache");

    FILE *users_data_file = fopen(FILE_USERSDATA, "w");

    if (!users_data_file)
        die("Failed to open %s", FILE_USERSDATA);

    fprintf(users_data_file, "%s", users_data_string);

    fclose(users_data_file);
    free(users_data_string);
}

static cJSON *get_or_create_user_data(const int_fast64_t chat_id, const int save_on_creation)
{
    char chat_id_string[MAX_CHAT_ID_SIZE + 1];
    sprintf(chat_id_string,
            "%" PRIdFAST64,
            chat_id);

    cJSON *user_data = cJSON_GetObjectItem(users_data_cache, chat_id_string);

    if (!user_data)
    {
        user_data = cJSON_CreateObject();

        cJSON_AddNumberToObject(user_data, "account_ban_state", 0);
        cJSON_AddNumberToObject(user_data, "problem_pending_state", 1);
        cJSON_AddNumberToObject(user_data, "problem_description_state", 0);
        cJSON_AddItemToObject(users_data_cache, chat_id_string, user_data);

        if (save_on_creation)
            save_users_data();
    }

    return user_data;
}
