#!/bin/sh

# Test tss_file_tool for correctness

echo -e "\nRunning tritech-utils testsuite...\n"
test ! -x ./tss_file_tool && echo "Build before testing, code monkey!" && exit 1

ERR=0
echo -n "tss_file_tool GPT yes:     "
if [ "$(./tss_file_tool gpt test/gpt)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool GPT no:      "
if [ "$(./tss_file_tool gpt test/ntfs)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool NTFS yes:    "
if [ "$(./tss_file_tool ntfs test/ntfs)" = "yes" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool NTFS no:     "
if [ "$(./tss_file_tool ntfs test/gpt)" = "no" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool winexec yes: "
if [ "$(./tss_file_tool winexec test/winexec)" = "test/winexec" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool winexec no:  "
if [ "$(./tss_file_tool winexec test/gpt)" = "not_winexec" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

echo -n "tss_file_tool ntfsgeom:    "
cp test/ntfs test/ntfs_test
./tss_file_tool ntfsgeom test/ntfs_test 66 2>&1 >/dev/null
if cmp -s test/ntfs_test test/ntfs_expected
	then echo "OK"
	else echo "broken" && ERR=1
fi
rm -f test/ntfs_test

echo -n "read_inf_section works:    "
if [ "$(./read_inf_section test/ris section1)" = "foo=bar" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi
echo -n "RIS case-sensitive?:       "
if ! ./read_inf_section test/ris section2 2>/dev/null >/dev/null
	then echo "yes"
	else echo "no" && ERR=1
fi

echo -n "suggest_decompressor:      "
if [ "$(./suggest_decompressor foo.tar.lzo)" = "lzop" ]
	then echo "OK"
	else echo "broken" && ERR=1
fi

test "$ERR" = "0" && echo "--- All tests passed ---"
test "$ERR" = "1" && echo "-x- Some tests failed -x-"
echo
exit $ERR
