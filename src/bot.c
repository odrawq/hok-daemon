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
#include <string.h>
#include <cjson/cJSON.h>
#include "config.h" // This file is created after the configure.sh successfully executed.
#include "log.h"
#include "requests.h"
#include "data.h"
#include "bot.h"

static void unset_outdated_problems(void);
static void handle_updates(cJSON *updates, int_fast32_t *last_update_id);
static void *handle_message(void *cjson_message);
static void handle_problem(const int_fast64_t chat_id, const char *username, const char *problem);
static void handle_command(const int_fast64_t chat_id,
                           const int is_root_user,
                           const char *username,
                           const char *command);
static void handle_banlist_command(const int_fast64_t chat_id, const int is_root_user);
static void handle_ban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_unban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_start_command(const int_fast64_t chat_id, const int is_root_user, const char *username);
static void handle_helpme_command(const int_fast64_t chat_id, const char *username);
static void handle_helpsomeone_command(const int_fast64_t chat_id, const int is_root_user);
static void handle_closeproblem_command(const int_fast64_t chat_id);

void start_bot(void)
{
    int_fast32_t last_update_id = 0;

    for (;;)
    {
        unset_outdated_problems();
        cJSON *updates = get_updates(last_update_id);

        if (!updates)
            continue;

        handle_updates(updates, &last_update_id);
        cJSON_Delete(updates);
    }
}

static void unset_outdated_problems(void)
{
    cJSON *outdated_problems_chat_ids = get_outdated_problems_chat_ids();

    if (outdated_problems_chat_ids)
    {
        const int outdated_problems_chat_ids_size = cJSON_GetArraySize(outdated_problems_chat_ids);

        for (int i = 0; i < outdated_problems_chat_ids_size; ++i)
        {
            const int_fast64_t chat_id = strtoll(cJSON_GetStringValue(cJSON_GetArrayItem(outdated_problems_chat_ids, i)), NULL, 10);
            unset_problem(chat_id);

            report("User %" PRIdFAST64
                   " problem automatically closed",
                   chat_id);

            send_message_with_keyboard(chat_id,
                                       EMOJI_ATTENTION " Извините, ваша проблема привысила временной лимит хранения и была закрыта\n\n"
                                       "Если вам всё ещё нужна помощь, пожалуйста, продублируйте вашу проблему!",
                                       NOKEYBOARD);
            send_message_with_keyboard(chat_id,
                                       EMOJI_BACK " Возвращаю вас в меню",
                                       get_current_keyboard(chat_id));
        }

        cJSON_Delete(outdated_problems_chat_ids);
    }
}

static void handle_updates(cJSON* updates, int_fast32_t *last_update_id)
{
    const cJSON *result = cJSON_GetObjectItem(updates, "result");
    const int result_size = cJSON_GetArraySize(result);

    for (int i = 0; i < result_size; ++i)
    {
        const cJSON *update = cJSON_GetArrayItem(result, i);
        *last_update_id = cJSON_GetNumberValue(cJSON_GetObjectItem(update, "update_id")) + 1;

        const cJSON *message = cJSON_GetObjectItem(update, "message");

        if (!message)
            continue;

        pthread_t thread;

        if (pthread_create(&thread,
                           NULL,
                           handle_message,
                           cJSON_Duplicate(message, 1)))
            die("Failed to create thread for message handle");

        pthread_detach(thread);
    }
}

static void *handle_message(void *cjson_message)
{
    cJSON *message = (cJSON *) cjson_message;

    const cJSON *chat = cJSON_GetObjectItem(message, "chat");
    const int_fast64_t chat_id = cJSON_GetNumberValue(cJSON_GetObjectItem(chat, "id"));
    const int is_root_user = (chat_id == ROOT_CHAT_ID);

    if (get_state(chat_id, "ban_state"))
    {
        if (is_root_user)
            set_state(chat_id, "ban_state", 0);
        else
        {
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, ваш аккаунт заблокирован",
                                       NOKEYBOARD);
            send_message_with_keyboard(chat_id,
                                       EMOJI_BACK " Возвращаю вас в меню",
                                       get_current_keyboard(chat_id));

            cJSON_Delete(message);
            pthread_exit(NULL);
        }
    }

    const cJSON *username = cJSON_GetObjectItem(chat, "username");
    const cJSON *text = cJSON_GetObjectItem(message, "text");

    if (get_state(chat_id, "problem_description_state"))
        handle_problem(chat_id,
                       username ? username->valuestring : NULL,
                       text ? text->valuestring : NULL);
    else
        handle_command(chat_id,
                       is_root_user,
                       username ? username->valuestring : NULL,
                       text ? text->valuestring : NULL);

    cJSON_Delete(message);
    pthread_exit(NULL);
}

