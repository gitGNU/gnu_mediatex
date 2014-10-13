#!/bin/bash
#=======================================================================
# * Version: $Id: env.sh,v 1.1 2014/10/13 19:39:28 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * script called by command.c within its unit test context
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014  Nicolas Roche
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

echo "** This script show the MediaTex environment variables:"

echo "* Parameters:"
echo "MDTX_MDTXUSER      = $MDTX_MDTXUSER"
echo "MDTX_LOG_FILE      = $MDTX_LOG_FILE"
echo "MDTX_LOG_FACILITY  = $MDTX_LOG_FACILITY"
echo "MDTX_LOG_SEVERITY  = $MDTX_LOG_SEVERITY"
echo "MDTX_DRY_RUN       = $MDTX_DRY_RUN"
echo "PARAMETER_1        = $1"

