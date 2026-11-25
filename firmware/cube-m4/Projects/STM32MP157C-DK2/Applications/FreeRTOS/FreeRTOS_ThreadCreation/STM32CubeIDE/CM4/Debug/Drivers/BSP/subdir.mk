################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/jim/projects/stm32mp157_openwrt_dev/firmware/cube-m4/Drivers/BSP/STM32MP15xx_DISCO/stm32mp15xx_disco.c 

OBJS += \
./Drivers/BSP/stm32mp15xx_disco.o 

C_DEPS += \
./Drivers/BSP/stm32mp15xx_disco.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/stm32mp15xx_disco.o: /home/jim/projects/stm32mp157_openwrt_dev/firmware/cube-m4/Drivers/BSP/STM32MP15xx_DISCO/stm32mp15xx_disco.c Drivers/BSP/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32MP157Cxx -DCORE_CM4 -DDEBUG -c -I../../../../../../../../Drivers/STM32MP1xx_HAL_Driver/Inc -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../../../../../../../Drivers/CMSIS/Device/ST/STM32MP1xx/Include -I../../../../../../../../Drivers/CMSIS/RTOS2/Include -I../../../../../../../../Utilities/ResourcesManager -I../../../../../../../../Drivers/STM32MP1xx_HAL_Driver/Inc/Legacy -I../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../../../../../../../Drivers/BSP/STM32MP15xx_DISCO -I../../../Inc -I../../../../../../../../Drivers/CMSIS/Include -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -ftls-model=local-exec -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP

clean-Drivers-2f-BSP:
	-$(RM) ./Drivers/BSP/stm32mp15xx_disco.cyclo ./Drivers/BSP/stm32mp15xx_disco.d ./Drivers/BSP/stm32mp15xx_disco.o ./Drivers/BSP/stm32mp15xx_disco.su

.PHONY: clean-Drivers-2f-BSP

