#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "limits.h"
#include "scsi.h"

extern dev_t scsi_majors[];

int main (int argc, char **argv) {

	char *device_name;
	int i = 0;
	dev_t device;
   char c1,c2;

   device_name = malloc(256);
   for (c1='a'; c1<='z'; c1++) {
   for (i=1; i<16; i++) {
   	sprintf(device_name, "sd%c%d", c1, i);
   	device = lookup_scsi_device_number(device_name);
   	printf("0x%04x", device);
   	printf("  %s", device_name);
      sprintf(device_name, "invalid");
		lookup_scsi_device_name(device_name, device);
		printf("  %s\n",device_name);
	}
   }
	for (c2='a'; c2<'j'; c2++) {
   for (c1='a'; c1<='z'; c1++) {
   for (i=1; i<16; i++) {
   	sprintf(device_name, "sd%c%c%d", c2, c1, i);
   	device = lookup_scsi_device_number(device_name);
   	printf("0x%04x", device);
   	printf("  %s", device_name);
      sprintf(device_name, "invalid");
		lookup_scsi_device_name(device_name, device);
		printf("  %s\n",device_name);
   }
   }
   }
   
}
