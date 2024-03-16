#!/bin/bash
name='jadziaro'

# Получить текущую дату и время в формате ДД.ММ.ГГ чч.мм
time=$(date +'%d.%m.%y %H.%M')

# Получить сообщение для коммита из аргументов скрипта
msg="$@"

# Выполнить команду git commit
git add -A
git commit -m "$name: $time: $msg"
git push origin develop

