##############################################################################
#                                                                            #
#   _             _                _                                         #
#  | |__    ___  | | __         __| |  __ _   ___  _ __ ___    ___   _ __    #
#  | '_ \  / _ \ | |/ / _____  / _` | / _` | / _ \| '_ ` _ \  / _ \ | '_ \   #
#  | | | || (_) ||   < |_____|| (_| || (_| ||  __/| | | | | || (_) || | | |  #
#  |_| |_| \___/ |_|\_\        \__,_| \__,_| \___||_| |_| |_| \___/ |_| |_|  #
#                                                                            #
# Copyright (C) 2024-2025 qwardo <odrawq.qwardo@gmail.com>                   #
#                                                                            #
# This file is part of hok-daemon.                                           #
#                                                                            #
# hok-daemon is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by       #
# the Free Software Foundation, either version 3 of the License, or          #
# (at your option) any later version.                                        #
#                                                                            #
# hok-daemon is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of             #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               #
# GNU General Public License for more details.                               #
#                                                                            #
# You should have received a copy of the GNU General Public License          #
# along with hok-daemon. If not, see <http://www.gnu.org/licenses/>.         #
#                                                                            #
##############################################################################

TARGET         = hok-daemon
CC             = gcc
CFLAGS         = -Wall -Wextra -O2 -Iinclude/ -lpthread -lcurl -lcjson
SRC_DIR        = src/
BUILD_DIR      = build/
BIN_DIR        = /usr/local/bin/
DATA_DIR       = /var/lib/$(TARGET)/
LOG_DIR        = /var/log/$(TARGET)/
USERS_FILE     = users.json
ERROR_LOG_FILE = error_log
INFO_LOG_FILE  = info_log

SHELL := /usr/bin/env bash

SRC_FILES := $(wildcard $(SRC_DIR)*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)%.c, $(BUILD_DIR)%.o, $(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

.PHONY := build clean install uninstall purge

build: $(BUILD_DIR) $(BUILD_DIR)$(TARGET)

$(BUILD_DIR):
	@echo -e "\e[0;33;1mBuilding $(TARGET)...\e[0m"
	mkdir -p $@

$(BUILD_DIR)$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(CFLAGS)
	@echo -e "\e[0;32;1mBuilding done!\n$$($(BUILD_DIR)$(TARGET) -v) on $$(uname -mo)\e[0m"

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) -c $< -o $@ -MMD $(CFLAGS)

-include $(DEP_FILES)

clean:
	@echo -e "\e[0;33;1mCleaning build files...\e[0m"
	rm -rf $(BUILD_DIR)
	@echo -e "\e[0;32;1mCleaning done!\e[0m"

install:
	@echo -e "\e[0;33;1mInstalling $(TARGET)...\e[0m"
	sudo cp $(BUILD_DIR)$(TARGET) $(BIN_DIR)

	@echo -e "\e[0;33;1mCreating $(TARGET) files...\e[0m"

	sudo mkdir -p $(LOG_DIR)
	sudo touch $(LOG_DIR)$(ERROR_LOG_FILE) $(LOG_DIR)$(INFO_LOG_FILE)

	sudo mkdir -p $(DATA_DIR)

	if ! sudo test -f $(DATA_DIR)$(USERS_FILE); then \
		sudo touch $(DATA_DIR)$(USERS_FILE); \
		echo "{}" | sudo tee $(DATA_DIR)$(USERS_FILE) > /dev/null; \
	fi

	@echo -e "\e[0;33;1mCreating $(TARGET) user...\e[0m"

	if ! id -u $(TARGET) > /dev/null 2>&1; then \
		sudo useradd -r -s /bin/false $(TARGET); \
	fi

	@echo -e "\e[0;33;1mSetting access permissions...\e[0m"

	sudo chown -R $(TARGET):$(TARGET) $(LOG_DIR) $(DATA_DIR)
	sudo chmod 700 $(LOG_DIR) $(DATA_DIR)

	@echo -e "\e[0;32;1mInstallation done!\e[0m"

uninstall:
	@echo -e "\e[0;33;1mDeleting $(TARGET)...\e[0m"
	sudo rm -f $(BIN_DIR)$(TARGET)
	@echo -e "\e[0;32;1mUninstallation done!\e[0m"

purge: uninstall
	@echo -e "\e[0;33;1mDeleting $(TARGET) files...\e[0m"
	sudo rm -rf $(LOG_DIR) $(DATA_DIR)

	@echo -e "\e[0;33;1mDeleting $(TARGET) user...\e[0m"

	if id -u $(TARGET) > /dev/null 2>&1; then \
		sudo userdel $(TARGET); \
	fi

	@echo -e "\e[0;32;1mPurging done!\e[0m"
