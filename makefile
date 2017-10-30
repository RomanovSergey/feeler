################################################################################
# Makefile for Release Build
# Author: Romanov S.V.
# Date: august, 2016
################################################################################

CPU := cortex-m0
INSTRUCTION_MODE := thumb
TARGET := feeler
TARGET_EXT := elf
LD_SCRIPT := ./linker/stm32f0_linker.ld

#LIBS := ../../libSTM32F103/Debug/libstm32f103.a

INCLUDE += -I ./Libraries/CMSIS/Include/
INCLUDE += -I ./Libraries/CMSIS/STM32F0xx/Include/
INCLUDE += -I ./Libraries/STM32F0xx_StdPeriph_Driver/inc/
INCLUDE += -I ./src/
#INCLUDE += -I "c:\Program Files (x86)\CodeSourcery\Sourcery_CodeBench_Lite_for_ARM_EABI\lib\gcc\arm-none-eabi\4.7.3\include"

OBJ_FOLDER := objs

COMPILER_OPTIONS = -O3 -Os -Wall -fno-strict-aliasing -std=c99 \
	-fmessage-length=0 -fno-builtin  -m$(INSTRUCTION_MODE) \
	-mcpu=$(CPU) -MMD -MP -ffunction-sections -fdata-sections
	
DEPEND_OPTS = -MF $(OBJ_FOLDER)/$(patsubst %.o,%.d,$(notdir $@)) \
	-MT $(OBJ_FOLDER)/$(patsubst %.o,%.d,$(notdir $@))

CC = arm-none-eabi-gcc
CFLAGS = $(COMPILER_OPTIONS) $(INCLUDE) $(DEPEND_OPTS) -c

CXX = arm-none-eabi-g++
CXXFLAGS = $(COMPILER_OPTIONS) $(INCLUDE) $(DEPEND_OPTS) -c
AS = arm-none-eabi-gcc
ASFLAGS = $(COMPILER_OPTIONS) $(INCLUDE) $(DEPEND_OPTS)  -c

LD = arm-none-eabi-gcc
LD_OPTIONS = -Wl,--gc-sections,-Map=$(TARGET).map $(COMPILER_OPTIONS)  -L ../ -T $(LD_SCRIPT) $(INCLUDE)

OBJCP = arm-none-eabi-objcopy
OBJCPFLAGS = -O ihex

AR = arm-none-eabi-ar
ARFLAGS = cr

RM := rm -rf

USER_OBJS :=
C_SRCS := 
S_SRCS := 
C_OBJS :=
S_OBJS :=

# Every subdirectory with source files must be described here
SUBDIRS := ./src
SUBDIRS += ./src/fonts
SUBDIRS += ./startup
SUBDIRS += ./Libraries/CMSIS/STM32F0xx/Source/
SUBDIRS += ./Libraries/STM32F0xx_StdPeriph_Driver/src/

C_SRCS := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
C_OBJS := $(patsubst %.c,$(OBJ_FOLDER)/%.o,$(notdir $(C_SRCS)))
S_SRCS := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.s))
S_OBJS := $(patsubst %.s,$(OBJ_FOLDER)/%.o,$(notdir $(S_SRCS)))

VPATH := $(SUBDIRS)

$(OBJ_FOLDER)/%.o : %.c
	@echo "Building file: $(@F)"
	@echo "Invoking: MCU C Compiler"
	$(CC) $(CFLAGS) $< -o $@
	@echo "Finished building: $(@F)"
	@echo " "
	
$(OBJ_FOLDER)/%.o : %.s
	@echo "Building file: $(@F)"
	@echo "Invoking: MCU Assembler"
	$(AS) $(ASFLAGS) $< -o $@
	@echo "Finished building: $(@F)"
	@echo " "

# All Target
all: $(TARGET).$(TARGET_EXT)

# Tool invocations
$(TARGET).$(TARGET_EXT): $(C_OBJS) $(S_OBJS)
	@echo "Building target: $@"
	@echo "Invoking: MCU Linker"
	$(LD) $(LD_OPTIONS) $(C_OBJS) $(S_OBJS) $(USER_OBJS) $(LIBS) -o$(TARGET).$(TARGET_EXT)
	@echo "Finished building target: $@"
	@echo " "
	$(MAKE) --no-print-directory post-build


erase_stlink:
	st-flash erase 40

flash_stlink: $(TARGET).bin
	st-flash write $(TARGET).bin 0x8000000



# Other Targets
clean:
	-$(RM) $(TARGET).$(TARGET_EXT) $(TARGET).hex $(TARGET).bin $(TARGET).map $(OBJ_FOLDER)/*.*
	-@echo " "

post-build:
	-@echo "Performing post-build steps"
	arm-none-eabi-size -t $(TARGET).$(TARGET_EXT)
	arm-none-eabi-objcopy $(OBJCPFLAGS) $(TARGET).$(TARGET_EXT) $(TARGET).hex
	arm-none-eabi-objcopy -O binary -S  $(TARGET).$(TARGET_EXT) $(TARGET).bin
	-@echo " "

.PHONY: all clean dependents
.SECONDARY: post-build

