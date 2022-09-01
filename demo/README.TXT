/* gfs 
 * qq: 25088970 maohw
 */

arm-hisiv500-linux-gcc -o tt *.c -I../inc -L../lib -lgfs -lpthread

export LD_LIBRARY_PATH=./
./tt

