CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
LDLIBS += -lcrypto -lm
SRC = src/json.c src/policy.c src/governance.c src/experiments.c src/main.c

.PHONY: all demo test clean sanitize
all: nerp
nerp: $(SRC) include/json.h include/nerp.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)
demo: nerp
	./nerp demo data config.json run
test: nerp
	python3 tests/integration.py ./nerp
sanitize:
	$(CC) $(CPPFLAGS) -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(SRC) -o nerp_sanitized $(LDLIBS)
	ASAN_OPTIONS=detect_leaks=0 python3 tests/integration.py ./nerp_sanitized
clean:
	rm -f nerp nerp_sanitized
