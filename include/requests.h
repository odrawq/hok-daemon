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

#ifndef REQUESTS_H
    #define REQUESTS_H

    #include <stdint.h>

    #include <cjson/cJSON.h>

    #include "config.h" // This file is created after the configure.sh successfully executed.

    #define BOT_API_URL "https://api.telegram.org/bot" BOT_TOKEN

    #define MAX_URL_SIZE        512
    #define MAX_POSTFIELDS_SIZE 4096

    #define MAX_CONNECT_TIMEOUT  15
    #define MAX_RESPONSE_TIMEOUT 30

    #define MAX_REQUEST_RETRIES 3

    void init_requests_module(void);

    /*
     * Returns updates from the Telegram Bot API.
     */
    cJSON *get_updates(const int_fast32_t update_id);

    /*
     * Returns a chat from the Telegram Bot API.
     */
    cJSON *get_chat(const int_fast64_t chat_id);

    /*
     * Sends a message with a keyboard to a chat via the Telegram Bot API.
     */
    void send_message_with_keyboard(const int_fast64_t chat_id, const char *message, const char *keyboard);

#endif
