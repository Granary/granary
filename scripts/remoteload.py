#!/usr/bin/env python
"""
Load the Granary module into a remote target and retrieve debugging symbols.

In the Granary repository, see `docs/ssh-config-conveniences.md` for some tips
on how to best make use of this.

Based on load.py by Peter Goodman, revised and simplified by Peter McCormick.
"""

import sys
import os
import argparse
import subprocess

LOAD_GRANARY = """sudo insmod %s"""

# get sections
GET_SECTIONS_FMT = """
touch /tmp/{module}.sections;
rm /tmp/{module}.sections;
for x in $(find /sys/module/{module}/sections -type f); do
  echo $(basename $x),$(sudo cat $x) >> /tmp/{module}.sections;
done"""

# get /proc/kallsyms
GET_SYMBOLS = """sudo cat /proc/kallsyms > %s"""

class SshAgent(object):
    def __init__(self, target):
        # The format of `target` maps to SSH invocations as follows:
        #
        #   barehost        -> ssh barehost ...
        #   user@host       -> ssh user@host
        #   barehost:port   -> ssh -P port barehost
        #   user@host:port  -> ssh -P port user@host
        #

        self.target = target
        self.port = None

        if ':' in target:
            self.target, port = target.rsplit(":", 1)
            if not port.isdigit():
                # And we're not going to consult /etc/services for you!!
                raise Exception("Non-numeric port %r" % (port, ))

            self.port = int(port)

        ssh_opts = [ '-t']
        scp_opts = []

        if self.port is not None:
            # NB: SCP and SSH differ on the capitalization of the port argument
            ssh_opts.append("-p %d" % (self.port, ))
            scp_opts.append("-P %d" % (self.port, ))

        self._ssh_opts = " ".join(ssh_opts)
        self._scp_opts = " ".join(scp_opts)

    def _run(self, cmd):
        subprocess.call(cmd, shell=True)

    def copy_to_target(self, local_src, target_dest):
        # FIXME a terribly unsafe use of unquoted paths everywhere!
        cmd = """scp %s %s %s:%s""" % (self._scp_opts, local_src, self.target, target_dest)
        return self._run(cmd)

    def copy_from_target(self, target_src, local_dest):
        # FIXME a terribly unsafe use of unquoted paths everywhere!
        cmd = """scp %s %s:%s %s""" % (self._scp_opts, self.target, target_src, local_dest)
        return self._run(cmd)

    def run_on_target(self, user_cmd):
        # FIXME a terribly unsafe use of unquoted paths everywhere!
        cmd = """ssh %s %s '%s'""" % (self._ssh_opts, self.target, user_cmd)
        return self._run(cmd)

def main(argv):
    parser = argparse.ArgumentParser(
            description='Get debugging symbols for use by gdb from the Granary kernel module.')

    parser.add_argument(
            '--remote', type=str,
            help="Remote SSH target of the form: [user@]host[:port]")

    parser.add_argument(
            '--symbols', action='store_true',
            help='Tell Granary to load /proc/kallsyms from the target system. This will not load granary.')

    parser.add_argument(
            '--module', type=str, default='granary',
            help='Name of the module for which symbols should be fetched.')

    args = parser.parse_args(argv)

    # get the granary .ko
    rel_path = ""
    if "scripts" in sys.path[0]:
        rel_path = "../"

    local_granary_file = os.path.join(sys.path[0], "%sgranary.ko" % rel_path)
    local_symbols_file = os.path.join(sys.path[0], "%skernel.syms" % rel_path)
    local_granary_symbols_file = os.path.join(sys.path[0], "%sgranary.syms" % rel_path)

    if not args.remote:
        print("Error: No remote target given!")
        parser.print_usage()
        return 1

    agent = SshAgent(args.remote)

    if args.symbols:
        # On the target, get the current kernel's symbols
        agent.run_on_target(GET_SYMBOLS % "/tmp/kernel.syms")

        # Copy the symbols from the target to local
        agent.copy_from_target("/tmp/kernel.syms", local_symbols_file)

    else:
        # Copy the kernel module binary to the target
        agent.copy_to_target(local_granary_file, "/tmp/granary.ko")

        # Load Granary module & get sections
        cmd = [
                LOAD_GRANARY % "/tmp/granary.ko",
                GET_SECTIONS_FMT.format(module=args.module)
                ]
        agent.run_on_target(";".join(cmd))

        tmp_module_sections = "/tmp/%s.sections" % (args.module, )
        agent.copy_from_target(tmp_module_sections, tmp_module_sections)

    # parse the sections
    if not args.symbols:
        sections = {}

        with open("/tmp/granary.sections", "r") as lines:
            for line in lines:
                section, address = line.strip(" \r\n\t").split(",")
                sections[section] = address

        with open(local_granary_symbols_file, "w") as f:
            f.write("add-symbol-file %s %s" % (
                local_granary_file,
                sections[".text"]))

            del sections[".text"]

            for section, address in sections.items():
                f.write(" -s %s %s" % (section, address))
            f.write("\n")

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

