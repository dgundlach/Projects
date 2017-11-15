BINDIR=/usr/local/bin
INCLUDES=-I lib
LIBS=
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}

MDB_FUNCS_OBJS = mdb_functions.o keypair.o lib/xalloc.o lib/stripspaces.o lib/tstr.o
MDB_BACKUP_OBJS = ${MDB_FUNCS_OBJS} lib/getdu.o lib/getavail.o mdb_backup.o
MDB_CREATE_OBJS = ${MDB_FUNCS_OBJS} lib/getdu.o lib/getavail.o mdb_create.o
MDB_DELETE_OBJS = ${MDB_FUNCS_OBJS} mdb_delete.o
MDB_EMPTY_TRASH_OBJS = ${MDB_FUNCS_OBJS} lib/rm.o mdb_empty_trash.o
MDB_INITDB_OBJS = ${MDB_FUNCS_OBJS} mdb_initdb.o
MDB_LINKS_OBJS = ${MDB_FUNCS_OBJS} mdb_links.o
MDB_LIST_OBJS = ${MDB_FUNCS_OBJS} mdb_list.o
MDB_LIST_DELETED_OBJS = ${MDB_FUNCS_OBJS} mdb_list_deleted.o
MDB_PROCESSREQUEST_OBJS = ${MDB_FUNCS_OBJS} mdb_processrequest.o
MDB_RENAME_OBJS = ${MDB_FUNCS_OBJS} mdb_rename.o
MDB_SETUP_OBJS = ${MDB_FUNCS_OBJS} lib/strfilter.o mdb_setup.o
MDB_SHOWMISSING_OBJS = ${MDB_FUNCS_OBJS} mdb_showmissing.o
MDB_UNDELETE_OBJS = ${MDB_FUNCS_OBJS} mdb_undelete.o

EXECS = mdb_backup mdb_create mdb_delete mdb_initdb mdb_links mdb_list \
		mdb_processrequest mdb_rename mdb_setup mdb_showmissing \
		mdb_list_deleted mdb_undelete mdb_empty_trash

all: ${EXECS}

mdb_backup: ${MDB_BACKUP_OBJS}
	${LINK} ${MDB_BACKUP_OBJS} -o mdb_backup

mdb_create: ${MDB_CREATE_OBJS}
	${LINK} ${MDB_CREATE_OBJS} -o mdb_create

mdb_delete: ${MDB_DELETE_OBJS}
	${LINK} ${MDB_DELETE_OBJS} -o mdb_delete

mdb_empty_trash: ${MDB_EMPTY_TRASH_OBJS}
	${LINK} ${MDB_EMPTY_TRASH_OBJS} -o mdb_empty_trash

mdb_initdb: ${MDB_INITDB_OBJS}
	${LINK} ${MDB_INITDB_OBJS} -o mdb_initdb

mdb_links: ${MDB_LINKS_OBJS}
	${LINK} ${MDB_LINKS_OBJS} -o mdb_links

mdb_list: ${MDB_LIST_OBJS}
	${LINK} ${MDB_LIST_OBJS} -o mdb_list

mdb_list_deleted: ${MDB_LIST_DELETED_OBJS}
	${LINK} ${MDB_LIST_DELETED_OBJS} -o mdb_list_deleted

mdb_processrequest: ${MDB_PROCESSREQUEST_OBJS}
	${LINK} ${MDB_PROCESSREQUEST_OBJS} -o mdb_processrequest

mdb_rename: ${MDB_RENAME_OBJS}
	${LINK} ${MDB_RENAME_OBJS} -o mdb_rename

mdb_setup: ${MDB_SETUP_OBJS}
	${LINK} ${MDB_SETUP_OBJS} -o mdb_setup

mdb_showmissing: ${MDB_SHOWMISSING_OBJS}
	${LINK} ${MDB_SHOWMISSING_OBJS} -o mdb_showmissing

mdb_undelete: ${MDB_UNDELETE_OBJS}
	${LINK} ${MDB_UNDELETE_OBJS} -o mdb_undelete

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f lib/*.o *.o ${EXECS}

install: ${EXECS}
	install -t ${BINDIR} ${EXECS}
