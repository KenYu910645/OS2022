#include "syscall.h"

main()
        {
                //int n[256];
                //int* n_ptr = &n[0];
                //PrintInt(n_ptr);
                //for (n[128]=20;n[128]<=25;n[128]++)
                //        PrintInt(n[128]);
                //PrintInt(n[128]*100); 
                // busy waiting for other process
                // for(n[200]=0;n[200]<=9999999;n[200]++);
                int n;
                for(n=20;n<=25;n++){
                    PrintInt(n);
                }
        }
