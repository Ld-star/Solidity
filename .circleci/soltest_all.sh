#! /bin/bash
#------------------------------------------------------------------------------
# Bash script to execute the Solidity tests by CircleCI.
#
# The documentation for solidity is hosted at:
#
#     https://solidity.readthedocs.org
#
# ------------------------------------------------------------------------------
# This file is part of solidity.
#
# solidity is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# solidity is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with solidity.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016-2019 solidity contributors.
# ------------------------------------------------------------------------------
set -e

REPODIR="$(realpath $(dirname $0)/..)"

EVM=istanbul OPTIMIZE=1 ABI_ENCODER_V2=1 ${REPODIR}/.circleci/soltest.sh

for OPTIMIZE in 0 1; do
    for EVM in homestead byzantium constantinople petersburg istanbul; do
        EVM=$EVM OPTIMIZE=$OPTIMIZE BOOST_TEST_ARGS="-t !@nooptions" ${REPODIR}/.circleci/soltest.sh
    done
done
