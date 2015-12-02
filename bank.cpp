#include "userinfo.cpp"
#include <mutex>
/*
** bank.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <sys/mman.h>
#include <pthread.h>



#define BACKLOG 10     // how many pending connections queue will hold
std::map<std::string, userInfo> users;
int attempts = 0;
std::chrono::time_point<std::chrono::system_clock> last_reset;
pthread_mutex_t attempts_lock;
pthread_mutex_t users_lock;

void reset_attempts(){
	pthread_mutex_lock(&attempts_lock);
	attempts = 0;
	last_reset = std::chrono::system_clock::now();
	pthread_mutex_unlock(&attempts_lock);

}

int user_session(int new_fd, userInfo& user){
  char buffer[30];
  while(1){
    bzero(buffer,30);
    int n = read(new_fd,buffer,30);
    if (n > 0){
      //parsing

      //balance
      if( strncmp("balance\n", buffer, 7)==0){
				pthread_mutex_lock(&users_lock);
        std::string s = std::to_string(user.get_balance());
				pthread_mutex_unlock(&users_lock);
        char const *pchar = s.c_str();
        if (send(new_fd, pchar, s.length() + 1, 0) == -1)
            perror("send");
        continue;
      }
      //withdraw
      else if(strncmp("withdraw[\n", buffer, 9)==0){
        char amount[n-10];
        //overflow check
        if (n > 18 || strncmp("-\n", buffer + 9, 1) == 0){
          if (send(new_fd, "Pick a legitimate number buddy", 28, 0) == -1)
              perror("send");
          continue;
        }
        strncpy(amount, buffer + 9, n-10);
        int temp = atoi(amount);
        temp = temp * -1;
				pthread_mutex_lock(&users_lock);
        int error = user.add_balance(temp);
				pthread_mutex_unlock(&users_lock);
        //overflow(too high)
        if (error > 0){
          if (send(new_fd, "Your balance is too high with the new number. Start a new bank account!", 72, 0) == -1)
              perror("send");
          continue;
        }
        //not enough balance
        else if(error < 0){
          if (send(new_fd, "Insufficient funds", 19, 0) == -1)
              perror("send");
          continue;
        }
				pthread_mutex_lock(&users_lock);
        std::string s = std::to_string(user.get_balance());
				pthread_mutex_unlock(&users_lock);
        char const *pchar = s.c_str();
        //if (send(new_fd, "New Balance:", 13, 0) == -1)
            //perror("send");
        if (send(new_fd, pchar, s.length() + 1, 0) == -1)
            perror("send");
        continue;

      }
      //transfer
      else if (strncmp("transfer[\n", buffer, 9)==0){

        //find index of first ]
        char * pch;
        pch=strchr(buffer,']');
        int index = pch-buffer + 1;

        char amount[index-9];
        if (index-9 > 8 || strncmp("-\n", buffer + 9, 1) == 0){
          if (send(new_fd, "Pick a legitimate number buddy", 28, 0) == -1)
              perror("send");
          continue;
        }
        strncpy(amount, buffer + 9, index-9);
        int temp = atoi(amount);
        int temp2 = temp * -1;
        char username2[n - (index + 2)];
        strncpy(username2, buffer + index + 1, n - (index + 2));
        printf("username2 %s\n",username2);
        std::map<std::string, userInfo>::iterator it;
        std::string usr(username2);
				pthread_mutex_lock(&users_lock);
        it = users.find(usr);
				pthread_mutex_unlock(&users_lock);
        if(it != users.end()){
				pthread_mutex_lock(&users_lock);
        userInfo &user2 = it->second;
        int error = user.error_check(temp2);
        int error2 = user2.error_check(temp);
				pthread_mutex_unlock(&users_lock);
        //not enough balance
        if(error < 0){
          if (send(new_fd, "Insufficient funds", 19, 0) == -1)
              perror("send");

          continue;
        }
        if (error2 > 0){
          if (send(new_fd, "Their balance is too high with the new number. Tell them to start a new bank account!", 86, 0) == -1)
              perror("send");

          continue;
        }
				pthread_mutex_lock(&users_lock);
        user.add_balance(temp2);
        user2.add_balance(temp);
				pthread_mutex_unlock(&users_lock);
        if (send(new_fd, "Sent the $$$$", 14, 0) == -1)
            perror("send");
        continue;
        }
        else {
          //not a real username
          if (send(new_fd, "The user you have entered does not exist. Maybe they died?", 60, 0) == -1)
              perror("send");
          continue;
        }
        continue;
      }
      //logout
        else if(strncmp("logout\n", buffer, 6)==0){
          if (send(new_fd, "Logging out", 12, 0) == -1) //do not change string
              perror("send");
          return 1;
        }
        //command not recognized
        if (send(new_fd, "I didn't recognize your command. Check yo spellin!", 51, 0) == -1)
            perror("send");
    }
    else {
      return -1;
    }

  }
  return 1;
}

int session(int new_fd){
  //receive RSA here
  char buffer[30];
  std::chrono::time_point<std::chrono::system_clock> now;
  std::chrono::duration<double, std::ratio<1, 1>> duration;
  while(1){
    bzero(buffer,30);
    int n = read(new_fd,buffer,30);
    if (n > 0){
      if (n > 7 && strncmp("login[\n", buffer, 6) == 0){
        char username[n-7];
        strncpy(username,buffer+ 6, n-7);
        printf("%s\n",username);
        //if user in users list
        std::map<std::string, userInfo>::iterator it;
        std::string usr(username);
        it = users.find(usr);
        if(it != users.end()){
          if (send(new_fd, "Please enter your pin, friend", 31, 0) == -1)
              perror("send");
          while(1){
            bzero(buffer,30);
            int n = read(new_fd,buffer,30);
						now = std::chrono::system_clock::now();
						duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - last_reset);
						if (duration.count() > 1200){
							reset_attempts();
						}
						//attempts check
						pthread_mutex_lock(&attempts_lock);
						if (attempts > 4){
							if (send(new_fd, "Too many bad pin attempts. Please try again in 20 minutes. get_rect()", 70, 0) == -1)
									perror("send");
							return 0;
						}
						pthread_mutex_unlock(&attempts_lock);
            if (n > 0){
              //check if pins are the same
              if (it->second.get_pin() == atoi(buffer)){
                if (send(new_fd, "Logged in. Available commands: balance, withdraw, transfer, logout", 67, 0) == -1)
                    perror("send");
                  if (user_session(new_fd,(it->second)) == -1)
                    return -1;
                  break;
              }
              else {
                if (send(new_fd, "Bad pin", 8, 0) == -1)
                    perror("send");
                    //increment attempt counter
										pthread_mutex_lock(&attempts_lock);
                    attempts++;
										pthread_mutex_unlock(&attempts_lock);
              }
            }
            else {
              return -1;
            }
          }
        }
        else {
          //not a real username
          if (send(new_fd, "The user you have entered does not exist. Maybe they died?", 60, 0) == -1)
              perror("send");
        }
        }
      else {
        if (send(new_fd, "something went wrong. The ATM should use the format login[username]", 68, 0) == -1)
            perror("send");
      }
      if (send(new_fd, "Received Message", 17, 0) == -1)
          perror("send");
    }
    else {
      return -1;
    }

}
    return 1;
}



void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void users_init(){
	pthread_mutex_lock(&users_lock);
  std::string line;
  std::ifstream myfile ("bankinit.txt");
  if (myfile.is_open())
  {
    std::vector<std::string> line_info;
	int pin;
	int balance;
    while ( getline (myfile,line) )
    {
      line_info = split(line, ' ');
      userInfo new_user;
	  pin = atoi(line_info[2].c_str());
	  balance = atoi(line_info[1].c_str());
      new_user.init(pin,balance);
      users.insert(std::pair<std::string, userInfo>(line_info[0],new_user));
	  //std::cout << line_info[0] << pin << balance;
    }
    myfile.close();
  }
  else std::cout << "Unable to open file";
	pthread_mutex_unlock(&users_lock);
}

void *connection_handler(void* input){
	int new_fd =  *((int *)input);
	session(new_fd);

	printf("closed connection\n");
	return 0;
}
void *admin_panel(void*){
	printf("Welcome Admin\n");
	std::string input;
	while(1){
		std::cin >> input;
		if( "balance[" == input.substr(0,8)){
			//char username[input.length() - 9];
			std::string username = input.substr(8,input.length()-9);
			std::cout << username << std::endl;
			std::map<std::string, userInfo>::iterator it;
			it = users.find(username);
			if(it != users.end()){
			pthread_mutex_lock(&users_lock);
			userInfo &user2 = it->second;
			std::cout << "balance: " + std::to_string(user2.get_balance()) << std::endl;
			pthread_mutex_unlock(&users_lock);
			continue;
		}
		else {
			std::cout << "That user doesn't exist dude!" << std::endl;
			continue;
		}
	}
	else if(( "deposit[" == input.substr(0,8))) {
		int index = input.find(']');
		std::string username = input.substr(8,index - 8);
		std::string amount = input.substr(index + 2,input.length() - index - 3);
		std::cout << "username " + username  + " amount " + amount << std::endl;
		std::map<std::string, userInfo>::iterator it;
		it = users.find(username);
		if(it != users.end()){
			userInfo &user = it->second;
			int error = user.error_check(std::stoi(amount));
			if (error > 0){
				std::cout << "That user has too much mulah. Please have them create a new account" << std::endl;
				continue;
			}
			pthread_mutex_lock(&users_lock);
			user.add_balance(std::stoi(amount));
			std::cout << "balance: " + std::to_string(user.get_balance()) << std::endl;
			pthread_mutex_unlock(&users_lock);
			continue;
	}
	else{
		std::cout << "That user doesn't exist dude!" << std::endl;

		}
	}
	std::cout << "invalid command" << std::endl;
}
	return 0;
}

int main(int argc , char *argv[])
{

    if (argc !=  2){
      std::cout << "bad arguments" << std::endl;
      return 1;
    }
	last_reset = std::chrono::system_clock::now();
	users_init();
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
		pthread_t thread_id;
		if( pthread_create( &thread_id , NULL ,  admin_panel, 0) < 0)
			{
					perror("failed to create thread");
					return 1;
			}
    while(1) {  // main accept() loop

        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
				pthread_t thread_id2;
				if( pthread_create( &thread_id2 , NULL ,  connection_handler , (void*) &new_fd) < 0)
	        {
	            perror("failed to create thread");
	            return 1;
	        }
        //close(new_fd);  // parent doesn't need this
    }

    return 0;
}


//add encrypted wrapper for this
  //send(new_fd, "Your balance is too high with the new number. Start a new bank account!", 72, 0) == -1)
  //the above message is 71 chars
  //dont forget that you should encrypt the 71 chars and then add 1 to the length of your ciphertext

  //    int n = read(new_fd,buffer,30);
  // add decrypter wrapper for this where it returns the amount of chars excluding the null char at the end and puts the decrypted text in buffer
