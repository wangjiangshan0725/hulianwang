#!/bin/bash


funwithsub()
{
count=`ls -lrt|grep eml|wc -l`
#echo $count
for ((i=1;i<=$count;i++))
do

sub=`cat $i.eml |grep Subject|grep -v From|awk -F ':' '{print $2}'`
echo "第$i封邮件主题为$sub">>temps.txt


done 
}

funwithbase()
{
line1=`cat temp.eml |grep  -n -|grep base64|awk -F ':' '{print $1}'|head -1`
line2=`cat temp.eml |grep  -n -|grep NextPart|grep -v boundary|tail -2|head -1|awk -F ':' '{print $1}'`
line3=$(($line1+1))
line4=$(($line2-1))
`sed -n ''${line3}','${line4}'p' temp.eml>temp.txt`
#echo $line1
#echo $line2
`cat temp.txt|tail -2|head -1>temp.txt`


}

funwithsearch()
{
count=`ls -lrt |grep z|wc -l`
text=`cat sear.txt|tr -d '\r'`

for ((i=1;i<=$count;i++))
do
t=`grep $text  $i.z`
if [ -n "$t" ]; then
   echo "邮件在第"$i"封存在">>req.txt
fi

done

text1=`cat req.txt`
if [ -n "$text1" ]; then
  echo  "已经找到邮件了">>req.txt
else
  echo "没有找到相关邮件~~~~~~">>req.txt
fi

}

case $1 in
sub)
    funwithsub
    ;;
base)
    funwithbase 
    ;;
sea)
    funwithsearch
    ;;
esac
