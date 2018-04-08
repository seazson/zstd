#! /bin/bash

echo "****************************echo***********************************"
#关于echo：echo后的字符串可加，也可不加单双引号
    #在echo中用到了一种引号，需要用另一种引号将它们括起来
	#单双引号都可以表示字符串。当字符串里有特殊字符时（如引号）需要使用单、双引号。配对的单双引号本身不会给变量。
	#echo时有空格可用可不用引号，但是赋值时需要加上引号。echo不用加@，shell默认不会回显。
	echo this's is it's        #引号会被忽略掉
#	echo this's is             #只用了一个引号是错误的
	echo "this's is it's"      #正确方式

	# -n去掉自动回车
	echo -n "this's is"        
	echo " me"

echo "****************************val************************************"
#基本语法
	#引用变量：{}防止字符被拆开解释,\$用来转义$
	echo $PATH      #以下四种用法相同
	echo "$PATH"
	echo ${PATH}
	echo "${PATH}"
	echo "it cost \$15"
	
	#变量赋值：变量名区分大小写，=号前后不能有空格。 ()当命令使用, ` `也是执行命令, 
	v1=""pwd""
	v2=${v1}
	v3=$(pwd)
	v4=$($v1)
	v5=`date`
	v6="I L U"  #变量赋值字符串时要加引号，不然会将第一个空格之后的当命令.会报找不到L这个命令
	echo v1=$v1 v2=${v2} v3=$v3 v4=$v4 v5=$v5 v6=$v6

echo "****************************expr***********************************"
#整数运算。
	#括号内可以没有空格
	t=$((1+5*5))
	tt=$[1+5*5]
	#这个必须有空格,特殊字符需要转义
	ttt=`expr 1 + 5 \* 5`
	echo $t $tt $ttt

#浮点运算，使用bc命令
	tf=`echo "13.5 * 16.7" | bc`

	tff=`bc << EOF 
	scale = 4
	a1 = ($t * $tt)
	a1 * 16.325
	`
	echo "float:" $tf $tff

echo "****************************if*************************************"
#if流程.if后面必须是一个命令.命令退出码是0才执行then
#[ condision ]  ：前后有空格，相当于test condision。包括数值大小比较，字符串大小比较，文件属性比较。
#(( 1 + 2 * 3 ))：允许简单数学运算
#[[ $val = *f ]]: 允许正则表达式	
	#标准if的两种写法
	if pwd
	then
		echo "1 if turn 0"
	fi

	if pwd; then
		echo "2 if turn 0"
	fi
	
	#标准if-else
	if test 1 -ge 2
	then
		echo "3 if cmd return 0"
	else
		echo "3 if cmd return not 0"
	fi
	
	#if-elif-else流程，if后指令会挨个执行，只有第一个返回0的指令的then才执行
	if [ "haha" > "wowo" ] || [ "haha" > "wowo" ]
	then
		echo "4 if cmd return 0"
	elif ((1))
	then
		echo "4 elif cmd return 0"
	else
		echo "4 if cmd return not 0"
	fi

#test格式: test condition = [ condition ]
#复合测试：[ condition ] || [ condition ] && [condition]
#数值比较：-eq -ge -gt -le -lt -ne
#字符串比较: = != < > -n -z
#文件比较: -d -e -f -r -s -w -x -O -G -nt -ot
	
echo "****************************case***********************************"
#case流程.注意双分号结束。
	case $USER in
		zengyang | phy)
			echo "This is z";;
		sea)
			echo "sea";;
		*)
			echo "USER $USER";;
	esac

echo "****************************for************************************"
#for流程.
	#in后面的列表3中方式：字符、变量、命令, 默认用空格分开	
	for list in I don't know if this'll work don\'t know "this'll" #需要转义
	do
		echo 1word is $list
	done	
    echo after list=$list   #最后list还会保持最后的值

	val="I don't know if this'll work"
	for list in $val
	do
		echo 2val is $list
	done	

	for list in `ls`
	do
		echo 3cmd is $list
	done	
	
	#修改默认分隔符
	IFSOLD=$IFS
	IFS=$'\n'   #只识别换行符
	val="I don't know if this'll work"
	for list in $val
	do
		echo 2val is $list
	done	
	IFS=$IFSOLD
	
	#类C的for循环。变量赋值可以后空格。空格比较随意，都可以添加。
	for((i=1;i<=10;i++))
	do
		echo i=$i
	done	

	for((i=1,j=10;i<=10;i++,j--))
	do
		if [ $i -eq 5 ]
		then
			break            #跳出循环 
			continue         #继续循环
		fi
		echo i=$i j=$j
	done > 1.txt             #可以用done > 1.txt重定向到文件中	

echo "****************************while************************************"
#while流程.
	val=10
	while [ $val -gt 0 ] #while和[之间要有空格，不然把while[当成一个命令
	do
		echo val=$val
		val=$[ $val - 1 ]
	done

	val=10
	while echo $val       #多个测试命令，只有最后一个决定
	      [ $val -gt 0 ] 
	do
		echo val=$val
		val=$[ $val - 1 ]
	done

echo "****************************until************************************"
#until流程.
	var=10
	until [ $var -eq 1 ]
	do
		echo var=$var
		var=$[ $var - 1 ]
	done


	
echo "****************************exit***********************************"
#退出状态：
# 		0       命令成功结束
# 		1       通用未知错误
# 		2       误用shell命令
# 		126     命令不可执行
# 		127     没找到命令
# 		128     无效退出命令
# 		128+x   linux信号x的严重错误
# 		130     命令通过ctrl+c终止
# 		255     退出状态码越界
	echo "last program exit stauts : $?"
	exit 5
	
	
	
	