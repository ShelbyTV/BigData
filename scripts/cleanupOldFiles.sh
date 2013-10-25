#!/bin/bash
#
# This script cleans up old folders of exported files from the
# updateVideoGraph.sh script
# cleanup old export folders
date=${date-$(date +%Y-%m-%d)}
for dir in $(find . -maxdepth 1 -name '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]' -type d)
do
  echo "Checking if $dir is a folder that needs to be removed"
  # only remove export files from earlier dates
  if [ $(basename $dir) \< $date ]
  then
    echo "Removing old export folder $dir"
    bash -c "set -x; rm -rf $dir"
  else
    echo "Nope, $dir isn't old enough yet"
  fi
done