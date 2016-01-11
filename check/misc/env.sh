#!/bin/bash
#=======================================================================
# * Version: $Id: env.sh,v 1.2 2015/08/09 13:20:33 nroche Exp $
# * Project: MediaTex
# * Module:  miscellaneous modules
# *
# * script called by command.c within its unit test context
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014 2015 2016 Nicolas Roche
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
echo "MDTX_MDTXUSER            = $MDTX_MDTXUSER"
echo "MDTX_LOG_FACILITY        = $MDTX_LOG_FACILITY"
echo "MDTX_LOG_FILE            = $MDTX_LOG_FILE"
echo "MDTX_LOG_SEVERITY_ALLOC  = $MDTX_LOG_SEVERITY_ALLOC"
echo "MDTX_LOG_SEVERITY_SCRIPT = $MDTX_LOG_SEVERITY_SCRIPT"
echo "MDTX_LOG_SEVERITY_MISC   = $MDTX_LOG_SEVERITY_MISC"
echo "MDTX_LOG_SEVERITY_MEMORY = $MDTX_LOG_SEVERITY_MEMORY"
echo "MDTX_LOG_SEVERITY_PARSER = $MDTX_LOG_SEVERITY_PARSER"
echo "MDTX_LOG_SEVERITY_COMMON = $MDTX_LOG_SEVERITY_COMMON"
echo "MDTX_LOG_SEVERITY_MAIN   = $MDTX_LOG_SEVERITY_MAIN"
echo "MDTX_DRY_RUN             = $MDTX_DRY_RUN"
echo "PARAMETER_1              = $1"

