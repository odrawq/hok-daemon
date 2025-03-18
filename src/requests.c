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
#include <inttypes.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "log.h"
#include "requests.h"

typedef struct
{
    char *data;
    size_t size;
}
ServerResponse;

static size_t write_callback(void *data,
                             const size_t data_size,
                             const size_t data_count,
                             void *server_response);

void init_requests_module(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

cJSON *get_updates(const int_fast32_t update_id)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("%s: %s: failed to initialize curl",
            __BASE_FILE__,
            __func__);

    ServerResponse response;
    response.data = malloc(1);

    if (!response.data)
        die("%s: %s: failed to allocate memory for response.data",
            __BASE_FILE__,
            __func__);

    response.size = 0;

    char url[MAX_URL_SIZE];
    sprintf(url,
            "%s/getUpdates?offset=%" PRIdFAST32
            "&timeout=%d",
            BOT_API_URL,
            update_id,
            MAX_RESPONSE_TIMEOUT);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, MAX_CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, MAX_RESPONSE_TIMEOUT);

    CURLcode code;
    int retries = 0;

    do
    {
        code = curl_easy_perform(curl);

        if (code == CURLE_OK)
            break;
    }
    while (++retries < MAX_REQUEST_RETRIES);

    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
    {
        free(response.data);
        return NULL;
    }

    cJSON *updates = cJSON_Parse(response.data);

    if (!updates)
        die("%s: %s: failed to parse response.data",
            __BASE_FILE__,
            __func__);

    free(response.data);
    return updates;
}

cJSON *get_chat(const int_fast64_t chat_id)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("%s: %s: failed to initialize curl",
            __BASE_FILE__,
            __func__);

    ServerResponse response;
    response.data = malloc(1);

    if (!response.data)
        die("%s: %s: failed to allocate memory for response.data",
            __BASE_FILE__,
            __func__);

    response.size = 0;

    char url[MAX_URL_SIZE];
    sprintf(url,
            "%s/getChat?chat_id=%" PRIdFAST64,
            BOT_API_URL,
            chat_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, MAX_CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, MAX_RESPONSE_TIMEOUT);

    CURLcode code;
    int retries = 0;

    do
    {
        code = curl_easy_perform(curl);

        if (code == CURLE_OK)
            break;
    }
    while (++retries < MAX_REQUEST_RETRIES);

    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
    {
        free(response.data);
        return NULL;
    }

    cJSON *chat = cJSON_Parse(response.data);

    if (!chat)
        die("%s: %s: failed to parse response.data",
            __BASE_FILE__,
            __func__);

    free(response.data);
    return chat;
}

void send_message_with_keyboard(const int_fast64_t chat_id, const char *message, const char *keyboard)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("%s: %s: failed to initialize curl",
            __BASE_FILE__,
            __func__);

    char *escaped_message = curl_easy_escape(curl, message, 0);

    if (!escaped_message)
        die("%s: %s: failed to escape message",
            __BASE_FILE__,
            __func__);

    char post_fields[MAX_POSTFIELDS_SIZE];
    sprintf(post_fields,
            "chat_id=%" PRIdFAST64
            "&text=%s"
            "&reply_markup=%s",
            chat_id,
            escaped_message,
            keyboard);

    char url[MAX_URL_SIZE];
    sprintf(url,
            "%s/sendMessage",
            BOT_API_URL);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, MAX_CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, MAX_RESPONSE_TIMEOUT);

    int retries = 0;

    do
        if (curl_easy_perform(curl) == CURLE_OK)
            break;
    while (++retries < MAX_REQUEST_RETRIES);

    curl_free(escaped_message);
    curl_easy_cleanup(curl);
}

static size_t write_callback(void *data,
                             const size_t data_size,
                             const size_t data_count,
                             void *server_response)
{
    const size_t data_real_size = data_size * data_count;

    ServerResponse *response = server_response;
    response->data = realloc(response->data, response->size + data_real_size + 1);

    if (!response->data)
        die("%s: %s: failed to reallocate memory for response->data",
            __BASE_FILE__,
            __func__);

    memcpy(&(response->data[response->size]), data, data_real_size);

    response->size += data_real_size;
    response->data[response->size] = 0;

    return data_real_size;
}
