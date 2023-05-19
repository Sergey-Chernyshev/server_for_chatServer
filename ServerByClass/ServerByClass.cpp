#include <iostream>
#include <WS2tcpip.h>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <thread>
//#include <chrono>
#include <algorithm>
#include <future>

#pragma comment (lib, "ws2_32.lib")
//using json = nlohmann::json;
using namespace std;
//#include <nlohmann/json.hpp>

//atomic_bool stopThread(false);

ostream& operator<<(std::ostream& os, const sockaddr_in& addr)
{
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
	os << ip << ":" << ntohs(addr.sin_port);
	return os;
}




class User
{
private:
	SOCKET socket;
	int ID;
	string ip;
	string username;
	vector<SOCKET> friends_sock;
	vector<SOCKET> friends_sock_wait;
	vector<string> friends_name;

	vector<string> friends_name_wait;

	map<string, SOCKET> map_friends_sock;
	//map<string, SOCKET> map_friends_sock_wait;


public:
	void set_Socket(const SOCKET sock) {
		socket = sock;
	}
	void set_id(const int id) {
		ID = id;
	}
	void set_name(const string name) {
		username = name;
	}

	void send_message_admin(const string message_from_admin) {
		send(socket, message_from_admin.c_str(), message_from_admin.size() + 1, 0);
	}

	void send_message_to_friend(string message_to_friend) {
		for (const auto& element : this->get_friend_map()) {
			SOCKET send_to = element.second;
			string name = element.first;
			cout << "from: " << name << " > " << message_to_friend << " " << to_string(this->socket);
			string mess = "from: " + name + " > " + message_to_friend;
			//string message_to_friend = "from: " + name + " > " + message_to_friend + to_string(this->socket);
			//string message_to_friend = "from:" + name + " > " message_to_friend;
			//cout << message_to_friend.size() <<  endl;
			send(send_to, mess.c_str(), mess.size() + 1, 0);


		}
	}
	//void send_message_to_friend_by_name(vector<User*>users, string mess) {
	//	for (int i = 0; i < friends_name.size(); i++)
	//	{
	//		string name = friends_name[i];
	//		SOCKET _sock = _GetCurrentSocketByName(name, users)->get_socket();
	//		send(_sock, mess.c_str(), mess.size() + 1, 0);
	//	}
	//}

	void get_ip(string ip_) {
		ip = ip_;
	}


	void add_friend_by_socket(SOCKET sock_f) {
		friends_sock.push_back(sock_f);
	}

	void add_friends_by_sock_wait(SOCKET sock_f) {
		friends_sock_wait.push_back(sock_f);
	}

	void move_from_waitFriend(SOCKET sock_f) {
		friends_sock.push_back(sock_f);
		friends_sock_wait.erase(std::remove_if(friends_sock_wait.begin(), friends_sock_wait.end(), [&sock_f](SOCKET x) { return x == sock_f; }), friends_sock_wait.end());
	}

	void add_friend_by_name(string name, SOCKET sock) {
		//friends_name.push_back(name);
		map_friends_sock.insert(make_pair(name, sock));
	}
	void add_friend_by_name_wait(string name) {
		friends_name_wait.push_back(name);
	}

	void clear_friends() {
		friends_sock.clear();
		friends_name.clear();

		map_friends_sock.clear();
	}

	int check_friend_on_wait(string name) {
		auto it = std::find(friends_name_wait.begin(), friends_name_wait.end(), name);
		if (it != friends_name_wait.end()) {
			return 1;
		}
		return 0;
	}

	vector<SOCKET> get_friend_by_socket() {
		return friends_sock;
	}

	map<string, SOCKET> get_friend_map() {
		cout << map_friends_sock.size();
		return map_friends_sock;
	}

	SOCKET get_socket() {
		return socket;
	}
	string get_name() {
		return username;
	}

	string get_ip() {
		return ip;
	}
};






int _IssetCurrentSocket(SOCKET sock, vector<User*> all_users) {
	for (int i = 0; i < all_users.size(); i++)
	{
		if (all_users[i]->get_socket() == sock) {
			return 1;
		}
	}
	return 0;
}

User* _GetCurrentSocketBySock(SOCKET sock, vector<User*>& all_users) {
	for (int i = 0; i < all_users.size(); i++)
	{
		if (all_users[i]->get_socket() == sock) {
			return all_users[i];
		}
	}
	return  nullptr;
}

User* _GetCurrentSocketByName(string name, vector<User*>& all_users) {
	for (int i = 0; i < all_users.size(); i++)
	{
		if (all_users[i]->get_name() == name) {
			return all_users[i];
		}
	}
	return  nullptr;
}

int _CheckUsernameOnAcess(string username, vector<User*>& all_users) {
	for (int i = 0; i < all_users.size(); i++)
	{
		if (all_users[i]->get_name() == username) {
			return 1;
		}
	}
	return 0;
}

User* _CheckOnWaitFriend(string name, vector<User*>& all_users) {
	for (int i = 0; i < all_users.size(); i++)
	{
		if (all_users[i]->check_friend_on_wait(name)) {
			return all_users[i];
		}
	}
	return nullptr;
}


