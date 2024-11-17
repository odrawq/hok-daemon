##############################################################################
#                                                                            #
#   _             _                _                                         #
#  | |__    ___  | | __         __| |  __ _   ___  _ __ ___    ___   _ __    #
#  | '_ \  / _ \ | |/ / _____  / _` | / _` | / _ \| '_ ` _ \  / _ \ | '_ \   #
#  | | | || (_) ||   < |_____|| (_| || (_| ||  __/| | | | | || (_) || | | |  #
#  |_| |_| \___/ |_|\_\        \__,_| \__,_| \___||_| |_| |_| \___/ |_| |_|  #
#                                                                            #
# Copyright (C) 2024 qwardo <odrawq.qwardo@gmail.com>                        #
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

TARGET          = hok-daemon
CC              = gcc
CFLAGS          = -Wall -Wextra -O2 -Iinclude/ -lpthread -lcurl -lcjson
SRC_DIR         = src/
BUILD_DIR       = build/
BIN_DIR         = /usr/local/bin/
DATA_DIR        = /var/lib/$(TARGET)/
LOG_DIR         = /var/log/$(TARGET)/
USERS_DATA_FILE = users_data.json
ERROR_LOG_FILE  = error_log
INFO_LOG_FILE   = info_log

SRC_FILES := $(wildcard $(SRC_DIR)*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)%.c,$(BUILD_DIR)%.o,$(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

.PHONY: build clean install uninstall

build: $(BUILD_DIR)$(TARGET)

$(BUILD_DIR)$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(CFLAGS)

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $< -o $@ -MMD $(CFLAGS)

-include $(DEP_FILES)

clean:
	rm -rf $(BUILD_DIR)

install:
	sudo cp $(BUILD_DIR)$(TARGET) $(BIN_DIR)
	sudo mkdir -p $(LOG_DIR)
	sudo touch $(LOG_DIR)$(ERROR_LOG_FILE)
	sudo touch $(LOG_DIR)$(INFO_LOG_FILE)
	sudo mkdir -p $(DATA_DIR)

	@if ! sudo test -f $(DATA_DIR)$(USERS_DATA_FILE); then \
		sudo touch $(DATA_DIR)$(USERS_DATA_FILE); \
		echo "{}" | sudo tee $(DATA_DIR)$(USERS_DATA_FILE) > /dev/null; \
	fi

	@if ! id -u $(TARGET) >/dev/null 2>&1; then \
		sudo useradd -r -s /bin/false $(TARGET); \
	fi

	sudo chown -R $(TARGET):$(TARGET) $(LOG_DIR) $(DATA_DIR)
	sudo chmod 700 $(LOG_DIR) $(DATA_DIR)

uninstall:
	sudo rm $(BIN_DIR)$(TARGET)
	sudo rm -rf $(LOG_DIR) $(DATA_DIR)
	sudo userdel $(TARGET)
