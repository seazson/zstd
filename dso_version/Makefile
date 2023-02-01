all:lib
	gcc -g -o main main.c -lver -L./

lib:
	gcc -g -shared -o libver.so libver.c -fPIC -Wl,--version-script,libver.map	

lib_v1:
	gcc -g -shared -o libver.so libver_v1.c -fPIC -Wl,--version-script,libver_v1.map -Wl,-soname,libver.so.1	
