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
static void handle_updates(cJSON *updates, int_fast32_t *last_update_id, const int maintenance_mode);
static void *handle_message_in_maintenance_mode(void *cjson_message);
static void *handle_message(void *cjson_message);
static void handle_problem(const int_fast64_t chat_id,
                           const int is_root_user,
                           const char *username,
                           const char *problem);
static void handle_command(const int_fast64_t chat_id,
                           const int is_root_user,
                           const char *username,
                           const char *command);
static void handle_pendinglist_command(const int_fast64_t chat_id, const int is_root_user);
static void handle_confirm_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_decline_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_banlist_command(const int_fast64_t chat_id, const int is_root_user);
static void handle_ban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_unban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg);
static void handle_start_command(const int_fast64_t chat_id, const int is_root_user, const char *username);
static void handle_helpme_command(const int_fast64_t chat_id, const char *username);
static void handle_helpsomeone_command(const int_fast64_t chat_id, const int is_root_user);
static void handle_closeproblem_command(const int_fast64_t chat_id);

void start_bot(const int maintenance_mode)
{
    int_fast32_t last_update_id = 0;

    for (;;)
    {
        if (!maintenance_mode)
            unset_outdated_problems();

        cJSON *updates = get_updates(last_update_id);

        if (!updates)
            continue;

        handle_updates(updates, &last_update_id, maintenance_mode);
        cJSON_Delete(updates);
    }
}

static void unset_outdated_problems(void)
{
    cJSON *outdated_problems_chat_ids = get_outdated_problems_chat_ids();

    if (!outdated_problems_chat_ids)
        return;

    const int outdated_problems_chat_ids_size = cJSON_GetArraySize(outdated_problems_chat_ids);

    for (int i = 0; i < outdated_problems_chat_ids_size; ++i)
    {
        const int_fast64_t chat_id = strtoll(cJSON_GetStringValue(cJSON_GetArrayItem(outdated_problems_chat_ids, i)), NULL, 10);
        unset_problem(chat_id);

        report("User %" PRIdFAST64
               " problem exceeded the storage time limit and automatically closed",
               chat_id);

        send_message_with_keyboard(chat_id,
                                   EMOJI_ATTENTION " Извините, ваша проблема привысила временной лимит хранения и была закрыта\n\n"
                                   "Если вам всё ещё нужна помощь, пожалуйста, опишите вашу проблему ещё раз!",
                                   get_current_keyboard(chat_id));
    }

    cJSON_Delete(outdated_problems_chat_ids);
}

static void handle_updates(cJSON *updates, int_fast32_t *last_update_id, const int maintenance_mode)
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
                           maintenance_mode ? handle_message_in_maintenance_mode : handle_message,
                           cJSON_Duplicate(message, 1)))
            die("Failed to create thread for message handle");

        pthread_detach(thread);
    }
}

static void *handle_message_in_maintenance_mode(void *cjson_message)
{
    cJSON *message = (cJSON *) cjson_message;

    const int_fast64_t chat_id = cJSON_GetNumberValue(cJSON_GetObjectItem(cJSON_GetObjectItem(message, "chat"), "id"));

    send_message_with_keyboard(chat_id,
                               EMOJI_FAILED " Извините, бот временно недоступен\n\n"
                               "Проводятся технические работы. Пожалуйста, ожидайте!",
                               "");

    cJSON_Delete(message);
    pthread_exit(NULL);
}

