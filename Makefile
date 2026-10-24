CC = cc
AR = ar
BUILD ?= build
CPPFLAGS += -Iinclude -Isrc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -MMD -MP
LDLIBS += -lglut -lGL -lm

CG_SOURCES := $(wildcard src/cg/*.c)
CG_OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(CG_SOURCES))
APP_SOURCES := src/app/config.c src/app/machine.c src/app/physics.c src/app/theme.c src/app/ui.c src/app/main.c
APP_OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(APP_SOURCES))
DEMO_OBJECT := $(BUILD)/examples/library_demo.o
TEST_OBJECT := $(BUILD)/tests/test_all.o
MACHINE_OBJECTS := $(BUILD)/src/app/config.o $(BUILD)/src/app/machine.o $(BUILD)/src/app/physics.o $(BUILD)/src/app/theme.o

.PHONY: all lib demo run example test sanitize smoke clean
all: $(BUILD)/prize_machine $(BUILD)/library_demo
lib: $(BUILD)/libcg.a
demo: $(BUILD)/library_demo
run: $(BUILD)/prize_machine
	./$(BUILD)/prize_machine
example: $(BUILD)/library_demo
	./$(BUILD)/library_demo
test: $(BUILD)/test_all $(BUILD)/test_alloc
	./$(BUILD)/test_all
	./$(BUILD)/test_alloc
sanitize:
	$(MAKE) BUILD=build/sanitize CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Wpedantic -MMD -MP -fno-omit-frame-pointer -fsanitize=address,undefined' LDFLAGS='-fsanitize=address,undefined' test
smoke: all
	./$(BUILD)/library_demo --smoke-test
	./$(BUILD)/prize_machine --smoke-test

$(BUILD)/libcg.a: $(CG_OBJECTS)
	$(AR) rcs $@ $^
$(BUILD)/prize_machine: $(APP_OBJECTS) $(BUILD)/libcg.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
$(BUILD)/library_demo: $(DEMO_OBJECT) $(BUILD)/libcg.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
$(BUILD)/test_all: $(TEST_OBJECT) $(MACHINE_OBJECTS) $(BUILD)/libcg.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
$(BUILD)/test_alloc: $(BUILD)/tests/test_alloc.o $(MACHINE_OBJECTS) $(BUILD)/libcg.a
	$(CC) $(LDFLAGS) -Wl,--wrap=malloc,--wrap=calloc,--wrap=free -o $@ $^ $(LDLIBS)
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	rm -rf build

-include $(CG_OBJECTS:.o=.d) $(APP_OBJECTS:.o=.d) $(DEMO_OBJECT:.o=.d) $(TEST_OBJECT:.o=.d) $(BUILD)/tests/test_alloc.d
