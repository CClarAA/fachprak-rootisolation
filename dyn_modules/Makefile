CC = /usr/bin/gcc
ALL = interval.so

.PHONY = all

all: $(ALL)

clean:
	rm -v $(ALL)

interval.so: interval.cc
	$(CC) -shared -DDYNAMIC_VERSION -fPIC `libsingular-config --cflags` $^ -o $@
