# This is makefile
# Note:

############################################################
CC      = gcc
AR      = ar

CFLAGS   = -Wall -g
CFLAGS  += -I./common/
CFLAGS  += -DARCH_LITTLE_ENDIAN
LDFLAGS  = -lpthread
LDFLAGS += -letcd -L./etcd-api/
AFLAGS   = -r 

#############################################################

LIB_M_TARGET = libm.a
LIB_M_OBJS = ./common/log_print.o \
						 ./common/l_mgw.o \
						 ./common/l_error.o \
						 ./common/l_scratch.o \
						 ./common/l_string.o

EXE_S_TARGET = rtps
LIB_S_TARGET = librtps.a
LIB_S_OBGS = rtp_server.o \
						 rtp_process.o \
						 l_rtp.o \
						 l_upd.o \
						 evs.o

EXE_C_TARGET = rtpc
LIB_C_TARGET = librtpc.a
LIB_C_OBGS = rtp_client.o

SUB_DIR = ./etcd-api

TARGET = $(LIB_M_TARGET) $(LIB_S_TARGET) $(EXE_S_TARGET) $(EXE_C_TARGET) $(LIB_C_TARGET)

#############################################################
all : $(TARGET)
$(LIB_M_TARGET) : $(LIB_M_OBJS)
	$(AR) $(AFLAGS) $@ $^
	make -C $(SUB_DIR)

$(LIB_S_TARGET) : $(LIB_S_OBGS)
	$(AR) $(AFLAGS) $@ $^

$(EXE_S_TARGET) : $(LIB_S_TARGET) $(LIB_M_TARGET)
	$(CC) $(LDFLAGS) -o $@ $^ 

$(LIB_C_TARGET) : $(LIB_C_OBGS)
	$(AR) $(AFLAGS) $@ $^

$(EXE_C_TARGET) : $(LIB_C_TARGET) $(LIB_M_TARGET)
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(EXE_S_TARGET)_run :
	export LD_LIBRARY_PATH=./etcd-api/:$(LD_LIBRARY_PATH)
	ldconfig
	./$(EXE_S_TARGET)

clean:
	rm -rf $(LIB_M_OBJS) $(LIB_S_OBGS) $(LIB_C_OBGS) $(TARGET)
	make realclean -C $(SUB_DIR)


