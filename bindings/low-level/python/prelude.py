from _prelude import *

class PreludeError(Exception):
    def __init__(self, errno, strerror=None):
        self.errno = errno
        self._strerror = strerror

    def __str__(self):
        if self._strerror:
            return self._strerror
        return prelude_strerror(self.errno)
