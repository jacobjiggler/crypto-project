#include <iostream>
#include <cstdlib>
#ifndef USERINFO_H
#define USERINFO_H

class userInfo{
	int pin, balance;
  public:
	void init(int, int);
	void deposit(int);
	int get_balance();
	int get_pin();
};

void userInfo::init(int p, int bal){
	pin = p;
	balance = bal;
}

void userInfo::deposit(int dep){
	balance = balance + dep;
}

int userInfo::get_balance(){
	return balance;
}

int userInfo::get_pin(){
	return pin;
}

#endif
