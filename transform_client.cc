#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <iostream>
#include <functional>
#include <mutex>

using namespace muduo;
using namespace muduo::net;

std::mutex mu;
int clientnum = 4;
const char* filepath = "./hello";
const int ksize = 1000;


typedef std::shared_ptr<FILE> FILEPTR;
class FileRecvClient{

	public:

		FileRecvClient(EventLoop *loop, const InetAddress& serverAddr, const string& id)			//这里根据io线程==写的接受文件的客户端
					:loop_(loop), client_(loop, serverAddr, id)
			{
				client_.setConnectionCallback(std::bind(&FileRecvClient::onConnection,this, std::placeholders::_1));
				client_.setMessageCallback(std::bind(&FileRecvClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			}

		void connect()
		{
			client_.connect();
		}



	private:
	void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime)
	{
		FILEPTR ofptr = boost::any_cast<FILEPTR>(conn->getContext());
	 	char buf[ksize];
		int n =  fwrite(buffer->peek(), 1, buffer->readableBytes(), ofptr.get());			//将buf里面的东西读出来

		buffer->retrieveAll();
		std::cout<<n<<std::endl;

	 	if(buffer->readableBytes() == 0)
	 	{
	 		mu.lock();
	 		clientnum--;
	 		if(clientnum == 0)
	 			exit(0);
	 		mu.unlock();

	 		client_.disconnect();
	 	}
	}

	void onConnection(const TcpConnectionPtr& con)
	{
		string filename = string(filepath)+client_.name();
		FILE *fp  = fopen(&*filename.begin(), "wt+");

		if(con->connected())
		{
			conn = client_.connection();
			if(fp == NULL)
			{
				fclose(fp);
				client_.disconnect();
			}	
			else
			{
				FILEPTR ofptr(fp, ::fclose);				//创建一个智能指针
				conn->setContext(ofptr);				//将conn的这块区域绑定智能指针，使之计数+1


			}
		}
	}
		EventLoop *loop_;
		TcpClient client_;
		FILE*		fp_;
		TcpConnectionPtr conn;			
		//这个应该是要用来绑定文件描述符的。。。我猜是这样
};	

int main()
{

	EventLoop loop;				//主io线程
	EventLoopThreadPool pool_(&loop, "threadpool");

	pool_.setThreadNum(2);			//建立两个子io线程
	pool_.start();					//这里启动线程池，使得该线程池创建出两个io线程

	boost::scoped_ptr<FileRecvClient> clients[8];			//这里建立一个client的指针数组
	InetAddress inet("127.0.0.1", 2018);

	auto tempnum = 4;
	for(int i=0 ;i< tempnum; i++)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", i+1);
		clients[i].reset(new FileRecvClient(pool_.getNextLoop(), inet, buf));		
		//这里new 只能重定向，而不能直接括号将里面new的东西放进去，因为创建的时候就已经初始化过了

		//这里坑爹的地方来了，，，scoped_ptr 只要在作用域内是不会释放指针的，也就是说指向的对象是不会调用析构函数的，
		//不调用析构函数意味着无法调用文件指针的close，也就没法写进去，只能通过fflush写，所以这里需要对当前剩余线程数进行判断
		//判断的数要用mutex保护起来，要么使用原子操作，将--和==号包裹在一起，否则会产生 race condition
		clients[i]->connect();
	}
	loop.loop();

	return 0;
}