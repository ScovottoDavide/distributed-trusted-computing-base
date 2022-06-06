#define the C compiler to use
CC = gcc

# define any library to link into executable:
#  if I want to link libraries (libx.so or libx.a) I use the -llibname option
LIBS = -ltss2-esys -ltss2-tctildr -ltss2-mu -lssl -lcrypto

# define the C source files
SRCS = PoC_TPA.c tpm2_createek.c tpm2_createak.c tpm2_quote.c createek_util.c createak_util.c PCR9Extend.c PEMconvertPub.c ../IMA/ima_read_writeOut_binary.c

# define rule for gcc
TARGET = PoC_TPA.c

# define the executable file
MAIN = TPA

all: $(TARGET)
	$(CC) -o $(MAIN) $(SRCS) $(LIBS)

clean:
	$(RM) $(MAIN)