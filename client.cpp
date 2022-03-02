/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
	Original author of the starter code
	
	Please include your name and UIN below
	Name: Khunnapat Reanpongnam	
	UIN: 828004748
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>

using namespace std;


int main(int argc, char *argv[]) {	
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool channel = false;
	int bufCap = 256; // can manually change here
	bool time = false, person = false, ecg = false;

	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c:m")) != -1) {
		switch (opt) {				
			case 'p':
				p = atoi (optarg);
				person = true;
				break;
			case 't':
				t = atof (optarg);
				time = true;
				break;
			case 'e':
				e = atoi (optarg);
				ecg = true;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				channel = true;
				break;				
			case 'm':
				bufCap = atoi(optarg);
				break;					
		}
	}

	//4 use fork and exec to run server and clinet in 1 terminal
	//char* args[] = {"./server", "-m", (char*)to_string(bufCap).c_str(), NULL};
	int pid = fork();
    if (pid == 0) {
		char* args[] = {"./server", NULL};
		execvp(args[0], args);
    }

	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);	


	// sending a non-sense message, you need to change this
	//char buf [MAX_MESSAGE]; // 256
	datamsg x (p, t, e);	
	double reply;
	int nbytes;

	if (time == true && person == true && ecg == true && filename == "") {
		chan.cwrite (&x, sizeof (datamsg)); // question
		nbytes = chan.cread (&reply, bufCap); //answer
		cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
	}
	else if (person == true && ecg == true && time == false) {
		//1 Check for p and then take data and write to x1.csv
		struct timeval sTime;
		struct timeval eTime;
		gettimeofday(&sTime, NULL);
		ofstream file;
		file.open("x1.csv");
		for (int i = 0; i < 1000; i++) {
			x.ecgno = 1;
			chan.cwrite (&x, sizeof (datamsg));
			nbytes = chan.cread (&reply, bufCap);
			double ecg1 = *(double *) &reply;
			x.ecgno = 2;
			chan.cwrite (&x, sizeof (datamsg));
			nbytes = chan.cread (&reply, bufCap);
			double ecg2 = *(double *) &reply;
			file << x.seconds << "," << ecg1 << "," << ecg2 << "\n";
			x.seconds += .004;
		}
		file.close();
		gettimeofday(&eTime, NULL);
		cout << "Collected 1000 data points in " << (double)((eTime.tv_sec * 1000000 + eTime.tv_usec) - (sTime.tv_sec * 1000000 + sTime.tv_usec)) / 1000000 << " seconds." << endl;
	}
	else if (filename != "") {
		//2 request file and then put received file in recieved directory
		//also measure time for transfer and put result in report as chart treat file as binary
		struct timeval sTime;
		struct timeval eTime;
		gettimeofday(&sTime, NULL);
		string location = "received/" + filename;	
		FILE *f;
		f = fopen(location.c_str(), "w");
		filemsg fm (0,0);
		int len = sizeof (filemsg) + filename.size()+1;
		char buf2 [len];
		memcpy (buf2, &fm, sizeof (filemsg));
		strcpy (buf2 + sizeof (filemsg), filename.c_str());
		chan.cwrite (buf2, len);  // I want the file length;
        __int64_t fLen;
        chan.cread(&fLen, sizeof(__int64_t));

        cout << "File length: " << fLen << endl;
		int getLen = ceil((double)fLen/bufCap);
		int remaining = (int) fLen;
		char* recBuf = new char[bufCap];
			
		for (int i = 0; i < getLen; i++) {
			fm.offset = i * bufCap;
			if (remaining >= bufCap) {
				fm.length = bufCap;
			}
			else {
				fm.length = remaining;
			}
			cout << "transferring chunk" << endl;
			memcpy (buf2, &fm, sizeof (filemsg));
			strcpy (buf2 + sizeof (filemsg), filename.c_str());
			chan.cwrite(buf2, sizeof(buf2));
			double bytes = 0;
			while(bytes != fm.length) {
				int currentBytes = chan.cread(recBuf, fm.length-bytes);
				fwrite(recBuf, sizeof(char), currentBytes/sizeof(char), f);
				bytes += currentBytes;
			}
			remaining = remaining - fm.length;
		}
		fclose(f);
		gettimeofday(&eTime, NULL);
		cout << "transferred file " << filename << " in " << (double)((eTime.tv_sec * 1000000 + eTime.tv_usec) - (sTime.tv_sec * 1000000 + sTime.tv_usec)) / 1000000 << " seconds." << endl;
	}

	if (channel) {
		//3 create new channel and communicate with it
		MESSAGE_TYPE m = NEWCHANNEL_MSG;
		char buf2 [30];
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		chan.cread(buf2, 30);
		string name = buf2;
		cout << "New channel: " << name << endl;
		FIFORequestChannel newChan (name, FIFORequestChannel::CLIENT_SIDE);
		newChan.cwrite (&x, sizeof (datamsg));
		nbytes = newChan.cread (&reply, bufCap);
		x.person = p;
		x.seconds = t;
		x.ecgno = e;
		cout << "Channel: " << name << " | For person " << x.person << ", at time " << x.seconds << ", the value of ecg " << x.ecgno << " is " << reply << endl;
		// can do file transfer too but not required for demonstration
		m = QUIT_MSG;
		newChan.cwrite (&m, sizeof (MESSAGE_TYPE));
	}

	//5 make sure channels are closed at the end
	// closing the channel    
	MESSAGE_TYPE m = QUIT_MSG;
	chan.cwrite (&m, sizeof (MESSAGE_TYPE));	
	
	wait(NULL);
}
