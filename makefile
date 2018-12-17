################################################################################
# If:
# src/Main.cpp:21:10: fatal error: GL/freeglut.h: No such file or directory
# #include "GL/freeglut.h"
#
# $> sudo apt-get install freeglut3-dev
#
#
################################################################################

RM := rm -rf

EXECUTABLE := Alta_DxfToGCode_1

# Every subdirectory with source files must be described here
SUBDIRS := \
src \

# Required libraries - linked in call order
LIBS := -lglut -lGL -lGLU

# All of the sources participating in the build are defined here
-include src/subdir.mk
 
# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(EXECUTABLE)

# Tool invocations
$(EXECUTABLE): $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "Alta_DxfToGCode_1" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) Alta_DxfToGCode_1
	-@echo ' '

.PHONY: all clean

install:
	cp $(EXECUTABLE) $(DESTDIR)/usr/local/bin/
	@echo ' '

