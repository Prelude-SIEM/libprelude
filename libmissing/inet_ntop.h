#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

# if defined HAVE_INET_NTOP && !HAVE_INET_NTOP

extern const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);

#endif
