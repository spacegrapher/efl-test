BIN_NAME    = transform_feedback_elm


# Debugging mode: yes, no
DEBUG_MODE  = no

REQ_PKGS    = dlog x11 gles20 native-buffer libdri2 libdrm libtbm elementary evas

LDFLAGS     = -lm -lpthread `pkg-config --libs $(REQ_PKGS)`

CC  = gcc

LD  = ld

CFLAGS    = -fPIC \
			-Wall \
			-I/media/1TB/work/Redwood/tasks/openglES_3.0_test_suite/elementary/src/lib \
			-I$(PWD)/include -I$(PWD)/src \
			`pkg-config --cflags $(REQ_PKGS)`

	CFLAGS   += -g -O0

C_HEADERS = 

C_SRCS    = src/transform_feedback_elm.c 


OBJS      = $(C_SRCS:%.c=%.o)

all: $(BIN_NAME)

$(BIN_NAME): $(OBJS) $(C_HEADERS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -name "*.o" -exec rm -vf {} \;
	rm -vf $(BIN_NAME)