static void handle_problem(const int_fast64_t chat_id, const char *username, const char *problem)
{
    if (!username)
    {
        set_state(chat_id, "problem_description_state", 0);

        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, для этой функции вам нужно "
                                   "создать имя пользователя в настройках Telegram",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Возвращаю вас в меню",
                                   get_current_keyboard(chat_id));
        return;
    }

    if (!problem)
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, я понимаю только текст",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Пожалуйста, попробуйте описать вашу проблему ещё раз",
                                   NOKEYBOARD);
        return;
    }

    if (strlen(problem) > MAX_PROBLEM_SIZE)
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, ваша проблема слишком большая",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Пожалуйста, попробуйте описать вашу проблему ещё раз",
                                   NOKEYBOARD);
        return;
    }

    char username_with_problem[MAX_USERNAME_SIZE + MAX_PROBLEM_SIZE + 4];
    sprintf(username_with_problem,
            "@%s: %s",
            username,
            problem);

    set_problem(chat_id, username_with_problem);
    set_state(chat_id, "problem_description_state", 0);

    report("User %" PRIdFAST64
           " created problem '%s'",
           chat_id,
           username_with_problem);

    send_message_with_keyboard(chat_id,
                               EMOJI_OK " Ваша проблема сохранена и будет автоматически закрыта через 30 дней\n\n"
                               "Надеюсь вам помогут как можно быстрее!",
                               NOKEYBOARD);
    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}

static void handle_command(const int_fast64_t chat_id,
                           const int is_root_user,
                           const char *username,
                           const char *command)
{
    if (!command)
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, я понимаю только текст",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Пожалуйста, попробуйте выбрать пункт в меню ещё раз",
                                   get_current_keyboard(chat_id));
        return;
    }

    if (!strcmp(command, COMMAND_BANLIST))
        handle_banlist_command(chat_id, is_root_user);
    else if (!strncmp(command, COMMAND_BAN, COMMAND_BAN_SIZE))
        handle_ban_command(chat_id,
                           is_root_user,
                           command + COMMAND_BAN_SIZE);
    else if (!strncmp(command, COMMAND_UNBAN, COMMAND_UNBAN_SIZE))
        handle_unban_command(chat_id,
                             is_root_user,
                             command + COMMAND_UNBAN_SIZE);
    else if (!strcmp(command, COMMAND_START))
        handle_start_command(chat_id, is_root_user, username);
    else if (!strcmp(command, COMMAND_HELPME))
        handle_helpme_command(chat_id, username);
    else if (!strcmp(command, COMMAND_HELPSOMEONE))
        handle_helpsomeone_command(chat_id, is_root_user);
    else if (!strcmp(command, COMMAND_CLOSEPROBLEM))
        handle_closeproblem_command(chat_id);
    else
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, я не знаю такой пункт",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Пожалуйста, попробуйте выбрать пункт в меню ещё раз",
                                   get_current_keyboard(chat_id));
    }
}

static void handle_banlist_command(const int_fast64_t chat_id, const int is_root_user)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   NOKEYBOARD);
    else
    {
        cJSON *problems = get_problems(1, 1);
        const int problems_size = cJSON_GetArraySize(problems);

        if (!problems_size)
            send_message_with_keyboard(chat_id,
                                       EMOJI_OK " Проблем заблокированных пользователей не найдено",
                                       NOKEYBOARD);
        else
        {
            for (int i = 0; i < problems_size; ++i)
                send_message_with_keyboard(chat_id,
                                           cJSON_GetStringValue(cJSON_GetArrayItem(problems, i)),
                                           NOKEYBOARD);
        }

        cJSON_Delete(problems);
    }

    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}

static void handle_ban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   NOKEYBOARD);
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, вы не указали id для блокировки",
                                       NOKEYBOARD);
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(chat_id,
                                           EMOJI_FAILED " Извините, указанный id для блокировки некорректен",
                                           NOKEYBOARD);
            else if (chat_id == target_chat_id)
                send_message_with_keyboard(chat_id,
                                           EMOJI_FAILED " Извините, вы не можете заблокировать сами себя",
                                           NOKEYBOARD);
            else if (get_state(target_chat_id, "ban_state"))
                send_message_with_keyboard(chat_id,
                                           EMOJI_FAILED " Извините, пользователь уже заблокирован",
                                           NOKEYBOARD);
            else
            {
                set_state(target_chat_id, "ban_state", 1);

                report("User %" PRIdFAST64
                       " banned user %" PRIdFAST64,
                       chat_id,
                       target_chat_id);
                send_message_with_keyboard(chat_id,
                                           EMOJI_OK " Пользователь заблокирован",
                                           NOKEYBOARD);
            }
        }
    }

    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}

static void handle_unban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   NOKEYBOARD);
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, вы не указали id для разблокировки",
                                       NOKEYBOARD);
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(chat_id,
                                           EMOJI_FAILED " Извините, указанный id для разблокировки некорректен",
                                           NOKEYBOARD);
            else if (!get_state(target_chat_id, "ban_state"))
                send_message_with_keyboard(chat_id,
                                           EMOJI_FAILED " Извините, пользователь не заблокирован",
                                           NOKEYBOARD);
            else
            {
                set_state(target_chat_id, "ban_state", 0);

                report("User %" PRIdFAST64
                       " unbanned user %" PRIdFAST64,
                       chat_id,
                       target_chat_id);
                send_message_with_keyboard(chat_id,
                                           EMOJI_OK " Пользователь разблокирован",
                                           NOKEYBOARD);
            }
        }
    }

    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}

