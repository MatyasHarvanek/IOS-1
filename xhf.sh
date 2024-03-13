#!/bin/bash

#TODO write error to stderr

#TODO
#List [X]
#List currency
data=""
fileName=""

export POSIXLY_CORRECT=yes
export LC_NUMERIC=C
if [ -z "$XTF_PROFIT" ]; then
    echo "debug has been set"
    export XTF_PROFIT=20
fi

echo "XTF_PROFIT is set to: $XTF_PROFIT"

writeArgumentError() {
    echo "Wrong argument syntax"
}

readData() {
    #write out error if target file does not exists
    if [ ! -f "$fileName" ]; then
        echo "Error: File '$fileName' does not exist."
        exit 1
    fi

    #unzip traget file if filename has .gz extesion or it will read raw text file
    if [[ "$fileName" == *.gz ]]; then
        data=$(gunzip -c "$fileName")
    else
        data=$(cat "$fileName")
    fi
}

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then

    echo "TODO help write"

elif [ "$1" = "list" ]; then
    #write out error if count of arguments is lower than 3
    if [ ! $# -gt 2 ]; then
        writeArgumentError
        exit 1
    fi

    fileName=$3
    readData

    #write filtered records depending on user name
    echo "$data" | awk -v var="$2" -F ';' '$1 == var'

elif [ "$1" = "list-currency" ]; then

    if [ ! $# -gt 2 ]; then
        writeArgumentError
        exit 1
    fi

    fileName=$3
    readData
    echo "$data" | awk -v var="$2" -F ';' '$1 == var && !seen[$3]++ {print $3} '

elif [ "$1" = "status" ]; then
    if [ ! $# -gt 2 ]; then
        writeArgumentError
        exit 1
    fi

    fileName=$3
    readData

    echo "$data" | awk -v var="$2" 'BEGIN { FS = ";" } $1 == var { sum[$3] += $4 } END { for (group in sum) printf "%s %.4f\n", group, sum[group] }'

elif [ "$1" = "profit" ]; then

    if [ ! $# -gt 2 ]; then
        writeArgumentError
        exit 1
    fi

    fileName=$3
    readData

    echo "$data" | awk -v var="$2" '
    BEGIN { FS = ";" }
    $1 == var {
        sum[$3] += $4 
    } 
    END {
        for (group in sum){
            if(sum[group] > 0)
            {
                sum[group] *= 1.2
            }
            printf "%s %.4f\n", group, sum[group] 
        }
    }'

elif [ "$1" = "profit" ]; then

    echo "uknown argument"

else
    #write out error if count of arguments is lower than 3
    if [ ! $# -gt 1 ]; then
        writeArgumentError
        exit 1
    fi

    fileName=$2
    readData

    #write filtered records depending on user name
    echo "$data" | awk -v var="$1" -F ';' '$1 == var'
fi
