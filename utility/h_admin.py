#!/usr/bin/python

# 
# file   : h_admin.py
#
# author :  Tim van Deurzen
# date   :  14/01/2011
#

import os
import sys
import time
import re
from datetime import datetime
from operator import itemgetter
from optparse import OptionParser

usage = "Usage: %prog [options] path"
parser = OptionParser(usage)

# Define the settings that can be altered using commandline arguments.
parser.add_option(
        "-l", 
        "--list", 
        help="List all versioning information for <path>.", 
        dest="listing", 
        action="store_true",
        default=False
        )

parser.add_option(
        "-r", 
        "--restore", 
        help="Restore a file or folder to a certain point in time.", 
        dest="restore", 
        action="store_true",
        default=False
        )

parser.add_option(
        "-d", 
        "--date", 
        help="The date to restore to in dd-mm-YYYY format.", 
        dest="date", 
        default="01-01-1970"
        )

parser.add_option(
        "-t", 
        "--time", 
        help="The time to restore to in hh.mm.ss format.", 
        dest="time", 
        default="00.00.00"
        )

parser.add_option(
        "--xdelta", 
        help="Use xdelta for restoring files. NOTE: only use this if hieronymus was compiled with -D_XDELTA!", 
        dest="xdelta", 
        action="store_true",
        default=False
        )

### Functions ###

def restore(path, date, time, using_xdelta = False):
    filename = extract_filename(path)
    directory = extract_directory(path)
    timestamp = parsedate(date, time)

    print "Restoring %s to snapshot closest to %s at %s" % (path, date, time)

    if len(directory) <= 1:
        version_path = ".version"
    else:
        version_path = "%s/.version" % directory

    if os.path.exists(path) and os.path.exists(version_path):
        snapshot = find_closest_snapshot(timestamp, version_path)
        patch = find_closest_patch(timestamp, snapshot)
    
        if using_xdelta:
            command = "xdelta3 -f -d -s %s/%s %s %s" % (snapshot, filename,
                    patch, path)
        else:
            command = "patch -o %s %s/%s %s" % (path, snapshot, filename, patch)

        os.system(command)


def find_closest_snapshot(timestamp, path):
    minlist = [(abs(int(x) - timestamp), x) for x in os.listdir(path)]
    minlist = sorted(minlist, key=itemgetter(0))

    (_, snapshot) = minlist[0]

    return "%s/%s" % (path, snapshot)


def find_closest_patch(timestamp, path):
    minlist = [(abs(get_patch_timestamp(x) - timestamp), x) 
               for x in os.listdir(path)]
    minlist = sorted(minlist, key=itemgetter(0))

    (_, patch) = minlist[0]

    return "%s/%s" % (path, patch)


def get_patch_timestamp(path):
    m = re.search('[a-zA-Z0-9\_-]-([0-9]{10,}).patch', path)

    if m is not None:
        timestamp = m.group(1)
    else:
        timestamp = 0

    return long(timestamp)


def parsedate(dateval, timeval):
    date_parts = [int(x) for x in dateval.split('-')]
    time_parts = [int(x) for x in timeval.split('.')]

    d = datetime(date_parts[2], date_parts[1], date_parts[0], time_parts[0],
            time_parts[1])

    return long(time.mktime(d.timetuple()))


def extract_filename(path):
    parts = path.split('/')

    return parts[len(parts) - 1]


def extract_directory(path):
    parts = path.split('/')

    directory = parts[:len(parts) - 1]

    return '/'.join(directory)


### Startup code ###

(options, args) = parser.parse_args()
MIN_DATE_LEN = 11
MIN_TIME_LEN = 8

if len(args) == 0:
    parser.print_help()
    sys.exit(0)

if options.listing == True:
    print "Unimplemented!"
    sys.exit(0)

if options.restore == True:
    if len(options.date) < MIN_DATE_LEN and len(options.time) < MIN_TIME_LEN:
        print "Please provide a date and time!\n"
        parser.print_help()
        sys.exit(0)

    restore(args[0], options.date, options.time, options.xdelta)

