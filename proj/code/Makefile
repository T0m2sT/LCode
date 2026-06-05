# name of the program (Minix service)
PROG=proj

# Framework path
FRAMEWORK_DIR      = ../../framework
FRAMEWORK_INCLUDES = ${FRAMEWORK_DIR}/includes

# Link .o files directly — linking the .a causes LCF stubs to win over framework
# implementations due to static archive scan order. This way implementations aren't missing at runtime.
FW_OBJS = ${FRAMEWORK_DIR}/timer.o \
          ${FRAMEWORK_DIR}/keyboard.o \
          ${FRAMEWORK_DIR}/mouse.o \
          ${FRAMEWORK_DIR}/kbc.o \
          ${FRAMEWORK_DIR}/video.o \
          ${FRAMEWORK_DIR}/utils.o \
          ${FRAMEWORK_DIR}/rtc.o \
          ${FRAMEWORK_DIR}/serial_port.o \
          ${FRAMEWORK_DIR}/queue.o

# additional compilation flags
# "-Wall -Wextra -Werror -I . -std=c11 -Wno-unused-parameter" are already set
CFLAGS += -pedantic -I${FRAMEWORK_INCLUDES}

# list of library dependencies (LCF)
DPADD += ${FW_OBJS} ${LIBLCF}
LDADD += ${FW_OBJS} -llcf
LDLIBS += -lm

# define headers dependencies
HDRS = $(find . -name *.c)

SRCS = proj.c ih.c render_state.c scene.c font.c mouse_cursor.c syntax.c highlight_cache.c editor.c keyboard.c mouse.c events.c commands.c command_bar.c filetree.c filetree_commands.c serial.c clock.c session_time.c
OBJS = ${SRCS:.c=.o}
.PATH: controller controller/ih controller/input view view/renderer view/editor model model/time
.PATH.o: build

.PHONY: run

run: all
	for obj in $(OBJS); do \
		mkdir -p build; \
		mv $$obj build/$$obj; \
	done

# include LCOM's makefile that does all the "heavy lifting"
.include <minix.lcom.mk>