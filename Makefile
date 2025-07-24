CC = gcc
CFLAGS = -Wall  -g # -fsanitize=address
LIBS = -pthread -lncursesw -lm

TARGET = quantum
BUILD_DIR = build

SRCS = 	main.c 		\
		inputs.c 	\
		display.c 	\
		chat.c 		\
		listes.c 	\
		client.c 	\
		server.c 	\
		utils.c 	\
		config.c 	\
		map.c		\
		test.c		\
		globals.c	\
		file.c		\

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

IS_WSL := $(shell grep -qi microsoft /proc/version && echo 1 || echo 0)
ifeq ($(IS_WSL),1)
WIN_HOME := $(shell wslpath "$$(powershell.exe '$$Env:USERPROFILE' | tr -d '\r')")
else
WIN_HOME := 
endif
WIN_TOAST := $(WIN_HOME)/toast.ps1

all: check-ncurses get-toast $(TARGET)
	@echo "✅ Build completed successfully!"
	@echo "🚀 You can now launch \033[4;32m./quantum\033[0m"

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "🔧 Compiling $< ..."
	@$(CC) $(CFLAGS) -c $< -o $@ $(LIBS) 

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	@echo "🔨 Linking $(TARGET) ..."
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

prepare:
	@echo "\nYou need to get ncurses first : 'sudo apt install libncurses5-dev libncursesw5-dev'\n"

re: fclean all

clean:
	@echo "🧹 Cleaning objects..."
	@rm -rf $(TARGET) $(BUILD_DIR)

fclean: clean	
	@echo "🧹 Cleaning exe..."
	@rm -f reccord.txt
	@rm -f $(TARGET)

get-toast:
ifeq ($(IS_WSL),1)
	@echo "Ensuring PowerShell execution policy allows toast.ps1"
	@powershell.exe -Command "if ((Get-ExecutionPolicy -Scope CurrentUser) -ne 'RemoteSigned') { Set-ExecutionPolicy RemoteSigned -Scope CurrentUser -Force }"
	@if [ ! -f "$(WIN_HOME)/toast.ps1" ]; then \
		cp ./toast.ps1 "$(WIN_HOME)/"; \
	else \
		echo "toast.ps1 already exists in Windows home. Skipping download."; \
	fi
else
	@echo "Not running in WSL"
endif


check-ncurses:
	@if grep -qE 'Ubuntu|Debian' /etc/os-release; then \
		if ! dpkg -s libncurses-dev >/dev/null 2>&1; then \
			echo "📦 Installing libncurses-dev for Debian/Ubuntu..."; \
			sudo apt update && sudo apt install -y libncurses-dev; \
		else \
			echo "✅ libncurses-dev already installed."; \
		fi \
	else \
		echo "⚠️  Not a Debian-based system — make sure you have ncurses-dev installed."; \
	fi