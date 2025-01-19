TARGET = cb-control

CXX = C:\msys64\mingw64\bin\g++.exe
CXXFLAGS = -g -pedantic -Wall -Wextra -Wno-sign-compare -std=c++20
SRCDIR = src
OBJDIR = obj
LDFLAGS = -g
LDLIBS = -LC:\MinGW\lib -lws2_32
INC=-Isrc

RM = del /f
MKDIR = powershell.exe 'md -Force $(1) | Out-Null'
FIND = powershell.exe 'Get-ChildItem -Filter $(2) -Recurse $(1) | Resolve-Path -Relative |  %{ $$_ -replace "\.\\", "" -replace "\\", "/" }'

SOURCES = $(shell $(call FIND,$(SRCDIR),*.cpp))
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rd /s /q $(OBJDIR)