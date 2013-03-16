"""Get debugging information about the Granary module, as loaded in
a VM."""

import sys
import os
import getpass
import argparse
import subprocess

COPY_GRANARY_REMOTE = """scp -P %s %s@%s:%s %s"""

LOAD_GRANARY = """sudo insmod %s"""

# get sections
GET_SECTIONS = """
touch /tmp/granary.sections;
rm /tmp/granary.sections;
for x in $(find /sys/module/granary/sections -type f); do 
  echo $(basename $x),$(sudo cat $x) >> /tmp/granary.sections; 
done"""

# copy the sections
COPY_SECTIONS_REMOTE = """scp -P %s /tmp/granary.sections %s@%s:/tmp/granary.sections"""


if "__main__" == __name__:
  parser = argparse.ArgumentParser(
    description='Get debugging symbols for use by gdb from the Granary kernel module.')

  parser.add_argument(
    '--remote', action='store_true',
    help='If Granary is being run on a remote system.')

  parser.add_argument(
    '--remote-user', type=str, default=getpass.getuser(),
    help='The username used to log into the remote system.')

  parser.add_argument(
    '--remote-host', type=str, default='localhost',
    help='The hostname of the remote system.')

  parser.add_argument(
    '--remote-port', type=str, default='5556',
    help='The port used to connect to the remote system.')

  parser.add_argument(
    '--local-user', type=str, default=getpass.getuser(),
    help='If Granary is running remotely, then this is the user name for the remote system to log into the local system.')

  parser.add_argument(
    '--local-host', type=str, default='10.0.2.2',
    help='If Granary is running remotely, then the host name of the local system.')

  parser.add_argument(
    '--local-port', type=str, default='22',
    help='If Granary is running remotely, then the port used to remotely log into the local system.')

  args = parser.parse_args()

  # get the granary .ko
  rel_path = ""
  if "scripts" in sys.path[0]:
    rel_path = "../"
  local_granary_file = os.path.join(sys.path[0], "%sgranary.ko" % rel_path)

  # get the sections
  with open("/dev/null", "w") as dev_null:

    # running Granary on a remote machine.
    if args.remote:
      print "If/when prompted, you will need to type", \
            "the root password on the remote machine."
      print 
      subprocess.call(
        "ssh -t -p %s %s@%s '%s;%s;%s;%s'" % (
          args.remote_port,
          args.remote_user,
          args.remote_host,
          COPY_GRANARY_REMOTE % (
            args.local_port,
            args.local_user,
            args.local_host,
            local_granary_file,
            "/tmp/granary.ko"),
          LOAD_GRANARY % "/tmp/granary.ko",
          GET_SECTIONS,
          COPY_SECTIONS_REMOTE % (
            args.local_port,
            args.local_user,
            args.local_host)),
        shell=True)
    
    # Running granary on the local machine
    else:
      print "If/when prompted, you will need to type", \
            "the root password on your current machine."
      subprocess.call(
        "%s;%s" % (
          LOAD_GRANARY % local_granary_file,
          GET_SECTIONS), 
        shell=True, 
        stdout=dev_null)

  # parse the sections
  sections = {}
  with open("/tmp/granary.sections", "r") as lines:
    for line in lines:
      section, address = line.strip(" \r\n\t").split(",")
      sections[section] = address

  with open("granary.syms", "w") as f:
    f.write("add-symbol-file %s %s" % (
      local_granary_file,
      sections[".text"]))

    del sections[".text"]
    for section, address in sections.items():
      f.write(" -s %s %s" % (section, address))
    f.write("\n")
  
