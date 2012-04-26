#!/bin/bash
#
# This script does the following:
#  - creates an AWS instance
#  - copies the ibf program and input.csv file to the AWS instance
#  - runs the ibf program
#  - concatenates the results into one file
#  - downloads the output file
#  - destroys the AWS instance

# make sure referenced file locations are relative to this script's locations
bigDataBinDir=$(readlink -f $(dirname $0))
ec2BinDir=$bigDataBinDir/../aws/ec2-api-tools-1.5.3.0/bin/

# needs to be based off of this script's directory location
export EC2_HOME=$bigDataBinDir/../aws/ec2-api-tools-1.5.3.0
export EC2_PRIVATE_KEY=$bigDataBinDir/../aws/ec2keys/x509/pk-XHDQHIP6UDQ34VVTWGPKDIB4VDYQ22SG.pem
export EC2_CERT=$bigDataBinDir/../aws/ec2keys/x509/cert-XHDQHIP6UDQ34VVTWGPKDIB4VDYQ22SG.pem

# should ideally only attempt to override if not already set
export JAVA_HOME=/usr/lib/jvm/java-6-openjdk

# spot instance request ID
sirID=$($ec2BinDir/ec2-request-spot-instances --price 0.66 --instance-count 1 --type one-time --instance-type c1.xlarge --group bigdata --key bigdata ami-f565ba9c | awk '{print $2;}')

echo "Checking to see if spot instance is active..."
sirStatus=$($ec2BinDir/ec2-describe-spot-instance-requests $sirID | awk '{print $6;}')
while [ $sirStatus != "active" ]
do
echo "Sleeping for 60 seconds, waiting for spot instance to turn active..."
sleep 60
sirStatus=$($ec2BinDir/ec2-describe-spot-instance-requests $sirID | awk '{print $6;}')
done
echo "Spot instance active."

instanceID=$($ec2BinDir/ec2-describe-spot-instance-requests $sirID | awk '{print $8;}')

echo "Checking to see if instance is running..."
instanceStatus=$($ec2BinDir/ec2-describe-instances $instanceID | tail -n 1 | awk '{print $6;}')
while [ $instanceStatus != "running" ]
do
echo "Sleeping for 60 seconds, waiting for instance to begin running..."
sleep 60
instanceStatus=$($ec2BinDir/ec2-describe-instances $instanceID | tail -n 1 | awk '{print $6;}')
done
echo "Instance is running."

instanceNetName=$($ec2BinDir/ec2-describe-instances $instanceID | tail -n 1 | awk '{print $4;}')

bigDataSSHKey=$bigDataBinDir/../aws/ec2keys/keypair/bigdata.pem
ec2InstanceSSHCommand="ssh ec2-user@$instanceNetName -i $bigDataSSHKey -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"
ec2InstanceSCPCommand="scp -i $bigDataSSHKey -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

echo "Check to see if ssh is working."
$ec2InstanceSSHCommand "sh -c \"uname\"" > /dev/null 2>&1
sshStatus=$?
while [ $sshStatus -ne 0 ]
do
echo "Sleeping for 60 seconds, waiting for ssh access..."
sleep 60
$ec2InstanceSSHCommand "sh -c \"uname\"" > /dev/null 2>&1
sshStatus=$?
done
echo "ssh is working."

echo "Copying files to spot instance."
$ec2InstanceSCPCommand $bigDataBinDir/ibf ec2-user@$instanceNetName:~/
$ec2InstanceSCPCommand input.csv ec2-user@$instanceNetName:~/
echo "Files copied to spot instance."

$ec2InstanceSSHCommand -n -f "sh -c \"cd ~; nohup ./ibf -t 8 >/dev/null 2>&1 &\""
ibfPid=$($ec2InstanceSSHCommand "sh -c \"ps -C ibf --no-headers | awk '{print \\\$1;}'\"")

$ec2InstanceSSHCommand "sh -c \"kill -0 $ibfPid\""
ibfRunning=$?
while [ $ibfRunning -eq 0 ]
do
echo "ibf still running, sleeping for 60 seconds..."
sleep 60
$ec2InstanceSSHCommand "sh -c \"kill -0 $ibfPid\""
ibfRunning=$?
done
echo "ibf finished."

$ec2InstanceSSHCommand "sh -c \"cat output* > output\"" > /dev/null 2>&1

$ec2InstanceSCPCommand ec2-user@$instanceNetName:~/output output

$ec2BinDir/ec2-terminate-instances $instanceID
$ec2BinDir/ec2-cancel-spot-instance-requests $sirID

# XXX perms in git? 
# nos@nos-db-s0-d:~/bigdata_do_not_delete/04.18.2012$ chmod 400 ../BigData/aws/ec2keys/keypair/bigdata.pem 
