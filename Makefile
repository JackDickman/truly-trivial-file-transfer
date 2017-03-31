PORT = 6666
FILE = 513bytes.bin #some file name in the server directory
SERVER = localhost
LOCALHOST = localhost
FILE_S = file-on-server.txt
FILE_C = file-on-client.txt

all:
	@echo "Targets: build, basic-test, ext-test, clean, submit"

build:
	make ttftp

ttftp: ttftp.c client.c server.c
	gcc ttftp.c client.c server.c -o ttftp

basic-test: ttftp
	echo `date` > ${FILE_S}
	./ttftp -L ${PORT} &
	@echo "server started"  
	./ttftp -h ${SERVER} -f ${FILE_S} ${PORT} > ${FILE_C}
	diff ${FILE_S} ${FILE_C}
	@echo DONE

ext-test:
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 0bytes.bin ${PORT} > f.out
	cmp 0bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 1bytes.bin ${PORT} > f.out
	cmp 1bytes.bin f.out	
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 1bytezero.bin ${PORT} > f.out
	cmp 1bytezero.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 16bytezero.bin ${PORT} > f.out
	cmp 16bytezero.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 1535bytes.bin ${PORT} > f.out
	cmp 1535bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 1536bytes.bin ${PORT} > f.out
	cmp 1536bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 1537bytes.bin ${PORT} > f.out
	cmp 1537bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 511bytes.bin ${PORT} > f.out
	cmp 511bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 512bytes.bin ${PORT} > f.out
	cmp 512bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 513bytes.bin ${PORT} > f.out
	cmp 513bytes.bin f.out
	./ttftp -L ${PORT} &
	./ttftp -h ${SERVER} -f 513byteszero.bin ${PORT} > f.out
	cmp 513byteszero.bin f.out
	@echo DONE

submit:
	svn commit -m "submitting proj4"

clean:
	rm ttftp *~ ${FILE_S} ${FILE_C} f.out
