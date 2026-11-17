#include <stdio.h>
#include <time.h>

int main(){
	time_t timeUTC;
	time(&timeUTC);

	time_t timePST = timeUTC - 28800;
	time_t timePDT = timeUTC - 25200;
	
	struct tm *tPST;
	tPST = gmtime(&timePST);
	char buffpst[50];
	strftime(buffpst, sizeof(buffpst), "%d.%m.%Y %H:%M:%S", tPST);
	printf ("time in california rn (PST): %s\n", buffpst);

	struct tm *tPDT;
	tPDT = gmtime(&timePDT);
	char buffpdt[50];
	strftime(buffpdt, sizeof(buffpdt), "%d.%m.%Y %H:%M:%S", tPDT);
	printf ("time in california rn (PDT): %s\n", buffpdt);
	
	return 0;
}