class TCPServer {

private:
	WSADATA wsData;
	WORD ver;
	int ws0k;
	SOCKET listening;

	sockaddr_in hint;


	fd_set master;

	vector<User*> users;
	vector<SOCKET> waitList;

	fd_set m_MasterSet;
	SOCKET m_MaxSocket;

	//flags
	bool consoleInput;
	future<void> consoleInputLoop;

	//settings
	int maxConnections = SOMAXCONN; // by default
	int port = 54000; 
	string serverIp;

public:

	TCPServer() {

		// console
		consoleInput = true;
		consoleInputLoop = async(launch::async, &TCPServer::ConsoleInputCkeck, this);

		// инициализируем WinSock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);
		int ws0k = WSAStartup(ver, &wsData);
		if (ws0k != 0) {
			std::cerr << "Can't init WinSock, quitting" << std::endl;
			exit(1);
		}

		// создаем сокет для прослушивания входящих подключений
		listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listening == INVALID_SOCKET) {
			std::cerr << "Can't create listening socket, quitting" << std::endl;
			exit(1);
		}
	}

	void ConsoleInputCkeck() {
		while (consoleInput)
		{
			string input;
			getline(cin, input);
			this->ConsoleSettingInput(input);
		}
	}

	void ConsoleSettingInput(string input) {
		cout << "ENTERED: " << input << endl;

		istringstream require(input);
		vector <string> require_words;
		string word;

		while (require >> word)
		{
			require_words.push_back(word);
		}

		if (require_words[0][0] == '/')
		{
			if (require_words[0] == "/cl_con")
			{
				this->disconectAllCliensts();
			}
		}
	}

	bool Bind(const std::string& ipAddress, int port) {
		// создаем структуру для привязки адреса и порта
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(port);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

		// привязываем адрес и порт к сокету
		if (::bind(listening, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
			std::cerr << "Can't bind to IP/port, quitting" << std::endl;
			return false;
		}

		return true;
	}

	bool Listen() {
		// переводим сокет в режим прослушивания входящих подключений
		if (listen(listening, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Can't listen on socket, quitting" << std::endl;
				
		}

		std::cout << "Server is listening on socket " << listening << std::endl;

		// добавляем сокет для прослушивания в множество сокетов
		FD_ZERO(&master);
		FD_SET(listening, &master);
		m_MaxSocket = listening;

		return true;
	}


	~TCPServer() {
		this->stop();
	}


	// остановка сервера 
	void stop() {
		closesocket(listening);
		
		// отключение всех сокетов
		for (auto clientSocket: users)
		{
			closesocket(clientSocket->get_socket());
		}

		WSACleanup();
	}

	//отключение опредленного client по сокету
	void disconectCurrentClientBySocket(const SOCKET sock) {
		if (_IssetCurrentSocket(sock, users))
		{
			closesocket(sock);
		}
		else
		{
			cerr << "Enter current socket" << endl;
		}
	}

	void disconectCurrentClientByName(const string name) {
		User* clietntToDiconect = _GetCurrentSocketByName(name, users);
		if (clietntToDiconect != nullptr)
		{
			closesocket(clietntToDiconect->get_socket());
		}
		else
		{
			cerr << "Enter current socket" << endl;
		}
	}

	//отключение всех клиентов
	void disconectAllCliensts() {
		for (auto clientSocket : users)
		{
			closesocket(clientSocket->get_socket());
		}
	}


	void Run() {
		while (true)
		{

			fd_set copy = master;

			int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

			for (int i = 0; i < socketCount; i++)
			{
				SOCKET sock = copy.fd_array[i];
				if (sock == listening)
				{
					// acept a new conection
					SOCKET client = accept(listening, nullptr, nullptr);
					cout << "new con" << endl;
					// check in wait list


					//get client`s adress 
					sockaddr_in clientAddr;
					int addrLen = sizeof(clientAddr);
					getpeername(client, (sockaddr*)&clientAddr, &addrLen);

					char addressBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(clientAddr.sin_addr), addressBuffer, INET_ADDRSTRLEN);
					std::stringstream clientIpAdress;
					clientIpAdress << addressBuffer;


					cout << clientIpAdress.str() << endl;

					

					
					User* username = new User;
					users.push_back(username);
					cout << clientAddr << endl;
					username->set_Socket(client);



					// add the new connection to the list of connected clients
					FD_SET(client, &master);
					// send a welcome message to the connected client
					string welcomeMsg = "Welcome to the Awesome Chat Server";
					ostringstream ss;
					ss << "SOCKET #" << client << ":" << welcomeMsg << "\r\n";
					cout << ss.str() << endl;
					string strOut = ss.str();
					send(client, strOut.c_str(), strOut.size() + 1, 0);
					

				}
				else

				{
					char buf[4096];
					ZeroMemory(buf, 4096);

					int byteIn = recv(sock, buf, 4096, 0);

					if (byteIn <= 0)
					{
						cout << "Socket disconected" << sock << endl;
						closesocket(sock);
						FD_CLR(sock, &master);
					}
					else
					{
						User* outUser = nullptr;
						for (int i = 0; i < users.size(); i++)
						{
							if (users[i]->get_socket() == sock)
							{
								outUser = users[i];
								cout << "founded" << endl;
								break;
							}
						}
						if (buf[0] == '@')
						{
							cout << buf;
							istringstream require(buf);
							vector <string> require_words;
							string word;
							while (require >> word)
							{
								require_words.push_back(word);
							}

							string req = require_words[0];
							if (req == "@set_name")
							{
								string name = require_words[1];
								if (!_CheckUsernameOnAcess(name, users))
								{
									outUser->set_name(name);
									outUser->send_message_admin("admin> New username, set...\n");
									outUser->send_message_admin("admin> Your new username: " + outUser->get_name());
									User* toUser = _CheckOnWaitFriend(name, users);
									if (toUser != nullptr)
									{
										cout << "\\\\\\\\\\\/////////" << endl;
										outUser->send_message_admin("admin> An interlocutor is waiting for you in the chat \n");
										outUser->send_message_admin("admin> Sey hi to: " + outUser->get_name());
										outUser->add_friend_by_name(toUser->get_name(), toUser->get_socket());

										toUser->send_message_admin("admin> Your interlocutor was connected to server, you can send him message");
										toUser->send_message_admin("admin> Sey hi to: " + outUser->get_name());
										toUser->add_friend_by_name(outUser->get_name(), outUser->get_socket());
									}
									else
									{
										cout << "not wait" << endl;
									}

								}
								else {
									outUser->send_message_admin("admin> Username is busy, select another, please ...\n");
								}

							}

							if (req == "@set_friends")
							{
								for (int i = 1; i < require_words.size(); i++)
								{
									SOCKET friend_sock_add = stoi(require_words[i]);
									//outUser->clear_friends();
									if (_IssetCurrentSocket(friend_sock_add, users))
									{
										outUser->add_friend_by_socket(friend_sock_add);
										User* friend_add = _GetCurrentSocketBySock(friend_sock_add, users);
										outUser->send_message_admin("@friend_add 1 " + require_words[i]);
										outUser->send_message_admin("admin> New friend added to our room: " + friend_add->get_name());
										friend_add->send_message_admin("admin > New friend added to our room : " + outUser->get_name());
										friend_add->add_friend_by_socket(friend_add->get_socket());
									}
									else {
										// error 0
										outUser->send_message_admin("@friend_add 0 " + require_words[i]);
										outUser->send_message_admin("Ожидаем подключения, данный пользователь не найден, он отправлен в режим ожидания..." + require_words[i]);
										outUser->add_friends_by_sock_wait(friend_sock_add);
									}
								}
							}
							if (req == "@set_f")
							{
								for (int i = 1; i < require_words.size(); i++)
								{
									string name = require_words[i];
									User* friend_add = _GetCurrentSocketByName(require_words[i], users);
									if (friend_add == nullptr)
									{
										cout << "nullptr" << endl;
										outUser->send_message_admin("@friend_add 0 " + require_words[i]);
										outUser->send_message_admin("We are waiting for connection, this user has not been found, he has been sent to standby mode..." + require_words[i]);
										outUser->add_friend_by_name_wait(name);
										continue;
									}
									else
									{
										cout << "not nullptr" << endl;
										SOCKET friend_sock_add = friend_add->get_socket();
										//outUser->clear_friends();

										if (_IssetCurrentSocket(friend_sock_add, users))
										{
											outUser->add_friend_by_name(name, friend_sock_add);
											outUser->send_message_admin("@friend_add 1 " + name);
											outUser->send_message_admin("admin> New friend added to our room: " + friend_add->get_name());
											friend_add->send_message_admin("admin > New friend added to our room : " + outUser->get_name());
											friend_add->add_friend_by_name(outUser->get_name(), outUser->get_socket());
											//cout << "er" << endl;
										}
									}

								}
							}
							if (req == "@get_sockets")
							{
								for (int i = 0; i < users.size(); i++)
								{
									cout << " SOCKET[" << i << "]: " << users[i]->get_socket() << endl;
								}
							}
							if (req == "@get_name")
							{
								outUser->send_message_admin("admin> Your username: " + outUser->get_name());
								break;

							}

							if (req == "@get_f")
							{
								ostringstream mes;
								cout << outUser->get_friend_map().size();
								for (const auto& element : outUser->get_friend_map()) {
									cout << "iter" << endl;
									mes << element.first << ": " << element.second << ", ";
								}
								outUser->send_message_admin("admin> Your friends...\n");
								outUser->send_message_admin(mes.str());
								break;

							}
							cout << "admin message, from:" << sock << " : " << buf << endl;

						}
						else
						{
							string message(buf, byteIn);

							cout << buf << endl;
							outUser->send_message_to_friend(buf);

						}

					}
				}

			}
		}
	}
	
	
	
	
	

};


void main()
{
	


	TCPServer server;
	if (!server.Bind("192.168.1.71", 54000)) {
		cout << "Eror 1" << endl;
	}
	if (!server.Listen()) {
		cout << "Eror 2" << endl;
	}
	server.Run();
	
}