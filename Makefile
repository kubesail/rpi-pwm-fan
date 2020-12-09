# the compiler: gcc for C program, define as g++ for C++
CC = gcc
RM = rm -f

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall
LIBS    = -lbcm2835

# the build target executable:
TARGET = pi_fan_hwpwm

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBS)

install: $(TARGET)
	install $(TARGET) /usr/local/sbin
	cp $(TARGET).service /etc/systemd/system/
	systemctl enable $(TARGET)
	systemctl start $(TARGET)

clean:
	$(RM) $(TARGET)
