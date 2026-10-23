# Lesson 23 builds three programs and runs three scenarios:
#
#   bootloader/        -> bootloader/build/firmware.elf  (0x08000000)
#   app/ APP_VERSION=1 -> build/app-v1.img               (header + app)
#   app/ APP_VERSION=2 -> build/app-v2.img
#
#   make run           all scenarios below, one after another
#   make run-valid     app v1 installed: the bootloader starts it
#   make run-corrupt   app v1 with a damaged word: the bootloader refuses it
#   make run-update    v1 installed, v2 staged: install v2, start it, reset, start it again

BUILD := build
BOOTLOADER := bootloader/build/firmware.elf
SCENARIOS := valid corrupt update

.PHONY: all clean run $(addprefix run-,$(SCENARIOS))

all:
	@$(MAKE) -C bootloader
	@$(MAKE) -C app BUILD=build/v1 APP_VERSION=1
	@$(MAKE) -C app BUILD=build/v2 APP_VERSION=2
	@mkdir -p $(BUILD)
	@python3 tools/sign.py app/build/v1/firmware.bin 1 $(BUILD)/app-v1.img
	@python3 tools/sign.py app/build/v2/firmware.bin 2 $(BUILD)/app-v2.img

run: $(addprefix run-,$(SCENARIOS))

$(addprefix run-,$(SCENARIOS)): run-%: all
	@printf '\n===== scenario: %s =====\n' $*
	@STIMULUS=scenario-$*.resc ../common/run.sh $(BOOTLOADER)

clean:
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C app BUILD=build/v1 clean
	@$(MAKE) -C app BUILD=build/v2 clean
	rm -rf $(BUILD)
