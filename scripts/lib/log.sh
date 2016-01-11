#!/bin/bash
#=======================================================================
# * Version: $Id: log.sh,v 1.5 2015/08/09 13:20:34 nroche Exp $
# * Project: MediaTex
# * Module : script libs
# *
# * Provide log facilities
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 Nicolas Roche
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#=======================================================================
#set -x
set -e

# includes
MDTX_SH_LOG=1

# print a log message:
# $1: severity 
# $2: message
# $3: optional indirection number : 
## =0: ${BASH_SOURCE[1]} ${BASH_LINENO[0]}
## =1: ${BASH_SOURCE[2]} ${BASH_LINENO[1]}
## =2: ${BASH_SOURCE[3]} ${BASH_LINENO[2]}
## ...
function Log()
{
    level=${3-0}
    level=$[$level+${ONLY_ROOT_LOG-0}]

    if [ "x$LOG_FACILITY" = "xfile" ]; then
	if [ -z $LOG_FILE ]; then
	    printf "[%s %s:%i] %s\n" $1 \
		${BASH_SOURCE[$[$level+1]]##*/} ${BASH_LINENO[$level]} \
		"$2" >&2

	else
	    printf "[%s %s:%i] %s\n" $1 \
		${BASH_SOURCE[$[$level+1]]##*/} ${BASH_LINENO[$level]} \
		"$2" >>$LOG_FILE
	fi
    else
	logger -t "script[$$]" -p "${LOG_FACILITY-local2}.$1" \
	    "[$1 ${BASH_SOURCE[$[$level+1]]##*/}:${BASH_LINENO[$level]}] $2"
    fi
}

# print an error message on the standard error stream:
# $1: message
# $2: optional indirection number
Error()
{
    Log "err" "$1" ${2-1}
    exit 2
}

# print an warning message on the standard error stream:
# $1: message
# $2: optional indirection number
Warning()
{
    case ${LOG_SEVERITY-info} in
	error) return 0;;
    esac
    
    Log "warn" "$1" ${2-1}
}

# print an information message on the standard error stream:
# $1: message
# $2: optional indirection number
Info()
{
    case ${LOG_SEVERITY-info} in
	error|warning|notice) return 0;;
    esac

    Log "info" "$1" ${2-1}
}

# print a debug message on the standard error stream:
# $1: message
# $2: optional indirection number
Debug()
{
   case ${LOG_SEVERITY-info} in
       error|warning|notice|info) return 0;;
   esac
    Log "debug" "$1" ${2-1}
}

# # this function allow non root user to make some checks
# # $@: the function to call followed by its arguments
# function only_root()
# {
#     if [ $(id -u) -ne 0 ]; then
# 	Warning "need to be root to call the \"$1\" function" 2
#     else
# 	ONLY_ROOT_LOG=1
# 	eval $@
# 	ONLY_ROOT_LOG=0
#     fi
# }

# main
LOG_FILE=${MDTX_LOG_FILE-""}
LOG_FACILITY=${MDTX_LOG_FACILITY-file}
LOG_SEVERITY=${MDTX_LOG_SEVERITY_SCRIPT-info}

Info "(script) LOG_SEVERITY = $LOG_SEVERITY"
