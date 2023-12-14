#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Kullanim: $0 <1|2|3>"
    exit 1
fi

if [ "$1" -eq 1 ]; then
    gcc server.c -o server 
    echo "Server derlendi"
elif [ "$1" -eq 2 ]; then
    gcc client.c -o client
    echo "Client derlendi"
elif [ "$1" -eq 3 ]; then
    gcc server.c -o server 
    gcc client.c -o client
    echo "Her ikisi de derlendi"
else
    echo "Geçersiz parametre. 1, 2 veya 3 giriniz."
fi