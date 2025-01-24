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

#ifndef BOT_H
    #define BOT_H

    #define EMOJI_OK        "\U00002705"
    #define EMOJI_FAILED    "\U0000274C"
    #define EMOJI_SEARCH    "\U0001F50E"
    #define EMOJI_ATTENTION "\U00002757"
    #define EMOJI_WRITE     "\U0001F58A"
    #define EMOJI_GREETING  "\U0001F44B"
    #define EMOJI_INFO      "\U00002139"

    #define COMMAND_PENDINGLIST  "/pendinglist"
    #define COMMAND_CONFIRM      "/confirm"
    #define COMMAND_DECLINE      "/decline"
    #define COMMAND_BANLIST      "/banlist"
    #define COMMAND_BAN          "/ban"
    #define COMMAND_UNBAN        "/unban"
    #define COMMAND_START        "/start"
    #define COMMAND_CANCEL       "/cancel"
    #define COMMAND_HELPME       EMOJI_ATTENTION " Мне нужна помощь"
    #define COMMAND_HELPSOMEONE  EMOJI_SEARCH    " Помочь кому-нибудь"
    #define COMMAND_CLOSEPROBLEM EMOJI_OK        " Моя проблема решена"

    #define MAX_COMMAND_CONFIRM_SIZE 8
    #define MAX_COMMAND_DECLINE_SIZE 8
    #define MAX_COMMAND_BAN_SIZE     4
    #define MAX_COMMAND_UNBAN_SIZE   6

    #define NOKEYBOARD "{\"remove_keyboard\":true}"
    #define get_current_keyboard(chat_id) (has_problem(chat_id) ? \
                                           "{\"keyboard\":" \
                                           "[[{\"text\":\"" COMMAND_CLOSEPROBLEM "\"}," \
                                           "{\"text\":\"" COMMAND_HELPSOMEONE "\"}]]," \
                                           "\"resize_keyboard\":true}" : \
                                           "{\"keyboard\":" \
                                           "[[{\"text\":\"" COMMAND_HELPME "\"}," \
                                           "{\"text\":\"" COMMAND_HELPSOMEONE "\"}]]," \
                                           "\"resize_keyboard\":true}")

    void start_bot(const int maintenance_mode);

#endif
