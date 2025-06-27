cambricon@ubuntu20:/data/source/zstd/cpython$ python3 setup.py build
running build
running build_ext
building 'Test' extension
x86_64-linux-gnu-gcc -pthread -Wno-unused-result -Wsign-compare -DNDEBUG -g -fwrapv -O2 -Wall -g -fstack-protector-strong -Wformat -Werror=format-security -g -fwrapv -O2 -g -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -fPIC -I/usr/include/python3.8 -c test.c -o build/temp.linux-x86_64-3.8/test.o
creating build/lib.linux-x86_64-3.8
x86_64-linux-gnu-gcc -pthread -shared -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-Bsymbolic-functions -Wl,-z,relro -g -fwrapv -O2 -Wl,-Bsymbolic-functions -Wl,-z,relro -g -fwrapv -O2 -g -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 build/temp.linux-x86_64-3.8/test.o -o build/lib.linux-x86_64-3.8/Test.cpython-38-x86_64-linux-gnu.so

cambricon@ubuntu20:/data/source/zstd/cpython$ sudo python3 setup.py install
running install
running build
running build_ext
running install_lib
copying build/lib.linux-x86_64-3.8/Test.cpython-38-x86_64-linux-gnu.so -> /usr/local/lib/python3.8/dist-packages
running install_egg_info
Writing /usr/local/lib/python3.8/dist-packages/Test-1.0.egg-info


>> import Test
>>> help(Test)
>>> Test.add_one(1)

