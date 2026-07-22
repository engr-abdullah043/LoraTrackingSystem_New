################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_uart.c \
../Core/Src/gpio.c \
../Core/Src/icache.c \
../Core/Src/lora_protocol.c \
../Core/Src/lora_radio.c \
../Core/Src/main.c \
../Core/Src/node_app.c \
../Core/Src/spi.c \
../Core/Src/stm32u5xx_hal_msp.c \
../Core/Src/stm32u5xx_it.c \
../Core/Src/sx1262_board.c \
../Core/Src/sx1262_bringup.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32u5xx.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/app_uart.o \
./Core/Src/gpio.o \
./Core/Src/icache.o \
./Core/Src/lora_protocol.o \
./Core/Src/lora_radio.o \
./Core/Src/main.o \
./Core/Src/node_app.o \
./Core/Src/spi.o \
./Core/Src/stm32u5xx_hal_msp.o \
./Core/Src/stm32u5xx_it.o \
./Core/Src/sx1262_board.o \
./Core/Src/sx1262_bringup.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32u5xx.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/app_uart.d \
./Core/Src/gpio.d \
./Core/Src/icache.d \
./Core/Src/lora_protocol.d \
./Core/Src/lora_radio.d \
./Core/Src/main.d \
./Core/Src/node_app.d \
./Core/Src/spi.d \
./Core/Src/stm32u5xx_hal_msp.d \
./Core/Src/stm32u5xx_it.d \
./Core/Src/sx1262_board.d \
./Core/Src/sx1262_bringup.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32u5xx.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U585xx -c -I../Core/Inc -I../Drivers/SX126x/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_uart.cyclo ./Core/Src/app_uart.d ./Core/Src/app_uart.o ./Core/Src/app_uart.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/icache.cyclo ./Core/Src/icache.d ./Core/Src/icache.o ./Core/Src/icache.su ./Core/Src/lora_protocol.cyclo ./Core/Src/lora_protocol.d ./Core/Src/lora_protocol.o ./Core/Src/lora_protocol.su ./Core/Src/lora_radio.cyclo ./Core/Src/lora_radio.d ./Core/Src/lora_radio.o ./Core/Src/lora_radio.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/node_app.cyclo ./Core/Src/node_app.d ./Core/Src/node_app.o ./Core/Src/node_app.su ./Core/Src/spi.cyclo ./Core/Src/spi.d ./Core/Src/spi.o ./Core/Src/spi.su ./Core/Src/stm32u5xx_hal_msp.cyclo ./Core/Src/stm32u5xx_hal_msp.d ./Core/Src/stm32u5xx_hal_msp.o ./Core/Src/stm32u5xx_hal_msp.su ./Core/Src/stm32u5xx_it.cyclo ./Core/Src/stm32u5xx_it.d ./Core/Src/stm32u5xx_it.o ./Core/Src/stm32u5xx_it.su ./Core/Src/sx1262_board.cyclo ./Core/Src/sx1262_board.d ./Core/Src/sx1262_board.o ./Core/Src/sx1262_board.su ./Core/Src/sx1262_bringup.cyclo ./Core/Src/sx1262_bringup.d ./Core/Src/sx1262_bringup.o ./Core/Src/sx1262_bringup.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32u5xx.cyclo ./Core/Src/system_stm32u5xx.d ./Core/Src/system_stm32u5xx.o ./Core/Src/system_stm32u5xx.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

