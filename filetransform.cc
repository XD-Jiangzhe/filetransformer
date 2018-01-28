#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

typedef std::shared_ptr<FILE>  FILEPTR;
const char *filepath = "../hello.txt";
const int ksize = 10;

void onConnection(const TcpConnectionPtr& conn)
{
	if(conn->connected())
	{
		FILE* pf = fopen(filepath, "rb");
		FILEPTR ifptr(pf, ::fclose);				//这里先声明一个文件指针，然后用shared_ptr来对其进行管理

		char buf[ksize];
		if(ifptr)
		{
			conn->setContext(ifptr);
			int n = fread(buf, 1, ksize, ifptr.get());			//这里应该要要获取智能指针读
			std::cout<<n<<std::endl;
			conn->send(buf, n);
		}
		else 
		{

			LOG_INFO<<"no such file";
		}
	}
	else
	{
		conn->shutdown();
	}
}

void onWriteComplete(const TcpConnectionPtr& conn)
{
	const FILEPTR ifptr1 = boost::any_cast<const FILEPTR&>(conn->getContext());			//将获取到的转成智能指针的形式

	char kbuf[ksize];
	int nread = fread(kbuf, 1, ksize, ifptr1.get());
	if(nread > 0)
	{
		conn->send(kbuf, nread);
	}
	else
	{
		conn->shutdown();
		LOG_INFO<<"file has been send all";
	}

}


int main()
{
	EventLoop loop;
	InetAddress listen_addr(2018);

	TcpServer server_(&loop, listen_addr, "filetransform");
	server_.setConnectionCallback(onConnection);
	server_.setWriteCompleteCallback(onWriteComplete);

	server_.start();
	loop.loop();
}