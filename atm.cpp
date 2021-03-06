/**
    Some code from binarytides used as reference
*/
#include<iostream>    //cout
#include<fstream>
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<string>  //string
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent
#include<cstdlib>
#include"aeswrapper.h" //encrypt_send, decrypt_receive
using namespace std;

/**
    TCP Client class
*/
class tcp_client
{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;

public:
    tcp_client();
    bool conn(string, int);
    bool send_data(string data);
    string receive(int);
};

tcp_client::tcp_client()
{
    sock = -1;
    port = 0;
    address = "";
}

/**
    Connect to a host on a certain port number
*/
bool tcp_client::conn(string address , int port)
{
    //create socket if it is not already created
    if(sock == -1)
    {
        //Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            perror("Could not create socket");
        }

        cout<<"Socket created\n";
    }
    else    {   /* OK , nothing */  }

    //setup address structure
    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *he;
        struct in_addr **addr_list;

        //resolve the hostname, its not an ip address
        if ( (he = gethostbyname( address.c_str() ) ) == NULL)
        {
            //gethostbyname failed
            herror("gethostbyname");
            cout<<"Failed to resolve hostname\n";

            return false;
        }

        //Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
        addr_list = (struct in_addr **) he->h_addr_list;

        for(int i = 0; addr_list[i] != NULL; i++)
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];

            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;

            break;
        }
    }

    //plain ip address
    else
    {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }

    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    cout<<"Connected\n";
    return true;
}

/**
    Send data to the connected host
*/
bool tcp_client::send_data(string data, string key)
{
    encrypt_send(key, data);
    //Send some data
    if( send(sock , data.c_str() , strlen( data.c_str() ) , 0) < 0)
    {
        perror("Send failed : ");
        return false;
    }
    cout<<"Data sent\n";

    return true;
}

/**
    Receive data from the connected host
*/
string tcp_client::receive(int size=512)
{
    char buffer[size];
    string reply;

    //Receive a reply from the server
    if( recv(sock , buffer , sizeof(buffer) , 0) < 0)
    {
        puts("recv failed");
    }

    reply = buffer;
    decrypt_receive(key, reply);
    return reply;
}

int main(int argc , char *argv[])
{
    tcp_client c;
    string host;
    //changeme
    host = "localhost";
    int port;
    if (argc ==  2){
      port = atoi(argv[1]);
    }
    else {
      cout << "bad port" << endl;
      return 1;
    }
	
	c.conn(host , port);
	while(1){
	string input;
	string bank_msg = "";
    cout << "Please input your card number: " << endl;
	cin >> input;
	std::string line;
	std::string username = "abc";
  	std::ifstream myfile ((input + ".card").c_str());
	  if (myfile.is_open())
	  {
		while ( getline (myfile,line) )
		{
			username = line;
		}
		myfile.close();
	  }
	  else std::cout << "Unable to open card\n"; 
	  
	if (username != "abc"){
    	//connect to host
		c.send_data("login[" + username + "]");
		cout<<"----------------------------\n\n";
		cout<<c.receive(1024);
		cout<<"\n\n----------------------------\n\n";
		
		while(bank_msg!="Logging out"){
		  //send some data
		  //SEND RSA PUBLIC THEN RECEIVE BANK'S RSA
			cin >> input;		
			c.send_data(input, key);
		  	//receive and echo reply
		  	cout<<"----------------------------\n\n";
			bank_msg = c.receive(1024);
		  	cout<<bank_msg;
		  	cout<<"\n\n----------------------------\n\n";
			
		}
	 }
	}
    //done
    return 0;
}
