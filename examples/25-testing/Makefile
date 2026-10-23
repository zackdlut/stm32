# Lesson 25: the same source files built two ways.
#
#   make            firmware for the STM32 (build/firmware.elf)
#   make run        firmware in Renode with a few typed commands
#   make test       unit tests on the PC with Unity, ASan and UBSan
#   make coverage   unit tests with gcov line coverage of src/
#   make robot      system test of the firmware in Renode (Robot Framework)
#   make ci         everything a CI server runs: test + firmware + robot

UNITY := ../third_party/unity/src

SRCS := firmware/main.c src/ringbuf.c src/baud.c src/cmd.c
CPPFLAGS += -Isrc
RUN_TIME := 0.25

include ../common/cmsis.mk

# ---- host side ------------------------------------------------------------
HOST_CC     := gcc
HOST_BUILD  := build/host
HOST_CFLAGS := -std=c11 -g -O0 -Wall -Wextra -Werror -Isrc -I$(UNITY)
SANITIZE    := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
TESTS       := ringbuf baud cmd
MODULES     := src/ringbuf.c src/baud.c src/cmd.c

.PHONY: test coverage robot ci

$(HOST_BUILD)/test_%: test/test_%.c $(MODULES) $(UNITY)/unity.c
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(EXTRA_HOST_FLAGS) $^ -o $@

test: EXTRA_HOST_FLAGS := $(SANITIZE)
test: $(addprefix $(HOST_BUILD)/test_,$(TESTS))
	@for t in $^; do echo "== $$t"; ./$$t || exit 1; done

COV := build/coverage
coverage:
	@rm -rf $(COV) && mkdir -p $(COV)
	@for m in $(MODULES); do \
	    $(HOST_CC) $(HOST_CFLAGS) --coverage -c $$m -o $(COV)/$$(basename $$m .c).o || exit 1; \
	done
	@for t in $(TESTS); do \
	    $(HOST_CC) $(HOST_CFLAGS) test/test_$$t.c $(UNITY)/unity.c $(COV)/*.o --coverage \
	        -o $(COV)/test_$$t && $(COV)/test_$$t > /dev/null || exit 1; \
	done
	@gcov -n -o $(COV) $(MODULES) | grep -A1 "^File 'src/"

RENODE_TEST ?= renode-test
robot: all
	cd build && $(RENODE_TEST) ../test/firmware.robot

ci: test all robot
