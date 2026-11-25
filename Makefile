CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -D_POSIX_C_SOURCE=200809L

TARGET = watchdog
WORKER_TARGET = workers/worker

SRCS = src/main.c src/signal_manager.c src/process_manager.c src/logger.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET) $(WORKER_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(WORKER_TARGET): workers/worker.c
	$(CC) $(CFLAGS) -o $(WORKER_TARGET) workers/worker.c

clean:
	rm -f $(OBJS) $(TARGET) $(WORKER_TARGET) *.log

.PHONY: all clean