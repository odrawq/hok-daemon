/******************************************************************************
 *                                                                            *
 *   _             _                _                                         *
 *  | |__    ___  | | __         __| |  __ _   ___  _ __ ___    ___   _ __    *
 *  | '_ \  / _ \ | |/ / _____  / _` | / _` | / _ \| '_ ` _ \  / _ \ | '_ \   *
 *  | | | || (_) ||   < |_____|| (_| || (_| ||  __/| | | | | || (_) || | | |  *
 *  |_| |_| \___/ |_|\_\        \__,_| \__,_| \___||_| |_| |_| \___/ |_| |_|  *
 *                                                                            *
 * Copyright (C) 2024 qwardo <odrawq.qwardo@gmail.com>                        *
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

#ifndef DATA_H
    #define DATA_H

    #include <stdint.h>
    #include <cjson/cJSON.h>

    #define FILE_USERSDATA "/var/lib/hok-daemon/users_data.json"

    #define MAX_USERNAME_SIZE   32
    #define MAX_CHAT_ID_SIZE    20
    #define MAX_PROBLEM_SIZE    1024
    #define MAX_PROBLEM_SECONDS 2592000

    void init_data_module(void);
    void set_state(const int_fast64_t chat_id, const char *state_name, const int state_value);
    int get_state(const int_fast64_t chat_id, const char *state_name);
    void set_problem(const int_fast64_t chat_id, const char *problem);
    void unset_problem(const int_fast64_t chat_id);
    int has_problem(const int_fast64_t chat_id);
    cJSON *get_problems(const int include_chat_ids, const int banned_problems);
    cJSON *get_outdated_problems_chat_ids(void);

#endif
