//---------------------------------------------------------------------------------------------
//our functions
//---------------------------------------------------------------------------------------------

#include "exynos_driver.h"




/*void EXYNOSDispInfoVblankHandler(int fd, unsigned int frame, unsigned int tv_sec,
                     unsigned int tv_usec, void *event_data){

}*/

int ctrl_WriteEventsToClient = 0;

void WriteEventsToClient(ClientPtr pClient, int count, xEvent *events){
	ctrl_WriteEventsToClient = 1;
}

/*void exynosVideoOverlayVblankEventHandler (unsigned int frame, unsigned int tv_sec,
        unsigned int tv_usec, void *event_data){
}*/






