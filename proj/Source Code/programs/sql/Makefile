SERVER_INCLUDES := `pg_config --includedir-server`

TARGETS = billing.so

OBJS = change_user.o initialize_user.o \
       init_trigger.o salt.o

#OBJS = apply_fee.o change_user.o initialize_user.o preprocess_trans.o \
#       reconcile_amounts.o revoke_funds.o setup_prepaid.o salt.o \
#       init_trigger.o

OBJDIR = /usr/share/billing

CFLAGS = -Wall

all: $(TARGETS)

clean:
	rm -f *.o *.so

install: $(TARGETS)
	cp $? $(OBJDIR)

apply_fee.o: apply_fee.c
	cc $(CFLAGS) -fpic -c apply_fee.c -I$(SERVER_INCLUDES)

billing.so: $(OBJS)
	cc -shared -o billing.so $(OBJS) -lcrypt

change_user.o: change_user.c
	cc $(CFLAGS) -fpic -c change_user.c -I$(SERVER_INCLUDES)

crypt_text.o: crypt_text.c
	cc $(CFLAGS) -fpic -c crypt_text.c -I$(SERVER_INCLUDES)

initialize_user.o: initialize_user.c
	cc $(CFLAGS) -fpic -c initialize_user.c -I$(SERVER_INCLUDES)

init_trigger.o: init_trigger.c
	cc $(CFLAGS) -fpic -c init_trigger.c -I$(SERVER_INCLUDES)

preprocess_trans.o: preprocess_trans.c
	cc $(CFLAGS) -fpic -c preprocess_trans.c -I$(SERVER_INCLUDES)

reconcile_amounts.o: reconcile_amounts.c
	cc $(CFLAGS) -fpic -c reconcile_amounts.c -I$(SERVER_INCLUDES)

revoke_funds.o: revoke_funds.c
	cc $(CFLAGS) -fpic -c revoke_funds.c -I$(SERVER_INCLUDES)

salt.o: salt.c
	cc $(CFLAGS) -fpic -c salt.c

setup_prepaid.o: setup_prepaid.c
	cc $(CFLAGS) -fpic -c setup_prepaid.c -I$(SERVER_INCLUDES)
