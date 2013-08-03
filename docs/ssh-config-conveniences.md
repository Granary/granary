SSH Config Conveniences
=======================

When interacting with virtual machines in the course of Granary development
work, a few simple SSH configuration tweaks can make the process much smoother.

## Step 1: Create a public/private keypair for your machine

Only if you haven't already done so before:

```basemake
$ ssh-keygen -t rsa -C "$USER@$HOST" -f ~/.ssh/id_rsa
```

This step will create two files under ~/.ssh/, your secret private key `id_rsa`
and your shareable public key `id_rsa.pub`. Be sure to never overwrite or lose
your private key! In fact, why not set its permissions to read-only if
`ssh-keygen` did not do so already:

```basemake
$ chmod u=r,go= ~/.ssh/id_rsa
```

Consider too, if you regularly interact with multiple machines, that instead of
`id_rsa` you use a filename like `user@host` to more easily manage them all
(and to not confuse the the public keys that you will want to copy back and
forth.)

## Step 2: Setup your SSH client config

First, we'll need this directory:

```basemake
$ mkdir ~/.ssh/sessions/
```

Next, at the top of `~/.ssh/config`, add:

```basemake
# The private key that is unique to your machine/account/person
IdentityFile ~/.ssh/id_rsa

# The first SSH session you initiate makes the connection and authenticates,
# and then any additional logins or file transfers will not need to repeat
# those steps as they will proxy through the already established control master
# connection
ControlMaster auto
ControlPath ~/.ssh/sessions/%h-%r-%p
```

For each VM or target host, add a section like this with the necessary
user/host/port changes as applicable to your setup:

```basemake
Host deb7vm
    User root
    Hostname localhost
#    Hostname 10.0.2.15
    Port 5556

    # For throw-away VM's, this is useful because otherwise any change to the
    # IP address of the hostname (due to DHCP, for example) will cause SSH to
    # be suspicious and ask for confirmation
    StrictHostKeyChecking no
```

At the end of your configuration file, also consider adding these as well:

```basemake
# This adds these options for all hosts by default (the wildcard match)
Host *
    # Ensure we use the latest and most secure version of the SSH protocol
    Protocol 2
    VisualHostKey yes

    # Keep the master control connection alive indefinitely
    KeepAlive yes
    ServerAliveInterval 60
#    StrictHostKeyChecking yes
```

## Step 3: Authorize Each Host for Password-free Access

Next, ensure your public key is authorized for each host you setup. You will
need to enter a password for that host during this step.

```basemake
$ ssh-copy-id -i ~/id_rsa.pub deb7vm
```

Hereafter, you can simply `ssh deb7vm` or `scp somefile deb7vm:` or `scp
deb7vm:otherfile otherfile` and it will work without further authentication.

## Step 4: Use the remote load script

Now you can easily push a freshly recompiled Granary module to a running VM, by
using the helper tool under `scripts/remoteload.py` as follows:

```basemake
$ ./scripts/remoteload.py --remote deb7vm
```

