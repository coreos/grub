/^#@INSERT_SYS_SELECT_H@/ d
/^@FALLBACK_SOCKLEN_T@/ d
/^# *include <stdlib\.h>/ d
/^# *include <string\.h>/ d
/^# *include <winsock2\.h>/ d
/^# *include <ws2tcpip\.h>/ d
/^# *include <time\.h>/ d
/^# *include <sys\/socket\.h>/ d
/^# *include <sys\/time\.h>/ d
/^# *include <gpg-error\.h>/ s,#include <gpg-error.h>,#include <grub/gcrypt/gpg-error.h>,
s,_gcry_mpi_invm,gcry_mpi_invm,g
p