EXEC=g++
SOURCE=Server.cpp HttpWebServer.cpp HttpMethod.cpp
TARGET=HttpWebServer
LIBRARY=-lpthread

HttpWebServer:$(SOURCE)
	$(EXEC) -o $(TARGET) $(SOURCE) $(LIBRARY)

clean:
	rm -rf $(TARGET)
