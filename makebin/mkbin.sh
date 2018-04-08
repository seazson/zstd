#/bin/sh

flashname=image

bootname=u-boot.bin
mspname=msp_823v2.axf
cspname=kernel-linux_2.6.22.19-4.07.0-M82331.zImage
fsname=voip

bootmtd=$(( (256+128+128)*1024 ))   #boot area 256+128+128k = 524288
mspmtd=$(( (2*1024+256)*1024 ))     #msp area 2.256M = 2359296
cspmtd=$(( (1*1024+256)*1024 ))      #csp area 1.256M = 1310720
fsmtd=$(( 4*1024*1024 ))            #fs area 4M = 4194304

echo "make voip flash image"
rm -rf $flashname

	bootsize=`stat -c "%s" $bootname`
	bootbank=`expr $bootmtd - $bootsize`
	echo "making bootmtd totalsize:${bootmtd} use:${bootsize} bank:${bootbank}...."
#	dd if=/dev/zero of=$bootname.bank count=$bootbank bs=1 > /dev/null
	./fillfile $bootname.bank 0xff $bootbank
	cat $bootname $bootname.bank >> $flashname
	rm -rf $bootname.bank
	
	mspsize=`stat -c "%s" $mspname`
	mspbank=`expr $mspmtd - $mspsize`
	echo "making mspmtd totalsize:${mspmtd} use:${mspsize} bank:${mspbank}...."
#	dd if=/dev/zero of=$mspname.bank count=$mspbank bs=1
	./fillfile $mspname.bank 0xff $mspbank
	cat $mspname $mspname.bank >> $flashname
	rm -rf $mspname.bank
	
	cspsize=`stat -c "%s" $cspname`
	cspbank=`expr $cspmtd - $cspsize`
	echo "making cspmtd totalsize:${cspmtd} use:${cspsize} bank:${cspbank}...."
#	dd if=/dev/zero of=$cspname.bank count=$cspbank bs=1
	./fillfile $cspname.bank 0xff $cspbank
	cat $cspname $cspname.bank >> $flashname
	rm -rf $cspname.bank

	fssize=`stat -c "%s" $fsname`
	fsbank=`expr $fsmtd - $fssize`
	echo "making fsmtd totalsize:${fsmtd} use:${fssize} bank:${fsbank}...."
#	dd if=/dev/zero of=$fsname.bank count=$fsbank bs=1
	./fillfile $fsname.bank 0xff $fsbank
	cat $fsname $fsname.bank >> $flashname
	rm -rf $fsname.bank

	echo "making flash $flashname done" 