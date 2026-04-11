# name of the program (Minix service)
PROG=lab3

# source code files to be compiled
# Compile timer.c directly so lab3 does not depend on external libtimer.a availability.
SRCS = lab3.c kbc.c utils.c ../lab2/timer.c

# additional compilation flags
# "-Wall -Wextra -Werror -I . -std=c11 -Wno-unused-parameter" are already set
CFLAGS += -pedantic
CFLAGS += -DLAB3
CFLAGS += -I../lab2       # Specify directories to the compiler to search for header files
                          # This is not needed if you included them in the `libtimers.a` archive

# list of library dependencies (LCF library)
DPADD += ${LIBLCF}
LDADD += -llcf

# include LCOM's makefile that does all the "heavy lifting"
.include <minix.lcom.mk>

