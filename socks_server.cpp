#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include<unordered_map>
#include<algorithm>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<boost/asio.hpp>
#include<boost/algorithm/string/split.hpp> // boost split
#include<boost/algorithm/string/classification.hpp> // is_any_of

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

// declare global io_context !!
io_context io_context_;

class CheckInfo : public enable_shared_from_this<CheckInfo>{
public:
    CheckInfo(tcp::socket socket)
        : socket_(move(socket)){}

    void start(){ 
        get_packet_info();
    }

private:
    tcp::socket socket_;
    enum { max_length = 1025 };
    unsigned char data_[max_length];

    void get_packet_info(){
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, size_t length){
                if (!ec){
                    unsigned char vn = data_[0];
                    unsigned char cd = data_[1];

                    bool legitimate = false;
                    ifstream ifs("./socks.conf");
                    string tmp;
                    while(getline(ifs, tmp)){
                        // get firewall config
                        vector<string> info = {};
                        istringstream is(tmp);
                        string _tmp;
                        while(is >> _tmp){
                            info.push_back(_tmp);
                        }
                        // get destination address
                        vector<string> ip = {};
                        size_t start, end = 0;
                        start = info[2].find_first_not_of('.', end);
                        while(start != string::npos){
                            end = info[2].find_first_of('.', start);
                            ip.push_back(info[2].substr(start,end-start));
                            start = info[2].find_first_not_of('.', end);
                        }
                        if((cd == 1 && info[1] == "c") || (cd == 2 && info[1] == "b")){
                            // check the firewall info
                            if((data_[4] == (unsigned char)atoi(ip[0].c_str()) || ip[0] == "*") && (data_[5] == (unsigned char)atoi(ip[1].c_str()) || ip[1] == "*") && (data_[6] == (unsigned char)atoi(ip[2].c_str()) || ip[2] == "*") && (data_[7] == (unsigned char)atoi(ip[3].c_str()) || ip[3] == "*")){
                                legitimate = true;
                                break;
                            }
                        }
                    }

                    // print info of connect
                    printInfo(cd, legitimate);

                    // do function
                    if(legitimate == 1){
                        shared_ptr<tcp::socket> socket_ptr = make_shared<ip::tcp::socket>(move(_socket));
                        if(cd == 1){
                            make_shared<ServerConnect>(socket_ptr, data_, io_context_)->start();
                        }
                        else{
                            make_shared<ServerBind>(socket_ptr, data_, io_context_)->start();
                        }
                    }
                    else{
                        // send reject message back to console.cgi
                        unsigned char reject_data[8];
                        reject_data[0] = 0;
                        reject_data[1] = 0x5b;
                        reject_data[2] = data_[2];
                        reject_data[3] = data_[3];
                        reject_data[4] = data_[4];
                        reject_data[5] = data_[5];
                        reject_data[6] = data_[6];
                        reject_data[7] = data_[7];
                        socket_.async_send(buffer(reject_data, 8),[self](boost::system::error_code ec, std::size_t length){
                            if(!ec){

                            }
                        });
                        get_packet_info()
                    }
                }
            }
        );
    }

    void printInfo(unsigned char _cd, bool _leg){
        cout<<"<S_IP>: "<<socket_.remote_endpoint().address()<<endl;
        cout<<"<S_PORT>: "<<socket_.remote_endpoint().port()<<endl;
        cout<<"<D_IP>: "<<(int)data_[4]<<"."<<(int)data_[5]<<"."<<(int)data_[6]<<"."<<(int)data_[7]<<endl;
        cout<<"<D_PORT>: "<<((int)data_[2]*(0x100)+(int)data_[3])<<endl;
        cout<<"<Command>: ";
        if(_cd == 1){
            cout<<"CONNECT"<<endl;
        }
        else{
            cout<<"BIND"<<endl;
        }
        cout<<"<Reply>: "<<endl;
        if(_leg == true){
            cout<<"Accept"<<endl;
        }
        else{
            cout<<"Reject"<<endl;
        }
    }

class server{
private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    void do_accept(){
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec){
            if(!ec){
                io_context_.notify_fork(boost::asio::io_service::fork_prepare);
                pid_t _pid = fork();
                // child process
                if(_pid == 0){
                    io_context_.notify_fork(boost::asio::io_service::fork_child);
                    // close socks server to accept another client
                    acceptor_.close();
                    make_shared<CheckInfo>(move(socket_))->start();
                }
                // parent process
                else{
                    io_context_.notify_fork(boost::asio::io_service::fork_parent);
                    socket_.close();
                }
            }
            do_accept();
        });
    }

public:
    server(short port)
    : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), socket_(io_context_){
        do_accept();
    }
};

int main(int argc, char* argv[]){

    if(argc != 2){
        cerr<<"Error: ./socks_server [port]"<<endl;
        exit(1);
    }
    int port = atoi(argv[1]);
    
    server s(port);
    // server s(io_context, port);
    io_context_.run();

    return 0;
}