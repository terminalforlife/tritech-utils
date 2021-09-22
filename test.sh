#!/bin/bash

# Test tss_file_tool for correctness

echo -e "\nRunning tritech-utils testsuite...\n"
test ! -x ./tss_file_tool && echo "Build before testing, code monkey!" && exit 1

ERR=0
echo -n "tss_file_tool GPT yes:      "
if [ "$(./tss_file_tool gpt test/gpt)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool GPT no:       "
if [ "$(./tss_file_tool gpt test/ntfs)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool NTFS yes:     "
if [ "$(./tss_file_tool ntfs test/ntfs)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool NTFS no:      "
if [ "$(./tss_file_tool ntfs test/gpt)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool HFS+ yes:     "
if [ "$(./tss_file_tool hfsplus test/hfsplus)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool HFS+ no:      "
if [ "$(./tss_file_tool hfsplus test/gpt)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool winexec yes:  "
if [ "$(./tss_file_tool winexec test/winexec)" = "test/winexec" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool winexec no:   "
if [ "$(./tss_file_tool winexec test/gpt)" = "not_winexec" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool registry yes: "
if [ "$(./tss_file_tool registry test/registry)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool registry no:  "
if [ "$(./tss_file_tool registry test/ntfs)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool exfat yes:    "
if [ "$(./tss_file_tool exfat test/exfat)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool exfat no:     "
if [ "$(./tss_file_tool exfat test/ntfs)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool ntfsgeom:     "
cp test/ntfs test/ntfs_test
./tss_file_tool ntfsgeom test/ntfs_test 66 2>&1 >/dev/null
if cmp -s test/ntfs_test test/ntfs_expected
	then echo "OK"
	else echo "broken" && ERR=1
fi
rm -f test/ntfs_test

echo -n "raid0merge works:           "
./raid0merge 512 test/raid0_1 test/raid0_2 test/raid0_3 test/raid0_test 2>&1 >/dev/null
if cmp -s test/raid0_correct test/raid0_test
	then echo "OK"
	else echo "broken" && ERR=1
fi
rm -f test/raid0_test

echo -n "read_inf_section works:     "
if [ "$(./read_inf_section test/ris section1)" = "foo=bar" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "suggest_decompressor:       "
if [ "$(./suggest_decompressor foo.tar.lzo)" = "lzop" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "format_device_entry:        "
if [ "$(echo ',hdaudio\func_01&ven_8086&dev_24c5&subsys_deadbeef&rev_c0' | ./format_device_entry x y z)" = "hdaudio:ven=8086:dev=24c5:subsys=deadbeef:rev=c0:func=01:win=x:dv=y:inf=z" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

test "$ERR" = "0" && echo "--- All tests passed ---"
test "$ERR" = "1" && echo "-x- Some tests failed -x-"
echo
exit $ERR