static void *handle_message(void *cjson_message)
{
    cJSON *message = (cJSON *) cjson_message;

    const cJSON *chat = cJSON_GetObjectItem(message, "chat");
    const int_fast64_t chat_id = cJSON_GetNumberValue(cJSON_GetObjectItem(chat, "id"));

    const int is_root_user = (chat_id == ROOT_CHAT_ID);

    if (get_state(chat_id, "account_ban_state"))
    {
        if (is_root_user)
            set_state(ROOT_CHAT_ID, "account_ban_state", 0);
        else
        {
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, ваш аккаунт заблокирован",
                                       "");

            cJSON_Delete(message);
            pthread_exit(NULL);
        }
    }

    const cJSON *username = cJSON_GetObjectItem(chat, "username");
    const cJSON *text = cJSON_GetObjectItem(message, "text");

    if (get_state(chat_id, "problem_description_state"))
        handle_problem(chat_id,
                       is_root_user,
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

static void handle_problem(const int_fast64_t chat_id,
                           const int is_root_user,
                           const char *username,
                           const char *problem)
{
    if (!problem)
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, я понимаю только текст",
                                   "");
        return;
    }

    if (!strcmp(problem, COMMAND_CANCEL))
    {
        set_state(chat_id, "problem_description_state", 0);
        send_message_with_keyboard(chat_id,
                                   EMOJI_OK " Описание проблемы отменено",
                                   get_current_keyboard(chat_id));
        return;
    }

    if (!username)
    {
        set_state(chat_id, "problem_description_state", 0);
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, для этой функции вам нужно "
                                   "создать имя пользователя в настройках Telegram",
                                   get_current_keyboard(chat_id));
        return;
    }

    if (strlen(problem) > MAX_PROBLEM_SIZE)
    {
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, ваша проблема слишком большая",
                                   "");
        return;
    }

    char username_with_problem[MAX_USERNAME_SIZE + MAX_PROBLEM_SIZE + 4];
    sprintf(username_with_problem,
            "@%s: %s",
            username,
            problem);

    set_state(chat_id, "problem_description_state", 0);
    set_problem(chat_id, username_with_problem);

    report("User %" PRIdFAST64
           " created problem '%s'",
           chat_id,
           username_with_problem);

    if (get_state(chat_id, "problem_pending_state"))
    {
        if (is_root_user)
            set_state(ROOT_CHAT_ID, "problem_pending_state", 0);
        else
        {
            send_message_with_keyboard(chat_id,
                                       EMOJI_INFO " Перед публикацией ваша проблема должна пройти проверку\n\n"
                                       "Пожалуйста, ожидайте!",
                                       get_current_keyboard(chat_id));
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_INFO " Появилась новая проблема для проверки",
                                       "");
           return;
        }
    }

    send_message_with_keyboard(chat_id,
                               EMOJI_OK " Ваша проблема сохранена и будет автоматически закрыта через 21 день\n\n"
                               "Надеюсь вам помогут как можно быстрее!",
                               get_current_keyboard(chat_id));
    send_message_with_keyboard(ROOT_CHAT_ID,
                               EMOJI_INFO " Появилась новая проблема",
                               "");
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
                                   "");
        return;
    }

    if (!strcmp(command, COMMAND_PENDINGLIST))
        handle_pendinglist_command(chat_id, is_root_user);
    else if (!strncmp(command, COMMAND_CONFIRM, MAX_COMMAND_CONFIRM_SIZE))
        handle_confirm_command(chat_id,
                               is_root_user,
                               command + MAX_COMMAND_CONFIRM_SIZE);
    else if (!strncmp(command, COMMAND_DECLINE, MAX_COMMAND_DECLINE_SIZE))
        handle_decline_command(chat_id,
                               is_root_user,
                               command + MAX_COMMAND_DECLINE_SIZE);
    else if (!strcmp(command, COMMAND_BANLIST))
        handle_banlist_command(chat_id, is_root_user);
    else if (!strncmp(command, COMMAND_BAN, MAX_COMMAND_BAN_SIZE))
        handle_ban_command(chat_id,
                           is_root_user,
                           command + MAX_COMMAND_BAN_SIZE);
    else if (!strncmp(command, COMMAND_UNBAN, MAX_COMMAND_UNBAN_SIZE))
        handle_unban_command(chat_id,
                             is_root_user,
                             command + MAX_COMMAND_UNBAN_SIZE);
    else if (!strcmp(command, COMMAND_START))
        handle_start_command(chat_id, is_root_user, username);
    else if (!strcmp(command, COMMAND_HELPME))
        handle_helpme_command(chat_id, username);
    else if (!strcmp(command, COMMAND_HELPSOMEONE))
        handle_helpsomeone_command(chat_id, is_root_user);
    else if (!strcmp(command, COMMAND_CLOSEPROBLEM))
        handle_closeproblem_command(chat_id);
    else if (!strcmp(command, COMMAND_CANCEL))
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, вы не описываете проблему",
                                   "");
    else
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, я не знаю такого действия",
                                   "");
}

static void handle_pendinglist_command(const int_fast64_t chat_id, const int is_root_user)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        cJSON *problems = get_problems(1, 0, 1);
        const int problems_size = cJSON_GetArraySize(problems);

        if (!problems_size)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_OK " Проблем на удержании не найдено",
                                       "");
        else
            for (int i = 0; i < problems_size; ++i)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           cJSON_GetStringValue(cJSON_GetArrayItem(problems, i)),
                                           i + 1 != problems_size ? NOKEYBOARD : get_current_keyboard(ROOT_CHAT_ID));

        cJSON_Delete(problems);
    }
}

