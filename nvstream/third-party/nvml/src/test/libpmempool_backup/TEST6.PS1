#
# Copyright 2016, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# libpmempool_backup/TEST6 -- test backup to poolset which parts already exist
# or almost all parts already exist
#
[CmdletBinding(PositionalBinding=$false)]
Param(
    [alias("d")]
    $DIR = ""
    )

$Env:UNITTEST_NAME = "libpmempool_backup/TEST6"
$Env:UNITTEST_NUM = "6"

. ..\unittest\unittest.ps1

require_test_type medium

require_fs_type pmem non-pmem

setup

. ./common.PS1

for ($i=0; $i -lt $POOL_TYPES.Count; $i++ ) {
	backup_cleanup

	# prepare poolset files
	create_poolset_variation 1
	create_poolset_variation 1 $BACKUP

	# create source poolset parts
	expect_normal_exit $PMEMPOOL create $POOL_TYPES[$i] `
		$POOL_CREATE_PARAMS[$i] $POOLSET

	# create backup poolset parts
	for ($j=1; $j -lt 5; $j++ ) {
		truncate -s 20M $POOL_PART$j$BACKUP
	}

	backup_and_compare $POOLSET $POOL_TYPES[$i]

	# one of parts is too small
	truncate -s 10M ${POOL_PART}3${BACKUP}
	backup_and_compare $POOLSET $POOL_TYPES[$i]

	# one of parts does not exist
	rm ${POOL_PART}3${BACKUP}
	backup_and_compare $POOLSET $POOL_TYPES[$i]
}

rm $OUT -Force -ea si
mv $OUT_TEMP $OUT

check

pass
