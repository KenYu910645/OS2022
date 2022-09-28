#include "syscall.h"
main()
	{
                //int i[256];
                //int* i_ptr = &i[0];
                //PrintInt(i_ptr);
		//for (i[99]=9;i[99]>5;i[99]--){
		//	PrintInt(i[99]);
                // }
                //PrintInt(i[99]*100);
                // busy waiting for other process
                // for(i[100]=0;i[100]<=9999999;i[100]++);
                int i;
                for (i=9;i>5;i--){
                    PrintInt(i);
                }
	}
