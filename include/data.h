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

#ifndef DATA_H
    #define DATA_H

    #include <stdint.h>

    #include <cjson/cJSON.h>

    #define FILE_USERS "/var/lib/hok-daemon/users.json"

    #define MAX_USERNAME_SIZE 32
    #define MAX_CHAT_ID_SIZE  20
    #define MAX_PROBLEM_SIZE  1024

    #define MAX_PROBLEM_LIFETIME 1814400 // 21 days.

    void init_data_module(void);

    /*
     * Returns 1 if a user exists, else 0.
     */
    int has_user(const int_fast64_t chat_id);

    /*
     * Creates a user.
     */
    void create_user(const int_fast64_t chat_id);

    /*
     * Returns a user state, which can be 0 or 1.
     * state_name must be 'account_ban_state', 'problem_pending_state' or 'problem_description_state'.
     */
    int get_state(const int_fast64_t chat_id, const char *state_name);

    /*
     * Sets a user state.
     * state_name must be 'account_ban_state', 'problem_pending_state' or 'problem_description_state'.
     * state_value must be 0 or 1.
     */
    void set_state(const int_fast64_t chat_id, const char *state_name, const int state_value);

    /*
     * Returns 1 if a user has a problem, else 0.
     */
    int has_problem(const int_fast64_t chat_id);

    /*
     * Creates a user problem.
     */
    void create_problem(const int_fast64_t chat_id, const char *problem_text, const int use_time_limit);

    /*
     * Modifies a user problem.
     */
    void modify_problem(const int_fast64_t chat_id, const char *problem_text);

    /*
     * Deletes a user problem.
     */
    void delete_problem(const int_fast64_t chat_id);

    /*
     * Returns all specified users problems.
     * Users problems can be
     * non-pending and non-banned, pending and non-banned, non-pending and banned or pending and banned.
     */
    cJSON *get_problems(const int include_chat_ids, const int pending_problems, const int banned_problems);

    /*
     * Returns all expired users problems chat ids.
     */
    cJSON *get_expired_problems_chat_ids(void);

#endif