static void handle_start_command(const int_fast64_t chat_id, const int is_root_user, const char *username)
{
    char *start_message;
    const char *user_greeting   = EMOJI_GREETING " Добро пожаловать";
    const char *bot_description = "Я создан для удобной автоматизации процесса человеческой взаимопомощи. " \
                                  "С помощью моих функций вы можете попросить о помощи, либо сами помочь кому-нибудь!";

    if (!username)
    {
        start_message = malloc(strlen(user_greeting) + strlen(bot_description) + 3);

        if (!start_message)
            die("Failed to allocate memory for start message");

        strcpy(start_message, user_greeting);
    }
    else
    {
        start_message = malloc(strlen(user_greeting) + strlen(bot_description) + strlen(username) + 6);

        if (!start_message)
            die("Failed to allocate memory for start message");

        strcpy(start_message, user_greeting);
        strcat(start_message, ", @");
        strcat(start_message, username);
    }

    strcat(start_message, "\n\n");
    strcat(start_message, bot_description);

    if (is_root_user)
    {
        const char *message_for_root_user = "\n\n" EMOJI_ATTENTION " ВЫ ЯВЛЯЕТЕСЬ АДМИНИСТРАТОРОМ" \
                                            "\n\n" EMOJI_INFO " Чтобы заблокировать пользователя, используйте команду:\n" \
                                            "/ban <id>" \
                                            "\n\n" EMOJI_INFO " Чтобы разблокировать пользователя, используйте команду:\n" \
                                            "/unban <id>" \
                                            "\n\n" EMOJI_INFO " Вместо <id> нужно указать идентификатор пользователя. "
                                            "Идентификатор находится перед проблемой пользователя в круглых скобках."
                                            "\n\n" EMOJI_INFO " Чтобы увидеть проблемы заблокированных пользователей, используйте команду:\n" \
                                            "/banlist";

        start_message = realloc(start_message, strlen(start_message) + strlen(message_for_root_user) + 1);

        if (!start_message)
            die("Failed to reallocate memory for start message");

        strcat(start_message, message_for_root_user);
    }

    send_message_with_keyboard(chat_id,
                               start_message,
                               NOKEYBOARD);
    send_message_with_keyboard(chat_id,
                               EMOJI_MENU " Пожалуйста, выберите пункт в меню",
                               get_current_keyboard(chat_id));

    free(start_message);
}

static void handle_helpme_command(const int_fast64_t chat_id, const char *username)
{
    if (has_problem(chat_id))
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, вы уже описали вашу проблему",
                                   NOKEYBOARD);
        send_message_with_keyboard(chat_id,
                                   EMOJI_BACK " Возвращаю вас в меню",
                                   get_current_keyboard(chat_id));
    }
    else
    {
        if (username)
        {
            set_state(chat_id, "problem_description_state", 1);
            send_message_with_keyboard(chat_id,
                                       EMOJI_WRITE " Пожалуйста, опишите вашу проблему",
                                       NOKEYBOARD);
        }
        else
        {
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, для этой функции вам нужно "
                                       "создать имя пользователя в настройках Telegram",
                                       NOKEYBOARD);
            send_message_with_keyboard(chat_id,
                                       EMOJI_BACK " Возвращаю вас в меню",
                                       get_current_keyboard(chat_id));
        }
    }
}

static void handle_helpsomeone_command(const int_fast64_t chat_id, const int is_root_user)
{
    cJSON *problems = get_problems(is_root_user, 0);
    const int problems_size = cJSON_GetArraySize(problems);

    if (!problems_size)
        send_message_with_keyboard(chat_id,
                                   EMOJI_OK " Пока что никто не нуждается в помощи",
                                   NOKEYBOARD);
    else
    {
        for (int i = 0; i < problems_size; ++i)
            send_message_with_keyboard(chat_id,
                                       cJSON_GetStringValue(cJSON_GetArrayItem(problems, i)),
                                       NOKEYBOARD);
    }

    cJSON_Delete(problems);
    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}

static void handle_closeproblem_command(const int_fast64_t chat_id)
{
    if (!has_problem(chat_id))
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас нет проблем для закрытия",
                                   NOKEYBOARD);
    else
    {
        unset_problem(chat_id);

        report("User %" PRIdFAST64
               " closed his problem",
               chat_id);
        send_message_with_keyboard(chat_id,
                                   EMOJI_OK " Ваша проблема закрыта\n\n"
                                   "Я очень рад, что вам смогли помочь!",
                                   NOKEYBOARD);
    }

    send_message_with_keyboard(chat_id,
                               EMOJI_BACK " Возвращаю вас в меню",
                               get_current_keyboard(chat_id));
}
