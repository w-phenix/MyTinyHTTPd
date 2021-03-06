# MyTinyHTTPd
基于TinyHTTPd改写的自己的HttpWebServer，把所有方法封装成了两个类：HttpWebServer和HttpMethod

**1、测试**

（1）确保`register.cgi`、`color.cgi`、`check.cgi`具有可执行权限

```bash
sudo chmod +x register.cgi、color.cgi、check.cgi
```

（2）编译

```bash
cd MyTinyHTTPd
make
```

（3）运行

```bash
./HttpWebServer
```

默认端口号为30405，可以在`Server.cpp`中修改端口号。服务器开启后，在浏览器中输入：

```
http://localhost:30405
```

即可看到网页，默认为`index.html`。可通过在`HttpMethod.cpp`中修改宏`DEFAULT_URL`，更换默认网页。

ps：htdocs中只有register.html和index.html，若要修改为其他网页，可以自己写了放进去。

**2、类**

（1）HttpWebServer

服务器的主要部分，负责初始化服务器，连接客户端，处理客户端请求等。执行顺序为：

```
InitServer()->Accept()->HandleRequest()
```

（2）HttpMethod

将Http请求方法剥离出来，方便之后对其进行扩展。设计了一个父类`HttpMethod`，两个子类`Get`和`Post`继承自`HttpMethod`（目前只实现了GET和POST两种方法）。

使用了简单工厂模式，在`HttpWebServer`中根据报头中的请求方法获取相应的方法类，之后再实现其他方法时则不用对`HttpWebServer`类进行修改了。

`HttpMethod`的执行顺序为：

```
GetURL()->DiscardHeader()->ExecuteRequest()->ServeFile()/ExecuteCGI()
```

（3）SimpleClient

一个简单的客户端。

**3、其他**

（1）Server.cpp

使用封装的`HttpWebServer`实现了一个服务器的处理过程，是编译后的服务器的执行入口。

（2）Client.cpp

使用封装的`SimpleClient`实现了一个客户端请求服务的过程，是编译后的客户端的执行入口。

如果要使用这个客户端而不是浏览器对服务器进行测试，需要先进行编译：

```
g++ -o client Client.cpp SimpleClient.cpp
```
