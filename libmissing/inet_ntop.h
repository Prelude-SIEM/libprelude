#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

# if defined HAVE_INET_NTOP && !HAVE_INET_NTOP

extern const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);

#endif
