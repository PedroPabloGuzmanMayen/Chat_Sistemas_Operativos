#include <iostream>
#include <string>
#include <vector>
using namespace std;


struct User {
    string username;
    string ip_addr;
    string status;

    void displayUserInfo(){
        std::cout << "Username: " << username << ", IP: " << ip_addr
        << ", Estado: " << status << std::endl; 
    }

};


struct Chat {

};