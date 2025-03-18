# hok-daemon
Полностью бесплатное и свободное программное обеспечение с открытым исходным кодом, являющееся демоном для управления Телеграм-ботом "Руки Добра".

hok-daemon сохраняет и систематизирует проблемы нуждающихся в помощи людей для того, чтобы другие могли им помочь.
## Требования
- GNU/Linux дистрибутив;
- Доступ к интернету.
>**⚠️ Важное замечание**
>
>hok-daemon разрабатывается исключительно под GNU/Linux дистрибутивы. Сборка на других Unix-подобных системах возможна, но корректная работа не гарантируется. Кодовая база hok-daemon полностью несовместима с Windows и сборка под эту систему невозможна.
## Зависимости
- git;
- make;
- gcc;
- libcurl;
- libcjson.
## Начальная конфигурация
1. Клонировать репозиторий:
```bash
git clone https://github.com/odrawq/hok-daemon.git
```
2. Перейти в репозиторий:
```bash
cd hok-daemon/
```
3. Запустить конфигурационный скрипт:
```bash
./configure.sh
```
Сначала надо ввести токен Телеграм-бота, которым будет управлять hok-daemon. Для того чтобы создать Телеграм-бота и получить его токен, воспользуйтесь Телеграм-ботом [BotFather](https://t.me/BotFather).

Затем надо ввести идентификатор пользователя, который будет администратором вашего Телеграм-бота. Для того чтобы узнать свой идентификатор, воспользуйтесь Телеграм-ботом [userinfobot](https://t.me/userinfobot).

После того как конфигурация завершится, потом в любое время вы сможете поменять введённые вами значения, выполнив конфигурационный скрипт ещё раз. Если вам нужно поменять только одно конкретное значение, вводить всё заново не нужно. Достаточно просто нажать Enter, тем самым будет использовано старое значение.
## Сборка, установка и запуск
1. Собрать hok-daemon:
```bash
make
```
2. Установить hok-daemon:
```bash
make install
```
3. Запустить hok-daemon:
- Нативный запуск:
```bash
sudo -u hok-daemon hok-daemon
```
- Запуск через systemd:
```bash
sudo systemctl start hok-daemon
```
## Режим обслуживания
Для проведения технических работ запустите hok-daemon в режиме обслуживания. В этом режиме hok-daemon будет отправлять пользователям сообщение о проведении технических работ:
- Нативный запуск:
```bash
sudo -u hok-daemon hok-daemon -m
```
- Запуск через systemd:
```bash
sudo systemctl start hok-daemon-maintenance
```
## Системы инициализации
Если вы запускаете hok-daemon через системы инициализации, то при критических ошибках hok-daemon будет сам перезапускаться в режиме обслуживания. Также вы можете легко добавить hok-daemon в автозапуск. При нативном запуске вы должны сами следить за hok-daemon.
## Очистка
Удалить файлы после сборки:
```bash
make clean
```
## Удаление
- Удалить hok-daemon:
```bash
make uninstall
```
- Удалить hok-daemon вместе со всеми данными:
```bash
make purge
```
## Логи и данные
- Файлы для запуска через systemd находятся по путям /etc/systemd/system/hok-daemon.service и /etc/systemd/system/hok-daemon-maintenance.service;
- Файл с логом обычной информации находится по пути /var/log/hok-daemon/info_log;
- Файл с логом ошибок находится по пути /var/log/hok-daemon/error_log;
- Файл с данными пользователей находится по пути /var/lib/hok-daemon/users.json.
## Лицензия
Этот проект лицензирован по лицензии GNU General Public License v3.0 (GPL-3.0) - подробности в файле [LICENSE](LICENSE).
