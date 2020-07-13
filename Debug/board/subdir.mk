################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_LPC54608J512ET180=1 -DCPU_LPC54608J512ET180_cm4 -D_DEBUG=1 -DCPU_LPC54608 -D__USE_CMSIS -DUSB_STACK_BM -DUSB_STACK_USE_DEDICATED_RAM=1 -DFSL_OSA_BM_TASK_ENABLE=0 -DFSL_OSA_BM_TIMER_CONFIG=0 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -DDEBUG -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\board" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\source" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\usb\host" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\usb\include" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\component\osa" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\drivers" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\device" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\CMSIS" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\component\lists" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\usb\host\class" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\utilities" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\component\serial_manager" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\component\uart" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\sdmmc\inc" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\fatfs\source" -I"C:\Users\embli\Documents\MCUXpressoIDE_11.1.0_3209\workspace\lpcxpresso54608_host_hid_mouse_keyboard_bm\fatfs\source\fsl_sd_disk" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


