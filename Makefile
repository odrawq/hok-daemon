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

SHELL := /usr/bin/env bash

TARGET := hok-daemon

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -Iinclude/ -MMD
LDFLAGS := -lpthread -lcurl -lcjson

INIT_DIR    := init/
SRC_DIR     := src/
BUILD_DIR   := build/
SYSTEMD_DIR := /etc/systemd/system/
BIN_DIR     := /usr/local/bin/
LOG_DIR     := /var/log/
DATA_DIR    := /var/lib/

USERS_FILE     := users.json
INFO_LOG_FILE  := info_log
ERROR_LOG_FILE := error_log

OBJ_FILES := $(patsubst $(SRC_DIR)%.c, $(BUILD_DIR)%.o, $(wildcard $(SRC_DIR)*.c))

build: $(BUILD_DIR) $(BUILD_DIR)$(TARGET)

$(BUILD_DIR):
	@echo -e '\e[0;33;1mBuilding $(TARGET)...\e[0m'

	mkdir -p $@

$(BUILD_DIR)$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS)

	@echo -e "\e[0;32;1mBuilding done!\n$$($(BUILD_DIR)$(TARGET) -v) on $$(uname -mo)\e[0m"

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) -c $< -o $@ $(CFLAGS)

-include $(OBJ_FILES:.o=.d)

clean:
	@echo -e '\e[0;33;1mCleaning build files...\e[0m'

	rm -rf $(BUILD_DIR)

	@echo -e '\e[0;32;1mCleaning done!\e[0m'

install:
	@echo -e '\e[0;33;1mInstalling $(TARGET)...\e[0m'

	sudo cp $(BUILD_DIR)$(TARGET) $(BIN_DIR)

	@echo -e '\e[0;33;1mCreating $(TARGET) files...\e[0m'

	sudo mkdir -p $(LOG_DIR)$(TARGET)/ $(DATA_DIR)$(TARGET)/
	sudo touch $(LOG_DIR)$(TARGET)/$(INFO_LOG_FILE) $(LOG_DIR)$(TARGET)/$(ERROR_LOG_FILE)
	sudo test -f $(DATA_DIR)$(TARGET)/$(USERS_FILE) || echo '{}' | sudo tee $(DATA_DIR)$(TARGET)/$(USERS_FILE) > /dev/null

	@echo -e '\e[0;33;1mCreating $(TARGET) user...\e[0m'

	id -u $(TARGET) > /dev/null 2>&1 || sudo useradd -Mrs /bin/false $(TARGET)

	@echo -e '\e[0;33;1mSetting access permissions...\e[0m'

	sudo chown -R $(TARGET):$(TARGET) $(LOG_DIR)$(TARGET)/ $(DATA_DIR)$(TARGET)/
	sudo chmod 700 $(LOG_DIR)$(TARGET)/ $(DATA_DIR)$(TARGET)/

	@echo -e '\e[0;33;1mChecking for systemd...\e[0m'

	which systemctl > /dev/null 2>&1 && sudo cp $(INIT_DIR)systemd/*.service $(SYSTEMD_DIR) && sudo systemctl daemon-reload || true

	@echo -e '\e[0;32;1mInstallation done!\e[0m'

uninstall:
	@echo -e '\e[0;33;1mDeleting $(TARGET)...\e[0m'

	sudo rm -f $(BIN_DIR)$(TARGET)

	@echo -e '\e[0;32;1mUninstallation done!\e[0m'

purge: uninstall
	@echo -e '\e[0;33;1mDeleting $(TARGET) files...\e[0m'

	sudo rm -rf $(LOG_DIR)$(TARGET)/ $(DATA_DIR)$(TARGET)/

	@echo -e '\e[0;33;1mDeleting $(TARGET) user...\e[0m'

	id -u $(TARGET) > /dev/null 2>&1 && sudo userdel $(TARGET) || true

	@echo -e '\e[0;33;1mChecking for systemd...\e[0m'

	which systemctl > /dev/null 2>&1 && sudo rm -f $(SYSTEMD_DIR)$(TARGET)*.service && sudo systemctl daemon-reload || true

	@echo -e '\e[0;32;1mPurging done!\e[0m'

.PHONY := build clean install uninstall purge
