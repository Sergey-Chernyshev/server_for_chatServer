#include <iostream>
#include <string>

using namespace std;

string GetIpWhithoutPort(sockaddr_in clientAddr)
{
	string ip = clientAddr;
	cout << ip << endl;


};

int main()
{
	GetIpWhithoutPort("192.168.1.71:7389")
	
}