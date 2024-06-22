#ifndef MY_CHAR_H
#define MY_CHAR_H

#include <asm/ioctl.h>

#define MY_CHAR_MAGIC 'k'

#define MYCHAR_IOCTL_GET_MAXLEN _IOR(MY_CHAR_MAGIC,1,int*)
#define MYCHAR_IOCTL_GET_CURLEN _IOR(MY_CHAR_MAGIC,2,int*)


#endif