static void handle_confirm_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_FAILED " Извините, вы не указали id",
                                       "");
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, указанный id некорректен",
                                           "");
            else if (!has_problem(target_chat_id))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, у пользователя нет проблемы",
                                           "");
            else if (!get_state(target_chat_id, "problem_pending_state"))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, проблема не удерживается",
                                           "");
            else
            {
                set_state(target_chat_id, "problem_pending_state", 0);

                report("User %" PRIdFAST64
                       " confirmed user %" PRIdFAST64
                       " problem",
                       ROOT_CHAT_ID,
                       target_chat_id);

                send_message_with_keyboard(target_chat_id,
                                           EMOJI_OK " Ваша проблема снята с удержания и будет автоматически закрыта через 21 день\n\n"
                                           "Надеюсь вам помогут как можно быстрее!",
                                           "");
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_OK " Проблема снята с удержания",
                                           "");
            }
        }
    }
}

static void handle_decline_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_FAILED " Извините, вы не указали id",
                                       "");
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, указанный id некорректен",
                                           "");
            else if (!has_problem(target_chat_id))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, у пользователя нет проблемы",
                                           "");
            else if (!get_state(target_chat_id, "problem_pending_state"))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, проблема не удерживается",
                                           "");
            else
            {
                unset_problem(target_chat_id);

                report("User %" PRIdFAST64
                       " declined user %" PRIdFAST64
                       " problem",
                       ROOT_CHAT_ID,
                       target_chat_id);

                send_message_with_keyboard(target_chat_id,
                                           EMOJI_FAILED " Извините, ваша проблема отклонена\n\n"
                                           "Пожалуйста, попробуйте описать вашу проблему ещё раз!",
                                           get_current_keyboard(target_chat_id));
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_OK " Проблема отклонена",
                                           "");
            }
        }
    }
}

static void handle_banlist_command(const int_fast64_t chat_id, const int is_root_user)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        cJSON *problems = get_problems(1, 1, 0);
        const int problems_size = cJSON_GetArraySize(problems);

        if (!problems_size)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_OK " Проблем заблокированных пользователей не найдено",
                                       "");
        else
            for (int i = 0; i < problems_size; ++i)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           cJSON_GetStringValue(cJSON_GetArrayItem(problems, i)),
                                           i + 1 != problems_size ? NOKEYBOARD : get_current_keyboard(ROOT_CHAT_ID));

        cJSON_Delete(problems);
    }
}

static void handle_ban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_FAILED " Извините, вы не указали id",
                                       "");
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, указанный id некорректен",
                                           "");
            else if (target_chat_id == ROOT_CHAT_ID)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, вы не можете заблокировать сами себя",
                                           "");
            else if (get_state(target_chat_id, "account_ban_state"))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, пользователь уже заблокирован",
                                           "");
            else
            {
                set_state(target_chat_id, "account_ban_state", 1);

                if (get_state(target_chat_id, "problem_pending_state"))
                    set_state(target_chat_id, "problem_pending_state", 0);

                report("User %" PRIdFAST64
                       " banned user %" PRIdFAST64,
                       ROOT_CHAT_ID,
                       target_chat_id);

                send_message_with_keyboard(target_chat_id,
                                           EMOJI_ATTENTION " Вы были заблокированы",
                                           "");
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_OK " Пользователь заблокирован",
                                           "");
            }
        }
    }
}

static void handle_unban_command(const int_fast64_t chat_id, const int is_root_user, const char *arg)
{
    if (!is_root_user)
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас недостаточно прав",
                                   "");
    else
    {
        while (*arg == ' ')
            ++arg;

        if (!*arg)
            send_message_with_keyboard(ROOT_CHAT_ID,
                                       EMOJI_FAILED " Извините, вы не указали id",
                                       "");
        else
        {
            char *end;
            const int_fast64_t target_chat_id = strtoll(arg, &end, 10);

            if (*end || end == arg)
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, указанный id некорректен",
                                           "");
            else if (!get_state(target_chat_id, "account_ban_state"))
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_FAILED " Извините, пользователь не заблокирован",
                                           "");
            else
            {
                if (has_problem(target_chat_id))
                    unset_problem(target_chat_id);

                set_state(target_chat_id, "problem_pending_state", 1);
                set_state(target_chat_id, "account_ban_state", 0);

                report("User %" PRIdFAST64
                       " unbanned user %" PRIdFAST64,
                       ROOT_CHAT_ID,
                       target_chat_id);

                send_message_with_keyboard(target_chat_id,
                                           EMOJI_OK " Вы были разблокированы",
                                           get_current_keyboard(target_chat_id));
                send_message_with_keyboard(ROOT_CHAT_ID,
                                           EMOJI_OK " Пользователь разблокирован",
                                           "");
            }
        }
    }
}

