
CXXFLAGS += -I.
default:
	@echo " ====== use mingw32-make ====="
	mingw64-make all

all: qt-example.exe

qt-example.exe: qt-example.o
	$(CXX) -o $@ $^ -lQtCore4

