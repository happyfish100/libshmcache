
TARGET_PREFIX=$DESTDIR/usr
LIB_VERSION=lib64

if [ -f /usr/include/fastcommon/_os_define.h ]; then
  OS_BITS=$(fgrep OS_BITS /usr/include/fastcommon/_os_define.h | awk '{print $NF;}')
elif [ -f /usr/local/include/fastcommon/_os_define.h ]; then
  OS_BITS=$(fgrep OS_BITS /usr/local/include/fastcommon/_os_define.h | awk '{print $NF;}')
else
  OS_BITS=64
fi

if [ "$OS_BITS" -eq 64 ]; then
  LIB_VERSION=lib64
else
  LIB_VERSION=lib
fi

uname=$(uname)
if [ "$uname" = "Linux" ]; then
  LIBS=
elif [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
  if [ "$uname" = "Darwin" ]; then
    out=$(echo $TARGET_PREFIX | fgrep local)
    if [ $? -ne 0 ]; then
      TARGET_PREFIX=$TARGET_PREFIX/local
    fi
    LIB_VERSION=lib
  fi
elif [ "$uname" = "SunOS" ]; then
  export CC=gcc
elif [ "$uname" = "AIX" ]; then
  export CC=gcc
fi

files=$(locate jni.h)
if [ -z "$files" ]; then
  echo "can't locate jni.h, please install java SDK first."
  exit 2
fi

count=$(echo "$files" | wc -l)
if [ $count -eq 1 ]; then
  filename=$files
else
  i=0
  for file in $files; do
     i=$(expr $i + 1)
     echo "$i. $file"
  done

  printf "please input the correct file no.: "
  read n
  if [ -z "$n" ]; then
     echo "invalid file no."
     exit 2
  fi

  filename=$(echo "$files" | head -n $n | tail -n 1)
fi

INCLUDES=
path=$(dirname $filename)
for d in $(find $path -type d); do
  INCLUDES="$INCLUDES -I$d"
done

sed_replace()
{
    sed_cmd=$1
    filename=$2
    if [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
       sed -i "" "$sed_cmd" $filename
    else
       sed -i "$sed_cmd" $filename
    fi
}

cp Makefile.in Makefile
sed_replace "s#\\\$(INCLUDES)#$INCLUDES#g" Makefile
sed_replace "s#\\\$(LIBS)#$LIBS#g" Makefile
sed_replace "s#\\\$(TARGET_PREFIX)#$TARGET_PREFIX#g" Makefile
sed_replace "s#\\\$(LIB_VERSION)#$LIB_VERSION#g" Makefile
make $1 $2 $3

if [ "$1" = "clean" ]; then
  /bin/rm -f Makefile
fi