static void handle_start_command(const int_fast64_t chat_id, const int is_root_user, const char *username)
{
    char *start_message;
    const char *user_greeting   = EMOJI_GREETING " Добро пожаловать";
    const char *bot_description = "Оказывайте поддержку и помощь другим, а также получайте решения собственных проблем!";

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
        const char *message_for_root_user = "\n\n" EMOJI_ATTENTION " ВЫ ЯВЛЯЕТЕСЬ АДМИНИСТРАТОРОМ"
                                            "\n\n" EMOJI_INFO " Вывести проблемы на удержании\n"
                                            "/pendinglist"
                                            "\n\n" EMOJI_INFO " Снять проблему с удержания\n"
                                            "/confirm <id>"
                                            "\n\n" EMOJI_INFO " Отклонить проблему на удержании\n"
                                            "/decline <id>"
                                            "\n\n" EMOJI_INFO " Вывести проблемы заблокированных пользователей\n"
                                            "/banlist"
                                            "\n\n" EMOJI_INFO " Заблокировать пользователя\n"
                                            "/ban <id>"
                                            "\n\n" EMOJI_INFO " Разблокировать пользователя\n"
                                            "/unban <id>"
                                            "\n\nВместо <id> нужно указать идентификатор пользователя. "
                                            "Идентификатор находится перед проблемой пользователя в круглых скобках.";

        start_message = realloc(start_message, strlen(start_message) + strlen(message_for_root_user) + 1);

        if (!start_message)
            die("Failed to reallocate memory for start message");

        strcat(start_message, message_for_root_user);
    }

    send_message_with_keyboard(chat_id,
                               start_message,
                               get_current_keyboard(chat_id));

    free(start_message);
}

static void handle_helpme_command(const int_fast64_t chat_id, const char *username)
{
    if (has_problem(chat_id))
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, вы уже описали вашу проблему",
                                   "");
    else
    {
        if (!username)
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, для этой функции вам нужно "
                                       "создать имя пользователя в настройках Telegram",
                                       "");
        else
        {
            set_state(chat_id, "problem_description_state", 1);
            send_message_with_keyboard(chat_id,
                                       EMOJI_WRITE " Пожалуйста, опишите вашу проблему (для отмены - /cancel)",
                                       NOKEYBOARD);
        }
    }
}

static void handle_helpsomeone_command(const int_fast64_t chat_id, const int is_root_user)
{
    cJSON *problems = get_problems(is_root_user, 0, 0);
    const int problems_size = cJSON_GetArraySize(problems);

    if (!problems_size)
        send_message_with_keyboard(chat_id,
                                   EMOJI_OK " Пока что никто не нуждается в помощи",
                                   "");
    else
        for (int i = 0; i < problems_size; ++i)
            send_message_with_keyboard(chat_id,
                                       cJSON_GetStringValue(cJSON_GetArrayItem(problems, i)),
                                       i + 1 != problems_size ? NOKEYBOARD : get_current_keyboard(chat_id));

    cJSON_Delete(problems);
}

static void handle_closeproblem_command(const int_fast64_t chat_id)
{
    if (!has_problem(chat_id))
        send_message_with_keyboard(chat_id,
                                   EMOJI_FAILED " Извините, у вас нет проблем для закрытия",
                                   "");
    else
    {
        if (get_state(chat_id, "problem_pending_state"))
            send_message_with_keyboard(chat_id,
                                       EMOJI_FAILED " Извините, вы не можете закрыть проблему, которая находится на удержании",
                                       "");
        else
        {
            unset_problem(chat_id);

            report("User %" PRIdFAST64
                   " closed his problem",
                   chat_id);

            send_message_with_keyboard(chat_id,
                                       EMOJI_OK " Ваша проблема закрыта\n\n"
                                       "Я очень рад, что ваша проблема решена!",
                                       get_current_keyboard(chat_id));
        }
    }
}
