#!/bin/bash

run_test () {
  echo -en "$1:\t"
  cat $1 | ~/skfs/build/sqlive --always-allow-joins | sort > /tmp/kk1
  cat $1 | sqlite3 | sort > /tmp/kk2
  diff /tmp/kk1 /tmp/kk2 > /dev/null
  if [ $? -eq 0 ]; then
      echo "OK"
  else
      echo "FAILED"
  fi
}

run_one_test () {
  cat $1 | time ~/skfs/build/sqlive --always-allow-joins | sort > /tmp/kk1
  cat $1 | time sqlite3 | sort > /tmp/kk2
  diff /tmp/kk1 /tmp/kk2
  if [ $? -eq 0 ]; then
     echo "OK"
  fi
}

echo ""
echo "*******************************************************************************"
echo "* SQLIVE TESTS *"
echo "*******************************************************************************"
echo ""

if ! [ -z "$1" ]; then
    if test -f "$1"; then
        echo "RUNNING TEST: $1"
        run_one_test $1
    else
        echo "File does not exist: $1"
    fi
    exit 0
fi

run_test 'test/select1.sql';
run_test 'test/select2.sql';
run_test 'test/select3.sql';
run_test 'test/select4.1.sql';
# run_test 'test/select4.2.sql';
run_test 'test/select5.1.sql';
# run_test 'test/select5.2.sql';

for i in test/random/expr/*.sql; do
    run_test $i;
done

for i in test/random/select/*.sql; do
    run_test $i;
done

for i in test/random/groupby/*.sql; do
    run_test $i;
done

for i in test/random/aggregates/*.sql; do
    run_test $i;
done

echo ""
echo "*******************************************************************************"
echo "* SQLIVE CONCURRENCY TESTS *"
echo "*******************************************************************************"
echo ""

for i in {1..10}; do ./test_concurrent_sync.sh; done
for i in {1..10}; do (cd ./test/concurrent/inserts/ && ./run.sh); done
for i in {1..10}; do (cd ./test/concurrent/sum/ && ./run.sh); done
for i in {1..10}; do (cd ./test/concurrent/sum_transaction/ && ./run.sh); done

echo ""
echo "*******************************************************************************"
echo "* SQLIVE LARGE TESTS *"
echo "*******************************************************************************"
echo ""

run_test 'test/select1_large.sql'
run_test 'test/select2_large.sql'
run_test 'test/select3_large.sql'

echo ""
echo "*******************************************************************************"
echo "* SQLIVE UNIT TESTS *"
echo "*******************************************************************************"
echo ""

run_test 'test/comments.sql'

echo ""
echo "*******************************************************************************"
echo "* SQLIVE DIFF TESTS *"
echo "*******************************************************************************"
echo ""

./test_diff.sh

echo ""
echo "*******************************************************************************"
echo "* UNIT TESTS *"
echo "*******************************************************************************"
echo ""

./unit_tests.sh

echo ""
echo "*******************************************************************************"
echo "* MEMORY *"
echo "*******************************************************************************"
echo ""

(cd ./test/memory/ && ./run.sh)

echo ""
echo "*******************************************************************************"
echo "* TPC-H *"
echo "*******************************************************************************"
echo ""

(cd ./test/TPC-h/ && ./test_tpch.sh)
