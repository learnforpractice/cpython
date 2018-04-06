import sys
with open('Modules/Setup.local', 'a+') as fout:
        fout.write("SSL=%s\n"%(sys.argv[1],))
        fout.write("_ssl _ssl.c -DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl -L$(SSL)/lib -lssl -lcrypto\n")
