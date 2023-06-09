CC = cl -nologo -c
CXX = cl -nologo -c
LD = cl -nologo

RM = del -F -Q
RMDIR = rmdir -S -Q
MKDIR = mkdir
COPY = xcopy /E /C /I /-I /Q /Y

CFLAGS := -std:c17 -O2
CXXFLAGS := -std:c++20 -EHs -EHc -O2
LDFLAGS := -nologo -subsystem:windows

OBJ_FILES = pipe.obj squatter.obj wincon.obj yaml.obj keys.obj main.obj
LINK_LIBS = user32.lib shell32.lib ole32.lib


all: hyperenable.exe
.PHONY: all clean


%.obj: %.cpp
	@echo Building $@
	$(CXX) $(CXXFLAGS) $< -Fo$@

hyperenable.exe: $(OBJ_FILES)
	@echo Linking $@
	$(LD) $(CXXFLAGS) $(OBJ_FILES) -link $(LDFLAGS) $(LINK_LIBS) -out:$@

clean:
	@-$(RM) *.o 2>nul &
	@-$(RM) *.obj 2>nul &
	@-$(RM) *.exe 2>nul &

