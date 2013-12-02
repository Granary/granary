"""Define which symbols should be ignored when wrapping functions.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import re

WHITELIST = {
  # Memory allocators, which we care about for watchpoints
  # applications.
  "malloc":   ("malloc", "__libc_malloc"),
  "valloc":   ("valloc", "__libc_valloc"),
  "calloc":   ("calloc", "__libc_calloc"),
  "realloc":  ("realloc", "__libc_realloc"),
  "free":     ("free", "__libc_free", "cfree"),

  # vfork is tricky to handle, so we punt on it and
  # pretend that turning it into a fork is okay (in
  # reality it's not okay in all cases).
  "vfork":    ("vfork", "__vfork"),

  # Not re-entrant with respect to these.
  "dlsym":                ("dlsym", "__libc_dlsym"),
  "dlopen":               ("dlopen", "__libc_dlopen_mode"),
  "dlclose":              ("dlclose", "__libc_dlclose"),
  "_dl_runtime_resolve":  ("_dl_runtime_resolve",),
  "_dl_fixup":            ("_dl_fixup",),
  "_dl_lookup_symbol_x":  ("_dl_lookup_symbol_x",),
  "_dl_name_match_p":     ("_dl_name_match_p",),
  "do_lookup_x":          ("do_lookup_x",),

  # Useful for user space debugging
  "__assert_fail":  ("__assert_fail", "__GI___assert_fail"),
  "stat": ("stat",),
  "__xstat": ("__xstat", "__xstat64"),
  "open": ("open", "open64"),
  "fopen": ("fopen",),
  "execve": ("execve",),
  "execv": ("execv",),
  "execvp": ("execvp",),
  "execvpe": ("execvpe",),
  "read": ("read",),
  "write": ("write",),

  "getc": ("getc", "_IO_getc",),
}

# TODO: currently have linker or platform errors for these.
IGNORE = set([
  "add_profil",

  "profil",
  "unwhiteout",
  "zopen",
  "_IO_cookie_init",
  "matherr",
  "setkey",
  "zopen",

  # apple-only?
  "pthread_rwlock_downgrade_np",
  "pthread_rwlock_upgrade_np",
  "pthread_rwlock_tryupgrade_np",
  "pthread_rwlock_held_np",
  "pthread_rwlock_rdheld_np",
  "pthread_rwlock_wrheld_np",
  "pthread_getname_np",
  "pthread_setname_np",
  "pthread_rwlock_longrdlock_np",
  "pthread_rwlock_yieldwrlock_np",

  # 'dangerous' Linux symbols
  "sigreturn",
  "tmpnam",
  "tmpnam_r",
  "tempnam",
  "gets", # hrmm
  "mktemp",
  "getpw",

  # non-portable?
  "pthread_mutexattr_setrobust",
  "pthread_mutexattr_getrobust",

  # things that are wacky to wrap
  "alloca",

  "setjmp",
  "_setjmp",
  "__setjmp",

  "sigsetjmp",
  "_sigsetjmp",
  "__sigsetjmp",

  "longjmp",
  "_longjmp",
  "__longjmp",

  "siglongjmp",
  "_siglongjmp",
  "__siglongjmp",

  # not necessarily implemented
  "setlogin",
  "lchmod",
  "revoke",
  "getumask",

  # TODO: make wrappers for these
  "pthread_create",
  "pthread_once",

  # built-in, must ignore
  "__cyg_profile_func_exit",
  "__cyg_profile_func_enter",

  # these don't need to be wrapped (note: these
  # aren't all necessarily real functions!)
  "exec",
  "execv",
  "execvp",
  "execvpe",
  "execl",
  "execle",
  "execlp",
  "execlpe",

  # MUST IGNORE THIS so that it doesn't detach before the
  # main program even starts!!!
  "__libc_start_main",

  # kernel stuff
  "early_printk",
  "warn_alloc_failed",
])

KERNEL_DEV = re.compile("dev_(emerg|alert|crit|err|warn|notice|info|debug|default|cont|printk)")

def should_ignore(name):
  global IGNORE, KERNEL_DEV
  if name in IGNORE:
    return True

  elif KERNEL_DEV.search(name):
    return True

  elif "printf" in name or "printk" in name:
    return True

  return False
