/^#@INSERT_SYS_SELECT_H@/ d
/^@FALLBACK_SOCKLEN_T@/ d
/^#include <stdlib\.h>/ d
/^#include <string\.h>/ d
/^#include <gpg-error\.h>/ s,#include <gpg-error.h>,#include <grub/gcrypt/gpg-error.h>,
s,_gcry_mpi_invm,gcry_mpi_invm,g
p