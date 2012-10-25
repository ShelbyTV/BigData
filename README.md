#BigData

###Overview

The Shelby database has a lot of valuable data. Code contained in this repository aims to process that data into a usable form.

Currently the main production usage is generating social video recommendations based on the social shares contained in the frames database. This is done using item-based-filtering, a collaborative filtering algorithm.

All of this code was written on `nos-db-s0-d`, and it may have dependencies on locally installed packages (especially for EC2 functionality). An attempt will be made to name these in this document, but if any are missed, look on `nos-db-s0-d` for clues.

### Code

Most of the heavy lifting code is written in C++0x (C++ with native thread, mutex, etc. support -- the next gen C++). There are some old scripts in python from various experiments, but python was generally too slow to do much data processing. The scripts for executing on EC2 are just shell scripts.

#### Building

The top-level BigData directory contains a SConstruct file that can be used to build the C++ programs and also copy scripts into the output bin directory.

To build, you should just need to type `scons`

To completely clean up all the built code, you should be able to run `scons -c`

### Video Recommendations

The basic steps for generating the social video recommendations (which power Shelby Genius) are outlined below in full detail.

The item-based filtering algorithm itself is outlined fairly clearly in `docs/amazonRecommendations.pdf`, an article that details Amazon's usage of item-based filtering.

Originally an attempt was made to use mahout (a Hadoop computation library) and Hadoop running on Amazon Elastic MapReduce. However, this turned out to be slow and expensive.

The current approach is using a custom C++ program that runs on a single large Amazon EC2 compute node. This eliminates the networking and storage overhead that hadoop introduces. However, it does mean that in the future we could be limited by the RAM on the machine that calculates the results or we could also limited in the overall number of items and users by the datatype used to store the unique identifiers for each item and user.

The other key item to note is that we're using Ted Dunning's log likelihood similarity function, which seemed to function best in practice. This algorithm compares items by also taking into account the overall frequency/likelihood of each item, which helps mitigate the effect of viral videos being shared (so they're not recommended for every video). The code for calculating log likelihood was ported into C++ from the Java implementation in mahout.

#### Acquire Data

After running `scons`, a program will exist at `bin/gtMongoVideoRollDataPrep`. This is used to capture all the necessary data regarding social video shares from the frames database. It must be run from a machine that has access to the rolls-frames database.

This command will output 3 .csv files: `videos.csv`, `users.csv`, and `broadcasts.csv`. These will be used as input for the following commands.

`videos.csv` contains the mapping from integers to mongo video IDs.

`users.csv` contains the mapping from integers to mongo roll IDs. In the past this used to be actual users, hence the filename.

`broadcasts.csv` contains a list of pairs of video and user integers, with each line indicating an instance of the video on a particular roll.

#### Filter

After running `scons`, a program will exist at `bin/csvPrune`. This is used to filter out videos (items) that have not been shared frequently and rolls (users) that have not shared many videos (items).

The default threshold is 5 for both instances.

This command takes as input by default the above `broadcasts.csv` file and outputs a file named `input.csv`. Confusing, apologies… This file, `input.csv` is the sole input file for the actual item-based filtering algorithm.

#### Generate Recommendations

After running `scons`, a program will exist at `bin/ibf`. This is the program that actually implements the item-based filtering algorithm. This is the most complicated program in BigData. By default it takes as input a file named `input.csv` (generated via the above `csvPrune` application) and generates a file named `output` that contains item-to-item (video-to-video) recommendations (and associated scores).

`ibf` can be run on any Linux system, but running it with data from the entire Shelby dataset works best on a beefy machine. And so we use Amazon EC2 to temporarily acquire a beefy machine to run the algorithm.

Running `scons` will also copy a script to `bin/ibfAws.sh`. This script does the following:

- creates an AWS spot instance (refer to amazon.com for info on AWS spot pricing)
- copies the `ibf` program and `input.csv` file to the AWS instance
- runs the `ibf` program
- concatenates the results from different threads into one `output` file
- downloads the `output` file
- destroys the AWS instance

The `bin/ibfAws.sh` script must be run from the directory that contains the `input.csv` file because it currently assumes that location.

The `bin/ibfAws.sh` script uses the keys in `aws/ec2keys` to access the Shelby AWS account (Dan's account). It uses scripts in `aws/ec2-api-tools-1.5.3.0/` to be able to interact with AWS.

Currently this script takes a little less than an hour to run (once a spot instance is allocated -- sometimes that can take a while).

**NOTE:** The `bin/ibfAws.sh` script may not be entirely reliable, especially in case of interruption or errors from AWS. One should always double-check the Amazon EC2 console to make sure that spot instances are not left running, that spot instance requests are cancelled, etc. A spot instance left running will continue to cost money.

**ALSO NOTE:** The spot instance pricing in the `bin/ibfAws.sh` script may need to be adjusted as the AWS spot instance market changes.

#### Update Video Database

After running `scons`, a program will exist at `bin/gtMongoUpdate`. This program takes in the `ibf` `output` file as well as the original `users.csv` and `videos.csv` files, and updates the recommendations in the production mongo databases.

This program must be run from a machine that has access to the production mongo databases.

### Language Detection

Inside `lib/compact-language-detector` is source code that was downloaded that originally came from Google Chrome. It uses `make` to be built, so it couldn't be easily integrated into the `scons` build infrastructure.

However, one can use `make` to build the library in this directory.

The top-level `Sconstruct` file has some commented out lines for building `mongoCommentsGetter`. This code has not been updated to work with the GT databases.

However, the code in `source/mongoCommentsGetter` shows how the compact language detector library can be used to detect various languages in share comments. In general it works fairly well and fairly quickly and can be used to identify a lot of comments that are obviously one language or another.

### Other Future Applications

Mostly the code in the BigData repository shows an example of how to get a lot of data from the Shelby database and process it in hours (not days). Ruby / python / etc. can just be too slow to get things done.

Using the C++ code in BigData as an example (and even just re-using `ibf` in different ways) could help with the following:

* users-to-follow recommendations
* video sharing statistics
* trending topics


### What Belongs In BigData

The intention is that the code in this repository remain focused on processing Shelby database data and turning it into usable information.

Ideally, the code to display and/or perform additional manipulations of the processed data would be stored elsewhere. e.g. `ibf` code is here to produce video recommendations, but the code for Shelby Genius is in the API server, front-end web, and iOS repositories.
