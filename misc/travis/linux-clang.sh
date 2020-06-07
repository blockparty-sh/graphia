#! /bin/bash
#
# Copyright © 2013-2020 Graphia Technologies Ltd.
#
# This file is part of Graphia.
#
# Graphia is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Graphia is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Graphia.  If not, see <http://www.gnu.org/licenses/>.
#

export CC=clang-10
export CXX=clang++-10
. /opt/qt514/bin/qt514-env.sh

scripts/linux-build.sh
installers/linux/build.sh

export CLANGTIDY="clang-tidy-10"
export CLAZY="clazy --standalone"
scripts/static-analysis.sh -v
