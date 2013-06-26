#!/bin/bash
# This script runs all the individual scripts and programs necessary
# to update the video graph recommendations
#
# THIS SCRIPT RELIES ON A PROPERLY SET $PATH ENVIRONMENT VARIABLE
# TO FIND ALL OF THE SCRIPTS AND PROGRAMS IT RUNS INTERNALLY
#
# You should run this script from within the folder where you want the
# intermediate input and output files to be placed, usually something
# like
#     nos@nos-db-s0-d:~/bigdata_do_not_delete/XX.XX.XXXX
# where the X part is a folder you've created and named after the
# date you are running the recommendation generator

echo 'Update Video Graph: START Getting video data from mongo'

gtMongoVideoRollDataPrep

echo 'Update Video Graph: COMPLETE Getting video data from mongo'

echo 'Update Video Graph: START Filter out videos and users with too few shares'

csvPrune

echo 'Update Video Graph: COMPLETE Filter out videos and users with too few shares'

echo 'Update Video Graph: START Item based filtering on AWS'

ibfAws.sh

echo 'Update Video Graph: COMPLETE Item based filtering on AWS'

echo 'Update Video Graph: START Write new video recommendations into mongo'

gtMongoUpdate

echo 'Update Video Graph: COMPLETE Write new video recommendations into mongo'