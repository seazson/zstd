#!/bin/bash

#############################命令行参数#################################
# $0         : 程序名，shift时也不会变
# $1~$9      : 1到9个参数
# ${10}~${n} : 10到n个参数
# $#         : 参数个数，不包括程序名
# $*         : 所有参数。当成一个字符串
# $@         : 所有参数。由空格分开的好多个。
# $$         : pid
name=`basename $0`                    #获取不带路径名
echo "program whole name: $0 pid=$$"
echo "program name: $name"

echo "number of args : $#"

count=1
while [ $# -ge $count ]
do
	echo "\$# arg $count = ${!count}"     #要在花括号内引用变量需要使用感叹号
	count=$[ $count + 1 ]
done

count=1
for list in "$*"                      #注意不带""时$*和$@是一样的
do
	echo "\$* args $count = $list" 
	count=$[ $count + 1 ]
done	

count=1
for list in "$@"
do
	echo "\$@ args $count = $list" 
	count=$[ $count + 1 ]
done	

#while [ -n "$1" ]
#do
#	echo "shift args $1"              #shift后参数会永远丢弃
#	shift
#done

#############################命令行选项#################################
# getopts 一个个处理，能分辨参数中的空格
# getopt 一起处理，相当于格式化
while getopts :ab:cd opt
do
	case "$opt" in
	a) echo "Found -a opt";;
	b) echo "Found -b opt,with val $OPTARG";;
	c) echo "Found -c opt";;
	d) echo "Found -d opt";;
	*) echo "Found unkonw $opt";;
	esac
done

shift $[ $OPTIND - 1 ]

count=1
for param in "$@"
do
	echo "Param $count: $param"
	count=$[ $count + 1 ]
done

#############################用户输入###################################
# read -p 自带输出
# read 后不跟变量默认输入到REPLY
# read -t x 等待xs
# read -nx 读取x个字符
# read -s 不回显
echo -n "Enter you name: "
read name
echo "Hello $name"

read -p "Enter you name2: " name   
echo "Hello $name"

read -p "Enter you name3: "
echo "Hello $REPLY"

if read -t 5 -p "Please enter in 5s:" name
then
	echo "Yea $name"
else
	echo "too slow"
fi

read -n3 -p "Please enter 3 word:" name

read -s -p "Please enter passwd:" name
echo "you enter $name"


##############################重定向####################################
#重定向错误 2> file 中间不能有空格
#重定向输出 1> file
#重定向输出和错误 &> file

echo "this is error" >&2   #重定向到文件描述符时需要加&

#在脚本中永久重定向.执行一次exec相当于打开一次fd
# 输出 exec 1> file
# 输入 exec 0< file 
# 读写 exec 5<> file
# 关闭 exec 5>&- 
exec 1> out.txt
exec 2> err.txt

echo "write to out"
echo "write to err" >&2

exec 5> fd5.txt            #创建自己的文件描述符
echo "write to ourself file" >&5
echo "add more" >&5                #追加一句
exec 5>&-                  #关闭文件描述符

exec 5>>fd5.txt            #以追加方式创建
echo "add once again" >&5

#临时修改标准std
exec 3>&1
exec 1>out.txt
echo "modify stdout"
exec 1>&3


