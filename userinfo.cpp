#include <iostream>
#include <cstdlib>
#include <climits>
#ifndef USERINFO_H
#define USERINFO_H

class userInfo{
	int pin, balance;
  public:
	void init(int, int);
	int add_balance(int);
	int error_check(int);
	int get_balance();
	int get_pin();
};

void userInfo::init(int p, int bal){
	pin = p;
	balance = bal;
}



int userInfo::get_balance(){
	return balance;
}

int userInfo::error_check(int input){
	long int a = input + balance;
	if (a > INT_MAX)
		return 1;
	if (a < 0)
		return -1;
	return 0;
}

int userInfo::add_balance(int input){
	int temp = error_check(input);
	if (temp == 0){
		balance+=input;
		return 0;
	}
	else
	{
		return temp;
	}
}

int userInfo::get_pin(){
	return pin;
}

#endif
