Changes made:

-Added a Makefile
To compile "server" and "client"
> make
To remove the binaries and test png
> make clean
NOTE: -Wall flag is on to catch warnings, so does not compile silently.

-Commented out unused variables msgcnt and buf

-Updated client usage message.
usage %s <ip_addr> <port> <filename>

-Deleted function compareStrings, using c standard library strcmp() instead.