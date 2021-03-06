EXEC=g++
SOURCE=Server.cpp HttpWebServer.cpp HttpMethod.cpp
TARGET=HttpWebServer

HttpWebServer:$(SOURCE)
	$(EXEC) -o $(TARGET) $(SOURCE)

clean:
	rm -rf $(TARGET)
