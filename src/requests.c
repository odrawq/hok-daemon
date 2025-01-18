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

static size_t write_callback(void *contents,
                             const size_t size,
                             const size_t nmemb,
                             void *userp);

struct ServerResponse
{
    char *data;
    size_t size;
};

void init_requests_module(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

cJSON *get_updates(const int_fast32_t update_id)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("Failed to initialize curl");

    struct ServerResponse response;
    response.data = malloc(1);

    if (!response.data)
        die("Failed to allocate memory for server response");

    response.size = 0;

    char url[MAX_URL_SIZE];
    sprintf(url,
            "%s/getUpdates?offset=%" PRIdFAST32,
            BOT_API_URL,
            update_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    const CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK)
    {
        free(response.data);
        return NULL;
    }

    cJSON *updates = cJSON_Parse(response.data);

    if (!updates)
        die("Failed to parse updates");

    free(response.data);
    return updates;
}

void send_message_with_keyboard(const int_fast64_t chat_id, const char *message, const char *keyboard)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("Failed to initialize curl");

    char *encoded_message = curl_easy_escape(curl, message, 0);

    if (!encoded_message)
        die("Failed to URL encode message");

    char post_fields[MAX_POSTFIELDS_SIZE];
    sprintf(post_fields,
            "chat_id=%" PRIdFAST64
            "&text=%s"
            "&reply_markup=%s",
            chat_id,
            encoded_message,
            keyboard);

    char url[MAX_URL_SIZE];
    sprintf(url,
            "%s/sendMessage",
            BOT_API_URL);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

    curl_easy_perform(curl);

    curl_free(encoded_message);
    curl_easy_cleanup(curl);
}

static size_t write_callback(void *contents,
                             const size_t size,
                             const size_t nmemb,
                             void *userp)
{
    const size_t real_size = size * nmemb;

    struct ServerResponse *response = (struct ServerResponse *) userp;
    response->data = realloc(response->data, response->size + real_size + 1);

    if (!response->data)
        die("Failed to reallocate memory for server response");

    memcpy(&(response->data[response->size]),
           contents,
           real_size);

    response->size += real_size;
    response->data[response->size] = 0;

    return real_size;
}
