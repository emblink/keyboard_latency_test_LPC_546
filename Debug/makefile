################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include usb/host/class/subdir.mk
-include usb/host/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include sdmmc/src/subdir.mk
-include sdmmc/port/subdir.mk
-include fatfs/source/fsl_sd_disk/subdir.mk
-include fatfs/source/fsl_ram_disk/subdir.mk
-include fatfs/source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/osa/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lpcxpresso54608_host_hid_mouse_keyboard_bm.axf

# Tool invocations
lpcxpresso54608_host_hid_mouse_keyboard_bm.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\libs" -Xlinker -Map="lpcxpresso54608_host_hid_mouse_keyboard_bm.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T lpcxpresso54608_host_hid_mouse_keyboard_bm_Debug.ld -o "lpcxpresso54608_host_hid_mouse_keyboard_bm.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lpcxpresso54608_host_hid_mouse_keyboard_bm.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lpcxpresso54608_host_hid_mouse_keyboard_bm.axf"; # arm-none-eabi-objcopy -v -O binary "lpcxpresso54608_host_hid_mouse_keyboard_bm.axf" "lpcxpresso54608_host_hid_mouse_keyboard_bm.bin" ; # checksum -p LPC54608J512 -d "lpcxpresso54608_host_hid_mouse_keyboard_bm.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
