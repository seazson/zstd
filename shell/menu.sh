#! /bin/bash

diskspace(){
	clear
	df -k
}

whoseon(){
	clear
	who
}

meminfo(){
	clear
	cat /proc/meminfo
}

menu(){
	clear
	echo
	echo -e "\t   Sys Admin Menu\n"
	echo -e "\t1.Display disk space"
	echo -e "\t2.Display logged on users"
	echo -e "\t3.Display memory usage"
	echo -e "\t0.Exit program"
	echo -en "\tEnter option:"
	read -n 1 option
}

while [ 1 ]
do
	menu
	case $option in
	0)
		break;;
	1)
		diskspace;;
	2)
		whoseon;;
	3)
		meminfo;;
	*)
		clear
		echo "wrong selection";;
	esac
	echo -en "\n\n\t\tHit any key to continue"
	read -n 1 line
done
clear


#使用select
PS3="Enter opt: "
select opt in "Display disk space" "Display logged on users" "Display memory usage" "Exit program"
do
	case $opt in
	"Exit program")
		break;;
	"Display disk space")
		diskspace;;
	"Display logged on users")
		whoseon;;
	"Display memory usage")
		meminfo;;
	*)
		clear
		echo "wrong args"
	esac
done
clear


#dialog
dialog --title Testing --msgbox "This is test" 10 20

dialog --title "Please answer" --yesno "Is vi?" 100 20
if [ $? -eq 0 ]
then
	echo "yes is vi"
fi

dialog --inputbox "Enter you name:" 20 50 2>name.txt
if [ $? -eq 0 ]
then
	name=`cat name.txt`
	echo "your name is $name"
fi

dialog --textbox /etc/passwd 15 45

dialog --menu "Sys Admin Menu" 20 30 10 1 "Display disk space" 2 "Display logged on users" 3 "Display memory usage" 4 "Exit program" 2>test.txt

dialog --title "Select a file" --fselect $HOME/ 10 50 2>name.txt