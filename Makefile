CC = gcc
RM = rm -f

CFLAGS  = -Wall
LIBS    = -lbcm2835

TARGET = pi_fan_hwpwm

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBS)

install: $(TARGET)
	install $(TARGET) /usr/local/sbin
	cp $(TARGET).service /etc/systemd/system/
	systemctl enable $(TARGET)
	! systemctl is-active --quiet $(TARGET) || systemctl stop $(TARGET)
	systemctl start $(TARGET)

clean:
	$(RM) $(TARGET)
