# This is makefile
# Note:

############################################################
CC      = gcc
AR      = ar

IPP_DIR = ./intel/ipp
USC_DIR = ./intel/usc/sources/speech-codecs/core/usc
EVS_DIR = ./intel/usc/sources/speech-codecs/codec/speech/evs
COMMON_DIR = ./intel/usc/sources/speech-codecs/codec/speech/common

CFLAGS   = -Wall -O2
CFLAGS  += -I./common
CFLAGS  += -I$(IPP_DIR)/include
CFLAGS  += -I$(USC_DIR)/include
CFLAGS  += -I$(EVS_DIR)/include
CFLAGS  += -I$(COMMON_DIR)/include
CFLAGS  += -DARCH_LITTLE_ENDIAN
CFLAGS  += -DBOARD_X86_VIRTUAL
LDFLAGS  = -lpthread
#LDFLAGS += -letcd -L./etcd-api/
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
			 evs.o \
			 m_evs.o \
			 m_src.o \
			 fir_filter_blk4.o \

EXE_C_TARGET = rtpc
LIB_C_TARGET = librtpc.a
LIB_C_OBGS = rtp_client.o

SUB_DIR = ./etcd-api

LIB_IPP = $(wildcard $(IPP_DIR)/lib/*.a)

LIB_COMMON = libcommon.a
LIB_COMMON_OBGS = $(patsubst %.c, %.o, $(wildcard $(COMMON_DIR)/src/*.c))

LIB_EVS = libevs.a
LIB_EVS_OBGS = $(patsubst %.c, %.o, $(wildcard $(EVS_DIR)/evs_usc/*.c))


TARGET = $(LIB_M_TARGET) $(LIB_S_TARGET) $(EXE_S_TARGET) $(EXE_C_TARGET) $(LIB_C_TARGET)

#############################################################
all : $(TARGET)

$(LIB_COMMON) : $(LIB_COMMON_OBGS)
	$(AR) $(AFLAGS) $@ $^

$(LIB_EVS) : $(LIB_EVS_OBGS)
	$(AR) $(AFLAGS) $@ $^

$(LIB_M_TARGET) : $(LIB_M_OBJS)
	$(AR) $(AFLAGS) $@ $^
	#make -C $(SUB_DIR)

$(LIB_S_TARGET) : $(LIB_S_OBGS)
	$(AR) $(AFLAGS) $@ $^

$(EXE_S_TARGET) : $(LIB_S_TARGET) $(LIB_M_TARGET) $(LIB_EVS) $(LIB_COMMON)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIB_IPP)

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
	rm -rf $(LIB_M_OBJS) $(LIB_S_OBGS) $(LIB_C_OBGS) $(LIB_COMMON) $(LIB_COMMON_OBGS) $(LIB_EVS) $(LIB_EVS_OBGS) $(TARGET)
	make realclean -C $(SUB_DIR)


