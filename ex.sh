#!/bin/bash
LOG_DIR=/var/log

ROOT_UID=0
LINES=50
E_XCD=86
E_NOTROOT=87
if [ "$UID" -ne "$ROOT_UID" ] 
then
    echo "Run as root, pleeeease"
      exit $E_NOTROOT
    fi
    if [ -n "$1" ]
    then
        lines=$1
      else
          lines=$LINES
        fi  
        E_WRONGARGS=85
        case "$1" in
            ""   ) lines=50;;
              *[!0-9]*) echo "Usage:` basename $0` lines-to-cleanup";
                    exit $E_WRONGARGS;;
                      *    ) lines=$1;;
                    esac
                    cd $LOG_DIR

                    cat /dev/null > messages
                    cat /dev/null > wtmp
                    echo "Logs cleaned up"
                    exit 


        esac
    fi
fi

