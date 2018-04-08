#! /bin/bash

##########################定义函数的两种形式########################################
function fun1 {                          #大括号前要有空格
	echo "this fun1"
}

fun2(){                                  #括号无特定要求
	echo "this fun2"
	return 55
}

fun3() {                                  
	echo "this fun2"
	echo "val"
}

fun4() {                                  
	echo "fun=$0"
	echo "parm1=$1"
	echo "parm2=$2"
}

fun1
echo "fun1 return $?"                    #有最后一条语句决定退出状态

fun2
echo "fun2 return $?"                    #用return返回数据

name=`fun3`
echo "fun3 return $name"                 #用两个echo返回

fun4 w h                                 #参数的使用

#库使用
#. ./input.sh                             #在当前shell环境下引入新的shell

#数组使用
dbl(){
	local origin
	local new
	local element
	local i
	origin=(`echo "$@"`)
	new=(`echo "$@"`)
	element=$[ $# - 1]
	for(( i = 0; i<= $element; i++ ))
	{
		new[$i]=$[ ${origin[$i]} * 2]
	}
	echo ${new[*]}
}

myarray=(1 2 3 4 5)
echo "The origin is: ${myarray[*]}"
arg1=`echo ${myarray[*]}`
result=(`dbl $arg1`)                       #传入数组
echo "The result is: ${result[*]}"
